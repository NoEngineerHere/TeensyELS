# TeensyELS - Electronic Lead Screw System

An advanced Electronic Lead Screw (ELS) implementation for lathes supporting both **Teensy 4.1** and **ESP32** microcontrollers. This system provides automated threading and feeding operations by synchronizing a stepper motor-driven leadscrew with spindle rotation, eliminating the need for traditional change gears.

## Features

- **Multi-Platform Support**: Teensy 4.1 (primary) and ESP32 (with WiFi/OTA)
- **Real-Time Control**: Sub-millisecond response times for precision machining  
- **Comprehensive Safety**: Stop positions, motion state management, bounds checking
- **Threading & Feeding**: Configurable pitch tables for metric/imperial systems
- **Visual Feedback**: OLED (Teensy) and TFT (ESP32) displays with real-time status
- **Extensive Testing**: Unit test suite with hardware abstraction mocks

## Architecture

Built on **PlatformIO** with a clean hardware abstraction layer enabling:
- Platform-specific optimizations (fast GPIO, hardware timers)
- Comprehensive unit testing on native platform  
- Real-time performance guarantees for machining precision

## Build Commands

```bash
# Build for specific platform
pio run --environment teensy41        # Production Teensy build
pio run --environment esp32dev        # ESP32 development build

# Upload firmware  
pio run --target upload --environment teensy41

# Run comprehensive test suite
pio test --environment native
```

## Installation

1. **Setup PlatformIO**: Install [PlatformIO](https://platformio.org/) with VSCode extension
2. **Clone Repository**: `git clone <repository-url>`
3. **Configure Hardware**: Edit `lib/config/config.h` for your specific lathe setup:
   - Encoder PPR (pulses per revolution)
   - Stepper motor specifications  
   - Leadscrew pitch and gear ratios
   - Pin assignments for your platform
4. **Build & Upload**: Use PlatformIO commands above

⚠️ **Important**: You must modify the configuration in `lib/config/config.h` to match your lathe's specifications before use.

## Testing

The project includes a comprehensive unit test suite that runs locally without hardware:

```bash
# Run all tests
pio test --environment native

# Verbose test output
pio test --environment native -v
```

**Test Coverage:**
- Global state management and configuration
- Axis position tracking and synchronization
- Leadscrew motion control and safety features
- Spindle encoder handling and position wrapping
- Hardware abstraction layer mocks
- Edge cases and error conditions

## Debugging

### Unit Test Debugging
The recommended debugging approach for this real-time system:

1. **Setup VSCode Debugging**:
   - Use VSCode with PlatformIO extension
   - Set environment to "native" (default for testing)
   - Ensure `gdb` is installed on your system

2. **Debug Specific Tests**:
   - To debug a single test: Set `test_debug` in `platformio.ini`
   - Set breakpoints in your test code
   - Use "PIO Debug" target in VSCode debug panel

3. **Hardware Debugging**:
   - Teensy boards don't support hardware debuggers directly
   - Use serial output and LED indicators for runtime debugging
   - Leverage the comprehensive test suite to validate logic before hardware deployment

### Key Debugging Tips
- **Time Simulation**: Tests use `MicrosSingleton` for deterministic timing
- **Mock Objects**: Hardware dependencies are abstracted for isolated testing
- **State Inspection**: GlobalState singleton provides centralized state visibility
- **Safety Verification**: Stop positions and motion modes prevent dangerous states

## Configuration

Key configuration parameters in `lib/config/config.h`:

```cpp
// Spindle encoder configuration
#define ELS_SPINDLE_ENCODER_PPR 1000    // Pulses per revolution

// Leadscrew motor configuration  
#define ELS_LEADSCREW_STEPPER_PPR 200   // Steps per revolution
#define ELS_LEADSCREW_PITCH_MM 2.0      // Leadscrew pitch in mm
#define ELS_GEARBOX_RATIO 4.0           // Gear reduction ratio

// Timing configuration
#define LEADSCREW_TIMER_US 4            // Update interval in microseconds
```

## Safety Features

- **Stop Positions**: Configurable left/right travel limits
- **Motion Modes**: DISABLED/ENABLED/JOG state machine
- **Bounds Checking**: Compile-time array bounds verification
- **Direction Control**: Automatic direction detection and control
- **Emergency Stops**: Button lock states and motion disable

## Contributing

- **Code Style**: Follow existing patterns and hardware abstraction
- **Testing**: Add unit tests for new functionality
- **Platform Support**: Test on both Teensy and ESP32 platforms
- **Documentation**: Update CLAUDE.md for architectural changes

For detailed architecture information, see [CLAUDE.md](CLAUDE.md).
