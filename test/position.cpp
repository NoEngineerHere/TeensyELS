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
  MicrosSingleton& microsSingleton = MicrosSingleton::getInstance();
  GlobalState* globalState = GlobalState::getInstance();
  LeadscrewIOMock leadscrewIOMock;
  AxisMock spindle;
  Leadscrew leadscrew(&spindle, &leadscrewIOMock);

  microsSingleton.setMicros(0);
  leadscrew.setCurrentPosition(0);
  leadscrew.setRatio(1.0);

  // test that the position tracks the given axis 1:1 following the accel curve
  leadscrew.update();
  EXPECT_EQ(leadscrew.getCurrentPosition(), 0);

  spindle.setCurrentPosition(100);
  leadscrew.update();
  EXPECT_EQ(leadscrew.getCurrentPosition(), 100);
}