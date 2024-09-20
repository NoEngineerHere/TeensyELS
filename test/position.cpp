#ifndef PIO_UNIT_TESTING
#define PIO_UNIT_TESTING  // for intellisense to pick up the MicrosSingleton etc
                          // classes
#endif

#include <config.h>
#include <els_elapsedMillis.h>
#include <globalstate.h>
#include <gmock/gmock.h>
#include <leadscrew.h>
#include <spindle.h>

#include <cstdint>
#include <vector>

using std::vector;

#include "mocks/axis_mock.h"
#include "mocks/leadscrewio_mock.h"

struct position {
  unsigned long micros;
  int leadscrewPosition;
};

unsigned long roundUp(unsigned long numToRound, unsigned long multiple) {
  assert(multiple);
  return ((numToRound + multiple - 1) / multiple) * multiple;
}

TEST(PositionTest, TestInitialPulseDelay) {
  MicrosSingleton& micros = MicrosSingleton::getInstance();
  GlobalState* globalState = GlobalState::getInstance();

  LeadscrewIOMock leadscrewIOMock;
  Spindle spindle;
  Leadscrew leadscrew(&spindle, &leadscrewIOMock, 100, 0.1, 100, 1, 1);
  // test data
  // define the time and the expected position of the leadscrew

  globalState->setMotionMode(GlobalMotionMode::ENABLED);
  spindle.setCurrentPosition(100);

  vector<position> expectedStepPositions = {
      {0, 0},                  // initial position
      {100 + 20, 1},           // first step after initial delay + step time
      {100 + 20 + 90 + 20, 2}  // second step with accel math
  };

  // find the max time in the expectedStepPositions
  unsigned long maxTime = 0;
  for (auto& step : expectedStepPositions) {
    if (step.micros > maxTime) {
      maxTime = step.micros;
    }
  }
  maxTime += 10;  // add a little extra time to make sure the test runs long
                    // enough

  printf("maxTime: %lu\n", maxTime);

  while (micros.micros() < maxTime) {
    micros.incrementMicros(LEADSCREW_TIMER_US);
    leadscrew.update();

    // assert when the time matches the expected position
    for (auto& step : expectedStepPositions) {
      if (step.micros == micros.micros()) {
        ASSERT_EQ(leadscrew.getCurrentPosition(), step.leadscrewPosition);
        break;
      }
      // if we're not at a set value in the position list, we should be at the
      // one immediately previous defined by the time find the previous position
      // based on the time
      position* previousPosition = &expectedStepPositions[0];
      for (auto& step : expectedStepPositions) {
        if (step.micros > micros.micros()) {
          break;
        }
        previousPosition = &step;
      }
      ASSERT_EQ(leadscrew.getCurrentPosition(),
                previousPosition->leadscrewPosition);
    }
  }
}

TEST(PositionTest, TestAccumulator) {
  MicrosSingleton& micros = MicrosSingleton::getInstance();
  GlobalState* globalState = GlobalState::getInstance();
  LeadscrewIOMock leadscrewIOMock;
  Spindle spindle;
  // no accel - only positioning
  Leadscrew leadscrew(&spindle, &leadscrewIOMock, 0, 0, 100, 1, 1);
  // test data
  // define the time and the expected position of the leadscrew

  globalState->setMotionMode(GlobalMotionMode::ENABLED);
  leadscrew.setRatio(6);

  spindle.setCurrentPosition(1);

  // 6
  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  ASSERT_EQ(leadscrew.getCurrentPosition(), 0);

  // acc: 5
  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  // 4
  ASSERT_EQ(leadscrew.getCurrentPosition(), 0);

  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  // 3
  ASSERT_EQ(leadscrew.getCurrentPosition(), 0);

  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  // 2
  ASSERT_EQ(leadscrew.getCurrentPosition(), 0);

  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  // 1
  ASSERT_EQ(leadscrew.getCurrentPosition(), 0);

  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  micros.incrementMicros(LEADSCREW_TIMER_US);
  leadscrew.update();
  // 0
  // position should be 1
  ASSERT_EQ(leadscrew.getCurrentPosition(), 1);
}