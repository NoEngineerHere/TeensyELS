#pragma once
#include <els/hardware_config.h>
#include "encoder_detents.h"
#if defined(ESP32)
#include <ESP32Encoder.h>
#elif defined(CORE_TEENSY)
#include <Encoder.h>
#endif

class UiEncoder {
#if defined(ESP32)
  ESP32Encoder encoder;
  int64_t lastCount = 0;
#elif defined(CORE_TEENSY)
  Encoder encoder;
#endif
  EncoderDetents detents{int64_t(hardware.encoder.countsPerDetent)
#if defined(CORE_TEENSY)
    * (hardware.encoder.mode == EncoderMode::SingleEdge ? 4 : 1)
#endif
  };
public:
  bool begin();
  int64_t poll();
};
