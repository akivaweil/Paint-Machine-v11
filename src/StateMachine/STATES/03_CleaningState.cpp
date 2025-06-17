#include "states/CleaningState.h"
#include <Arduino.h>
#include "motors/XYZ_Movements.h"
#include "motors/ServoMotor.h"
// #include "motors/servo_control.h" // Servo control removed
#include "utils/settings.h"
// #include "system/machine_state.h" // No longer needed
#include "hardware/paintGun_Functions.h"
#include "hardware/pressurePot_Functions.h"
#include "system/StateMachine.h" // Added include
// #include "hardware/Brush_Functions.h" // File does not exist
#include "motors/Rotation_Motor.h" // Added for rotateToAngle
#include "settings/painting.h"   // Added for SIDE4_ROTATION_ANGLE
#include "config/Pins_Definitions.h" // Added for button pin

// External variable for pressure pot state
extern bool isPressurePot_ON;
// External functions for pressure pot control
extern void PressurePot_ON();
extern void PressurePot_OFF();

// External servo instance
extern ServoMotor myServo;

// Reference to the global state machine instance
extern StateMachine* stateMachine;

// Cleaning state variables
bool cleaningInProgress = false;
bool cleaningCompleted = false;
unsigned long cleaningStartTime = 0;
int cleaningStep = 0;

// Hold-to-spray variables
bool hasMovedToCleaningPosition = false;
bool isSprayingActive = false;

// Movement speed settings for cleaning
const unsigned int CLEANING_X_SPEED = 2000; // Customize these values as needed
const unsigned int CLEANING_Y_SPEED = 2000;
const unsigned int CLEANING_Z_SPEED = 1000;

// Durations for cleaning steps (milliseconds)
const unsigned long NORMAL_PRESSURE_POT_INIT_DELAY = 100;
const unsigned long SHORT_PRESSURE_POT_INIT_DELAY = 100;
const unsigned long NORMAL_PAINT_GUN_ON_DELAY = 150;
const unsigned long SHORT_PAINT_GUN_ON_DELAY = 75; // Half of normal

CleaningState::CleaningState() : 
    _isCleaning(false),
    _cleaningComplete(false),
    shortMode(false) // Initialize shortMode to false
{
    // Constructor implementation
}

void CleaningState::setShortMode(bool mode) {
    shortMode = mode;
    Serial.print("CleaningState: Set to ");
    Serial.println(shortMode ? "SHORT mode" : "NORMAL mode");
}

void CleaningState::enter() {
    Serial.println("Entering Hold-to-Spray Cleaning State");
    
    //! Set Servo to cleaning angle
    myServo.setAngle(35);
    Serial.println("Servo set to cleaning angle (35 degrees)");

    //! Turn on pressure pot
    PressurePot_ON();
    delay(100); // Brief delay for pressure buildup
    
    // Reset cleaning state variables
    _isCleaning = true;
    _cleaningComplete = false;
    hasMovedToCleaningPosition = false;
    isSprayingActive = false;
    cleaningStartTime = millis(); // Initialize start time for timeout
    Serial.println("Hold-to-spray cleaning mode activated - hold center button to spray");
}

void CleaningState::update() {
    if (!_isCleaning || _cleaningComplete) {
        return;
    }
    
    // Check if center action button is currently pressed (Active HIGH)
    bool centerButtonPressed = digitalRead(ACTION_BUTTON_CENTER);
    
    //! Step 1: Move to cleaning position on first button press
    if (centerButtonPressed && !hasMovedToCleaningPosition) {
        Serial.println("Moving to cleaning position...");
        long cleaningX = 0.8 * STEPS_PER_INCH_XYZ;
        long cleaningY = 4.1 * STEPS_PER_INCH_XYZ;
        long cleaningZ = -3.0 * STEPS_PER_INCH_XYZ;
        moveToXYZ(cleaningX, CLEANING_X_SPEED, cleaningY, CLEANING_Y_SPEED, cleaningZ, CLEANING_Z_SPEED);
        hasMovedToCleaningPosition = true;
        Serial.println("Reached cleaning position - ready to spray");
    }
    
    //! Step 2: Control paint gun based on button state
    if (hasMovedToCleaningPosition) {
        if (centerButtonPressed && !isSprayingActive) {
            // Button pressed and not currently spraying - start spraying
            Serial.println("CENTER BUTTON PRESSED - Starting spray");
            paintGun_ON();
            isSprayingActive = true;
        }
        else if (!centerButtonPressed && isSprayingActive) {
            // Button released and currently spraying - stop spraying
            Serial.println("CENTER BUTTON RELEASED - Stopping spray");
            paintGun_OFF();
            isSprayingActive = false;
            
            // Button released - start return home sequence
            Serial.println("Button released - returning to home position...");
            
            //! Step 3: Return to home position
            long cleaningX = 0.8 * STEPS_PER_INCH_XYZ;
            long cleaningY = 4.1 * STEPS_PER_INCH_XYZ;
            
            // Retract the paint gun (Z to 0)
            moveToXYZ(cleaningX, CLEANING_X_SPEED, cleaningY, CLEANING_Y_SPEED, 0, CLEANING_Z_SPEED);
            // Move back to home position (X,Y to 0)
            moveToXYZ(0, CLEANING_X_SPEED, 0, CLEANING_Y_SPEED, 0, CLEANING_Z_SPEED);
            
            Serial.println("Hold-to-spray cleaning cycle complete");
            _cleaningComplete = true;
            _isCleaning = false;
        }
    }
    
    // If no button press within reasonable time and haven't moved, exit
    if (!hasMovedToCleaningPosition && (millis() - cleaningStartTime) > 5000) {
        Serial.println("No button press detected - exiting cleaning mode");
        _cleaningComplete = true;
        _isCleaning = false;
    }
}

void CleaningState::exit() {
    Serial.println("Exiting Hold-to-Spray Cleaning State");
    
    // Ensure paint gun is off
    if (isSprayingActive) {
        paintGun_OFF();
        isSprayingActive = false;
    }
    
    // Ensure pressure pot is off when exiting
    PressurePot_OFF(); 
    
    // Reset all flags
    _isCleaning = false;
    _cleaningComplete = false;
    hasMovedToCleaningPosition = false;
    isSprayingActive = false;
    shortMode = false;
    
    // Transition back to idle state
    if (stateMachine) {
        stateMachine->changeState(stateMachine->getIdleState());
    }
}

const char* CleaningState::getName() const {
    return "HOLD_TO_SPRAY_CLEANING";
}

// REMOVED Banner comment from the end of the file to avoid parsing issues.
// It should ideally be placed after includes.