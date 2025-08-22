#ifndef PIO_UNIT_TESTING
#define PIO_UNIT_TESTING
#endif

#include "arduino_test_mock.h"
#include <config.h>
#include "TestSpindle.h"
#include <gmock/gmock.h>

TEST(SpindleTest, DefaultConstructor) {
  Spindle spindle;
  EXPECT_EQ(spindle.getCurrentPosition(), 0);
  EXPECT_EQ(spindle.getEstimatedVelocityInRPM(), 0.0f);
  EXPECT_EQ(spindle.getEstimatedVelocityInPPS(), 0.0f);
}

TEST(SpindleTest, ParameterizedConstructor) {
  Spindle spindle(14, 15);  // Pin A, Pin B
  EXPECT_EQ(spindle.getCurrentPosition(), 0);
  EXPECT_EQ(spindle.getEstimatedVelocityInRPM(), 0.0f);
  EXPECT_EQ(spindle.getEstimatedVelocityInPPS(), 0.0f);
}

TEST(SpindleTest, PositionTracking) {
  Spindle spindle;
  
  spindle.setCurrentPosition(100);
  EXPECT_EQ(spindle.getCurrentPosition(), 100 % ELS_SPINDLE_ENCODER_PPR);
  
  spindle.setCurrentPosition(ELS_SPINDLE_ENCODER_PPR + 50);
  EXPECT_EQ(spindle.getCurrentPosition(), 50);
}

TEST(SpindleTest, PositionIncrement) {
  Spindle spindle;
  
  spindle.setCurrentPosition(100);
  int initialPos = spindle.getCurrentPosition();
  
  spindle.incrementCurrentPosition(50);
  EXPECT_EQ(spindle.getCurrentPosition(), (initialPos + 50) % ELS_SPINDLE_ENCODER_PPR);
}

TEST(SpindleTest, ConsumePositionTracking) {
  Spindle spindle;
  
  // Initially no unconsumed position
  EXPECT_EQ(spindle.consumePosition(), 0);
  
  // Set position should create unconsumed position delta
  spindle.setCurrentPosition(100);
  int consumed = spindle.consumePosition();
  EXPECT_EQ(consumed, 100);
  
  // After consuming, should be zero again
  EXPECT_EQ(spindle.consumePosition(), 0);
  
  // Increment should also create unconsumed position
  spindle.incrementCurrentPosition(25);
  consumed = spindle.consumePosition();
  EXPECT_GT(consumed, 0);
}

TEST(SpindleTest, WrappingBehavior) {
  Spindle spindle;
  
  // Test position wrapping at encoder PPR boundary
  spindle.setCurrentPosition(ELS_SPINDLE_ENCODER_PPR - 1);
  spindle.incrementCurrentPosition(2);
  EXPECT_EQ(spindle.getCurrentPosition(), 1);
  
  // Test negative wrapping
  spindle.setCurrentPosition(1);
  spindle.incrementCurrentPosition(-2);
  int expectedPos = (-1 + ELS_SPINDLE_ENCODER_PPR) % ELS_SPINDLE_ENCODER_PPR;
  EXPECT_EQ(spindle.getCurrentPosition(), expectedPos);
}