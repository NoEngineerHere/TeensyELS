#include <Encoder.h>
#include <axis.h>
#include <els_elapsedMillis.h>

#pragma once

class Spindle : public RotationalAxis {
 private:
  // the unconsumed position is the position that has been read from the encoder
  // but hasn't been used to update the current position of any driven axes
  int m_unconsumedPosition;

#ifndef ELS_SPINDLE_DRIVEN
  Encoder m_encoder;
#endif

 public:
#ifndef ELS_SPINDLE_DRIVEN
  Spindle(int pinA, int pinB);
#endif

  void update();
  void setCurrentPosition(int position);
  void incrementCurrentPosition(int amount);
  /**
   * This will return the unconsumed position and reset it to 0
   * used for updating the expected position of any driven axes
   */
  int consumePosition();
  float getEstimatedVelocityInRPM();
};