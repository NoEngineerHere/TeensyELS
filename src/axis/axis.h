#pragma once

/**
 * A basic axis - should have a position
 * This can either be driven externally or by the application
 */
class Axis {
 public:
  virtual int getCurrentPosition() = 0;
  virtual void resetCurrentPosition() = 0;
  virtual float getEstimatedVelocityInPulsesPerSecond() = 0;
};

/**
 * An axis that can have its position set (homed)
 */
class MobileAxis {
 public:
  virtual void setCurrentPosition(int position) = 0;
  virtual void incrementCurrentPosition(int amount) = 0;
};

/**
 * An axis that is derived from the position of another axis
 * Example: the leadscrew is derived from the position of the spindle
 */
class DerivedAxis {
 public:
  virtual void setRatio(float ratio) = 0;
  virtual float getRatio() = 0;
};

/**
 * An axis that is driven by the application
 * Example: The leadscrew is a driven axis since it has a motor we control
 * attached
 */
class DrivenAxis {
 public:
  virtual int getExpectedPosition() = 0;
  virtual void update() = 0;
  virtual int getPositionError() = 0;
};

class RotationalAxis : public Axis {
 public:
  virtual float getEstimatedVelocityInRPM() = 0;
};

class LinearAxis : public Axis {
 public:
  virtual float getEstimatedVelocityInMillimetersPerSecond() = 0;
};
