#include "spindle.h"
#include <critical_section.h>
#include <cmath>
#include <climits>

void Spindle::latchMotionFault() {
  ElsCriticalSection guard;
  m_motionFault = true;
  m_velocity.valid = false;
}
bool Spindle::hasMotionFault() const {
  ElsCriticalSection guard;
  return m_motionFault;
}
int Spindle::getCurrentPosition() {
  ElsCriticalSection guard;
  return m_currentPosition;
}
void Spindle::setCurrentPosition(int position) {
  ElsCriticalSection guard;
  const int64_t delta = int64_t(position) - m_currentPosition;
  if (delta < INT_MIN || delta > INT_MAX) {
    m_motionFault = true;
    m_velocity.valid = false;
    return;
  }
  incrementUnlocked(static_cast<int>(delta), elsMicros());
}
void Spindle::incrementCurrentPosition(int amount) {
  ElsCriticalSection guard;
  incrementUnlocked(amount, elsMicros());
}
void Spindle::incrementUnlocked(int amount, uint32_t now) {
  const uint32_t age = now - m_sampleTimestamp;
  if (age > 1000000 || m_idle) {
    m_velocity = {now, 0, 0, false};
    m_sampleTimestamp = now;
    m_sampleCounts = 0;
    m_idle = amount == 0;
  }
  if (m_motionFault) return;
  const int64_t pending = int64_t(m_unconsumedPosition) + amount;
  int64_t sample;
  if (pending < INT_MIN || pending > INT_MAX ||
      __builtin_add_overflow(m_sampleCounts, int64_t(amount), &sample)) {
    m_motionFault = true;
    m_velocity.valid = false;
    return;
  }
  m_unconsumedPosition = static_cast<int>(pending);
  const int64_t pos = int64_t(m_currentPosition) + amount;
  m_currentPosition = (pos % ELS_SPINDLE_ENCODER_PPR + ELS_SPINDLE_ENCODER_PPR) % ELS_SPINDLE_ENCODER_PPR;
  m_sampleCounts = sample;
  if (amount != 0) {
    m_lastPulseTimestamp = now;
    if (sample > ELS_SPEED_COUNTS || sample < -ELS_SPEED_COUNTS) {
      const uint32_t elapsed = now - m_sampleTimestamp;
      m_velocity = {now, elapsed, sample, elapsed != 0};
      m_sampleTimestamp = now;
      m_sampleCounts = 0;
    }
  }
}
int Spindle::consumePosition() {
  ElsCriticalSection guard;
  const int position = m_unconsumedPosition;
  m_unconsumedPosition = 0;
  return position;
}
float Spindle::getEstimatedVelocityInPPS() {
  VelocitySnapshot sample;
  uint32_t now;
  {
    ElsCriticalSection guard;
    sample = m_velocity;
    now = elsMicros();
  }
  const uint32_t age = now - sample.timestamp;
  if (!sample.valid || !sample.duration || age > 1000000) return 0;
  return std::abs(double(sample.counts)) * 1000000.0 / sample.duration;
}
float Spindle::getEstimatedVelocityInRPM() {
  return getEstimatedVelocityInPPS() * 60.0 / ELS_SPINDLE_ENCODER_PPR;
}
