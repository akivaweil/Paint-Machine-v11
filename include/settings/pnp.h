#ifndef SETTINGS_PNP_H
#define SETTINGS_PNP_H

#include "settings/painting.h"

// ==========================================================================
//                     PICK AND PLACE / TRAY PARAMETERS
// ==========================================================================
// Defaults, can be changed via UI/saved settings

// --- Component/Tray Dimensions (inches) ---
#define SQUARE_WIDTH 3.0f                      // Square width
#define TRAY_BORDER_WIDTH 0.25f                // Tray border width
#define TRAY_WIDTH 18.0f                       // Tray width
#define TRAY_HEIGHT 24.0f                      // Tray height

// --- Grid Layout ---
#define GRID_COLS 2                            // Grid columns (was 4, now 2 columns processed at a time)
#define GRID_ROWS 5                            // Grid rows
#define GRID_ORIGIN_X 18.2 + 3.55f - SIDE3_SHIFT_X       // Grid origin X position (shifted by Side 3 shift value)
#define GRID_ORIGIN_Y 32.4f                    // Grid origin Y position (top-right corner)

// --- PnP Position Targeting ---
enum PnPColumn {
    LEFT_COLUMN = 0,
    RIGHT_COLUMN = 1
};

enum PnPRow {
    ROW_1 = 1,  // Start from row 1, skip row 0
    ROW_2 = 2,
    ROW_3 = 3,
    ROW_4 = 4,
    ROW_5 = 5
};

// Helper function to convert row/column to grid position
// Note: Position 0 is skipped, so we start from position 1
inline int getGridPosition(PnPRow row, PnPColumn column) {
    if (row < ROW_1 || row > ROW_5) {
        return -1; // Invalid row
    }
    // Skip position 0, so row 1 starts at position 1
    return ((row - 1) * GRID_COLS) + column + 1;
}

// --- Pick Location (inches) ---
#define PICK_LOCATION_X 16.05f                  // X coordinate for component pickup
#define PICK_LOCATION_Y 2.0f                   // Y coordinate for component pickup

// --- PNP Operation Delays (ms) ---
// Delays for operations at the PICK location
#define PNP_PICK_DELAY_AFTER_CYLINDER_EXTEND 150  // Delay after extending cylinder at pick loc
#define PNP_PICK_DELAY_AFTER_VACUUM_ON 100        // Delay after engaging vacuum at pick loc
#define PNP_PICK_DELAY_AFTER_CYLINDER_RETRACT 150 // Delay after retracting cylinder at pick loc (before moving to place)

// Delays for operations at the PLACE location
#define PNP_PLACE_DELAY_AFTER_CYLINDER_EXTEND 150 // Delay after extending cylinder at place loc
#define PNP_PLACE_DELAY_AFTER_VACUUM_OFF 100      // Delay after disengaging vacuum at place loc
#define PNP_PLACE_DELAY_AFTER_CYLINDER_RETRACT 150// Delay after retracting cylinder at place loc (before next move)

// Timeout between allowed cycle switch presses
#define CYCLE_TIMEOUT 5 // milliseconds

#endif // SETTINGS_PNP_H 