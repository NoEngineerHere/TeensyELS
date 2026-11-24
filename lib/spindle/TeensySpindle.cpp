#ifdef CORE_TEENSY
#include "spindle.h"
#include "spindle_encoder.h"
SpindleEncoder::SpindleEncoder(int pinA, int pinB) : encoder(pinA, pinB) {}
void SpindleEncoder::sample(Spindle& spindle) {
  int position = encoder.read();
  encoder.write(0);
  spindle.incrementCurrentPosition(position);
}
#endif
