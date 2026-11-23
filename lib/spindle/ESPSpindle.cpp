#ifdef ESP32
#include "spindle.h"
#include "spindle_encoder.h"
#include <climits>
SpindleEncoder::SpindleEncoder(int pinA, int pinB) {
  ESP32Encoder::useInternalWeakPullResistors = puType::none;
  encoder.attachFullQuad(pinA, pinB);
  gpio_pullup_en((gpio_num_t)pinA);
  gpio_pullup_en((gpio_num_t)pinB);
  lastCount = encoder.getCount();
}
void SpindleEncoder::sample(Spindle& spindle) {
  const int64_t count = encoder.getCount();
  int64_t delta;
  if (__builtin_sub_overflow(count, lastCount, &delta) ||
      delta < INT_MIN || delta > INT_MAX) {
    spindle.latchMotionFault();
  } else {
    spindle.incrementCurrentPosition(static_cast<int>(delta));
  }
  lastCount = count;
}
#endif
