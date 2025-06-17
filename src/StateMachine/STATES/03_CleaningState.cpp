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
#include "settings/homing.h" // Added for homing acceleration constants
#include "settings/motion.h" // Added for default acceleration constants

// External variable for pressure pot state
extern bool isPressurePot_ON;
// External functions for pressure pot control
extern void PressurePot_ON();
extern void PressurePot_OFF();

// External servo instance
extern ServoMotor myServo;

// Reference to the global state machine instance
extern StateMachine* stateMachine;

// External stepper motor instances
extern FastAccelStepper *stepperX;
extern FastAccelStepper *stepperY_Left;
extern FastAccelStepper *stepperY_Right;
extern FastAccelStepper *stepperZ;
extern AccelStepper *rotationStepper;

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

    // Set homing acceleration for faster movement like homing state
    Serial.println("CleaningState: Setting homing acceleration");
    stepperX->setAcceleration(HOMING_ACCEL_X);
    stepperY_Left->setAcceleration(HOMING_ACCEL_Y);
    stepperY_Right->setAcceleration(HOMING_ACCEL_Y);
    stepperZ->setAcceleration(HOMING_ACCEL_Z);
    rotationStepper->setAcceleration(DEFAULT_ROT_ACCEL / 2); // Same as homing state
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
                
                // Automatically turn on paint gun when reaching clean position
                Serial.println("CleaningState: Reached clean position - automatically activating paint gun");
                paintGun_ON();
                paintGunActive = true;
                
                cleaningStep = 2;
            }
            break;
            
        case 2: // Hold paint gun while button is pressed
            {
                bool cleanupButtonPressed = (digitalRead(ACTION_BUTTON_CENTER) == HIGH);
                
                // Debug output to see button state
                static bool lastButtonState = false;
                if (cleanupButtonPressed != lastButtonState) {
                    Serial.printf("CleaningState: Button state changed to %s\n", cleanupButtonPressed ? "PRESSED" : "RELEASED");
                    lastButtonState = cleanupButtonPressed;
                }
                
                if (!cleanupButtonPressed && paintGunActive) {
                    // Button released and paint gun active - start servo rotation immediately, then turn off paint gun and start return sequence
                    Serial.println("CleaningState: Cleanup button released - starting servo rotation to idle angle immediately");
                    myServo.setAngle(180); // Start servo rotation to idle angle BEFORE any other movement
                    
                    Serial.println("CleaningState: Deactivating paint gun and returning home");
                    paintGun_OFF();
                    paintGunActive = false;
                    cleaningStep = 3; // Move to return home step
                } else if (cleanupButtonPressed && paintGunActive) {
                    // Button is being held and paint gun is active - keep spraying
                    Serial.println("CleaningState: Button held - maintaining paint gun ON");
                } else if (!cleanupButtonPressed && !paintGunActive) {
                    // Button not pressed and paint gun not active - this shouldn't happen in normal flow
                    // but handle it for safety
                    Serial.println("CleaningState: Button not pressed and paint gun off - transitioning to return home");
                    cleaningStep = 3; // Move to return home step
                }
                
                // Add a small delay to prevent excessive polling
                delay(50); // Increased delay for better responsiveness
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
    
    // Servo rotation now happens earlier when button is released, so no need to do it here
    
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

    // Restore default acceleration like homing state does
    Serial.println("CleaningState: Restoring default acceleration");
    stepperX->setAcceleration(DEFAULT_X_ACCEL);
    stepperY_Left->setAcceleration(DEFAULT_Y_ACCEL);
    stepperY_Right->setAcceleration(DEFAULT_Y_ACCEL);
    stepperZ->setAcceleration(DEFAULT_Z_ACCEL);
    rotationStepper->setAcceleration(DEFAULT_ROT_ACCEL); // Restore rotation accel too
}

const char* CleaningState::getName() const {
    return "CLEANING";
}

// REMOVED Banner comment from the end of the file to avoid parsing issues.
// It should ideally be placed after includes.