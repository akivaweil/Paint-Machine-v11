#ifndef XYZ_MOVEMENTS_H
#define XYZ_MOVEMENTS_H

#include <Arduino.h>
// Include FastAccelStepper if types are needed here, otherwise forward declare
// #include <FastAccelStepper.h>

// Declare functions defined in XYZ_Movements.cpp
void moveToXYZ(long x, unsigned int xSpeed, long y, unsigned int ySpeed, long z, unsigned int zSpeed);
void checkMotors(); // Function to update debouncers and check limit switches

// New function that checks for home command - returns true if completed, false if aborted
bool moveToXYZ_HomeCheck(long x, unsigned int xSpeed, long y, unsigned int ySpeed, long z, unsigned int zSpeed);

// New function for continuous move with an X-axis trigger
void moveToXYZ_with_X_trigger(long target_x, unsigned int x_speed, long target_y, unsigned int y_speed, long target_z, unsigned int z_speed, long trigger_x_pos, void (*trigger_action)());

// Helper function to move to position (1,1,0) before homing
void moveToPositionOneOneBeforeHoming();

// Potentially add homing function declarations here later
// void homeX();
// void homeY();
// void homeZ();
// void homeAll();

#endif // XYZ_MOVEMENTS_H 