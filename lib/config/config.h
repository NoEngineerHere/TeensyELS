// This file contains all hardware configs for your system

// not using pragma once to allow for multiple inclusion in tests, do not
// remove!
#ifndef ELS_CONFIG_H
#define ELS_CONFIG_H

/**
 * @brief      Calculates the number of items in an array.
 * @param      arr   The array
 * @return     The number of items in the array
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

 /**
  * @brief      A macro to check at compile time if an index is out of bounds
  * @param      idx    The index to check
  * @param      arr    The array to check against
  * @param      error  The error message to display if the index is out of bounds
  */
#define CHECK_BOUNDS(idx, arr, error) \
  static_assert(idx < ARRAY_SIZE(arr), error)

  //! the amount of microseconds in a second
#define US_PER_SECOND 1000000

#define ELS_BOARD_UNSET -1
#define ELS_BOARD_TEENSY 0
#define ELS_BOARD_ESP32 1

/**
 * @brief      The board type
 * @note       Options:
 *              - `ELS_BOARD_TEENSY`: Teensy 4.1
 *              - `ELS_BOARD_ESP32`: ESP32
 */
#ifdef CORE_TEENSY
#define ELS_BOARD ELS_BOARD_TEENSY
#elif defined(ESP32)
#define ELS_BOARD ELS_BOARD_ESP32
#else
#define ELS_BOARD ELS_BOARD_UNSET
#endif

 /**
  * @brief      Uncomment this line if your spindle is driven by a motor
  * controlled by this application.
  *
  * If this is uncommented, the application will drive the spindle using a
  * stepper motor. If it is commented out, the application will assume that the
  * spindle is driven by an external motor and will use an encoder to read its
  * position.
  *
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
#if ELS_BOARD == ELS_BOARD_ESP32
#define ELS_SPINDLE_ENCODER_A 37
#define ELS_SPINDLE_ENCODER_B 36
#else
#define ELS_SPINDLE_ENCODER_A 14
#define ELS_SPINDLE_ENCODER_B 15
#endif
#endif

   /**
    * @brief      Uncomment this line to enable the UI encoder
    *
    * If this is uncommented, the application will use a rotary encoder for user
    * input. The pins for the encoder are defined below.
    */
#define ELS_UI_ENCODER
#ifdef ELS_UI_ENCODER
#define ELS_UI_ENCODER_A 38  
#define ELS_UI_ENCODER_B 39  
#define ELS_IND_RED 22   
#define ELS_IND_GREEN 21  
#endif

    /**
     * @brief      This is used to define the valid values for the Human Interface Device (button names)
     */
constexpr char* HID_VALUES[] = {
  "ELS_RATE_INCREASE_BUTTON",
  "ELS_RATE_DECREASE_BUTTON",
  "ELS_MODE_CYCLE_BUTTON",
  "ELS_THREAD_SYNC_BUTTON",
  "ELS_HALF_NUT_BUTTON",
  "ELS_ENABLE_BUTTON",
  "ELS_LOCK_BUTTON",
  "ELS_JOG_LEFT_BUTTON",
  "ELS_JOG_RIGHT_BUTTON",
};

/**
 * @brief      A struct to define the button definition
 * @param      name  The name of the button - should be one of the values in the HID_VALUES array
 * @param      row   The row of the button - 0-indexed
 * @param      col   The column of the button - 0-indexed
 * @note if a button is connected to one pin only, set the col to -1
 */
struct ButtonDefinition {
  const char* name;
  int row;
  int col;
};

/**
 * @brief      Define the connections for your buttons here
 *
 */
constexpr ButtonDefinition BUTTON_DEFINITIONS[] = {
  { "ELS_RATE_INCREASE_BUTTON", 0, 0 },
  { "ELS_RATE_DECREASE_BUTTON", 0, 1 },
  { "ELS_MODE_CYCLE_BUTTON", 0, 2 },
  { "ELS_THREAD_SYNC_BUTTON", 0, 3 },
  { "ELS_HALF_NUT_BUTTON", 0, 4 },
  { "ELS_ENABLE_BUTTON", 0, 5 },
  { "ELS_LOCK_BUTTON", 0, 6 },
  { "ELS_JOG_LEFT_BUTTON", 0, 7 },
  { "ELS_JOG_RIGHT_BUTTON", 0, 8 },
};

/**
 * @brief      Platform-specific pinouts
 *
 * This section defines the pinouts for the different platforms that are
 * supported by the application.
 */
#if ELS_BOARD == ELS_BOARD_TEENSY
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

#define ELS_STEPPER_ENA 0


#elif ELS_BOARD == ELS_BOARD_ESP32
 /**
 * @brief      Uncomment this line to use the RMT peripheral for the leadscrew step and dir pins
 *
 * The RMT peripheral is a hardware peripheral that can be used to generate
 * PWM signals.It is more accurate than the analogWrite function, and can be
 * used to generate square waves with a frequency of up to 80MHz.
 *
 *For more information, see the ESP32 page on the module: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/rmt.html
 *
 *If this is uncommented, the application will not use RMT for the leadscrew step and dir pins.
 * This is only supported on ESP32.
 */
#define ELS_USE_RMT
#define ELS_LEADSCREW_STEP 25
#define ELS_LEADSCREW_STEP_BIT BIT25
#define ELS_LEADSCREW_DIR 26
#define ELS_LEADSCREW_DIR_BIT BIT26

#define ELS_STEPPER_ENA 17

#define ELS_RATE_INCREASE_BUTTON 17
#define ELS_RATE_DECREASE_BUTTON 9
#define ELS_MODE_CYCLE_BUTTON 33
#define ELS_THREAD_SYNC_BUTTON 10
#define ELS_HALF_NUT_BUTTON 18
#define ELS_ENABLE_BUTTON 34
#define ELS_LOCK_BUTTON 12
#define ELS_JOG_LEFT_BUTTON 20
#define ELS_JOG_RIGHT_BUTTON 36
 /**
  * @brief      Uncomment this line to use a button array for input
  *
  * If this is uncommented, the application will use a button array for user
  * input. The pins for the button array are defined below.
  */
#define ELS_USE_BUTTON_ARRAY
#endif

#if defined(ELS_USE_BUTTON_ARRAY)
#define ELS_PAD_H1 32
#define ELS_PAD_H2 33
#define ELS_PAD_H3 2

#define ELS_PAD_V1 15
#define ELS_PAD_V2 13
#define ELS_PAD_V3 12
#endif

  /**
   * @brief      Display settings
   *
   * This setting allows you to select what type of display you want to use.
   * The selection will hopefully grow as time goes on!
   *
   * @note       Options:
   *               - `SSD1306_128_64`: 128x64 oled
   *               - `ST7789_240_135`: 240x135 TFT
   */

#define SSD1306_128_64 0
#define ST7789_240_135 1
#define ELS_DISPLAY ST7789_240_135

#if ELS_DISPLAY == SSD1306_128_64
   // define this if you have a dedicated pin for the oled reset
#define PIN_DISPLAY_RESET -1
#endif

//! The number of pulses per revolution of the spindle encoder
#define ELS_SPINDLE_ENCODER_PPR 1200

/**
 * @brief      The number of spindle encoder pulses between speed updates.
 *
 * This value determines how frequently the spindle speed is recalculated.
 * A lower value will result in more frequent updates, but may be more
 * susceptible to noise.
 */
#define ELS_SPEED_COUNTS 300

 //! The number of pulses per revolution of the leadscrew stepper motor
#define ELS_LEADSCREW_STEPPER_PPR 400

/**
 * @brief      Uncomment this if your leadscrew direction is inverted to what is
 * expected. For example, if setting the right stop actually sets the left stop.
 */
#define ELS_INVERT_DIRECTION

#ifdef ELS_INVERT_DIRECTION
#define ELS_DIR_RIGHT 0
#define ELS_DIR_LEFT 1
#else
#define ELS_DIR_RIGHT 1
#define ELS_DIR_LEFT 0
#endif

 //! The gear ratio between the stepper motor and the leadscrew
#define ELS_GEARBOX_RATIO 2
//! The pitch of the leadscrew in millimeters
#define ELS_LEADSCREW_PITCH_MM ((float)(2.54))

/**
 * @brief      The number of steps the leadscrew stepper motor must take to move
 * the carriage by 1 millimeter.
 *
 * This is calculated as:
 * `(ELS_LEADSCREW_STEPPER_PPR * ELS_GEARBOX_RATIO) / ELS_LEADSCREW_PITCH_MM`
 */
#define ELS_LEADSCREW_STEPS_PER_MM \
  (float)((ELS_LEADSCREW_STEPPER_PPR * ELS_GEARBOX_RATIO) / ELS_LEADSCREW_PITCH_MM)

 // extra config options

 //! The jogging speed in millimeters per second
#define ELS_JOG_SPEED 250

/**
 * @brief      The delay between jog pulses in microseconds.
 *
 * This is calculated from the jog speed and the steps per millimeter.
 */
#define ELS_JOG_PULSE_DELAY   \
  ((float)US_PER_SECOND / \
   ((float)ELS_JOG_SPEED * (float)ELS_LEADSCREW_STEPS_PER_MM))

 /**
  * @brief      The unit mode the system should start up in.
  * @note       Options:
  *              - `GlobalUnitMode::METRIC`: Metric system
  *              - `GlobalUnitMode::IMPERIAL`: Imperial system
  */
#define DEFAULT_UNIT_MODE GlobalUnitMode::METRIC

  //! The default feed mode on startup.
#define DEFAULT_FEED_MODE GlobalFeedMode::FM_FEED

/**
 * @brief      The maximum allowable speed (in mm/s) for the leadscrew to
 * instantaneously start moving from 0.
 */
#define LEADSCREW_JERK 0.5

 //! The acceleration of the leadscrew in mm/s^2
#define LEADSCREW_ACCEL 90
//! The maximum speed of the leadscrew in mm/s
#define LEADSCREW_MAX_SPEED_MM 40
//! The maximum speed of the leadscrew in pulses per second
#define LEADSCREW_MAX_SPEED_PPS  LEADSCREW_MAX_SPEED_MM * ELS_LEADSCREW_STEPS_PER_MM

//! The timer interval for the leadscrew ISR in microseconds
#define LEADSCREW_TIMER_US 4

/**
 * @brief The initial delay between pulses in microseconds for the leadscrew
 * starting from 0.
 * @warning    Do not change this value directly. It is calculated from the
 * jerk value. To change the initial speed, modify `LEADSCREW_JERK`.
 */
#ifdef ACCEL_DISABLED
#define LEADSCREW_INITIAL_PULSE_DELAY_US 0
#define ACCEL_PULSE_SEC
#else
#define ACCEL_PULSE_SEC LEADSCREW_ACCEL * ELS_LEADSCREW_STEPS_PER_MM
#define LEADSCREW_INITIAL_PULSE_DELAY_US \
  ((float)US_PER_SECOND / ((float)LEADSCREW_JERK * (float)ELS_LEADSCREW_STEPS_PER_MM))
#endif


 //! @brief An array of metric thread pitches in mm/rev.
const float threadPitchMetric[] = { 0.35, 0.40, 0.45, 0.50, 0.60, 0.70, 0.80,
                                   1.00, 1.25, 1.50, 1.75, 2.00, 2.50, 3.00,
                                   3.50, 4.00, 4.50, 5.00, 5.50, 6.00 };
//! The default index for the metric thread pitch array.
#define DEFAULT_METRIC_THREAD_PITCH_IDX 8

//! @brief An array of metric feed pitches in mm/rev.
const float feedPitchMetric[] = { 0.05, 0.08, 0.10, 0.12, 0.15, 0.18, 0.20,
                                 0.23, 0.25, 0.28, 0.30, 0.35, 0.40, 0.45,
                                 0.50, 0.55, 0.60, 0.65, 0.70, 0.75 };
//! The default index for the metric feed pitch array.
#define DEFAULT_METRIC_FEED_PITCH_IDX 8

/**
 * @brief      An array of imperial thread pitches in TPI (threads per inch).
 * @note       These are retained as floats to allow for partial TPI values.
 */
const float threadPitchImperial[] = { 80, 72, 64, 56, 48, 44, 40, 36, 32, 28,
                                     24, 20, 18, 16, 14, 13, 12, 11, 10, 9 };
//! The default index for the imperial thread pitch array.
#define DEFAULT_IMPERIAL_THREAD_PITCH_IDX 8

//! @brief An array of imperial feed pitches in thou/rev (thousandths of an inch per revolution).
const float feedPitchImperial[] = {
    0.002, 0.003, 0.004, 0.005, 0.006, 0.007, 0.008, 0.009, 0.010, 0.011,
    0.012, 0.014, 0.016, 0.018, 0.020, 0.022, 0.024, 0.026, 0.028, 0.030 };
//! The default index for the imperial feed pitch array.
#define DEFAULT_IMPERIAL_FEED_PITCH_IDX 8

#endif