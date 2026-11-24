#ifdef ESP32
#include "ManualUpdateReceiver.h"
#include <Arduino.h>
#include <esp_app_format.h>
#include <esp_event.h>
#include <esp_mac.h>
#include <esp_ota_ops.h>
#include <esp_random.h>
#include <esp_wifi.h>
#include <esp_wifi_default.h>
#include <firmware_descriptor.h>
#include <firmware_upload.h>

static_assert(sizeof(esp_image_header_t) == FirmwareUpload::imageHeaderSize);
static_assert(sizeof(esp_image_segment_header_t) == FirmwareUpload::segmentHeaderSize);
static_assert(sizeof(esp_app_desc_t) == FirmwareUpload::appDescSize);
static_assert(offsetof(esp_image_header_t, chip_id) == FirmwareUpload::chipOffset);
static_assert(ESP_IMAGE_HEADER_MAGIC == FirmwareUpload::imageMagic &&
              ESP_APP_DESC_MAGIC_WORD == FirmwareUpload::appDescMagic && ESP_CHIP_ID_ESP32 == 0);

extern "C" __attribute__((section(".rodata_custom_desc"), aligned(4), used))
const FirmwareDescriptor els_hardware_descriptor = makeFirmwareDescriptor(hardware, pulseTiming);

namespace {
constexpr uint32_t armWindowMs = 5 * 60 * 1000, bodyIdleMs = 10 * 1000;

class InactivePartition {
  const esp_partition_t* partition;
  esp_ota_handle_t handle = 0;
  bool writing = false;
public:
  explicit InactivePartition(const esp_partition_t* partition) : partition(partition) {}
  ~InactivePartition() { if (writing) esp_ota_abort(handle); }
  bool begin(size_t length) { return writing = esp_ota_begin(partition, length, &handle) == ESP_OK; }
  bool write(const uint8_t* bytes, size_t count) { return esp_ota_write(handle, bytes, count) == ESP_OK; }
  bool finish() {
    writing = false; // esp_ota_end consumes the handle even when verification fails.
    return esp_ota_end(handle) == ESP_OK;
  }
  bool select() { return esp_ota_set_boot_partition(partition) == ESP_OK; }
};

const char* uploadError(FirmwareUpload::Result result) {
  switch (result) {
    case FirmwareUpload::Result::Installed: return nullptr;
    case FirmwareUpload::Result::TransferFailed: return "Transfer failed";
    case FirmwareUpload::Result::WrongImage: return "Wrong ESP app image";
    case FirmwareUpload::Result::WrongHardware: return "Wrong hardware";
    case FirmwareUpload::Result::EraseFailed: return "Flash open failed";
    case FirmwareUpload::Result::WriteFailed: return "Flash write failed";
    case FirmwareUpload::Result::VerifyFailed: return "Image invalid";
    case FirmwareUpload::Result::SelectFailed: return "Boot slot failed";
    case FirmwareUpload::Result::Cancelled: return "Upload cancelled";
  }
  return "Update failed";
}

bool headerPresent(httpd_req_t* request, const char* name) {
  char value[1];
  return httpd_req_get_hdr_value_str(request, name, value, sizeof(value)) != ESP_ERR_NOT_FOUND;
}
esp_err_t reply(httpd_req_t* request, const char* status, const char* message) {
  const auto type = httpd_resp_set_type(request, "text/plain");
  const auto code = httpd_resp_set_status(request, status);
  const auto connection = httpd_resp_set_hdr(request, "Connection", "close");
  if (type != ESP_OK || code != ESP_OK || connection != ESP_OK) return ESP_FAIL;
  return httpd_resp_sendstr(request, message);
}
esp_err_t reject(httpd_req_t* request, const char* status, const char* message) {
  reply(request, status, message);
  return ESP_FAIL; // Close the connection instead of draining a rejected body.
}
}

bool ManualUpdateReceiver::active() const {
  return !cancelled.load(std::memory_order_acquire) && uint32_t(millis() - openedAt) < armWindowMs;
}

bool ManualUpdateReceiver::startAp(UpdateStatus& status) {
  if (esp_netif_init() != ESP_OK) return false;
  const auto events = esp_event_loop_create_default();
  if (events != ESP_OK && events != ESP_ERR_INVALID_STATE) return false;
  const esp_netif_config_t interface = ESP_NETIF_DEFAULT_WIFI_AP();
  ap = esp_netif_new(&interface);
  if (!ap || esp_netif_attach_wifi_ap(ap) != ESP_OK || esp_wifi_set_default_wifi_ap_handlers() != ESP_OK)
    return false;

  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  init.nvs_enable = 0;
  if (esp_wifi_init(&init) != ESP_OK) return false;
  wifiInitialized = true;
  // Start the radio for entropy without a STA netif, credentials, connect or
  // reconnect handler. Do not enable the AP until its WPA2 config is set.
  if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK ||
      esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK || esp_wifi_start() != ESP_OK) return false;
  wifiRunning = true;
  uint8_t random[16], mac[6];
  esp_fill_random(random, sizeof(random));
  if (esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP) != ESP_OK) return false;
  if (esp_wifi_stop() != ESP_OK) return false;
  wifiRunning = false;

  std::snprintf(status.network, sizeof(status.network), "ELS-%02X%02X%02X", mac[3], mac[4], mac[5]);
  constexpr char hex[] = "0123456789abcdef";
  for (unsigned i = 0; i < sizeof(random); ++i) {
    status.key[2*i] = hex[random[i] >> 4];
    status.key[2*i+1] = hex[random[i] & 15];
  }
  wifi_config_t config{};
  config.ap.ssid_len = std::strlen(status.network);
  std::memcpy(config.ap.ssid, status.network, config.ap.ssid_len);
  std::memcpy(config.ap.password, status.key, sizeof(status.key));
  config.ap.channel = 1;
  config.ap.authmode = WIFI_AUTH_WPA2_PSK;
  config.ap.max_connection = 1;
  if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK ||
      esp_wifi_set_config(WIFI_IF_AP, &config) != ESP_OK || esp_wifi_start() != ESP_OK) return false;
  wifiRunning = true;

  esp_netif_ip_info_t ip{};
  if (esp_netif_get_ip_info(ap, &ip) != ESP_OK || !ip.ip.addr) return false;
  std::snprintf(status.address, sizeof(status.address), IPSTR, IP2STR(&ip.ip));
  return true;
}

bool ManualUpdateReceiver::shutdown() {
  bool ok = true;
  if (server) {
    ok = httpd_stop(server) == ESP_OK;
    server = nullptr;
  }
  if (wifiRunning) {
    ok = esp_wifi_stop() == ESP_OK && ok;
    wifiRunning = false;
  }
  if (wifiInitialized) {
    ok = esp_wifi_deinit() == ESP_OK && ok;
    wifiInitialized = false;
  }
  if (ap) {
    esp_netif_destroy_default_wifi(ap);
    ap = nullptr;
  }
  return ok;
}
void ManualUpdateReceiver::publishFailure(const char* detail) {
  UpdateStatus status{UpdatePhase::Failed};
  status.detail = detail;
  state.publishUpdate(status);
}
void ManualUpdateReceiver::arm() {
  UpdateStatus status{UpdatePhase::Waiting};
  if (!startAp(status)) {
    publishFailure(shutdown() ? "Wi-Fi start failed" : "Wi-Fi stop failed");
    return;
  }
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_open_sockets = 1;
  config.core_id = 0; // The motion task is parked; keep core 1 available to the display.
  config.max_uri_handlers = 1;
  config.recv_wait_timeout = 1;
  config.send_wait_timeout = 5;
  config.stack_size = 6144;
  const httpd_uri_t uri = {"/firmware", HTTP_PUT, [](httpd_req_t* request) {
    return static_cast<ManualUpdateReceiver*>(request->user_ctx)->upload(request);
  }, this};
  openedAt = millis();
  if (httpd_start(&server, &config) != ESP_OK || httpd_register_uri_handler(server, &uri) != ESP_OK) {
    publishFailure(shutdown() ? "HTTP start failed" : "Receiver stop failed");
    return;
  }
  state.publishUpdate(status);
}

esp_err_t ManualUpdateReceiver::upload(httpd_req_t* request) {
  if (!active() || uploadClaimed) return reject(request, "409 Conflict", "Receiver unavailable");
  if (headerPresent(request, "Origin") || headerPresent(request, "Transfer-Encoding"))
    return reject(request, "400 Bad Request", "Send a raw PUT without Origin or Transfer-Encoding");
  char type[25];
  if (httpd_req_get_hdr_value_str(request, "Content-Type", type, sizeof(type)) != ESP_OK ||
      std::strcmp(type, "application/octet-stream") != 0)
    return reject(request, "415 Unsupported Media Type", "Use application/octet-stream");
  const auto* slot = esp_ota_get_next_update_partition(nullptr);
  char lengthHeader[24];
  if (!slot || httpd_req_get_hdr_value_str(request, "Content-Length", lengthHeader, sizeof(lengthHeader)) != ESP_OK ||
      !FirmwareUpload::contentLength(lengthHeader, request->content_len, slot->size))
    return reject(request, "400 Bad Request", "Content-Length must fit the inactive app slot");
  uploadClaimed = true;
  previousBoot = esp_ota_get_boot_partition();
  if (!previousBoot) {
    failure = "No current boot slot";
    const auto response = reject(request, "500 Internal Server Error", failure);
    finished.store(true, std::memory_order_release);
    return response;
  }
  struct Body {
    ManualUpdateReceiver& owner;
    httpd_req_t* request;
    UpdateStatus status{UpdatePhase::Receiving};
    uint32_t lastRead = millis();
    bool active() const { return owner.active(); }
    int read(uint8_t* bytes, size_t count) {
      while (active() && uint32_t(millis() - lastRead) < bodyIdleMs) {
        const int n = httpd_req_recv(request, reinterpret_cast<char*>(bytes), count);
        if (n == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (n > 0) lastRead = millis();
        return n;
      }
      return 0;
    }
    void progress(size_t count) {
      status.received = count;
      owner.state.publishUpdate(status);
    }
  } body{*this, request};
  body.status.total = request->content_len;
  body.progress(0);
  InactivePartition writer(slot);
  const auto result = FirmwareUpload::install(body, writer, request->content_len, els_hardware_descriptor);
  failure = uploadError(result);
  esp_err_t response;
  if (failure) {
    response = reject(request, "422 Unprocessable Content", failure);
  } else {
    response = reply(request, "200 OK", "Firmware verified. Restarting.\n");
    if (response != ESP_OK) {
      failure = esp_ota_set_boot_partition(previousBoot) == ESP_OK ? "Reply failed" : "Restore slot failed";
    }
  }
  replyAt = millis();
  finished.store(true, std::memory_order_release);
  return response;
}

void ManualUpdateReceiver::poll(bool healthy) {
  if (!server) return;
  const bool done = finished.load(std::memory_order_acquire);
  if (!done && healthy && active()) return;
  if (done && healthy && !failure && uint32_t(millis() - replyAt) < 250) return;
  cancelled.store(true, std::memory_order_release);
  const bool stopped = shutdown();
  if (finished.load(std::memory_order_acquire) && !failure) {
    if (healthy && stopped) {
      state.publishUpdate(UpdateStatus{UpdatePhase::Complete});
      esp_restart();
    }
    if (esp_ota_set_boot_partition(previousBoot) != ESP_OK) { publishFailure("Restore slot failed"); return; }
  }
  publishFailure(!stopped ? "Receiver stop failed" : !healthy ? "Update inhibited" :
                 finished.load(std::memory_order_acquire) && failure ? failure : "Upload timed out");
}
#endif
