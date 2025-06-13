#ifndef CONTROL_PANEL_FUNCTIONS_H
#define CONTROL_PANEL_FUNCTIONS_H

#include <Arduino.h>

//* ************************************************************************
//* ************************ CONTROL PANEL FUNCTIONS *********************
//* ************************************************************************

// Function declarations for control panel button reading
bool isOnOffSwitchPressed();
bool isButton44Pressed();
bool isButton1Pressed();
bool isButton43Pressed();
bool isButton17Pressed();
bool isButton7Pressed();
bool isButton16Pressed();

// Function to update all control panel button debouncers
void updateControlPanelButtons();

#endif // CONTROL_PANEL_FUNCTIONS_H 