#include "critical_section.h"
#ifdef CORE_TEENSY
#include <cmsis_gcc.h>
ElsCriticalSection::ElsCriticalSection() : saved(__get_PRIMASK()) { __disable_irq(); }
ElsCriticalSection::~ElsCriticalSection() { __set_PRIMASK(saved); }
#elif defined(ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
static portMUX_TYPE motionMux = portMUX_INITIALIZER_UNLOCKED;
ElsCriticalSection::ElsCriticalSection() { portENTER_CRITICAL(&motionMux); }
ElsCriticalSection::~ElsCriticalSection() { portEXIT_CRITICAL(&motionMux); }
#else
#include <mutex>
static std::mutex motionMutex;
ElsCriticalSection::ElsCriticalSection() { motionMutex.lock(); }
ElsCriticalSection::~ElsCriticalSection() { motionMutex.unlock(); }
#endif
