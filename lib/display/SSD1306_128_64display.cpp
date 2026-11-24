#include <els/hardware_config.h>
#if (!defined(PIO_UNIT_TESTING) || defined(ELS_TEST_OLED)) && ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
#include <display.h>
#include <Wire.h>
#include <algorithm>
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
  auto& bus = displayI2cBus();
  auto transfer = [&bus](uint8_t control, const uint8_t* bytes, size_t count) {
    bus.beginTransmission(hardware.oled.address);
    const bool accepted = bus.write(control) == 1 && bus.write(bytes, count) == count;
    const uint8_t status = bus.endTransmission();
    return accepted && status == 0;
  };
  constexpr uint8_t window[] = {0x21, 0, screenWidth - 1, 0x22, 0, screenHeight / 8 - 1};
  constexpr size_t chunk = 31; // Control byte + data fits even a 32-byte Wire buffer.
  bus.setClock(hardware.oled.frequencyHz);
  bool ok;
  if (m_sendWindow) {
    ok = transfer(0x00, window, sizeof(window));
    m_sendWindow = false;
  } else {
    const size_t count = std::min(chunk, frameSize - m_frameOffset);
    ok = transfer(0x40, m_ssd1306.getBuffer() + m_frameOffset, count);
    m_frameOffset += count;
  }
  bus.setClock(hardware.oled.restoreHz);
  if (!ok) m_failed.store(true);
  return ok && m_frameOffset == frameSize;
}

bool Display::init() {
  if (m_failed) return false;
  m_failed = !m_ssd1306.begin(SSD1306_SWITCHCAPVCC, hardware.oled.address, true, false);
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
  if (m_globalState.getDisplayReset() && !init()) return;
  m_ssd1306.clearDisplay();
  const auto status = readDisplayStatus(m_globalState, *m_spindle, *m_leadscrew);
  DisplayLines lines{};
  if (composeText(lines, status)) drawLines(lines);
  else drawLegacy(status);
  queueFrame();
}

void Display::drawLines(const DisplayLines& lines) {
  m_ssd1306.setTextSize(1);
  m_ssd1306.setTextColor(WHITE);
  for (unsigned i = 0; i < 7; ++i) {
    m_ssd1306.setCursor(0, i * 8);
    m_ssd1306.print(lines[i]);
  }
}

void Display::drawPosition(const char* position) {
  m_ssd1306.setCursor(0, 56);
  m_ssd1306.setTextSize(1);
  m_ssd1306.setTextColor(WHITE);
  m_ssd1306.print(position);
}

void Display::drawSpindleRpm(const char* rpmString) {
  m_ssd1306.setCursor(0, 0);
  m_ssd1306.setTextSize(1);
  m_ssd1306.setTextColor(WHITE);
  // pad the rpm with spaces so the RPM text stays in the same place
  m_ssd1306.print(rpmString);
}

void Display::drawStopStatus(bool left, bool right) {
  m_ssd1306.setCursor(0, 8);
  m_ssd1306.setTextSize(1);
  m_ssd1306.setTextColor(WHITE);
  if (left) {
    m_ssd1306.print("[");
  } else {
    m_ssd1306.print(" ");
  }
  if (right) {
    m_ssd1306.print("]");
  } else {
    m_ssd1306.print(" ");
  }
}

void Display::drawSyncStatus(GlobalThreadSyncState sync) {
  m_ssd1306.setCursor(0, 16);
  m_ssd1306.setTextSize(2);
  m_ssd1306.setTextColor(WHITE);
  m_ssd1306.print("SYNC");
  // cross it out if not synced
  if (sync == GlobalThreadSyncState::SS_UNSYNC) {
    m_ssd1306.drawLine(0, 16, 64, 16, WHITE);
  }
}

void Display::drawMode(GlobalFeedMode mode) {

  if (mode == GlobalFeedMode::FM_FEED) {
    m_ssd1306.drawBitmap(57, 24, feedSymbol, 64, 32, WHITE);
  } else if (mode == GlobalFeedMode::FM_THREAD) {
    m_ssd1306.drawBitmap(57, 24, threadSymbol, 64, 32, WHITE);
  } else if (mode == FM_THREAD_REVERSE) {
    m_ssd1306.setTextSize(2);
    m_ssd1306.setTextColor(WHITE);
    m_ssd1306.setCursor(57, 38);
    m_ssd1306.print("Rev");
  }

}

void Display::drawPitch(const char* pitch) {
  m_ssd1306.setCursor(55, 8);
  m_ssd1306.setTextSize(2);
  m_ssd1306.setTextColor(WHITE);
  m_ssd1306.print(pitch);
}

void Display::drawEnabled(GlobalMotionMode mode) {
  m_ssd1306.fillRoundRect(26, 36, 20, 20, 2, WHITE);
  switch (mode) {
  case GlobalMotionMode::MM_DISABLED:
    m_ssd1306.drawBitmap(28, 38, pauseSymbol, 16, 16, BLACK);
    break;
  case GlobalMotionMode::MM_JOG_LEFT:
  case GlobalMotionMode::MM_JOG_RIGHT:
    // todo bitmap for jogging
    m_ssd1306.setCursor(28, 38);
    m_ssd1306.setTextSize(2);
    m_ssd1306.setTextColor(BLACK);
    m_ssd1306.print("J");
    break;
  case GlobalMotionMode::MM_ENABLED:
    m_ssd1306.drawBitmap(28, 38, runSymbol, 16, 16, BLACK);
    break;
  }
}

void Display::drawLocked(GlobalButtonLock lock) {
  m_ssd1306.fillRoundRect(2, 36, 20, 20, 2, WHITE);
  switch (lock) {
  case GlobalButtonLock::LK_LOCKED:
    m_ssd1306.drawBitmap(4, 38, lockedSymbol, 16, 16, BLACK);
    break;
  case GlobalButtonLock::LK_UNLOCKED:
    m_ssd1306.drawBitmap(4, 38, unlockedSymbol, 16, 16, BLACK);
    break;
  }
}
#endif