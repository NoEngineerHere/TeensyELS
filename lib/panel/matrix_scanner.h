#pragma once
#include <cstdint>

enum class MatrixGesture { None, Press, Click, Hold };
struct MatrixEvent {
  MatrixGesture gesture = MatrixGesture::None;
  uint8_t key = 0; // Row-major index, not a GPIO or legacy key code.
};
constexpr uint16_t matrixBit(unsigned row, unsigned column) { return 1u << (3 * row + column); }

class MatrixScanner {
  const uint16_t haltBit;
  uint16_t raw = 0, pressed = 0;
  uint32_t rawAt = 0, pressedAt = 0, haltAt = 0;
  bool haltPressed = false, suppressed = false, held = false;
  MatrixEvent event;
public:
  explicit MatrixScanner(uint16_t halt = 0) : haltBit(halt) {}
  void sample(uint16_t mask, uint32_t nowMs);
  MatrixEvent takeEvent();
  bool blocked() const { return suppressed; }
};
