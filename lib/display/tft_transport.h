#pragma once
#include <Adafruit_ST7789.h>
#include <els/hardware_config.h>

inline void initializeTft(Adafruit_ST7789& tft, uint16_t width, uint16_t height) {
  tft.init(width, height);
  tft.setSPISpeed(hardware.tft.frequencyHz);
  tft.setRotation(hardware.tft.rotation);
  tft.invertDisplay(hardware.tft.invert);
}
