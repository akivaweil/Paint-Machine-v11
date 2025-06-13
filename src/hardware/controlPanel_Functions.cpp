#include "hardware/controlPanel_Functions.h"
#include "hardware/GlobalDebouncers.h"
#include "settings/pins.h"

//* ************************************************************************
//* ************************ CONTROL PANEL FUNCTIONS *********************
//* ************************************************************************

/**
 * @brief Update all control panel button debouncers
 * This function should be called regularly in the main loop
 */
void updateControlPanelButtons() {
    g_onOffSwitchDebouncer.update();
    g_button44Debouncer.update();
    g_button1Debouncer.update();
    g_button43Debouncer.update();
    g_button17Debouncer.update();
    g_button7Debouncer.update();
    g_button16Debouncer.update();
}

/**
 * @brief Check if On/Off switch is pressed
 * @return true if switch is pressed, false otherwise
 */
bool isOnOffSwitchPressed() {
    return g_onOffSwitchDebouncer.read();
}

/**
 * @brief Check if Button 44 is pressed
 * @return true if button is pressed, false otherwise
 */
bool isButton44Pressed() {
    return g_button44Debouncer.read();
}

/**
 * @brief Check if Button 1 is pressed  
 * @return true if button is pressed, false otherwise
 */
bool isButton1Pressed() {
    return g_button1Debouncer.read();
}

/**
 * @brief Check if Button 43 is pressed
 * @return true if button is pressed, false otherwise
 */
bool isButton43Pressed() {
    return g_button43Debouncer.read();
}

/**
 * @brief Check if Button 17 is pressed
 * @return true if button is pressed, false otherwise
 */
bool isButton17Pressed() {
    return g_button17Debouncer.read();
}

/**
 * @brief Check if Button 7 is pressed
 * @return true if button is pressed, false otherwise
 */
bool isButton7Pressed() {
    return g_button7Debouncer.read();
}

/**
 * @brief Check if Button 16 is pressed
 * @return true if button is pressed, false otherwise
 */
bool isButton16Pressed() {
    return g_button16Debouncer.read();
} 