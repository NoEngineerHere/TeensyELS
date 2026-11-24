#pragma once
#include <leadscrew_io.h>

// Compatibility defaults are not qualified for any particular driver.
#ifndef ELS_COMPAT_PULSE_US
#define ELS_COMPAT_PULSE_US 4
#endif
#ifndef ELS_STEP_HIGH_US
#define ELS_STEP_HIGH_US ELS_COMPAT_PULSE_US
#endif
#ifndef ELS_STEP_LOW_US
#define ELS_STEP_LOW_US ELS_COMPAT_PULSE_US
#endif
#ifndef ELS_DIR_SETUP_US
#define ELS_DIR_SETUP_US 4
#endif
#ifndef ELS_DIR_HOLD_US
#define ELS_DIR_HOLD_US 4
#endif

inline constexpr PulseTiming pulseTiming{
    ELS_STEP_HIGH_US, ELS_STEP_LOW_US, ELS_DIR_SETUP_US, ELS_DIR_HOLD_US};
