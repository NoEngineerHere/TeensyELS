#pragma once
#include <ui_encoder.h>
#include <AbleButtons.h>
#include <controls.h>

using Button = AblePullupDoubleClickerButton;
using ButtonList = AblePullupDoubleClickerButtonList;

class ButtonHandler {
 private:
  Controls& controls;
  UiEncoder encoder;

  Button m_rateIncrease;
  Button m_rateDecrease;
  Button m_modeCycle;
  Button m_threadSync;
  Button m_enable;
  Button m_lock;
  Button m_jogLeft;
  Button m_jogRight;
#ifdef ESP32
  Button m_halfNut;
  bool m_updateHoldHandled = false;
#endif

  bool m_modeHoldHandled = false;
  bool m_leftHoldHandled = false;
  bool m_rightHoldHandled = false;

  void rateIncreaseHandler();
  void rateDecreaseHandler();
  void modeCycleHandler();
  void threadSyncHandler();
  void enableHandler();
  void lockHandler();

  enum JogDirection { LEFT = -1, RIGHT = 1 };

  void jogDirectionHandler(JogDirection direction);
  void jogHandler();

 public:
  explicit ButtonHandler(Controls& controls);

  bool begin();
  void poll(uint32_t nowMs);
};
