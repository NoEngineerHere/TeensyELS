#include <config.h>
#if !defined(PIO_UNIT_TESTING) && ELS_DISPLAY == ELS_SSD1306_128_64
#include <display.h>
#include <rpm_format.h>
#include <boot_diagnostics.h>
#include <globalstate.h>
// Images
#include <icons/feedSymbol.h>
#include <icons/lockedSymbol.h>
#include <icons/pauseSymbol.h>
#include <icons/runSymbol.h>
#include <icons/threadSymbol.h>
#include <icons/unlockedSymbol.h>


bool Display::init() {
  m_failed = !m_ssd1306.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  if (m_failed) { m_leadscrew->setFaultInhibit(true); return false; }
  m_ssd1306.clearDisplay();
  return true;
}

void Display::showBootStatus(bool fault) {
  if (m_failed) return;
  m_ssd1306.clearDisplay();
  m_ssd1306.setCursor(0, 16);
  m_ssd1306.setTextSize(1);
  m_ssd1306.setTextColor(WHITE);
  m_ssd1306.print(fault ? "BOOT / FAULT" : "BOOT / LOCKED");
  m_ssd1306.display();
}

void Display::update() {
  if (m_failed) return;
  m_ssd1306.clearDisplay();

  int bytes = GlobalState::getInstance()->getOTABytes();
  int length = GlobalState::getInstance()->getOTALength();
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

  m_ssd1306.display();
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
  GlobalThreadSyncState sync = GlobalState::getInstance()->getThreadSyncState();
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
  GlobalFeedMode mode = GlobalState::getInstance()->getFeedMode();

  if (mode == GlobalFeedMode::FM_FEED) {
    m_ssd1306.drawBitmap(57, 32, feedSymbol, 64, 32, WHITE);
  } else if (mode == GlobalFeedMode::FM_THREAD) {
    m_ssd1306.drawBitmap(57, 32, threadSymbol, 64, 32, WHITE);
  }

}

void Display::drawPitch() {
  GlobalState* state = GlobalState::getInstance();
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
  GlobalState* state = GlobalState::getInstance();
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
  digitalWrite(ELS_IND_GREEN, (c & 2) == 2);
  digitalWrite(ELS_IND_RED, c & 1);

}
#endif


void Display::updateLed() {
#ifdef ELS_IND_GREEN

  GlobalState* state = GlobalState::getInstance();
  GlobalMotionMode mode = state->getMotionMode();
  GlobalButtonLock lock = GlobalState::getInstance()->getButtonLock();

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
  GlobalButtonLock lock = GlobalState::getInstance()->getButtonLock();
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