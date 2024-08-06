
// no pragma once, this file is meant to be included multiple times to reset
// between tests

// this file unsets all user config when included
// used for tests only to have a consistent environment

#include "reset_config_undef.h"

// makes the math easier...
#define ELS_SPINDLE_ENCODER_PPR 360
#define ELS_LEADSCREW_STEPPER_PPR 360
#define ELS_LEADSCREW_PITCH_MM 1
#define LEADSCREW_JERK 0.1
#define LEADSCREW_ACCEL 20