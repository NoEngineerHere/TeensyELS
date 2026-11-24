#ifdef ESP32
#include "spindle.h"
#include <climits>
Spindle::Spindle(int pinA, int pinB) {
  ESP32Encoder::useInternalWeakPullResistors = puType::none;
  m_encoder.attachFullQuad(pinA, pinB);
  gpio_pullup_en((gpio_num_t)pinA);
  gpio_pullup_en((gpio_num_t)pinB);
  m_lastEncoderCount = m_encoder.getCount();
}
void Spindle::update() {
  const int64_t count = m_encoder.getCount();
  int64_t delta;
  if (__builtin_sub_overflow(count, m_lastEncoderCount, &delta) ||
      delta < INT_MIN || delta > INT_MAX) {
    latchMotionFault();
  } else {
    incrementCurrentPosition(static_cast<int>(delta));
  }
  m_lastEncoderCount = count;
}
#endif
