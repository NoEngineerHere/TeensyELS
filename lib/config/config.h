// This file contains all hardware configs for your system

// not using pragma once to allow for multiple inclusion in tests, do not
// remove!
#ifndef ELS_CONFIG_H
#define ELS_CONFIG_H

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

// Macro to check at compile time if an index is out of bounds
#define CHECK_BOUNDS(idx, arr, error) \
  static_assert(idx < ARRAY_SIZE(arr), error)

// the amount of microseconds in a second
#define US_PER_SECOND 1000000

/**
 * Uncomment this line if your spindle is driven by a motor controlled by this
 * application. If it is uncommented we will assume you have an encoder attached
 * to your spindle
 * TODO: Implement this for real
 */
// #define ELS_SPINDLE_DRIVEN

/**
 * IO Pins
 */
#ifdef ELS_SPINDLE_DRIVEN
// set your spindle driver pins here
#define ELS_SPINDLE_STEP -1
#define ELS_SPINDLE_DIR -1
#else
#define ELS_SPINDLE_ENCODER_A 14
#define ELS_SPINDLE_ENCODER_B 15
#endif

#define ELS_LEADSCREW_STEP 2
#define ELS_LEADSCREW_DIR 3

#define ELS_RATE_INCREASE_BUTTON 4
#define ELS_RATE_DECREASE_BUTTON 5
#define ELS_MODE_CYCLE_BUTTON 6
#define ELS_THREAD_SYNC_BUTTON 7
#define ELS_HALF_NUT_BUTTON 8
#define ELS_ENABLE_BUTTON 9
#define ELS_LOCK_BUTTON 10
#define ELS_JOG_LEFT_BUTTON 24
#define ELS_JOG_RIGHT_BUTTON 25

/**
 * Display
 *
 * This setting allows you to select what type of display you want to use.
 * The selection will hopefully grow as time goes on!
 *
 * Options:
 *   SSD1306_128_64: 128x64 oled
 */
#define ELS_DISPLAY SSD1306_128_64

#if ELS_DISPLAY == SSD1306_128_64
// define this if you have a dedicated pin for the oled reset
#define PIN_DISPLAY_RESET -1
#endif

#define ELS_SPINDLE_ENCODER_PPR 400
#define ELS_LEADSCREW_STEPPER_PPR 400
#define ELS_LEADSCREW_PITCH_MM 1.25

#define ELS_LEADSCREW_STEPS_PER_MM \
  (float)(ELS_LEADSCREW_STEPPER_PPR / ELS_LEADSCREW_PITCH_MM)

// extra config options
// jog speed in mm/s
#define JOG_SPEED 100

#define JOG_PULSE_DELAY   \
  ((float)US_PER_SECOND / \
   ((float)JOG_SPEED * (float)ELS_LEADSCREW_STEPS_PER_MM))

/**
 * The unit mode the system should start up in
 * Options:
 *  GlobalUnitMode::METRIC: Metric system
 *  GlobalUnitMode::IMPERIAL: Imperial system
 */
#define DEFAULT_UNIT_MODE GlobalUnitMode::METRIC
#define DEFAULT_FEED_MODE GlobalFeedMode::FEED

// The default starting speed for leadscrew in mm/s
// this is the maximum allowable speed (in mm/s) for the leadscrew to
// instantaneously start moving from 0
// #define ACCEL_DISABLED
#define LEADSCREW_JERK 0.5
// The acceleration of the leadscrew in mm/s^2

#define LEADSCREW_ACCEL 100

#define LEADSCREW_TIMER_US 20

// The initial delay between pulses in microseconds for the leadscrew starting
// from 0 do not change - this is a calculated value, to change the initial
// speed look at the jerk value
#ifdef ACCEL_DISABLED
#define LEADSCREW_INITIAL_PULSE_DELAY_US 0
#else
#define LEADSCREW_INITIAL_PULSE_DELAY_US \
  ((float)US_PER_SECOND /                \
   ((float)LEADSCREW_JERK * (float)ELS_LEADSCREW_STEPS_PER_MM))
#endif

// The amount of time to increment/decrement the pulse delay by in microseconds
// for the leadscrew This is calculated based on the acceleration value
#ifdef ACCEL_DISABLED
#define LEADSCREW_PULSE_DELAY_STEP_US 0
#else
#define LEADSCREW_PULSE_DELAY_STEP_US \
  ((float)LEADSCREW_ACCEL / ((float)ELS_LEADSCREW_STEPS_PER_MM))
#endif

// metric thread pitch is defined as mm/rev
const float threadPitchMetric[] = {0.35, 0.40, 0.45, 0.50, 0.60, 0.70, 0.80,
                                   1.00, 1.25, 1.50, 1.75, 2.00, 2.50, 3.00,
                                   3.50, 4.00, 4.50, 5.00, 5.50, 6.00};
#define DEFAULT_METRIC_THREAD_PITCH_IDX 8

// defined as mm/rev
const float feedPitchMetric[] = {0.05, 0.08, 0.10, 0.12, 0.15, 0.18, 0.20,
                                 0.23, 0.25, 0.28, 0.30, 0.35, 0.40, 0.45,
                                 0.50, 0.55, 0.60, 0.65, 0.70, 0.75};
#define DEFAULT_METRIC_FEED_PITCH_IDX 8

// for convenience these are defined as TPI - retained as float to allow for
// partial TPI for whatever reason
const float threadPitchImperial[] = {80, 72, 64, 56, 48, 44, 40, 36, 32, 28,
                                     24, 20, 18, 16, 14, 13, 12, 11, 10, 9};
#define DEFAULT_IMPERIAL_THREAD_PITCH_IDX 8
// defined as thou/rev
const float feedPitchImperial[] = {
    0.002, 0.003, 0.004, 0.005, 0.006, 0.007, 0.008, 0.009, 0.010, 0.011,
    0.012, 0.014, 0.016, 0.018, 0.020, 0.022, 0.024, 0.026, 0.028, 0.030};
#define DEFAULT_IMPERIAL_FEED_PITCH_IDX 8

#endif