#pragma once

#include <els/hardware_config.h>
#include <display_bus.h>
#include "display_status.h"
#include <cstdio>
#include <atomic>


#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
#include <Adafruit_SSD1306.h>
#else
#include <Adafruit_ST7789.h>
#if ELS_SCREEN == ELS_SCREEN_ST7789_320X240_LVGL
#include <lvgl.h>
#endif
#endif

class Display {
private:
  Spindle* m_spindle;
  Leadscrew* m_leadscrew;
  GlobalState& m_globalState;
  const Mk2Panel* m_panel;
  std::atomic<bool> m_failed{false};
  bool composeText(DisplayLines& lines, const DisplayStatus& status) const {
    if (m_globalState.hasOTA()) {
      std::snprintf(lines[0], sizeof(lines[0]), "Updating");
      const int total = m_globalState.getOTALength();
      if (total > 0) std::snprintf(lines[1], sizeof(lines[1]), "%d / %d",
          m_globalState.getOTABytes(), total);
      return true;
    }
    if (!m_panel) return false;
    formatDisplayLines(lines, status, m_panel);
    return true;
  }
  void drawLines(const DisplayLines& lines);
#if ELS_SCREEN == ELS_SCREEN_ST7789_240X135
  char m_rpmString[8]{};
  char m_pitchString[10]{};
  GlobalFeedMode m_mode = GlobalFeedMode::FM_UNSET;
  GlobalMotionMode m_motionMode = GlobalMotionMode::MM_UNSET;
  GlobalButtonLock m_locked = GlobalButtonLock::LK_UNSET;
  GlobalThreadSyncState m_sync = GlobalThreadSyncState::SS_UNSET;
  bool m_textShown = false;
  void resetStatus();
#elif ELS_SCREEN == ELS_SCREEN_ST7789_320X240_LVGL
  lv_display_t* m_screen = nullptr;
  lv_obj_t* m_labels[7]{};
  DisplayLines m_lines{};
  alignas(4) uint16_t m_pixels[320 * 8];
  uint32_t m_lastServiceMs = 0;
#endif

public:
#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
  static constexpr int screenWidth = 128, screenHeight = 64;
  Adafruit_SSD1306 m_ssd1306{screenWidth, screenHeight, &displayI2cBus(),
      hardware.oled.reset, hardware.oled.frequencyHz, hardware.oled.restoreHz};
private:
  static constexpr size_t frameSize = screenWidth * screenHeight / 8;
  size_t m_frameOffset = frameSize;
  bool m_sendWindow = false;
  void queueFrame() { m_sendWindow = true; m_frameOffset = 0; }
public:
#else
  Adafruit_ST7789 tft{&displaySpiBus(), hardware.tft.cs, hardware.tft.dc, hardware.tft.reset};
#endif
  Display(GlobalState& state, Spindle* spindle, Leadscrew* leadscrew, const Mk2Panel* panel = nullptr)
      : m_spindle(spindle), m_leadscrew(leadscrew), m_globalState(state), m_panel(panel) {}

  bool init();
  void showBootStatus(bool fault);
  void update();
  // Pump at most one transfer. True when there is no frame pending.
#if ELS_SCREEN != ELS_SCREEN_ST7789_240X135
  bool service(uint32_t nowMs);
#else
  bool service(uint32_t) { return true; }
#endif
  bool failed() const { return m_failed.load(); }

protected:
  void drawMode(GlobalFeedMode mode);
  void drawPitch(const char* pitch);
  void drawEnabled(GlobalMotionMode mode);
  void drawLocked(GlobalButtonLock lock);
  void drawSpindleRpm(const char* rpm);
  void drawStopStatus(bool left, bool right);
  void drawSyncStatus(GlobalThreadSyncState sync);
  void drawLegacy(const DisplayStatus& s) {
    drawMode(s.feed);
    drawPitch(s.pitch);
    drawLocked(s.lock);
    drawEnabled(s.motion);
    drawSpindleRpm(s.rpm);
    drawSyncStatus(s.sync);
    drawStopStatus(s.leftStop, s.rightStop);
  }
};