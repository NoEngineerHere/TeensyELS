#pragma once

#include "platform_abstraction.h"
#include <memory>

#ifdef ESP32

/**
 * ESP32 platform implementation
 * Consolidates all ESP32-specific functionality including WiFi, OTA, dual-core, and RMT
 */
class ESP32Platform : public IPlatformAbstraction {
private:
    HardwarePins m_pins;
    DisplayConfig m_displayConfig;
    PlatformCapabilities m_capabilities;
    
    void initializePinConfiguration();
    void initializeSerialDebug();
    void initializeWiFi();
    void initializeDualCoreScheduler();

public:
    ESP32Platform();
    ~ESP32Platform() override = default;
    
    // Hardware configuration
    const HardwarePins& getHardwarePins() const override;
    const DisplayConfig& getDisplayConfig() const override;
    const PlatformCapabilities& getCapabilities() const override;
    
    // Hardware initialization
    void initializeHardware() override;
    void initializeTimer(void (*callback)(), uint32_t intervalMicros) override;
    void initializeDisplay() override;
    void initializeTaskScheduler() override;
    
    // Component creation
    std::unique_ptr<LeadscrewIO> createLeadscrewIO() override;
    std::unique_ptr<ISpindle> createSpindle() override;
    
    // ESP32-specific components
    std::unique_ptr<IKeyArray> createKeyArray(ILeadscrew* leadscrew) override;
    std::unique_ptr<ICommsManager> createCommsManager() override;
    
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
    
    // ESP32-specific methods
    void initializeRMT();
    void setupWatchdogTasks();
};

#endif // ESP32