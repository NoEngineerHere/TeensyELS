// Libraries
#include <AbleButtons.h>
#include <SPI.h>
#include <Wire.h>
#include <globalstate.h>
#include <leadscrew/leadscrew.h>
#include <spindle.h>

#include "config.h"
#include "display.h"

using Button = AblePullupCallbackDoubleClickerButton;
using ButtonList = AblePullupCallbackDoubleClickerButtonList;

void rateIncCall(Button::CALLBACK_EVENT, unsigned char);
void rateDecCall(Button::CALLBACK_EVENT, unsigned char);
void modeCycleCall(Button::CALLBACK_EVENT, unsigned char);
void threadSyncCall(Button::CALLBACK_EVENT, unsigned char);
void halfNutCall(Button::CALLBACK_EVENT, unsigned char);
void enaCall(Button::CALLBACK_EVENT, unsigned char);
void lockCall(Button::CALLBACK_EVENT, unsigned char);
void jogLeftCall(Button::CALLBACK_EVENT, unsigned char);
void jogRightCall(Button::CALLBACK_EVENT, unsigned char);

Button rateInc(4, rateIncCall);
Button rateDec(5, rateDecCall);
Button modeCycle(6, modeCycleCall);
Button threadSync(7, threadSyncCall);
Button halfNut(8, halfNutCall);
Button ena(9, enaCall);
Button lock(10, lockCall);
Button jogLeft(24, jogLeftCall);
Button jogRight(25, jogRightCall);

Button* keys[] = {&rateInc, &rateDec, &modeCycle, &threadSync, &halfNut,
                  &ena,     &lock,    &jogLeft,   &jogRight

};

ButtonList keyPad(keys);
Button setHeldTime(100);

Display display;
GlobalState* globalState = GlobalState::getInstance();

int EncoderMatrix[16] = {
    0, -1, 1, 2, 1, 0,  2, -1, -1,
    2, 0,  1, 2, 1, -1, 0};  // encoder output matrix, output = X (old) * 4 + Y
                             // (new)

volatile int oldPos;
volatile int newPos;

const int pinA = 14;  // encoder pin A
const int pinB = 15;  // encoder pin B
const int stp = 2;    // step output pin
const int dir = 3;    // direction output pin

const int safetyDelay = 100000;

// todo maybe integer maths if this is too slow
float accumulator;

int jogUnsyncedCount;  // when we jog in thread mode we may be "in between"
                       // threads, this stores how many pulses we require to be
                       // back in line
int pulsesBackToSync;  // when jogging in thread mode, this stores how many
                       // pulses we need to get back in sync i.e: the stop point
                       // of the thread
long long lastPulse;

bool lockState = true;
bool readyToThread = false;
boolean jogLeftHeld;
boolean jogRightHeld;

Spindle spindle;
Leadscrew leadscrew(&spindle);

// UI Values

void Achange();
void Bchange();
void modeHandle();

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

  pinMode(14, INPUT_PULLUP);  // encoder pin 1
  pinMode(15, INPUT_PULLUP);  // encoder pin 2
  pinMode(2, OUTPUT);         // step output pin
  pinMode(3, OUTPUT);         // direction output pin
  pinMode(4, INPUT_PULLUP);   // rate Inc
  pinMode(5, INPUT_PULLUP);   // rate Dec
  pinMode(6, INPUT_PULLUP);   // mode cycle
  pinMode(7, INPUT_PULLUP);   // thread sync
  pinMode(8, INPUT_PULLUP);   // half nut
  pinMode(9, INPUT_PULLUP);   // enable toggle
  pinMode(10, INPUT_PULLUP);  // lock toggle
  pinMode(24, INPUT_PULLUP);  // jog left
  pinMode(25, INPUT_PULLUP);  // jog right

  // Interupts

  attachInterrupt(digitalPinToInterrupt(pinA), Achange,
                  CHANGE);  // encoder channel A interrupt
  attachInterrupt(digitalPinToInterrupt(pinB), Bchange,
                  CHANGE);  // encoder channel B interrupt

#ifdef DEBUG
  debug.begin(SerialUSB1);
  halt_cpu();
#endif

  // Display Initalisation

  display.init();

  display.update(lockState);

  globalState->printState();
}

void buttonHeldHandle() {
  int currentMicros = micros();

  int positionError =
      leadscrew.getExpectedPosition() - leadscrew.getCurrentPosition();

  if (positionError > 5) {
    return;
  }

  if (jogLeftHeld) {
    globalState->setMotionMode(GlobalMotionMode::JOG);
    globalState->setThreadSyncState(GlobalThreadSyncState::UNSYNC);
    leadscrew.incrementCurrentPosition(-1);

  } else if (jogRightHeld) {
    globalState->setMotionMode(GlobalMotionMode::JOG);
    globalState->setThreadSyncState(GlobalThreadSyncState::UNSYNC);
    leadscrew.incrementCurrentPosition(1);
  }
}

void loop() {
  // print out current state of the machine
  keyPad.handle();
  buttonHeldHandle();

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

    lastPrint = currentMicros;
  }

  leadscrew.update();
}

void Achange() {  // validates encoder pulses, adds to pulse variable

  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(pinA));
  bitWrite(newPos, 1,
           digitalReadFast(pinB));  // adds A to B, converts to integer
  spindle.incrementCurrentPosition(EncoderMatrix[(oldPos * 4) + newPos]);
}

void Bchange() {  // validates encoder pulses, adds to pulse variable

  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(pinA));
  bitWrite(newPos, 1,
           digitalReadFast(pinB));  // adds A to B, converts to integer
  spindle.incrementCurrentPosition(
      EncoderMatrix[(oldPos * 4) + newPos]);  // assigns value from encoder
                                              // matrix to determine validity
                                              // and direction of encoder pulse
}

void rateIncCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // increases feedSelect variable on button press
  Serial.println("Rate Inc Call");

  if ((micros() - lastPulse) > safetyDelay && lockState == false) {
    if (event == Button::SINGLE_CLICKED_EVENT) {
      globalState->nextFeedPitch();
      leadscrew.setRatio(globalState->getCurrentFeedPitch());
      display.update(lockState);
    }
  }
}

void rateDecCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // decreases feedSelect variable on button press
  Serial.println("Rate Dec Call");

  if (((micros() - lastPulse) > safetyDelay && lockState == false)) {
    if (event == Button::PRESSED_EVENT) {
      globalState->prevFeedPitch();
      leadscrew.setRatio(globalState->getCurrentFeedPitch());
      display.update(lockState);
    }
  }
}

void halfNutCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // sets readyToThread to true on button hold
  Serial.println("Half Nut Call");

  if (event == Button::SINGLE_CLICKED_EVENT &&
      globalState->getFeedMode() == GlobalFeedMode::THREAD) {
    readyToThread = true;
    globalState->setMotionMode(GlobalMotionMode::ENABLED);
    globalState->setThreadSyncState(GlobalThreadSyncState::SYNC);
    pulsesBackToSync = 0;
  }
}

void enaCall(Button::CALLBACK_EVENT event,
             uint8_t) {  // toggles enabled variable on button press
  Serial.println("Enable Call");

  if (event == Button::PRESSED_EVENT) {
    if (globalState->getMotionMode() == GlobalMotionMode::ENABLED) {
      globalState->setMotionMode(GlobalMotionMode::DISABLED);
    }

    else {
      globalState->setMotionMode(GlobalMotionMode::ENABLED);
    }

    display.update(lockState);
  }
}

void lockCall(Button::CALLBACK_EVENT event,
              uint8_t) {  // toggles lockState variable on button press
  Serial.println("Lock Call");

  if (event == Button::PRESSED_EVENT) {
    lockState = !lockState;

    display.update(lockState);
  }
}

void threadSyncCall(Button::CALLBACK_EVENT event,
                    uint8_t) {  // toggles sync status on button hold
  Serial.println("Thread Sync Call");

  if (event == Button::PRESSED_EVENT) {
    if (globalState->getMotionMode() == GlobalMotionMode::ENABLED) {
      globalState->setThreadSyncState(GlobalThreadSyncState::UNSYNC);
    } else {
      globalState->setThreadSyncState(GlobalThreadSyncState::SYNC);
    }
  }
}

void modeCycleCall(
    Button::CALLBACK_EVENT event,
    uint8_t) {  // toggles between thread / feed modes on button press
  Serial.print("Mode Cycle Call");
  Serial.println(event);

  if ((micros() - lastPulse) < safetyDelay || lockState == true) {
    return;
  }

  // holding mode button swaps between metric and imperial
  if (event == Button::HELD_EVENT) {
    switch (globalState->getUnitMode()) {
      case GlobalUnitMode::METRIC:
        globalState->setUnitMode(GlobalUnitMode::IMPERIAL);
        break;
      case GlobalUnitMode::IMPERIAL:
        globalState->setUnitMode(GlobalUnitMode::METRIC);
        break;
    }
  }
  // pressing mode button swaps between feed and thread
  else if (event == Button::SINGLE_CLICKED_EVENT) {
    switch (globalState->getFeedMode()) {
      case GlobalFeedMode::FEED:
        globalState->setFeedMode(GlobalFeedMode::THREAD);
        break;
      case GlobalFeedMode::THREAD:
        globalState->setFeedMode(GlobalFeedMode::FEED);
        break;
    }
  }
  leadscrew.setRatio(globalState->getCurrentFeedPitch());
  display.update(lockState);
}

void jogLeftCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // jogs left on button hold (one day)
  Serial.println("Jog Left Call");

  // only allow jogging when disabled
  // this event doesn't repeat, so set a flag to keep track of it
  if (event == Button::HELD_EVENT &&
      globalState->getMotionMode() == GlobalMotionMode::DISABLED) {
    globalState->setMotionMode(GlobalMotionMode::JOG);
    jogLeftHeld = true;
    display.update(lockState);
  } else if (event == Button::RELEASED_EVENT) {
    jogLeftHeld = false;
    globalState->setMotionMode(GlobalMotionMode::DISABLED);
    display.update(lockState);
  }
}

void jogRightCall(Button::CALLBACK_EVENT event,
                  uint8_t) {  /// jogs right on button hold (one day)
  Serial.println("Jog Right Call");

  // only allow jogging when disabled
  // this event doesn't repeat, so set a flag to keep track of it
  if (event == Button::HELD_EVENT &&
      globalState->getMotionMode() == GlobalMotionMode::DISABLED) {
    globalState->setMotionMode(GlobalMotionMode::JOG);
    jogRightHeld = true;
    display.update(lockState);
  } else if (event == Button::RELEASED_EVENT) {
    jogRightHeld = false;
    globalState->setMotionMode(GlobalMotionMode::DISABLED);
    display.update(lockState);
  }
}
