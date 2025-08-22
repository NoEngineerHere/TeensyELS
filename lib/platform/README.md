# Platform Abstraction Layer

This directory contains the platform abstraction layer for TeensyELS, which consolidates all platform-specific functionality to eliminate scattered `#ifdef` blocks throughout the codebase.

## Overview

The platform abstraction layer centralizes platform differences behind a common interface, making the main application logic platform-agnostic while maintaining optimal performance on each target.

## Architecture

```
lib/platform/
├── platform_abstraction.h     # Core interfaces and abstractions
├── teensy_platform.h          # Teensy platform implementation
├── teensy_platform.cpp        # Teensy-specific functionality
├── esp32_platform.h           # ESP32 platform implementation  
├── esp32_platform.cpp         # ESP32-specific functionality
├── platform_factory.cpp       # Platform factory for creation
└── README.md                   # This documentation
```

## Key Concepts

### Hardware Abstraction
- **Pin Configuration**: Centralized pin assignments per platform
- **Display Types**: Platform-specific display selection and configuration
- **Capabilities**: Runtime platform feature detection

### Platform Capabilities
Each platform exposes its capabilities through a common interface:

```cpp
struct PlatformCapabilities {
    bool hasWiFi;           // WiFi connectivity support
    bool hasOTA;            // Over-the-air update support  
    bool hasDualCore;       // Multi-core processing
    bool hasButtonMatrix;   // Button matrix vs individual buttons
    bool hasRMT;            // ESP32 Remote Control Transceiver
    bool hasHardwareTimers; // Hardware timer support
    const char* platformName; // Human-readable platform name
};
```

## Usage Pattern

### System Creation
```cpp
// Create platform abstraction
auto platform = PlatformAbstractionFactory::create();

// Initialize platform-specific hardware
platform->initializeHardware();

// Query platform capabilities
if (platform->getCapabilities().hasDualCore) {
    platform->initializeTaskScheduler();
} else {
    platform->initializeTimer(callback, intervalMicros);
}

// Create platform-specific components
auto spindle = platform->createSpindle();
auto leadscrewIO = platform->createLeadscrewIO();
```

### Hardware Access
```cpp
// Platform-abstracted hardware operations
auto pins = platform->getHardwarePins();
platform->digitalWrite(pins.stepperEnable, true);
bool buttonPressed = platform->digitalRead(pins.enableButton);
uint64_t timestamp = platform->getMicros();
```

### Platform Detection
```cpp
// Runtime platform feature detection
if (platform->getCapabilities().hasButtonMatrix) {
    // ESP32 with button matrix
    auto keyArray = platform->createKeyArray(leadscrew);
} else {
    // Teensy with individual buttons
    auto buttonHandler = platform->createButtonHandler(spindle, leadscrew);
}
```

## Platform Implementations

### Teensy Platform
- **Target**: Teensy 4.1 microcontroller
- **Display**: SSD1306 128x64 OLED over I2C
- **Inputs**: Individual button pins with pull-ups
- **Timing**: IntervalTimer for precise 4μs callbacks
- **Architecture**: Single-core with hardware timer interrupt

### ESP32 Platform  
- **Target**: LilyGO T-Display (ESP32-based)
- **Display**: ST7789 240x135 TFT over SPI
- **Inputs**: 3x3 button matrix with interrupt handling
- **Timing**: FreeRTOS tasks on dual cores
- **Features**: WiFi, OTA updates, RMT for step generation

## Configuration Consolidation

### Pin Assignments
Platform-specific pin assignments are centralized:

```cpp
// Teensy pins
struct HardwarePins teensyPins = {
    .spindleEncoderA = 14,
    .spindleEncoderB = 15,
    .leadscrewStep = 2,
    .leadscrewDir = 3,
    .enableButton = 9,
    // ... individual button pins
};

// ESP32 pins  
struct HardwarePins esp32Pins = {
    .spindleEncoderA = 37,
    .spindleEncoderB = 36, 
    .leadscrewStep = 25,
    .leadscrewDir = 26,
    .padH1 = 32, .padH2 = 33, .padH3 = 2,  // Button matrix
    .padV1 = 15, .padV2 = 13, .padV3 = 12,
};
```

### Display Configuration
```cpp
// Platform-specific display settings
DisplayConfig teensyDisplay = {
    .type = DisplayType::SSD1306_128x64,
    .resetPin = -1
};

DisplayConfig esp32Display = {
    .type = DisplayType::ST7789_240x135, 
    .resetPin = -1  // Handled by TFT_eSPI library
};
```

## Benefits Achieved

### Code Organization
- **Single Source of Truth**: All platform differences in one location
- **Eliminated Scattered #ifdefs**: Reduced conditional compilation from 13+ files to 4 platform files
- **Clear Separation**: Hardware concerns separated from business logic

### Maintainability  
- **Easier Platform Addition**: Add new platform by implementing interface
- **Reduced Complexity**: Main application logic is platform-agnostic
- **Better Testing**: Platform abstraction can be mocked for unit tests

### Performance
- **Zero Runtime Overhead**: Platform capabilities determined at compile time
- **Optimal Code Generation**: Platform-specific optimizations preserved
- **Real-Time Compliance**: Critical timing paths maintain deterministic behavior

## Integration with Dependency Injection

The platform abstraction integrates seamlessly with the DI system:

```cpp
std::unique_ptr<DependencyContainer> SystemFactory::createSystem() {
    auto container = std::make_unique<DependencyContainer>();
    
    // Create and register platform abstraction
    auto platform = PlatformAbstractionFactory::create();
    platform->initializeHardware();
    container->registerSingleton<IPlatformAbstraction>(std::move(platform));
    
    // Use platform to create components
    auto spindle = platform->createSpindle();
    auto leadscrewIO = platform->createLeadscrewIO();
    
    return container;
}
```

## Testing Strategy

### Mock Platform for Testing
```cpp
class MockPlatform : public IPlatformAbstraction {
public:
    MOCK_METHOD(void, digitalWrite, (int pin, bool value), (override));
    MOCK_METHOD(bool, digitalRead, (int pin), (override));
    MOCK_METHOD(uint64_t, getMicros, (), (override));
    // ... other mocks
};

TEST(PlatformTest, DigitalIO) {
    auto mockPlatform = std::make_unique<MockPlatform>();
    EXPECT_CALL(*mockPlatform, digitalWrite(25, true)).Times(1);
    
    // Test platform-dependent functionality
}
```

### Platform-Specific Integration Tests
Each platform implementation should be tested on actual hardware to verify:
- Pin assignments are correct
- Timing characteristics meet real-time requirements  
- Hardware initialization succeeds
- Component creation produces working objects

## Future Enhancements

### Additional Platforms
The abstraction makes it easy to add new platforms:
- Arduino Mega/Uno support
- STM32 platform support
- Raspberry Pi Pico support

### Enhanced Capabilities
- Power management abstraction
- Wireless communication abstraction
- File system abstraction
- Sensor interface abstraction

## Migration Guide

### Before (Scattered Platform Code)
```cpp
// In main.cpp
#ifdef ESP32
    pinMode(ELS_LEADSCREW_STEP, OUTPUT);
    TaskHandle_t spindleTask;
    xTaskCreatePinnedToCore(SpindleTask, "Spindle", 2048, NULL, 24, &spindleTask, 0);
#else
    pinMode(ELS_LEADSCREW_STEP, OUTPUT);
    timer.begin(timerCallback, LEADSCREW_TIMER_US);
#endif

// In multiple other files
#ifdef ESP32
    // ESP32-specific code
#else  
    // Teensy-specific code
#endif
```

### After (Platform Abstraction)
```cpp  
// In main.cpp - clean and platform-agnostic
auto platform = systemContainer->resolve<IPlatformAbstraction>();

if (platform->getCapabilities().hasDualCore) {
    platform->initializeTaskScheduler();
    // Create dual-core tasks...
} else {
    platform->initializeTimer(timerCallback, LEADSCREW_TIMER_US);
}

// No platform-specific code in other files
```

This platform abstraction layer successfully consolidates scattered platform-specific code into a clean, maintainable architecture while preserving the real-time performance characteristics essential for precision machining applications.