#pragma once
#include <els/hardware_config.h>
#include "hardware_validation.h"

#if defined(ESP32)
#include <esp_bit_defs.h>
#include <soc/soc_caps.h>
static_assert(SOC_GPIO_VALID_GPIO_MASK == hardwareValidation::espInputMask &&
    SOC_GPIO_VALID_OUTPUT_GPIO_MASK == hardwareValidation::espOutputMask);
inline constexpr Processor compiledProcessor = Processor::Esp32;
#elif defined(CORE_TEENSY)
#include <Arduino.h>
static_assert(CORE_NUM_DIGITAL == hardwareValidation::teensyPinCount);
inline constexpr Processor compiledProcessor = Processor::Teensy41;
#elif defined(ELS_NATIVE_PROCESSOR)
inline constexpr Processor compiledProcessor = ELS_NATIVE_PROCESSOR;
#else
#error "Unsupported processor"
#endif

namespace hardwareValidation {
inline constexpr Direction encoderDirection = hardware.encoder.externalPulls
    ? Direction::Input : Direction::PullInput;
inline constexpr Signal activePins[] = {
    {hardware.motion.step, Direction::Output}, {hardware.motion.dir, Direction::Output},
    {hardware.motion.enable, Direction::Output},
    {hardware.motion.spindleA, Direction::Input}, {hardware.motion.spindleB, Direction::Input},
    {hardware.motion.alarm, Direction::Input, false},
#if ELS_INPUT_LAYOUT == ELS_INPUT_DIRECT
    {hardware.direct.increase, Direction::PullInput}, {hardware.direct.decrease, Direction::PullInput},
    {hardware.direct.mode, Direction::PullInput}, {hardware.direct.sync, Direction::PullInput},
    {hardware.direct.halfNut, Direction::PullInput}, {hardware.direct.enable, Direction::PullInput},
    {hardware.direct.lock, Direction::PullInput}, {hardware.direct.left, Direction::PullInput},
    {hardware.direct.right, Direction::PullInput},
#else
    {hardware.matrix.rows[0], Direction::PullInput}, {hardware.matrix.rows[1], Direction::PullInput},
    {hardware.matrix.rows[2], Direction::PullInput},
    {hardware.matrix.columns[0], Direction::Output}, {hardware.matrix.columns[1], Direction::Output},
    {hardware.matrix.columns[2], Direction::Output},
#endif
    {hardware.encoder.a, encoderDirection, false}, {hardware.encoder.b, encoderDirection, false},
    {hardware.indicators.red, Direction::Output, false}, {hardware.indicators.green, Direction::Output, false},
    {hardware.indicators.blue, Direction::Output, false},
#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
    {hardware.oled.sda, Direction::Output}, {hardware.oled.scl, Direction::Output},
    {hardware.oled.reset, Direction::Output, false},
#else
    {hardware.tft.mosi, Direction::Output}, {hardware.tft.sclk, Direction::Output},
    {hardware.tft.miso, Direction::Input, compiledProcessor == Processor::Teensy41},
    {hardware.tft.cs, Direction::Output}, {hardware.tft.dc, Direction::Output},
    {hardware.tft.reset, Direction::Output, false}, {hardware.tft.backlight, Direction::Output, false},
#endif
};
inline constexpr unsigned pinCount = sizeof(activePins) / sizeof(activePins[0]);
static_assert(hardware.processor == compiledProcessor, "Hardware processor mismatch");
static_assert(hasRequiredPins(activePins, pinCount) &&
    ((hardware.encoder.a == -1) == (hardware.encoder.b == -1)), "Missing required hardware pins");
static_assert(validDirections(compiledProcessor, activePins, pinCount), "Invalid GPIO direction or pin");
static_assert(uniquePins(activePins, pinCount), "Hardware pin collision");
static_assert(hardware.encoder.a == -1 || (hardware.encoder.countsPerDetent > 0 &&
    (hardware.encoder.mode == EncoderMode::SingleEdge || hardware.encoder.mode == EncoderMode::FullQuadrature)),
    "Invalid UI encoder settings");
#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
static_assert(validBus(compiledProcessor, hardware.oled.bus), "Unsupported display bus");
static_assert(i2cRoute(hardware), "Invalid display bus routing");
static_assert(hardware.oled.address >= 0x08 && hardware.oled.address <= 0x77 &&
    hardware.oled.frequencyHz > 0 && hardware.oled.restoreHz > 0, "Invalid OLED settings");
#else
static_assert(validBus(compiledProcessor, hardware.tft.bus), "Unsupported display bus");
static_assert(spiRoute(hardware), "Invalid display bus routing");
static_assert((hardware.tft.rotation == 1 || hardware.tft.rotation == 3) &&
    hardware.tft.frequencyHz > 0, "Invalid TFT settings");
#endif
}
