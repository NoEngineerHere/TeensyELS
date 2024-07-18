// Libraries
#include <AbleButtons.h>
#include <SPI.h>
#include <Wire.h>

#include "config.h"
#include "display.h"
#include "globalstate.h"

#define CW digitalWriteFast(3, HIGH);  // direction output pin
#define CCW digitalWriteFast(3, LOW);  // direction output pin

using Button = AblePullupCallbackButton;  // AblePullupButton; // Using basic
                                          // pull-up resistor circuit.
using ButtonList =
    AblePullupCallbackButtonList;  // AblePullupButtonList; // Using basic
                                   // pull-up button list.

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
Spindle spindle;
Leadscrew leadscrew(&spindle);

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

bool enabled = false;
bool lockState = true;
bool readyToThread = false;
bool synced = false;
bool hasPreviouslySynced = false;
int jogRate;
boolean jogLeftHeld;
boolean jogRightHeld;

// UI Values

void Achange();
void Bchange();
void modeHandle();

void printState() {
  Serial.println();
  Serial.print("Drive Mode: ");
  switch (globalState->getFeedMode()) {
    case GlobalFeedMode::FEED:
      Serial.println("Feed");
      break;
    case GlobalFeedMode::THREAD:
      Serial.println("Thread");
      break;
  }
  Serial.print("Motion Mode: ");
  switch (globalState->getMotionMode()) {
    case GlobalMotionMode::DISABLED:
      Serial.println("Disabled");
      break;
    case GlobalMotionMode::JOG:
      Serial.println("Jog");
      break;
    case GlobalMotionMode::ENABLED:
      Serial.println("Enabled");
      break;
  }
  Serial.print("Lock State: ");
  Serial.println(lockState ? "True" : "False");
  Serial.print("Ready to Thread: ");
  Serial.println(readyToThread ? "True" : "False");
  Serial.print("Synced: ");
  Serial.println(synced ? "True" : "False");
  Serial.print("Feed Mode: ");
  switch (globalState->getFeedMode()) {
    case GlobalFeedMode::FEED:
      Serial.println("Feed");
      break;
    case GlobalFeedMode::THREAD:
      Serial.println("Thread");
      break;
  }
  Serial.print("Feed Select: ");
  switch (globalState->getFeedMode()) {
    case GlobalFeedMode::FEED:
      Serial.println(globalState->getCurrentFeedPitch());
      break;
    case GlobalFeedMode::THREAD:
      Serial.println(globalState->getCurrentFeedPitch());
      break;
  }
  Serial.print("Unit Mode: ");
  switch (globalState->getUnitMode()) {
    case GlobalUnitMode::METRIC:
      Serial.println("Metric");
      break;
    case GlobalUnitMode::IMPERIAL:
      Serial.println("Imperial");
      break;
  }
  Serial.print("Jog Unsynced Count: ");
  Serial.println(jogUnsyncedCount);
  Serial.print("Expected Position: ");
  Serial.println(leadscrew.getExpectedPosition());
  Serial.print("Current Position: ");
  Serial.println(leadscrew.getCurrentPosition());
  Serial.print("Last Pulse: ");
  Serial.println(lastPulse);
  Serial.print("Pulses Back to Sync: ");
  Serial.println(pulsesBackToSync);
  Serial.print("Has previously synced: ");
  Serial.println(hasPreviouslySynced);
  Serial.print("micros: ");
  Serial.println(micros());
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

  // Display Initalisation

  display.init();

  display.update(lockState, enabled);

  printState();
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
    synced = false;
    leadscrew.incrementCurrentPosition(-1);
    jogUnsyncedCount--;
    if (hasPreviouslySynced) {
      pulsesBackToSync++;
    }

  } else if (jogRightHeld) {
    globalState->setMotionMode(GlobalMotionMode::JOG);
    synced = false;
    leadscrew.incrementCurrentPosition(1);
    jogUnsyncedCount++;
    if (hasPreviouslySynced) {
      pulsesBackToSync--;
    }
  }
}

void loop() {
  // print out current state of the machine
  keyPad.handle();
  buttonHeldHandle();

  uint32_t currentMicros = micros();
  static uint32_t lastPrint = currentMicros;

  if (currentMicros - lastPrint > 1000 * 1000) {
    printState();
    lastPrint = currentMicros;
  }

  int mod = (int)((ELS_SPINDLE_ENCODER_PPR * leadscrew.getRatio()) /
                  ELS_LEADSCREW_PITCH_MM);
  int positionError =
      leadscrew.getExpectedPosition() - leadscrew.getCurrentPosition();
  int directionIncrement = 0;

  if (positionError > 0) {
    CW;
    directionIncrement = 1;
  } else {
    CCW;
    directionIncrement = -1;
  }

  switch (globalState->getMotionMode()) {
    case GlobalMotionMode::DISABLED:
      // ignore the spindle, pretend we're in sync all the time
      leadscrew.resetCurrentPosition();
      break;
    case GlobalMotionMode::JOG:
      // only send a pulse if we haven't sent one recently
      if (currentMicros - lastPulse < JOG_PULSE_DELAY_US) {
        break;
      }
      // if jog is complete go back to disabled motion mode
      if (leadscrew.getCurrentPosition() == leadscrew.getExpectedPosition()) {
        Serial.println("Jog Complete");
        globalState->setMotionMode(GlobalMotionMode::DISABLED);
      }
      // jogging is a fixed rate based on JOG_PULSE_DELAY_US
      digitalWriteFast(stp, HIGH);
      delayMicroseconds(2);
      digitalWriteFast(stp, LOW);
      delayMicroseconds(2);
      lastPulse = currentMicros;

      break;
    case GlobalMotionMode::ENABLED:
      // attempt to keep in sync with the leadscrew
      accumulator += ELS_LEADSCREW_STEPS_PER_MM * leadscrew.getRatio() /
                     ELS_LEADSCREW_STEPPER_PPR;

      while (accumulator >= 0) {  // sends required motor steps to motor

        accumulator--;

        // todo try and leverage hardware PWM timer to send set amount of
        // pulses
        digitalWriteFast(stp, HIGH);
        delayMicroseconds(2);
        digitalWriteFast(stp, LOW);
        delayMicroseconds(2);
        leadscrew.incrementCurrentPosition(directionIncrement);
      }
      lastPulse = currentMicros;

      break;
  }
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
  printState();

  if (globalState->getFeedMode() == GlobalFeedMode::FEED ||
      ((micros() - lastPulse) > safetyDelay && lockState == false)) {
    if (event == Button::PRESSED_EVENT) {
      globalState->nextFeedPitch();
      display.update(lockState, enabled);
    }
  }
}

void rateDecCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // decreases feedSelect variable on button press
  Serial.println("Rate Dec Call");
  printState();
  if (globalState->getFeedMode() == GlobalFeedMode::FEED ||
      ((micros() - lastPulse) > safetyDelay && lockState == false)) {
    if (event == Button::PRESSED_EVENT) {
      globalState->prevFeedPitch();

      display.update(lockState, enabled);
    }
  }
}

void halfNutCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // sets readyToThread to true on button hold
  Serial.println("Half Nut Call");
  printState();
  if (event == Button::PRESSED_EVENT &&
      globalState->getFeedMode() == GlobalFeedMode::THREAD) {
    readyToThread = true;
    synced = true;
    hasPreviouslySynced = true;
    pulsesBackToSync = 0;
  }
}

void enaCall(Button::CALLBACK_EVENT event,
             uint8_t) {  // toggles enabled variable on button press
  Serial.println("Enable Call");
  printState();
  if (event == Button::PRESSED_EVENT) {
    if (enabled == true) {
      enabled = false;
    }

    else {
      // set jogUnsyncedCount to remainder of spindle pulses based on current
      // feed rate

      int mod = (int)((ELS_SPINDLE_ENCODER_PPR * leadscrew.getRatio()) /
                      ELS_LEADSCREW_PITCH_MM);
      jogUnsyncedCount = jogUnsyncedCount % mod;

      Serial.print("mod:");
      Serial.println(mod);
      Serial.print("reenabling!");
      Serial.print("Jog Unsynced Count: ");
      Serial.println(jogUnsyncedCount);
      enabled = true;
    }

    display.update(lockState, enabled);
  }
}

void lockCall(Button::CALLBACK_EVENT event,
              uint8_t) {  // toggles lockState variable on button press
  Serial.println("Lock Call");
  printState();
  if (event == Button::PRESSED_EVENT) {
    lockState = !lockState;

    display.update(lockState, enabled);
  }
}

void threadSyncCall(Button::CALLBACK_EVENT event,
                    uint8_t) {  // toggles sync status on button hold
  Serial.println("Thread Sync Call");
  printState();
  if (event == Button::PRESSED_EVENT) {
    if (synced == false) {
      lockState = true;
      synced = true;
    }

    else {
      synced = false;
    }
  }
}

void modeCycleCall(
    Button::CALLBACK_EVENT event,
    uint8_t) {  // toggles between thread / feed modes on button press
  Serial.println("Mode Cycle Call");
  printState();
  if (event == Button::PRESSED_EVENT && (micros() - lastPulse) > safetyDelay &&
      lockState == false) {
    switch (globalState->getFeedMode()) {
      case GlobalFeedMode::FEED:
        globalState->setFeedMode(GlobalFeedMode::THREAD);
        break;
      case GlobalFeedMode::THREAD:
        globalState->setFeedMode(GlobalFeedMode::FEED);
        break;
    }

    display.update(lockState, enabled);
  }
}

void jogLeftCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // jogs left on button hold (one day)
  Serial.println("Jog Left Call");
  printState();

  // this event doesn't repeat, so set a flag to keep track of it
  if (event == Button::HELD_EVENT && enabled == false) {
    jogLeftHeld = true;
  } else if (event == Button::RELEASED_EVENT) {
    jogLeftHeld = false;
  }
}

void jogRightCall(Button::CALLBACK_EVENT event,
                  uint8_t) {  /// jogs right on button hold (one day)
  Serial.println("Jog Right Call");
  printState();

  // this event doesn't repeat, so set a flag to keep track of it
  if (event == Button::HELD_EVENT && enabled == false) {
    jogRightHeld = true;
  } else if (event == Button::RELEASED_EVENT) {
    jogRightHeld = false;
  }
}
