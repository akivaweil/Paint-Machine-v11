#ifndef ROTATION_MOTOR_H
#define ROTATION_MOTOR_H

#include <AccelStepper.h>
#include <Arduino.h>
#include "settings.h" // Include settings to get the defines

// // Define GPIO pins for the rotation motor - MOVED TO settings.h
// // !!! UPDATE THESE VALUES IF THEY ARE DIFFERENT !!!
// #define ROTATION_STEP_PIN 25 // Example pin
// #define ROTATION_DIR_PIN 26  // Example pin

// // Define default speed and acceleration for the rotation motor - MOVED TO settings.h
// // !!! UPDATE THESE VALUES AS NEEDED !!!
// #define DEFAULT_ROT_SPEED 5000 // Example speed in Hz
// #define DEFAULT_ROT_ACCEL 10000 // Example acceleration in steps/s^2


// Declare the rotation stepper motor object pointer
extern AccelStepper *rotationStepper;

/**
 * @brief Initializes the rotation stepper motor.
 * Creates the AccelStepper instance with step and direction pins.
 * Sets default speed and acceleration.
 */
void setupRotationMotor();

/**
 * @brief Rotates to a specific angle.
 * @param angle The target angle in degrees.
 */
void rotateToAngle(float angle);

/**
 * @brief Set smoother motion parameters to reduce stuttering.
 * Reduces acceleration and slightly lowers max speed for smoother movement.
 */
void setSmoothRotationMotion();

/**
 * @brief Manually rotates clockwise by 90 degrees WITHOUT position tracking.
 * This function is for manual websocket commands only and allows operators
 * to reorient the tray without affecting automatic paint cycles.
 * 
 * !!! WARNING: Does NOT update position tracking - use only for manual control !!!
 */
void manualRotateClockwise90();

/**
 * @brief Manually rotates counter-clockwise by 90 degrees WITHOUT position tracking.
 * This function is for manual websocket commands only and allows operators
 * to reorient the tray without affecting automatic paint cycles.
 * 
 * !!! WARNING: Does NOT update position tracking - use only for manual control !!!
 */
void manualRotateCounterClockwise90();

// Add any other rotation-specific functions here if needed in the future
// e.g., void rotateToAngle(float angle);
// e.g., void setRotationSpeed(uint32_t speed);

#endif // ROTATION_MOTOR_H 