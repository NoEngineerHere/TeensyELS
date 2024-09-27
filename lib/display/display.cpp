#include <config.h>
#include <display.h>
#include <globalstate.h>

// Images
#include <icons/feedSymbol.h>
#include <icons/lockedSymbol.h>
#include <icons/pauseSymbol.h>
#include <icons/runSymbol.h>
#include <icons/threadSymbol.h>
#include <icons/unlockedSymbol.h>

void Display::init() {
#if ELS_DISPLAY == SSD1306_128_64
  if (!this->m_ssd1306.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  m_ssd1306.clearDisplay();
#endif
}

void Display::update() {
#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.clearDisplay();
#endif

  drawMode();
  drawPitch();
  drawLocked();
  drawEnabled();
  drawSpindleRpm();
  
  drawStopStatus();

#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.display();
#endif
}

void Display::drawSpindleRpm() {
#if ELS_DISPLAY == SSD1306_128_64
  int rpm = m_spindle->getEstimatedVelocityInRPM();
  char rpmString[10];
  m_ssd1306.setCursor(0, 0);
  m_ssd1306.setTextSize(1);
  m_ssd1306.setTextColor(WHITE);
  // pad the rpm with spaces so the RPM text stays in the same place
  sprintf(rpmString, "%4dRPM", rpm);
  m_ssd1306.print(rpmString);
#endif
}

void Display::drawStopStatus() {
#if ELS_DISPLAY == SSD1306_128_64
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
#endif
}

void Display::drawSyncStatus() {
  GlobalThreadSyncState sync = GlobalState::getInstance()->getThreadSyncState();
#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.setCursor(0,16);
  m_ssd1306.setTextSize(2);
  m_ssd1306.setTextColor(WHITE);
  m_ssd1306.print("SYNC");
  // cross it out if not synced
  if(sync == GlobalThreadSyncState::UNSYNC) {
    m_ssd1306.drawLine(0, 16, 64, 16, WHITE);
  }
#endif
}

void Display::drawMode() {
  GlobalFeedMode mode = GlobalState::getInstance()->getFeedMode();

#if ELS_DISPLAY == SSD1306_128_64
  if (mode == GlobalFeedMode::FEED) {
    m_ssd1306.drawBitmap(57, 32, feedSymbol, 64, 32, WHITE);
  } else if (mode == GlobalFeedMode::THREAD) {
    m_ssd1306.drawBitmap(57, 32, threadSymbol, 64, 32, WHITE);
  }
#endif
}

void Display::drawPitch() {
  GlobalState *state = GlobalState::getInstance();
  GlobalUnitMode unit = state->getUnitMode();
  GlobalFeedMode mode = state->getFeedMode();
  int feedSelect = state->getFeedSelect();
  char pitch[10];
  if (unit == GlobalUnitMode::METRIC) {
    if (mode == GlobalFeedMode::THREAD) {
      sprintf(pitch, "%.2fmm", threadPitchMetric[feedSelect]);
    } else {
      sprintf(pitch, "%.2fmm", feedPitchMetric[feedSelect]);
    }
  } else {
    if (mode == GlobalFeedMode::THREAD) {
      sprintf(pitch, "%dTPI", (int)threadPitchImperial[feedSelect]);
    } else {
      sprintf(pitch, "%dth", (int)(feedPitchImperial[feedSelect] * 1000));
    }
  }

#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.setCursor(55, 8);
  m_ssd1306.setTextSize(2);
  m_ssd1306.setTextColor(WHITE);
  m_ssd1306.print(pitch);
#endif
}

void Display::drawEnabled() {
  GlobalState *state = GlobalState::getInstance();
  GlobalMotionMode mode = state->getMotionMode();

#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.fillRoundRect(26, 40, 20, 20, 2, WHITE);
  switch (mode) {
    case GlobalMotionMode::DISABLED:
      m_ssd1306.drawBitmap(28, 42, pauseSymbol, 16, 16, BLACK);
      break;
    case GlobalMotionMode::JOG:
      // todo bitmap for jogging
      m_ssd1306.setCursor(28, 42);
      m_ssd1306.setTextSize(2);
      m_ssd1306.setTextColor(BLACK);
      m_ssd1306.print("J");
      break;
    case GlobalMotionMode::ENABLED:
      m_ssd1306.drawBitmap(28, 42, runSymbol, 16, 16, BLACK);
      break;
  }
#endif
}

void Display::drawLocked() {
  GlobalButtonLock lock = GlobalState::getInstance()->getButtonLock();
#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.fillRoundRect(2, 40, 20, 20, 2, WHITE);
  switch (lock) {
    case GlobalButtonLock::LOCKED:
      m_ssd1306.drawBitmap(4, 42, lockedSymbol, 16, 16, BLACK);
      break;
    case GlobalButtonLock::UNLOCKED:
      m_ssd1306.drawBitmap(4, 42, unlockedSymbol, 16, 16, BLACK);
      break;
  }
#endif
}