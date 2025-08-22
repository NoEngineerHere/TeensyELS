// Implementation file for UserInteractionHandler
// Handler map for each HIDActionType is now available in the base class (see header).
// Currently empty as all logic is in the header for now.
// Add implementation here if/when needed for non-inline methods. 

#include "UserInteractionHandler.h"
#include <algorithm>
#include <unordered_map>
#include <tuple>

namespace {
    // Helper hash for tuple<int,int,ButtonActionType>
    struct ButtonPosHasher {
        std::size_t operator()(const std::tuple<int, int, ButtonActionType>& t) const noexcept {
            auto h1 = std::hash<int>{}(std::get<0>(t));
            auto h2 = std::hash<int>{}(std::get<1>(t));
            auto h3 = std::hash<int>{}(static_cast<int>(std::get<2>(t)));
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

// Local lookup table generated from BUTTON_DEFINITIONS so we don't have to
// iterate the array on every lookup. Generated once at program start.
static const std::unordered_map<std::tuple<int, int, ButtonActionType>, HIDValue, ButtonPosHasher>& getButtonLookupTable() {
    static std::unordered_map<std::tuple<int, int, ButtonActionType>, HIDValue, ButtonPosHasher> table;
    static bool initialised = false;
    if (!initialised) {
        for (size_t i = 0; i < sizeof(BUTTON_DEFINITIONS) / sizeof(BUTTON_DEFINITIONS[0]); ++i) {
            const auto& def = BUTTON_DEFINITIONS[i];
            table.emplace(std::make_tuple(def.row, def.col, def.actionType), def.name);
        }
        initialised = true;
    }
    return table;
}

UserInteractionHandler::UserInteractionHandler() {
    std::fill_n(buttonHandlerMap, static_cast<size_t>(ButtonActionType::RELEASE) + 1, nullptr);

    // Populate actionMap and encoderActionMap with stub lambdas.
    for (size_t i = 0; i < static_cast<size_t>(HIDValue::NUM_VALUES); ++i) {
        HIDValue v = static_cast<HIDValue>(i);
        actionMap[v] = [v]() {
            // Stub lambda – replace in application code.
            (void)v; // suppress unused warnings
            };
    }

    for (size_t i = 0; i < sizeof(ENCODER_DEFINITIONS) / sizeof(ENCODER_DEFINITIONS[0]); ++i) {
        HIDValue v = ENCODER_DEFINITIONS[i].name;
        encoderActionMap[v] = [v]() { (void)v; };
    }
}

void UserInteractionHandler::setButtonHandler(ButtonActionType actionType, ButtonHandlerFunc handler) {
    buttonHandlerMap[static_cast<size_t>(actionType)] = handler;
}

const ButtonDefinition* UserInteractionHandler::findButtonDefinition(HIDValue value, ButtonActionType actionType) {
    for (size_t i = 0; i < sizeof(BUTTON_DEFINITIONS) / sizeof(BUTTON_DEFINITIONS[0]); ++i) {
        if (BUTTON_DEFINITIONS[i].name == value && BUTTON_DEFINITIONS[i].actionType == actionType) {
            return &BUTTON_DEFINITIONS[i];
        }
    }
    return nullptr;
}

HIDValue UserInteractionHandler::getHIDValueForButton(int row, int col, ButtonActionType actionType) {
    const auto& table = getButtonLookupTable();
    auto it = table.find(std::make_tuple(row, col, actionType));
    if (it == table.end()) {
        return HIDValue::NUM_VALUES; // sentinel for not found
    }
    return it->second;
}

constexpr const ButtonDefinition* UserInteractionHandler::getButtonDefinitions() {
    return BUTTON_DEFINITIONS;
}

constexpr size_t UserInteractionHandler::getButtonDefinitionCount() {
    return sizeof(BUTTON_DEFINITIONS) / sizeof(BUTTON_DEFINITIONS[0]);
}

// ---------------- Action registration helpers -----------------

void UserInteractionHandler::registerAction(HIDValue value, const std::function<void()>& handler) {
    actionMap[value] = handler;
}

void UserInteractionHandler::registerEncoderAction(HIDValue value, const std::function<void()>& handler) {
    encoderActionMap[value] = handler;
}

// --------------- Action execution ----------------------------

void UserInteractionHandler::handleButtonAction(int row, int col, ButtonActionType actionType) {

    auto it = getButtonLookupTable().find(std::make_tuple(row, col, actionType));
    if (it != getButtonLookupTable().end()) {
        HIDValue value = it->second;
        auto it2 = actionMap.find(value);
        if (it2 != actionMap.end() && it2->second) {
            it2->second();
        }
    }

}

void UserInteractionHandler::handleEncoderAction(HIDValue value, EncoderActionType /*actionType*/) {
    auto it = encoderActionMap.find(value);
    if (it != encoderActionMap.end() && it->second) {
        it->second();
    }
}
