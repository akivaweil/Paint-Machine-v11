#ifndef GLOBAL_DEBOUNCERS_H
#define GLOBAL_DEBOUNCERS_H

#include <Bounce2.h>

// Global debouncer for the PNP Cycle Sensor/Switch
extern Bounce g_pnpCycleSensorDebouncer;

// Control Panel Button Debouncers
extern Bounce g_onOffSwitchDebouncer;
extern Bounce g_button44Debouncer;
extern Bounce g_button1Debouncer;
extern Bounce g_button43Debouncer;
extern Bounce g_button17Debouncer;
extern Bounce g_button7Debouncer;
extern Bounce g_button16Debouncer;

// Add other global debouncers here if needed in the future

/**
 * @brief Initializes all global debouncer objects.
 * This function should be called once during setup.
 */
void initializeGlobalDebouncers();

#endif // GLOBAL_DEBOUNCERS_H 