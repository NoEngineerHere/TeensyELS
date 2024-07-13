// Libraries
#include <AbleButtons.h>
#include <SPI.h>
#include <Wire.h>

#include "display.h"

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

Button *keys[] = {&rateInc, &rateDec, &modeCycle, &threadSync, &halfNut,
                  &ena,     &lock,    &jogLeft,   &jogRight

};

ButtonList keyPad(keys);
Button setHeldTime(100);

Display display;

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
volatile int pulseCount;
long long lastPulse;

bool jogMode = true;
bool driveMode = true;  // select threading mode (true) or feeding mode (false)
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
  Serial.println(driveMode ? "Thread" : "Feed");
  Serial.print("Enabled: ");
  Serial.println(enabled ? "True" : "False");
  Serial.print("Lock State: ");
  Serial.println(lockState ? "True" : "False");
  Serial.print("Ready to Thread: ");
  Serial.println(readyToThread ? "True" : "False");
  Serial.print("Synced: ");
  Serial.println(synced ? "True" : "False");
  Serial.print("Feed Select: ");
  Serial.println(driveMode == true ? threadPitch[feedSelect]
                                   : feedPitch[feedSelect]);
  Serial.print("Jog Mode: ");
  Serial.println(jogMode ? "True" : "False");
  Serial.print("Jog Unsynced Count: ");
  Serial.println(jogUnsyncedCount);
  Serial.print("Pulse Count: ");
  Serial.println(pulseCount);
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
  display.update(
      driveMode,
      driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
      lockState, enabled);

  printState();
}

void buttonHeldHandle() {
  int currentMicros = micros();

  if (jogLeftHeld && currentMicros - lastPulse > JOG_PULSE_DELAY_US) {
    jogMode = true;
    synced = false;
    pulseCount--;
    jogUnsyncedCount--;
    if (hasPreviouslySynced) {
      pulsesBackToSync++;
    }

  } else if (jogRightHeld && currentMicros - lastPulse > JOG_PULSE_DELAY_US) {
    jogMode = true;
    synced = false;
    pulseCount++;
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

  if (pulseCount != 0 ||
      (jogMode && currentMicros - lastPulse > JOG_PULSE_DELAY_US)) {
    int directionIncrement = pulseCount > 0 ? -1 : 1;

    // state 1, motion enabled
    if (enabled || jogMode) {
      // we have jogged and need to wait for the spindle to move to a point we
      // can restart we assume we only want to do this in a CW direction (CCW
      // todo)
      float ratio =
          (driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect]);
      int mod =
          (int)((ELS_SPINDLE_ENCODER_PPR * ratio) / ELS_LEADSCREW_PITCH_MM);
      // todo unspaghettify this logic
      if (synced) {
        // state 2, motion disabled

        // negates encoder pulses if at sync point
        pulseCount = 0;

        if (driveMode == true) {
          // keep track of pulses to get back in sync with the thread
          pulsesBackToSync += directionIncrement;
          pulsesBackToSync %= mod;
        }

      } else if (enabled && jogUnsyncedCount != 0 &&
                 abs(jogUnsyncedCount) != mod) {
        // state 1, jog mode or motion enabled
        // "consume" the pulse by using up the jogUnsyncedCount
        jogUnsyncedCount += directionIncrement;
        pulseCount += directionIncrement;
        lastPulse = micros();

        if (jogUnsyncedCount == mod) {
          jogUnsyncedCount = 0;
        }
      } else {
        // change direction based on sign of the pulse count
        if (pulseCount > 0) {
          CW;
        } else {
          CCW;
        }

        // "bresenham algorithm", carries
        // remainder of required motor steps to
        // next pulse received from spindle

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

        pulseCount += directionIncrement;

        lastPulse = micros();
        if (!enabled && pulseCount == 0 && jogMode == true) {
          jogMode = false;
        }
        // if we are threading
        if (enabled && driveMode == true && hasPreviouslySynced) {
          pulsesBackToSync += directionIncrement;
        }
        if (enabled && pulsesBackToSync == 0 && hasPreviouslySynced) {
          synced = true;
        }
      }
    }
  }
}

void Achange() {  // validates encoder pulses, adds to pulse variable

  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(pinA));
  bitWrite(newPos, 1,
           digitalReadFast(pinB));  // adds A to B, converts to integer
  pulseCount = EncoderMatrix[(oldPos * 4) + newPos];
}

void Bchange() {  // validates encoder pulses, adds to pulse variable

  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(pinA));
  bitWrite(newPos, 1,
           digitalReadFast(pinB));  // adds A to B, converts to integer
  pulseCount =
      EncoderMatrix[(oldPos * 4) + newPos];  // assigns value from encoder
                                             // matrix to determine validity
                                             // and direction of encoder pulse
}

void rateIncCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // increases feedSelect variable on button press
  Serial.println("Rate Inc Call");
  printState();

  if (driveMode == false ||
      ((micros() - lastPulse) > safetyDelay && lockState == false)) {
    if (event == Button::PRESSED_EVENT) {
      if (feedSelect < 19) {
        feedSelect++;
      }

      else {
        feedSelect = 0;
      }
      display.update(
          driveMode,
          driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
          lockState, enabled);
    }
  }
}

void rateDecCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // decreases feedSelect variable on button press
  Serial.println("Rate Dec Call");
  printState();
  if (driveMode == false ||
      ((micros() - lastPulse) > safetyDelay && lockState == false)) {
    if (event == Button::PRESSED_EVENT) {
      if (feedSelect > 0) {
        feedSelect--;
      }

      else {
        feedSelect = 19;
      }

      display.update(
          driveMode,
          driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
          lockState, enabled);
    }
  }
}

void halfNutCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // sets readyToThread to true on button hold
  Serial.println("Half Nut Call");
  printState();
  if (event == Button::PRESSED_EVENT && driveMode == true) {
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
      float ratio =
          (driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect]);
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

    display.update(
        driveMode,
        driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
        lockState, enabled);
  }
}

void lockCall(Button::CALLBACK_EVENT event,
              uint8_t) {  // toggles lockState variable on button press
  Serial.println("Lock Call");
  printState();
  if (event == Button::PRESSED_EVENT) {
    lockState = !lockState;

    display.update(
        driveMode,
        driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
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
    if (driveMode == false) {
      driveMode = true;
    }

    else {
      driveMode = false;
    }

    display.update(
        driveMode,
        driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
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
