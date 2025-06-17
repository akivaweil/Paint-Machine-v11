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
//* ************************ SIDE 2 STATE CLASS ***************************
//* ************************************************************************

class Side2State : public State {
public:
    Side2State();
    void enter() override;
    void update() override;
    void exit() override;
    const char* getName() const override;

private:
    enum Side2SubStep {
        S2_IDLE,                                    // 1. Initial idle state
        S2_SET_SERVO_ANGLE,                        // 2. Set servo to Side 2 angle
        S2_TURN_ON_PRESSURE_POT,                   // 3. Turn on pressure pot
        S2_MOVE_TO_SAFE_Z,                         // 4. Move to safe Z height
        S2_ROTATE_TO_POSITION,                     // 5. Rotate to Side 2 position
        S2_MOVE_TO_START_POSITION,                 // 6. Move to start X,Y position
        S2_LOWER_TO_PAINTING_Z,                    // 7. Lower to painting Z height
        S2_WAIT_FOR_MODIFIER_BUTTON,               // 8. Wait for modifier button release
        S2_EXECUTE_Y_SWEEPS,                       // 9. Execute 5 Y-sweeps with -X shifts
        S2_WAIT_FOR_MODIFIER_BUTTON_AFTER_SWEEPS,  // 10. Wait for modifier button after sweeps
        S2_PERFORM_END_SEQUENCE_X_MOVE,            // 11. Move -2 inches in X
        S2_WAIT_FOR_MODIFIER_BUTTON_BEFORE_SERVO,  // 12. Wait before servo change
        S2_SET_SERVO_FOR_FINAL_X,                  // 13. Set servo to 85° and Z to -1.75"
        S2_EXECUTE_FINAL_X_SWEEP,                  // 14. Execute +23" X sweep with paint control
        S2_WAIT_FOR_MODIFIER_BUTTON_FINAL,         // 15. Final modifier button wait
        S2_RAISE_TO_SAFE_Z,                        // 16. Raise to safe Z height
        S2_WAIT_FOR_MODIFIER_BUTTON_LAST,          // 17. Last modifier button wait
        S2_MOVE_TO_HOME_POSITION,                  // 18. Move to (1,1,0) before homing
        S2_TRANSITION_TO_HOMING                    // 19. Transition to homing state
    };
    Side2SubStep currentStep;
    
    // Movement variables
    long startX_steps;
    long startY_steps;
    long zPos;
    long sideZPos;
    long currentX;
    long currentY;
    long sweepYDistance;
    long shiftXDistance;
    long paint_x_speed;
    long paint_y_speed;
    long first_sweep_paint_y_speed_side2;
    int currentSweep;
    bool paintGunActivated;
    bool paintGunDeactivated;
    unsigned long moveStartTime;
    
    // Helper methods
    void performCurrentStep();
    void transitionToNextStep();
    void executeYSweep(int sweepNumber);
    void performCombinedXShiftYMove();
};

Side2State::Side2State() : currentStep(S2_IDLE), currentSweep(0), paintGunActivated(false), paintGunDeactivated(false) {
    Serial.println("Side2State: Constructor called");
}

void Side2State::enter() {
    Serial.println("Side2State: Entering Side 2 painting state");
    currentStep = S2_SET_SERVO_ANGLE;
}

void Side2State::update() {
    performCurrentStep();
}

void Side2State::exit() {
    Serial.println("Side2State: Exiting Side 2 painting state");
    paintGun_OFF();
}

const char* Side2State::getName() const {
    return "Side2State";
}

void Side2State::performCurrentStep() {
    switch (currentStep) {
        case S2_IDLE:
            // Do nothing, wait for enter() to be called
            break;
            
        case S2_SET_SERVO_ANGLE:
            Serial.println("Side2State: Setting servo angle");
            {
                float servoAngle = paintingSettings.getServoAngleSide2();
                myServo.setAngle(servoAngle);
                Serial.printf("Servo set to: %.1f degrees for Side 2\n", servoAngle);
            }
            transitionToNextStep();
            break;
            
        case S2_TURN_ON_PRESSURE_POT:
            Serial.println("Side2State: Turning on pressure pot");
            PressurePot_ON();
            transitionToNextStep();
            break;
            
        case S2_MOVE_TO_SAFE_Z:
            Serial.println("Side2State: Moving to safe Z height");
            sideZPos = (long)(paintingSettings.getSide2SideZHeight() * STEPS_PER_INCH_XYZ);
            moveToXYZ(stepperX->getCurrentPosition(), DEFAULT_X_SPEED,
                      stepperY_Left->getCurrentPosition(), DEFAULT_Y_SPEED,
                      sideZPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S2_ROTATE_TO_POSITION:
            Serial.println("Side2State: Rotating to Side 2 position");
            rotateToAngle(SIDE2_ROTATION_ANGLE);
            transitionToNextStep();
            break;
            
        case S2_MOVE_TO_START_POSITION:
            Serial.println("Side2State: Moving to start position");
            startX_steps = (long)(paintingSettings.getSide2StartX() * STEPS_PER_INCH_XYZ);
            startY_steps = (long)(paintingSettings.getSide2StartY() * STEPS_PER_INCH_XYZ);
            moveToXYZ(startX_steps, DEFAULT_X_SPEED, startY_steps, DEFAULT_Y_SPEED, sideZPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S2_LOWER_TO_PAINTING_Z:
            Serial.println("Side2State: Lowering to painting Z height");
            zPos = (long)(paintingSettings.getSide2ZHeight() * STEPS_PER_INCH_XYZ);
            moveToXYZ(startX_steps, DEFAULT_X_SPEED, startY_steps, DEFAULT_Y_SPEED, zPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S2_WAIT_FOR_MODIFIER_BUTTON:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                Serial.println("Side2State: Modifier button released, proceeding to Y sweeps");
                // Initialize sweep variables
                currentX = startX_steps;
                currentY = startY_steps;
                sweepYDistance = (long)(paintingSettings.getSide2SweepY() * STEPS_PER_INCH_XYZ);
                shiftXDistance = (long)(paintingSettings.getSide2ShiftX() * STEPS_PER_INCH_XYZ);
                paint_x_speed = paintingSettings.getSide2PaintingXSpeed();
                paint_y_speed = paintingSettings.getSide2PaintingYSpeed();
                first_sweep_paint_y_speed_side2 = (long)(paint_y_speed * 0.75f);
                currentSweep = 0;
                transitionToNextStep();
            }
            break;
            
        case S2_EXECUTE_Y_SWEEPS:
            executeYSweep(currentSweep);
            break;
            
        case S2_WAIT_FOR_MODIFIER_BUTTON_AFTER_SWEEPS:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                Serial.println("Side2State: Modifier button released after sweeps");
                transitionToNextStep();
            }
            break;
            
        case S2_PERFORM_END_SEQUENCE_X_MOVE:
            Serial.println("Side2State: Performing end sequence X move");
            currentX -= (long)(2.0f * STEPS_PER_INCH_XYZ); // Move -2 inches in X
            moveToXYZ(currentX, DEFAULT_X_SPEED, currentY, DEFAULT_Y_SPEED, zPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S2_WAIT_FOR_MODIFIER_BUTTON_BEFORE_SERVO:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                Serial.println("Side2State: Modifier button released before servo change");
                transitionToNextStep();
            }
            break;
            
        case S2_SET_SERVO_FOR_FINAL_X:
            Serial.println("Side2State: Setting servo to 85° and Z to -1.75\"");
            myServo.setAngle(85);
            zPos = (long)(-1.75f * STEPS_PER_INCH_XYZ);
            moveToXYZ(currentX, DEFAULT_X_SPEED, currentY, DEFAULT_Y_SPEED, zPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S2_EXECUTE_FINAL_X_SWEEP:
            Serial.println("Side2State: Executing final X sweep");
            {
                long finalX = currentX + (long)(23.0f * STEPS_PER_INCH_XYZ);
                long paintStartX = currentX + (long)(0.25f * STEPS_PER_INCH_XYZ);
                long paintStopX = finalX - (long)(0.75f * STEPS_PER_INCH_XYZ);
                
                stepperX->moveTo(finalX);
                stepperX->setSpeedInHz(paint_x_speed);
                
                bool paintGunOn = false;
                while (stepperX->isRunning()) {
                    long currentXPos = stepperX->getCurrentPosition();
                    
                    // Check for pause button press
                    if (digitalRead(MODIFIER_BUTTON_RIGHT) == LOW) {
                        bool wasGunOn = paintGunOn;
                        if (paintGunOn) {
                            paintGun_OFF();
                            paintGunOn = false;
                            Serial.println("Side2State: Paint gun OFF (paused)");
                        }
                        stepperX->forceStop();
                        Serial.println("Side2State: Movement paused");
                        
                        // Wait for button release
                        while (digitalRead(MODIFIER_BUTTON_RIGHT) == LOW) {
                            delay(10);
                        }
                        
                        // Resume movement
                        stepperX->moveTo(finalX);
                        stepperX->setSpeedInHz(paint_x_speed);
                        if (wasGunOn && currentXPos >= paintStartX && currentXPos < paintStopX) {
                            paintGun_ON();
                            paintGunOn = true;
                            Serial.println("Side2State: Paint gun ON (resumed)");
                        }
                        Serial.println("Side2State: Movement resumed");
                    }
                    
                    if (!paintGunOn && currentXPos >= paintStartX) {
                        paintGun_ON();
                        paintGunOn = true;
                    }
                    
                    if (paintGunOn && currentXPos >= paintStopX) {
                        paintGun_OFF();
                        paintGunOn = false;
                    }
                    
                    processWebSocketEventsFrequently();
                    delay(1);
                }
                
                if (paintGunOn) {
                    paintGun_OFF();
                }
                
                currentX = finalX;
            }
            transitionToNextStep();
            break;
            
        case S2_WAIT_FOR_MODIFIER_BUTTON_FINAL:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                Serial.println("Side2State: Final modifier button released");
                transitionToNextStep();
            }
            break;
            
        case S2_RAISE_TO_SAFE_Z:
            Serial.println("Side2State: Raising to safe Z height");
            moveToXYZ(currentX, DEFAULT_X_SPEED, currentY, DEFAULT_Y_SPEED, sideZPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S2_WAIT_FOR_MODIFIER_BUTTON_LAST:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                Serial.println("Side2State: Last modifier button released");
                transitionToNextStep();
            }
            break;
            
        case S2_MOVE_TO_HOME_POSITION:
            Serial.println("Side2State: Moving to home position");
            moveToPositionOneOneBeforeHoming();
            transitionToNextStep();
            break;
            
        case S2_TRANSITION_TO_HOMING:
            Serial.println("Side2State: Side 2 painting completed");
            // Check if we're in "Paint All Sides" mode
            if (stateMachine && stateMachine->isInPaintAllSidesMode()) {
                Serial.println("Side2State: Paint All Sides mode - returning to PaintingState");
                PaintingState* paintingState = static_cast<PaintingState*>(stateMachine->getPaintingState());
                stateMachine->changeState(paintingState);
                paintingState->onSideCompleted(); // Notify that this side is complete
            } else {
                Serial.println("Side2State: Individual side mode - transitioning to homing");
                stateMachine->changeState(stateMachine->getHomingState());
            }
            break;
    }
}

void Side2State::transitionToNextStep() {
    currentStep = static_cast<Side2SubStep>(static_cast<int>(currentStep) + 1);
}

void Side2State::executeYSweep(int sweepNumber) {
    const int num_y_sweeps = 5;
    
    if (sweepNumber >= num_y_sweeps) {
        Serial.println("Side2State: All Y sweeps completed");
        transitionToNextStep();
        return;
    }
    
    bool isNegativeYSweep = (sweepNumber % 2 == 0); // 0th, 2nd, 4th sweeps are -Y
    long current_paint_y_speed = paint_y_speed;
    
    if (sweepNumber == 0) {
        current_paint_y_speed = first_sweep_paint_y_speed_side2;
        Serial.printf("Side2State: Applying 75%% speed for first sweep: %ld\n", current_paint_y_speed);
        
        // First sweep - move to top position only
        Serial.printf("Side2State: Moving to top position Y=%ld without painting at fast speed\n", startY_steps);
        moveToXYZ(currentX, DEFAULT_X_SPEED, startY_steps, DEFAULT_Y_SPEED, zPos, DEFAULT_Z_SPEED);
    }
    
    // Paint down to bottom position (all sweeps go -Y in Side2)
    long finalY = startY_steps - sweepYDistance;
    Serial.printf("Side2State: Painting while moving -Y down to Y=%ld\n", finalY);
    
    // Calculate smooth movement parameters
    float totalDistance = (float)sweepYDistance / STEPS_PER_INCH_XYZ;
    float timeToStart = 0.25f * STEPS_PER_INCH_XYZ / (float)current_paint_y_speed;
    float timeToStop = (totalDistance - 0.5f) * STEPS_PER_INCH_XYZ / (float)current_paint_y_speed;
    
    // Start smooth movement for BOTH Y motors
    moveStartTime = millis();
    stepperY_Left->moveTo(finalY);
    stepperY_Left->setSpeedInHz(current_paint_y_speed);
    stepperY_Right->moveTo(finalY);
    stepperY_Right->setSpeedInHz(current_paint_y_speed);
    
    paintGunActivated = false;
    paintGunDeactivated = false;
    
    while(stepperY_Left->isRunning() || stepperY_Right->isRunning()) {
        unsigned long currentTime = millis();
        float elapsedSeconds = (currentTime - moveStartTime) / 1000.0f;
        
        // Check for pause button press
        if (digitalRead(MODIFIER_BUTTON_RIGHT) == LOW) {
            bool wasGunOn = paintGunActivated && !paintGunDeactivated;
            if (wasGunOn) {
                paintGun_OFF();
                Serial.println("Side2State: Paint gun OFF (paused)");
            }
            stepperY_Left->forceStop();
            stepperY_Right->forceStop();
            Serial.println("Side2State: Movement paused");
            
            // Wait for button release
            while (digitalRead(MODIFIER_BUTTON_RIGHT) == LOW) {
                delay(10);
            }
            
            // Resume movement
            stepperY_Left->moveTo(finalY);
            stepperY_Left->setSpeedInHz(current_paint_y_speed);
            stepperY_Right->moveTo(finalY);
            stepperY_Right->setSpeedInHz(current_paint_y_speed);
            moveStartTime = millis() - (unsigned long)(elapsedSeconds * 1000); // Adjust start time
            if (wasGunOn && elapsedSeconds >= timeToStart && elapsedSeconds < timeToStop) {
                paintGun_ON();
                Serial.println("Side2State: Paint gun ON (resumed)");
            }
            Serial.println("Side2State: Movement resumed");
        }
        
        if (!paintGunActivated && elapsedSeconds >= timeToStart) {
            paintGun_ON();
            paintGunActivated = true;
        }
        
        if (paintGunActivated && !paintGunDeactivated && elapsedSeconds >= timeToStop) {
            paintGun_OFF();
            paintGunDeactivated = true;
        }
        
        processWebSocketEventsFrequently();
        delay(1);
    }
    
    paintGun_OFF();
    currentY = finalY;
    
    // Move to next sweep or finish
    currentSweep++;
    
    if (currentSweep < num_y_sweeps) {
        // Perform combined X shift and Y top move for next sweep
        performCombinedXShiftYMove();
    }
}

void Side2State::performCombinedXShiftYMove() {
    // Wait for modifier button release
    while(digitalRead(MODIFIER_BUTTON_RIGHT) == LOW) {
        delay(10);
    }
    
    Serial.printf("Side2State: Combined shift -X and move to top Y after sweep %d at fast speed\n", currentSweep);
    currentX -= shiftXDistance; // Shift in -X direction
    moveToXYZ(currentX, DEFAULT_X_SPEED, startY_steps, DEFAULT_Y_SPEED, zPos, DEFAULT_Z_SPEED);
}

//* ************************************************************************
//* ************************** SIDE 2 PAINTING ***************************
//* ************************************************************************
//* SIDE 2 PAINTING PATTERN (Vertical Sweeps, specific sequence) - NEW LOGIC
//* Based on original Side 4 pattern (5-sweep, starts -Y, shifts -X)
//*
//*    P9                      P7                      P5                      P3                      P1 (SIDE2_START_X,Y)
//*    | (-Y)                | (+Y)                  | (-Y)                  | (+Y)                  | (-Y) Start Sweep
//*    ↓                     ↑                       ↓                       ↑                       ↓
//*    P10 (END)               P8                      P6                      P4                      P2
//*         ← (-X shift)          ← (-X shift)          ← (-X shift)          ← (-X shift)
//* Pattern: Start at (Side2_StartX, Side2_StartY). Sweep -Y. Shift -X. Sweep +Y. Shift -X. Sweep -Y ... for 5 Y sweeps.

void paintSide2Pattern() {
    Serial.println("Starting Side 2 Pattern Painting (Using Side2State)");
    
    // Create and transition to Side2State
    Side2State* side2State = new Side2State();
    
    if (stateMachine) {
        stateMachine->changeState(side2State);
        Serial.println("Transitioned to Side2State for pattern execution");
    } else {
        Serial.println("ERROR: StateMachine not available for Side 2 pattern");
        delete side2State;
    }
}