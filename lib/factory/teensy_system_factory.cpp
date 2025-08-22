#include "system_factory.h"
#include <config.h>
#include <globalstate.h>
#include <spindle.h>
#include <leadscrew.h>
#include <display.h>
#include <leadscrew_io_teensy.h>
#include "../../src/buttons.h"

#ifndef ESP32

std::unique_ptr<ISpindle> SystemFactory::createSpindle() {
#ifdef ELS_SPINDLE_DRIVEN
    return std::make_unique<Spindle>();
#else
    return std::make_unique<Spindle>(ELS_SPINDLE_ENCODER_A, ELS_SPINDLE_ENCODER_B);
#endif
}

std::unique_ptr<LeadscrewIO> SystemFactory::createLeadscrewIO() {
    return std::make_unique<LeadscrewIOTeensy>();
}

std::unique_ptr<ILeadscrew> SystemFactory::createLeadscrew(ISpindle* spindle, LeadscrewIO* io) {
    // Cast to concrete Spindle type for constructor compatibility
    auto concreteSpindle = static_cast<Spindle*>(spindle);
    
    return std::make_unique<Leadscrew>(
        concreteSpindle,
        io,
        ACCEL_PULSE_SEC,
        LEADSCREW_INITIAL_PULSE_DELAY_US,
        ELS_LEADSCREW_STEPPER_PPR * ELS_GEARBOX_RATIO,
        ELS_LEADSCREW_PITCH_MM,
        ELS_SPINDLE_ENCODER_PPR
    );
}

std::unique_ptr<IDisplay> SystemFactory::createDisplay(ISpindle* spindle, ILeadscrew* leadscrew) {
    // Cast to concrete types for constructor compatibility
    auto concreteSpindle = static_cast<Spindle*>(spindle);
    auto concreteLeadscrew = static_cast<Leadscrew*>(leadscrew);
    
    return std::make_unique<Display>(concreteSpindle, concreteLeadscrew);
}

std::unique_ptr<IButtonHandler> SystemFactory::createButtonHandler(ISpindle* spindle, ILeadscrew* leadscrew) {
    // Cast to concrete types for constructor compatibility
    auto concreteSpindle = static_cast<Spindle*>(spindle);
    auto concreteLeadscrew = static_cast<Leadscrew*>(leadscrew);
    
    return std::make_unique<ButtonHandler>(concreteSpindle, concreteLeadscrew);
}

#endif // !ESP32