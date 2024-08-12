// this is a copy/paste of the elapsedMillis.h file from the Teensy library
// with some minor modifications to make it mockable for timing

#pragma once

#ifndef PIO_UNIT_TESTING
#include <elapsedMillis.h>
#else

// remove the millis and micros functions from the global namespace so we can
// use our versions
#undef millis
#undef micros

class MillisSingleton {
 private:
  unsigned long m_millis;

 public:
  MillisSingleton() : m_millis(0) {}
  unsigned long millis() { return m_millis; }
  void incrementMillis() { m_millis++; }
  void incrementMillis(unsigned long millis) { m_millis += millis; }
  void setMillis(unsigned long millis) { m_millis = millis; }

  static MillisSingleton &getInstance() {
    static MillisSingleton instance;
    return instance;
  }
};

class MicrosSingleton {
 private:
  unsigned long m_micros;

 public:
  MicrosSingleton() : m_micros(0) {}
  unsigned long micros() { return m_micros; }
  void incrementMicros() { m_micros++; }
  void incrementMicros(unsigned long micros) { m_micros += micros; }
  void setMicros(unsigned long micros) { m_micros = micros; }

  static MicrosSingleton &getInstance() {
    static MicrosSingleton instance;
    return instance;
  }
};

// have to inline these as a hack to get the tests to compile to compile
inline unsigned long millis() {
  return MillisSingleton::getInstance().millis();
}
inline unsigned long micros() {
  return MicrosSingleton::getInstance().micros();
}

class elapsedMillis {
 private:
  unsigned long ms;

 public:
  elapsedMillis(void) { ms = millis(); }
  elapsedMillis(unsigned long val) { ms = millis() - val; }
  elapsedMillis(const elapsedMillis &orig) { ms = orig.ms; }
  operator unsigned long() const { return millis() - ms; }
  elapsedMillis &operator=(const elapsedMillis &rhs) {
    ms = rhs.ms;
    return *this;
  }
  elapsedMillis &operator=(unsigned long val) {
    ms = millis() - val;
    return *this;
  }
  elapsedMillis &operator-=(unsigned long val) {
    ms += val;
    return *this;
  }
  elapsedMillis &operator+=(unsigned long val) {
    ms -= val;
    return *this;
  }
  elapsedMillis operator-(int val) const {
    elapsedMillis r(*this);
    r.ms += val;
    return r;
  }
  elapsedMillis operator-(unsigned int val) const {
    elapsedMillis r(*this);
    r.ms += val;
    return r;
  }
  elapsedMillis operator-(long val) const {
    elapsedMillis r(*this);
    r.ms += val;
    return r;
  }
  elapsedMillis operator-(unsigned long val) const {
    elapsedMillis r(*this);
    r.ms += val;
    return r;
  }
  elapsedMillis operator+(int val) const {
    elapsedMillis r(*this);
    r.ms -= val;
    return r;
  }
  elapsedMillis operator+(unsigned int val) const {
    elapsedMillis r(*this);
    r.ms -= val;
    return r;
  }
  elapsedMillis operator+(long val) const {
    elapsedMillis r(*this);
    r.ms -= val;
    return r;
  }
  elapsedMillis operator+(unsigned long val) const {
    elapsedMillis r(*this);
    r.ms -= val;
    return r;
  }
};

// elapsedMicros acts as an integer which autoamtically increments 1 million
// times per second.  Useful for creating delays, timeouts, or measuing how long
// an operation takes.  You can create as many elapsedMicros variables as
// needed. All of them are independent.  Any may be written, modified or read at
// any time.
class elapsedMicros {
 private:
  unsigned long us;

 public:
  elapsedMicros(void) { us = micros(); }
  elapsedMicros(unsigned long val) { us = micros() - val; }
  elapsedMicros(const elapsedMicros &orig) { us = orig.us; }
  operator unsigned long() const { return micros() - us; }
  elapsedMicros &operator=(const elapsedMicros &rhs) {
    us = rhs.us;
    return *this;
  }
  elapsedMicros &operator=(unsigned long val) {
    us = micros() - val;
    return *this;
  }
  elapsedMicros &operator-=(unsigned long val) {
    us += val;
    return *this;
  }
  elapsedMicros &operator+=(unsigned long val) {
    us -= val;
    return *this;
  }
  elapsedMicros operator-(int val) const {
    elapsedMicros r(*this);
    r.us += val;
    return r;
  }
  elapsedMicros operator-(unsigned int val) const {
    elapsedMicros r(*this);
    r.us += val;
    return r;
  }
  elapsedMicros operator-(long val) const {
    elapsedMicros r(*this);
    r.us += val;
    return r;
  }
  elapsedMicros operator-(unsigned long val) const {
    elapsedMicros r(*this);
    r.us += val;
    return r;
  }
  elapsedMicros operator+(int val) const {
    elapsedMicros r(*this);
    r.us -= val;
    return r;
  }
  elapsedMicros operator+(unsigned int val) const {
    elapsedMicros r(*this);
    r.us -= val;
    return r;
  }
  elapsedMicros operator+(long val) const {
    elapsedMicros r(*this);
    r.us -= val;
    return r;
  }
  elapsedMicros operator+(unsigned long val) const {
    elapsedMicros r(*this);
    r.us -= val;
    return r;
  }
};

#endif