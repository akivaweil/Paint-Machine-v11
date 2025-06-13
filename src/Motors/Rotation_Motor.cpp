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
        // Small delay to prevent watchdog timeout
        delay(1);
    }

    Serial.print("Rotation completed. Current position: ");
    Serial.print(rotationStepper->currentPosition());
    Serial.print(" steps (");
    Serial.print((float)rotationStepper->currentPosition() / STEPS_PER_DEGREE);
    Serial.println(" degrees)");
}

// Implement other rotation-specific functions here if needed 