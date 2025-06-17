// ===========================================================================
//                        CLEAN PNP STATE IMPLEMENTATION
// ===========================================================================
// This is a cleaner, more readable version of the PnP functionality
// Uses standalone functions instead of complex class-based state machine

#include <Arduino.h>
#include "utils/settings.h" 
#include "motors/XYZ_Movements.h"
#include "hardware/cylinder_Functions.h"
#include "hardware/vacuum_Functions.h"
#include "system/StateMachine.h"
#include "hardware/GlobalDebouncers.h"
#include "settings/pnp.h"
#include "system/machine_state.h"  // For physicalHomeButtonPressed
#include "hardware/controlPanel_Functions.h"  // For paintAllSidesTwice function

// ===========================================================================
//                              PNP CONFIGURATION  
// ===========================================================================
struct PnPConfig {
    // Grid positions (calculated once)
    float gridX[GRID_ROWS * GRID_COLS];
    float gridY[GRID_ROWS * GRID_COLS];
    
    // Pick location in steps
    long pickX_steps;
    long pickY_steps;
    
    // Current operation state
    int currentPosition;
    int targetPosition;
    bool isFullCycle;
    bool isInitialized;
};

static PnPConfig g_pnp = {0}; // Global PnP configuration

// ===========================================================================
//                              HOME BUTTON CHECKING
// ===========================================================================

/**
 * @brief Check if home button was pressed and handle abort if needed
 * @return true if home button was pressed and operation should abort
 */
bool pnp_checkForHomeButton() {
    extern volatile bool physicalHomeButtonPressed;
    
    if (physicalHomeButtonPressed) {
        Serial.println("PnP: HOME button pressed - aborting PnP operation!");
        
        // Stop all motors immediately
        extern FastAccelStepper* stepperX;
        extern FastAccelStepper* stepperY_Left;
        extern FastAccelStepper* stepperY_Right;
        extern FastAccelStepper* stepperZ;
        
        if (stepperX && stepperX->isRunning()) {
            stepperX->forceStopAndNewPosition(stepperX->getCurrentPosition());
        }
        if (stepperY_Left && stepperY_Left->isRunning()) {
            stepperY_Left->forceStopAndNewPosition(stepperY_Left->getCurrentPosition());
        }
        if (stepperY_Right && stepperY_Right->isRunning()) {
            stepperY_Right->forceStopAndNewPosition(stepperY_Right->getCurrentPosition());
        }
        if (stepperZ && stepperZ->isRunning()) {
            stepperZ->forceStopAndNewPosition(stepperZ->getCurrentPosition());
        }
        
        // Turn off vacuum and retract cylinder for safety
        vacuumOff();
        cylinderUp();
        
        Serial.println("PnP: Motors stopped, vacuum off, cylinder retracted");
        
        // Let the state machine handle the homing transition
        extern StateMachine* stateMachine;
        if (stateMachine) {
            stateMachine->changeState(stateMachine->getHomingState());
            Serial.println("PnP: Transitioning to homing state");
        }
        
        return true;
    }
    
    return false;
}

// ===========================================================================
//                              SENSOR RELEASE FUNCTION
// ===========================================================================

/**
 * @brief Wait for PnP sensor to be released (go HIGH) after placing component
 * This prevents double-triggering by ensuring sensor is cleared before next cycle
 * @return true if sensor was released, false if aborted
 */
bool pnp_waitForSensorRelease() {
    Serial.println("PnP: Waiting for sensor to be released (go HIGH) before next cycle...");
    
    // Check current sensor state first
    g_pnpCycleSensorDebouncer.update();
    if (g_pnpCycleSensorDebouncer.read() == HIGH) {
        Serial.println("PnP: Sensor already released");
        return true;
    }
    
    const unsigned long RELEASE_TIMEOUT_MS = 10000; // 10 second timeout
    unsigned long startTime = millis();
    
    while (true) {
        // Check for timeout
        if (millis() - startTime > RELEASE_TIMEOUT_MS) {
            Serial.println("PnP: WARNING - Sensor did not release within timeout, continuing anyway");
            return true; // Continue anyway to prevent getting stuck
        }
        
        // Check for home button
        if (pnp_checkForHomeButton()) {
            return false; // Abort operation
        }
        
        g_pnpCycleSensorDebouncer.update();
        
        // Wait for sensor to go HIGH (released)
        if (g_pnpCycleSensorDebouncer.read() == HIGH) {
            Serial.println("PnP: Sensor released - ready for next cycle");
            return true;
        }
        
        // Print status every 2 seconds during wait
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint > 2000) {
            unsigned long elapsed = (millis() - startTime) / 1000;
            Serial.printf("PnP: Waiting for sensor release (%lu/%lu sec) - Current value: %d (need HIGH)\n", 
                         elapsed, RELEASE_TIMEOUT_MS/1000, g_pnpCycleSensorDebouncer.read());
            lastPrint = millis();
        }
        
        delay(10); // Small delay to prevent busy waiting
    }
}

// ===========================================================================
//                              CORE PNP FUNCTIONS
// ===========================================================================

void pnp_calculateGridPositions() {
    const float X_SHIFT = 9.4f; // Inches between columns  
    const float Y_SHIFT = 5.0f; // Inches between rows
    
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            int index = row * GRID_COLS + col;
            g_pnp.gridX[index] = GRID_ORIGIN_X - (col * X_SHIFT);
            g_pnp.gridY[index] = GRID_ORIGIN_Y - (row * Y_SHIFT);
        }
    }
    Serial.println("PnP: Grid positions calculated");
}

void pnp_initialize() {
    if (!g_pnp.isInitialized) {
        pnp_calculateGridPositions();
        g_pnp.pickX_steps = (long)(PICK_LOCATION_X * STEPS_PER_INCH_XYZ);
        g_pnp.pickY_steps = (long)(PICK_LOCATION_Y * STEPS_PER_INCH_XYZ);
        g_pnp.isInitialized = true;
        Serial.println("PnP: Initialized");
    }
}

bool pnp_waitForSensor(const char* message = "Waiting for cycle sensor...") {
    Serial.println(message);
    
    // First check if sensor is already active (HIGH) - if so, proceed immediately
    g_pnpCycleSensorDebouncer.update();
    if (g_pnpCycleSensorDebouncer.read() == HIGH) {
        Serial.println("PnP: Sensor is already active (HIGH) - proceeding with cycle immediately");
        return true;
    }
    
    // If sensor is not active, wait for it to become active (go HIGH)
    Serial.println("PnP: Sensor not active, waiting for activation...");
    
    const unsigned long SENSOR_TIMEOUT_MS = 30000; // 30 second timeout
    unsigned long startTime = millis();
    
    while (true) {
        // Check for timeout first
        if (millis() - startTime > SENSOR_TIMEOUT_MS) {
            Serial.println("PnP: TIMEOUT - Sensor did not activate within 30 seconds!");
            Serial.println("PnP: Operation aborted due to sensor timeout");
            return false; // Abort operation due to timeout
        }
        
        // Check for home button - highest priority
        if (pnp_checkForHomeButton()) {
            return false; // Abort operation
        }
        
        g_pnpCycleSensorDebouncer.update();
        
        // Run cycle when sensor reads HIGH (active)
        if (g_pnpCycleSensorDebouncer.read() == HIGH) {
            Serial.println("PnP: Sensor activated (HIGH) - proceeding with cycle");
            return true;
        }
        
        // Print status every 2 seconds during wait
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint > 2000) {
            unsigned long elapsed = (millis() - startTime) / 1000;
            Serial.printf("PnP: Waiting for sensor (%lu/%lu sec) - Current value: %d (need HIGH)\n", 
                         elapsed, SENSOR_TIMEOUT_MS/1000, g_pnpCycleSensorDebouncer.read());
            lastPrint = millis();
        }
        
        delay(10); // Small delay to prevent busy waiting
    }
}

bool pnp_moveToPickLocation(const char* reason = "Moving to pick location") {
    Serial.printf("PnP: %s...\n", reason);
    
    // Check for home button before movement
    if (pnp_checkForHomeButton()) {
        return false; // Abort operation
    }
    
    extern float g_pnp_x_speed, g_pnp_x_accel, g_pnp_y_speed, g_pnp_y_accel;
    
    moveToXYZ(
        g_pnp.pickX_steps, g_pnp_x_speed,
        g_pnp.pickY_steps, g_pnp_y_speed,
        0, DEFAULT_Z_SPEED
    );
    
    Serial.println("PnP: Arrived at pick location");
    return true;
}

bool pnp_pickComponent() {
    Serial.println("PnP: Picking component...");
    
    // Check for home button before each step
    if (pnp_checkForHomeButton()) return false;
    
    cylinderDown();
    delay(PNP_PICK_DELAY_AFTER_CYLINDER_EXTEND);
    
    if (pnp_checkForHomeButton()) return false;
    
    vacuumOn();
    delay(PNP_PICK_DELAY_AFTER_VACUUM_ON + 50);
    
    if (pnp_checkForHomeButton()) return false;
    
    cylinderUp();
    delay(PNP_PICK_DELAY_AFTER_CYLINDER_RETRACT);
    
    Serial.println("PnP: Component picked");
    return true;
}

bool pnp_placeComponent(int position) {
    // Check for home button before starting
    if (pnp_checkForHomeButton()) return false;
    
    // Convert position (1-10) to array index (0-9)
    int gridIndex = position - 1;
    
    if (gridIndex < 0 || gridIndex >= (GRID_ROWS * GRID_COLS)) {
        Serial.printf("ERROR: Invalid position %d\n", position);
        return false;
    }
    
    // Get target coordinates
    float targetX = g_pnp.gridX[gridIndex];
    float targetY = g_pnp.gridY[gridIndex];
    long targetX_steps = (long)(targetX * STEPS_PER_INCH_XYZ);
    long targetY_steps = (long)(targetY * STEPS_PER_INCH_XYZ);
    
    int row = gridIndex / GRID_COLS + 1;
    int col = gridIndex % GRID_COLS;
    const char* colName = (col == LEFT_COLUMN) ? "Left" : "Right";
    
    Serial.printf("PnP: Moving to Row %d %s (position %d)\n", row, colName, position);
    
    // Check for home button before movement
    if (pnp_checkForHomeButton()) return false;
    
    // Move to place location
    moveToXYZ(
        targetX_steps, DEFAULT_X_SPEED,
        targetY_steps, DEFAULT_Y_SPEED,
        0, DEFAULT_Z_SPEED
    );
    
    Serial.println("PnP: Placing component...");
    
    // Check for home button before each step
    if (pnp_checkForHomeButton()) return false;
    
    cylinderDown();
    delay(PNP_PLACE_DELAY_AFTER_CYLINDER_EXTEND);
    
    if (pnp_checkForHomeButton()) return false;
    
    vacuumOff();
    delay(PNP_PLACE_DELAY_AFTER_VACUUM_OFF);
    
    if (pnp_checkForHomeButton()) return false;
    
    cylinderUp();
    delay(PNP_PLACE_DELAY_AFTER_CYLINDER_RETRACT);
    
    Serial.printf("PnP: Component placed at Row %d %s\n", row, colName);
    return true;
}

bool pnp_processSinglePosition(int position) {
    pnp_initialize();
    
    Serial.printf("PnP: Processing position %d\n", position);
    
    // Step 1: Move to pick location
    if (!pnp_moveToPickLocation("Starting PnP cycle")) {
        Serial.println("PnP: Operation aborted during initial move");
        return false;
    }
    
    // Step 2: Wait for sensor activation
    if (!pnp_waitForSensor("Ready to pick - activate sensor")) {
        Serial.println("PnP: Operation aborted during sensor wait");
        return false;
    }
    
    // Step 3: Pick component
    if (!pnp_pickComponent()) {
        Serial.println("PnP: Operation aborted during pick");
        return false;
    }
    
    // Step 4: Place component
    if (!pnp_placeComponent(position)) {
        Serial.println("PnP: Operation aborted during place");
        return false;
    }
    
    // Step 5: Wait for sensor to be released before continuing
    if (!pnp_waitForSensorRelease()) {
        Serial.println("PnP: Operation aborted during sensor release wait");
        return false;
    }
    
    // Step 6: Return to pick location
    if (!pnp_moveToPickLocation("Returning after placement")) {
        Serial.println("PnP: Operation aborted during return move");
        return false;
    }
    
    Serial.printf("PnP: Position %d complete\n", position);
    
    // Automatically start painting all sides twice after PnP completion
    Serial.println("PnP: Starting automatic painting sequence (all sides twice)...");
    paintAllSidesTwice();
    
    return true;
}

// ===========================================================================
//                          SIMPLE PNP FUNCTIONS
// ===========================================================================
// These are the functions you actually call - much simpler!

void startPnPFullCycle() {
    pnp_initialize();
    Serial.println("PnP: Starting full cycle (positions 1-10)");
    
    if (!pnp_moveToPickLocation("Starting full cycle")) {
        Serial.println("PnP: Full cycle aborted during initial move");
        return;
    }
    
    for (int pos = 1; pos <= (GRID_ROWS * GRID_COLS); pos++) {
        Serial.printf("\n=== PnP Full Cycle: Position %d of %d ===\n", pos, GRID_ROWS * GRID_COLS);
        
        // Wait for sensor activation
        if (!pnp_waitForSensor()) {
            Serial.println("PnP: Full cycle aborted during sensor wait");
            return;
        }
        
        // Pick component
        if (!pnp_pickComponent()) {
            Serial.println("PnP: Full cycle aborted during pick");
            return;
        }
        
        // Place component
        if (!pnp_placeComponent(pos)) {
            Serial.println("PnP: Full cycle aborted during place");
            return;
        }
        
        // Wait for sensor to be released before continuing to next position
        if (!pnp_waitForSensorRelease()) {
            Serial.println("PnP: Full cycle aborted during sensor release wait");
            return;
        }
        
        // Return to pick location for next cycle (or completion)
        if (pos < (GRID_ROWS * GRID_COLS)) {
            if (!pnp_moveToPickLocation("Preparing for next position")) {
                Serial.println("PnP: Full cycle aborted during return move");
                return;
            }
        } else {
            if (!pnp_moveToPickLocation("Full cycle complete - final return")) {
                Serial.println("PnP: Full cycle aborted during final return");
                return;
            }
        }
    }
    
    Serial.println("PnP: Full cycle completed!");
    
    // Automatically start painting all sides twice after PnP completion
    Serial.println("PnP: Starting automatic painting sequence (all sides twice)...");
    paintAllSidesTwice();
}

// Individual position functions - now just simple wrappers
void pnpRow1Left()  { pnp_processSinglePosition(1); }
void pnpRow1Right() { pnp_processSinglePosition(2); }
void pnpRow2Left()  { pnp_processSinglePosition(3); }
void pnpRow2Right() { pnp_processSinglePosition(4); }
void pnpRow3Left()  { pnp_processSinglePosition(5); }
void pnpRow3Right() { pnp_processSinglePosition(6); }
void pnpRow4Left()  { pnp_processSinglePosition(7); }
void pnpRow4Right() { pnp_processSinglePosition(8); }
void pnpRow5Left()  { pnp_processSinglePosition(9); }
void pnpRow5Right() { pnp_processSinglePosition(10); }

// Even simpler - direct position access
void pnpPosition(int position) {
    if (position >= 1 && position <= (GRID_ROWS * GRID_COLS)) {
        pnp_processSinglePosition(position);
    } else {
        Serial.printf("ERROR: Invalid position %d. Valid range: 1-%d\n", 
                     position, GRID_ROWS * GRID_COLS);
    }
}

// ===========================================================================
//                          USAGE EXAMPLES
// ===========================================================================
/*
SUPER SIMPLE USAGE:

// Full cycle - process all 10 positions
startPnPFullCycle();

// Individual positions by name
pnpRow3Left();    // Position 5 (Row 3, Left column)
pnpRow2Right();   // Position 4 (Row 2, Right column)

// Direct position number (easiest!)
pnpPosition(5);   // Same as pnpRow3Left()
pnpPosition(4);   // Same as pnpRow2Right()

Grid Layout:
Row 1: [1-Left] [2-Right]
Row 2: [3-Left] [4-Right] 
Row 3: [5-Left] [6-Right]  
Row 4: [7-Left] [8-Right]
Row 5: [9-Left] [10-Right]

HOME BUTTON SUPPORT:
- Press Modifier Right + Action Left to activate home button
- PnP operations will immediately stop all motors, turn off vacuum, retract cylinder
- Machine will transition to homing state automatically
- Home button is checked at every major step in PnP operations
*/ 