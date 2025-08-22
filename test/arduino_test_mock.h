#pragma once

// Mock Arduino.h functionality for unit testing
#ifdef PIO_UNIT_TESTING

#include <cstdint>
#include <cstdio>

// Time abstraction singletons
class MicrosSingleton {
private:
    static MicrosSingleton* instance;
    uint64_t current_micros;
    
public:
    static MicrosSingleton& getInstance() {
        if (!instance) {
            instance = new MicrosSingleton();
        }
        return *instance;
    }
    
    uint64_t micros() const { return current_micros; }
    void setMicros(uint64_t micros) { current_micros = micros; }
    void incrementMicros(uint64_t delta) { current_micros += delta; }
    
private:
    MicrosSingleton() : current_micros(0) {}
};

class MillisSingleton {
private:
    static MillisSingleton* instance;
    uint64_t current_millis;
    
public:
    static MillisSingleton& getInstance() {
        if (!instance) {
            instance = new MillisSingleton();
        }
        return *instance;
    }
    
    uint64_t millis() const { return current_millis; }
    void setMillis(uint64_t millis) { current_millis = millis; }
    void incrementMillis(uint64_t delta) { current_millis += delta; }
    
private:
    MillisSingleton() : current_millis(0) {}
};

// Global Arduino-like functions
inline uint64_t micros() {
    return MicrosSingleton::getInstance().micros();
}

inline uint64_t millis() {
    return MillisSingleton::getInstance().millis();
}

// Debug output
#define DEBUG_F(fmt, ...) printf(fmt, ##__VA_ARGS__)

// Common Arduino types/constants
typedef uint8_t byte;
#define HIGH 1
#define LOW 0

// Delay functions
inline void delay(unsigned long ms) {
    // No-op in tests
}

inline void delayMicroseconds(unsigned int us) {
    // No-op in tests
}

#endif // PIO_UNIT_TESTING