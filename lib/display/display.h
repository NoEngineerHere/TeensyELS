#pragma once  

#include <config.h>
#include <globalstate.h>
#include <leadscrew.h>
#include <spindle.h>

class Display {
public:
  void init();
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

  Spindle* m_spindle;
  Leadscrew* m_leadscrew;
  GlobalState* m_globalState;
#ifdef ELS_UI_ENCODER
  EncoderColour firstColour = EC_NONE;
  EncoderColour secondColour = EC_NONE;
#endif
  bool updating = false;
};

#if ELS_DISPLAY == ST7789_240_135
#include "ST7789_240_135/ST7789_240_135.hpp"
#elif ELS_DISPLAY == SSD1306_128_64
#include "SSD1306_128_64/SSD1306_128_64.hpp"
#else
#error "ELS_DISPLAY is not valid, please check your config.h file"
#endif