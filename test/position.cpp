#ifndef PIO_UNIT_TESTING
#define PIO_UNIT_TESTING  // for intellisense to pick up the MicrosSingleton etc
                          // classes
#endif

#include <els_elapsedMillis.h>
#include <globalstate.h>
#include <gmock/gmock.h>
#include <leadscrew.h>
#include <spindle.h>

#include <cstdint>

#include "mocks/axis_mock.h"
#include "mocks/leadscrewio_mock.h"
#include "mocks/reset_config.h"

TEST(PositionTest, TestPositionUpdateOverTime) {
  MicrosSingleton& micros = MicrosSingleton::getInstance();
  GlobalState* globalState = GlobalState::getInstance();
  LeadscrewIOMock leadscrewIOMock;
  Spindle spindle;
  Leadscrew leadscrew(&spindle, &leadscrewIOMock);

  micros.setMicros(0);
  leadscrew.setCurrentPosition(0);
  leadscrew.setRatio(1.0);
  globalState->setMotionMode(GlobalMotionMode::ENABLED);

  // test that the position tracks the given axis 1:1 following the accel curve
  leadscrew.update();
  EXPECT_EQ(leadscrew.getCurrentPosition(), 0);

  spindle.setCurrentPosition(100);
  micros.setMicros(10);
  leadscrew.update();
  EXPECT_EQ(leadscrew.getCurrentPosition(), 0);
  micros.setMicros(20);
  leadscrew.update();
  EXPECT_EQ(leadscrew.getCurrentPosition(), 1);
}