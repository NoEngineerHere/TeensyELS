#ifdef CORE_TEENSY
#include "spindle.h"
Spindle::Spindle(int pinA, int pinB) : m_encoder(pinA, pinB) {}
void Spindle::update() {
  int position = m_encoder.read();
  m_encoder.write(0);
  incrementCurrentPosition(position);
}
#endif
