#include "hardware/GlobalDebouncers.h"
#include "settings/pins.h" // For PNP_CYCLE_SENSOR_PIN
#include <Arduino.h>       // For INPUT, pinMode
#include "settings/debounce_settings.h" // Added for centralized debounce intervals

// Definition of the global PNP Cycle Sensor debouncer
Bounce g_pnpCycleSensorDebouncer;

// Definition of the global Control Panel Button debouncers
Bounce g_onOffSwitchDebouncer;
Bounce g_button44Debouncer;
Bounce g_button1Debouncer;
Bounce g_button43Debouncer;
Bounce g_button17Debouncer;
Bounce g_button7Debouncer;
Bounce g_button16Debouncer;

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

    // Control Panel Buttons - Physical switches are Active HIGH (use INPUT_PULLDOWN)
    // Based on image: "Pulldowns Active High"
    g_onOffSwitchDebouncer.attach(ON_OFF_SWITCH_PIN, INPUT_PULLDOWN);
    g_onOffSwitchDebouncer.interval(GENERAL_DEBOUNCE_MS);
    
    g_button44Debouncer.attach(BUTTON_PIN_44, INPUT_PULLDOWN);
    g_button44Debouncer.interval(GENERAL_DEBOUNCE_MS);
    
    g_button1Debouncer.attach(BUTTON_PIN_1, INPUT_PULLDOWN);
    g_button1Debouncer.interval(GENERAL_DEBOUNCE_MS);
    
    g_button43Debouncer.attach(BUTTON_PIN_43, INPUT_PULLDOWN);
    g_button43Debouncer.interval(GENERAL_DEBOUNCE_MS);
    
    g_button17Debouncer.attach(BUTTON_PIN_17, INPUT_PULLDOWN);
    g_button17Debouncer.interval(GENERAL_DEBOUNCE_MS);
    
    g_button7Debouncer.attach(BUTTON_PIN_7, INPUT_PULLDOWN);
    g_button7Debouncer.interval(GENERAL_DEBOUNCE_MS);
    
    g_button16Debouncer.attach(BUTTON_PIN_16, INPUT_PULLDOWN);
    g_button16Debouncer.interval(GENERAL_DEBOUNCE_MS);
}