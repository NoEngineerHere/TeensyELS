#include <axis.h>
#include <els_elapsedMillis.h>

class Spindle : public RotationalAxis {
 private:
  elapsedMicros m_lastPulseMicros;

 public:
  void incrementCurrentPosition(int amount);
  float getEstimatedVelocityInRPM();
};