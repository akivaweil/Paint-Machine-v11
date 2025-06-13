#include "hardware/controlPanel_Functions.h"
#include "hardware/GlobalDebouncers.h"
#include "settings/pins.h"
#include "motors/PaintingSides.h"  // Include for side painting functions
#include "system/StateMachine.h"   // Include for state machine functions
#include "functionality/ManualControl.h" // Include for rotation functions
#include "hardware/paintGun_Functions.h" // Include for paint gun functions
#include "hardware/pressurePot_Functions.h" // Include for pressure pot functions
#include <FastAccelStepper.h>      // Include for stepper access
#include <AccelStepper.h>          // Include for rotation stepper

// External references
extern StateMachine* stateMachine;
extern FastAccelStepper* stepperX;
extern FastAccelStepper* stepperY_Left;
extern FastAccelStepper* stepperY_Right;
extern FastAccelStepper* stepperZ;

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
    return g_onOffSwitchDebouncer.fell();
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
    // Check which modifier is currently pressed (active low - pressed = LOW)
    if (!g_modifierLeftDebouncer.read()) return MODIFIER_LEFT;
    if (!g_modifierCenterDebouncer.read()) return MODIFIER_CENTER;
    if (!g_modifierRightDebouncer.read()) return MODIFIER_RIGHT;
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
ActionButton getTriggeredAction() {
    // Check which action was just pressed (active high - pressed = rising edge)
    if (g_actionLeftDebouncer.read()) return ACTION_LEFT;
    if (g_actionCenterDebouncer.read()) return ACTION_CENTER;
    if (g_actionRightDebouncer.read()) return ACTION_RIGHT;
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
 * @brief Get current button combination
 * @return ButtonCombination structure
 */
ButtonCombination getCurrentButtonCombination() {
    ButtonCombination combo;
    combo.modifier = getCurrentModifier();
    combo.action = getTriggeredAction();
    return combo;
}

//* ************************************************************************
//* ************************** COMBINATION HANDLERS ***********************
//* ************************************************************************

/**
 * @brief Main function to handle button combinations
 * This should be called regularly in the main loop
 */
void handleButtonCombinations() {
    ButtonCombination combo = getCurrentButtonCombination();
    
    // Only process if an action button was triggered
    if (combo.action == ACTION_NONE) {
        return; // No action button pressed
    }
    
    // Handle combinations based on modifier + action
    if (combo.modifier == MODIFIER_LEFT) {
        switch (combo.action) {
            case ACTION_LEFT:   handleModifierLeftActionLeft(); break;
            case ACTION_CENTER: handleModifierLeftActionCenter(); break;
            case ACTION_RIGHT:  handleModifierLeftActionRight(); break;
            default: break;
        }
    }
    else if (combo.modifier == MODIFIER_CENTER) {
        switch (combo.action) {
            case ACTION_LEFT:   handleModifierCenterActionLeft(); break;
            case ACTION_CENTER: handleModifierCenterActionCenter(); break;
            case ACTION_RIGHT:  handleModifierCenterActionRight(); break;
            default: break;
        }
    }
    else if (combo.modifier == MODIFIER_RIGHT) {
        switch (combo.action) {
            case ACTION_LEFT:   handleModifierRightActionLeft(); break;
            case ACTION_CENTER: handleModifierRightActionCenter(); break;
            case ACTION_RIGHT:  handleModifierRightActionRight(); break;
            default: break;
        }
    }
    else { // No modifier pressed - single action button
        switch (combo.action) {
            case ACTION_LEFT:   handleActionLeftOnly(); break;
            case ACTION_CENTER: handleActionCenterOnly(); break;
            case ACTION_RIGHT:  handleActionRightOnly(); break;
            default: break;
        }
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
    Serial.println("COMBO: Modifier Center + Action Center - Rotate Tray 90° CW");

        handleManualRotateClockwise90();
}

void handleModifierCenterActionRight() {
    Serial.println("COMBO: Modifier Center + Action Right - Rotate Tray 90° CCW");
    handleManualRotateCounterClockwise90();
}

void handleModifierRightActionLeft() {
    Serial.println("COMBO: Modifier Right + Action Left - PHYSICAL FORCE HOME");
    
    // Set the PHYSICAL home button flag - this will be checked by painting operations between movements
    extern volatile bool physicalHomeButtonPressed;
    physicalHomeButtonPressed = true;
    
    Serial.println("Physical home button pressed - painting operations will abort at next check point");
    
    // If machine is currently idle, immediately go to homing state
    if (stateMachine && stateMachine->getCurrentState() == stateMachine->getIdleState()) {
        Serial.println("Machine is idle - transitioning to homing state immediately");
        stateMachine->changeState(stateMachine->getHomingState());
    } else {
        Serial.println("Machine is busy - physical home command will be processed at next safe point");
    }
}

void handleModifierRightActionCenter() {
    Serial.println("COMBO: Modifier Right + Action Center - Test Paint Gun");
    
    // Only allow paint gun test in IDLE state for safety
    if (stateMachine && stateMachine->getCurrentState() != stateMachine->getIdleState()) {
        Serial.println("Paint gun test rejected: Machine must be in IDLE state");
        return;
    }
    
    Serial.println("Starting 3-second paint gun test...");
    
    // Turn on pressure pot first
    extern void PressurePot_ON();
    PressurePot_ON();
    delay(100); // Brief delay for pressure buildup
    
    // Turn on paint gun for 3 seconds
    paintGun_ON();
    delay(3000); // 3 second test
    paintGun_OFF();
    
    Serial.println("Paint gun test completed (3 seconds)");
}

void handleModifierRightActionRight() {
    Serial.println("COMBO: Modifier Right + Action Right - Move to Tip Inspection Position");
    
    // Only allow in IDLE state for safety
    if (stateMachine && stateMachine->getCurrentState() != stateMachine->getIdleState()) {
        Serial.println("Tip inspection rejected: Machine must be in IDLE state");
        return;
    }
    
    // Transition to InspectTipState
    if (stateMachine) {
        stateMachine->changeState(stateMachine->getInspectTipState());
        Serial.println("Transitioning to Inspect Tip state");
    } else {
        Serial.println("ERROR: StateMachine not available for tip inspection");
    }
}

// Single action button handlers (no modifier pressed)

void handleActionLeftOnly() {
    Serial.println("SINGLE ACTION: Left Button - Return Machine Home");
    if (stateMachine) {
        stateMachine->changeState(stateMachine->getHomingState());
    } else {
        Serial.println("ERROR: StateMachine not available for homing");
    }
}

void handleActionCenterOnly() {
    Serial.println("SINGLE ACTION: Center Button - Start Cleaning Cycle");
    if (stateMachine) {
        stateMachine->changeState(stateMachine->getCleaningState());
    } else {
        Serial.println("ERROR: StateMachine not available for cleaning");
    }
}

void handleActionRightOnly() {
    Serial.println("SINGLE ACTION: Right Button - Paint All Sides");
    if (stateMachine) {
        g_requestedCoats = 1; // Set to single coat
        stateMachine->setTransitioningToPaintAllSides(true);
        stateMachine->changeState(stateMachine->getPaintingState());
    } else {
        Serial.println("ERROR: StateMachine not available for painting");
    }
} 