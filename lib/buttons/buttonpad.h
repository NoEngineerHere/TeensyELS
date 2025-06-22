#ifdef ELS_USE_BUTTON_ARRAY
#include <leadscrew.h>
#include <spindle.h>
#include <keyarray.h>

class ButtonPad {
 private:
  Spindle *m_spindle;
  Leadscrew *m_leadscrew;
  KeyArray *m_pad;

  void rateIncreaseHandler(ButtonInfo press);
  void rateDecreaseHandler(ButtonInfo press);
  void modeCycleHandler(ButtonInfo press);
  void threadSyncHandler(ButtonInfo press);
  void halfNutHandler(ButtonInfo press);
  void enableHandler(ButtonInfo press);
  void lockHandler(ButtonInfo press);

  enum JogDirection { LEFT = -1, RIGHT = 1 };

  void jogDirectionHandler(ButtonInfo direction);
  void jogHandler(ButtonInfo press);

 public:
  ButtonPad(Spindle *spindle, Leadscrew *leadscrew, KeyArray *pad);

  volatile int keycode;
  void handle();
};
#endif