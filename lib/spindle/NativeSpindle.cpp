#ifdef PIO_UNIT_TESTING
#include "spindle.h"
Spindle::Spindle() = default;
void Spindle::update() { incrementCurrentPosition(0); }
#endif