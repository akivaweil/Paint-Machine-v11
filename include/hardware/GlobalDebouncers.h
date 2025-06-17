#ifndef GLOBAL_DEBOUNCERS_H
#define GLOBAL_DEBOUNCERS_H

#include <Bounce2.h>

// Global debouncer for the PNP Cycle Sensor/Switch
extern Bounce g_pnpCycleSensorDebouncer;

// Control Panel Button Debouncers
extern Bounce g_onOffSwitchDebouncer;

// Modifier Buttons (Top Row - Active Low)
extern Bounce g_modifierLeftDebouncer;
extern Bounce g_modifierCenterDebouncer;
extern Bounce g_modifierRightDebouncer;

// Action Buttons (Bottom Row - Active High)
extern Bounce g_actionLeftDebouncer;
extern Bounce g_actionCenterDebouncer;
extern Bounce g_actionRightDebouncer;

// Add other global debouncers here if needed in the future

/**
 * @brief Initializes all global debouncer objects.
 * This function should be called once during setup.
 */
void initializeGlobalDebouncers();

#endif // GLOBAL_DEBOUNCERS_H 