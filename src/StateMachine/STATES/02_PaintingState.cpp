#include "states/PaintingState.h"
#include <Arduino.h>
#include "motors/PaintingSides.h" // Include header for paintAllSides
#include "system/StateMachine.h"  // Include header for StateMachine access
// #include "motors/Homing.h"        // REMOVE: Homing will be handled by HomingState
#include <FastAccelStepper.h>      // Include for stepper access
#include "persistence/PaintingSettings.h"
#include "motors/Rotation_Motor.h"
// #include "system/machine_state.h" // No longer needed
#include "hardware/paintGun_Functions.h" // Added include for paintGun_OFF
#include "motors/XYZ_Movements.h"      // ADDED: For moveToXYZ
#include "utils/settings.h"            // ADDED: For default speeds
#include "system/GlobalState.h"        // ADDED: For isPaused global variable

// Define necessary variables or includes specific to PaintingState if known
// #include "settings.h"
// #include "XYZ_Movements.h"

extern StateMachine *stateMachine; // Access the global state machine instance

// Need access to the global stepper instances and engine
extern FastAccelStepperEngine engine;
extern FastAccelStepper* stepperX;
extern FastAccelStepper* stepperY_Left;
extern FastAccelStepper* stepperY_Right;
extern FastAccelStepper* stepperZ;

//* ************************************************************************
//* ************************* PAINTING STATE ***************************
//* ************************************************************************

PaintingState::PaintingState() : currentStep(PS_IDLE) {
    // Constructor implementation
}

void PaintingState::enter() {
    Serial.print("PaintingState: enter() called. CurrentStep BEFORE logic: ");
    Serial.println(currentStep); // Log its value *before* the if. Add specific name if possible later.
    
    // Clear any lingering pause state from previous cycles
    isPaused = false;
    Serial.println("PaintingState: Cleared pause state for new painting cycle");
    
    // Check if we are in the special "Paint All Sides" transition
    if (stateMachine && stateMachine->isTransitioningToPaintAllSides()) {
        Serial.println("PaintingState: Detected 'Paint All Sides' transition. Starting with Side 4.");
        currentStep = PS_START_SIDE4_PAINTING; // Start with Side 4
        stateMachine->clearTransitioningToPaintAllSidesFlag(); // Clear the flag as it has been handled
        stateMachine->setInPaintAllSidesMode(true); // Set persistent mode flag
    } else if (currentStep == PS_IDLE) {
        Serial.println("PaintingState: enter() - Normal entry. Starting with Side 4.");
        currentStep = PS_START_SIDE4_PAINTING; // Start with Side 4
    } else {
        Serial.print("PaintingState: enter() - CurrentStep is not IDLE. Preserving currentStep: ");
        Serial.println(currentStep); // Log its value if preserved.
    }
    Serial.print("PaintingState: enter() finished. CurrentStep AFTER logic: ");
    Serial.println(currentStep);
    // Update will handle the rest based on currentStep
}

void PaintingState::update() {
    // Declare variables outside of switch statement
    long xPos, yPos, zPos;
    
    switch (currentStep) {
        case PS_START_SIDE4_PAINTING:
            Serial.println("PaintingState: Starting Side 4 painting");
            paintSide4Pattern(); // This will transition to Side4State
            currentStep = PS_WAIT_FOR_SIDE4_COMPLETION;
            break;
            
        case PS_WAIT_FOR_SIDE4_COMPLETION:
            // Wait for Side4State to complete and return to PaintingState
            // The Side4State will transition back to PaintingState when done
            break;
            
        case PS_START_SIDE3_PAINTING:
            Serial.println("PaintingState: Starting Side 3 painting");
            paintSide3Pattern(); // This will transition to Side3State
            currentStep = PS_WAIT_FOR_SIDE3_COMPLETION;
            break;
            
        case PS_WAIT_FOR_SIDE3_COMPLETION:
            // Wait for Side3State to complete and return to PaintingState
            break;
            
        case PS_START_SIDE2_PAINTING:
            Serial.println("PaintingState: Starting Side 2 painting");
            paintSide2Pattern(); // This will transition to Side2State
            currentStep = PS_WAIT_FOR_SIDE2_COMPLETION;
            break;
            
        case PS_WAIT_FOR_SIDE2_COMPLETION:
            // Wait for Side2State to complete and return to PaintingState
            break;
            
        case PS_START_SIDE1_PAINTING:
            Serial.println("PaintingState: Starting Side 1 painting");
            paintSide1Pattern(); // This will transition to Side1State
            currentStep = PS_WAIT_FOR_SIDE1_COMPLETION;
            break;
            
        case PS_WAIT_FOR_SIDE1_COMPLETION:
            // Wait for Side1State to complete and return to PaintingState
            break;

        case PS_PERFORM_ALL_SIDES_PAINTING:
            Serial.println("PaintingState: Starting all sides painting routine.");
            paintAllSides(); // This is assumed to be a blocking call
            Serial.println("PaintingState: All Sides Painting routine finished.");
            currentStep = PS_MOVE_TO_POSITION_BEFORE_HOMING;
            break;

        case PS_MOVE_TO_POSITION_BEFORE_HOMING:
            Serial.println("PaintingState: Moving to position (1,1,0) before Homing.");
            // Convert inches to steps
            xPos = (long)(1.0 * STEPS_PER_INCH_XYZ);
            yPos = (long)(1.0 * STEPS_PER_INCH_XYZ);
            zPos = 0;
            
            moveToXYZ(xPos, DEFAULT_X_SPEED, yPos, DEFAULT_Y_SPEED, zPos, DEFAULT_Z_SPEED); // Blocking
            Serial.println("PaintingState: Reached position (1,1,0).");
            currentStep = PS_REQUEST_HOMING;
            // Fall through intentionally to PS_REQUEST_HOMING
        
        case PS_REQUEST_HOMING:
            Serial.println("PaintingState: Sequence complete. Requesting Homing State.");
            if (stateMachine) {
                stateMachine->setInPaintAllSidesMode(false); // Clear Paint All Sides mode
                if (stateMachine->getHomingState()) {
                    stateMachine->changeState(stateMachine->getHomingState());
                } else {
                    Serial.println("ERROR: PaintingState - Cannot transition to HomingState.");
                    if (stateMachine->getIdleState()) {
                       stateMachine->changeState(stateMachine->getIdleState());
                    }
                }
            }
            currentStep = PS_IDLE; // Reset for next entry into PaintingState
            break;
        
        case PS_IDLE:
            // Waiting for a new painting command (which would call enter() and reset currentStep)
            break;
    }
}

void PaintingState::exit() {
    Serial.println("Exiting Painting State (All Sides)");
    // setMachineState(MachineState::UNKNOWN); // REMOVED
    paintGun_OFF(); 
    // currentStep = PS_IDLE; // REMOVED: Reset step on exit - This was causing the loop.
                            // currentStep is now reset at the end of the painting sequence within update().
}

const char* PaintingState::getName() const {
    return "PAINTING";
}

// New method to handle side completion callbacks
void PaintingState::onSideCompleted() {
    switch (currentStep) {
        case PS_WAIT_FOR_SIDE4_COMPLETION:
            Serial.println("PaintingState: Side 4 completed, starting Side 3");
            currentStep = PS_START_SIDE3_PAINTING;
            break;
            
        case PS_WAIT_FOR_SIDE3_COMPLETION:
            Serial.println("PaintingState: Side 3 completed, starting Side 2");
            currentStep = PS_START_SIDE2_PAINTING;
            break;
            
        case PS_WAIT_FOR_SIDE2_COMPLETION:
            Serial.println("PaintingState: Side 2 completed, starting Side 1");
            currentStep = PS_START_SIDE1_PAINTING;
            break;
            
        case PS_WAIT_FOR_SIDE1_COMPLETION:
            Serial.println("PaintingState: Side 1 completed, moving to completion");
            currentStep = PS_MOVE_TO_POSITION_BEFORE_HOMING;
            break;
            
        default:
            Serial.println("PaintingState: Unexpected side completion callback");
            break;
    }
}



//* ************************************************************************
//* ************************** PAINTING STATE ****************************
//* ************************************************************************ 