#pragma once

/**
 * A basic axis - should have a position
 * This can either be driven externally or by the application
 */
class Axis {
 public:
  virtual int getCurrentPosition();
  virtual void resetCurrentPosition();
};

class MobileAxis {
 public:
  virtual void setCurrentPosition(int position);
  virtual void incrementCurrentPosition(int amount);
};

/**
 * An axis that is derived from the position of another axis
 * Example: the leadscrew is derived from the position of the spindle
 */
class DerivedAxis {
 public:
  virtual void setRatio(float ratio);
  virtual float getRatio();
};

/**
 * An axis that is driven by the application
 * Example: The leadscrew is a driven axis since it has a motor we control
 * attached
 */
class DrivenAxis {
 public:
  virtual int getExpectedPosition();
};