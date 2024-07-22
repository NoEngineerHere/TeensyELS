#include "../axis.h"

class Spindle : public Axis, public MobileAxis {
 private:
  // this is volatile since it is updated by an interrupt
  volatile int m_currentPosition;

 public:
  int getCurrentPosition();
  void resetCurrentPosition();
  void setCurrentPosition(int position);
  void incrementCurrentPosition(int amount);
};