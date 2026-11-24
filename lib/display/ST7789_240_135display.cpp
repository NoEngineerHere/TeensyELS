#include <els/hardware_config.h>

#if (!defined(PIO_UNIT_TESTING) || defined(ELS_TEST_TFT)) && ELS_SCREEN == ELS_SCREEN_ST7789_240X135
#include <display.h>
#include "tft_transport.h"
#include <cstring>
// Images
#include <icons/feedSymbol.h>
#include <icons/lockedSymbol.h>
#include <icons/pauseSymbol.h>
#include <icons/runSymbol.h>
#include <icons/threadSymbol.h>
#include <icons/unlockedSymbol.h>

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
    std::memcpy(dest + i * bytesx * 4 + bytesx * 2, dest + i * bytesx * 4, bytesx * 2);
  }
}

void Display::resetStatus() {
  m_rpmString[0] = m_pitchString[0] = m_positionString[0] = '\0';
  m_mode = FM_UNSET;
  m_motionMode = MM_UNSET;
  m_locked = LK_UNSET;
  m_sync = SS_UNSET;
}

bool Display::init() {
  initializeTft(tft, 135, 240);
  tft.fillScreen(ST77XX_BLACK);
  resetStatus();
  m_textShown = false;
  return true; // The transport cannot establish panel presence.
}

void Display::showBootStatus(bool fault) {
  if (m_failed) return;
  tft.fillScreen(ST77XX_BLACK);
  m_textShown = true;
  tft.setCursor(10, 50);
  tft.setTextSize(2);
  tft.setTextColor(fault ? ST77XX_RED : ST77XX_WHITE);
  tft.print(fault ? "BOOT / FAULT" : "BOOT / LOCKED");
}

void Display::update() {
  if (m_globalState.getDisplayReset()) init();
  const auto status = readDisplayStatus(m_globalState, *m_spindle, *m_leadscrew);
  DisplayLines lines{};
  if (composeText(lines, status)) {
    drawLines(lines);
    return;
  }
  if (m_textShown) {
    tft.fillScreen(ST77XX_BLACK);
    resetStatus();
    m_textShown = false;
  }
  drawLegacy(status);
}

void Display::drawLines(const DisplayLines& lines) {
  if (!m_textShown) tft.fillScreen(ST77XX_BLACK);
  m_textShown = true;
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  for (unsigned i = 0; i < 7; ++i) {
    // Opaque padding removes previous text without clearing the whole TFT.
    char row[21];
    std::snprintf(row, sizeof(row), "%-20.20s", lines[i]);
    tft.setCursor(0, 4 + 18 * i);
    tft.print(row);
  }
}

void Display::drawPosition(const char* position) {
  if (std::strcmp(position, m_positionString) == 0) return;
  std::strcpy(m_positionString, position);
  char row[21];
  std::snprintf(row, sizeof(row), "%-20s", position);
  tft.setCursor(0, 56);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.print(row);
}

void Display::drawSpindleRpm(const char* rpmString) {
  // pad the rpm with spaces so the RPM text stays in the same place
  if (strcmp(rpmString, m_rpmString)) {
    tft.setCursor(100, 0);
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_WHITE);
    tft.fillRect(100, 0, 126, 32, ST77XX_BLACK);
    tft.print(rpmString);
    std::memcpy(m_rpmString, rpmString, sizeof(m_rpmString));
  }
}

void Display::drawStopStatus(bool left, bool right) {
  tft.setCursor(0, 0);
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  tft.fillRect(0, 0, 100, 32, ST77XX_BLACK);
  if (left) {
    tft.print("[");
  } else {
    tft.print(" ");
  }
  if (right) {
    tft.print("]");
  } else {
    tft.print(" ");
  }
}

void Display::drawSyncStatus(GlobalThreadSyncState sync) {
  if (sync == m_sync)return;
  m_sync = sync;
  if (sync == GlobalThreadSyncState::SS_UNSYNC) {
    tft.drawLine(0, 32, 70, 48, ST77XX_RED);
    tft.setCursor(0, 32);
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_RED);
    tft.print("SYNC");
  } else {
    tft.drawLine(0, 32, 70, 48, ST77XX_BLACK);
    tft.setCursor(0, 32);
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("SYNC");
  }
}

void Display::drawMode(GlobalFeedMode mode) {

  if (mode == m_mode)return;
  m_mode = mode;
  tft.fillRect(104, 64, 128, 64, ST77XX_BLACK);
  if (mode == FM_FEED || mode == FM_THREAD) {
    uint8_t scaled[128 * 64 / 8];
    ScaleBMP(mode == FM_FEED ? feedSymbol : threadSymbol, scaled, 64, 32);
    tft.drawBitmap(104, 64, scaled, 128, 64, ST77XX_WHITE);
  }
}

void Display::drawPitch(const char* pitch) {

  if (!strcmp(pitch, m_pitchString))return;
  strcpy(m_pitchString, pitch);
  tft.fillRect(110, 32, 130, 21, ST77XX_BLACK);
  tft.setCursor(110, 32);
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(pitch);
}

void Display::drawEnabled(GlobalMotionMode mode) {
  if (mode == m_motionMode)return;
  m_motionMode = mode;
  tft.fillRoundRect(52, 80, 40, 40, 4, mode == GlobalMotionMode::MM_ENABLED ? ST77XX_GREEN : ((mode == GlobalMotionMode::MM_JOG_LEFT || mode == GlobalMotionMode::MM_JOG_RIGHT) ? ST77XX_YELLOW : ST77XX_WHITE));
  uint8_t scaled[128];
  switch (mode) {
  case GlobalMotionMode::MM_DISABLED:
    ScaleBMP(pauseSymbol, scaled, 16, 16);
    tft.drawBitmap(56, 84, scaled, 32, 32, ST77XX_BLACK);
    break;
  case GlobalMotionMode::MM_JOG_LEFT:
  case GlobalMotionMode::MM_JOG_RIGHT:
    // todo bitmap for jogging
    tft.setCursor(55, 88);
    tft.setTextSize(4);
    tft.setTextColor(ST77XX_BLACK);
    tft.print("<>");
    break;
  case GlobalMotionMode::MM_ENABLED:
    ScaleBMP(runSymbol, scaled, 16, 16);
    tft.drawBitmap(56, 84, scaled, 32, 32, ST77XX_BLACK);
    break;
  }
}

void Display::drawLocked(GlobalButtonLock lock) {
  if (lock == m_locked)return;
  m_locked = lock;

  tft.fillRoundRect(4, 80, 40, 40, 4, lock == LK_LOCKED ? ST77XX_RED : ST77XX_GREEN);
  uint8_t scaled[128];
  switch (lock) {
  case GlobalButtonLock::LK_LOCKED:
    ScaleBMP(lockedSymbol, scaled, 16, 16);
    tft.drawBitmap(8, 84, scaled, 32, 32, ST77XX_BLACK);
    break;
  case GlobalButtonLock::LK_UNLOCKED:
    ScaleBMP(unlockedSymbol, scaled, 16, 16);
    tft.drawBitmap(8, 84, scaled, 32, 32, ST77XX_BLACK);
    break;
  }
}
#endif