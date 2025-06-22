#include <config.h>

#if ELS_DISPLAY == ST7789_240_135
#include "ST7789_240_135.hpp"

// Images
#include <icons/feedSymbol.h>
#include <icons/lockedSymbol.h>
#include <icons/pauseSymbol.h>
#include <icons/runSymbol.h>
#include <icons/threadSymbol.h>
#include <icons/unlockedSymbol.h>

Display_ST7789_240_135::Display_ST7789_240_135(Spindle* spindle, Leadscrew* leadscrew) {
  this->m_spindle = spindle;
  this->m_leadscrew = leadscrew;
  this->m_globalState = GlobalState::getInstance();
}

// Add some basic bitmap scaling for double-resolution screens.
uint8_t spread(uint8_t in) {
  uint8_t a = (in & 0x1) | ((in & 0x2) << 1) | ((in & 0x4) << 2) | ((in & 0x8) << 3);
  return a | (a << 1);
}

void shiftByte(const uint8_t source[], uint8_t dest[], int sidx, int didx) {
  uint8_t s = source[sidx];
  uint8_t a = (s & 0xF0) >> 4;
  uint8_t b = s & 0x0F;
  dest[didx] = spread(a);
  dest[didx + 1] = spread(b);
}

void fillDest(const uint8_t source[], uint8_t dest[], int sidx, int didx, int bytes) {
  for (int i = 0; i < bytes; i++) {
    shiftByte(source, dest, sidx + i, didx + (i * 2));
  }
}

void ScaleBMP(const uint8_t source[], uint8_t dest[], int sizex, int sizey) {
  int bytesx = sizex / 8;
  for (int i = 0; i < sizey; i++) {
    fillDest(source, dest, i * bytesx, i * bytesx * 4, bytesx);
    fillDest(source, dest, i * bytesx, (bytesx * 2) + (i * bytesx * 4), bytesx);
  }
}

void Display_ST7789_240_135::init() {
  strcpy(m_rpmString, "");
  strcpy(m_pitchString, "");
  m_mode = GlobalFeedMode::FM_UNSET;
  m_motionMode = GlobalMotionMode::MM_UNSET;
  m_locked = GlobalButtonLock::LK_UNSET;
  m_sync = GlobalThreadSyncState::SS_UNSET;
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
}

void Display_ST7789_240_135::update() {
  //  tft.fillScreen(TFT_BLACK); // Rely on localised blanking to avoid blink, for now.
  if (GlobalState::getInstance()->getDisplayReset()) {
    init();
  }

  int bytes = GlobalState::getInstance()->getOTABytes();
  int length = GlobalState::getInstance()->getOTALength();
  if (GlobalState::getInstance()->hasOTA()) {
    if (!updating) {
      tft.fillRect(0, 0, 240, 135, TFT_BLACK);
      tft.setCursor(10, 10);
      tft.setTextSize(3);
      tft.setTextColor(TFT_WHITE);
      tft.print("UPDATING");
      updating = true;
    }
    tft.drawRect(0, 70, 240, 40, TFT_WHITE);
    int percent = (((float)(bytes * 240)) / ((float)length));
    if (bytes > 0)tft.fillRect(0, 70, percent, 40, TFT_WHITE);

  } else {
    drawMode();
    drawPitch();
    drawLocked();
    drawEnabled();
    drawSpindleRpm();
    drawSyncStatus();

    drawStopStatus();
  }
#if ELS_BOARD == ELS_BOARD_ESP32
  writeLed();
#endif
}

void Display_ST7789_240_135::drawSpindleRpm() {
  int rpm = m_spindle->getEstimatedVelocityInRPM();
  char rpmString[10];
  sprintf(rpmString, "%4dRPM", rpm);
  // pad the rpm with spaces so the RPM text stays in the same place
  if (strcmp(rpmString, m_rpmString)) {
    tft.setCursor(100, 0);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.fillRect(100, 0, 85, 32, TFT_BLACK);
    tft.print(rpmString);
    strcpy(m_rpmString, rpmString);
  }
}

void Display_ST7789_240_135::drawStopStatus() {
  tft.setCursor(0, 0);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.fillRect(0, 0, 100, 32, TFT_BLACK);
  if (m_leadscrew->getStopPositionState(LeadscrewStopPosition::LEFT) ==
    LeadscrewStopState::SET) {
    tft.print("[");
  } else {
    tft.print(" ");
  }
  if (m_leadscrew->getStopPositionState(LeadscrewStopPosition::RIGHT) ==
    LeadscrewStopState::SET) {
    tft.print("]");
  } else {
    tft.print(" ");
  }
}

void Display_ST7789_240_135::drawSyncStatus() {
  GlobalThreadSyncState sync = GlobalState::getInstance()->getThreadSyncState();
  if (sync == m_sync)return;
  m_sync = sync;
  if (sync == GlobalThreadSyncState::SS_UNSYNC) {
    tft.drawLine(0, 32, 70, 48, TFT_RED);
    tft.setCursor(0, 32);
    tft.setTextSize(3);
    tft.setTextColor(TFT_RED);
    tft.print("SYNC");
  } else {
    tft.drawLine(0, 32, 70, 48, TFT_BLACK);
    tft.setCursor(0, 32);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.print("SYNC");
  }
}

void Display_ST7789_240_135::drawMode() {
  GlobalFeedMode mode = GlobalState::getInstance()->getFeedMode();

  if (mode == m_mode)return;
  m_mode = mode;
  tft.fillRect(104, 64, 128, 64, TFT_BLACK);
  if (mode == GlobalFeedMode::FM_FEED) {
    uint8_t scaled[128 * 64 / 2];
    ScaleBMP(feedSymbol, scaled, 128, 64);
    tft.drawBitmap(104, 64, scaled, 128, 64, TFT_WHITE);
  } else if (mode == GlobalFeedMode::FM_THREAD) {
    uint8_t scaled[128 * 64 / 2];
    ScaleBMP(threadSymbol, scaled, 128, 64);
    tft.drawBitmap(104, 64, scaled, 128, 64, TFT_WHITE);
  }
}

void Display_ST7789_240_135::drawPitch() {
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

  if (!strcmp(pitch, m_pitchString))return;
  strcpy(m_pitchString, pitch);
  tft.fillRect(110, 32, 130, 21, TFT_BLACK);
  tft.setCursor(110, 32);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.print(pitch);
}

void Display_ST7789_240_135::drawEnabled() {
  GlobalState* state = GlobalState::getInstance();
  GlobalMotionMode mode = state->getMotionMode();

  if (mode == m_motionMode)return;
  m_motionMode = mode;
  tft.fillRoundRect(52, 80, 40, 40, 4, mode == GlobalMotionMode::MM_ENABLED ? TFT_GREEN : ((mode == GlobalMotionMode::MM_JOG_LEFT || mode == GlobalMotionMode::MM_JOG_RIGHT) ? TFT_YELLOW : TFT_WHITE));
  uint8_t scaled[128];
  GlobalButtonLock lock = GlobalState::getInstance()->getButtonLock();
  switch (mode) {
  case GlobalMotionMode::MM_DISABLED:
    ScaleBMP(pauseSymbol, scaled, 16, 16);
    tft.drawBitmap(56, 84, scaled, 32, 32, TFT_BLACK);
    break;
  case GlobalMotionMode::MM_JOG_LEFT:
  case GlobalMotionMode::MM_JOG_RIGHT:
    // todo bitmap for jogging
    tft.setCursor(55, 88);
    tft.setTextSize(4);
    tft.setTextColor(TFT_BLACK);
    tft.print("<>");
    break;
  case GlobalMotionMode::MM_ENABLED:
    ScaleBMP(runSymbol, scaled, 16, 16);
    tft.drawBitmap(56, 84, scaled, 32, 32, TFT_BLACK);
    break;
  }
  updateLed();
}


void Display_ST7789_240_135::writeLed() {
#ifdef ELS_UI_ENCODER
  int64_t time = micros() / 250000;
  EncoderColour c = time % 2 == 1 ? firstColour : secondColour;
  digitalWrite(ELS_IND_GREEN, (c & 2) == 2);
  digitalWrite(ELS_IND_RED, c & 1);
#endif
}



void Display_ST7789_240_135::updateLed() {
#ifdef ELS_UI_ENCODER

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

void Display_ST7789_240_135::drawLocked() {
  GlobalButtonLock lock = GlobalState::getInstance()->getButtonLock();
  if (lock == m_locked)return;
  m_locked = lock;

  tft.fillRoundRect(4, 80, 40, 40, 4, lock == LK_LOCKED ? TFT_RED : TFT_GREEN);
  uint8_t scaled[128];
  switch (lock) {
  case GlobalButtonLock::LK_LOCKED:
    ScaleBMP(lockedSymbol, scaled, 16, 16);
    tft.drawBitmap(8, 84, scaled, 32, 32, TFT_BLACK);
    break;
  case GlobalButtonLock::LK_UNLOCKED:
    ScaleBMP(unlockedSymbol, scaled, 16, 16);
    tft.drawBitmap(8, 84, scaled, 32, 32, TFT_BLACK);
    break;
  }
  updateLed();
}
#endif