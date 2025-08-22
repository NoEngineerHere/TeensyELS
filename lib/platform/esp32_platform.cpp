#include "esp32_platform.h"
#include <config.h>
#include "../interfaces/system_interfaces.h"

#ifdef ESP32

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <leadscrew_io_esp.h>
#include <spindle.h>
#include "../../src/keyarray.h"
#include "../../src/buttonpad.h"
#include "../../src/ESPCommsManager.h"

ESP32Platform::ESP32Platform() {
    initializePinConfiguration();
    
    // Set display configuration
    m_displayConfig = {
        .type = DisplayType::ST7789_240x135,
        .resetPin = -1  // Reset handled by TFT_eSPI library
    };
    
    // Set platform capabilities
    m_capabilities = {
        .hasWiFi = true,
        .hasOTA = true,
        .hasDualCore = true,
        .hasButtonMatrix = true,
        .hasRMT = true,
        .hasHardwareTimers = true,
        .platformName = "ESP32 LilyGO T-Display"
    };
}

void ESP32Platform::initializePinConfiguration() {
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
        
        // Individual buttons (not used on ESP32 with matrix)
        .rateIncreaseButton = -1,
        .rateDecreaseButton = -1,
        .modeCycleButton = -1,
        .threadSyncButton = -1,
        .halfNutButton = -1,
        .enableButton = -1,
        .lockButton = -1,
        .jogLeftButton = -1,
        .jogRightButton = -1,
        
        // ESP32 button matrix pins
        .padH1 = ELS_PAD_H1,
        .padH2 = ELS_PAD_H2,
        .padH3 = ELS_PAD_H3,
        .padV1 = ELS_PAD_V1,
        .padV2 = ELS_PAD_V2,
        .padV3 = ELS_PAD_V3
    };
}

const HardwarePins& ESP32Platform::getHardwarePins() const {
    return m_pins;
}

const DisplayConfig& ESP32Platform::getDisplayConfig() const {
    return m_displayConfig;
}

const PlatformCapabilities& ESP32Platform::getCapabilities() const {
    return m_capabilities;
}

void ESP32Platform::initializeHardware() {
    initializeSerialDebug();
    initializePinConfiguration();
    
    // Initialize stepper pins
    ::pinMode(m_pins.leadscrewStep, OUTPUT);
    ::pinMode(m_pins.leadscrewDir, OUTPUT);
    ::pinMode(m_pins.stepperEnable, OUTPUT);
    ::digitalWrite(m_pins.stepperEnable, LOW);  // Enable stepper driver
    
    // Initialize LED indicators
    ::pinMode(m_pins.indicatorGreen, OUTPUT);
    ::pinMode(m_pins.indicatorRed, OUTPUT);
    
    // Initialize RMT for precise step generation
#ifdef ELS_USE_RMT
    initializeRMT();
#endif
    
    // WiFi and OTA will be initialized by CommsManager when needed
}

void ESP32Platform::initializeSerialDebug() {
    Serial.begin(921600);
    while (!Serial && millis() < 1000) {
        // Wait for serial connection (up to 1 second)
    }
    Serial.println("ESP32 Platform Initialized");
}

void ESP32Platform::initializeTimer(void (*callback)(), uint32_t intervalMicros) {
    // ESP32 uses task scheduler instead of hardware timer for main callback
    // This will be handled in initializeTaskScheduler()
}

void ESP32Platform::initializeDisplay() {
    // Display initialization handled by TFT_eSPI library
    // Platform ensures SPI pins are configured correctly
}

void ESP32Platform::initializeTaskScheduler() {
    // Create dual-core task scheduler for real-time performance
    initializeDualCoreScheduler();
    setupWatchdogTasks();
}

void ESP32Platform::initializeDualCoreScheduler() {
    // Task creation will be handled by main application using this platform
    // This method sets up the scheduler environment
    disableLoopWDT();
    esp_task_wdt_delete(xTaskGetHandle("IDLE0"));
    esp_task_wdt_delete(xTaskGetHandle("IDLE1"));
}

void ESP32Platform::setupWatchdogTasks() {
    // Watchdog management for real-time tasks
    // Individual tasks will call esp_task_wdt_reset() as needed
}

void ESP32Platform::initializeRMT() {
#ifdef ELS_USE_RMT
    // RMT initialization for precise step pulse generation
    // This will be handled by LeadscrewIOESP when created
#endif
}

std::unique_ptr<LeadscrewIO> ESP32Platform::createLeadscrewIO() {
    return std::make_unique<LeadscrewIOESP>();
}

std::unique_ptr<ISpindle> ESP32Platform::createSpindle() {
#ifdef ELS_SPINDLE_DRIVEN
    return std::make_unique<Spindle>();
#else
    return std::make_unique<Spindle>(m_pins.spindleEncoderA, m_pins.spindleEncoderB);
#endif
}


std::unique_ptr<IKeyArray> ESP32Platform::createKeyArray(ILeadscrew* leadscrew) {
    auto concreteLeadscrew = static_cast<Leadscrew*>(leadscrew);
    return std::make_unique<KeyArray>(concreteLeadscrew);
}

std::unique_ptr<ICommsManager> ESP32Platform::createCommsManager() {
    return std::make_unique<ESPCommsManager>();
}

void ESP32Platform::digitalWrite(int pin, bool value) {
    ::digitalWrite(pin, value ? HIGH : LOW);
}

bool ESP32Platform::digitalRead(int pin) {
    return ::digitalRead(pin) == HIGH;
}

void ESP32Platform::pinMode(int pin, int mode) {
    ::pinMode(pin, mode);
}

uint64_t ESP32Platform::getMicros() {
    return ::micros();
}

void ESP32Platform::delayMicros(uint32_t micros) {
    ::delayMicroseconds(micros);
}

void ESP32Platform::enableRealTimeMode() {
    // ESP32 real-time optimizations
    // Set CPU frequency to maximum
    setCpuFrequencyMhz(240);
    
    // Configure task scheduler for real-time performance
    // This is handled in the task creation process
}

void ESP32Platform::disableWatchdog() {
    disableLoopWDT();
}

void ESP32Platform::yieldProcessor() {
    // ESP32 dual-core yield
    vTaskDelay(1);
}

#endif // ESP32