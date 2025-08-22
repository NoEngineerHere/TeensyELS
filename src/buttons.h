#include <AbleButtons.h>
#include <leadscrew.h>
#include <spindle.h>
#include "../lib/interfaces/system_interfaces.h"

using Button = AblePullupDoubleClickerButton;
using ButtonList = AblePullupDoubleClickerButtonList;

class ButtonHandler : public IButtonHandler {
 private:
  Spindle *m_spindle;
  Leadscrew *m_leadscrew;

  Button m_rateIncrease;
  Button m_rateDecrease;
  Button m_modeCycle;
  Button m_threadSync;
  Button m_halfNut;
  Button m_enable;
  Button m_lock;
  Button m_jogLeft;
  Button m_jogRight;

  void rateIncreaseHandler();
  void rateDecreaseHandler();
  void modeCycleHandler();
  void threadSyncHandler();
  void halfNutHandler();
  void enableHandler();
  void lockHandler();

  enum JogDirection { LEFT = -1, RIGHT = 1 };

  void jogDirectionHandler(JogDirection direction);
  void jogHandler();

 public:
  ButtonHandler(Spindle *spindle, Leadscrew *leadscrew);

  volatile int keycode;
  void handle();
};
