#ifndef PIO_UNIT_TESTING
#define PIO_UNIT_TESTING
#endif

#include "arduino_test_mock.h"
#include <config.h>
#include <globalstate.h>
#include "TestSpindle.h"
#include <leadscrew.h>
#include <gmock/gmock.h>
#include "mocks/leadscrewio_mock.h"
#include "../lib/di/dependency_container.h"
#include "../lib/interfaces/system_interfaces.h"

/**
 * Example of how existing tests can be improved with dependency injection
 * This shows the same tests as test_leadscrew.cpp but using the DI container
 */

class LeadscrewWithDITest : public ::testing::Test {
protected:
    void SetUp() override {
        container = std::make_unique<DependencyContainer>();
        
        // Create dependencies
        auto spindle = std::make_unique<Spindle>();
        auto ioMock = std::make_unique<LeadscrewIOMock>();
        
        // Store raw pointers for testing
        spindlePtr = spindle.get();
        ioMockPtr = ioMock.get();
        
        // Register with container
        container->registerInstance<ISpindle>(std::move(spindle));
        container->registerInstance<LeadscrewIO>(std::move(ioMock));
        
        // Create leadscrew with dependencies from container
        auto leadscrew = std::make_unique<Leadscrew>(
            static_cast<Spindle*>(container->resolve<ISpindle>()),
            container->resolve<LeadscrewIO>(),
            100.0, 1000.0, 200, 2.0, 1000
        );
        
        leadscrewPtr = leadscrew.get();
        container->registerInstance<ILeadscrew>(std::move(leadscrew));
    }

    std::unique_ptr<DependencyContainer> container;
    Spindle* spindlePtr;
    LeadscrewIOMock* ioMockPtr;
    Leadscrew* leadscrewPtr;
};

TEST_F(LeadscrewWithDITest, ConstructionThroughDI) {
    auto leadscrew = container->resolve<ILeadscrew>();
    
    EXPECT_EQ(leadscrew->getCurrentPosition(), 0);
    EXPECT_EQ(leadscrew->getPositionError(), 0);
}

TEST_F(LeadscrewWithDITest, StopPositionManagementThroughDI) {
    // Initially stop positions should be unset
    EXPECT_EQ(leadscrewPtr->getStopPositionState(LeadscrewStopPosition::LEFT), 
             LeadscrewStopState::UNSET);
    EXPECT_EQ(leadscrewPtr->getStopPositionState(LeadscrewStopPosition::RIGHT), 
             LeadscrewStopState::UNSET);
    
    // Set left stop position
    leadscrewPtr->setStopPosition(LeadscrewStopPosition::LEFT, 100);
    EXPECT_EQ(leadscrewPtr->getStopPositionState(LeadscrewStopPosition::LEFT), 
             LeadscrewStopState::SET);
    EXPECT_EQ(leadscrewPtr->getStopPosition(LeadscrewStopPosition::LEFT), 100);
    
    // Set right stop position
    leadscrewPtr->setStopPosition(LeadscrewStopPosition::RIGHT, 500);
    EXPECT_EQ(leadscrewPtr->getStopPositionState(LeadscrewStopPosition::RIGHT), 
             LeadscrewStopState::SET);
    EXPECT_EQ(leadscrewPtr->getStopPosition(LeadscrewStopPosition::RIGHT), 500);
    
    // Unset stop positions
    leadscrewPtr->unsetStopPosition(LeadscrewStopPosition::LEFT);
    EXPECT_EQ(leadscrewPtr->getStopPositionState(LeadscrewStopPosition::LEFT), 
             LeadscrewStopState::UNSET);
}

TEST_F(LeadscrewWithDITest, ComponentInteractionThroughDI) {
    auto spindle = container->resolve<ISpindle>();
    auto leadscrew = container->resolve<ILeadscrew>();
    
    // Set up mock expectations
    EXPECT_CALL(*ioMockPtr, setStepperDirection(::testing::_)).Times(::testing::AtLeast(0));
    EXPECT_CALL(*ioMockPtr, stepPulse()).Times(::testing::AtLeast(0));
    
    // Simulate spindle movement
    spindle->setCurrentPosition(100);
    
    // Set target pitch
    leadscrew->setTargetPitchMM(1.25f);
    
    // Update leadscrew - should respond to spindle movement
    leadscrew->update();
    
    // Position error should be calculated based on spindle position and target pitch
    int positionError = leadscrew->getPositionError();
    EXPECT_NE(positionError, 0); // Should have some error to correct
}

/**
 * This test shows how DI makes it easier to test complex interactions
 * by allowing us to inject controlled dependencies
 */
TEST_F(LeadscrewWithDITest, SystemLevelIntegrationWithDI) {
    auto spindle = container->resolve<ISpindle>();
    auto leadscrew = container->resolve<ILeadscrew>();
    
    // Set up expectations for a complete motion sequence
    EXPECT_CALL(*ioMockPtr, setStepperDirection(::testing::_))
        .Times(::testing::AtLeast(1));
    EXPECT_CALL(*ioMockPtr, stepPulse())
        .Times(::testing::AtLeast(1));
    
    // Simulate a complete threading operation
    leadscrew->setTargetPitchMM(1.25f); // 1.25mm thread pitch
    
    // Simulate spindle rotation
    for (int i = 0; i < 100; ++i) {
        spindle->incrementCurrentPosition(1);
        leadscrew->update();
    }
    
    // Verify leadscrew has moved appropriately
    EXPECT_NE(leadscrew->getCurrentPosition(), 0);
}

/**
 * Factory-based test showing how SystemFactory could be used in tests
 */
class SystemFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // In a real test, we might create a TestSystemFactory that creates mock components
        // For now, we'll just show the concept
        container = std::make_unique<DependencyContainer>();
        
        // Register test doubles
        auto mockSpindle = std::make_unique<Spindle>();
        auto mockIO = std::make_unique<LeadscrewIOMock>();
        
        ioMockPtr = mockIO.get();
        
        container->registerInstance<ISpindle>(std::move(mockSpindle));
        container->registerInstance<LeadscrewIO>(std::move(mockIO));
    }

    std::unique_ptr<DependencyContainer> container;
    LeadscrewIOMock* ioMockPtr;
};

TEST_F(SystemFactoryTest, CouldUseFactoryForTestSetup) {
    // This demonstrates how a TestSystemFactory could work
    // In practice, you'd create a TestSystemFactory that injects mocks
    
    auto spindle = container->resolve<ISpindle>();
    auto leadscrewIO = container->resolve<LeadscrewIO>();
    
    EXPECT_NE(spindle, nullptr);
    EXPECT_NE(leadscrewIO, nullptr);
    
    // Mock expectations would be set up here
    EXPECT_CALL(*ioMockPtr, setStepperDirection(::testing::_))
        .Times(::testing::AnyNumber());
}

/**
 * Benefits of DI approach demonstrated:
 * 1. Cleaner test setup - dependencies managed by container
 * 2. Better isolation - can inject specific mocks for each test
 * 3. Consistent dependency resolution - same pattern as production code
 * 4. Easier to test complex interactions - all dependencies available through container
 * 5. Future-proof - easy to add new dependencies without changing test setup
 */