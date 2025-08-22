# Dependency Injection System

This directory contains the dependency injection (DI) system for TeensyELS, designed specifically for embedded systems without RTTI or exception support.

## Overview

The DI system provides loose coupling between components while maintaining real-time performance characteristics. It uses compile-time type identification and avoids dynamic allocations during runtime.

## Architecture

```
lib/di/
├── dependency_container.h      # Core DI container implementation
└── README.md                   # This documentation

lib/interfaces/
└── system_interfaces.h         # Component interfaces

lib/factory/
├── system_factory.h            # Factory interface
├── system_factory.cpp          # Common factory logic
├── teensy_system_factory.cpp   # Teensy-specific implementations
└── esp32_system_factory.cpp    # ESP32-specific implementations
```

## Core Components

### DependencyContainer

The `DependencyContainer` class manages object lifecycles and resolves dependencies:

```cpp
auto container = std::make_unique<DependencyContainer>();

// Register components
container->registerSingleton<ISpindle>(std::move(spindle));
container->registerSingleton<ILeadscrew>(std::move(leadscrew));

// Resolve dependencies
auto spindle = container->resolve<ISpindle>();
auto leadscrew = container->resolve<ILeadscrew>();
```

### Type System

The system uses different type identification strategies based on the target platform:

#### Embedded Systems (No RTTI)
```cpp
struct TypeKey {
    size_t hash;
    // Compile-time hash of __PRETTY_FUNCTION__
};

template<typename T>
constexpr TypeKey getTypeKey() {
    return TypeKey(hash_string(__PRETTY_FUNCTION__));
}
```

#### Unit Testing (Full C++)
```cpp
typedef std::type_index TypeKey;

template<typename T>
TypeKey getTypeKey() {
    return std::type_index(typeid(T));
}
```

### Error Handling

Error handling is platform-dependent:

- **Embedded**: Returns `nullptr` on failure (no exceptions)
- **Testing**: Throws `std::runtime_error` for better debugging

## Usage Patterns

### Basic Registration and Resolution

```cpp
// Create container
auto container = std::make_unique<DependencyContainer>();

// Register singleton
auto service = std::make_unique<ConcreteService>();
container->registerSingleton<IService>(std::move(service));

// Resolve dependency
auto resolvedService = container->resolve<IService>();
if (resolvedService) {
    resolvedService->doWork();
}
```

### Factory Pattern Integration

The `SystemFactory` creates entire object graphs:

```cpp
// Create complete system
auto systemContainer = SystemFactory::createSystem();

// All components are wired and ready
auto spindle = systemContainer->resolve<ISpindle>();
auto leadscrew = systemContainer->resolve<ILeadscrew>();
```

### Testing with Mock Objects

```cpp
class MockSpindle : public ISpindle {
public:
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(int, getCurrentPosition, (), (override));
    // ... other mocks
};

TEST(MyTest, SpindleInteraction) {
    auto container = std::make_unique<DependencyContainer>();
    
    auto mockSpindle = std::make_unique<MockSpindle>();
    EXPECT_CALL(*mockSpindle, update()).Times(1);
    
    container->registerSingleton<ISpindle>(std::move(mockSpindle));
    
    auto spindle = container->resolve<ISpindle>();
    spindle->update(); // Mock expectation verified
}
```

## Design Principles

### 1. Zero Runtime Overhead
- Type resolution uses compile-time hashing
- No virtual function calls for type identification
- Minimal memory footprint

### 2. Exception Safety
- No exceptions thrown on embedded platforms
- Graceful degradation with nullptr returns
- Safety checks in calling code

### 3. Real-Time Compatibility
- All registrations happen at startup
- No dynamic allocations during operation
- Deterministic resolution times

### 4. Platform Abstraction
- Conditional compilation only in DI layer
- Same API across all platforms
- Testing code identical to production code

## Implementation Details

### String Hashing Algorithm

Uses FNV-1a hash for compile-time type identification:

```cpp
constexpr size_t hash_string(const char* str) {
    size_t hash = 2166136261u;
    while (*str) {
        hash ^= static_cast<size_t>(*str++);
        hash *= 16777619u;
    }
    return hash;
}
```

### Memory Management

All objects are managed via `std::shared_ptr` for safe lifetime management:

```cpp
std::unordered_map<TypeKey, std::shared_ptr<void>> m_instances;
```

### Platform Detection

Compilation strategy determined at build time:

```cpp
#ifndef PIO_UNIT_TESTING
    // Embedded implementation
#else
    // Full C++ implementation for testing
#endif
```

## Integration Guide

### Adding New Components

1. **Define Interface** (in `lib/interfaces/system_interfaces.h`):
```cpp
class INewComponent {
public:
    virtual void doSomething() = 0;
    virtual ~INewComponent() = default;
};
```

2. **Implement Interface** in existing class:
```cpp
class NewComponent : public ExistingBase, public INewComponent {
public:
    void doSomething() override { /* implementation */ }
};
```

3. **Add to Factory**:
```cpp
// In SystemFactory
static std::unique_ptr<INewComponent> createNewComponent();

// In platform-specific factory
std::unique_ptr<INewComponent> SystemFactory::createNewComponent() {
    return std::make_unique<NewComponent>(/* dependencies */);
}
```

4. **Register in Container**:
```cpp
auto newComponent = createNewComponent();
container->registerSingleton<INewComponent>(std::move(newComponent));
```

### Testing New Components

```cpp
class MockNewComponent : public INewComponent {
public:
    MOCK_METHOD(void, doSomething, (), (override));
};

TEST(ComponentTest, DoesWork) {
    auto container = std::make_unique<DependencyContainer>();
    
    auto mock = std::make_unique<MockNewComponent>();
    EXPECT_CALL(*mock, doSomething()).Times(1);
    
    container->registerSingleton<INewComponent>(std::move(mock));
    
    // Test code using container->resolve<INewComponent>()
}
```

## Best Practices

### 1. Interface Design
- Keep interfaces focused and cohesive
- Avoid large interfaces (Interface Segregation Principle)
- Use pure virtual destructors

### 2. Factory Organization
- Group related creation logic in same factory method
- Use platform-specific factory files for conditional code
- Keep creation logic separate from business logic

### 3. Container Usage
- Register all singletons at startup
- Resolve dependencies at startup when possible
- Check for nullptr in embedded code

### 4. Testing Strategy
- Use mock objects for all dependencies
- Test components in isolation
- Verify container behavior separately from business logic

## Performance Characteristics

| Operation | Time Complexity | Memory Usage |
|-----------|-----------------|--------------|
| Registration | O(1) | O(1) per object |
| Resolution | O(1) | O(0) |
| Type Hashing | O(1) compile-time | O(0) |

## Limitations

### Current Limitations
1. **Singleton Only**: Only singleton lifetime supported
2. **No Lazy Loading**: All objects created at startup
3. **Platform Specific**: Some features only available per platform

### Future Enhancements
- Support for transient and scoped lifetimes
- Lazy loading for optional components
- Circular dependency detection
- Configuration-based registration

## Troubleshooting

### Common Issues

**Build Error: "Type not found"**
- Ensure interface is included in calling code
- Check factory implementation exists for target platform

**Runtime: Nullptr Resolution**
- Verify registration in factory
- Check conditional compilation flags

**Test Failures: Mock Not Called**
- Ensure mock is registered before test execution
- Verify mock expectations are set correctly

### Debug Tips

1. **Enable Verbose Logging** (testing only):
```cpp
#ifdef PIO_UNIT_TESTING
// Add debug logging to resolve() method
#endif
```

2. **Check Registration Status**:
```cpp
if (!container->isRegistered<IMyComponent>()) {
    // Component not registered
}
```

3. **Verify Factory Platform**:
```cpp
#ifdef ESP32
    // ESP32-specific debug code
#else
    // Teensy-specific debug code
#endif
```

This dependency injection system provides a robust foundation for scalable, testable embedded software while maintaining the real-time performance characteristics required for precision machining applications.