#include "hardware/GlobalDebouncers.h"
#include "../config/Pins_Definitions.h" // For PNP_CYCLE_SENSOR_PIN
#include <Arduino.h>       // For INPUT, pinMode
#include "settings/debounce_settings.h" // Added for centralized debounce intervals

// Definition of the global PNP Cycle Sensor debouncer
Bounce g_pnpCycleSensorDebouncer;

// Definition of the global Control Panel Button debouncers
Bounce g_onOffSwitchDebouncer;

// Modifier Buttons (Top Row - Active Low)
Bounce g_modifierLeftDebouncer;
Bounce g_modifierCenterDebouncer;
Bounce g_modifierRightDebouncer;

// Action Buttons (Bottom Row - Active High)
Bounce g_actionLeftDebouncer;
Bounce g_actionCenterDebouncer;
Bounce g_actionRightDebouncer;

/**
 * @brief Initializes all global debouncer objects.
 * Configures pin modes and attaches debouncers.
 */
void initializeGlobalDebouncers() {
    // PNP_CYCLE_SENSOR_PIN is an active LOW IR sensor.
    // Use INPUT_PULLUP so the pin is HIGH when idle and LOW when active.
    // Bounce2's attach(pin, mode) sets the pinMode internally.
    g_pnpCycleSensorDebouncer.attach(PNP_CYCLE_SENSOR_PIN, INPUT_PULLUP);
    g_pnpCycleSensorDebouncer.interval(PNP_CYCLE_SENSOR_DEBOUNCE_MS);

    // On/Off Switch - Physical switch is Active HIGH (use INPUT_PULLDOWN)
    g_onOffSwitchDebouncer.attach(ON_OFF_SWITCH_PIN, INPUT_PULLDOWN);
    g_onOffSwitchDebouncer.interval(GENERAL_DEBOUNCE_MS);

    // Modifier Buttons (Top Row) - Active LOW switches with internal pullups
    // These buttons are designed to be held down as modifiers
    g_modifierLeftDebouncer.attach(MODIFIER_BUTTON_LEFT, INPUT_PULLUP);
    g_modifierLeftDebouncer.interval(GENERAL_DEBOUNCE_MS);
    
    g_modifierCenterDebouncer.attach(MODIFIER_BUTTON_CENTER, INPUT_PULLUP);
    g_modifierCenterDebouncer.interval(GENERAL_DEBOUNCE_MS);
    
    g_modifierRightDebouncer.attach(MODIFIER_BUTTON_RIGHT, INPUT_PULLUP);
    g_modifierRightDebouncer.interval(GENERAL_DEBOUNCE_MS);

    // Action Buttons (Bottom Row) - Active HIGH switches with internal pulldowns
    // These buttons trigger actions when pressed in combination with modifiers
    g_actionLeftDebouncer.attach(ACTION_BUTTON_LEFT, INPUT_PULLDOWN);
    g_actionLeftDebouncer.interval(GENERAL_DEBOUNCE_MS);
    
    g_actionCenterDebouncer.attach(ACTION_BUTTON_CENTER, INPUT_PULLDOWN);
    g_actionCenterDebouncer.interval(GENERAL_DEBOUNCE_MS);
    
    g_actionRightDebouncer.attach(ACTION_BUTTON_RIGHT, INPUT_PULLDOWN);
    g_actionRightDebouncer.interval(GENERAL_DEBOUNCE_MS);
}