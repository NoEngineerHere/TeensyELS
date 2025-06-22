#pragma once
#include <config.h>

#if ELS_DISPLAY == ST7789_240_135
#include "ST7789_240_135.hpp"

#include <TFT_eSPI.h>
#include <SPI.h>
#include <display.h>
#include <globalstate.h>

/**
 * Add definitions for specific display member variables here
 */
class Display_ST7789_240_135 : public Display {
private:
    TFT_eSPI tft;
    char m_rpmString[10];
    char m_pitchString[10];
    GlobalFeedMode m_mode = GlobalFeedMode::FM_UNSET;
    GlobalMotionMode m_motionMode = GlobalMotionMode::MM_UNSET;
    GlobalButtonLock m_locked = GlobalButtonLock::LK_UNSET;
    GlobalThreadSyncState m_sync = GlobalThreadSyncState::SS_UNSET;
public:
    Display_ST7789_240_135(Spindle* spindle, Leadscrew* leadscrew);
    void init();
    void update();
protected:
    void drawMode();
    void drawPitch();
    void drawEnabled();
    void drawLocked();
    void drawSpindleRpm();
    void drawStopStatus();
    void drawSyncStatus();
    void updateLed();
    void writeLed();
};

#endif