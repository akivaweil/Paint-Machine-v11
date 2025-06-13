#include "hardware/controlPanel_Functions.h"
#include "hardware/GlobalDebouncers.h"
#include "settings/pins.h"
#include "motors/PaintingSides.h"  // Include for side painting functions

//* ************************************************************************
//* ************************ CONTROL PANEL FUNCTIONS *********************
//* ************************************************************************

/**
 * @brief Update all control panel button debouncers
 * This function should be called regularly in the main loop
 */
void updateControlPanelButtons() {
    g_onOffSwitchDebouncer.update();
    g_modifierLeftDebouncer.update();
    g_modifierCenterDebouncer.update();
    g_modifierRightDebouncer.update();
    g_actionLeftDebouncer.update();
    g_actionCenterDebouncer.update();
    g_actionRightDebouncer.update();
}

//* ************************************************************************
//* ************************** BASIC BUTTON READING **********************
//* ************************************************************************

/**
 * @brief Check if On/Off switch is pressed
 * @return true if switch is pressed, false otherwise
 */
bool isOnOffSwitchPressed() {
    return g_onOffSwitchDebouncer.read();
}

/**
 * @brief Check if Left Modifier button is pressed (Active Low)
 * @return true if button is pressed, false otherwise
 */
bool isModifierLeftPressed() {
    return !g_modifierLeftDebouncer.read();  // Invert because Active Low
}

/**
 * @brief Check if Center Modifier button is pressed (Active Low)
 * @return true if button is pressed, false otherwise
 */
bool isModifierCenterPressed() {
    return !g_modifierCenterDebouncer.read();  // Invert because Active Low
}

/**
 * @brief Check if Right Modifier button is pressed (Active Low)
 * @return true if button is pressed, false otherwise
 */
bool isModifierRightPressed() {
    return !g_modifierRightDebouncer.read();  // Invert because Active Low
}

/**
 * @brief Check if Left Action button is pressed (Active High)
 * @return true if button is pressed, false otherwise
 */
bool isActionLeftPressed() {
    return g_actionLeftDebouncer.read();
}

/**
 * @brief Check if Center Action button is pressed (Active High)
 * @return true if button is pressed, false otherwise
 */
bool isActionCenterPressed() {
    return g_actionCenterDebouncer.read();
}

/**
 * @brief Check if Right Action button is pressed (Active High)
 * @return true if button is pressed, false otherwise
 */
bool isActionRightPressed() {
    return g_actionRightDebouncer.read();
}

//* ************************************************************************
//* ************************** MODIFIER HELPERS ***************************
//* ************************************************************************

/**
 * @brief Get the currently pressed modifier button
 * @return ModifierButton enum value
 */
ModifierButton getCurrentModifier() {
    if (isModifierLeftPressed()) return MODIFIER_LEFT;
    if (isModifierCenterPressed()) return MODIFIER_CENTER;
    if (isModifierRightPressed()) return MODIFIER_RIGHT;
    return MODIFIER_NONE;
}

/**
 * @brief Check if any modifier button is currently pressed
 * @return true if any modifier is pressed, false otherwise
 */
bool isAnyModifierPressed() {
    return getCurrentModifier() != MODIFIER_NONE;
}

//* ************************************************************************
//* ************************** ACTION DETECTION ****************************
//* ************************************************************************

/**
 * @brief Get action button that was just pressed (rising edge)
 * @return ActionButton enum value
 */
ActionButton getActionButtonPressed() {
    if (g_actionLeftDebouncer.rose()) return ACTION_LEFT;
    if (g_actionCenterDebouncer.rose()) return ACTION_CENTER;
    if (g_actionRightDebouncer.rose()) return ACTION_RIGHT;
    return ACTION_NONE;
}

/**
 * @brief Get action button that is currently being held
 * @return ActionButton enum value
 */
ActionButton getActionButtonHeld() {
    if (isActionLeftPressed()) return ACTION_LEFT;
    if (isActionCenterPressed()) return ACTION_CENTER;
    if (isActionRightPressed()) return ACTION_RIGHT;
    return ACTION_NONE;
}

//* ************************************************************************
//* ************************** COMBINATION DETECTION **********************
//* ************************************************************************

/**
 * @brief Detect button combination when action button is pressed
 * @return ButtonCombination structure
 */
ButtonCombination detectButtonCombination() {
    ButtonCombination combo;
    combo.modifier = MODIFIER_NONE;
    combo.action = ACTION_NONE;

    // Check if an action button was just pressed
    ActionButton actionPressed = getActionButtonPressed();
    if (actionPressed != ACTION_NONE) {
        combo.action = actionPressed;
        combo.modifier = getCurrentModifier();  // Get modifier state at time of action press
    }

    return combo;
}

//* ************************************************************************
//* ************************** COMBINATION HANDLERS ***********************
//* ************************************************************************

/**
 * @brief Main function to handle button combinations
 * Call this in your main loop after updateControlPanelButtons()
 */
void handleButtonCombinations() {
    ButtonCombination combo = detectButtonCombination();
    
    if (combo.action == ACTION_NONE) return;  // No action button pressed

    // Handle combinations
    switch (combo.modifier) {
        case MODIFIER_LEFT:
            switch (combo.action) {
                case ACTION_LEFT:
                    handleModifierLeftActionLeft();
                    break;
                case ACTION_CENTER:
                    handleModifierLeftActionCenter();
                    break;
                case ACTION_RIGHT:
                    handleModifierLeftActionRight();
                    break;
            }
            break;

        case MODIFIER_CENTER:
            switch (combo.action) {
                case ACTION_LEFT:
                    handleModifierCenterActionLeft();
                    break;
                case ACTION_CENTER:
                    handleModifierCenterActionCenter();
                    break;
                case ACTION_RIGHT:
                    handleModifierCenterActionRight();
                    break;
            }
            break;

        case MODIFIER_RIGHT:
            switch (combo.action) {
                case ACTION_LEFT:
                    handleModifierRightActionLeft();
                    break;
                case ACTION_CENTER:
                    handleModifierRightActionCenter();
                    break;
                case ACTION_RIGHT:
                    handleModifierRightActionRight();
                    break;
            }
            break;

        case MODIFIER_NONE:
            // Handle single action button presses (no modifier held)
            switch (combo.action) {
                case ACTION_LEFT:
                    Serial.println("Action Left pressed (no modifier)");
                    break;
                case ACTION_CENTER:
                    Serial.println("Action Center pressed (no modifier)");
                    break;
                case ACTION_RIGHT:
                    Serial.println("Action Right pressed (no modifier)");
                    break;
            }
            break;
    }
}

//* ************************************************************************
//* **************** INDIVIDUAL COMBINATION FUNCTIONS *********************
//* ************************************************************************

void handleModifierLeftActionLeft() {
    Serial.println("COMBO: Modifier Left + Action Left - Paint Side 1");
    paintSide1Pattern();
}

void handleModifierLeftActionCenter() {
    Serial.println("COMBO: Modifier Left + Action Center - Paint Side 2");
    paintSide2Pattern();
}

void handleModifierLeftActionRight() {
    Serial.println("COMBO: Modifier Left + Action Right - Paint Side 3");
    paintSide3Pattern();
}

void handleModifierCenterActionLeft() {
    Serial.println("COMBO: Modifier Center + Action Left - Paint Side 4");
    paintSide4Pattern();
}

void handleModifierCenterActionCenter() {
    Serial.println("COMBO: Modifier Center + Action Center - Calibrate Machine");
    // Add your functionality here - example: calibration routine
}

void handleModifierCenterActionRight() {
    Serial.println("COMBO: Modifier Center + Action Right - Settings Menu");
    // Add your functionality here - example: enter settings mode
}

void handleModifierRightActionLeft() {
    Serial.println("COMBO: Modifier Right + Action Left - Test Paint Gun");
    // Add your functionality here - example: paint gun test
}

void handleModifierRightActionCenter() {
    Serial.println("COMBO: Modifier Right + Action Center - Clean Cycle");
    // Add your functionality here - example: cleaning cycle
}

void handleModifierRightActionRight() {
    Serial.println("COMBO: Modifier Right + Action Right - System Info");
    // Add your functionality here - example: display system information
} 