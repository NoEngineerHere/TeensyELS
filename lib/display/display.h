#pragma once

#include <config.h>
#include <globalstate.h>
#include <leadscrew.h>
#include <spindle.h>


#if ELS_DISPLAY == ELS_SSD1306_128_64

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// some displays can have different addresses, this is what we attempt to init
#define SCREEN_ADDRESS 0x3C

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#elif  ELS_DISPLAY == ST7789_240_135
#include <TFT_eSPI.h>
#else

#error "Please choose a valid display. Refer to config.h for options"

#endif

class Display {
private:
  Spindle* m_spindle;
  Leadscrew* m_leadscrew;
  GlobalState* m_globalState;
#ifdef ELS_UI_ENCODER
  EncoderColour firstColour = EC_NONE;
  EncoderColour secondColour = EC_NONE;
#endif
  bool updating = false;
  bool m_failed = false;
#if ELS_DISPLAY == ST7789_240_135
  char m_rpmString[8]{};
  char m_pitchString[10];
  GlobalFeedMode m_mode = GlobalFeedMode::FM_UNSET;
  GlobalMotionMode m_motionMode = GlobalMotionMode::MM_UNSET;
  GlobalButtonLock m_locked = GlobalButtonLock::LK_UNSET;
  GlobalThreadSyncState m_sync = GlobalThreadSyncState::SS_UNSET;
#endif

public:
#if ELS_DISPLAY == ELS_SSD1306_128_64
  Adafruit_SSD1306 m_ssd1306;
#elif ELS_DISPLAY == ST7789_240_135
  TFT_eSPI tft = TFT_eSPI();
#endif
  Display(Spindle* spindle, Leadscrew* leadscrew) {
    this->m_spindle = spindle;
    this->m_leadscrew = leadscrew;
    this->m_globalState = GlobalState::getInstance();
#if ELS_DISPLAY == ELS_SSD1306_128_64
    this->m_ssd1306 =
      Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, PIN_DISPLAY_RESET);
#elif ELS_DISPLAY == ST7789_240_135
#endif
  }

  bool init();
  void showBootStatus(bool fault);
  void update();

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