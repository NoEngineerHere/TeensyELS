#pragma once
#include <config.h>
#include <leadscrew.h>
#include <spindle.h>

#if ELS_BOARD != ELS_BOARD_UNSET
#include <AbleButtons.h>
using Button = AblePullupDoubleClickerButton;
using ButtonList = AblePullupDoubleClickerButtonList;
#endif

class ButtonHandler {
  virtual void handle() = 0;
};
