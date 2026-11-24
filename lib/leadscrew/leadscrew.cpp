#include "leadscrew.h"
#include <globalstate.h>
#include <critical_section.h>
#include <algorithm>
#include <cmath>
#include <climits>

Leadscrew::Leadscrew(Spindle* spindle, LeadscrewIO* io,
    float accel, float delay, int motorPPR, float pitch, int spindlePPR)
    : m_spindle(spindle), m_io(io), motorPulsePerRevolution(motorPPR),
      leadscrewPitch(pitch), encoderPPR(spindlePPR), initialPulseDelay(delay),
      m_currentPulseDelay(delay), m_leadscrewAccel(accel), jogMicros(elsMicros()) {
  setTargetPitchMM(GlobalState::getInstance()->getCurrentFeedPitch());
  if (!std::isfinite(accel) || accel <= 0 || !std::isfinite(delay) || delay <= 0)
    m_numericFault = true;
}

void Leadscrew::setTargetPitchMM(float pitch) {
  const double ratio = double(pitch) * motorPulsePerRevolution /
      (double(leadscrewPitch) * encoderPPR);
  const bool valid = std::isfinite(pitch) && pitch > 0 &&
      std::isfinite(leadscrewPitch) && leadscrewPitch > 0 &&
      motorPulsePerRevolution > 0 && encoderPPR > 0 &&
      std::isfinite(ratio) && ratio > 0 && ratio <= FLT_MAX;
  ElsCriticalSection guard;
  m_ratio = valid ? static_cast<float>(ratio) : 0;
  if (!valid || !std::isfinite(m_ratio) || m_ratio <= 0) m_numericFault = true;
}

// Caller holds the shared guard: the stop coordinate and its phase are one tuple.
bool Leadscrew::phaseAtPosition(int position, int& phase) const {
  int anchor;
  if (m_syncPositionState == LeadscrewSpindleSyncPositionState::LEFT &&
      m_leftStopState == LeadscrewStopState::SET) anchor = m_leftStopPosition;
  else if (m_syncPositionState == LeadscrewSpindleSyncPositionState::RIGHT &&
      m_rightStopState == LeadscrewStopState::SET) anchor = m_rightStopPosition;
  else return false;
  if (encoderPPR <= 0 || !std::isfinite(m_ratio) || m_ratio <= 0) return false;
  const double quotient = double(int64_t(position) - int64_t(anchor)) / double(m_ratio);
  if (!std::isfinite(quotient) || quotient < INT32_MIN || quotient > INT32_MAX) return false;
  const int64_t value = int64_t(static_cast<int32_t>(quotient)) + m_spindleSyncPosition;
  phase = (value % encoderPPR + encoderPPR) % encoderPPR;
  return true;
}

void Leadscrew::unsetStopPosition(LeadscrewStopPosition stop) {
  ElsCriticalSection guard;
  const bool left = stop == LeadscrewStopPosition::LEFT;
  const auto active = left ? LeadscrewSpindleSyncPositionState::LEFT : LeadscrewSpindleSyncPositionState::RIGHT;
  if (m_syncPositionState == active) {
    int phase;
    const bool otherSet = (left ? m_rightStopState : m_leftStopState) == LeadscrewStopState::SET;
    const bool transfer = otherSet && phaseAtPosition(left ? m_rightStopPosition : m_leftStopPosition, phase);
    m_syncPositionState = transfer ? (left ? LeadscrewSpindleSyncPositionState::RIGHT : LeadscrewSpindleSyncPositionState::LEFT)
                                  : LeadscrewSpindleSyncPositionState::UNSET;
    m_spindleSyncPosition = transfer ? phase : 0;
    m_syncInvalid = otherSet && !transfer;
    if (!transfer) GlobalState::getInstance()->setThreadSyncState(SS_UNSYNC);
  }
  if (left) { m_leftStopState = LeadscrewStopState::UNSET; m_leftStopPosition = INT32_MIN; }
  else { m_rightStopState = LeadscrewStopState::UNSET; m_rightStopPosition = INT32_MAX; }
}

void Leadscrew::setStopPosition(LeadscrewStopPosition stop) {
  ElsCriticalSection guard;
  setStopPositionUnlocked(stop, m_currentPosition);
}
void Leadscrew::setStopPosition(LeadscrewStopPosition stop, int position) {
  ElsCriticalSection guard;
  setStopPositionUnlocked(stop, position);
}
void Leadscrew::setStopPositionUnlocked(LeadscrewStopPosition stop, int position) {
  const int spindlePhase = m_spindle->positionUnlocked();
  const bool left = stop == LeadscrewStopPosition::LEFT;
  const auto anchor = left ? LeadscrewSpindleSyncPositionState::LEFT : LeadscrewSpindleSyncPositionState::RIGHT;
  if (m_syncPositionState == anchor) {
    int phase;
    if (phaseAtPosition(position, phase)) m_spindleSyncPosition = phase;
    else {
      m_syncPositionState = LeadscrewSpindleSyncPositionState::UNSET;
      m_spindleSyncPosition = 0;
      m_syncInvalid = true;
      GlobalState::getInstance()->setThreadSyncState(SS_UNSYNC);
    }
  }
  if (left) { m_leftStopPosition = position; m_leftStopState = LeadscrewStopState::SET; }
  else { m_rightStopPosition = position; m_rightStopState = LeadscrewStopState::SET; }
  if (m_syncPositionState == LeadscrewSpindleSyncPositionState::UNSET &&
      position == m_currentPosition && encoderPPR > 0 && m_ratio > 0 && !m_numericFault) {
    m_spindleSyncPosition = (int64_t(spindlePhase) % encoderPPR + encoderPPR) % encoderPPR;
    m_syncPositionState = anchor;
    m_syncInvalid = false;
    GlobalState::getInstance()->setThreadSyncState(SS_SYNC);
  }
}
LeadscrewStopState Leadscrew::getStopPositionState(LeadscrewStopPosition stop) {
  ElsCriticalSection guard;
  return stop == LeadscrewStopPosition::LEFT ? m_leftStopState : m_rightStopState;
}
int Leadscrew::getStopPosition(LeadscrewStopPosition stop) {
  ElsCriticalSection guard;
  return stop == LeadscrewStopPosition::LEFT ? m_leftStopPosition : m_rightStopPosition;
}
int Leadscrew::getCurrentPosition() {
  ElsCriticalSection guard;
  return m_currentPosition;
}
void Leadscrew::setCurrentPosition(int position) {
  ElsCriticalSection guard;
  m_currentPosition = position;
  m_expectedPosition = position;
}

int Leadscrew::boundedInt(double value) {
  if (!std::isfinite(value)) {
    ElsCriticalSection guard;
    m_numericFault = true;
    return 0;
  }
  if (value >= INT32_MAX) return INT32_MAX;
  if (value <= INT32_MIN) return INT32_MIN;
  return static_cast<int>(value);
}
int Leadscrew::getPositionError() {
  double error;
  { ElsCriticalSection guard; error = m_expectedPosition - double(m_currentPosition); }
  return boundedInt(error);
}
int Leadscrew::getStoppingDistanceInPulses() {
  return boundedInt(double(m_leadscrewSpeed) * m_leadscrewSpeed / (2.0 * m_leadscrewAccel));
}
int Leadscrew::getTargetSpeedDistanceInPulses() {
  float ratio;
  { ElsCriticalSection guard; ratio = m_ratio; }
  const double difference = m_leadscrewSpeed - double(m_spindle->getEstimatedVelocityInPPS()) * ratio;
  return boundedInt(difference * std::abs(difference) / (2.0 * m_leadscrewAccel));
}
bool Leadscrew::initializeOutputs() {
  if (m_initAttempted) return false;
  m_initAttempted = true;
  const bool ready = m_io->begin();
  {
    ElsCriticalSection guard;
    m_initialized = ready;
    m_ioFault = !ready;
    m_inhibit = true;
    m_lowTimestamp = m_dirTimestamp = m_lastPulseStartTimestamp = elsMicros();
    GlobalState::getInstance()->requestMotionUnlocked(MM_DISABLED);
  }
  resetProfile(elsMicros());
  return ready;
}
void Leadscrew::setFaultInhibit(bool inhibit) {
  ElsCriticalSection guard;
  m_faultInhibitLatched = m_faultInhibitLatched || inhibit;
  m_inhibit = m_faultInhibitLatched || !m_initialized || m_ioFault || m_numericFault;
  auto* state = GlobalState::getInstance();
  state->requestMotionUnlocked(MM_DISABLED);
  state->m_motionResetRequested = true;
  if (inhibit) state->setThreadSyncState(SS_UNSYNC);
}
bool Leadscrew::isOutputIdle() const {
  ElsCriticalSection guard;
  return m_initialized && !m_pulsePending && m_positionValid;
}
void Leadscrew::latchIOFault(bool uncertain) {
  ElsCriticalSection guard;
  m_ioFault = true;
  m_inhibit = true;
  if (uncertain) m_positionValid = false;
  auto* state = GlobalState::getInstance();
  state->requestMotionUnlocked(MM_DISABLED);
  state->setThreadSyncState(SS_UNSYNC);
}
void Leadscrew::resetProfile(uint32_t now) {
  m_spindle->consumePosition();
  ElsCriticalSection guard;
  m_leadscrewSpeed = 0;
  m_currentPulseDelay = initialPulseDelay;
  m_currentDirection = LeadscrewDirection::UNKNOWN;
  m_lastPulseStartTimestamp = m_lastPulseTimestamp = jogMicros = now;
  m_lastFullPulseDurationMicros = 0;
  m_expectedPosition = m_currentPosition;
  m_resetPending = false;
}

void Leadscrew::update() {
  const uint32_t now = elsMicros();
  auto* state = GlobalState::getInstance();
  bool reset;
  const auto mode = state->takeMotionRequest(reset);
  m_resetPending = m_resetPending || reset;
  bool initialized;
  { ElsCriticalSection guard; initialized = m_initialized; }
  if (!initialized) { m_spindle->consumePosition(); return; }

  const auto pulse = m_io->servicePulse(now);
  if (pulse == LeadscrewIO::PulseResult::FAULT) { latchIOFault(true); return; }
  if (pulse == LeadscrewIO::PulseResult::COMPLETE) {
    const uint32_t completedAt = elsMicros();
    ElsCriticalSection guard;
    if (!m_pulsePending) { m_ioFault = m_inhibit = true; return; }
    const int64_t completed = int64_t(m_currentPosition) + static_cast<int>(m_pulseDirection);
    if (completed < INT32_MIN || completed > INT32_MAX) {
      m_numericFault = m_inhibit = true;
      m_positionValid = false;
    } else m_currentPosition = static_cast<int>(completed);
    m_pulsePending = false;
    const uint32_t elapsed = completedAt - m_lastPulseTimestamp;
    m_lastFullPulseDurationMicros = elapsed;
    m_lastPulseTimestamp = completedAt;
#ifdef ESP32
    // RMT COMPLETE includes the configured low phase.
    m_lowTimestamp = completedAt - ELS_STEP_LOW_US;
#else
    m_lowTimestamp = completedAt;
#endif
  }
  bool fault = m_spindle->hasMotionFault();
  bool inhibited, pending;
  {
    ElsCriticalSection guard;
    fault = fault || m_numericFault || m_ioFault;
    inhibited = m_inhibit || fault;
    pending = m_pulsePending;
  }
  if (fault) setFaultInhibit(true);
  if (inhibited) state->setMotionMode(MM_DISABLED);
  if (m_resetPending || inhibited || mode == MM_DISABLED || mode == MM_UNSET) {
    m_spindle->consumePosition();
    if (!pending) resetProfile(now);
    return;
  }
  if (pulse == LeadscrewIO::PulseResult::COMPLETE) return;
  const uint32_t jogElapsed = now - jogMicros;
  const uint32_t pulseElapsed = now - m_lastPulseStartTimestamp;
  const int delta = m_spindle->consumePosition();
  bool syncBlocked = false;
  int current, left, right;
  bool leftSet, rightSet;
  float ratio;
  {
    ElsCriticalSection guard;
    current = m_currentPosition;
    left = m_leftStopPosition; right = m_rightStopPosition;
    leftSet = m_leftStopState == LeadscrewStopState::SET;
    rightSet = m_rightStopState == LeadscrewStopState::SET;
    ratio = m_ratio;
    fault = fault || m_numericFault;
    syncBlocked = m_syncInvalid;
    if (m_syncPositionState != LeadscrewSpindleSyncPositionState::UNSET && state->getThreadSyncState() == SS_UNSYNC) {
      int phase;
      if (!phaseAtPosition(current, phase)) {
        syncBlocked = true;
      } else if (m_spindle->positionUnlocked() == phase) {
        m_expectedPosition = current;
        state->setThreadSyncState(SS_SYNC);
        syncBlocked = true; // no pulse from pre-registration movement
      } else syncBlocked = true;
    }
    if (mode == MM_DISABLED || mode == MM_UNSET || fault || syncBlocked) m_expectedPosition = current;
    else if (mode == MM_JOG_LEFT || mode == MM_JOG_RIGHT) {
      if (jogElapsed > ELS_JOG_PULSE_DELAY) {
        m_expectedPosition = double(int64_t(current) + (mode == MM_JOG_LEFT ? -1500 : 1500));
        jogMicros = now;
      }
    } else m_expectedPosition += double(delta) * ratio;
  }
  if (fault) { state->setMotionMode(MM_DISABLED); return; }
  if (syncBlocked || pending) return;
  const int error = getPositionError();
  const bool hitLeft = leftSet && current <= left;
  const bool hitRight = rightSet && current >= right;
  if ((hitLeft || hitRight) && std::abs(double(error)) > double(encoderPPR) * ratio) {
    { ElsCriticalSection guard; m_expectedPosition = current; }
    if ((mode == MM_JOG_LEFT && hitLeft) || (mode == MM_JOG_RIGHT && hitRight) || (mode == MM_ENABLED && hitLeft)) {
      state->setMotionMode(MM_DISABLED);
      state->setThreadSyncState(SS_UNSYNC);
      return;
    }
  }
  if (error == 0) return;
  LeadscrewDirection next = LeadscrewDirection::UNKNOWN;
  if (error > 0 && !hitRight) next = LeadscrewDirection::RIGHT;
  else if (error < 0 && !hitLeft) next = LeadscrewDirection::LEFT;
  if (next == LeadscrewDirection::UNKNOWN) return;
  if (m_currentDirection != next && m_leadscrewSpeed == 0) {
    const uint32_t holdElapsed = now - m_lastPulseStartTimestamp;
    if (holdElapsed < ELS_DIR_HOLD_US) return;
    { ElsCriticalSection guard; m_currentDirection = next; }
    m_io->writeDirPin(next == LeadscrewDirection::RIGHT ? ELS_DIR_RIGHT : ELS_DIR_LEFT);
    m_dirTimestamp = elsMicros();
    return; // A fresh update must observe the actual DIR write plus setup time.
  }
  const int pulsesToStop = getStoppingDistanceInPulses();
  const int pulsesToTarget = getTargetSpeedDistanceInPulses();
  const int64_t nextPosition = int64_t(current) + static_cast<int>(m_currentDirection);
  const uint32_t lowElapsed = now - m_lowTimestamp;
  const uint32_t dirElapsed = now - m_dirTimestamp;
  if (pulseElapsed < m_currentPulseDelay || lowElapsed < ELS_STEP_LOW_US ||
      dirElapsed < ELS_DIR_SETUP_US) return;
  {
    ElsCriticalSection guard;
    if (nextPosition < INT32_MIN || nextPosition > INT32_MAX) m_numericFault = true;
    if (m_numericFault || m_inhibit || state->m_motionResetRequested || state->m_motionMode != mode) return;
    m_pulseDirection = m_currentDirection;
    m_pulsePending = true; // Acceptance is serialized with foreground requests; IO stays outside the guard.
  }
  const uint32_t startedAt = elsMicros();
  if (!m_io->startPulse(startedAt)) {
    { ElsCriticalSection guard; m_pulsePending = false; }
    latchIOFault(false);
    return;
  }
  m_lastPulseStartTimestamp = startedAt;
  const bool shouldStop = double(static_cast<int>(m_currentDirection)) * error < pulsesToTarget ||
      next != m_currentDirection || (leftSet && nextPosition - pulsesToStop <= left) ||
      (rightSet && nextPosition + pulsesToStop >= right);
  const float speedChange = m_leadscrewAccel * std::min<float>(m_currentPulseDelay, initialPulseDelay) / US_PER_SECOND;
  m_leadscrewSpeed = shouldStop ? std::max<float>(m_leadscrewSpeed - speedChange, 0)
      : std::min<float>(m_leadscrewSpeed + speedChange, LEADSCREW_MAX_SPEED_PPS);
  m_currentPulseDelay = m_leadscrewSpeed == 0 ? initialPulseDelay : std::min<float>(initialPulseDelay, US_PER_SECOND / m_leadscrewSpeed);
}
LeadscrewDirection Leadscrew::getCurrentDirection() {
  ElsCriticalSection guard;
  return m_currentDirection;
}
float Leadscrew::getEstimatedVelocityInMillimetersPerSecond() {
  if (motorPulsePerRevolution <= 0 || !std::isfinite(leadscrewPitch)) return 0;
  uint32_t duration;
  { ElsCriticalSection guard; duration = m_lastFullPulseDurationMicros; }
  if (!duration || duration > 1000) return 0;
  return (1000000 / duration) * leadscrewPitch / motorPulsePerRevolution;
}
