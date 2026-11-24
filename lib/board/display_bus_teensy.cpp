#if defined(CORE_TEENSY) || defined(ELS_TEST_TEENSY_BUS)
#include "board.h"
#include "display_bus.h"

#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
#include <Wire.h>
TwoWire& displayI2cBus() {
  if constexpr (hardware.oled.bus == 1) return Wire1;
  if constexpr (hardware.oled.bus == 2) return Wire2;
  return Wire;
}
bool initializeDisplayBus() {
  auto& bus = displayI2cBus();
  bus.setSDA(hardware.oled.sda);
  bus.setSCL(hardware.oled.scl);
  bus.begin();
  bus.setClock(hardware.oled.restoreHz);
  return true;
}
#else
#include <SPI.h>
SPIClass& displaySpiBus() {
  if constexpr (hardware.tft.bus == 1) return SPI1;
  if constexpr (hardware.tft.bus == 2) return SPI2;
  return SPI;
}
bool initializeDisplayBus() {
  auto& bus = displaySpiBus();
  bus.setMOSI(hardware.tft.mosi);
  bus.setMISO(hardware.tft.miso);
  bus.setSCK(hardware.tft.sclk);
  bus.begin();
  return true;
}
#endif
#endif
