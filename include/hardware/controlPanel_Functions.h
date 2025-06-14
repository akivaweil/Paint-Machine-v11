#ifndef CONTROL_PANEL_FUNCTIONS_H
#define CONTROL_PANEL_FUNCTIONS_H

#include <Arduino.h>

//* ************************************************************************
//* ************************ CONTROL PANEL FUNCTIONS *********************
//* ************************************************************************

// Enumeration for modifier buttons
enum ModifierButton {
    MODIFIER_NONE = 0,
    MODIFIER_LEFT = 1,
    MODIFIER_CENTER = 2,
    MODIFIER_RIGHT = 3
};

// Enumeration for action buttons
enum ActionButton {
    ACTION_NONE = 0,
    ACTION_LEFT = 1,
    ACTION_CENTER = 2,
    ACTION_RIGHT = 3
};

// Structure to hold button combination
struct ButtonCombination {
    ModifierButton modifier;
    ActionButton action;
};

// Basic button state reading functions
bool isOnOffSwitchPressed();
bool isModifierLeftPressed();
bool isModifierCenterPressed();
bool isModifierRightPressed();
bool isActionLeftPressed();
bool isActionCenterPressed();
bool isActionRightPressed();

// Modifier button helper functions
ModifierButton getCurrentModifier();
bool isAnyModifierPressed();

// Action button detection functions
ActionButton getTriggeredAction();          // Returns the action button that was just pressed (rose edge)
ActionButton getActionButtonHeld();         // Returns the action button currently being held

// Combination detection function
ButtonCombination getCurrentButtonCombination();

// Function to update all control panel button debouncers
void updateControlPanelButtons();

// Main combination handler
void handleButtonCombinations();

// Control panel action functions - named by their functionality
void physicalForceHome();
void testPaintGun();
void moveToTipInspectionPosition();
void returnMachineHome();
void startCleaningCycle();
void paintAllSidesTwice();

#endif // CONTROL_PANEL_FUNCTIONS_H 