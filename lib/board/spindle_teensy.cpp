#ifdef CORE_TEENSY
#include <spindle.h>
#include "spindle_encoder.h"
bool SpindleEncoder::begin() {
  encoder.begin(a, b);
  encoder.write(0);
  return true;
}
void SpindleEncoder::sample(Spindle& spindle) {
  spindle.incrementCurrentPosition(encoder.readAndReset());
}
#endif
