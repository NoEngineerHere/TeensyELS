#include <axis.h>
#include <els_elapsedMillis.h>

#pragma once

class Spindle : public RotationalAxis {
 public:
  void incrementCurrentPosition(int amount);
  float getEstimatedVelocityInRPM();
};