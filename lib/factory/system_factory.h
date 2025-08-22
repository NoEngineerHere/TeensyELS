#pragma once

#include <memory>
#include "../di/dependency_container.h"
#include "../interfaces/system_interfaces.h"
#include "leadscrew_io.h"

/**
 * Factory for creating and configuring the entire system
 * Handles platform-specific object creation and dependency wiring
 */
class SystemFactory {
public:
    /**
     * Create a fully configured system with all dependencies wired
     * @return Container with all system components registered
     */
    static std::unique_ptr<DependencyContainer> createSystem();
    
private:
    /**
     * Create platform-specific spindle implementation
     */
    static std::unique_ptr<ISpindle> createSpindle();
    
    /**
     * Create platform-specific leadscrew IO implementation
     */
    static std::unique_ptr<LeadscrewIO> createLeadscrewIO();
    
    /**
     * Create leadscrew with proper dependencies
     */
    static std::unique_ptr<ILeadscrew> createLeadscrew(ISpindle* spindle, LeadscrewIO* io);
    
    /**
     * Create display with proper dependencies
     */
    static std::unique_ptr<IDisplay> createDisplay(ISpindle* spindle, ILeadscrew* leadscrew);
    
    /**
     * Create platform-specific button handler
     */
    static std::unique_ptr<IButtonHandler> createButtonHandler(ISpindle* spindle, ILeadscrew* leadscrew);
    
    /**
     * Create command-based button handler (recommended for new implementations)
     */
    static std::unique_ptr<IButtonHandler> createCommandButtonHandler(ISpindle* spindle, ILeadscrew* leadscrew);
    
#ifdef ESP32
    /**
     * Create ESP32 button handler with KeyArray dependency
     */
    static std::unique_ptr<IButtonHandler> createButtonHandler(ISpindle* spindle, ILeadscrew* leadscrew, IKeyArray* keyArray);
    
    /**
     * Create ESP32 command-based button handler with KeyArray dependency
     */
    static std::unique_ptr<IButtonHandler> createCommandButtonHandler(ISpindle* spindle, ILeadscrew* leadscrew, IKeyArray* keyArray);
#endif
    
#ifdef ESP32
    /**
     * Create ESP32-specific key array
     */
    static std::unique_ptr<IKeyArray> createKeyArray(ILeadscrew* leadscrew);
    
    /**
     * Create ESP32 communications manager
     */
    static std::unique_ptr<ICommsManager> createCommsManager();
#endif
};