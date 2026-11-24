#pragma once
#include <els/hardware_config.h>
#include <pulse_timing.h>
#include <array>
#include <cstddef>
#include <cstdint>

inline constexpr uint32_t descriptorMagic = 0x31534c45; // ELS1 in little endian.
inline constexpr uint32_t descriptorVersion = 1;
constexpr auto makeFirmwareDescriptor(const HardwareConfig& config, PulseTiming timing) {
  const auto& m = config.motion;
  const auto& e = config.encoder;
  const auto& i = config.indicators;
#if ELS_INPUT_LAYOUT == ELS_INPUT_DIRECT
  const auto& p = config.direct;
#else
  const auto& p = config.matrix;
#endif
#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
  const auto& d = config.oled;
#else
  const auto& d = config.tft;
#endif
  const uint32_t values[] = {
    uint32_t(config.processor), ELS_SCREEN, ELS_INPUT_LAYOUT,
    uint32_t(m.step), uint32_t(m.dir), uint32_t(m.enable),
    uint32_t(m.spindleA), uint32_t(m.spindleB), uint32_t(m.alarm), m.alarm >= 0 && m.alarmActiveLow,
#if ELS_INPUT_LAYOUT == ELS_INPUT_DIRECT
    uint32_t(p.increase), uint32_t(p.decrease), uint32_t(p.mode), uint32_t(p.sync),
    uint32_t(p.halfNut), uint32_t(p.enable), uint32_t(p.lock), uint32_t(p.left), uint32_t(p.right),
#else
    uint32_t(p.rows[0]), uint32_t(p.rows[1]), uint32_t(p.rows[2]),
    uint32_t(p.columns[0]), uint32_t(p.columns[1]), uint32_t(p.columns[2]),
#endif
    uint32_t(e.a), uint32_t(e.b), e.a >= 0 ? uint32_t(e.mode) : 0,
    e.a >= 0 ? uint32_t(e.countsPerDetent) : 0, e.a >= 0 && e.externalPulls,
    uint32_t(i.red), uint32_t(i.green), uint32_t(i.blue),
    (i.red >= 0 || i.green >= 0 || i.blue >= 0) && i.activeLow,
#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
    uint32_t(d.bus), uint32_t(d.sda), uint32_t(d.scl), uint32_t(d.address), uint32_t(d.reset),
    d.frequencyHz, d.restoreHz,
#else
    uint32_t(d.bus), uint32_t(d.mosi), uint32_t(d.miso), uint32_t(d.sclk), uint32_t(d.cs), uint32_t(d.dc),
    uint32_t(d.reset), uint32_t(d.backlight), d.backlight >= 0 && d.backlightActiveHigh,
    d.invert, uint32_t(d.rotation), d.frequencyHz,
#endif
    timing.highUs, timing.lowUs, timing.dirSetupUs, timing.dirHoldUs,
  };
  std::array<uint8_t, sizeof(values) + 12> descriptor{};
  std::size_t offset = 0;
  const auto append = [&](uint32_t word) {
    for (unsigned byte = 0; byte < 4; ++byte) descriptor[offset++] = uint8_t(word >> (byte * 8));
  };
  append(descriptorMagic);
  append(descriptorVersion);
  append(descriptor.size());
  for (auto value : values) append(value);
  return descriptor;
}

using FirmwareDescriptor = decltype(makeFirmwareDescriptor(hardware, pulseTiming));
