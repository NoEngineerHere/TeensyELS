#pragma once
#include <els/hardware_config.h>

#if ELS_INPUT_LAYOUT == ELS_INPUT_DIRECT
#include "direct_panel.h"
using PanelInput = ButtonHandler;
#else
#include <matrix_gpio.h>
#include <ui_encoder.h>
#include "legacy_panel.h"

template<class Binding>
class MatrixPanelInput {
  Binding binding;
  MatrixScanner scanner{Binding::haltMask};
  UiEncoder encoder;
public:
  explicit MatrixPanelInput(Controls& controls) : binding(controls) {}
  bool begin() {
    initializeMatrix();
    if constexpr (hardware.encoder.a >= 0) return encoder.begin();
    return true;
  }
  void poll(uint32_t nowMs) {
    scanner.sample(readMatrix(), nowMs);
    binding.key(scanner.takeEvent());
    if constexpr (hardware.encoder.a >= 0) {
      const auto detents = encoder.poll();
      if (detents && !scanner.blocked()) binding.turn(detents);
    }
  }
};
using PanelInput = MatrixPanelInput<LegacyPanel>;
#endif
