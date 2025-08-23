#include "system_factory.h"
#include <config.h>
#include <globalstate.h>
#include "../platform/platform_abstraction.h"
#include "../commands/command_button_handler.h"
#include <leadscrew.h>
#include <display.h>
#include <memory>

std::unique_ptr<EnhancedDependencyContainer> SystemFactory::createSystem() {
    auto container = std::make_unique<EnhancedDependencyContainer>();
    
    // Register container for self-injection
    container->registerInstance<EnhancedDependencyContainer>(container.get());
    container->registerInstance<DependencyContainer>(container.get());
    
    // Create and register platform abstraction first
    auto platform = PlatformAbstractionFactory::create();
    platform->initializeHardware();
    container->registerSingleton(std::shared_ptr<IPlatformAbstraction>(platform.release()));
    
    // Register components using factory pattern for proper DI
    container->registerFactory<ISpindle>([](EnhancedDependencyContainer* di) {
        auto platform = di->resolve<IPlatformAbstraction>();
        return platform->createSpindle();
    });
    
    container->registerFactory<LeadscrewIO>([](EnhancedDependencyContainer* di) {
        auto platform = di->resolve<IPlatformAbstraction>();
        return platform->createLeadscrewIO();
    });
    
    container->registerFactory<ILeadscrew>([](EnhancedDependencyContainer* di) {
        return std::make_unique<Leadscrew>(di);
    });
    
    container->registerFactory<IDisplay>([](EnhancedDependencyContainer* di) {
        return std::make_unique<Display>(di);
    });
    
    container->registerFactory<IButtonHandler>([](EnhancedDependencyContainer* di) {
#ifdef ESP32
        auto platform = di->resolve<IPlatformAbstraction>();
        if (platform->getCapabilities().hasButtonMatrix) {
            return std::make_unique<ESP32CommandButtonHandler>(di);
        }
#endif
        return std::make_unique<CommandButtonHandler>(di);
    });
    
#ifdef ESP32
    container->registerFactory<IKeyArray>([](EnhancedDependencyContainer* di) {
        auto platform = di->resolve<IPlatformAbstraction>();
        auto leadscrew = di->resolve<ILeadscrew>();
        return platform->createKeyArray(leadscrew);
    });
    
    container->registerFactory<ICommsManager>([](EnhancedDependencyContainer* di) {
        auto platform = di->resolve<IPlatformAbstraction>();
        return platform->createCommsManager();
    });
#endif
    
    return container;
}

std::unique_ptr<DependencyContainer> SystemFactory::createSystemLegacy() {
    auto container = std::make_unique<DependencyContainer>();
    
    // Legacy implementation with manual dependency wiring
    auto platform = PlatformAbstractionFactory::create();
    platform->initializeHardware();
    
    auto spindle = platform->createSpindle();
    auto leadscrewIO = platform->createLeadscrewIO();
    auto leadscrew = createLeadscrew(spindle.get(), leadscrewIO.get());
    auto display = createDisplay(spindle.get(), leadscrew.get());
    
    auto spindlePtr = spindle.get();
    auto leadscrewPtr = leadscrew.get();
    
    std::unique_ptr<IButtonHandler> buttonHandler;
    
#ifdef ESP32
    if (platform->getCapabilities().hasButtonMatrix) {
        auto keyArray = platform->createKeyArray(leadscrewPtr);
        auto keyArrayPtr = keyArray.get();
        buttonHandler = createButtonHandler(spindlePtr, leadscrewPtr, keyArrayPtr);
        container->registerSingleton<IKeyArray>(std::move(keyArray));
        container->registerSingleton<ICommsManager>(platform->createCommsManager());
    } else {
        buttonHandler = createButtonHandler(spindlePtr, leadscrewPtr);
    }
#else
    buttonHandler = createButtonHandler(spindlePtr, leadscrewPtr);
#endif
    
    container->registerSingleton(std::shared_ptr<IPlatformAbstraction>(platform.release()));
    container->registerSingleton(std::shared_ptr<ISpindle>(spindle.release()));
    container->registerSingleton(std::shared_ptr<LeadscrewIO>(leadscrewIO.release()));
    container->registerSingleton(std::shared_ptr<ILeadscrew>(leadscrew.release()));
    container->registerSingleton(std::shared_ptr<IDisplay>(display.release()));
    container->registerSingleton(std::shared_ptr<IButtonHandler>(buttonHandler.release()));
    
    return container;
}

// Platform-specific factory methods are implemented in:
// - teensy_system_factory.cpp for Teensy platform
// - esp32_system_factory.cpp for ESP32 platform