#include <Arduino.h>
#include "../../include/system/StateMachine.h"
#include "../../include/states/State.h"
#include "../../include/states/PaintingState.h"
#include "../../include/motors/XYZ_Movements.h"
#include "../../include/utils/settings.h"
#include "../../include/motors/Rotation_Motor.h"
#include "../../include/hardware/paintGun_Functions.h"
#include "../../include/hardware/pressurePot_Functions.h"
#include "../../include/settings/painting.h"
#include "../../include/persistence/PaintingSettings.h"
#include <FastAccelStepper.h>
#include "../../include/motors/ServoMotor.h"
#include "../../include/web/Web_Dashboard_Commands.h"
#include "../config/Pins_Definitions.h"

// External references
extern FastAccelStepper *stepperX;
extern FastAccelStepper *stepperY_Left;
extern FastAccelStepper *stepperY_Right;
extern FastAccelStepper *stepperZ;
extern ServoMotor myServo;
extern PaintingSettings paintingSettings;
extern StateMachine* stateMachine;

//* ************************************************************************
//* ************************ SIDE 1 STATE CLASS ***************************
//* ************************************************************************

class Side1State : public State {
public:
    Side1State();
    void enter() override;
    void update() override;
    void exit() override;
    const char* getName() const override;

private:
    enum Side1SubStep {
        S1_IDLE,                                    // 1. Initial idle state
        S1_SET_SERVO_ANGLE,                        // 2. Set servo to Side 1 angle
        S1_TURN_ON_PRESSURE_POT,                   // 3. Turn on pressure pot
        S1_MOVE_TO_SAFE_Z,                         // 4. Move to safe Z height
        S1_ROTATE_TO_POSITION,                     // 5. Rotate to Side 1 position
        S1_MOVE_TO_START_POSITION,                 // 6. Move to start X,Y position
        S1_LOWER_TO_PAINTING_Z,                    // 7. Lower to painting Z height
        S1_WAIT_FOR_MODIFIER_BUTTON,               // 8. Wait for modifier button release
        S1_EXECUTE_PAINTING_PATTERN,               // 9. Execute X movement with paint gun control
        S1_WAIT_FOR_MODIFIER_BUTTON_AFTER_PAINT,   // 10. Wait for modifier button after painting
        S1_RAISE_TO_SAFE_Z,                        // 11. Raise to safe Z height
        S1_WAIT_FOR_MODIFIER_BUTTON_FINAL,         // 12. Final modifier button wait
        S1_MOVE_TO_HOME_POSITION,                  // 13. Move to (1,1,0) before homing
        S1_TRANSITION_TO_HOMING                    // 14. Transition to homing state
    };
    Side1SubStep currentStep;
    
    // Movement variables
    long startX_steps;
    long startY_steps;
    long zPos;
    long sideZPos;
    long finalX;
    long paintStartX;
    long paintStopX;
    bool paintGunOn;
    
    // Helper methods
    void performCurrentStep();
    void transitionToNextStep();
};

Side1State::Side1State() : currentStep(S1_IDLE), paintGunOn(false) {
    Serial.println("Side1State: Constructor called");
}

void Side1State::enter() {
    Serial.println("Side1State: Entering Side 1 painting state");
    currentStep = S1_SET_SERVO_ANGLE;
}

void Side1State::update() {
    performCurrentStep();
}

void Side1State::exit() {
    Serial.println("Side1State: Exiting Side 1 painting state");
    paintGun_OFF();
}

const char* Side1State::getName() const {
    return "Side1State";
}

void Side1State::performCurrentStep() {
    switch (currentStep) {
        case S1_IDLE:
            // Do nothing, wait for enter() to be called
            break;
            
        case S1_SET_SERVO_ANGLE:
            Serial.println("Side1State: Setting servo angle");
            {
                float servoAngle = paintingSettings.getServoAngleSide1();
                myServo.setAngle(77);
                Serial.printf("Servo set to: %.1f degrees for Side 1\n", servoAngle);
            }
            transitionToNextStep();
            break;
            
        case S1_TURN_ON_PRESSURE_POT:
            Serial.println("Side1State: Turning on pressure pot");
            PressurePot_ON();
            transitionToNextStep();
            break;
            
        case S1_MOVE_TO_SAFE_Z:
            Serial.println("Side1State: Moving to safe Z height");
            sideZPos = (long)(paintingSettings.getSide1SideZHeight() * STEPS_PER_INCH_XYZ);
            moveToXYZ(stepperX->getCurrentPosition(), DEFAULT_X_SPEED,
                      stepperY_Left->getCurrentPosition(), DEFAULT_Y_SPEED,
                      sideZPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S1_ROTATE_TO_POSITION:
            Serial.println("Side1State: Rotating to Side 1 position");
            rotateToAngle(SIDE1_ROTATION_ANGLE);
            transitionToNextStep();
            break;
            
        case S1_MOVE_TO_START_POSITION:
            Serial.println("Side1State: Moving to start position");
            startX_steps = (long)(paintingSettings.getSide1StartX() * STEPS_PER_INCH_XYZ);
            startY_steps = (long)(paintingSettings.getSide1StartY() * STEPS_PER_INCH_XYZ);
            moveToXYZ(startX_steps, DEFAULT_X_SPEED, startY_steps, DEFAULT_Y_SPEED, sideZPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S1_LOWER_TO_PAINTING_Z:
            Serial.println("Side1State: Lowering to painting Z height");
            zPos = (long)(paintingSettings.getSide1ZHeight() * STEPS_PER_INCH_XYZ);
            moveToXYZ(startX_steps, DEFAULT_X_SPEED, startY_steps, DEFAULT_Y_SPEED, zPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S1_WAIT_FOR_MODIFIER_BUTTON:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                Serial.println("Side1State: Modifier button released, proceeding to painting");
                transitionToNextStep();
            }
            break;
            
        case S1_EXECUTE_PAINTING_PATTERN:
            {
                // Calculate movement parameters
                long shiftXDistance = (long)(paintingSettings.getSide1ShiftX() * STEPS_PER_INCH_XYZ);
                long xSpeed = paintingSettings.getSide1PaintingXSpeed();
                finalX = startX_steps + shiftXDistance;
                paintStartX = startX_steps + (long)(0.25f * STEPS_PER_INCH_XYZ);
                paintStopX = finalX - (long)(0.75f * STEPS_PER_INCH_XYZ);
                
                Serial.println("Side1State: Executing painting pattern");
                
                // Start continuous movement
                stepperX->moveTo(finalX);
                stepperX->setSpeedInHz(xSpeed);
                
                // Monitor movement and control paint gun
                while (stepperX->isRunning()) {
                    long currentX = stepperX->getCurrentPosition();
                    
                    // Check for pause button press
                    if (digitalRead(MODIFIER_BUTTON_RIGHT) == LOW) {
                        bool wasGunOn = paintGunOn;
                        if (paintGunOn) {
                            paintGun_OFF();
                            paintGunOn = false;
                            Serial.println("Side1State: Paint gun OFF (paused)");
                        }
                        stepperX->forceStop();
                        Serial.println("Side1State: Movement paused");
                        
                        // Wait for button release
                        while (digitalRead(MODIFIER_BUTTON_RIGHT) == LOW) {
                            delay(10);
                        }
                        
                        // Resume movement
                        stepperX->moveTo(finalX);
                        stepperX->setSpeedInHz(xSpeed);
                        if (wasGunOn && currentX >= paintStartX && currentX < paintStopX) {
                            paintGun_ON();
                            paintGunOn = true;
                            Serial.println("Side1State: Paint gun ON (resumed)");
                        }
                        Serial.println("Side1State: Movement resumed");
                    }
                    
                    // Turn paint gun ON when reaching paint start position
                    if (!paintGunOn && currentX >= paintStartX) {
                        paintGun_ON();
                        paintGunOn = true;
                        Serial.println("Side1State: Paint gun ON");
                    }
                    
                    // Turn paint gun OFF when reaching paint stop position
                    if (paintGunOn && currentX >= paintStopX) {
                        paintGun_OFF();
                        paintGunOn = false;
                        Serial.println("Side1State: Paint gun OFF");
                    }
                    
                    processWebSocketEventsFrequently();
                    delay(1);
                }
            
                // Ensure paint gun is OFF after movement completes
                if (paintGunOn) {
                    paintGun_OFF();
                    paintGunOn = false;
                }
                
                transitionToNextStep();
            }
            break;
            
        case S1_WAIT_FOR_MODIFIER_BUTTON_AFTER_PAINT:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                Serial.println("Side1State: Modifier button released after painting");
                transitionToNextStep();
            }
            break;
            
        case S1_RAISE_TO_SAFE_Z:
            Serial.println("Side1State: Raising to safe Z height");
            moveToXYZ(finalX, DEFAULT_X_SPEED, startY_steps, DEFAULT_Y_SPEED, sideZPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S1_WAIT_FOR_MODIFIER_BUTTON_FINAL:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                Serial.println("Side1State: Final modifier button released");
                transitionToNextStep();
            }
            break;
            
        case S1_MOVE_TO_HOME_POSITION:
            Serial.println("Side1State: Moving to home position");
            moveToPositionOneOneBeforeHoming();
            transitionToNextStep();
            break;
            
        case S1_TRANSITION_TO_HOMING:
            Serial.println("Side1State: Side 1 painting completed");
            // Check if we're in "Paint All Sides" mode
            if (stateMachine && stateMachine->isInPaintAllSidesMode()) {
                Serial.println("Side1State: Paint All Sides mode - returning to PaintingState");
                PaintingState* paintingState = static_cast<PaintingState*>(stateMachine->getPaintingState());
                stateMachine->changeState(paintingState);
                paintingState->onSideCompleted(); // Notify that this side is complete
            } else {
                Serial.println("Side1State: Individual side mode - transitioning to homing");
                stateMachine->changeState(stateMachine->getHomingState());
            }
            break;
    }
}

void Side1State::transitionToNextStep() {
    currentStep = static_cast<Side1SubStep>(static_cast<int>(currentStep) + 1);
}

//* ************************************************************************
//* *************************** SIDE 1 *************************************
//* ************************************************************************

// Simplified hardcoded painting function using Side1State
bool paintSide1Pattern() {
    Serial.println("Starting Side 1 Pattern Painting (Using Side1State)");
    
    // Create and transition to Side1State
    Side1State* side1State = new Side1State();
    
    if (stateMachine) {
        stateMachine->changeState(side1State);
        Serial.println("Transitioned to Side1State for pattern execution");
        return true;
    } else {
        Serial.println("ERROR: StateMachine not available for Side 1 pattern");
        delete side1State;
        return false;
    }
}
