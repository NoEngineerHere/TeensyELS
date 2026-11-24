#if defined(ESP32) || defined(ELS_TEST_ESP_BUS)
#include "board.h"
#include "display_bus.h"

#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
#include <Wire.h>
TwoWire& displayI2cBus() {
  if constexpr (hardware.oled.bus == 1) return Wire1;
  return Wire;
}
bool initializeDisplayBus() {
  const auto& w = hardware.oled;
  return displayI2cBus().begin(w.sda, w.scl, w.restoreHz);
}
#else
#include <SPI.h>
SPIClass& displaySpiBus() {
  static SPIClass bus(hardware.tft.bus == 0 ? VSPI : HSPI);
  return bus;
}
bool initializeDisplayBus() {
  const auto& s = hardware.tft;
  displaySpiBus().begin(s.sclk, s.miso, s.mosi, -1);
  return true;
}
#endif
#endif
