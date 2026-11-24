#if defined(ESP32) || defined(CORE_TEENSY)
#include "board.h"
#if ELS_INPUT_LAYOUT != ELS_INPUT_DIRECT
#include <Arduino.h>
#include "matrix_gpio.h"

void initializeMatrix() {
  for (auto row : hardware.matrix.rows) pinMode(row, INPUT_PULLDOWN);
  for (auto column : hardware.matrix.columns) pinMode(column, INPUT);
}
uint16_t readMatrix() {
  uint16_t mask = 0;
  for (unsigned column = 0; column < 3; ++column) {
    const int pin = hardware.matrix.columns[column];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    delayMicroseconds(2);
    for (unsigned row = 0; row < 3; ++row)
      if (digitalRead(hardware.matrix.rows[row])) mask |= 1u << (3 * row + column);
    pinMode(pin, INPUT);
  }
  return mask;
}
#endif
#endif
