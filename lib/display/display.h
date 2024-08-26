
#include <config.h>
#include <globalstate.h>
#include <leadscrew.h>
#include <spindle.h>

#define SSD1306_128_64 0

#if ELS_DISPLAY == SSD1306_128_64

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// some displays can have different addresses, this is what we attempt to init
#define SCREEN_ADDRESS 0x3C

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#else

#error "Please choose a valid display. Refer to config.h for options"

#endif

class Display {
 private:
  Spindle* m_spindle;
  Leadscrew* m_leadscrew;
  GlobalState* m_globalState;

 public:
#if ELS_DISPLAY == SSD1306_128_64
  Adafruit_SSD1306 m_ssd1306;
#endif
  Display(Spindle* spindle, Leadscrew* leadscrew) {
    this->m_spindle = spindle;
    this->m_leadscrew = leadscrew;
    this->m_globalState = GlobalState::getInstance();
#if ELS_DISPLAY == SSD1306_128_64
    this->m_ssd1306 =
        Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, PIN_DISPLAY_RESET);
#endif
  }

  void init();
  void update();

 protected:
  void drawMode();
  void drawPitch();
  void drawEnabled();
  void drawLocked();
  void drawSpindleRpm();
  void drawStopStatus();
};