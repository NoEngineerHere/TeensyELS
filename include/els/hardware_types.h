#pragma once

#define ELS_SCREEN_SSD1306_128X64 1
#define ELS_SCREEN_ST7789_240X135 2
#define ELS_SCREEN_ST7789_320X240_LVGL 3

#define ELS_INPUT_DIRECT 1
#define ELS_INPUT_LEGACY_MATRIX 2
#define ELS_INPUT_MK2_MATRIX 3

enum class Processor { Unspecified, Teensy41, Esp32 };
enum class EncoderMode { SingleEdge, FullQuadrature };

struct HardwareConfig {
  Processor processor = Processor::Unspecified;
  struct {
    int step = -1, dir = -1, enable = -1;
    int spindleA = -1, spindleB = -1, alarm = -1;
    bool alarmActiveLow = true;
  } motion;
  struct {
    int increase = -1, decrease = -1, mode = -1, sync = -1, halfNut = -1;
    int enable = -1, lock = -1, left = -1, right = -1;
  } direct;
  struct {
    int rows[3] = {-1, -1, -1};
    int columns[3] = {-1, -1, -1};
  } matrix;
  struct {
    int a = -1, b = -1;
    EncoderMode mode = EncoderMode::SingleEdge;
    int countsPerDetent = 1;
    bool externalPulls = false;
  } encoder;
  struct {
    int red = -1, green = -1, blue = -1;
    bool activeLow = false;
  } indicators;
  struct {
    int bus = 0, sda = -1, scl = -1, address = 0x3c, reset = -1;
    unsigned frequencyHz = 400000, restoreHz = 100000;
  } oled;
  struct {
    int bus = 0, mosi = -1, miso = -1, sclk = -1, cs = -1, dc = -1;
    int reset = -1, backlight = -1;
    bool backlightActiveHigh = true, invert = true;
    int rotation = 3;
    unsigned frequencyHz = 40000000;
  } tft;
};
