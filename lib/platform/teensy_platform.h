#pragma once

#include "platform_abstraction.h"
#include <memory>

#ifndef ESP32

/**
 * Teensy platform implementation
 * Consolidates all Teensy-specific functionality
 */
class TeensyPlatform : public IPlatformAbstraction {
private:
    HardwarePins m_pins;
    DisplayConfig m_displayConfig;
    PlatformCapabilities m_capabilities;
    
    void initializePinConfiguration();
    void initializeSerialDebug();
    void initializeRealTimeTimer();

public:
    TeensyPlatform();
    ~TeensyPlatform() override = default;
    
    // Hardware configuration
    const HardwarePins& getHardwarePins() const override;
    const DisplayConfig& getDisplayConfig() const override;
    const PlatformCapabilities& getCapabilities() const override;
    
    // Hardware initialization
    void initializeHardware() override;
    void initializeTimer(void (*callback)(), uint32_t intervalMicros) override;
    void initializeDisplay() override;
    
    // Component creation
    std::unique_ptr<LeadscrewIO> createLeadscrewIO() override;
    std::unique_ptr<ISpindle> createSpindle() override;
    
    // Hardware abstraction
    void digitalWrite(int pin, bool value) override;
    bool digitalRead(int pin) override;
    void pinMode(int pin, int mode) override;
    uint64_t getMicros() override;
    void delayMicros(uint32_t micros) override;
    
    // Platform-specific optimizations
    void enableRealTimeMode() override;
    void disableWatchdog() override;
    void yieldProcessor() override;
};

#endif // !ESP32