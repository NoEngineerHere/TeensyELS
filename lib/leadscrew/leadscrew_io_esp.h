#pragma once
#include "leadscrew_io.h"
#ifdef ESP32
#include <driver/rmt.h>
#include <driver/gpio.h>
#endif
#if defined(ESP32) || defined(ELS_TEST_ESP_IO)
class LeadscrewIOESP : public LeadscrewIO {
  bool initialized = false;
  bool pending = false;
  bool fault = false;
  rmt_item32_t item{};
public:
  bool begin() override {
    if (initialized || fault) return false;
    gpio_set_level((gpio_num_t)ELS_LEADSCREW_STEP, 0);
    gpio_set_direction((gpio_num_t)ELS_LEADSCREW_STEP, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)ELS_LEADSCREW_DIR, 0);
    gpio_set_direction((gpio_num_t)ELS_LEADSCREW_DIR, GPIO_MODE_OUTPUT);
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX((gpio_num_t)ELS_LEADSCREW_STEP, RMT_CHANNEL_0);
    config.clk_div = 80;
    config.tx_config.carrier_en = false;
    config.tx_config.loop_en = false;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    static_assert(ELS_STEP_HIGH_US > 0 && ELS_STEP_HIGH_US <= 32767, "RMT high duration out of range");
    static_assert(ELS_STEP_LOW_US > 0 && ELS_STEP_LOW_US <= 32767, "RMT low duration out of range");
    item.level0 = 1; item.duration0 = ELS_STEP_HIGH_US;
    item.level1 = 0; item.duration1 = ELS_STEP_LOW_US;
    if (rmt_config(&config) != ESP_OK || rmt_driver_install(RMT_CHANNEL_0, 0, 0) != ESP_OK) {
      fault = true;
      return false;
    }
    initialized = true;
    return true;
  }
  void writeDirPin(uint8_t val) override { gpio_set_level((gpio_num_t)ELS_LEADSCREW_DIR, val); }
  bool startPulse(uint32_t) override {
    if (!initialized || fault || pending) return false;
    if (rmt_write_items(RMT_CHANNEL_0, &item, 1, false) != ESP_OK) {
      fault = true;
      return false;
    }
    pending = true;
    return true;
  }
  PulseResult servicePulse(uint32_t) override {
    if (fault) return PulseResult::FAULT;
    if (!pending) return PulseResult::IDLE;
    const auto result = rmt_wait_tx_done(RMT_CHANNEL_0, 0);
    if (result == ESP_ERR_TIMEOUT) return PulseResult::PENDING;
    if (result != ESP_OK) { fault = true; return PulseResult::FAULT; }
    pending = false;
    return PulseResult::COMPLETE;
  }
};
#endif
