#include <els/hardware_config.h>
#if !defined(PIO_UNIT_TESTING) && ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
#include <display.h>
#include <rpm_format.h>
#include <boot_diagnostics.h>
#include <algorithm>
#include <globalstate.h>
// Images
#include <icons/feedSymbol.h>
#include <icons/lockedSymbol.h>
#include <icons/pauseSymbol.h>
#include <icons/runSymbol.h>
#include <icons/threadSymbol.h>
#include <icons/unlockedSymbol.h>


bool Display::service(uint32_t) {
  if (m_failed) return false;
  if (m_frameOffset == frameSize) return true;
  auto transfer = [](uint8_t control, const uint8_t* bytes, size_t count) {
    Wire.beginTransmission(SCREEN_ADDRESS);
    const bool accepted = Wire.write(control) == 1 && Wire.write(bytes, count) == count;
    const uint8_t status = Wire.endTransmission();
    return accepted && status == 0;
  };
  constexpr uint8_t window[] = {0x21, 0, SCREEN_WIDTH - 1, 0x22, 0, SCREEN_HEIGHT / 8 - 1};
  constexpr size_t chunk = 31; // Control byte + data fits even a 32-byte Wire buffer.
  Wire.setClock(hardware.oled.frequencyHz);
  bool ok;
  if (m_sendWindow) {
    ok = transfer(0x00, window, sizeof(window));
    m_sendWindow = false;
  } else {
    const size_t count = std::min(chunk, frameSize - m_frameOffset);
    ok = transfer(0x40, m_ssd1306.getBuffer() + m_frameOffset, count);
    m_frameOffset += count;
  }
  Wire.setClock(hardware.oled.restoreHz);
  if (!ok) {
    m_leadscrew->setFaultInhibit(true);
    m_failed.store(true);
  }
  return ok && m_frameOffset == frameSize;
}

bool Display::init() {
  if (m_failed) return false;
  m_failed = !m_ssd1306.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  if (m_failed) m_leadscrew->setFaultInhibit(true);
  return !m_failed;
}

void Display::showBootStatus(bool fault) {
  if (m_failed || m_frameOffset != frameSize) return;
  m_ssd1306.clearDisplay();
  m_ssd1306.setCursor(0, 16);
  m_ssd1306.setTextSize(1);
  m_ssd1306.setTextColor(WHITE);
  m_ssd1306.print(fault ? "BOOT / FAULT" : "BOOT / LOCKED");
  queueFrame();
}

void Display::update() {
  if (m_failed || m_frameOffset != frameSize) return;
  m_ssd1306.clearDisplay();

  int bytes = m_globalState.getOTABytes();
  int length = m_globalState.getOTALength();
  if (bytes > 0) {
  } else {
    drawMode();
    drawPitch();
    drawLocked();
    drawEnabled();
    drawSpindleRpm();
    drawSyncStatus();

    drawStopStatus();
  }
#ifdef ESP32
  writeLed();
#endif

  queueFrame();
}

void Display::drawSpindleRpm() {
  char rpmString[8];
  formatRpm(m_spindle->getEstimatedVelocityInRPM(), rpmString);
  m_ssd1306.setCursor(0, 0);
  m_ssd1306.setTextSize(1);
  m_ssd1306.setTextColor(WHITE);
  // pad the rpm with spaces so the RPM text stays in the same place
  m_ssd1306.print(rpmString);
}

void Display::drawStopStatus() {
  m_ssd1306.setCursor(0, 8);
  m_ssd1306.setTextSize(1);
  m_ssd1306.setTextColor(WHITE);
  if (m_leadscrew->getStopPositionState(LeadscrewStopPosition::LEFT) ==
    LeadscrewStopState::SET) {
    m_ssd1306.print("[");
  } else {
    m_ssd1306.print(" ");
  }
  if (m_leadscrew->getStopPositionState(LeadscrewStopPosition::RIGHT) ==
    LeadscrewStopState::SET) {
    m_ssd1306.print("]");
  } else {
    m_ssd1306.print(" ");
  }
}

void Display::drawSyncStatus() {
  GlobalThreadSyncState sync = m_globalState.getThreadSyncState();
  m_ssd1306.setCursor(0, 16);
  m_ssd1306.setTextSize(2);
  m_ssd1306.setTextColor(WHITE);
  m_ssd1306.print("SYNC");
  // cross it out if not synced
  if (sync == GlobalThreadSyncState::SS_UNSYNC) {
    m_ssd1306.drawLine(0, 16, 64, 16, WHITE);
  }
}

void Display::drawMode() {
  GlobalFeedMode mode = m_globalState.getFeedMode();

  if (mode == GlobalFeedMode::FM_FEED) {
    m_ssd1306.drawBitmap(57, 32, feedSymbol, 64, 32, WHITE);
  } else if (mode == GlobalFeedMode::FM_THREAD) {
    m_ssd1306.drawBitmap(57, 32, threadSymbol, 64, 32, WHITE);
  }

}

void Display::drawPitch() {
  GlobalState* state = &m_globalState;
  GlobalUnitMode unit = state->getUnitMode();
  GlobalFeedMode mode = state->getFeedMode();
  int feedSelect = state->getFeedSelect();
  char pitch[10];
  if (unit == GlobalUnitMode::METRIC) {
    if (mode == GlobalFeedMode::FM_THREAD) {
      sprintf(pitch, "%.2fmm", threadPitchMetric[feedSelect]);
    } else {
      sprintf(pitch, "%.2fmm", feedPitchMetric[feedSelect]);
    }
  } else {
    if (mode == GlobalFeedMode::FM_THREAD) {
      sprintf(pitch, "%dTPI", (int)threadPitchImperial[feedSelect]);
    } else {
      sprintf(pitch, "%dth", (int)(feedPitchImperial[feedSelect] * 1000));
    }
  }

  m_ssd1306.setCursor(55, 8);
  m_ssd1306.setTextSize(2);
  m_ssd1306.setTextColor(WHITE);
  m_ssd1306.print(pitch);
}

void Display::drawEnabled() {
  GlobalState* state = &m_globalState;
  GlobalMotionMode mode = state->getMotionMode();

  m_ssd1306.fillRoundRect(26, 40, 20, 20, 2, WHITE);
  switch (mode) {
  case GlobalMotionMode::MM_DISABLED:
    m_ssd1306.drawBitmap(28, 42, pauseSymbol, 16, 16, BLACK);
    break;
  case GlobalMotionMode::MM_JOG_LEFT:
  case GlobalMotionMode::MM_JOG_RIGHT:
    // todo bitmap for jogging
    m_ssd1306.setCursor(28, 42);
    m_ssd1306.setTextSize(2);
    m_ssd1306.setTextColor(BLACK);
    m_ssd1306.print("J");
    break;
  case GlobalMotionMode::MM_ENABLED:
    m_ssd1306.drawBitmap(28, 42, runSymbol, 16, 16, BLACK);
    break;
  }
  updateLed();
}

#ifdef ESP32   // TODO Make portable
void Display::writeLed() {
  int64_t time = micros() / 250000;
  EncoderColour c = time % 2 == 1 ? firstColour : secondColour;
  digitalWrite(hardware.indicators.green, (c & 2) == 2);
  digitalWrite(hardware.indicators.red, c & 1);

}
#endif


void Display::updateLed() {
#ifdef ESP32

  GlobalState* state = &m_globalState;
  GlobalMotionMode mode = state->getMotionMode();
  GlobalButtonLock lock = m_globalState.getButtonLock();

  switch (mode) {
  case GlobalMotionMode::MM_DISABLED:
    firstColour = lock == LK_LOCKED ? EC_RED : EC_NONE;
    secondColour = lock == LK_LOCKED ? EC_RED : EC_NONE;
    break;
  case GlobalMotionMode::MM_JOG_LEFT:
  case GlobalMotionMode::MM_JOG_RIGHT:
    firstColour = EC_YELLOW;
    secondColour = EC_YELLOW;
    break;
  case GlobalMotionMode::MM_ENABLED:
    firstColour = lock == LK_LOCKED ? EC_RED : EC_GREEN;
    secondColour = EC_GREEN;
    break;
  }
#endif

}

void Display::drawLocked() {
  GlobalButtonLock lock = m_globalState.getButtonLock();
  m_ssd1306.fillRoundRect(2, 40, 20, 20, 2, WHITE);
  switch (lock) {
  case GlobalButtonLock::LK_LOCKED:
    m_ssd1306.drawBitmap(4, 42, lockedSymbol, 16, 16, BLACK);
    break;
  case GlobalButtonLock::LK_UNLOCKED:
    m_ssd1306.drawBitmap(4, 42, unlockedSymbol, 16, 16, BLACK);
    break;
  }
}
#endif