#pragma once

#include <cstdint>
#include <memory>

/**
 * Platform abstraction interface for TeensyELS
 * Centralizes all platform-specific functionality to eliminate scattered #ifdef blocks
 */

// Forward declarations for platform-specific types
class LeadscrewIO;
class ISpindle;
class ILeadscrew;
class IDisplay;
class IButtonHandler;

#ifdef ESP32
class IKeyArray;
class ICommsManager;
#endif

/**
 * Hardware pin configuration abstraction
 */
struct HardwarePins {
    // Spindle encoder pins
    int spindleEncoderA;
    int spindleEncoderB;
    
    // Leadscrew stepper pins
    int leadscrewStep;
    int leadscrewDir;
    int stepperEnable;
    
    // UI elements
    int uiEncoderA;
    int uiEncoderB;
    int indicatorRed;
    int indicatorGreen;
    
    // Buttons (Teensy individual pins)
    int rateIncreaseButton;
    int rateDecreaseButton;
    int modeCycleButton;
    int threadSyncButton;
    int halfNutButton;
    int enableButton;
    int lockButton;
    int jogLeftButton;
    int jogRightButton;
    
#ifdef ESP32
    // ESP32 button matrix pins
    int padH1, padH2, padH3;
    int padV1, padV2, padV3;
#endif
};

/**
 * Display configuration abstraction
 */
enum class DisplayType {
    SSD1306_128x64,  // Teensy OLED
    ST7789_240x135   // ESP32 TFT
};

struct DisplayConfig {
    DisplayType type;
    int resetPin;  // -1 if not used
};

/**
 * Platform capabilities abstraction
 */
struct PlatformCapabilities {
    bool hasWiFi;
    bool hasOTA;
    bool hasDualCore;
    bool hasButtonMatrix;
    bool hasRMT;  // ESP32 Remote Control Transceiver
    bool hasHardwareTimers;
    const char* platformName;
};

/**
 * Main platform abstraction interface
 */
class IPlatformAbstraction {
public:
    virtual ~IPlatformAbstraction() = default;
    
    // Hardware configuration
    virtual const HardwarePins& getHardwarePins() const = 0;
    virtual const DisplayConfig& getDisplayConfig() const = 0;
    virtual const PlatformCapabilities& getCapabilities() const = 0;
    
    // Hardware initialization
    virtual void initializeHardware() = 0;
    virtual void initializeTimer(void (*callback)(), uint32_t intervalMicros) = 0;
    virtual void initializeDisplay() = 0;
    
    // Platform-specific component creation
    virtual std::unique_ptr<LeadscrewIO> createLeadscrewIO() = 0;
    virtual std::unique_ptr<ISpindle> createSpindle() = 0;
    
#ifdef ESP32
    // ESP32-specific components
    virtual std::unique_ptr<IKeyArray> createKeyArray(ILeadscrew* leadscrew) = 0;
    virtual std::unique_ptr<ICommsManager> createCommsManager() = 0;
    virtual void initializeTaskScheduler() = 0;
#endif
    
    // Hardware abstraction methods
    virtual void digitalWrite(int pin, bool value) = 0;
    virtual bool digitalRead(int pin) = 0;
    virtual void pinMode(int pin, int mode) = 0;
    virtual uint64_t getMicros() = 0;
    virtual void delayMicros(uint32_t micros) = 0;
    
    // Platform-specific optimizations
    virtual void enableRealTimeMode() = 0;
    virtual void disableWatchdog() = 0;
    virtual void yieldProcessor() = 0;
};

/**
 * Factory for creating platform abstraction instances
 */
class PlatformAbstractionFactory {
public:
    static std::unique_ptr<IPlatformAbstraction> create();
};