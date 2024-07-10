// This file contains all hardware configs for your system

#pragma once

/**
 * Display
 *
 * This setting allows you to select what type of display you want to use.
 * The selection will hopefully grow as time goes on!
 *
 * Options:
 *   SSD1306_128_64: 128x64 oled
 */
#define ELS_DISPLAY SSD1306_128_64

#if ELS_DISPLAY == SSD1306_128_64
// define this if you have a dedicated pin for the oled reset
#define PIN_DISPLAY_RESET -1
#endif
