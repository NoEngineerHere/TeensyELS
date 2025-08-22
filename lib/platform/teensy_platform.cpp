#include "teensy_platform.h"

#ifndef ESP32

#include <config.h>
#include "../interfaces/system_interfaces.h"
#include <leadscrew_io_teensy.h>
#include <spindle.h>
#include <leadscrew.h>

// Forward declarations to avoid library dependency issues
class ButtonHandler;
#include <Arduino.h>
#include <IntervalTimer.h>

// Global timer instance for Teensy
static IntervalTimer teensyTimer;

TeensyPlatform::TeensyPlatform() {
    initializePinConfiguration();
    
    // Set display configuration
    m_displayConfig = {
        .type = DisplayType::SSD1306_128x64,
        .resetPin = -1  // No reset pin for Teensy SSD1306
    };
    
    // Set platform capabilities
    m_capabilities = {
        .hasWiFi = false,
        .hasOTA = false,
        .hasDualCore = false,
        .hasButtonMatrix = false,
        .hasRMT = false,
        .hasHardwareTimers = true,
        .platformName = "Teensy 4.1"
    };
}

void TeensyPlatform::initializePinConfiguration() {
    m_pins = {
        // Spindle encoder
        .spindleEncoderA = ELS_SPINDLE_ENCODER_A,
        .spindleEncoderB = ELS_SPINDLE_ENCODER_B,
        
        // Leadscrew stepper
        .leadscrewStep = ELS_LEADSCREW_STEP,
        .leadscrewDir = ELS_LEADSCREW_DIR,
        .stepperEnable = ELS_STEPPER_ENA,
        
        // UI elements
        .uiEncoderA = ELS_UI_ENCODER_A,
        .uiEncoderB = ELS_UI_ENCODER_B,
        .indicatorRed = ELS_IND_RED,
        .indicatorGreen = ELS_IND_GREEN,
        
        // Individual button pins for Teensy
        .rateIncreaseButton = ELS_RATE_INCREASE_BUTTON,
        .rateDecreaseButton = ELS_RATE_DECREASE_BUTTON,
        .modeCycleButton = ELS_MODE_CYCLE_BUTTON,
        .threadSyncButton = ELS_THREAD_SYNC_BUTTON,
        .halfNutButton = ELS_HALF_NUT_BUTTON,
        .enableButton = ELS_ENABLE_BUTTON,
        .lockButton = ELS_LOCK_BUTTON,
        .jogLeftButton = ELS_JOG_LEFT_BUTTON,
        .jogRightButton = ELS_JOG_RIGHT_BUTTON
    };
}

const HardwarePins& TeensyPlatform::getHardwarePins() const {
    return m_pins;
}

const DisplayConfig& TeensyPlatform::getDisplayConfig() const {
    return m_displayConfig;
}

const PlatformCapabilities& TeensyPlatform::getCapabilities() const {
    return m_capabilities;
}

void TeensyPlatform::initializeHardware() {
    initializeSerialDebug();
    initializePinConfiguration();
    
    // Initialize stepper pins
    ::pinMode(m_pins.leadscrewStep, OUTPUT);
    ::pinMode(m_pins.leadscrewDir, OUTPUT);
    ::pinMode(m_pins.stepperEnable, OUTPUT);
    ::digitalWrite(m_pins.stepperEnable, LOW);  // Enable stepper driver
    
    // Initialize LED indicators
#ifdef ELS_IND_GREEN
    ::pinMode(m_pins.indicatorGreen, OUTPUT);
    ::pinMode(m_pins.indicatorRed, OUTPUT);
#endif
    
    // Initialize button pins with pull-ups
    ::pinMode(m_pins.rateIncreaseButton, INPUT_PULLUP);
    ::pinMode(m_pins.rateDecreaseButton, INPUT_PULLUP);
    ::pinMode(m_pins.modeCycleButton, INPUT_PULLUP);
    ::pinMode(m_pins.threadSyncButton, INPUT_PULLUP);
    ::pinMode(m_pins.halfNutButton, INPUT_PULLUP);
    ::pinMode(m_pins.enableButton, INPUT_PULLUP);
    ::pinMode(m_pins.lockButton, INPUT_PULLUP);
    ::pinMode(m_pins.jogLeftButton, INPUT_PULLUP);
    ::pinMode(m_pins.jogRightButton, INPUT_PULLUP);
}

void TeensyPlatform::initializeSerialDebug() {
    Serial.begin(921600);
    while (!Serial && millis() < 1000) {
        // Wait for serial connection (up to 1 second)
    }
    Serial.println("Teensy Platform Initialized");
}

void TeensyPlatform::initializeTimer(void (*callback)(), uint32_t intervalMicros) {
    teensyTimer.begin(callback, intervalMicros);
}

void TeensyPlatform::initializeDisplay() {
    // Display-specific initialization handled by Display class
    // Platform just ensures I2C pins are ready
}

std::unique_ptr<LeadscrewIO> TeensyPlatform::createLeadscrewIO() {
    return std::make_unique<LeadscrewIOTeensy>();
}

std::unique_ptr<ISpindle> TeensyPlatform::createSpindle() {
#ifdef ELS_SPINDLE_DRIVEN
    return std::make_unique<Spindle>();
#else
    return std::make_unique<Spindle>(m_pins.spindleEncoderA, m_pins.spindleEncoderB);
#endif
}


void TeensyPlatform::digitalWrite(int pin, bool value) {
    ::digitalWrite(pin, value ? HIGH : LOW);
}

bool TeensyPlatform::digitalRead(int pin) {
    return ::digitalRead(pin) == HIGH;
}

void TeensyPlatform::pinMode(int pin, int mode) {
    ::pinMode(pin, mode);
}

uint64_t TeensyPlatform::getMicros() {
    return ::micros();
}

void TeensyPlatform::delayMicros(uint32_t micros) {
    ::delayMicroseconds(micros);
}

void TeensyPlatform::enableRealTimeMode() {
    // Teensy doesn't require special real-time mode setup
    // IntervalTimer already provides precise timing
}

void TeensyPlatform::disableWatchdog() {
    // Teensy doesn't have a watchdog to disable by default
}

void TeensyPlatform::yieldProcessor() {
    // Teensy doesn't require explicit yielding in single-core environment
    ::yield();
}

#endif // !ESP32