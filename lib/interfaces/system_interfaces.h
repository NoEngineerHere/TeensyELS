#pragma once

#include <cstdint>

// Forward declare platform abstraction
class IPlatformAbstraction;

/**
 * Core system interfaces for dependency injection
 * These interfaces define the contracts for the main components
 * allowing for better testability and loose coupling
 */

class ISpindle {
public:
    virtual void update() = 0;
    virtual void setCurrentPosition(int position) = 0;
    virtual void incrementCurrentPosition(int amount) = 0;
    virtual int getCurrentPosition() = 0;
    virtual int consumePosition() = 0;
    virtual float getEstimatedVelocityInRPM() = 0;
    virtual float getEstimatedVelocityInPPS() = 0;
    virtual uint32_t getEstimatedVelocityInPulsesPerSecond() = 0;
    virtual ~ISpindle() = default;
};

class ILeadscrew {
public:
    virtual void update() = 0;
    virtual void setTargetPitchMM(float ratio) = 0;
    virtual void setCurrentPosition(int position) = 0;
    virtual int getCurrentPosition() = 0;
    virtual int getPositionError() = 0;
    virtual float getEstimatedVelocityInMillimetersPerSecond() = 0;
    virtual ~ILeadscrew() = default;
};

class IDisplay {
public:
    virtual void init() = 0;
    virtual void update() = 0;
    virtual ~IDisplay() = default;
};

class IButtonHandler {
public:
    virtual void handle() = 0;
    virtual ~IButtonHandler() = default;
};

#ifdef ESP32
class IKeyArray {
public:
    virtual void initPad() = 0;
    virtual ~IKeyArray() = default;
};

class ICommsManager {
public:
    virtual void loop() = 0;
    virtual ~ICommsManager() = default;
};
#endif