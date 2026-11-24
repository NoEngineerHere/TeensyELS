#pragma once
#include <els/hardware_config.h>

#if ELS_INPUT_LAYOUT == ELS_INPUT_DIRECT
#include "direct_panel.h"
using PanelInput = ButtonHandler;
#else
#include <matrix_gpio.h>
#include <ui_encoder.h>
#include <board.h>
#if ELS_INPUT_LAYOUT == ELS_INPUT_LEGACY_MATRIX
#include "legacy_panel.h"
#else
#include "mk2_panel.h"
#endif

template<class Binding>
class MatrixPanelInput {
  Binding binding;
  MatrixScanner scanner{Binding::haltMask};
  UiEncoder encoder;
public:
  explicit MatrixPanelInput(Controls& controls) : binding(controls, compiledProcessor == Processor::Esp32) {}
  const Binding& actions() const { return binding; }
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
#if ELS_INPUT_LAYOUT == ELS_INPUT_LEGACY_MATRIX
using PanelInput = MatrixPanelInput<LegacyPanel>;
#else
using PanelInput = MatrixPanelInput<Mk2Panel>;
#endif
#endif
