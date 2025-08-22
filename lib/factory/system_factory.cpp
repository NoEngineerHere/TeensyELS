#include "system_factory.h"
#include <config.h>
#include <globalstate.h>
#include "../platform/platform_abstraction.h"

std::unique_ptr<DependencyContainer> SystemFactory::createSystem() {
    auto container = std::make_unique<DependencyContainer>();
    
    // Create platform abstraction - this centralizes all platform-specific logic
    auto platform = PlatformAbstractionFactory::create();
    
    // Initialize platform hardware
    platform->initializeHardware();
    
    // Create components using platform abstraction
    auto spindle = platform->createSpindle();
    auto leadscrewIO = platform->createLeadscrewIO();
    auto leadscrew = createLeadscrew(spindle.get(), leadscrewIO.get());
    auto display = createDisplay(spindle.get(), leadscrew.get());
    
    // Store raw pointers for registration
    auto spindlePtr = spindle.get();
    auto leadscrewPtr = leadscrew.get();
    
    // Platform-specific component creation
    std::unique_ptr<IButtonHandler> buttonHandler;
    
#ifdef ESP32
    if (platform->getCapabilities().hasButtonMatrix) {
        // ESP32 with button matrix
        auto keyArray = platform->createKeyArray(leadscrewPtr);
        auto keyArrayPtr = keyArray.get();
        buttonHandler = createButtonHandler(spindlePtr, leadscrewPtr, keyArrayPtr);
        
        auto commsManager = platform->createCommsManager();
        container->registerSingleton<IKeyArray>(std::move(keyArray));
        container->registerSingleton<ICommsManager>(std::move(commsManager));
    } else {
        // ESP32 without button matrix (fallback)
        buttonHandler = createButtonHandler(spindlePtr, leadscrewPtr);
    }
#else
    // Teensy with individual buttons
    buttonHandler = createButtonHandler(spindlePtr, leadscrewPtr);
#endif
    
    // Register platform abstraction and all components with container
    container->registerSingleton<IPlatformAbstraction>(std::move(platform));
    container->registerSingleton<ISpindle>(std::move(spindle));
    container->registerSingleton<LeadscrewIO>(std::move(leadscrewIO));
    container->registerSingleton<ILeadscrew>(std::move(leadscrew));
    container->registerSingleton<IDisplay>(std::move(display));
    container->registerSingleton<IButtonHandler>(std::move(buttonHandler));
    
    return container;
}

// Platform-specific factory methods are implemented in:
// - teensy_system_factory.cpp for Teensy platform
// - esp32_system_factory.cpp for ESP32 platform