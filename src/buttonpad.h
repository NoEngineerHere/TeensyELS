#pragma once
#include <els/hardware_config.h>
#if ELS_INPUT_LAYOUT == ELS_INPUT_LEGACY_MATRIX
#include <controls.h>
#include <keyarray.h>

class ButtonPad {
 private:
  Controls& controls;
  KeyArray& pad;

 public:
  ButtonPad(Controls& controls, KeyArray& pad) : controls(controls), pad(pad) {}
  void handle();
};
#endif