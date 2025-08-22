#ifndef PIO_UNIT_TESTING
#define PIO_UNIT_TESTING
#endif

#include "arduino_test_mock.h"
#include <config.h>
#include <globalstate.h>
#include "TestSpindle.h"
#include <leadscrew.h>
#include <gmock/gmock.h>
#include "mocks/leadscrewio_mock.h"

TEST(LeadscrewTest, Construction) {
  LeadscrewIOMock ioMock;
  Spindle spindle;
  
  Leadscrew leadscrew(&spindle, &ioMock, 100.0, 1000.0, 200, 2.0, 1000);
  
  EXPECT_EQ(leadscrew.getCurrentPosition(), 0);
  EXPECT_EQ(leadscrew.getPositionError(), 0);
}

TEST(LeadscrewTest, StopPositionManagement) {
  LeadscrewIOMock ioMock;
  Spindle spindle;
  Leadscrew leadscrew(&spindle, &ioMock, 100.0, 1000.0, 200, 2.0, 1000);
  
  // Initially stop positions should be unset
  EXPECT_EQ(leadscrew.getStopPositionState(LeadscrewStopPosition::LEFT), 
           LeadscrewStopState::UNSET);
  EXPECT_EQ(leadscrew.getStopPositionState(LeadscrewStopPosition::RIGHT), 
           LeadscrewStopState::UNSET);
  
  // Set left stop position
  leadscrew.setStopPosition(LeadscrewStopPosition::LEFT, 100);
  EXPECT_EQ(leadscrew.getStopPositionState(LeadscrewStopPosition::LEFT), 
           LeadscrewStopState::SET);
  EXPECT_EQ(leadscrew.getStopPosition(LeadscrewStopPosition::LEFT), 100);
  
  // Set right stop position
  leadscrew.setStopPosition(LeadscrewStopPosition::RIGHT, 500);
  EXPECT_EQ(leadscrew.getStopPositionState(LeadscrewStopPosition::RIGHT), 
           LeadscrewStopState::SET);
  EXPECT_EQ(leadscrew.getStopPosition(LeadscrewStopPosition::RIGHT), 500);
  
  // Unset stop positions
  leadscrew.unsetStopPosition(LeadscrewStopPosition::LEFT);
  EXPECT_EQ(leadscrew.getStopPositionState(LeadscrewStopPosition::LEFT), 
           LeadscrewStopState::UNSET);
  
  leadscrew.unsetStopPosition(LeadscrewStopPosition::RIGHT);
  EXPECT_EQ(leadscrew.getStopPositionState(LeadscrewStopPosition::RIGHT), 
           LeadscrewStopState::UNSET);
}

TEST(LeadscrewTest, PitchConfiguration) {
  LeadscrewIOMock ioMock;
  Spindle spindle;
  Leadscrew leadscrew(&spindle, &ioMock, 100.0, 1000.0, 200, 2.0, 1000);
  
  // Test setting different pitch values
  leadscrew.setTargetPitchMM(1.0);
  leadscrew.setTargetPitchMM(1.5);
  leadscrew.setTargetPitchMM(0.5);
  
  // Should not crash or have invalid state
  EXPECT_EQ(leadscrew.getCurrentPosition(), 0);
}

TEST(LeadscrewTest, PositionSetting) {
  LeadscrewIOMock ioMock;
  Spindle spindle;
  Leadscrew leadscrew(&spindle, &ioMock, 100.0, 1000.0, 200, 2.0, 1000);
  
  leadscrew.setCurrentPosition(150);
  EXPECT_EQ(leadscrew.getCurrentPosition(), 150);
  
  leadscrew.setCurrentPosition(-75);
  EXPECT_EQ(leadscrew.getCurrentPosition(), -75);
}

TEST(LeadscrewTest, MotionWithSpindleSync) {
  MicrosSingleton& micros = MicrosSingleton::getInstance();
  GlobalState* globalState = GlobalState::getInstance();
  LeadscrewIOMock ioMock;
  Spindle spindle;
  
  Leadscrew leadscrew(&spindle, &ioMock, 0, 1000, 200, 2.0, 1000);
  
  // Set up for motion
  globalState->setMotionMode(GlobalMotionMode::MM_ENABLED);
  leadscrew.setTargetPitchMM(1.0);
  
  micros.setMicros(0);
  
  // Simulate spindle movement
  spindle.setCurrentPosition(100);
  
  // Run leadscrew updates
  for (int i = 0; i < 10; i++) {
    micros.incrementMicros(LEADSCREW_TIMER_US);
    leadscrew.update();
  }
  
  // Should have moved from initial position
  // Exact position depends on timing and configuration
  EXPECT_NE(leadscrew.getPositionError(), 0);
}

TEST(LeadscrewTest, DisabledMotionMode) {
  MicrosSingleton& micros = MicrosSingleton::getInstance();
  GlobalState* globalState = GlobalState::getInstance();
  LeadscrewIOMock ioMock;
  Spindle spindle;
  
  Leadscrew leadscrew(&spindle, &ioMock, 0, 1000, 200, 2.0, 1000);
  
  // Disable motion
  globalState->setMotionMode(GlobalMotionMode::MM_DISABLED);
  
  micros.setMicros(0);
  spindle.setCurrentPosition(100);
  
  int initialPosition = leadscrew.getCurrentPosition();
  
  // Run updates - should not move
  for (int i = 0; i < 10; i++) {
    micros.incrementMicros(LEADSCREW_TIMER_US);
    leadscrew.update();
  }
  
  EXPECT_EQ(leadscrew.getCurrentPosition(), initialPosition);
}

TEST(LeadscrewTest, DirectionTracking) {
  MicrosSingleton& micros = MicrosSingleton::getInstance();
  GlobalState* globalState = GlobalState::getInstance();
  LeadscrewIOMock ioMock;
  Spindle spindle;
  
  Leadscrew leadscrew(&spindle, &ioMock, 0, 1000, 200, 2.0, 1000);
  
  globalState->setMotionMode(GlobalMotionMode::MM_ENABLED);
  leadscrew.setTargetPitchMM(1.0);
  
  // Initially should be unknown
  EXPECT_EQ(leadscrew.getCurrentDirection(), LeadscrewDirection::UNKNOWN);
  
  micros.setMicros(0);
  
  // Move spindle in positive direction
  for (int pos = 0; pos < 50; pos++) {
    spindle.setCurrentPosition(pos);
    micros.incrementMicros(LEADSCREW_TIMER_US);
    leadscrew.update();
  }
  
  // Should eventually establish a direction
  LeadscrewDirection dir = leadscrew.getCurrentDirection();
  EXPECT_TRUE(dir == LeadscrewDirection::LEFT || dir == LeadscrewDirection::RIGHT);
}