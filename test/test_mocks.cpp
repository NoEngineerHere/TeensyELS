#ifndef PIO_UNIT_TESTING
#define PIO_UNIT_TESTING
#endif

#include "arduino_test_mock.h"
#include <gmock/gmock.h>
#include "mocks/leadscrewio_mock.h"
#include "mocks/axis_mock.h"

TEST(MocksTest, LeadscrewIOMockBasicFunctionality) {
  LeadscrewIOMock ioMock;
  
  // Test step pin operations
  ioMock.writeStepPin(HIGH);
  EXPECT_EQ(ioMock.readStepPin(), HIGH);
  
  ioMock.writeStepPin(LOW);
  EXPECT_EQ(ioMock.readStepPin(), LOW);
  
  // Test dir pin operations
  ioMock.writeDirPin(HIGH);
  EXPECT_EQ(ioMock.readDirPin(), HIGH);
  
  ioMock.writeDirPin(LOW);
  EXPECT_EQ(ioMock.readDirPin(), LOW);
}

TEST(MocksTest, LeadscrewIOMockStateIndependence) {
  LeadscrewIOMock ioMock;
  
  // Step and dir pins should be independent
  ioMock.writeStepPin(HIGH);
  ioMock.writeDirPin(LOW);
  
  EXPECT_EQ(ioMock.readStepPin(), HIGH);
  EXPECT_EQ(ioMock.readDirPin(), LOW);
  
  ioMock.writeStepPin(LOW);
  EXPECT_EQ(ioMock.readStepPin(), LOW);
  EXPECT_EQ(ioMock.readDirPin(), LOW);  // Should not change
}

TEST(MocksTest, AxisMockExpectations) {
  AxisMock axisMock;
  
  // Set up expectations
  EXPECT_CALL(axisMock, getCurrentPosition())
    .WillOnce(::testing::Return(100))
    .WillOnce(::testing::Return(150));
  
  EXPECT_CALL(axisMock, setCurrentPosition(200))
    .Times(1);
  
  EXPECT_CALL(axisMock, incrementCurrentPosition(50))
    .Times(1);
  
  // Test expectations
  EXPECT_EQ(axisMock.getCurrentPosition(), 100);
  EXPECT_EQ(axisMock.getCurrentPosition(), 150);
  
  axisMock.setCurrentPosition(200);
  axisMock.incrementCurrentPosition(50);
}

TEST(MocksTest, TimeAbstractionSingletons) {
  MicrosSingleton& micros = MicrosSingleton::getInstance();
  MillisSingleton& millis = MillisSingleton::getInstance();
  
  // Test micros singleton
  micros.setMicros(1000);
  EXPECT_EQ(micros.micros(), 1000);
  EXPECT_EQ(::micros(), 1000);  // Global function should match
  
  micros.incrementMicros(500);
  EXPECT_EQ(micros.micros(), 1500);
  EXPECT_EQ(::micros(), 1500);
  
  // Test millis singleton
  millis.setMillis(100);
  EXPECT_EQ(millis.millis(), 100);
  EXPECT_EQ(::millis(), 100);  // Global function should match
  
  millis.incrementMillis(50);
  EXPECT_EQ(millis.millis(), 150);
  EXPECT_EQ(::millis(), 150);
  
  // Test independence
  micros.setMicros(2000);
  EXPECT_EQ(millis.millis(), 150);  // Should not affect millis
}