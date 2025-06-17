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
#include "../../config/Pins_Definitions.h" // Added for button pin definitions

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
    shortMode(false), // Initialize shortMode to false
    cleaningStep(0),
    atCleanPosition(false),
    paintGunActive(false)
{
    // Constructor implementation
}

void CleaningState::setShortMode(bool mode) {
    shortMode = mode;
    Serial.print("CleaningState: Set to ");
    Serial.println(shortMode ? "SHORT mode" : "NORMAL mode");
}

void CleaningState::enter() {
    Serial.print("Entering Cleaning State (");
    Serial.print(shortMode ? "SHORT" : "NORMAL");
    Serial.println(" mode)");
    
    //! Set Servo to cleaning angle
    myServo.setAngle(35);
    Serial.println("Servo set to cleaning angle (35 degrees)");

    //! Rotate to Side 4 position concurrently with cleaning prep
    // This is called here assuming 'paintAllSides' will always start with Side 4 after cleaning.
    // If cleaning is used for other purposes, this might need more sophisticated logic
    // to determine which angle to rotate to, or if rotation is needed at all.
    if (stateMachine && stateMachine->isTransitioningToPaintAllSides()) { // Hypothetical check
        Serial.println("CleaningState: Initiating rotation to Side 4 angle during cleaning prep.");
        rotateToAngle(SIDE4_ROTATION_ANGLE); // Rotate for the first side of 'paintAllSides'
    } else {
        Serial.println("CleaningState: Not rotating, as not transitioning to Paint All Sides or stateMachine unavailable.");
    }
    
    // Reset cleaning state variables
    _isCleaning = true;
    _cleaningComplete = false;
    cleaningStep = 0;
    atCleanPosition = false;
    paintGunActive = false;
    Serial.println("Cleaning process initiated...");
    // DO NOT execute blocking cycle here
}

void CleaningState::update() {
    // Handle different cleaning steps based on current step
    switch (cleaningStep) {
        case 0: // Initialize pressure pot
            {
                Serial.println("CleaningState: Step 0 - Initializing pressure pot");
                PressurePot_ON();
                unsigned long pressurePotInitDelay = shortMode ? SHORT_PRESSURE_POT_INIT_DELAY : NORMAL_PRESSURE_POT_INIT_DELAY;
                delay(pressurePotInitDelay);
                cleaningStep = 1;
            }
            break;
            
        case 1: // Move to clean station
            {
                Serial.println("CleaningState: Step 1 - Moving to clean station");
                long cleaningX = 0.8 * STEPS_PER_INCH_XYZ;
                long cleaningY = 4.1 * STEPS_PER_INCH_XYZ;
                long cleaningZ = -3.0 * STEPS_PER_INCH_XYZ;
                moveToXYZ(cleaningX, CLEANING_X_SPEED, cleaningY, CLEANING_Y_SPEED, cleaningZ, CLEANING_Z_SPEED);
                atCleanPosition = true;
                cleaningStep = 2;
            }
            break;
            
        case 2: // Hold paint gun while button is pressed
            {
                bool cleanupButtonPressed = (digitalRead(ACTION_BUTTON_CENTER) == HIGH);
                
                if (cleanupButtonPressed && !paintGunActive) {
                    // Button pressed and paint gun not active - turn on paint gun
                    Serial.println("CleaningState: Cleanup button pressed - activating paint gun");
                    paintGun_ON();
                    paintGunActive = true;
                } else if (!cleanupButtonPressed && paintGunActive) {
                    // Button released and paint gun active - turn off paint gun and start return sequence
                    Serial.println("CleaningState: Cleanup button released - deactivating paint gun and returning home");
                    paintGun_OFF();
                    paintGunActive = false;
                    cleaningStep = 3; // Move to return home step
                } else if (!cleanupButtonPressed && !paintGunActive && shortMode) {
                    // For short mode, run for the specified duration if button not held
                    Serial.println("CleaningState: Short mode - running paint gun for specified duration");
                    paintGun_ON();
                    unsigned long paintGunOnDelay = SHORT_PAINT_GUN_ON_DELAY;
                    delay(paintGunOnDelay);
                    paintGun_OFF();
                    cleaningStep = 3; // Move to return home step
                }
            }
            break;
            
        case 3: // Return to home position
            {
                Serial.println("CleaningState: Step 3 - Returning to home position");
                long cleaningX = 0.8 * STEPS_PER_INCH_XYZ;
                long cleaningY = 4.1 * STEPS_PER_INCH_XYZ;
                
                // Retract the paint gun
                moveToXYZ(cleaningX, CLEANING_X_SPEED, cleaningY, CLEANING_Y_SPEED, 0, CLEANING_Z_SPEED);
                // Move back to home position
                moveToXYZ(0, CLEANING_X_SPEED, 0, CLEANING_Y_SPEED, 0, CLEANING_Z_SPEED);
                
                Serial.println("Cleaning Cycle Complete.");
                _cleaningComplete = true;
                _isCleaning = false;
                cleaningStep = 4; // Mark as complete
            }
            break;
            
        case 4: // Transition out of cleaning state
            {
                State* overrideState = nullptr;
                if (stateMachine) { // Check stateMachine first
                    overrideState = stateMachine->getNextStateOverrideAndClear();
                }

                if (overrideState) {
                    Serial.println("CleaningState: Short clean complete. Transitioning to override state.");
                    if(stateMachine) stateMachine->changeState(overrideState);
                } else {
                    Serial.println("CleaningState: Normal clean complete. Transitioning to Idle State.");
                    if (stateMachine) {
                         stateMachine->changeState(stateMachine->getIdleState());
                    } else {
                        Serial.println("ERROR: StateMachine pointer null. Cannot transition to Idle.");
                    }
                }
                _cleaningComplete = false; // Reset for next entry
            }
            break;
    }
}

void CleaningState::exit() {
    Serial.println("Exiting Cleaning State");
    // Ensure paint gun and pressure pot are off when exiting
    if (paintGunActive) {
        paintGun_OFF();
        paintGunActive = false;
    }
    PressurePot_OFF(); 
    _isCleaning = false; // Ensure flags are reset
    _cleaningComplete = false;
    shortMode = false; // Ensure mode is reset on any exit
    cleaningStep = 0;
    atCleanPosition = false;
}

const char* CleaningState::getName() const {
    return "CLEANING";
}

// REMOVED Banner comment from the end of the file to avoid parsing issues.
// It should ideally be placed after includes.