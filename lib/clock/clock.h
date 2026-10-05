#pragma once
#include <cstdint>
uint32_t elsMicros();
#ifdef PIO_UNIT_TESTING
void setTestMicros(uint32_t value);
#endif
