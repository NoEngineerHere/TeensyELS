#include "system_factory.h"
#include <config.h>
#include <globalstate.h>

// Platform-specific implementations are in separate files

std::unique_ptr<DependencyContainer> SystemFactory::createSystem() {
    auto container = std::make_unique<DependencyContainer>();
    
    // Create components in dependency order
    auto spindle = createSpindle();
    auto leadscrewIO = createLeadscrewIO();
    auto leadscrew = createLeadscrew(spindle.get(), leadscrewIO.get());
    auto display = createDisplay(spindle.get(), leadscrew.get());
    
    // Store raw pointers for registration
    auto spindlePtr = spindle.get();
    auto leadscrewPtr = leadscrew.get();
    
#ifdef ESP32
    // For ESP32, create KeyArray first, then ButtonPad
    auto keyArray = createKeyArray(leadscrewPtr);
    auto keyArrayPtr = keyArray.get();
    auto buttonHandler = createButtonHandler(spindlePtr, leadscrewPtr, keyArrayPtr);
    auto commsManager = createCommsManager();
    
    container->registerSingleton<IKeyArray>(std::move(keyArray));
    container->registerSingleton<ICommsManager>(std::move(commsManager));
#else
    auto buttonHandler = createButtonHandler(spindlePtr, leadscrewPtr);
#endif
    
    // Register interfaces with container
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