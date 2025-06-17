#include "states/ErrorState.h"
#include <Arduino.h>
#include "system/StateMachine.h"
#include "system/GlobalState.h"

// Reference to the global state machine instance
extern StateMachine* stateMachine;

//* ************************************************************************
//* ***************************** ERROR STATE *****************************
//* ************************************************************************
// This file defines the ErrorState class, which represents the state
// where the machine encounters an error condition requiring user intervention.
// It handles error reporting, safety measures, and recovery procedures.

ErrorState::ErrorState() {
    // Constructor implementation
}

void ErrorState::enter() {
    Serial.println("!!! ENTERING ERROR STATE !!!");
    
    // Stop all motors immediately for safety
    // stopAllMotors(); // Implement this function to stop all stepper and servo motors
    
    // Turn off all pneumatic systems for safety
    // turnOffAllPneumatics(); // Implement this function to safely disable pneumatics
    
    // Set error indicators
    // setErrorIndicators(true); // Implement this function to show error LEDs/indicators
    
    // Log the error entry
    Serial.println("ERROR: Machine has entered error state");
    Serial.println("ERROR: All systems stopped for safety");
    Serial.println("ERROR: Manual intervention required");
    
    // Clear any active operations
    isPaused = false;
    isActivePainting = false;
    
    Serial.println("ERROR: To recover, resolve the issue and restart the system");
}

void ErrorState::update() {
    // In error state, we typically don't perform continuous operations
    // Instead, we wait for manual intervention or system reset
    
    // Optionally flash error indicators
    static unsigned long lastFlashTime = 0;
    static bool errorIndicatorState = false;
    
    if (millis() - lastFlashTime > 1000) { // Flash every 1 second
        errorIndicatorState = !errorIndicatorState;
        // flashErrorIndicators(errorIndicatorState);
        lastFlashTime = millis();
    }
    
    // Print error message periodically (every 10 seconds)
    static unsigned long lastErrorMessage = 0;
    if (millis() - lastErrorMessage > 10000) {
        Serial.println("ERROR: System in error state - manual intervention required");
        lastErrorMessage = millis();
    }
    
    // Check for reset conditions (if implemented)
    // For example, a specific button combination or web command
    // if (checkForErrorReset()) {
    //     Serial.println("ERROR: Reset condition detected, returning to IDLE");
    //     stateMachine->changeState(stateMachine->getIdleState());
    // }
}

void ErrorState::exit() {
    Serial.println("Exiting Error State");
    
    // Turn off error indicators
    // setErrorIndicators(false);
    
    // Perform any cleanup necessary when leaving error state
    // resetErrorConditions();
    
    Serial.println("ERROR: Error state cleared - resuming normal operation");
}

const char* ErrorState::getName() const {
    return "ERROR";
} 