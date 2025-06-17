#ifndef PNPSTATE_H
#define PNPSTATE_H

#include "states/State.h"
#include "settings/pnp.h"
#include "utils/settings.h"

// Forward declarations
class StateMachine;

//* ************************************************************************
//* ************************** PnP STATE CLASS ****************************
//* ************************************************************************

class PnPState : public State {
public:
    PnPState();
    
    void enter() override;
    void update() override;
    void exit() override;
    const char* getName() const override;
    
    // Reset state and return to idle
    void resetStateAndReturnToIdle();

private:
    // --- Grid Position Storage ---
    float gridPositionsX[GRID_ROWS * GRID_COLS];
    float gridPositionsY[GRID_ROWS * GRID_COLS];
    
    // --- Pick Location in Steps ---
    long pickLocationX_steps;
    long pickLocationY_steps;
    
    // --- State Variables ---
    int currentPnPGridPosition;    // Current position in grid (0-based index)
    bool pnpCycleIsComplete;       // Flag indicating if all cycles are done
    int pnp_step;                  // Current step in PnP state machine
    unsigned long lastCycleTime;   // Timestamp of last cycle completion
    
    // --- Timeout Handling ---
    unsigned long cycleTimeoutMs;     // Timeout for current cycle
    unsigned long cycleStartTimeMs;   // Start time of current cycle
    
    // --- Private Helper Methods ---
    void calculateGridPositions();
    void initializeHardware();
    void moveToPickLocation(bool initialMove = false);
    void process_single_pnp_cycle();
};

// Reference to the global state machine instance
extern StateMachine* stateMachine;

#endif // PNPSTATE_H 