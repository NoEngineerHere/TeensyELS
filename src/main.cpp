// Libraries
#include <AbleButtons.h>
#include <SPI.h>
#include <Wire.h>

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
int feedSelect = 8;
int jogRate;
boolean jogLeftHeld;
boolean jogRightHeld;

// UI Values
// these are how many leadscrew pulses we need to send per spindle pulse
const float threadPitch[20] = {0.35, 0.40, 0.45, 0.50, 0.60, 0.70, 0.80,
                               1.00, 1.25, 1.50, 1.75, 2.00, 2.50, 3.00,
                               3.50, 4.00, 4.50, 5.00, 5.50, 6.00};
const float feedPitch[20] = {0.05, 0.08, 0.10, 0.12, 0.15, 0.18, 0.20,
                             0.23, 0.25, 0.28, 0.30, 0.35, 0.40, 0.45,
                             0.50, 0.55, 0.60, 0.65, 0.70, 0.75};

void Achange();
void Bchange();
void modeHandle();

void printState() {
  Serial.println();
  Serial.print("Drive Mode: ");
  switch (globalState->getMode()) {
    case GlobalMajorMode::FEED:
      Serial.println("Feed");
      break;
    case GlobalMajorMode::THREAD:
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
  Serial.print("Feed Select: ");
  switch (globalState->getMode()) {
    case GlobalMajorMode::FEED:
      Serial.println(feedPitch[feedSelect]);
      break;
    case GlobalMajorMode::THREAD:
      Serial.println(threadPitch[feedSelect]);
      break;
  }
  Serial.print("Jog Unsynced Count: ");
  Serial.println(jogUnsyncedCount);
  Serial.print("Expected Position: ");
  Serial.println(globalState->getExpectedPosition());
  Serial.print("Current Position: ");
  Serial.println(globalState->getCurrentPosition());
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
  display.update(globalState->getMode() == GlobalMajorMode::THREAD
                     ? threadPitch[feedSelect]
                     : feedPitch[feedSelect],
                 lockState, enabled);

  printState();
}

void buttonHeldHandle() {
  int currentMicros = micros();

  int positionError =
      globalState->getExpectedPosition() - globalState->getCurrentPosition();

  if (positionError > 5) {
    return;
  }

  if (jogLeftHeld) {
    globalState->setMotionMode(GlobalMotionMode::JOG);
    synced = false;
    globalState->incrementExpectedPosition(-1);
    jogUnsyncedCount--;
    if (hasPreviouslySynced) {
      pulsesBackToSync++;
    }

  } else if (jogRightHeld) {
    globalState->setMotionMode(GlobalMotionMode::JOG);
    synced = false;
    globalState->incrementExpectedPosition(1);
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

  float ratio = (globalState->getMode() == GlobalMajorMode::THREAD
                     ? threadPitch[feedSelect]
                     : feedPitch[feedSelect]);
  int mod = (int)((ELS_SPINDLE_ENCODER_PPR * ratio) / ELS_LEADSCREW_PITCH_MM);
  int positionError =
      globalState->getExpectedPosition() - globalState->getCurrentPosition();
  int directionIncrement = 0;

  // Perfectly in sync, nothing to change
  if (positionError == 0) {
    return;
  }

  if (positionError > 0) {
    CW;
    directionIncrement = 1;
  } else {
    CCW;
    directionIncrement = -1;
  }

  switch (globalState->getMotionMode()) {
    case GlobalMotionMode::DISABLED:
      // ignore the spindle
      break;
    case GlobalMotionMode::JOG:
      // only send a pulse if we haven't sent one recently
      if (currentMicros - lastPulse < JOG_PULSE_DELAY_US) {
        break;
      }
      // if jog is complete go back to disabled motion mode
      if (globalState->getExpectedPosition() -
              globalState->getCurrentPosition() ==
          0) {
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
      accumulator +=
          ELS_LEADSCREW_STEPS_PER_MM * ratio / ELS_LEADSCREW_STEPPER_PPR;

      while (accumulator >= 0) {  // sends required motor steps to motor

        accumulator--;

        // todo try and leverage hardware PWM timer to send set amount of
        // pulses
        digitalWriteFast(stp, HIGH);
        delayMicroseconds(2);
        digitalWriteFast(stp, LOW);
        delayMicroseconds(2);
      }
      lastPulse = currentMicros;
      globalState->incrementCurrentPosition(directionIncrement);
      break;
  }
}

void Achange() {  // validates encoder pulses, adds to pulse variable

  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(pinA));
  bitWrite(newPos, 1,
           digitalReadFast(pinB));  // adds A to B, converts to integer
  globalState->incrementExpectedPosition(EncoderMatrix[(oldPos * 4) + newPos]);
}

void Bchange() {  // validates encoder pulses, adds to pulse variable

  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(pinA));
  bitWrite(newPos, 1,
           digitalReadFast(pinB));  // adds A to B, converts to integer
  globalState->incrementExpectedPosition(
      EncoderMatrix[(oldPos * 4) + newPos]);  // assigns value from encoder
                                              // matrix to determine validity
                                              // and direction of encoder pulse
}

void rateIncCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // increases feedSelect variable on button press
  Serial.println("Rate Inc Call");
  printState();

  if (globalState->getMode() == GlobalMajorMode::FEED ||
      ((micros() - lastPulse) > safetyDelay && lockState == false)) {
    if (event == Button::PRESSED_EVENT) {
      if (feedSelect < 19) {
        feedSelect++;
      }

      else {
        feedSelect = 0;
      }
      display.update(globalState->getMode() == GlobalMajorMode::THREAD
                         ? threadPitch[feedSelect]
                         : feedPitch[feedSelect],
                     lockState, enabled);
    }
  }
}

void rateDecCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // decreases feedSelect variable on button press
  Serial.println("Rate Dec Call");
  printState();
  if (globalState->getMode() == GlobalMajorMode::FEED ||
      ((micros() - lastPulse) > safetyDelay && lockState == false)) {
    if (event == Button::PRESSED_EVENT) {
      if (feedSelect > 0) {
        feedSelect--;
      }

      else {
        feedSelect = 19;
      }

      display.update(globalState->getMode() == GlobalMajorMode::THREAD
                         ? threadPitch[feedSelect]
                         : feedPitch[feedSelect],
                     lockState, enabled);
    }
  }
}

void halfNutCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // sets readyToThread to true on button hold
  Serial.println("Half Nut Call");
  printState();
  if (event == Button::PRESSED_EVENT &&
      globalState->getMode() == GlobalMajorMode::THREAD) {
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
      float ratio = (globalState->getMode() == GlobalMajorMode::THREAD
                         ? threadPitch[feedSelect]
                         : feedPitch[feedSelect]);
      int mod =
          (int)((ELS_SPINDLE_ENCODER_PPR * ratio) / ELS_LEADSCREW_PITCH_MM);
      jogUnsyncedCount = jogUnsyncedCount % mod;

      Serial.print("mod:");
      Serial.println(mod);
      Serial.print("reenabling!");
      Serial.print("Jog Unsynced Count: ");
      Serial.println(jogUnsyncedCount);
      enabled = true;
    }

    display.update(globalState->getMode() == GlobalMajorMode::THREAD
                       ? threadPitch[feedSelect]
                       : feedPitch[feedSelect],
                   lockState, enabled);
  }
}

void lockCall(Button::CALLBACK_EVENT event,
              uint8_t) {  // toggles lockState variable on button press
  Serial.println("Lock Call");
  printState();
  if (event == Button::PRESSED_EVENT) {
    lockState = !lockState;

    display.update(globalState->getMode() == GlobalMajorMode::THREAD
                       ? threadPitch[feedSelect]
                       : feedPitch[feedSelect],
                   lockState, enabled);
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
    switch (globalState->getMode()) {
      case GlobalMajorMode::FEED:
        globalState->setMode(GlobalMajorMode::THREAD);
        break;
      case GlobalMajorMode::THREAD:
        globalState->setMode(GlobalMajorMode::FEED);
        break;
    }

    display.update(globalState->getMode() == GlobalMajorMode::THREAD
                       ? threadPitch[feedSelect]
                       : feedPitch[feedSelect],
                   lockState, enabled);
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
