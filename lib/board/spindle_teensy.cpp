#ifdef CORE_TEENSY
#include <spindle.h>
#include "spindle_encoder.h"
bool SpindleEncoder::begin() {
  encoder.begin(a, b);
  encoder.write(0);
  return true;
}
int SpindleEncoder::sample(Spindle& spindle) {
  const int delta = encoder.readAndReset();
  spindle.incrementCurrentPosition(delta);
  return delta;
}
#endif
