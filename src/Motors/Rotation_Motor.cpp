#include "motors/Rotation_Motor.h"
#include "utils/settings.h"

// Define the global rotation stepper pointer
AccelStepper *rotationStepper = NULL;

//* ************************************************************************
//* ************************* ROTATION MOTOR *************************
//* ************************************************************************

void setupRotationMotor() {
    Serial.println("Initializing Rotation Stepper...");
    
    // Create AccelStepper instance with DRIVER interface (step and direction pins)
    rotationStepper = new AccelStepper(AccelStepper::DRIVER, ROTATION_STEP_PIN, ROTATION_DIR_PIN);
    
    if (rotationStepper) {
        rotationStepper->setMaxSpeed(DEFAULT_ROT_SPEED);
        rotationStepper->setAcceleration(DEFAULT_ROT_ACCEL);
        rotationStepper->setCurrentPosition(0); // Set current position as 0
        Serial.println("Rotation stepper initialized successfully.");
    } else {
        Serial.println("Failed to initialize Rotation stepper!");
        // Handle error appropriately, maybe halt or log
    }
}

/**
 * Rotates the turntable to a specific angle
 * @param angle The target angle in degrees (0-360)
 */
void rotateToAngle(float angle) {
    // Check if rotation stepper is initialized
    if (!rotationStepper) {
        Serial.println("ERROR: Rotation stepper not initialized!");
        return;
    }

    // Get current position and angle
    long currentPosition = rotationStepper->currentPosition();
    float currentAngle = (float)currentPosition / STEPS_PER_DEGREE;

    // Calculate the difference in angle
    float deltaAngle = angle - currentAngle;

    // Normalize the delta angle to the range [-180, 180] to find the shortest path
    while (deltaAngle > 180.0f) deltaAngle -= 360.0f;
    while (deltaAngle <= -180.0f) deltaAngle += 360.0f;

    // Calculate the target position in steps
    long targetPosition = currentPosition + (long)(deltaAngle * STEPS_PER_DEGREE);

    // Debug output
    Serial.print("Rotating from angle ");
    Serial.print(currentAngle);
    Serial.print(" to angle ");
    Serial.print(angle);
    Serial.print(" (delta: ");
    Serial.print(deltaAngle);
    Serial.print(" degrees, ");
    Serial.print((long)(deltaAngle * STEPS_PER_DEGREE));
    Serial.println(" steps)");

    // Move to the target position
    rotationStepper->moveTo(targetPosition);
    
    // Run the stepper until it reaches the target position
    while (rotationStepper->distanceToGo() != 0) {
        rotationStepper->run();
        // Remove delay(1) - it interferes with smooth acceleration
        // Use yield() instead to prevent watchdog timeout on ESP32
        yield();
    }

    Serial.print("Rotation completed. Current position: ");
    Serial.print(rotationStepper->currentPosition());
    Serial.print(" steps (");
    Serial.print((float)rotationStepper->currentPosition() / STEPS_PER_DEGREE);
    Serial.println(" degrees)");
}

/**
 * Set smoother motion parameters to reduce stuttering
 * Call this if experiencing stuttering issues
 */
void setSmoothRotationMotion() {
    if (!rotationStepper) {
        Serial.println("ERROR: Rotation stepper not initialized!");
        return;
    }
    
    // Use gentler acceleration for smoother motion
    rotationStepper->setMaxSpeed(DEFAULT_ROT_SPEED * 0.8f);  // Slightly slower max speed
    rotationStepper->setAcceleration(DEFAULT_ROT_ACCEL * 0.6f);  // Much gentler acceleration
    
    Serial.println("Smooth rotation motion parameters applied");
}

// Implement other rotation-specific functions here if needed 

//* ************************************************************************
//* ****************** MANUAL ROTATION (NON-TRACKING) ********************
//* ************************************************************************
// !!! SPECIAL NOTE: The following functions are for MANUAL ROTATION ONLY !!!
// !!! These functions DO NOT track position changes and should ONLY be !!!
// !!! used for manual websocket commands. This allows operators to !!!
// !!! manually reorient the tray without affecting automatic paint cycles. !!!
// !!! DO NOT CHANGE THIS BEHAVIOR - it's intentional! !!!
// !!! If you need position tracking, use rotateToAngle() instead. !!!

/**
 * Manually rotates the turntable clockwise by exactly 90 degrees
 * WITHOUT updating the internal position tracking.
 * This allows manual reorientation without affecting subsequent paint cycles.
 * 
 * !!! WARNING: This function does NOT track position changes !!!
 * !!! Use ONLY for manual websocket commands !!!
 */
void manualRotateClockwise90() {
    // Check if rotation stepper is initialized
    if (!rotationStepper) {
        Serial.println("ERROR: Rotation stepper not initialized!");
        return;
    }

    Serial.println("Manual CW 90° rotation (non-tracking) - Position will NOT be updated");
    
    // Calculate 90 degrees in steps
    long steps90Degrees = (long)(90.0f * STEPS_PER_DEGREE);
    
    // Get current position (for reference only)
    long currentPosition = rotationStepper->currentPosition();
    float currentAngle = (float)currentPosition / STEPS_PER_DEGREE;
    
    Serial.print("Current tracked position: ");
    Serial.print(currentPosition);
    Serial.print(" steps (");
    Serial.print(currentAngle);
    Serial.println(" degrees)");
    
    // Move 90 degrees clockwise relative to current physical position
    // but DON'T update the stepper's internal position tracking
    rotationStepper->move(steps90Degrees);
    
    // Run the stepper until it reaches the target
    while (rotationStepper->distanceToGo() != 0) {
        rotationStepper->run();
        yield(); // Prevent watchdog timeout
    }
    
    // Reset the stepper's internal position to what it was before the move
    // This makes the move "invisible" to position tracking
    rotationStepper->setCurrentPosition(currentPosition);
    
    Serial.println("Manual CW 90° rotation complete - Position tracking unchanged");
    Serial.print("Tracked position remains: ");
    Serial.print(rotationStepper->currentPosition());
    Serial.print(" steps (");
    Serial.print((float)rotationStepper->currentPosition() / STEPS_PER_DEGREE);
    Serial.println(" degrees)");
}

/**
 * Manually rotates the turntable counter-clockwise by exactly 90 degrees
 * WITHOUT updating the internal position tracking.
 * This allows manual reorientation without affecting subsequent paint cycles.
 * 
 * !!! WARNING: This function does NOT track position changes !!!
 * !!! Use ONLY for manual websocket commands !!!
 */
void manualRotateCounterClockwise90() {
    // Check if rotation stepper is initialized
    if (!rotationStepper) {
        Serial.println("ERROR: Rotation stepper not initialized!");
        return;
    }

    Serial.println("Manual CCW 90° rotation (non-tracking) - Position will NOT be updated");
    
    // Calculate 90 degrees in steps (negative for counter-clockwise)
    long steps90Degrees = (long)(-90.0f * STEPS_PER_DEGREE);
    
    // Get current position (for reference only)
    long currentPosition = rotationStepper->currentPosition();
    float currentAngle = (float)currentPosition / STEPS_PER_DEGREE;
    
    Serial.print("Current tracked position: ");
    Serial.print(currentPosition);
    Serial.print(" steps (");
    Serial.print(currentAngle);
    Serial.println(" degrees)");
    
    // Move 90 degrees counter-clockwise relative to current physical position
    // but DON'T update the stepper's internal position tracking
    rotationStepper->move(steps90Degrees);
    
    // Run the stepper until it reaches the target
    while (rotationStepper->distanceToGo() != 0) {
        rotationStepper->run();
        yield(); // Prevent watchdog timeout
    }
    
    // Reset the stepper's internal position to what it was before the move
    // This makes the move "invisible" to position tracking
    rotationStepper->setCurrentPosition(currentPosition);
    
    Serial.println("Manual CCW 90° rotation complete - Position tracking unchanged");
    Serial.print("Tracked position remains: ");
    Serial.print(rotationStepper->currentPosition());
    Serial.print(" steps (");
    Serial.print((float)rotationStepper->currentPosition() / STEPS_PER_DEGREE);
    Serial.println(" degrees)");
} 