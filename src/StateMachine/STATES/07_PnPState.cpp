#include "states/PnPState.h"
#include <Arduino.h>
#include "utils/settings.h" 
#include "motors/XYZ_Movements.h"
#include "motors/stepper_globals.h" 
#include "hardware/cylinder_Functions.h"
#include "hardware/vacuum_Functions.h"
#include "motors/ServoMotor.h"
#include "system/StateMachine.h"
#include "states/IdleState.h"
#include "motors/Homing.h"
#include "states/HomingState.h"
#include "states/PaintingState.h"
#include "hardware/GlobalDebouncers.h"
#include "hardware/controlPanel_Functions.h"

// Reference to the global state machine instance
extern StateMachine* stateMachine; 

// Stepper motors are globally accessible
extern FastAccelStepperEngine engine;
extern FastAccelStepper *stepperX;
extern FastAccelStepper *stepperY_Left;
extern FastAccelStepper *stepperY_Right;

// External global variables for PNP settings
extern float g_pnp_x_speed;
extern float g_pnp_x_accel;
extern float g_pnp_y_speed;
extern float g_pnp_y_accel;

//* ************************************************************************
//* ************************** PnP STATE **********************************
//* ************************************************************************

PnPState::PnPState() : 
    currentPnPGridPosition(0), 
    pnpCycleIsComplete(false), 
    pnp_step(0), 
    lastCycleTime(0),
    pickLocationX_steps(0),
    pickLocationY_steps(0),
    cycleTimeoutMs(0),
    cycleStartTimeMs(0)
{
    // Constructor: Initialize members
    calculateGridPositions();

    // Calculate pick location in steps
    pickLocationX_steps = (long)(PICK_LOCATION_X * STEPS_PER_INCH_XYZ);
    pickLocationY_steps = (long)(PICK_LOCATION_Y * STEPS_PER_INCH_XYZ);
    Serial.printf("PnPState Constructed. Pick Location Steps: X=%ld, Y=%ld\n", pickLocationX_steps, pickLocationY_steps);
}

void PnPState::enter() {
    Serial.println("Entering PnP State...");

    // Initialize PnP specific things
    calculateGridPositions();
    initializeHardware();

    // Calculate pick location in steps
    pickLocationX_steps = (long)(PICK_LOCATION_X * STEPS_PER_INCH_XYZ);
    pickLocationY_steps = (long)(PICK_LOCATION_Y * STEPS_PER_INCH_XYZ);
    Serial.printf("Pick Location Steps: X=%ld, Y=%ld\n", pickLocationX_steps, pickLocationY_steps);

    // Reset state variables - start at position 1 (skip position 0)
    currentPnPGridPosition = 1;
    pnpCycleIsComplete = false;
    pnp_step = 0; // Start with initial move to pick location

    //! ************************************************************************
    //! STEP 1: INITIATE FIRST MOVE TO PICK LOCATION (NON-BLOCKING)
    //! ************************************************************************
    Serial.println("Initiating initial move to pick location...");
    moveToPickLocation(true); // Mark as initial move, starts non-blocking move

    Serial.println("PnP State Setup Complete. Initial move started.");
}

void PnPState::update() {
    // Main update loop for PnP state
    
    g_pnpCycleSensorDebouncer.update(); // Update the global debouncer each cycle

    // Check if the cycle has a timeout set and has exceeded it
    if (cycleTimeoutMs > 0 && (millis() - cycleStartTimeMs > cycleTimeoutMs)) {
        Serial.println("PnP cycle timeout exceeded! Returning to idle state.");
        resetStateAndReturnToIdle();
        return;
    }

    // Debug: Print sensor value regularly during critical wait phases
    if (pnp_step == 1) { // Only when waiting for sensor activation
        static unsigned long lastDebugTime = 0;
        if (millis() - lastDebugTime > 500) { // Print every 0.5 seconds during this phase
            Serial.printf("PnP Cycle Sensor (debounced) value in PnPState: %d (Active when LOW)\n", g_pnpCycleSensorDebouncer.read());
            lastDebugTime = millis();
        }
    }
    
    //! ************************************************************************
    //! PnP STATE MACHINE LOGIC
    //! ************************************************************************
    switch (pnp_step) {
        case 0: // Moving to initial pick location
            if (stepperX && stepperY_Left && stepperY_Right && // Ensure steppers are valid
                !stepperX->isRunning() &&
                !stepperY_Left->isRunning() &&
                !stepperY_Right->isRunning())
            {
                Serial.println("Initial move complete. Reached pick location.");
                Serial.println("Now waiting for cycle sensor activation...");
                pnp_step = 1; // Transition to waiting state
            }
            // Otherwise, still moving, do nothing else this loop iteration
            break;

        case 1: // Waiting at Pick Location for Sensor Activation
            // Sensor is active LOW. We need to check if it *is* LOW.
            if (g_pnpCycleSensorDebouncer.read() == LOW) {
                Serial.println("Cycle sensor active (LOW) at pick location. Proceeding...");

                // Check if all positions were already completed
                if (currentPnPGridPosition >= (GRID_ROWS * GRID_COLS)) {
                    Serial.println("All positions already completed. Setting complete flag.");
                    pnpCycleIsComplete = true;
                    pnp_step = 4; // Go to completion state
                } else {
                    Serial.printf("Proceeding to process cycle for position %d.\n", currentPnPGridPosition);
                    pnp_step = 2; // Move to processing state
                }
            }
            // If switch not pressed, do nothing, stay in step 1.
            break;

        case 2: // Process Single PnP Cycle (Blocking Pick->Move->Place)
            // Check bounds just in case before processing
            if (currentPnPGridPosition < 0 || currentPnPGridPosition >= (GRID_ROWS * GRID_COLS)) {
                Serial.printf("ERROR: Invalid currentPnPGridPosition before processing cycle: %d\n", currentPnPGridPosition);
                pnpCycleIsComplete = true; // Force exit
                pnp_step = 4;
                break; // Exit switch statement
            }

            //! ************************************************************************
            //! START THE BLOCKING CYCLE FOR THE CURRENT POSITION
            //! ************************************************************************
            Serial.printf("Starting PnP cycle for position %d...\n", currentPnPGridPosition);
            process_single_pnp_cycle(); // This function blocks until done

            //! ************************************************************************
            //! CYCLE FINISHED FOR THIS POSITION
            //! ************************************************************************
            currentPnPGridPosition += 2; // Increment by 2 to process every other square
            Serial.printf("Cycle actions complete. Next logical position is %d.\n", currentPnPGridPosition);

            //! ************************************************************************
            //! DECISION POINT: CHECK IF ALL POSITIONS ARE NOW COMPLETED
            //! ************************************************************************
            if (currentPnPGridPosition >= (GRID_ROWS * GRID_COLS)) {
                Serial.println("All PnP positions are now completed.");
                pnpCycleIsComplete = true;
                // Move back to pick location one last time (non-blocking) before completing
                Serial.println("Moving back to pick location before exiting...");
                moveToPickLocation(false); // Initiate non-blocking move back to pick
                pnp_step = 3; // Go to 'returning to pick' state before the final completion state
            } else {
                // Move back to pick location to wait for next cycle sensor activation
                Serial.println("Moving back to pick location...");
                moveToPickLocation(false); // Initiate non-blocking move back to pick
                pnp_step = 3; // Set state to 'moving back to pick location'
            }
            break; // End case 2

        case 3: // Moving back to pick location (after cycle completion or before final exit)
             if (stepperX && stepperY_Left && stepperY_Right && // Ensure steppers are valid
                !stepperX->isRunning() &&
                !stepperY_Left->isRunning() &&
                !stepperY_Right->isRunning())
            {
                if (pnpCycleIsComplete) {
                     Serial.println("Returned to pick location. PnP process complete.");
                     pnp_step = 4; // Transition to completion state
                } else {
                     Serial.println("Returned to pick location.");
                     Serial.println("Now waiting for cycle sensor activation for next position...");
                     pnp_step = 1; // Transition back to waiting state
                }
            }
            // Otherwise, still moving, do nothing else this loop iteration
            break;

        case 4: // Completion state
            //! ************************************************************************
            //! PnP CYCLE COMPLETE - START AUTOMATIC PAINTING SEQUENCE
            //! ************************************************************************
            Serial.println("PnP Cycle complete. Starting automatic painting sequence (all sides twice)...");
            paintAllSidesTwice();
            
            // Transition to Homing State
            if (stateMachine) {
                Serial.println("Transitioning to Homing State after PnP completion.");
                stateMachine->changeState(stateMachine->getHomingState());
            } else {
                Serial.println("ERROR: StateMachine pointer is null in PnPState! Cannot transition to Homing.");
                if (stateMachine) stateMachine->changeState(stateMachine->getIdleState());
            }
            break;

        default:
            Serial.printf("ERROR: Invalid pnp_step: %d\n", pnp_step);
            // Attempt recovery
            pnpCycleIsComplete = true;
            moveToPickLocation(false); // Try to return to pick
            pnp_step = 3; // Go to return state as a safe default before completing
            break;
    }
}

void PnPState::exit() {
    Serial.println("Exiting PnP State");
    // Turn off any PnP specific indicators
    // Restore speeds if they were changed
}

const char* PnPState::getName() const {
    return "PNP";
}

// Reset state and return to idle
void PnPState::resetStateAndReturnToIdle() {
    // Stop any running motors
    if (stepperX && stepperX->isRunning()) stepperX->stopMove();
    if (stepperY_Left && stepperY_Left->isRunning()) stepperY_Left->stopMove();
    if (stepperY_Right && stepperY_Right->isRunning()) stepperY_Right->stopMove();
    
    // Reset hardware to safe state
    vacuumOff();
    cylinderUp();
    
    // Reset internal state
    pnp_step = 0;
    pnpCycleIsComplete = false;
    
    // Transition to idle
    if (stateMachine) {
        stateMachine->changeState(stateMachine->getIdleState());
    } else {
        Serial.println("ERROR: StateMachine pointer is null in PnPState! Cannot return to idle.");
    }
}

//* ************************************************************************
//* ************************ PRIVATE HELPER METHODS **********************
//* ************************************************************************

void PnPState::calculateGridPositions() {
    Serial.println("Calculating grid positions...");
    const float X_SHIFT = 9.4f; // Inches between columns (updated from old 4.7f)
    const float Y_SHIFT = 5.0f;  // Inches between rows
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            int index = row * GRID_COLS + col;
            gridPositionsX[index] = GRID_ORIGIN_X - (col * X_SHIFT);
            gridPositionsY[index] = GRID_ORIGIN_Y - (row * Y_SHIFT);
        }
    }
    Serial.println("Grid positions calculated.");
}

void PnPState::initializeHardware() {
    Serial.println("Initializing PnP Hardware...");
    Serial.println("PNP Cycle Sensor debouncer is now global and initialized in Setup.");

    // Initialize hardware to safe state
    vacuumOff();
    cylinderUp();
    Serial.println("Vacuum off, Cylinder up.");
}

// Initiates a non-blocking move to the pre-calculated pick location
void PnPState::moveToPickLocation(bool initialMove) {
    Serial.print(initialMove ? "Initiating initial move" : "Moving back");
    Serial.println(" to pick location...");
    
    if (stepperX && stepperY_Left && stepperY_Right) {
        // Use PNP specific speeds and accelerations
        stepperX->setAcceleration(g_pnp_x_accel);
        stepperY_Left->setAcceleration(g_pnp_y_accel);
        stepperY_Right->setAcceleration(g_pnp_y_accel);
        stepperX->setSpeedInHz(g_pnp_x_speed);
        stepperY_Left->setSpeedInHz(g_pnp_y_speed);
        stepperY_Right->setSpeedInHz(g_pnp_y_speed);

        Serial.printf("PNP Speeds: X=%.0f, Y=%.0f\n", g_pnp_x_speed, g_pnp_y_speed);
        Serial.printf("PNP Accels: X=%.0f, Y=%.0f\n", g_pnp_x_accel, g_pnp_y_accel);

        stepperX->moveTo(pickLocationX_steps);
        stepperY_Left->moveTo(pickLocationY_steps); 
        stepperY_Right->moveTo(pickLocationY_steps); 
        
        pnp_step = initialMove ? 0 : 3; // Set state to 'moving to pick'
    } else {
        Serial.println("ERROR: Steppers not initialized! Cannot move to pick location.");
        pnpCycleIsComplete = true; // Prevent operation
        pnp_step = 4; // Go directly to 'complete' state to signal error/need for reset
    }
}

// Performs actions for the current 'currentPnPGridPosition' (BLOCKING)
void PnPState::process_single_pnp_cycle() {
    // Check bounds just in case
    if (currentPnPGridPosition < 0 || currentPnPGridPosition >= (GRID_ROWS * GRID_COLS)) {
         Serial.printf("ERROR: Invalid currentPnPGridPosition in process_single_pnp_cycle: %d\n", currentPnPGridPosition);
         pnpCycleIsComplete = true; // Force exit in update loop
         return; // Exit this function early
    }

    // Ensure motors are stopped before starting blocking moves
    if (stepperX && stepperX->isRunning()) stepperX->stopMove();
    if (stepperY_Left && stepperY_Left->isRunning()) stepperY_Left->stopMove();
    if (stepperY_Right && stepperY_Right->isRunning()) stepperY_Right->stopMove();

    Serial.printf("Processing PnP position %d (%d of %d)\n",
                  currentPnPGridPosition, currentPnPGridPosition + 1, GRID_ROWS * GRID_COLS);

    //! ************************************************************************
    //! STEP 1: CONFIRM ALREADY AT PICK LOCATION
    //! ************************************************************************
    Serial.println("Verifying at pick location...");
     moveToXYZ(
         pickLocationX_steps, DEFAULT_X_SPEED,
         pickLocationY_steps, DEFAULT_Y_SPEED,
         0, DEFAULT_Z_SPEED // Z assumed at home/0
     );
    Serial.println("Confirmed at pick location.");

    //! ************************************************************************
    //! STEP 2: EXTEND CYLINDER, ACTIVATE VACUUM, RETRACT CYLINDER
    //! ************************************************************************
    Serial.println("Extending cylinder...");
    cylinderDown();
    delay(PNP_PICK_DELAY_AFTER_CYLINDER_EXTEND);
    Serial.println("Activating vacuum...");
    vacuumOn();
    delay(PNP_PICK_DELAY_AFTER_VACUUM_ON + 50);
    Serial.println("Retracting cylinder...");
    cylinderUp();
    delay(PNP_PICK_DELAY_AFTER_CYLINDER_RETRACT);
    Serial.println("Pick sequence complete.");

    //! ************************************************************************
    //! STEP 3: GET TARGET GRID POSITION COORDINATES
    //! ************************************************************************
    float targetX_inch = gridPositionsX[currentPnPGridPosition];
    float targetY_inch = gridPositionsY[currentPnPGridPosition];
    long targetX_steps = (long)(targetX_inch * STEPS_PER_INCH_XYZ);
    long targetY_steps = (long)(targetY_inch * STEPS_PER_INCH_XYZ);

    //! ************************************************************************
    //! STEP 4: MOVE TO PLACE POSITION (BLOCKING)
    //! ************************************************************************
    int row = currentPnPGridPosition / GRID_COLS;
    int col = currentPnPGridPosition % GRID_COLS;
    Serial.printf("Moving to grid position [%d][%d] (%d): X=%.2f (%ld steps), Y=%.2f (%ld steps)\n",
                  row, col, currentPnPGridPosition, targetX_inch, targetX_steps, targetY_inch, targetY_steps);

    float y_speed_for_placement = DEFAULT_Y_SPEED; // Initialize with default Y speed
    int total_positions = GRID_ROWS * GRID_COLS;

    // Check if the current PnP position is the last one that will be processed before completion.
    bool isThisTheFinalPlacement = ((currentPnPGridPosition + 2) >= total_positions);

    if (isThisTheFinalPlacement) {
        Serial.println("INFO: This is the final PnP placement. Reducing Y speed by half for this move.");
        y_speed_for_placement = DEFAULT_Y_SPEED / 2.0f;
        if (y_speed_for_placement < 1.0f) { // Safeguard against excessively slow speed
            y_speed_for_placement = 1.0f;
            Serial.println("WARN: Calculated Y speed for final placement is very low. Clamped to 1.0 Hz.");
        }
    }

    moveToXYZ(
        targetX_steps, DEFAULT_X_SPEED,
        targetY_steps, y_speed_for_placement, // Use potentially modified Y speed
        0, DEFAULT_Z_SPEED // Z assumed at home/0
    );
     Serial.println("Arrived at place location.");

    //! ************************************************************************
    //! STEP 5: EXTEND CYLINDER, DEACTIVATE VACUUM, RETRACT CYLINDER
    //! ************************************************************************
    Serial.println("Extending cylinder to place component...");
    cylinderDown();
    delay(PNP_PLACE_DELAY_AFTER_CYLINDER_EXTEND);
    Serial.println("Deactivating vacuum...");
    vacuumOff();
    delay(PNP_PLACE_DELAY_AFTER_VACUUM_OFF);
    Serial.println("Retracting cylinder...");
    cylinderUp();
    delay(PNP_PLACE_DELAY_AFTER_CYLINDER_RETRACT);

    Serial.println("Place sequence complete. Cycle finished, machine is at Place Location.");
    Serial.println("------------------------------------");
} 