#pragma once

// this file unsets all user config when included
// used for tests only to have a consistent environment

#undef ELS_SPINDLE_ENCODER_PPR
#undef ELS_LEADSCREW_STEPPER_PPR
#undef ELS_LEADSCREW_PITCH_MM
#undef LEADSCREW_JERK
#undef LEADSCREW_ACCEL

// makes the math easier...
#define ELS_SPINDLE_ENCODER_PPR 360
#define ELS_LEADSCREW_STEPPER_PPR 360
#define ELS_LEADSCREW_PITCH_MM 1
#define LEADSCREW_JERK 0.1
#define LEADSCREW_ACCEL 20