#pragma once
#include <config.h>

#if ELS_DISPLAY == SSD1306_128_64

#include <SSD1306_128_64/SSD1306_128_64.hpp>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class Display_SSD1306_128_64 : public Display {
private:
    Adafruit_SSD1306 m_ssd1306;
public:
    Display_SSD1306_128_64(Spindle* spindle, Leadscrew* leadscrew);
};

#endif
