#include <axis.h>
#include <els_elapsedMillis.h>

#pragma once

enum SpindleLimitState { SET, UNSET };
enum SpindleLimitOption {
  LEFT,
  RIGHT
}

class Spindle : public RotationalAxis {
 private:
  SpindleLimitState m_leftLimitState;
  int m_leftLimitPosition;
  SpindleLimitState m_rightLimitState;
  int m_rightLimitPosition;

 public:
  void incrementCurrentPosition(int amount);
  float getEstimatedVelocityInRPM();

  /**
   * These will limit the position to one rotation within the position set
   * Left will be within position and position- ELS_SPINDLE_ENCODER_PPR
   * Right will be within position and position + ELS_SPINDLE_ENCODER_PPR
   */
  void setPositionLimit(SpindleLimitOption option, int position);
  void unsetPositionLimit(SpindleLimitOption option);
};