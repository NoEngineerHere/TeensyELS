#pragma once

#include "dependency_container.h"
#include <functional>

/**
 * Enhanced dependency injection container with factory support
 * 
 * This extends the basic DI container to support:
 * - Factory registration (lazy instantiation)
 * - Automatic dependency resolution
 * - Circular dependency detection
 */
class EnhancedDependencyContainer : public DependencyContainer {
private:
    // Factory function type that takes the container and returns a shared_ptr
    using FactoryFunction = std::function<std::shared_ptr<void>(EnhancedDependencyContainer*)>;
    
    std::unordered_map<TypeKey, FactoryFunction> m_factories;
    std::unordered_map<TypeKey, bool> m_resolving; // For circular dependency detection
    
public:
    /**
     * Register a factory function that will create instances on-demand
     * @param factory Function that creates and returns the instance
     */
    template<typename T>
    void registerFactory(std::function<std::unique_ptr<T>(EnhancedDependencyContainer*)> factory) {
        m_factories[getTypeKey<T>()] = [factory](EnhancedDependencyContainer* container) -> std::shared_ptr<void> {
            auto instance = factory(container);
            return std::shared_ptr<T>(instance.release());
        };
    }
    
    /**
     * Register a raw pointer instance (for self-injection of container)
     * Creates a non-owning shared_ptr
     */
    template<typename T>
    void registerInstance(T* instance) {
        if (!instance) {
#ifdef PIO_UNIT_TESTING
            throw std::runtime_error("Cannot register null instance");
#else
            return;
#endif
        }
        // Use aliasing constructor to create shared_ptr without ownership
        m_instances[getTypeKey<T>()] = std::shared_ptr<T>(std::shared_ptr<void>(), instance);
    }
    
    /**
     * Enhanced resolve that supports factory instantiation
     * @return Pointer to the instance, creating it via factory if needed
     */
    template<typename T>
    T* resolve() {
        TypeKey key = getTypeKey<T>();
        
        // Check for circular dependency
        if (m_resolving[key]) {
#ifdef PIO_UNIT_TESTING
            throw std::runtime_error("Circular dependency detected");
#else
            return nullptr;
#endif
        }
        
        // First check if instance already exists
        auto instanceIt = m_instances.find(key);
        if (instanceIt != m_instances.end()) {
            return static_cast<T*>(instanceIt->second.get());
        }
        
        // Try to create via factory
        auto factoryIt = m_factories.find(key);
        if (factoryIt != m_factories.end()) {
            // Mark as resolving to detect circular dependencies
            m_resolving[key] = true;
            
            auto instance = factoryIt->second(this);
            if (instance) {
                m_instances[key] = instance;
                m_resolving[key] = false;
                return static_cast<T*>(instance.get());
            } else {
                m_resolving[key] = false;
                return nullptr;
            }
        }
        
        // Fall back to base class behavior
        return DependencyContainer::resolve<T>();
    }
    
    /**
     * Check if a type can be resolved (either registered or has factory)
     */
    template<typename T>
    bool canResolve() const {
        TypeKey key = getTypeKey<T>();
        return m_instances.find(key) != m_instances.end() || 
               m_factories.find(key) != m_factories.end();
    }
    
    /**
     * Clear all registrations and factories
     */
    void clear() override {
        DependencyContainer::clear();
        m_factories.clear();
        m_resolving.clear();
    }
    
    /**
     * Get factory count for diagnostics
     */
    size_t getFactoryCount() const {
        return m_factories.size();
    }
    
    /**
     * Get instance count for diagnostics
     */
    size_t getInstanceCount() const {
        return m_instances.size();
    }
};

/**
 * Example of how the improved system would work:
 */

/*
// 1. Register factories instead of creating objects upfront
container->registerFactory<ISpindle>([](auto* di) {
    auto platform = di->resolve<IPlatformAbstraction>();
    return platform->createSpindle();
});

container->registerFactory<ILeadscrew>([](auto* di) {
    auto spindle = di->resolve<ISpindle>();      // DI resolves automatically
    auto leadscrewIO = di->resolve<LeadscrewIO>(); // DI resolves automatically
    return std::make_unique<Leadscrew>(spindle, leadscrewIO);
});

container->registerFactory<IDisplay>([](auto* di) {
    auto spindle = di->resolve<ISpindle>();      // DI resolves automatically  
    auto leadscrew = di->resolve<ILeadscrew>();  // DI resolves automatically
    return std::make_unique<Display>(spindle, leadscrew);
});

// 2. Resolve any dependency - the container figures out the creation order
auto display = container->resolve<IDisplay>();  // Creates spindle, leadscrew, then display

// 3. No more manual dependency passing!
*/