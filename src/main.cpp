// Libraries

#include <SPI.h>
#include <Wire.h>
#include <globalstate.h>
#include <leadscrew.h>
#include <leadscrew_io_impl.h>
#include <spindle.h>

#include "buttons.h"
#include "config.h"
#include "display.h"

IntervalTimer timer;

GlobalState* globalState = GlobalState::getInstance();
#ifdef ELS_SPINDLE_DRIVEN
Spindle spindle;
#else
Spindle spindle(ELS_SPINDLE_ENCODER_A, ELS_SPINDLE_ENCODER_B);
#endif
LeadscrewIOImpl leadscrewIOImpl;
Leadscrew leadscrew(&spindle, &leadscrewIOImpl,
                    LEADSCREW_INITIAL_PULSE_DELAY_US,
                    LEADSCREW_PULSE_DELAY_STEP_US, ELS_LEADSCREW_STEPPER_PPR*ELS_GEARBOX_RATIO,
                    ELS_LEADSCREW_PITCH_MM, ELS_SPINDLE_ENCODER_PPR);
ButtonHandler keyPad(&spindle, &leadscrew);
Display display(&spindle, &leadscrew);

// have to handle the leadscrew updates in a timer callback so we can update the
// screen independently without losing pulses
void timerCallback() {
  spindle.update();
  leadscrew.update();
}

void setup() {
  // config - compile time checks for safety
  CHECK_BOUNDS(DEFAULT_METRIC_THREAD_PITCH_IDX, threadPitchMetric,
               "DEFAULT_METRIC_THREAD_PITCH_IDX out of bounds");
  CHECK_BOUNDS(DEFAULT_METRIC_FEED_PITCH_IDX, feedPitchMetric,
               "DEFAULT_METRIC_FEED_PITCH_IDX out of bounds");
  CHECK_BOUNDS(DEFAULT_IMPERIAL_THREAD_PITCH_IDX, threadPitchImperial,
               "DEFAULT_IMPERIAL_THREAD_PITCH_IDX out of bounds");
  CHECK_BOUNDS(DEFAULT_IMPERIAL_FEED_PITCH_IDX, feedPitchImperial,
               "DEFAULT_IMPERIAL_FEED_PITCH_IDX out of bounds");

  // Pinmodes

#ifndef ELS_SPINDLE_DRIVEN
  pinMode(ELS_SPINDLE_ENCODER_A, INPUT_PULLUP);  // encoder pin 1
  pinMode(ELS_SPINDLE_ENCODER_B, INPUT_PULLUP);  // encoder pin 2
#endif
  pinMode(ELS_LEADSCREW_STEP, OUTPUT);              // step output pin
  pinMode(ELS_LEADSCREW_DIR, OUTPUT);               // direction output pin
  pinMode(ELS_RATE_INCREASE_BUTTON, INPUT_PULLUP);  // rate Inc
  pinMode(ELS_RATE_DECREASE_BUTTON, INPUT_PULLUP);  // rate Dec
  pinMode(ELS_MODE_CYCLE_BUTTON, INPUT_PULLUP);     // mode cycle
  pinMode(ELS_THREAD_SYNC_BUTTON, INPUT_PULLUP);    // thread sync
  pinMode(ELS_HALF_NUT_BUTTON, INPUT_PULLUP);       // half nut
  pinMode(ELS_ENABLE_BUTTON, INPUT_PULLUP);         // enable toggle
  pinMode(ELS_LOCK_BUTTON, INPUT_PULLUP);           // lock toggle
  pinMode(ELS_JOG_LEFT_BUTTON, INPUT_PULLUP);       // jog left
  pinMode(ELS_JOG_RIGHT_BUTTON, INPUT_PULLUP);      // jog right

  // Display Initalisation

  display.init();

  leadscrew.setRatio(globalState->getCurrentFeedPitch());

  display.update();

  timer.begin(timerCallback, LEADSCREW_TIMER_US);

  delay(2000);

  Serial.print("Initial pulse delay: ");
  Serial.println(LEADSCREW_INITIAL_PULSE_DELAY_US);
  Serial.print("Pulse delay step: ");
  Serial.println(LEADSCREW_PULSE_DELAY_STEP_US);
}

void loop() {
  keyPad.handle();

  static elapsedMicros lastPrint;
  if (lastPrint > 1000 * 500) {
    lastPrint = 0;
    globalState->printState();
    Serial.print("Micros: ");
    Serial.println(micros());
    leadscrew.printState();
    Serial.print("Spindle position: ");
    Serial.println(spindle.getCurrentPosition());
    Serial.print("Spindle velocity: ");
    Serial.println(spindle.getEstimatedVelocityInRPM());
    Serial.print("Spindle velocity pulses: ");
    Serial.println(spindle.getEstimatedVelocityInPulsesPerSecond());
    keyPad.printState();
  }

  display.update();
}
