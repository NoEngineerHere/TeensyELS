// Libraries

#include <SPI.h>
#include <Wire.h>
#include <globalstate.h>
#include <leadscrew.h>
#include <spindle.h>

#include "buttons.h"
#include "config.h"
#include "display.h"

IntervalTimer timer;

int EncoderMatrix[16] = {
    0, -1, 1, 2, 1, 0,  2, -1, -1,
    2, 0,  1, 2, 1, -1, 0};  // encoder output matrix, output = X (old) * 4 + Y
                             // (new)

volatile int oldPos;
volatile int newPos;

GlobalState* globalState = GlobalState::getInstance();
Spindle spindle;
Leadscrew leadscrew(&spindle);
ButtonHandler keyPad(&spindle, &leadscrew);
Display display(&spindle, &leadscrew);

void Achange();
void Bchange();

// have to handle the leadscrew updates in a timer callback so we can update the
// screen independently without losing pulses
void timerCallback() { leadscrew.update(); }

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

  // Interupts

  attachInterrupt(digitalPinToInterrupt(ELS_SPINDLE_ENCODER_A), Achange,
                  CHANGE);
  attachInterrupt(digitalPinToInterrupt(ELS_SPINDLE_ENCODER_B), Bchange,
                  CHANGE);

  // Display Initalisation

  display.init();

  display.update();

  globalState->printState();

  timer.begin(timerCallback, 50);
}

void loop() {
  keyPad.handle();

  uint32_t currentMicros = micros();
  static uint32_t lastPrint = currentMicros;

  if (currentMicros - lastPrint > 1000 * 500) {
    globalState->printState();
    Serial.print("Micros: ");
    Serial.println(currentMicros);
    Serial.print("Leadscrew position: ");
    Serial.println(leadscrew.getCurrentPosition());
    Serial.print("Leadscrew expected position: ");
    Serial.println(leadscrew.getExpectedPosition());
    Serial.print("Leadscrew ratio: ");
    Serial.println(leadscrew.getRatio());
    Serial.print("Spindle position: ");
    Serial.println(spindle.getCurrentPosition());
    Serial.print("Spindle velocity: ");
    Serial.println(spindle.getEstimatedVelocityInRPM());
    Serial.print("Spindle velocity pulses: ");
    Serial.println(spindle.getEstimatedVelocityInPulsesPerSecond());

    lastPrint = currentMicros;
  }

  display.update();
}

void Achange() {  // validates encoder pulses, adds to pulse variable

  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(ELS_SPINDLE_ENCODER_A));
  bitWrite(newPos, 1,
           digitalReadFast(
               ELS_SPINDLE_ENCODER_B));  // adds A to B, converts to integer
  spindle.incrementCurrentPosition(EncoderMatrix[(oldPos * 4) + newPos]);
}

void Bchange() {  // validates encoder pulses, adds to pulse variable

  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(ELS_SPINDLE_ENCODER_A));
  bitWrite(newPos, 1,
           digitalReadFast(
               ELS_SPINDLE_ENCODER_B));  // adds A to B, converts to integer
  spindle.incrementCurrentPosition(
      EncoderMatrix[(oldPos * 4) + newPos]);  // assigns value from encoder
                                              // matrix to determine validity
                                              // and direction of encoder pulse
}
