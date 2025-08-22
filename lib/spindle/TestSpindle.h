#ifdef PIO_UNIT_TESTING
#include <axis.h>

#pragma once

// For testing, we define Spindle as TestSpindle implementation  
class Spindle : public RotationalAxis {
 private:
  // the unconsumed position is the position that has been read from the encoder
  // but hasn't been used to update the current position of any driven axes
  int m_unconsumedPosition;

 public:
  Spindle();
  Spindle(int pinA, int pinB);

  void update();
  void setCurrentPosition(int position);
  void incrementCurrentPosition(int amount);
  /**
   * This will return the unconsumed position and reset it to 0
   * used for updating the expected position of any driven axes
   */
  int consumePosition();
  float getEstimatedVelocityInRPM();
  float getEstimatedVelocityInPPS();
};
#endif