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
    
    while (true) {
        g_pnpCycleSensorDebouncer.update();
        
        if (g_pnpCycleSensorDebouncer.read() == LOW) {
            Serial.println("PnP: Sensor activated");
            return true;
        }
        
        // Print status every 500ms during wait
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint > 500) {
            Serial.printf("PnP: Sensor value: %d (waiting for LOW)\n", 
                         g_pnpCycleSensorDebouncer.read());
            lastPrint = millis();
        }
        
        delay(10); // Small delay to prevent busy waiting
    }
}

void pnp_moveToPickLocation(const char* reason = "Moving to pick location") {
    Serial.printf("PnP: %s...\n", reason);
    
    extern float g_pnp_x_speed, g_pnp_x_accel, g_pnp_y_speed, g_pnp_y_accel;
    
    moveToXYZ(
        g_pnp.pickX_steps, g_pnp_x_speed,
        g_pnp.pickY_steps, g_pnp_y_speed,
        0, DEFAULT_Z_SPEED
    );
    
    Serial.println("PnP: Arrived at pick location");
}

void pnp_pickComponent() {
    Serial.println("PnP: Picking component...");
    
    cylinderDown();
    delay(PNP_PICK_DELAY_AFTER_CYLINDER_EXTEND);
    
    vacuumOn();
    delay(PNP_PICK_DELAY_AFTER_VACUUM_ON + 50);
    
    cylinderUp();
    delay(PNP_PICK_DELAY_AFTER_CYLINDER_RETRACT);
    
    Serial.println("PnP: Component picked");
}

void pnp_placeComponent(int position) {
    // Convert position (1-10) to array index (0-9)
    int gridIndex = position - 1;
    
    if (gridIndex < 0 || gridIndex >= (GRID_ROWS * GRID_COLS)) {
        Serial.printf("ERROR: Invalid position %d\n", position);
        return;
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
    
    // Move to place location
    moveToXYZ(
        targetX_steps, DEFAULT_X_SPEED,
        targetY_steps, DEFAULT_Y_SPEED,
        0, DEFAULT_Z_SPEED
    );
    
    Serial.println("PnP: Placing component...");
    
    cylinderDown();
    delay(PNP_PLACE_DELAY_AFTER_CYLINDER_EXTEND);
    
    vacuumOff();
    delay(PNP_PLACE_DELAY_AFTER_VACUUM_OFF);
    
    cylinderUp();
    delay(PNP_PLACE_DELAY_AFTER_CYLINDER_RETRACT);
    
    Serial.printf("PnP: Component placed at Row %d %s\n", row, colName);
}

void pnp_processSinglePosition(int position) {
    pnp_initialize();
    
    Serial.printf("PnP: Processing position %d\n", position);
    
    // Step 1: Move to pick location
    pnp_moveToPickLocation("Starting PnP cycle");
    
    // Step 2: Wait for sensor activation
    pnp_waitForSensor("Ready to pick - activate sensor");
    
    // Step 3: Pick component
    pnp_pickComponent();
    
    // Step 4: Place component
    pnp_placeComponent(position);
    
    // Step 5: Return to pick location
    pnp_moveToPickLocation("Returning after placement");
    
    Serial.printf("PnP: Position %d complete\n", position);
}

// ===========================================================================
//                          SIMPLE PNP FUNCTIONS
// ===========================================================================
// These are the functions you actually call - much simpler!

void startPnPFullCycle() {
    pnp_initialize();
    Serial.println("PnP: Starting full cycle (positions 1-10)");
    
    pnp_moveToPickLocation("Starting full cycle");
    
    for (int pos = 1; pos <= (GRID_ROWS * GRID_COLS); pos++) {
        Serial.printf("\n=== PnP Full Cycle: Position %d of %d ===\n", pos, GRID_ROWS * GRID_COLS);
        
        // Wait for sensor activation
        pnp_waitForSensor();
        
        // Pick component
        pnp_pickComponent();
        
        // Place component
        pnp_placeComponent(pos);
        
        // Return to pick location for next cycle (or completion)
        if (pos < (GRID_ROWS * GRID_COLS)) {
            pnp_moveToPickLocation("Preparing for next position");
        } else {
            pnp_moveToPickLocation("Full cycle complete - final return");
        }
    }
    
    Serial.println("PnP: Full cycle completed!");
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
*/ 