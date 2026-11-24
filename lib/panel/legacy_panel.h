#pragma once
#include "matrix_scanner.h"
#include <controls.h>

class LegacyPanel {
  Controls& controls;
public:
  static constexpr uint16_t haltMask = 0;
  explicit LegacyPanel(Controls& controls) : controls(controls) {}
  void key(MatrixEvent event);
  void turn(int64_t detents) { controls.changePitch(detents); }
};
