#pragma once
#include "hardware_types.h"

#if ELS_SCREEN != ELS_SCREEN_SSD1306_128X64 && ELS_SCREEN != ELS_SCREEN_ST7789_240X135 && ELS_SCREEN != ELS_SCREEN_ST7789_320X240_LVGL
#error "Missing or unknown ELS_SCREEN selector"
#endif
inline constexpr int selectedScreen = ELS_SCREEN;

#ifndef ELS_HARDWARE_CONFIG
#error "Missing ELS_HARDWARE_CONFIG header selector"
#else
#include ELS_HARDWARE_CONFIG
inline constexpr HardwareConfig hardware = makeHardwareConfig();
static_assert(ELS_SCREEN == selectedScreen, "Wiring headers must not override ELS_SCREEN");
#endif


#if ELS_INPUT_LAYOUT != ELS_INPUT_DIRECT && ELS_INPUT_LAYOUT != ELS_INPUT_LEGACY_MATRIX && ELS_INPUT_LAYOUT != ELS_INPUT_MK2_MATRIX
#error "Missing or unknown ELS_INPUT_LAYOUT selector"
#endif
