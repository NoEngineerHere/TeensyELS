
#ifndef PIO_UNIT_TESTING
#define PIO_UNIT_TESTING  // for intellisense to pick up the MicrosSingleton etc
#endif 

#include "arduino_test_mock.h"

// Define singleton instances
MicrosSingleton* MicrosSingleton::instance = nullptr;
MillisSingleton* MillisSingleton::instance = nullptr;

#include <gmock/gmock.h>
#include <globalstate.h>
#include "TestSpindle.h"
#include <leadscrew.h>
#include "mocks/leadscrewio_mock.h"

#if defined(ARDUINO)
#include <Arduino.h>

void setup() {
  // should be the same value as for the `test_speed` option in "platformio.ini"
  // default value is test_speed=115200
  Serial.begin(115200);

  ::testing::InitGoogleTest();
  // if you plan to use GMock, replace the line above with
  // ::testing::InitGoogleMock();
}

void loop() {
  // Run tests
  if (RUN_ALL_TESTS())
    ;

  // sleep for 1 sec
  delay(1000);
}

#else
int main(int argc, char **argv) {
  //::testing::InitGoogleTest(&argc, argv);
  // if you plan to use GMock, replace the line above with
  // ::testing::InitGoogleMock(&argc, argv);

  //if (RUN_ALL_TESTS())
    //;

  // Always return zero-code and allow PlatformIO to parse results

  MicrosSingleton& micros = MicrosSingleton::getInstance();
  MillisSingleton& millis = MillisSingleton::getInstance();

  LeadscrewIOMock leadscrewIOMock;
  Spindle spindle;
  Leadscrew leadscrew(&spindle, &leadscrewIOMock, 100.0,
    LEADSCREW_INITIAL_PULSE_DELAY_US, ELS_LEADSCREW_STEPPER_PPR* ELS_GEARBOX_RATIO,
    ELS_LEADSCREW_PITCH_MM, ELS_SPINDLE_ENCODER_PPR);

  leadscrew.setStopPosition(LeadscrewStopPosition::LEFT, 0);
  leadscrew.setStopPosition(LeadscrewStopPosition::RIGHT, 10000); 
  leadscrew.setCurrentPosition(10000);
  leadscrew.setTargetPitchMM(0.25);
  GlobalState::getInstance()->setMotionMode(GlobalMotionMode::MM_ENABLED); 
  GlobalState::getInstance()->setThreadSyncState(GlobalThreadSyncState::SYNC); 


  micros.setMicros(0);
  millis.setMillis(0);

  for(int i = 0; i < 10000; i++){
    micros.incrementMicros(100);
    millis.incrementMillis(1);
    leadscrew.update();
  }


  DEBUG_F("Hello\n");

  return 0;
}
#endif