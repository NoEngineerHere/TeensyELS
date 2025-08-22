#pragma once
#include <config.h>
#include <stddef.h>
#include <functional>
#include <unordered_map>
#include <tuple>
#include <type_traits>

// Utility hasher that works for enum class types so we can use them in unordered_map
struct EnumClassHash {
    template <typename T>
    std::size_t operator()(T t) const noexcept {
        return std::hash<std::underlying_type_t<T>>{}(static_cast<std::underlying_type_t<T>>(t));
    }
};

// The UserInteractionHandler class provides a flexible and extensible way to handle
// user interactions (buttons, encoders, etc.) using BUTTON_DEFINITIONS and the HID enums.
class UserInteractionHandler {
public:
    /**
     * @brief Handle a button coming from the hardware layer.
     * This looks up the configured HID value and executes the
     * associated lambda stored in the action map.
     */
    void handleButtonAction(int row, int col, ButtonActionType actionType);

    /**
     * @brief Handle an encoder event (CW / CCW).
     * Currently just executes a stub lambda held in encoderActionMap.
     */
    void handleEncoderAction(HIDValue value, EncoderActionType actionType);

    // Utility: Find the ButtonDefinition for a given value and action type
    static const ButtonDefinition* findButtonDefinition(HIDValue value, ButtonActionType actionType);

    // Utility: Iterate all button definitions (for extensibility)
    static constexpr const ButtonDefinition* getButtonDefinitions();
    static constexpr size_t getButtonDefinitionCount();

    /**
     * @brief Given a physical button location and an action type (press/hold/etc),
     *        return the HID value that should be triggered. Returns HIDValue::NUM_VALUES
     *        if the lookup fails.
     */
    static HIDValue getHIDValueForButton(int row, int col, ButtonActionType actionType);

    /**
     * @brief Register/override the lambda for a particular HID action.
     */
    void registerAction(HIDValue value, const std::function<void()>& handler);

    /**
     * @brief Register/override the lambda for an encoder action.
     */
    void registerEncoderAction(HIDValue value, const std::function<void()>& handler);

    virtual ~UserInteractionHandler() = default;

protected:
    using ButtonHandlerFunc = void(*)(HIDValue, ButtonActionType);
    UserInteractionHandler();
    void setButtonHandler(ButtonActionType actionType, ButtonHandlerFunc handler);
    ButtonHandlerFunc buttonHandlerMap[static_cast<size_t>(ButtonActionType::NUM_BUTTON_ACTIONS)];

    // Map holding action lambdas for buttons/encoders.
    std::unordered_map<HIDValue, std::function<void()>, EnumClassHash> actionMap;
    std::unordered_map<HIDValue, std::function<void()>, EnumClassHash> encoderActionMap;
    // You can add encoder handler map if needed
};

// Example: To extend, subclass UserInteractionHandler and override handleAction.
// You can add new HIDActionType or HIDValue in config.h and update BUTTON_DEFINITIONS accordingly.

// Example: To extend, subclass UserInteractionHandler and override handleAction.
// You can add new HIDActionType or HIDValue in config.h and update BUTTON_DEFINITIONS accordingly. 