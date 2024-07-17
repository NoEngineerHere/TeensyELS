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

 public:
  Leadscrew(Axis* leadAxis);
  int getCurrentPosition();
  void resetCurrentPosition();
  void setRatio(float ratio);
  float getRatio();
  int getExpectedPosition();
  void setCurrentPosition(int position);
  void incrementCurrentPosition(int amount);
};
