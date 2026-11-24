#pragma once
#include "matrix_scanner.h"
#include <controls.h>

class LegacyPanel {
  Controls& controls;
  const bool supportsUpdate;
public:
  static constexpr uint16_t haltMask = 0;
  LegacyPanel(Controls& controls, bool supportsUpdate) : controls(controls), supportsUpdate(supportsUpdate) {}
  void key(MatrixEvent event);
  void turn(int64_t detents) { controls.changePitch(detents); }
};
