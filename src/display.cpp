#include "display.h"

#include "config.h"
#include "globalstate.h"

// Images
#include "feedSymbol.h"
#include "lockedSymbol.h"
#include "pauseSymbol.h"
#include "runSymbol.h"
#include "threadSymbol.h"
#include "unlockedSymbol.h"

void Display::init() {
#if ELS_DISPLAY == SSD1306_128_64
  if (!this->m_ssd1306.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  m_ssd1306.clearDisplay();
#endif
}

void Display::update(boolean lock, boolean enabled) {
#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.clearDisplay();
#endif

  this->drawMode();
  this->drawPitch();
  this->drawLocked(lock);
  this->drawEnabled(enabled);

#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.display();
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
      sprintf(pitch, "%dth", (int)feedPitchImperial[feedSelect] * 1000);
    }
  }

#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.setCursor(55, 8);
  m_ssd1306.setTextSize(2);
  m_ssd1306.setTextColor(WHITE);
  m_ssd1306.print(pitch);
#endif
}

void Display::drawEnabled(boolean enabled) {
#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.fillRoundRect(26, 40, 20, 20, 2, WHITE);

  if (enabled == true) {
    m_ssd1306.drawBitmap(28, 42, runSymbol, 16, 16, BLACK);
  } else {
    m_ssd1306.drawBitmap(28, 42, pauseSymbol, 16, 16, BLACK);
  }
#endif
}

void Display::drawLocked(boolean locked) {
#if ELS_DISPLAY == SSD1306_128_64
  m_ssd1306.fillRoundRect(2, 40, 20, 20, 2, WHITE);

  if (locked == true) {
    m_ssd1306.drawBitmap(4, 42, lockedSymbol, 16, 16, BLACK);
  } else {
    m_ssd1306.drawBitmap(4, 42, unlockedSymbol, 16, 16, BLACK);
  }
#endif
}