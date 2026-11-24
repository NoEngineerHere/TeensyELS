#pragma once
#ifdef ESP32
#include <atomic>
#include <esp_http_server.h>
#include <esp_netif.h>
#include <esp_ota_ops.h>
#include <globalstate.h>

class ManualUpdateReceiver {
  GlobalState& state;
  esp_netif_t* ap = nullptr;
  httpd_handle_t server = nullptr;
  bool wifiInitialized = false;
  bool wifiRunning = false;
  bool uploadClaimed = false; // Owned by the HTTP server task.
  uint32_t openedAt = 0;
  std::atomic<bool> cancelled{false}, finished{false};
  const char* failure = nullptr; // Published by finished.
  uint32_t replyAt = 0;
  const esp_partition_t* previousBoot = nullptr;

  bool startAp(UpdateStatus& status);
  bool shutdown();
  bool active() const;
  void publishFailure(const char* detail);
  esp_err_t upload(httpd_req_t* request);
public:
  explicit ManualUpdateReceiver(GlobalState& state) : state(state) {}
  void arm();
  void poll(bool healthy);
};
#endif
