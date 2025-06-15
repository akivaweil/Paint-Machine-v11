#include <Arduino.h>
// #include "system/machine_state.h" // No longer needed
#include "utils/settings.h" 
#include "motors/XYZ_Movements.h"
#include "motors/stepper_globals.h" 
#include "hardware/cylinder_Functions.h"
#include "hardware/vacuum_Functions.h"
#include "motors/ServoMotor.h"
#include "system/StateMachine.h"
// #include "persistence/PnPData.h" // File does not exist
#include "states/IdleState.h" // Include IdleState for transition
#include "motors/Homing.h" // For homeAllAxes if needed, or a HomingState
#include "states/HomingState.h" // Include HomingState
#include "states/PaintingState.h" // ADDED: Include PaintingState for transition
#include "hardware/GlobalDebouncers.h" // For g_pnpCycleSensorDebouncer

// Additional includes from header file
#include <FastAccelStepper.h>
#include "states/State.h" // Assuming a base State class exists
#include "settings/pnp.h" // For PnP enums and position helpers

// Assuming StateMachine is needed for transitions
class StateMachine; 
extern StateMachine* stateMachine; 

// Base PnP State - can be used for full cycle or targeted positions
class PnPState : public State {
public:
    PnPState(); // Full cycle constructor
    PnPState(PnPRow targetRow, PnPColumn targetColumn); // Targeted position constructor
    PnPState(int specificPosition); // Direct position constructor
    
    void enter() override;
    void update() override;
    void exit() override;
    const char* getName() const override;

    // PnP specific methods
    void setTargetPosition(PnPRow row, PnPColumn column);
    void setTargetPosition(int position);
    void setFullCycle(bool enable = true);

private:
    // Grid configuration
    float gridPositionsX[GRID_ROWS * GRID_COLS];
    float gridPositionsY[GRID_ROWS * GRID_COLS];

    // Position tracking
    int currentPnPGridPosition;
    int targetPosition; // -1 for full cycle, specific position for targeted
    bool isFullCycle; // true for full cycle, false for single position
    bool pnpCycleIsComplete;
    long pickLocationX_steps; // Pick location X in steps
    long pickLocationY_steps; // Pick location Y in steps

    // Cycle sensor (direct read)
    // Bounce pnpCycleSensorDebouncer; // REMOVED: Now using global g_pnpCycleSensorDebouncer
    
    // Cycle timeout
    unsigned long lastCycleTime;  // Timestamp of the last cycle completion
    unsigned long cycleTimeoutMs; // Timeout duration for cycles
    unsigned long cycleStartTimeMs; // Start time for the current cycle

    // Simplified state tracking within PnPState
    // 0: Moving to initial pick location
    // 1: Waiting at Pick Location
    // 2: Processing a PnP cycle (blocking Pick->Move->Place)
    // 3: Moving back to Pick Location (non-blocking)
    // 4: PnP Complete, ready for homing/exit transition
    int pnp_step; 

    // Flag to signal that homing is needed after PnP completion
    bool homingNeededAfterPnP; 

    // Private helper methods
    void calculateGridPositions();
    void initializeHardware();
    void moveToPickLocation(bool initialMove = false); // Moves to pick location (non-blocking)
    void process_single_pnp_cycle(); 
    void resetStateAndReturnToIdle(); // Reset state and return to idle

    // Stepper references (assuming they are globally accessible or passed somehow)
    // If they are global like in pnp_State.cpp, we might not need them as members.
    // extern FastAccelStepperEngine engine; // Example if needed
    // extern FastAccelStepper *stepperX, *stepperY_Left, *stepperY_Right; // Example
};

// Simple PnP Functions - Much easier to use than classes!
// These functions create the right PnP state and start it automatically

// Full cycle function
void startPnPFullCycle();

// Individual position functions (skip position 0, start from row 1)
void pnpRow1Left();   // Position 1
void pnpRow1Right();  // Position 2
void pnpRow2Left();   // Position 3
void pnpRow2Right();  // Position 4
void pnpRow3Left();   // Position 5 
void pnpRow3Right();  // Position 6
void pnpRow4Left();   // Position 7
void pnpRow4Right();  // Position 8
void pnpRow5Left();   // Position 9
void pnpRow5Right();  // Position 10

/*
SIMPLE USAGE EXAMPLES:

// Full cycle processing (all positions, skipping position 0)
startPnPFullCycle();

// Target specific positions - just call the function!
pnpRow3Left();    // Your example: row 3, left column
pnpRow2Right();   // Row 2, right column
pnpRow1Left();    // Row 1, left column
pnpRow5Right();   // Row 5, right column

// Grid Layout (5 rows x 2 columns, position 0 is skipped):
// Row 1: [1-Left] [2-Right]
// Row 2: [3-Left] [4-Right] 
// Row 3: [5-Left] [6-Right]  ← pnpRow3Left() targets this
// Row 4: [7-Left] [8-Right]
// Row 5: [9-Left] [10-Right]

// How to use in your code:
// Just call the function you want:
// pnpRow3Left();  // This does row 3, left column automatically!

// No need to create states or manage classes - just simple function calls!
*/

// Reference to the global state machine instance
// extern StateMachine* stateMachine;

// Assume stepper motors are globally accessible as in the original file
extern FastAccelStepperEngine engine;
extern FastAccelStepper *stepperX;
extern FastAccelStepper *stepperY_Left;
extern FastAccelStepper *stepperY_Right;

// External global variables for PNP settings (defined in Web_Dashboard_Commands.cpp)
extern float g_pnp_x_speed;
extern float g_pnp_x_accel;
extern float g_pnp_y_speed;
extern float g_pnp_y_accel;

//* ************************************************************************
//* ************************** PnP STATE **********************************
//* ************************************************************************

// Default constructor for full cycle
PnPState::PnPState() : 
    currentPnPGridPosition(0), 
    targetPosition(-1), // -1 indicates full cycle
    isFullCycle(true), // Full cycle mode
    pnpCycleIsComplete(false), 
    pnp_step(0), 
    lastCycleTime(0),
    pickLocationX_steps(0), // Added for storing pick location
    pickLocationY_steps(0),  // Added for storing pick location
    cycleTimeoutMs(0),
    cycleStartTimeMs(0)
{
    // Constructor: Initialize members. Debouncer needs attach in enter().
    calculateGridPositions();
    // initializeHardware(); // Called in enter() instead

    // Calculate pick location in steps
    pickLocationX_steps = (long)(PICK_LOCATION_X * STEPS_PER_INCH_XYZ);
    pickLocationY_steps = (long)(PICK_LOCATION_Y * STEPS_PER_INCH_XYZ);
    Serial.printf("PnPState Constructed (Full Cycle). Pick Location Steps: X=%ld, Y=%ld\n", pickLocationX_steps, pickLocationY_steps);

    // Don't reset state variables or move here, do it in enter()
}

// Constructor for targeted row/column
PnPState::PnPState(PnPRow targetRow, PnPColumn targetColumn) : 
    currentPnPGridPosition(0), 
    targetPosition(getGridPosition(targetRow, targetColumn)),
    isFullCycle(false), // Single position mode
    pnpCycleIsComplete(false), 
    pnp_step(0), 
    lastCycleTime(0),
    pickLocationX_steps(0),
    pickLocationY_steps(0),
    cycleTimeoutMs(0),
    cycleStartTimeMs(0)
{
    calculateGridPositions();
    
    // Calculate pick location in steps
    pickLocationX_steps = (long)(PICK_LOCATION_X * STEPS_PER_INCH_XYZ);
    pickLocationY_steps = (long)(PICK_LOCATION_Y * STEPS_PER_INCH_XYZ);
    Serial.printf("PnPState Constructed (Targeted: Row %d, Column %d, Position %d). Pick Location Steps: X=%ld, Y=%ld\n", 
                  targetRow, targetColumn, targetPosition, pickLocationX_steps, pickLocationY_steps);
}

// Constructor for specific position
PnPState::PnPState(int specificPosition) : 
    currentPnPGridPosition(0), 
    targetPosition(specificPosition),
    isFullCycle(false), // Single position mode
    pnpCycleIsComplete(false), 
    pnp_step(0), 
    lastCycleTime(0),
    pickLocationX_steps(0),
    pickLocationY_steps(0),
    cycleTimeoutMs(0),
    cycleStartTimeMs(0)
{
    calculateGridPositions();
    
    // Calculate pick location in steps
    pickLocationX_steps = (long)(PICK_LOCATION_X * STEPS_PER_INCH_XYZ);
    pickLocationY_steps = (long)(PICK_LOCATION_Y * STEPS_PER_INCH_XYZ);
    Serial.printf("PnPState Constructed (Direct Position %d). Pick Location Steps: X=%ld, Y=%ld\n", 
                  specificPosition, pickLocationX_steps, pickLocationY_steps);
}

void PnPState::enter() {
    Serial.println("Entering PnP State...");
    // setMachineState(MachineState::PNP); // REMOVED

    // Initialize PnP specific things
    calculateGridPositions();
    initializeHardware();

    // Calculate pick location in steps
    pickLocationX_steps = (long)(PICK_LOCATION_X * STEPS_PER_INCH_XYZ);
    pickLocationY_steps = (long)(PICK_LOCATION_Y * STEPS_PER_INCH_XYZ);
    Serial.printf("Pick Location Steps: X=%ld, Y=%ld\n", pickLocationX_steps, pickLocationY_steps);

    // Reset state variables based on mode
    if (isFullCycle) {
        // Full cycle mode - start from position 1 (skip position 0)
        currentPnPGridPosition = 1;
        Serial.println("PnP Mode: Full Cycle - processing all positions (skipping position 0)");
    } else {
        // Targeted mode - go directly to target position
        currentPnPGridPosition = targetPosition;
        Serial.printf("PnP Mode: Targeted - processing position %d only\n", targetPosition);
        
        // Validate target position (should be 1-10, not 0)
        if (targetPosition < 1 || targetPosition > (GRID_ROWS * GRID_COLS)) {
            Serial.printf("ERROR: Invalid target position %d! Valid range: 1-%d\n", targetPosition, GRID_ROWS * GRID_COLS);
            pnpCycleIsComplete = true;
            pnp_step = 4; // Go to completion state
            return;
        }
    }
    
    pnpCycleIsComplete = false;
    pnp_step = 0; // Start with initial move to pick location

    // --- Initiate FIRST Move to Pick Location (Non-Blocking) ---
    Serial.println("Initiating initial move to pick location...");
    if (isFullCycle) {
        Serial.printf("PnP Grid Configuration: %d rows x %d columns = %d total positions\n", GRID_ROWS, GRID_COLS, GRID_ROWS * GRID_COLS);
    } else {
        int row = targetPosition / GRID_COLS;
        int col = targetPosition % GRID_COLS;
        Serial.printf("PnP Target: Row %d, Column %d (Position %d)\n", row, col, targetPosition);
    }
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

    // Update the switch debouncer at the start of each update cycle
    // pnpCycleSensorDebouncer.update(); // This line is now handled at the top of the function
    // bool sensorJustActivated = pnpCycleSensorDebouncer.fell(); // MODIFIED: Check for falling edge
    
    // Debug: Print sensor value regularly during critical wait phases
    if (pnp_step == 1) { // Only when waiting for sensor activation
        static unsigned long lastDebugTime = 0;
        if (millis() - lastDebugTime > 500) { // Print every 0.5 seconds during this phase
            // MODIFIED: Use debouncer's state and update log message
            Serial.printf("PnP Cycle Sensor (debounced) value in PnPState: %d (Active when LOW)\n", g_pnpCycleSensorDebouncer.read());
            lastDebugTime = millis();
        }
    }
    
    // --- PnP State Machine Logic ---
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
            if (g_pnpCycleSensorDebouncer.read() == LOW) { // MODIFIED: Check direct debounced state (LOW = active)
                Serial.println("Cycle sensor active (LOW) at pick location. Proceeding...");

                // Check if all positions were already completed
                if (currentPnPGridPosition > (GRID_ROWS * GRID_COLS)) {
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
            if (currentPnPGridPosition < 1 || currentPnPGridPosition > (GRID_ROWS * GRID_COLS)) {
                Serial.printf("ERROR: Invalid currentPnPGridPosition before processing cycle: %d\n", currentPnPGridPosition);
                pnpCycleIsComplete = true; // Force exit
                pnp_step = 4;
                break; // Exit switch statement
            }

            // --- Start the Blocking Cycle for the current position ---
            Serial.printf("Starting PnP cycle for position %d...\n", currentPnPGridPosition);
            process_single_pnp_cycle(); // This function blocks until done

            // --- Cycle Finished for this position ---
            if (isFullCycle) {
                // Full cycle mode - increment to next position
                currentPnPGridPosition += 1; // MODIFIED: Increment by 1 for 2-column grid
                Serial.printf("Full Cycle: Completed position, next position is %d.\n", currentPnPGridPosition);
                
                // Check if all positions are completed
                if (currentPnPGridPosition > (GRID_ROWS * GRID_COLS)) {
                    Serial.println("All PnP positions are now completed.");
                    pnpCycleIsComplete = true;
                    // Move back to pick location one last time before completing
                    Serial.println("Moving back to pick location before exiting...");
                    moveToPickLocation(false);
                    pnp_step = 3;
                } else {
                    // Move back to pick location for next cycle
                    Serial.println("Moving back to pick location...");
                    moveToPickLocation(false);
                    pnp_step = 3;
                }
            } else {
                // Targeted mode - single position complete
                Serial.printf("Targeted: Position %d completed.\n", currentPnPGridPosition);
                pnpCycleIsComplete = true;
                // Move back to pick location before completing
                Serial.println("Moving back to pick location before exiting...");
                moveToPickLocation(false);
                pnp_step = 3;
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
            // Logic to transition out is handled at the very beginning of update()
            // This state now mainly exists to ensure the final move back completes
            // before the transition out check happens.
            // MODIFIED: Transition to Homing State instead of relying on other logic or Idle directly
            if (stateMachine) {
                Serial.println("PnP Cycle complete. Transitioning to Homing State.");
                stateMachine->changeState(stateMachine->getHomingState()); // Assuming getHomingState() exists
            } else {
                Serial.println("ERROR: StateMachine pointer is null in PnPState! Cannot transition to Homing.");
                // Fallback or error handling: maybe try to go to Idle or just log heavily
                // For now, let's assume stateMachine is valid and try to go to Idle as a last resort if homing fails.
                // This part might need more robust error handling depending on system design.
                if (stateMachine) stateMachine->changeState(stateMachine->getIdleState());
            }
            break;

        default:
            Serial.printf("ERROR: Invalid pnp_step: %d\n", pnp_step);
            // Attempt recovery? Maybe force to pick location or complete?
            pnpCycleIsComplete = true;
            moveToPickLocation(false); // Try to return to pick
            pnp_step = 3; // Go to return state as a safe default before completing
            break;
    }
}


void PnPState::exit() {
    // setMachineState(MachineState::UNKNOWN); // REMOVED
    Serial.println("Exiting PnP State");
    // Restore speeds if they were changed
    // Turn off any PnP specific indicators
}

const char* PnPState::getName() const {
    if (isFullCycle) {
        return "PnP Full Cycle";
    } else {
        static char nameBuffer[32];
        int row = targetPosition / GRID_COLS;
        int col = targetPosition % GRID_COLS;
        const char* columnName = (col == LEFT_COLUMN) ? "Left" : "Right";
        snprintf(nameBuffer, sizeof(nameBuffer), "PnP Row %d %s", row, columnName);
        return nameBuffer;
    }
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

// --- Private Helper Methods ---

void PnPState::calculateGridPositions() {
    Serial.println("Calculating grid positions...");
    const float X_SHIFT = 9.4f; // Inches between columns (updated for 2-column layout, was 4.7f for 4 columns)
    const float Y_SHIFT = 5.0f;  // Inches between rows
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            int index = row * GRID_COLS + col;
            gridPositionsX[index] = GRID_ORIGIN_X - (col * X_SHIFT);
            gridPositionsY[index] = GRID_ORIGIN_Y - (row * Y_SHIFT);
        }
    }
    Serial.println("Grid positions calculated.");
    // Optional: Print grid for verification
    // for(int i=0; i<GRID_ROWS*GRID_COLS; ++i) {
    //     Serial.printf("Pos %d: (%.2f, %.2f)\n", i, gridPositionsX[i], gridPositionsY[i]); // Fixed printf
    // }
}

void PnPState::initializeHardware() {
    Serial.println("Initializing PnP Hardware...");
    // Initialize cycle sensor pin - comment indicates moved to main setup
    // pinMode(PNP_CYCLE_SENSOR_PIN, INPUT_PULLUP);
    // Serial.println("Cycle sensor pin initialized with internal pull-up for direct read.");

    // Setup debouncer for PNP Cycle Sensor
    // Assumes PNP_CYCLE_SENSOR_PIN is already configured (e.g., INPUT_PULLUP in main setup)
    // pnpCycleSensorDebouncer.attach(PNP_CYCLE_SENSOR_PIN); // REMOVED: Using global debouncer
    // pnpCycleSensorDebouncer.; // REMOVED: Using global debouncer
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
        stepperY_Right->setAcceleration(g_pnp_y_accel); // Assuming Y-axis PNP accel is the same for both
        stepperX->setSpeedInHz(g_pnp_x_speed);
        stepperY_Left->setSpeedInHz(g_pnp_y_speed);
        stepperY_Right->setSpeedInHz(g_pnp_y_speed);   // Assuming Y-axis PNP speed is the same for both

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
         // Don't set pnp_step here, let the update loop handle error transition
         return; // Exit this function early
    }

    // Ensure motors are stopped before starting blocking moves (should be already stopped if coming from wait state)
    if (stepperX && stepperX->isRunning()) stepperX->stopMove();
    if (stepperY_Left && stepperY_Left->isRunning()) stepperY_Left->stopMove();
    if (stepperY_Right && stepperY_Right->isRunning()) stepperY_Right->stopMove();

    Serial.printf("Processing PnP position %d (%d of %d)\n",
                  currentPnPGridPosition, currentPnPGridPosition, GRID_ROWS * GRID_COLS);

    //! STEP 1: Confirm already at pick location (or move if somehow drifted - should not happen in normal flow)
    // Since we always return to pick location now, this move *should* be instantaneous or very small
    // Can be removed if confident state machine always ensures starting at pick location
    Serial.println("Verifying at pick location...");
     moveToXYZ(
         pickLocationX_steps, DEFAULT_X_SPEED,
         pickLocationY_steps, DEFAULT_Y_SPEED,
         0, DEFAULT_Z_SPEED // Z assumed at home/0
     );
    Serial.println("Confirmed at pick location.");


    //! STEP 2: Extend cylinder, activate vacuum, retract cylinder
    Serial.println("Extending cylinder...");
    cylinderDown();
    delay(PNP_PICK_DELAY_AFTER_CYLINDER_EXTEND); // CORRECTED
    Serial.println("Activating vacuum...");
    vacuumOn();
    delay(PNP_PICK_DELAY_AFTER_VACUUM_ON + 50); // CORRECTED (and kept +50)
    Serial.println("Retracting cylinder...");
    cylinderUp();
    delay(PNP_PICK_DELAY_AFTER_CYLINDER_RETRACT); // CORRECTED
    Serial.println("Pick sequence complete.");

    //! STEP 3: Get target grid position coordinates
    // Convert position (1-10) to grid array index (0-9)
    int gridIndex = currentPnPGridPosition - 1;
    float targetX_inch = gridPositionsX[gridIndex];
    float targetY_inch = gridPositionsY[gridIndex];
    long targetX_steps = (long)(targetX_inch * STEPS_PER_INCH_XYZ);
    long targetY_steps = (long)(targetY_inch * STEPS_PER_INCH_XYZ);

    //! STEP 4: Move to place position (Blocking)
    int row = gridIndex / GRID_COLS;
    int col = gridIndex % GRID_COLS;
    Serial.printf("Moving to grid position [%d][%d] (position %d): X=%.2f (%ld steps), Y=%.2f (%ld steps)\n",
                  row + 1, col, currentPnPGridPosition, targetX_inch, targetX_steps, targetY_inch, targetY_steps);

    float y_speed_for_placement = DEFAULT_Y_SPEED; // Initialize with default Y speed
    int total_positions = GRID_ROWS * GRID_COLS;

    // Check if the current PnP position is the last one that will be processed before completion.
    // Completion occurs when (currentPnPGridPosition_after_increment > total_positions).
    // So, if (currentPnPGridPosition + 1 > total_positions), this is the last cycle.
    bool isThisTheFinalPlacement = ((currentPnPGridPosition + 1) > total_positions);

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

    //! STEP 5: Extend cylinder, deactivate vacuum, retract cylinder
    Serial.println("Extending cylinder to place component...");
    cylinderDown();
    delay(PNP_PLACE_DELAY_AFTER_CYLINDER_EXTEND); // CORRECTED
    Serial.println("Deactivating vacuum...");
    vacuumOff();
    delay(PNP_PLACE_DELAY_AFTER_VACUUM_OFF); // CORRECTED
    Serial.println("Retracting cylinder...");
    cylinderUp();
    delay(PNP_PLACE_DELAY_AFTER_CYLINDER_RETRACT); // CORRECTED

    Serial.println("Place sequence complete. Cycle finished, machine is at Place Location.");
    Serial.println("------------------------------------");
    // NOTE: Function ends here. The machine is currently at the place location.
    // The state machine (update loop) will now initiate the move back to the pick location.
}

// Setter methods for changing target during runtime
void PnPState::setTargetPosition(PnPRow row, PnPColumn column) {
    targetPosition = getGridPosition(row, column);
    isFullCycle = false;
    Serial.printf("PnP Target updated to Row %d, Column %d (Position %d)\n", row, column, targetPosition);
}

void PnPState::setTargetPosition(int position) {
    if (position >= 0 && position < (GRID_ROWS * GRID_COLS)) {
        targetPosition = position;
        isFullCycle = false;
        int row = position / GRID_COLS;
        int col = position % GRID_COLS;
        Serial.printf("PnP Target updated to Position %d (Row %d, Column %d)\n", position, row, col);
    } else {
        Serial.printf("ERROR: Invalid position %d! Valid range: 0-%d\n", position, (GRID_ROWS * GRID_COLS - 1));
    }
}

void PnPState::setFullCycle(bool enable) {
    isFullCycle = enable;
    if (enable) {
        targetPosition = -1;
        Serial.println("PnP Mode set to Full Cycle");
    } else {
        Serial.println("PnP Mode set to Targeted (use setTargetPosition to specify)");
    }
}

// Simple PnP Functions - Much easier to use than classes!
// These functions create the right PnP state and start it automatically

void startPnPFullCycle() {
    if (stateMachine) {
        Serial.println("Starting PnP Full Cycle");
        stateMachine->changeState(new PnPState());
    } else {
        Serial.println("ERROR: StateMachine not available for PnP Full Cycle");
    }
}

void pnpRow1Left() {
    if (stateMachine) {
        Serial.println("Starting PnP Row 1 Left");
        stateMachine->changeState(new PnPState(ROW_1, LEFT_COLUMN));
    } else {
        Serial.println("ERROR: StateMachine not available for PnP Row 1 Left");
    }
}

void pnpRow1Right() {
    if (stateMachine) {
        Serial.println("Starting PnP Row 1 Right");
        stateMachine->changeState(new PnPState(ROW_1, RIGHT_COLUMN));
    } else {
        Serial.println("ERROR: StateMachine not available for PnP Row 1 Right");
    }
}

void pnpRow2Left() {
    if (stateMachine) {
        Serial.println("Starting PnP Row 2 Left");
        stateMachine->changeState(new PnPState(ROW_2, LEFT_COLUMN));
    } else {
        Serial.println("ERROR: StateMachine not available for PnP Row 2 Left");
    }
}

void pnpRow2Right() {
    if (stateMachine) {
        Serial.println("Starting PnP Row 2 Right");
        stateMachine->changeState(new PnPState(ROW_2, RIGHT_COLUMN));
    } else {
        Serial.println("ERROR: StateMachine not available for PnP Row 2 Right");
    }
}

void pnpRow3Left() {
    if (stateMachine) {
        Serial.println("Starting PnP Row 3 Left");
        stateMachine->changeState(new PnPState(ROW_3, LEFT_COLUMN));
    } else {
        Serial.println("ERROR: StateMachine not available for PnP Row 3 Left");
    }
}

void pnpRow3Right() {
    if (stateMachine) {
        Serial.println("Starting PnP Row 3 Right");
        stateMachine->changeState(new PnPState(ROW_3, RIGHT_COLUMN));
    } else {
        Serial.println("ERROR: StateMachine not available for PnP Row 3 Right");
    }
}

void pnpRow4Left() {
    if (stateMachine) {
        Serial.println("Starting PnP Row 4 Left");
        stateMachine->changeState(new PnPState(ROW_4, LEFT_COLUMN));
    } else {
        Serial.println("ERROR: StateMachine not available for PnP Row 4 Left");
    }
}

void pnpRow4Right() {
    if (stateMachine) {
        Serial.println("Starting PnP Row 4 Right");
        stateMachine->changeState(new PnPState(ROW_4, RIGHT_COLUMN));
    } else {
        Serial.println("ERROR: StateMachine not available for PnP Row 4 Right");
    }
}

void pnpRow5Left() {
    if (stateMachine) {
        Serial.println("Starting PnP Row 5 Left");
        stateMachine->changeState(new PnPState(ROW_5, LEFT_COLUMN));
    } else {
        Serial.println("ERROR: StateMachine not available for PnP Row 5 Left");
    }
}

void pnpRow5Right() {
    if (stateMachine) {
        Serial.println("Starting PnP Row 5 Right");
        stateMachine->changeState(new PnPState(ROW_5, RIGHT_COLUMN));
    } else {
        Serial.println("ERROR: StateMachine not available for PnP Row 5 Right");
    }
}

// Function declarations for use by other files (replaces the deleted header file)
// These functions can be called from anywhere in the project
extern void startPnPFullCycle();
extern void pnpRow1Left();
extern void pnpRow1Right();
extern void pnpRow2Left();
extern void pnpRow2Right();
extern void pnpRow3Left();
extern void pnpRow3Right();
extern void pnpRow4Left();
extern void pnpRow4Right();
extern void pnpRow5Left();
extern void pnpRow5Right();