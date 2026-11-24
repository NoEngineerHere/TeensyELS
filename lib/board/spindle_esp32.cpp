#ifdef ESP32
#include <spindle.h>
#include "spindle_encoder.h"
#include <climits>
bool SpindleEncoder::begin() {
  ESP32Encoder::useInternalWeakPullResistors = puType::none;
  encoder.attachFullQuad(a, b);
  if (!encoder.isAttached()) return false;
  lastCount = encoder.getCount();
  return true;
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
