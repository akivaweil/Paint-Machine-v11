#ifndef PNP_FUNCTIONS_H
#define PNP_FUNCTIONS_H

// ===========================================================================
//                         SIMPLE PNP FUNCTIONS
// ===========================================================================
// Clean, easy-to-use PnP functions that replace the complex state machine

// Full cycle function - processes all positions 1-10
void startPnPFullCycle();

// Individual position functions (much easier than classes!)
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

// Direct position access - even simpler!
void pnpPosition(int position); // position 1-10

// Core PnP functions (for advanced use)
void pnp_initialize();
void pnp_processSinglePosition(int position);

/*
USAGE EXAMPLES:

// Full cycle
startPnPFullCycle();

// Specific positions by name
pnpRow3Left();    // Row 3, Left column
pnpRow2Right();   // Row 2, Right column

// Direct position (easiest!)
pnpPosition(5);   // Same as pnpRow3Left()
pnpPosition(4);   // Same as pnpRow2Right()

Grid Layout:
Row 1: [1-Left] [2-Right]
Row 2: [3-Left] [4-Right] 
Row 3: [5-Left] [6-Right]  
Row 4: [7-Left] [8-Right]
Row 5: [9-Left] [10-Right]
*/

#endif // PNP_FUNCTIONS_H 