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

const int safetyDelay = 100;

int accumulator;
int numerator;
int denominator;

volatile int pulseCount;
volatile int pulseID;
volatile int positionCount;
int spindleAngle;
int spindleRotations;
int leadscrewAngle;
int leadscrewAngleCumulative;
long long lastPulse;

bool jogMode = true;
volatile bool driveMode =
    true;  // select threading mode (true) or feeding mode (false)
volatile bool enabled = false;
volatile bool lockState = true;
volatile bool readyToThread = false;
volatile bool synced = false;
int feedSelect = 19;
int jogRate;
int jogStepTime = 10;

// UI Values
const char gearLetter[3] = {65, 66, 67};
const float threadPitch[20] = {0.35, 0.40, 0.45, 0.50, 0.60, 0.70, 0.80,
                               1.00, 1.25, 1.50, 1.75, 2.00, 2.50, 3.00,
                               3.50, 4.00, 4.50, 5.00, 5.50, 6.00};
const float feedPitch[20] = {0.05, 0.08, 0.10, 0.12, 0.15, 0.18, 0.20,
                             0.23, 0.25, 0.28, 0.30, 0.35, 0.40, 0.45,
                             0.50, 0.55, 0.60, 0.65, 0.70, 0.75};

// Ratio Values

const int numeratorTable[20] = {
    7, 8, 9, 2,  12, 14, 16, 4, 1,  6,
    7, 8, 2, 12, 14, 16, 18, 4, 22, 24};  // metric threading ratio tables

const int denominatorTable[20] = {25, 25, 25, 5, 25, 25, 25, 5, 1, 5,
                                  5,  5,  1,  5, 5,  5,  5,  1, 5, 5};

const int numeratorTable2[20] = {
    1, 3, 1, 3, 3, 7,  1, 9,  1, 11,
    3, 7, 1, 9, 1, 11, 3, 13, 7, 3};  // feed ratio tables

const int denominatorTable2[20] = {80, 160, 40, 100, 80, 160, 20, 160, 16, 160,
                                   40, 80,  10, 80,  8,  80,  20, 80,  40, 16};

void Achange();
void Bchange();
void modeHandle();

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

  numerator = numeratorTable[feedSelect];
  denominator = denominatorTable[feedSelect];

  display.init();
        display.update(
          driveMode,
          driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
          lockState, enabled);

}

void loop() {
  keyPad.handle();
  modeHandle();

  uint32_t currentMicros = micros();

  if (pulseCount != 0 || jogMode && currentMicros - lastPulse > 500) {
    int directionIncrement = pulseCount > 0 ? -1 : 1;

    // state 1, motion enabled
    if (enabled || jogMode) {
      accumulator =
          numerator +
          accumulator;  // "bresenham algorithm", carries remainder of required
                        // motor steps to next pulse received from spindle

      // change direction based on sign of the pulse count
      if (pulseCount > 0) {
        CW;
      } else {
        CCW;
      }

      while (accumulator >=
             denominator) {  // sends required motor steps to motor

        accumulator = accumulator - denominator;

        // todo try and leverage hardware PWM timer to send set amount of pulses
        digitalWriteFast(stp, HIGH);
        delayMicroseconds(2);
        digitalWriteFast(stp, LOW);
        delayMicroseconds(2);
      }

      pulseCount += directionIncrement;
      lastPulse = micros();
      if(pulseCount == 0 && jogMode == true) {
        jogMode = false;
      }

      if (leadscrewAngleCumulative > 0) {  // checks leadscrew position against
                                           // "sync" point in one direction

        leadscrewAngle += directionIncrement;
        leadscrewAngleCumulative += directionIncrement;

        if (leadscrewAngle == -1) {
          leadscrewAngle = 1999;
        } else if (leadscrewAngle == 1999) {
          leadscrewAngle = 0;
        }

        if (leadscrewAngleCumulative ==
            0) {  // disables motor when leadscrew reaches "sync" point

          enabled = false;
        }
      }
    } else if (synced) {  // state 2, motion disabled

      if (driveMode == false) {
        // negates encoder pulses if disabled while in feed mode
        pulseCount += directionIncrement;
      }

      else {
        // converts encoder pulses to stored spindle angle if disabled
        // while in thread mode
        spindleAngle -= directionIncrement;
        if (spindleAngle >= 2000) {
          spindleAngle = 0;
        } else if (spindleAngle <= -1) {
          spindleAngle = 1999;
        }

        pulseCount += directionIncrement;
      }

      if (((spindleAngle * 10) * numeratorTable[feedSelect]) ==
              ((leadscrewAngle * 10) * denominatorTable[feedSelect]) &&
          readyToThread == true) {
        // compares leadscrew angle to spindle angle
        // using ratio - if matching, and user has
        // pressed "nut", state 1 is restored

        enabled = true;
        readyToThread = false;
      }
    }
  }
}

void modeHandle() {  // sets pulse/motor steps ratio based on driveMode (true ==
                     // thread mode, false == feed mode)

  if (driveMode == false) {
    numerator = numeratorTable2[feedSelect];
    denominator = denominatorTable2[feedSelect];
  }

  else {
    numerator = numeratorTable[feedSelect];
    denominator = denominatorTable[feedSelect];
  }
}

void Achange() {  // validates encoder pulses, adds to pulse variable

  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(pinA));
  bitWrite(newPos, 1,
           digitalReadFast(pinB));  // adds A to B, converts to integer
  pulseID = EncoderMatrix[(oldPos * 4) + newPos];

  pulseCount += pulseID;
}

void Bchange() {  // validates encoder pulses, adds to pulse variable

  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(pinA));
  bitWrite(newPos, 1,
           digitalReadFast(pinB));  // adds A to B, converts to integer
  pulseID =
      EncoderMatrix[(oldPos * 4) +
                    newPos];  // assigns value from encoder matrix to determine
                              // validity and direction of encoder pulse

  pulseCount += pulseID;
}

void rateIncCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // increases feedSelect variable on button press

  if (driveMode == false ||
      ((millis() - lastPulse) > safetyDelay && lockState == false)) {
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

  if (driveMode == false ||
      ((millis() - lastPulse) > safetyDelay && lockState == false)) {
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

  if (event == Button::HELD_EVENT && driveMode == true) {
    readyToThread = true;
  }
}

void enaCall(Button::CALLBACK_EVENT event,
             uint8_t) {  // toggles enabled variable on button press

  if (event == Button::PRESSED_EVENT) {
    if (enabled == true) {
      enabled = false;
    }

    else {
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

  if (event == Button::HELD_EVENT) {
    if (synced == false) {
      lockState = true;
      leadscrewAngle = 0;
      leadscrewAngleCumulative = 0;
      spindleAngle = 0;
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

  if (event == Button::PRESSED_EVENT && (millis() - lastPulse) > safetyDelay &&
      lockState == false) {
    if (driveMode == false) {
      driveMode = true;
    }

    else {
      driveMode = false;
    }

    modeHandle();
    display.update(
        driveMode,
        driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
        lockState, enabled);
  }
}

void jogLeftCall(Button::CALLBACK_EVENT event,
                 uint8_t) {  // jogs left on button hold (one day)

                   // when in thread mode, a single press should jog one "thread" in the
  // specified direction
  if (event == Button::PRESSED_EVENT) {
    if (driveMode == true && enabled == false && jogMode == false) {
      jogMode = true;
      long unsigned int fullThreadRotation = 2000 * numerator / denominator;
      pulseCount -= fullThreadRotation;
    }
  }

  if (event == Button::HELD_EVENT && enabled == false) {
    // todo have a better system for handling jogging - move but keep track for
    // re-sync on spindle startup
    CW;

    while (digitalReadFast(24) == HIGH) {
      if ((millis() - lastPulse) > jogStepTime) {
        lastPulse = millis();
        digitalWriteFast(stp, HIGH);
        delayMicroseconds(2);
        digitalWriteFast(stp, LOW);
        delayMicroseconds(2);

        leadscrewAngle++;
        leadscrewAngleCumulative++;

        if (leadscrewAngle >= 2000) {
          leadscrewAngle = 0;
        }
      }
    }
  }
}

void jogRightCall(Button::CALLBACK_EVENT event,
                  uint8_t) {  /// jogs right on button hold (one day)
  if (event == Button::PRESSED_EVENT) {
    if (driveMode == true && enabled == false && jogMode == false) {
      jogMode = true;
      long unsigned int fullThreadRotation = 2000 * numerator / denominator;
      pulseCount += fullThreadRotation;
    }
  }

  if (event == Button::HELD_EVENT && enabled == false) {
    // todo have a better system for handling jogging - move but keep track for
    // re-sync on spindle startup
    CCW;

    while (digitalReadFast(25) == HIGH) {
      if ((millis() - lastPulse) > jogStepTime) {
        lastPulse = millis();
        digitalWriteFast(stp, HIGH);
        delayMicroseconds(2);
        digitalWriteFast(stp, LOW);
        delayMicroseconds(2);

        leadscrewAngle--;
        leadscrewAngleCumulative--;

        if (leadscrewAngle <= -1) {
          leadscrewAngle = 1999;
        }
      }
    }
  }
}
