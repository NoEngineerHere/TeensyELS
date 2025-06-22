#pragma once
#include "config.h"

/**
 * @brief      Macro to validate that all button names in BUTTON_DEFINITIONS are valid HID_VALUES
 *
 * This macro creates a compile-time check to ensure that every button name in the
 * BUTTON_DEFINITIONS array exists in the HID_VALUES array. If any button name is
 * not found, the compilation will fail with an error.
 */
#define VALIDATE_BUTTON_DEFINITIONS() \
  static_assert([]() { \
    for (const auto& def : BUTTON_DEFINITIONS) { \
      bool found = false; \
      for (const auto& valid : HID_VALUES) { \
        if (strcmp(def.name, valid) == 0) { \
          found = true; \
          break; \
        } \
      } \
      if (!found) return false; \
    } \
    return true; \
  }(), "All button names in BUTTON_DEFINITIONS must be valid HID_VALUES")

VALIDATE_BUTTON_DEFINITIONS();


/**
 * @brief      Macro to validate that row and col pin definitions do not overlap
 *
 * This macro creates a compile-time check to ensure that the ROW_PINS and COL_PINS
 * arrays do not contain any of the same values. If any pin is found in both arrays
 * the compilation will fail with an error.
 */
#define VALIDATE_PIN_DEFINITIONS() \
  static_assert([]() constexpr { \
    /* Create arrays to hold unique row and column pins */ \
    int row_pins[ARRAY_SIZE(BUTTON_DEFINITIONS)] = {}; \
    int col_pins[ARRAY_SIZE(BUTTON_DEFINITIONS)] = {}; \
    size_t row_count = 0; \
    size_t col_count = 0; \
    /* Extract unique row and column pins from BUTTON_DEFINITIONS */ \
    for (const auto& def : BUTTON_DEFINITIONS) { \
      /* Check for unique row pins */ \
      bool row_found = false; \
      for (size_t i = 0; i < row_count; ++i) { \
        if (row_pins[i] == def.row) { \
          row_found = true; \
          break; \
        } \
      } \
      if (!row_found) { \
        row_pins[row_count++] = def.row; \
      } \
      /* Check for unique column pins */ \
      if (def.col != -1) { \
        bool col_found = false; \
        for (size_t i = 0; i < col_count; ++i) { \
          if (col_pins[i] == def.col) { \
            col_found = true; \
            break; \
          } \
        } \
        if (!col_found) { \
          col_pins[col_count++] = def.col; \
        } \
      } \
    } \
    /* Check for overlapping pins */ \
    for (size_t i = 0; i < row_count; ++i) { \
      for (size_t j = 0; j < col_count; ++j) { \
        if (row_pins[i] == col_pins[j]) { \
          return false; \
        } \
      } \
    } \
    return true; \
  }(), "BUTTON_DEFINITIONS cannot have overlapping row and column pins")

VALIDATE_PIN_DEFINITIONS();
