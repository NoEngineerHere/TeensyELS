#if defined(CORE_TEENSY) || defined(ELS_TEST_TEENSY_RUNTIME)
#ifndef PIO_UNIT_TESTING
#include <Arduino.h>
#include "runtime.h"
static IntervalTimer timer;
#endif

static uint32_t lastInputMs = 0;
static void motionInterrupt() {
  if (motionServiceNeeded()) motionTick();
}
static void serviceInput() {
  const uint32_t now = millis();
  if (now - lastInputMs < 5) return;
  lastInputMs = now;
  inputTick();
}
RuntimeStartResult startRuntime() {
  return timer.begin(motionInterrupt, 4) ? RuntimeStartResult::Started : RuntimeStartResult::MotionFailed;
}
void serviceRuntimeForeground() {
  serviceInput();
  displayTick();
  serviceInput();
}
#endif
