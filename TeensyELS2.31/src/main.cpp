// Libraries
#include <AbleButtons.h>
#include <SPI.h>
#include <Wire.h>

#include "display.h"

#define CW digitalWriteFast(3, HIGH);  // direction output pin
#define CCW digitalWriteFast(3, LOW);  // direction output pin

using Button = AblePullupCallbackButton;  // AblePullupButton 
                                          // Using basic pull-up resistor circuit.

using ButtonList = AblePullupCallbackButtonList;  // AblePullupButtonList; 
                                                  // Using basic pull-up button list.

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
Button threadSync(7, threadSyncCall); // thread sync (hold) and set/jog to start point (press)
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
const int spindlePulsesPerRev = 2000;
const int leadscrewStepsPerRev = 2000;
double leadscrewPitch = 2.5;
int desiredThreadRevolutions = 1;

const int safetyDelay = 500000;




volatile int pulseCount;
volatile int jogpulseCount;
volatile int jogdirectionIncrement;
volatile int pulseID;
volatile int spindlePulsePosition = 1;
volatile int spindleAngle;
volatile int leadscrewAngle;
volatile int leadscrewPulsePosition;
volatile int leadscrewAbsolutePosition;
volatile int lastDir;
volatile int threadStart;

int feedSelect = 8;
int jogRate;
int jogStepTime = 100;

long long lastPulse;
long long lastjogPulse;

volatile bool jogging = false;
volatile bool jogLeftButtonPressed = false;
volatile bool jogRightButtonPressed = false;
volatile bool driveMode = true;  // select threading mode (true) or feeding mode (false)
volatile bool enabled = false;
volatile bool lockState = false;
volatile bool readyToThread = false;
volatile bool synced = false;

volatile bool threadStartSet = false;




// UI Values
const char gearLetter[3] = {65, 66, 67};

const float threadPitch[20] = {0.35, 0.40, 0.45, 0.50, 0.60, 0.70, 0.80, 1.00, 1.25, 1.50, 1.75, 2.00, 2.50, 3.00, 3.50, 4.00, 4.50, 5.00, 5.50, 6.00};

const float feedPitch[20] = {0.05, 0.08, 0.10, 0.12, 0.15, 0.18, 0.20, 0.23, 0.25, 0.28, 0.30, 0.35, 0.40, 0.45, 0.50, 0.55, 0.60, 0.65, 0.70, 0.75};

// Ratio Values

const int numeratorTable[20] = {7, 8, 9, 2,  12, 14, 16, 4, 1,  6, 7, 8, 2, 12, 14, 16, 18, 4, 22, 24};  // metric threading ratio tables

const int denominatorTable[20] = {25, 25, 25, 5, 25, 25, 25, 5, 1, 5, 5,  5,  1,  5, 5,  5,  5,  1, 5, 5};

const int numeratorTable2[20] = {1, 3, 1, 3, 3, 7,  1, 9,  1, 11, 3, 7, 1, 9, 1, 11, 3, 13, 7, 3};  // feed ratio tables

const int denominatorTable2[20] = {80, 160, 40, 100, 80, 160, 20, 160, 16, 160, 40, 80,  10, 80,  8,  80,  20, 80,  40, 16};

volatile int accumulator;
volatile int numerator = numeratorTable[feedSelect];
volatile int denominator = denominatorTable[feedSelect];

void Achange();
void Bchange();
void bresenham();
void spindleCount();
void threadToShoulder();
void goToThreadStart();
void threadMode();
void feedMode();
void modeHandle();
void jogToThreadStart();

bool checkSynchronization(int desiredSpindleAngle, int currentSpindlePosition, int currentLeadscrewPosition, int feedSelect, const int* numeratorTable, const int* denominatorTable);
int calculateDesiredSpindleAngle(int feedSelect, double leadscrewPitch, int desiredThreadRevolutions);
const int tolerance = 0;

void setup() {

  Serial.begin(115200);
  // Pinmodes

  pinMode(14, INPUT_PULLUP);  // encoder pin 1
  pinMode(15, INPUT_PULLUP);  // encoder pin 2
  pinMode(2, OUTPUT);         // step output pin
  pinMode(3, OUTPUT);         // direction output pin
  pinMode(4, INPUT_PULLUP);   // rate Inc (press)
  pinMode(5, INPUT_PULLUP);   // rate Dec (press)
  pinMode(6, INPUT_PULLUP);   // mode cycle (press)
  pinMode(7, INPUT_PULLUP);   // thread sync (hold) and set/jog to start point (press)
  pinMode(8, INPUT_PULLUP);   // half nut (press?)
  pinMode(9, INPUT_PULLUP);   // enable toggle (press)
  pinMode(10, INPUT_PULLUP);  // lock toggle (hold)
  pinMode(24, INPUT_PULLUP);  // jog left (press)
  pinMode(25, INPUT_PULLUP);  // jog right (press)

  // Interupts

  attachInterrupt(digitalPinToInterrupt(pinA), Achange, CHANGE);  // encoder channel A interrupt
  attachInterrupt(digitalPinToInterrupt(pinB), Bchange, CHANGE);  // encoder channel B interrupt

  // Display Initalisation

  

  display.init();
        
  display.update(
    driveMode,
    driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
    lockState, enabled, jogging, synced, threadStartSet);

}

void loop() {

  keyPad.handle();
  modeHandle();

  
  
  if (!driveMode) // feed mode
  {
    if (enabled && pulseCount != 0) { // state 1, enabled

        bresenham();

    }

    if(!enabled && pulseCount != 0)
    {
      pulseCount --;
    }
  }

  if (driveMode) //thread mode
  {

      
    
    
      if (enabled && pulseCount > 0) //if threadmode, enabled, run as normal
      {
        bresenham();       
      }

      if (!enabled && pulseCount > 0) //if thread mode, disabled, count spindle encoder pulses
      {

        
        spindleCount();

        int currentSpindlePosition = spindlePulsePosition;
        int currentLeadscrewPosition = leadscrewAbsolutePosition;

        int desiredSpindleAngle = calculateDesiredSpindleAngle(feedSelect, leadscrewPitch, desiredThreadRevolutions);

        bool synchronised = checkSynchronization(desiredSpindleAngle, currentSpindlePosition, currentLeadscrewPosition, feedSelect, numeratorTable, denominatorTable);

        Serial.print("spindleAngle: ");
        Serial.print(currentSpindlePosition);
        Serial.print(" ");
        Serial.print("desiredSpindleAngle: ");
        Serial.print(desiredSpindleAngle);
        Serial.print(" ");
        Serial.print("synchronised: ");
        Serial.print(synchronised ? "true" : "false");
        Serial.print(" ");
        Serial.print("leadscrewAbsolutePosition: ");
        Serial.print(leadscrewAbsolutePosition);
        Serial.print("\r\n");

        if (synchronised && readyToThread && synced) 
        {
          enabled = true;
          readyToThread = false;
          display.update(
            driveMode,
            driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
            lockState, enabled, jogging, synced, threadStartSet);
        }

      
      
      }
    



    if (jogging) //jogging active
    {

      

      while (jogpulseCount != 0)
      {
        if (micros() - lastjogPulse > jogStepTime && jogpulseCount > 0)
      {
        CW;
        digitalWriteFast(stp, HIGH);
        delayMicroseconds(2);
        digitalWriteFast(stp, LOW);
        delayMicroseconds(2);
        jogpulseCount --;
        leadscrewPulsePosition ++;
        leadscrewAbsolutePosition ++;
        if (leadscrewPulsePosition == 2000)
        {
          leadscrewPulsePosition = 1;
        }
        lastjogPulse = micros();
      }

      else if (micros() - lastjogPulse > jogStepTime && jogpulseCount < 0)
      {
        CCW;
        digitalWriteFast(stp, HIGH);
        delayMicroseconds(2);
        digitalWriteFast(stp, LOW);
        delayMicroseconds(2);
        jogpulseCount ++;
        leadscrewPulsePosition --;
        leadscrewAbsolutePosition --;
        if (leadscrewPulsePosition == 0)
        {
          leadscrewPulsePosition = 2000;
        }
        lastjogPulse = micros();
        
      } 
      
      

      }

      jogging = false; 
         
        display.update(
          driveMode,
          driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
          lockState, enabled, jogging, synced, threadStartSet);
      
      
      
    }

  } 

} 


int calculateDesiredSpindleAngle(int feedSelect, double leadscrewPitch, int desiredThreadRevolutions) {
    // Calculate selected thread pitch in mm
    double selectedThreadPitch = threadPitch[feedSelect];

    // Calculate ratio from numerator and denominator tables
    double ratio = (double) numeratorTable[feedSelect] / (double) denominatorTable[feedSelect];

    // Calculate lead of the thread
    double lead = selectedThreadPitch * ratio;

    // Calculate desired spindle angle in pulses
    double totalAngleDegrees = (lead / leadscrewPitch) * 360.0 * desiredThreadRevolutions;
    int desiredSpindleAngle = static_cast<int>(totalAngleDegrees / 360.0 * spindlePulsesPerRev);

    return desiredSpindleAngle;
}


bool checkSynchronization(int desiredSpindleAngle, int currentSpindlePosition, int currentLeadscrewPosition, int feedSelect, const int* numeratorTable, const int* denominatorTable) {
    // Calculate spindle angle based on encoder position
    int spindleAngle = (currentSpindlePosition - 1) % spindlePulsesPerRev + 1; // Ensure within 1 to spindlePulsesPerRev range

    // Adjust desired spindle angle within one revolution
    desiredSpindleAngle = desiredSpindleAngle % (spindlePulsesPerRev * denominatorTable[feedSelect] / numeratorTable[feedSelect]);

    // Calculate spindle movement adjusted by the ratio tables
    double ratio = (double) numeratorTable[feedSelect] / (double) denominatorTable[feedSelect];
    int spindleMovement = static_cast<int>((spindleAngle - 1) * ratio) + 1; // Adjust for zero-based indexing

    // Check if spindle movement matches the desired spindle angle within tolerance
    if (abs(spindleMovement - desiredSpindleAngle) <= tolerance) {
        return true; // Synchronized for reengagement
    }

    return false; // Not synchronized
}



bool needToJogToThreadStart() {
    // Determine if leadscrew needs to be jogged to thread start point based on application logic
    if (jogging) {
        return false; // Already jogging, no need to jog again
    }

    // Check if leadscrew position is within the range of 0 to threadStart
    if (leadscrewAbsolutePosition >= 0 && leadscrewAbsolutePosition <= threadStart) {
        return true; // Need to jog towards threadStart
    } else if (leadscrewAbsolutePosition <= 0 && leadscrewAbsolutePosition >= threadStart) {
        return true; // Need to jog towards threadStart
    } else {
        return false; // No need to jog
    }
}



void bresenham(){ //sends pulses to motor based on pitch ratio, carries remainders

        
      
      accumulator = numerator + accumulator;  // "bresenham algorithm", carries remainder of required
                                              // motor steps to next pulse received from spindle

                                              // change direction based on sign of the pulse count
      

      while (accumulator >=
             denominator) {                   // sends required motor steps to motor

        accumulator = accumulator - denominator;

        digitalWriteFast(stp, HIGH);
        delayMicroseconds(2);
        digitalWriteFast(stp, LOW);
        delayMicroseconds(2);
        int leadscrewIncrement = lastDir ? 1 : -1;
        leadscrewAbsolutePosition += leadscrewIncrement;
        if (synced && leadscrewAbsolutePosition == 0) //if thread shoulder has been established with "sync", disable at sync point
        {
          enabled = false;
          display.update(
          driveMode,
          driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
          lockState, enabled, jogging, synced, threadStartSet);
        }       
        
      }

      pulseCount --;
      lastPulse = micros();


}



void jogToThreadStart() {
    // Calculate jog pulse count based on direction (move towards 0 or towards threadStart)
    if (leadscrewAbsolutePosition > 0) {
        jogpulseCount = -leadscrewAbsolutePosition; // Move towards 0 from positive position
    } else if (leadscrewAbsolutePosition < 0) {
        jogpulseCount = -leadscrewAbsolutePosition; // Move towards 0 from negative position
    } else {
        jogpulseCount = threadStart; // Move towards threadStart from 0 position
    }

    // Update jogging state
    jogging = true;
}

void spindleCount(){ //keeps track of spindle angle when disabled in thread mode ("thread dial indicator")

  if (lastDir == true)
  {
    spindlePulsePosition ++;
    pulseCount --;
    //if (spindlePulsePosition > 2000)
    //{
    //  spindlePulsePosition = 1;
    //}
    
  }

  else if (lastDir == false)
  {
    spindlePulsePosition --;
    pulseCount --;

    //if (spindlePulsePosition < 1)
    //{
    //  spindlePulsePosition = 2000;
    //}
  }


  
  
}

void modeHandle(){ //sets pitch ratio based on feedSelect variable

  if (driveMode){

  numerator = numeratorTable[feedSelect];
  denominator = denominatorTable[feedSelect];

  }

  if (!driveMode){

  numerator = numeratorTable2[feedSelect];
  denominator = denominatorTable2[feedSelect];

  }

}

void Achange(){  // validates encoder pulses, adds to pulse variable


  
  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(pinA));
  bitWrite(newPos, 1, digitalReadFast(pinB));  // adds A to B, converts to integer
  pulseID = EncoderMatrix[(oldPos * 4) + newPos];

  if (pulseID == 1)
  {
    CW
    lastDir = true;
    pulseCount ++;
  }

  else if (pulseID == -1)
  {
    CCW
    lastDir = false;
    pulseCount ++;
  }
  

  
  
  

}

void Bchange(){  // validates encoder pulses, adds to pulse variable

  oldPos = newPos;
  bitWrite(newPos, 0, digitalReadFast(pinA));
  bitWrite(newPos, 1, digitalReadFast(pinB));  // adds A to B, converts to integer
  pulseID = EncoderMatrix[(oldPos * 4) + newPos];  // assigns value from encoder matrix to determine validity and direction of encoder pulse
                                                   
  if (pulseID == 1)
  {
    CW
    lastDir = true;
    pulseCount ++;
  }

  else if (pulseID == -1)
  {
    CCW
    lastDir = false;
    pulseCount ++;
  }
  
  
}

void rateIncCall(Button::CALLBACK_EVENT event, uint8_t) {  // increases feedSelect variable on button press

  if (driveMode == false || ((micros() - lastPulse) > safetyDelay && lockState == false)) {
    
    if (event == Button::PRESSED_EVENT) 
    {

      if (feedSelect < 19) 
      {
        feedSelect++;
      }


        display.update(
          driveMode,
          driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
          lockState, enabled, jogging, synced, threadStartSet);

    }
  }
}

void rateDecCall(Button::CALLBACK_EVENT event, uint8_t) {  // decreases feedSelect variable on button press

  if (driveMode == false || ((micros() - lastPulse) > safetyDelay && lockState == false)) {

    if (event == Button::PRESSED_EVENT) 
    {
      if (feedSelect > 0) 
      {
        feedSelect--;
      }



      display.update(
          driveMode,
          driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
          lockState, enabled, jogging, synced, threadStartSet);
    }
  }
}

void halfNutCall(Button::CALLBACK_EVENT event, uint8_t) {  // sets readyToThread to true on button hold, starts threading

  if (event == Button::PRESSED_EVENT && driveMode && synced)   
  {    
    readyToThread = true;   
    Serial.println("Ready To Thread");
  }
}

void enaCall(Button::CALLBACK_EVENT event, uint8_t) {  // toggles enabled variable on button press

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
        lockState, enabled, jogging, synced, threadStartSet);
  }
}

void lockCall(Button::CALLBACK_EVENT event, uint8_t) {  // toggles lockState variable on button press

  if (event == Button::HELD_EVENT) {
    lockState = !lockState;
    synced = false;
    readyToThread = false;
    threadStartSet = false;
    enabled = false;

    display.update(
        driveMode,
        driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
        lockState, enabled, jogging, synced, threadStartSet);
  }
}

void threadSyncCall(Button::CALLBACK_EVENT event, uint8_t) {  // toggles sync status on button hold, when synced, single press first sets start point, when start point is set, single press jogs to start point

  if (event == Button::HELD_EVENT) {
    if (synced == false && driveMode) {
      lockState = true;
      leadscrewPulsePosition = 1;
      leadscrewAbsolutePosition = 0;
      spindlePulsePosition = 1;
      synced = true;
    }

    else {
      synced = false;
      threadStartSet = false;     
    }

   

  }

  if (event == Button::PRESSED_EVENT && synced == true && !jogging) {
    if(!threadStartSet)
    {
      threadStart = leadscrewAbsolutePosition;
      threadStartSet = true;
    }
    else if (!jogging && needToJogToThreadStart())
    {
      jogToThreadStart();
    }
    

  }

   display.update(
        driveMode,
        driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
        lockState, enabled, jogging, synced, threadStartSet);
  
}

void modeCycleCall(Button::CALLBACK_EVENT event, uint8_t) {  // toggles between thread / feed modes on button press

  if (event == Button::PRESSED_EVENT && (micros() - lastPulse) > safetyDelay && lockState == false) {
    if (driveMode == false) {
      driveMode = true;
    }

    else {
      driveMode = false;
      synced = false;
    }

    modeHandle();
    display.update(
        driveMode,
        driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
        lockState, enabled, jogging, synced, threadStartSet);
  }
}

void jogLeftCall(Button::CALLBACK_EVENT event, uint8_t) {  // when disabled in thread mode, a single press jogs one "thread" in the specified direction

  
                   
  if (event == Button::PRESSED_EVENT) {
    if (driveMode == true && enabled == false) {
      jogging = true;
      long unsigned int fullThreadRotation = 2000 * numerator / denominator;
      jogpulseCount -= fullThreadRotation;      
    }

    display.update(
      driveMode,
      driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
      lockState, enabled, jogging, synced, threadStartSet);
  }

  
}

void jogRightCall(Button::CALLBACK_EVENT event, uint8_t) {  // when disabled in thread mode, a single press jogs one "thread" in the specified direction
 
  if (event == Button::PRESSED_EVENT) {
    if (driveMode == true && enabled == false) {
      jogging = true;
      long unsigned int fullThreadRotation = 2000 * numerator / denominator;
      jogpulseCount += fullThreadRotation;
    }

    display.update(
        driveMode,
        driveMode == true ? threadPitch[feedSelect] : feedPitch[feedSelect],
        lockState, enabled, jogging, synced, threadStartSet);
  }
  
}










