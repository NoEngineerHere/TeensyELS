#ifdef ELS_USE_BUTTON_ARRAY
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