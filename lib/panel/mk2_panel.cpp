#include "mk2_panel.h"
#include <critical_section.h>

namespace {
enum Key { Mode, Rate, Stops, Left, Ok, Right, Halt, Menu, Enable };
}

Mk2Panel::Mk2Panel(Controls& controls, bool supportsUpdate) : controls(controls) {
  selected.menuCount = supportsUpdate ? 5 : 4;
}
Mk2Selection Mk2Panel::selection() const {
  ElsCriticalSection guard;
  return selected;
}
void Mk2Panel::select(Mk2Selection selection) {
  ElsCriticalSection guard;
  selected = selection;
}
void Mk2Panel::turn(int64_t detents) {
  if (!detents) return;
  auto next = selection();
  switch (next.view) {
    case Mk2View::Rate: controls.changePitch(detents); return;
    case Mk2View::Stops:
      next.stop = detents < 0 ? LeadscrewStopPosition::LEFT : LeadscrewStopPosition::RIGHT;
      break;
    case Mk2View::Menu: {
      const int64_t length = next.menuCount;
      next.menu = (next.menu + length + detents % length) % length;
      break;
    }
  }
  select(next);
}
void Mk2Panel::arrow(LeadscrewStopPosition stop, MatrixGesture gesture) {
  const auto view = selection().view;
  if (view == Mk2View::Rate) {
    if (gesture == MatrixGesture::Hold) controls.toggleStop(stop);
    else controls.jogToStop(stop, Controls::JogGesture::MatrixToggle);
  } else if (gesture == MatrixGesture::Click) {
    turn(stop == LeadscrewStopPosition::LEFT ? -1 : 1);
  }
}
void Mk2Panel::key(MatrixEvent event) {
  if (event.gesture == MatrixGesture::Press && event.key == Halt) {
    controls.stopMotion();
    return;
  }
  if (event.gesture == MatrixGesture::Hold) {
    switch (event.key) {
      case Mode: controls.toggleUnits(); break;
      case Left: arrow(LeadscrewStopPosition::LEFT, event.gesture); break;
      case Right: arrow(LeadscrewStopPosition::RIGHT, event.gesture); break;
      case Ok: if (selection().view == Mk2View::Rate) controls.requestDisplayReset(); break;
    }
    return;
  }
  if (event.gesture != MatrixGesture::Click) return;

  auto next = selection();
  switch (event.key) {
    case Mode: controls.toggleFeedMode(); return;
    case Rate: next.view = Mk2View::Rate; break;
    case Stops: next.view = Mk2View::Stops; break;
    case Menu: next.view = next.view == Mk2View::Menu ? Mk2View::Rate : Mk2View::Menu; break;
    case Enable: controls.toggleMotion(); return;
    case Left: arrow(LeadscrewStopPosition::LEFT, event.gesture); return;
    case Right: arrow(LeadscrewStopPosition::RIGHT, event.gesture); return;
    case Ok:
      switch (next.view) {
        case Mk2View::Rate: controls.syncThread(); break;
        case Mk2View::Stops: controls.toggleStop(next.stop); break;
        case Mk2View::Menu:
          switch (static_cast<Mk2Menu>(next.menu)) {
            case Mk2Menu::Lock: controls.toggleLock(); break;
            case Mk2Menu::Units: controls.toggleUnits(); break;
            case Mk2Menu::Sync: controls.syncThread(); break;
            case Mk2Menu::ResetDisplay: controls.requestDisplayReset(); break;
            case Mk2Menu::Update: controls.requestUpdate(); break;
          }
          break;
      }
      return;
    default: return;
  }
  select(next);
}
