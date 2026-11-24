#pragma once
#include "matrix_scanner.h"
#include <controls.h>

enum class Mk2View { Rate, Stops, Menu };
enum class Mk2Menu { Lock, Units, Sync, ResetDisplay, Update };
struct Mk2Selection {
  Mk2View view = Mk2View::Rate;
  LeadscrewStopPosition stop = LeadscrewStopPosition::LEFT;
  unsigned menu = 0;
  unsigned menuCount = 4;
};

class Mk2Panel {
  Controls& controls;
  Mk2Selection selected;
  void select(Mk2Selection selection);
  void arrow(LeadscrewStopPosition stop, MatrixGesture gesture);
public:
  static constexpr uint16_t haltMask = matrixBit(2, 0);
  Mk2Panel(Controls& controls, bool supportsUpdate);
  Mk2Selection selection() const;
  void key(MatrixEvent event);
  void turn(int64_t detents);
};
