//Libraries  
  #include <SPI.h>
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #include <AbleButtons.h>
  
  
//Images 
  #include "lockedSymbol.h"
  #include "unlockedSymbol.h"
  #include "threadSymbol.h"
  #include "feedSymbol.h"
  #include "runSymbol.h"
  #include "pauseSymbol.h"

//OLED
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  #define OLED_RESET     -1 
  #define SCREEN_ADDRESS 0x3C
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define CW digitalWriteFast(3, HIGH); //direction output pin
#define CCW digitalWriteFast(3, LOW); //direction output pin

void modePrint();
void modeHandle();
void enaPrint();
void pulse();
void ratePrint();
void displayUpdate();

using Button = AblePullupCallbackButton; //AblePullupButton; // Using basic pull-up resistor circuit.
using ButtonList = AblePullupCallbackButtonList; //AblePullupButtonList; // Using basic pull-up button list.

void rateIncCall(Button::CALLBACK_EVENT, uint8_t);
void rateDecCall(Button::CALLBACK_EVENT, uint8_t);
void modeCycleCall(Button::CALLBACK_EVENT, uint8_t);
void threadSyncCall(Button::CALLBACK_EVENT, uint8_t);
void halfNutCall(Button::CALLBACK_EVENT, uint8_t);
void enaCall(Button::CALLBACK_EVENT, uint8_t);
void lockCall(Button::CALLBACK_EVENT, uint8_t);
void jogLeftCall(Button::CALLBACK_EVENT, uint8_t);
void jogRightCall(Button::CALLBACK_EVENT, uint8_t);

Button rateInc(4, rateIncCall);
Button rateDec(5, rateDecCall);
Button modeCycle(6, modeCycleCall);
Button threadSync(7, threadSyncCall);
Button halfNut(8,halfNutCall);
Button ena(9, enaCall);
Button lock(10, lockCall);
Button jogLeft(24, jogLeftCall);
Button jogRight(25, jogRightCall);

Button *keys[] = {
&rateInc,
&rateDec,
&modeCycle,
&threadSync,
&halfNut,
&ena,
&lock,
&jogLeft,
&jogRight

};

ButtonList keyPad(keys);
Button setHeldTime(100);

int EncoderMatrix[16] = {0,-1,1,2,1,0,2,-1,-1,2,0,1,2,1,-1,0}; //encoder output matrix, output = X (old) * 4 + Y (new)

volatile int oldPos;
volatile int newPos;

const int pinA = 14; //encoder pin A
const int pinB = 15; //encoder pin B
const int stp = 2; //step output pin
const int dir = 3;//direction output pin

const int safetyDelay = 100;

volatile int accumulator;
volatile int numerator; 
volatile int denominator;

volatile int pulseCount;
volatile int pulseID;
volatile int positionCount;
volatile int spindleAngle;
volatile int spindleRotations;
volatile int leadscrewAngle;
volatile int leadscrewAngleCumulative;
volatile long long lastPulse;

volatile bool driveMode = false; //select threading mode (true) or feeding mode (false)
volatile bool enabled = false;
volatile bool lockState = true;
volatile bool lastDirection; 
volatile bool readyToThread = false;
volatile bool synced = false;
int feedSelect = 19;
int jogRate;
int jogStepTime = 10;

//UI Values
  const char gearLetter[3] = {65, 66, 67};
  const float threadPitch[20] = {0.35,	0.40,	0.45,	0.50,	0.60,	0.70,	0.80,	1.00,	1.25,	1.50,	1.75,	2.00,	2.50,	3.00,	3.50,	4.00,	4.50,	5.00,	5.50,	6.00};
  const float feedPitch[20] = {0.05,0.08,0.10,0.12,0.15,0.18,0.20,0.23,0.25,0.28,0.30,0.35,0.40,0.45,0.50,0.55,0.60,0.65,0.70,0.75};


//Ratio Values

  const int numeratorTable[20] = {7,8,9,2,12,14,16,4,1,6,7,8,2,12,14,16,18,4,22,24}; //metric threading ratio tables

  const int denominatorTable[20] = {25,25,25,5,25,25,25,5,1,5,5,5,1,5,5,5,5,1,5,5};

  const int numeratorTable2[20] = {1,3,1,3,3,7,1,9,1,11,3,7,1,9,1,11,3,13,7,3}; //feed ratio tables

  const int denominatorTable2[20] = {80,160,40,100,80,160,20,160,16,160,40,80,10,80,8,80,20,80,40,16};



void setup() {


  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

//Pinmodes

  pinMode(14, INPUT_PULLUP); //encoder pin 1
  pinMode(15, INPUT_PULLUP); //encoder pin 2
  pinMode(2, OUTPUT); //step output pin
  pinMode(3, OUTPUT); //direction output pin
  pinMode(4, INPUT_PULLUP); //rate Inc 
  pinMode(5, INPUT_PULLUP); //rate Dec
  pinMode(6, INPUT_PULLUP); //mode cycle 
  pinMode(7, INPUT_PULLUP); //thread sync
  pinMode(8, INPUT_PULLUP); //half nut
  pinMode(9, INPUT_PULLUP); //enable toggle
  pinMode(10, INPUT_PULLUP); //lock toggle
  pinMode(24, INPUT_PULLUP); //jog left
  pinMode(25, INPUT_PULLUP); //jog right
 

//Interupts  

  attachInterrupt(digitalPinToInterrupt(pinA), Achange, CHANGE); //encoder channel A interrupt
  attachInterrupt(digitalPinToInterrupt(pinB), Bchange, CHANGE); //encoder channel B interrupt

//Display Initalisation

  numerator = numeratorTable[feedSelect];
  denominator = denominatorTable[feedSelect];
    
  display.setTextColor(SSD1306_WHITE);
  displayUpdate();   
  display.display();

}

void loop() {

  keyPad.handle();
  modeHandle();

  if (pulseCount > 0 && enabled == true){ // state 1, motion enabled
      
          accumulator = numerator + accumulator; // "bresenham algorithm", carries remainder of required motor steps to next pulse received from spindle
          

          while (accumulator >= denominator){ // sends required motor steps to motor

              accumulator = accumulator - denominator;  

              digitalWriteFast(stp, HIGH);              
              delayMicroseconds(2);
              digitalWriteFast(stp, LOW);
              delayMicroseconds(2);

          }

          pulseCount --;
          lastPulse = millis();

          if(lastDirection == true && leadscrewAngleCumulative > 0){ // checks leadscrew position against "sync" point in one direction

            leadscrewAngle --;
            leadscrewAngleCumulative --;          

            if (leadscrewAngle <= -1){

                leadscrewAngle = 1999;

              }

            if (leadscrewAngleCumulative == 0){ // disables motor when leadscrew reaches "sync" point

                enabled = false;
                
            }

            }

          else if(lastDirection == false && leadscrewAngleCumulative < 0){ // checks axis position against "sync" point in other direction

            leadscrewAngle ++;
            leadscrewAngleCumulative ++;          

            if (leadscrewAngle >= 1999){

                leadscrewAngle = 0;

              }

            if (leadscrewAngleCumulative == 0){ // disables motor when leadscrew reaches "sync" point

                enabled = false;
                
            }

            }

          }
  

  

  else if (pulseCount > 0 && enabled == false && synced == true){ // state 2, motion disabled
    
    
        if(driveMode == false){ //negates encoder pulses if disabled while in feed mode

            pulseCount --;

        }

        else { //converts encoder pulses to stored spindle angle if disabled while in thread mode


          if (lastDirection == true){

              spindleAngle ++;

              if(spindleAngle >= 2000){

                  spindleAngle = 0;

                }

              }

          else if (lastDirection == false){

              spindleAngle --;

              if (spindleAngle <= -1){

                spindleAngle = 1999;
              
                }

              }

          pulseCount --;

        }

        if (((spindleAngle * 10) * numeratorTable[feedSelect]) == ((leadscrewAngle * 10) * denominatorTable[feedSelect]) && readyToThread == true){ // compares leadscrew angle to spindle angle using ratio - if matching, and user has pressed "nut", state 1 is restored

          enabled = true;
          readyToThread = false;

        }

       
          

}

}

void modeHandle(){ // sets pulse/motor steps ratio based on driveMode (true == thread mode, false == feed mode)

    if(driveMode == false){

      numerator = numeratorTable2[feedSelect];
      denominator = denominatorTable2[feedSelect];

      }

    else{

      numerator = numeratorTable[feedSelect];
      denominator = denominatorTable[feedSelect];

      }

}

void Achange(){ //validates encoder pulses, adds to pulse variable

    oldPos = newPos;
    bitWrite(newPos, 0, digitalReadFast(pinA));
    bitWrite(newPos, 1, digitalReadFast(pinB));   //adds A to B, converts to integer
    pulseID = EncoderMatrix[(oldPos * 4) + newPos];

    

    if (pulseID == 1){

      CW;   //set DIR pin HIGH
      lastDirection = true;
      pulseCount ++;

    }


    else if (pulseID == -1){

      CCW;   //set DIR pin LOW
      lastDirection = false;
      pulseCount ++;

        }

}

void Bchange(){ //validates encoder pulses, adds to pulse variable

  oldPos = newPos;
    bitWrite(newPos, 0, digitalReadFast(pinA));
    bitWrite(newPos, 1, digitalReadFast(pinB));   //adds A to B, converts to integer
    pulseID = EncoderMatrix[(oldPos * 4) + newPos]; //assigns value from encoder matrix to determine validity and direction of encoder pulse

    if (pulseID == 1){

      CW;   //set DIR pin HIGH
      pulseCount ++;
      
    }

    else if (pulseID == -1){

      CCW;   //set DIR pin LOW
      pulseCount ++;
      
    }


  
}

void rateIncCall(Button::CALLBACK_EVENT event, uint8_t){ //increases feedSelect variable on button press

  if(driveMode == false || ((millis() - lastPulse) > safetyDelay && lockState == false)){

    if(event == Button::PRESSED_EVENT){
    
        if (feedSelect < 19){

          feedSelect ++;

          }

        else {

          feedSelect = 0;

          }


     
        displayUpdate();
    
        }

  }

}

void rateDecCall(Button::CALLBACK_EVENT event, uint8_t){ // decreases feedSelect variable on button press
  
  if(driveMode == false || ((millis() - lastPulse) > safetyDelay && lockState == false)){

    if(event == Button::PRESSED_EVENT){
    
      if (feedSelect > 0){

        feedSelect --;

        }

      else {

        feedSelect = 19;

        }


      displayUpdate();
    
        }

    }

}

void halfNutCall(Button::CALLBACK_EVENT event, uint8_t){ // sets readyToThread to true on button hold

      if(event == Button::HELD_EVENT && driveMode == true){

        readyToThread = true;

        }

}

void enaCall(Button::CALLBACK_EVENT event, uint8_t){ //toggles enabled variable on button press
  
  if(event == Button::PRESSED_EVENT){

    if(enabled == true){

      enabled = false;      

      }

    else{

      enabled = true;

      }

    displayUpdate();

    }

}

void lockCall(Button::CALLBACK_EVENT event, uint8_t){ // toggles lockState variable on button press

  if(event == Button::PRESSED_EVENT){

    if (lockState == true){

      lockState = false;      
      displayUpdate();

      }

    else{

      lockState = true;      
      displayUpdate();

      }

    }

}

void threadSyncCall(Button::CALLBACK_EVENT event, uint8_t){ // toggles sync status on button hold

  if(event == Button::HELD_EVENT){

    if(synced == false){

        lockState = true;
        leadscrewAngle = 0;
        leadscrewAngleCumulative = 0;
        spindleAngle = 0;          
        synced = true;

      }


    else{

        synced = false;

      }  
    }


}

void modeCycleCall(Button::CALLBACK_EVENT event, uint8_t){ // toggles between thread / feed modes on button press

  if(event == Button::PRESSED_EVENT && (millis()-lastPulse) > safetyDelay && lockState == false){

    if (driveMode == false){

        driveMode = true;

      }

    else {
        driveMode = false;
      }

    modeHandle();
    displayUpdate();

    }

}

void jogLeftCall(Button::CALLBACK_EVENT event, uint8_t){ // jogs left on button hold (one day)

  if(event == Button::HELD_EVENT && enabled == false){     
    
    CW;
    
    while (digitalReadFast(24) == HIGH){
      
      if ((millis() - lastPulse) > jogStepTime){

        lastPulse = millis();
        digitalWriteFast(stp, HIGH);
        delayMicroseconds(2);
        digitalWriteFast(stp, LOW);
        delayMicroseconds(2);

        leadscrewAngle ++;
        leadscrewAngleCumulative ++;
        
        if(leadscrewAngle >= 2000){

          leadscrewAngle = 0;

          }
    
        }

      }

    }

}

void jogRightCall(Button::CALLBACK_EVENT event, uint8_t){ /// jogs right on button hold (one day)

  if(event == Button::HELD_EVENT && enabled == false){     
        
    CCW;

    while (digitalReadFast(25) == HIGH){
          
      if ((millis() - lastPulse) > jogStepTime){

        lastPulse = millis();
        digitalWriteFast(stp, HIGH);
        delayMicroseconds(2);
        digitalWriteFast(stp, LOW);
        delayMicroseconds(2);

        leadscrewAngle --;
        leadscrewAngleCumulative --;

        if(leadscrewAngle <= -1){

          leadscrewAngle = 1999;

          }

        }

      }

    }


}

void displayUpdate(){ // updates display elements to reflectt current system state

  display.clearDisplay();
  modePrint();
  ratePrint();
  lockPrint();
  enaPrint();
  display.display();
  
  
}

void modePrint() { // prints mode status to display

  if(driveMode == true){

    display.drawBitmap(57, 32, threadSymbol, 64, 32, WHITE);
  
    }

  else{

    display.drawBitmap(57, 32, feedSymbol, 64, 32, WHITE);
  
    }

}

void ratePrint() { // prints rate status to display

  if(driveMode == true){

    display.setCursor(55, 8);
    display.setTextSize(3);
    display.print(threadPitch[feedSelect]);

    }

  else{

    display.setCursor(55, 8);
    display.setTextSize(3);
    display.print(feedPitch[feedSelect]);

    }

}

void lockPrint(){ //prints lock status to display

  if(lockState == true){

    display.fillRoundRect(2, 40, 20, 20, 2, WHITE);
    display.drawBitmap(4, 42, lockedSymbol, 16, 16, BLACK);

    }

  if(lockState == false){

    display.fillRoundRect(2, 40, 20, 20, 2, WHITE);
    display.drawBitmap(4, 42, unlockedSymbol, 16, 16, BLACK);

    }

}

void enaPrint(){ // prints enable status to display



    if(enabled == true){

    display.fillRoundRect(26, 40, 20, 20, 2, WHITE);
    display.drawBitmap(28, 42, runSymbol, 16, 16, BLACK);

      }

    if (enabled == false){

      display.fillRoundRect(26, 40, 20, 20, 2, WHITE);
      display.drawBitmap(28, 42, pauseSymbol, 16, 16, BLACK);

      }



}
