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
  Leadscrew leadscrew(&spindle, &leadscrewIOMock);

  // log the current settings in config.h
  printf("LEADSCREW_INITIAL_PULSE_DELAY_US: %f\n",
         LEADSCREW_INITIAL_PULSE_DELAY_US);
  printf("LEADSCREW_PULSE_DELAY_STEP_US: %f\n", LEADSCREW_PULSE_DELAY_STEP_US);

  // test data
  // define the time and the expected position of the leadscrew

  globalState->setMotionMode(GlobalMotionMode::ENABLED);
  spindle.setCurrentPosition(100);

  printf("Step1 timing: %d\n",
         roundUp(LEADSCREW_INITIAL_PULSE_DELAY_US + 10, 10));
  printf("Step2 timing: %d\n", roundUp(LEADSCREW_INITIAL_PULSE_DELAY_US * 2 -
                                           LEADSCREW_PULSE_DELAY_STEP_US + 10,
                                       10));

  // todo more accurate predictions of when we should step
  vector<position> expectedStepPositions = {
      {0, 0},  // initial position
      {roundUp((int)(LEADSCREW_INITIAL_PULSE_DELAY_US + 10), 10), 1},
      {roundUp((int)(LEADSCREW_INITIAL_PULSE_DELAY_US * 2 -
                     LEADSCREW_PULSE_DELAY_STEP_US + 20),
               10),
       2}};

  // find the max time in the expectedStepPositions
  unsigned long maxTime = 0;
  for (auto& step : expectedStepPositions) {
    if (step.micros > maxTime) {
      maxTime = step.micros;
    }
  }
  maxTime += 1000;  // add a little extra time to make sure the test runs long
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