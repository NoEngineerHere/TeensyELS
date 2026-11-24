#pragma once

#include <els/hardware_config.h>
#include <globalstate.h>
#include <leadscrew.h>
#include <spindle.h>
#include <atomic>


#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// some displays can have different addresses, this is what we attempt to init
inline constexpr int SCREEN_ADDRESS = hardware.oled.address;

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#elif  ELS_SCREEN == ELS_SCREEN_ST7789_240X135
#include <TFT_eSPI.h>
#else

#error "Please choose a valid display. Refer to config.h for options"

#endif

class Display {
private:
  Spindle* m_spindle;
  Leadscrew* m_leadscrew;
  GlobalState& m_globalState;
#ifdef ESP32
  EncoderColour firstColour = EC_NONE;
  EncoderColour secondColour = EC_NONE;
#endif
  bool updating = false;
  std::atomic<bool> m_failed{false};
#if ELS_SCREEN == ELS_SCREEN_ST7789_240X135
  char m_rpmString[8]{};
  char m_pitchString[10];
  GlobalFeedMode m_mode = GlobalFeedMode::FM_UNSET;
  GlobalMotionMode m_motionMode = GlobalMotionMode::MM_UNSET;
  GlobalButtonLock m_locked = GlobalButtonLock::LK_UNSET;
  GlobalThreadSyncState m_sync = GlobalThreadSyncState::SS_UNSET;
#endif

public:
#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
  Adafruit_SSD1306 m_ssd1306;
private:
  static constexpr size_t frameSize = SCREEN_WIDTH * SCREEN_HEIGHT / 8;
  size_t m_frameOffset = frameSize;
  bool m_sendWindow = false;
  void queueFrame() { m_sendWindow = true; m_frameOffset = 0; }
public:
#elif ELS_SCREEN == ELS_SCREEN_ST7789_240X135
  TFT_eSPI tft = TFT_eSPI();
#endif
  Display(GlobalState& state, Spindle* spindle, Leadscrew* leadscrew)
      : m_spindle(spindle), m_leadscrew(leadscrew), m_globalState(state) {
#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
    this->m_ssd1306 =
      Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, hardware.oled.reset);
#elif ELS_SCREEN == ELS_SCREEN_ST7789_240X135
#endif
  }

  bool init();
  void showBootStatus(bool fault);
  void update();
  // Pump at most one transfer. True when there is no frame pending.
#if ELS_SCREEN == ELS_SCREEN_SSD1306_128X64
  bool service(uint32_t nowMs);
#else
  bool service(uint32_t) { return true; }
#endif
  bool failed() const { return m_failed.load(); }

protected:
  void drawMode();
  void drawPitch();
  void drawEnabled();
  void drawLocked();
  void drawSpindleRpm();
  void drawStopStatus();
  void drawSyncStatus();
  void updateLed();
  void writeLed();
};