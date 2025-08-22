#ifndef PIO_UNIT_TESTING
#define PIO_UNIT_TESTING
#endif

#include "arduino_test_mock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../lib/di/dependency_container.h"
#include "../lib/interfaces/system_interfaces.h"
#include "mocks/axis_mock.h"

// Mock implementations for testing
class MockSpindle : public ISpindle {
public:
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(void, setCurrentPosition, (int position), (override));
    MOCK_METHOD(void, incrementCurrentPosition, (int amount), (override));
    MOCK_METHOD(int, getCurrentPosition, (), (override));
    MOCK_METHOD(int, consumePosition, (), (override));
    MOCK_METHOD(float, getEstimatedVelocityInRPM, (), (override));
    MOCK_METHOD(float, getEstimatedVelocityInPPS, (), (override));
    MOCK_METHOD(uint32_t, getEstimatedVelocityInPulsesPerSecond, (), (override));
};

class MockLeadscrew : public ILeadscrew {
public:
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(void, setTargetPitchMM, (float ratio), (override));
    MOCK_METHOD(void, setCurrentPosition, (int position), (override));
    MOCK_METHOD(int, getCurrentPosition, (), (override));
    MOCK_METHOD(int, getPositionError, (), (override));
    MOCK_METHOD(float, getEstimatedVelocityInMillimetersPerSecond, (), (override));
};

class MockDisplay : public IDisplay {
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, update, (), (override));
};

class DependencyInjectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        container = std::make_unique<DependencyContainer>();
    }

    std::unique_ptr<DependencyContainer> container;
};

TEST_F(DependencyInjectionTest, CanRegisterAndResolveSpindle) {
    // Arrange
    auto mockSpindle = std::make_unique<MockSpindle>();
    auto spindlePtr = mockSpindle.get();
    
    // Act
    container->registerSingleton<ISpindle>(std::move(mockSpindle));
    auto resolved = container->resolve<ISpindle>();
    
    // Assert
    EXPECT_EQ(resolved, spindlePtr);
    EXPECT_TRUE(container->isRegistered<ISpindle>());
}

TEST_F(DependencyInjectionTest, CanRegisterAndResolveLeadscrew) {
    // Arrange
    auto mockLeadscrew = std::make_unique<MockLeadscrew>();
    auto leadscrewPtr = mockLeadscrew.get();
    
    // Act
    container->registerSingleton<ILeadscrew>(std::move(mockLeadscrew));
    auto resolved = container->resolve<ILeadscrew>();
    
    // Assert
    EXPECT_EQ(resolved, leadscrewPtr);
    EXPECT_TRUE(container->isRegistered<ILeadscrew>());
}

TEST_F(DependencyInjectionTest, CanRegisterAndResolveDisplay) {
    // Arrange
    auto mockDisplay = std::make_unique<MockDisplay>();
    auto displayPtr = mockDisplay.get();
    
    // Act
    container->registerSingleton<IDisplay>(std::move(mockDisplay));
    auto resolved = container->resolve<IDisplay>();
    
    // Assert
    EXPECT_EQ(resolved, displayPtr);
    EXPECT_TRUE(container->isRegistered<IDisplay>());
}

TEST_F(DependencyInjectionTest, ThrowsExceptionForUnregisteredType) {
    // Assert
    EXPECT_THROW(container->resolve<ISpindle>(), std::runtime_error);
    EXPECT_FALSE(container->isRegistered<ISpindle>());
}

TEST_F(DependencyInjectionTest, CanRegisterMultipleComponents) {
    // Arrange
    auto mockSpindle = std::make_unique<MockSpindle>();
    auto mockLeadscrew = std::make_unique<MockLeadscrew>();
    auto mockDisplay = std::make_unique<MockDisplay>();
    
    auto spindlePtr = mockSpindle.get();
    auto leadscrewPtr = mockLeadscrew.get();
    auto displayPtr = mockDisplay.get();
    
    // Act
    container->registerSingleton<ISpindle>(std::move(mockSpindle));
    container->registerSingleton<ILeadscrew>(std::move(mockLeadscrew));
    container->registerSingleton<IDisplay>(std::move(mockDisplay));
    
    // Assert
    EXPECT_EQ(container->resolve<ISpindle>(), spindlePtr);
    EXPECT_EQ(container->resolve<ILeadscrew>(), leadscrewPtr);
    EXPECT_EQ(container->resolve<IDisplay>(), displayPtr);
    
    EXPECT_TRUE(container->isRegistered<ISpindle>());
    EXPECT_TRUE(container->isRegistered<ILeadscrew>());
    EXPECT_TRUE(container->isRegistered<IDisplay>());
}

TEST_F(DependencyInjectionTest, ComponentsCanInteractThroughInterfaces) {
    // Arrange
    auto mockSpindle = std::make_unique<MockSpindle>();
    auto mockLeadscrew = std::make_unique<MockLeadscrew>();
    
    // Set up expectations
    EXPECT_CALL(*mockSpindle, update()).Times(1);
    EXPECT_CALL(*mockLeadscrew, update()).Times(1);
    EXPECT_CALL(*mockSpindle, consumePosition()).WillOnce(::testing::Return(100));
    
    container->registerSingleton<ISpindle>(std::move(mockSpindle));
    container->registerSingleton<ILeadscrew>(std::move(mockLeadscrew));
    
    // Act - Simulate timer callback behavior
    auto spindle = container->resolve<ISpindle>();
    auto leadscrew = container->resolve<ILeadscrew>();
    
    spindle->update();
    leadscrew->update();
    int position = spindle->consumePosition();
    
    // Assert
    EXPECT_EQ(position, 100);
}

class IntegrationTestWithDI : public ::testing::Test {
protected:
    void SetUp() override {
        container = std::make_unique<DependencyContainer>();
        
        // Register mock components
        auto mockSpindle = std::make_unique<MockSpindle>();
        auto mockLeadscrew = std::make_unique<MockLeadscrew>();
        auto mockDisplay = std::make_unique<MockDisplay>();
        
        spindleMock = mockSpindle.get();
        leadscrewMock = mockLeadscrew.get();
        displayMock = mockDisplay.get();
        
        container->registerSingleton<ISpindle>(std::move(mockSpindle));
        container->registerSingleton<ILeadscrew>(std::move(mockLeadscrew));
        container->registerSingleton<IDisplay>(std::move(mockDisplay));
    }

    std::unique_ptr<DependencyContainer> container;
    MockSpindle* spindleMock;
    MockLeadscrew* leadscrewMock;
    MockDisplay* displayMock;
};

TEST_F(IntegrationTestWithDI, CanSimulateSystemInitialization) {
    // Set up expectations for system initialization
    EXPECT_CALL(*displayMock, init()).Times(1);
    EXPECT_CALL(*leadscrewMock, setTargetPitchMM(::testing::_)).Times(1);
    EXPECT_CALL(*displayMock, update()).Times(1);
    
    // Act - Simulate initialization sequence
    auto display = container->resolve<IDisplay>();
    auto leadscrew = container->resolve<ILeadscrew>();
    
    display->init();
    leadscrew->setTargetPitchMM(1.25f); // Example pitch
    display->update();
    
    // All expectations verified in destructor
}

TEST_F(IntegrationTestWithDI, CanSimulateTimerCallback) {
    // Set up expectations for timer callback
    EXPECT_CALL(*spindleMock, update()).Times(1);
    EXPECT_CALL(*leadscrewMock, update()).Times(1);
    
    // Act - Simulate timer callback
    auto spindle = container->resolve<ISpindle>();
    auto leadscrew = container->resolve<ILeadscrew>();
    
    spindle->update();
    leadscrew->update();
    
    // All expectations verified in destructor
}