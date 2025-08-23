#pragma once

#include <memory>
#include <unordered_map>

#ifndef PIO_UNIT_TESTING
// For embedded systems without RTTI/exceptions, use hash-based type system
#include <functional>

struct TypeKey {
    size_t hash;
    
    TypeKey(size_t h) : hash(h) {}
    
    bool operator==(const TypeKey& other) const {
        return hash == other.hash;
    }
};

// Hash function for TypeKey
namespace std {
    template<>
    struct hash<TypeKey> {
        size_t operator()(const TypeKey& key) const {
            return key.hash;
        }
    };
}

// Simple compile-time string hash (FNV-1a)
constexpr size_t hash_string(const char* str) {
    return *str ? (hash_string(str + 1) ^ static_cast<size_t>(*str)) * 16777619u : 2166136261u;
}

template<typename T>
constexpr TypeKey getTypeKey() {
    return TypeKey(hash_string(__PRETTY_FUNCTION__));
}
#else
// For unit testing with full C++ support
#include <typeindex>
#include <stdexcept>
typedef std::type_index TypeKey;

template<typename T>
TypeKey getTypeKey() {
    return std::type_index(typeid(T));
}
#endif

/**
 * Simple dependency injection container for managing object lifecycles
 * and resolving dependencies throughout the application
 * 
 * Uses compile-time type identification to work without RTTI on embedded systems
 */
class DependencyContainer {
protected:
    std::unordered_map<TypeKey, std::shared_ptr<void>> m_instances;
    
public:
    /**
     * Register a singleton instance in the container
     * @param instance The instance to register
     */
    template<typename T>
    void registerSingleton(std::shared_ptr<T> instance) {
        m_instances[getTypeKey<T>()] = instance;
    }
    
    /**
     * Register a singleton instance created from a unique_ptr
     * @param instance The instance to register
     */
    template<typename T>
    void registerInstance(std::unique_ptr<T> instance) {
        registerSingleton(std::shared_ptr<T>(instance.release()));
    }
    
    /**
     * Resolve a dependency by type
     * @return Pointer to the registered instance, nullptr if not found
     */
    template<typename T>
    T* resolve() {
        auto it = m_instances.find(getTypeKey<T>());
        if (it == m_instances.end()) {
#ifdef PIO_UNIT_TESTING
            throw std::runtime_error("Type not registered in container");
#else
            return nullptr; // Return nullptr instead of throwing on embedded systems
#endif
        }
        return static_cast<T*>(it->second.get());
    }
    
    /**
     * Check if a type is registered
     * @return true if type is registered
     */
    template<typename T>
    bool isRegistered() const {
        return m_instances.find(getTypeKey<T>()) != m_instances.end();
    }
    
    /**
     * Clear all registered instances
     */
    virtual void clear() {
        m_instances.clear();
    }
    
    /**
     * Virtual destructor for proper inheritance
     */
    virtual ~DependencyContainer() = default;
};