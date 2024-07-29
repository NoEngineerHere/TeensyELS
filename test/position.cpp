#ifndef PIO_UNIT_TESTING
#define PIO_UNIT_TESTING  // for intellisense to pick up the MicrosSingleton etc
                          // classes
#endif

#include <els_elapsedMillis.h>
#include <globalstate.h>
#include <gmock/gmock.h>
#include <spindle.h>

TEST(PositionTest, TestPositionUpdateOverTime) {
  MicrosSingleton& microsSingleton = MicrosSingleton::getInstance();
  GlobalState* globalState = GlobalState::getInstance();

  EXPECT_EQ(micros(), 0);
  microsSingleton.incrementMicros();
  EXPECT_EQ(micros(), 1);
}