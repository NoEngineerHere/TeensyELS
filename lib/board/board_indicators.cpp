#if defined(ESP32) || defined(CORE_TEENSY)
#include <Arduino.h>
#include "board.h"
#include "board_indicators.h"
#include <globalstate.h>

void initializeIndicators() {
  for (const int pin : {hardware.indicators.red, hardware.indicators.green, hardware.indicators.blue}) {
    if (pin < 0) continue;
    digitalWrite(pin, hardware.indicators.activeLow);
    pinMode(pin, OUTPUT);
  }
}
void updateIndicators(GlobalState& state, uint32_t nowMs) {
  if constexpr (hardware.indicators.red < 0 && hardware.indicators.green < 0) return;
  static unsigned last = 0;
  const bool locked = state.getButtonLock() == LK_LOCKED;
  unsigned lights = 0;
  switch (state.getMotionMode()) {
    case MM_DISABLED: lights = locked ? 1 : 0; break;
    case MM_ENABLED: lights = locked && (nowMs / 250) % 2 ? 1 : 2; break;
    case MM_JOG_LEFT:
    case MM_JOG_RIGHT: lights = 3; break;
    case MM_UNSET: break;
  }
  if (last == lights) return;
  last = lights;
  if constexpr (hardware.indicators.red >= 0)
    digitalWrite(hardware.indicators.red, bool(lights & 1) != hardware.indicators.activeLow);
  if constexpr (hardware.indicators.green >= 0)
    digitalWrite(hardware.indicators.green, bool(lights & 2) != hardware.indicators.activeLow);
}
#endif
