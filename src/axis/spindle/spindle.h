#include <elapsedMillis.h>

#include "../axis.h"

class Spindle : public RotationalAxis, public MobileAxis {
 private:
  // this is volatile since it is updated by an interrupt
  volatile int m_currentPosition;
  elapsedMicros m_lastPulseMicros;

 public:
  int getCurrentPosition();
  void resetCurrentPosition();
  void setCurrentPosition(int position);
  void incrementCurrentPosition(int amount);
  float getEstimatedVelocityInPulsesPerSecond();
  float getEstimatedVelocityInRPM();
};