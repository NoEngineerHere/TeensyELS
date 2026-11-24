#include "matrix_scanner.h"

static MatrixEvent gesture(MatrixGesture kind, uint16_t bit) {
  return {kind, static_cast<uint8_t>(__builtin_ctz(bit))};
}

void MatrixScanner::sample(uint16_t mask, uint32_t now) {
  event = {};
  const bool halt = (mask & haltBit) != 0;
  if (halt != ((raw & haltBit) != 0)) haltAt = now;
  if (raw != mask) { raw = mask; rawAt = now; }
  if (halt != haltPressed && now - haltAt >= 10) {
    haltPressed = halt;
    if (halt) event = gesture(MatrixGesture::Press, haltBit);
  }

  if (halt || (mask & (mask - 1)) || (pressed && mask && mask != pressed)) {
    suppressed = true;
    pressed = 0;
  }
  if (suppressed) {
    if (!mask && now - rawAt >= 10) suppressed = false;
    return;
  }

  if (now - rawAt < 10) return;
  if (mask != pressed) {
    if (mask) {
      event = gesture(MatrixGesture::Press, mask);
      pressedAt = now;
      held = false;
    } else if (!held) {
      event = gesture(MatrixGesture::Click, pressed);
    }
    pressed = mask;
  } else if (pressed && !held && now - pressedAt >= 1000) {
    held = true;
    event = gesture(MatrixGesture::Hold, pressed);
  }
}

MatrixEvent MatrixScanner::takeEvent() {
  const auto taken = event;
  event = {};
  return taken;
}
