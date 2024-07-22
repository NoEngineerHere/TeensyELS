#include "../axis.h"

#pragma once

class Leadscrew : public Axis,
                  public DerivedAxis,
                  public DrivenAxis,
                  public MobileAxis {
 private:
  Axis* m_leadAxis;
  int m_currentPosition;
  float m_ratio;

  int m_lastPulseMicros;
  int m_currentPulseDelay;
  float m_accumulator;

  float getAccumulatorUnit();
  void sendPulse();

 public:
  Leadscrew(Axis* leadAxis);
  int getCurrentPosition();
  void resetCurrentPosition();
  void setRatio(float ratio);
  float getRatio();
  int getExpectedPosition();
  void setCurrentPosition(int position);
  void incrementCurrentPosition(int amount);
  void update();
};
