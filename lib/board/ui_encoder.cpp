#if defined(ESP32) || defined(CORE_TEENSY)
#include "ui_encoder.h"

bool UiEncoder::begin() {
  const auto& pins = hardware.encoder;
#if defined(ESP32)
  ESP32Encoder::useInternalWeakPullResistors = pins.externalPulls ? puType::none : puType::up;
  if (pins.mode == EncoderMode::FullQuadrature) encoder.attachFullQuad(pins.a, pins.b);
  else encoder.attachSingleEdge(pins.a, pins.b);
  if (!encoder.isAttached()) return false;
  encoder.setFilter(1023);
  lastCount = encoder.getCount();
#else
  encoder.begin(pins.a, pins.b);
  if (pins.externalPulls) {
    pinMode(pins.a, INPUT);
    pinMode(pins.b, INPUT);
  }
  encoder.write(0);
#endif
  return true;
}

int64_t UiEncoder::poll() {
#if defined(ESP32)
  const int64_t count = encoder.getCount();
  int64_t delta;
  const bool overflow = __builtin_sub_overflow(count, lastCount, &delta);
  lastCount = count;
  return overflow ? 0 : detents.consume(delta);
#else
  return detents.consume(encoder.readAndReset());
#endif
}
#endif
