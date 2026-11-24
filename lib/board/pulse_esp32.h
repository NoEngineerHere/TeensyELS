#pragma once
#include <leadscrew_io.h>
#include <clock.h>
#ifdef ESP32
#include <driver/rmt.h>
#include <driver/gpio.h>
#endif
#if defined(ESP32) || defined(ELS_TEST_ESP_IO)
template<int StepPin, int DirPin>
class LeadscrewIOESP : public LeadscrewIO {
  bool initialized = false;
  bool pending = false;
  bool fault = false;
  rmt_item32_t item{};
public:
  explicit LeadscrewIOESP(PulseTiming timing) {
    item.level0 = 1; item.duration0 = timing.highUs;
    item.level1 = 0; item.duration1 = timing.lowUs;
  }
  bool begin() override {
    if (initialized || fault) return false;
    constexpr auto step = static_cast<gpio_num_t>(StepPin);
    constexpr auto dir = static_cast<gpio_num_t>(DirPin);
    gpio_set_level(step, 0);
    gpio_set_direction(step, GPIO_MODE_OUTPUT);
    gpio_set_level(dir, 0);
    gpio_set_direction(dir, GPIO_MODE_OUTPUT);
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(step, RMT_CHANNEL_0);
    config.clk_div = 80;
    config.tx_config.carrier_en = false;
    config.tx_config.loop_en = false;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    if (rmt_config(&config) != ESP_OK || rmt_driver_install(RMT_CHANNEL_0, 0, 0) != ESP_OK) {
      fault = true;
      return false;
    }
    initialized = true;
    return true;
  }
  void writeDirPin(uint8_t val) override { gpio_set_level(static_cast<gpio_num_t>(DirPin), val); }
  bool startPulse(uint32_t) override {
    if (!initialized || fault || pending) return false;
    if (rmt_write_items(RMT_CHANNEL_0, &item, 1, false) != ESP_OK) {
      fault = true;
      return false;
    }
    pending = true;
    return true;
  }
  PulseStatus servicePulse(uint32_t) override {
    if (fault) return {PulseResult::FAULT};
    if (!pending) return {PulseResult::IDLE};
    const auto result = rmt_wait_tx_done(RMT_CHANNEL_0, 0);
    if (result == ESP_ERR_TIMEOUT) return {PulseResult::PENDING};
    if (result != ESP_OK) { fault = true; return {PulseResult::FAULT}; }
    pending = false;
    return {PulseResult::COMPLETE, elsMicros() - item.duration1};
  }
};
#endif
