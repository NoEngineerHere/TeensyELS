#include "clock.h"
#ifdef PIO_UNIT_TESTING
#include <atomic>
static std::atomic<uint32_t> now{0};
uint32_t elsMicros() { return now.load(std::memory_order_relaxed); }
void setTestMicros(uint32_t value) { now.store(value, std::memory_order_relaxed); }
#else
#include <Arduino.h>
uint32_t elsMicros() { return micros(); }
#endif
