#ifndef PIO_UNIT_TESTING
#define PIO_UNIT_TESTING
#endif

#include "arduino_test_mock.h"
#include <axis.h>
#include <gmock/gmock.h>

class TestableAxis : public Axis {
public:
  TestableAxis() : Axis() {}
};

TEST(AxisTest, InitialPosition) {
  TestableAxis axis;
  EXPECT_EQ(axis.getCurrentPosition(), 0);
}

TEST(AxisTest, SetCurrentPosition) {
  TestableAxis axis;
  
  axis.setCurrentPosition(100);
  EXPECT_EQ(axis.getCurrentPosition(), 100);
  
  axis.setCurrentPosition(-50);
  EXPECT_EQ(axis.getCurrentPosition(), -50);
}

TEST(AxisTest, IncrementPosition) {
  TestableAxis axis;
  
  axis.incrementCurrentPosition(10);
  EXPECT_EQ(axis.getCurrentPosition(), 10);
  
  axis.incrementCurrentPosition(-5);
  EXPECT_EQ(axis.getCurrentPosition(), 5);
  
  axis.incrementCurrentPosition(0);
  EXPECT_EQ(axis.getCurrentPosition(), 5);
}

TEST(AxisTest, CombinedPositionOperations) {
  TestableAxis axis;
  
  axis.setCurrentPosition(50);
  axis.incrementCurrentPosition(25);
  EXPECT_EQ(axis.getCurrentPosition(), 75);
  
  axis.incrementCurrentPosition(-100);
  EXPECT_EQ(axis.getCurrentPosition(), -25);
  
  axis.setCurrentPosition(0);
  EXPECT_EQ(axis.getCurrentPosition(), 0);
}

TEST(AxisTest, VelocityEstimationWithoutPulses) {
  TestableAxis axis;
  
  // Without any pulse duration set, velocity should be 0
  EXPECT_EQ(axis.getEstimatedVelocityInPulsesPerSecond(), 0);
}

TEST(AxisTest, VelocityEstimationWithOldPulses) {
  TestableAxis axis;
  
  // Simulate a very old pulse (> 1000 microseconds ago)
  // Should return 0 velocity
  MicrosSingleton& micros = MicrosSingleton::getInstance();
  micros.setMicros(2000);
  
  EXPECT_EQ(axis.getEstimatedVelocityInPulsesPerSecond(), 0);
}