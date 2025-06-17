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
//* ************************ SIDE 3 STATE CLASS ***************************
//* ************************************************************************

class Side3State : public State {
public:
    Side3State();
    void enter() override;
    void update() override;
    void exit() override;
    const char* getName() const override;

private:
    enum Side3SubStep {
        S3_IDLE,                        // 1. Initial idle state
        S3_SET_SERVO_ANGLE,            // 2. Set servo to Side 3 angle
        S3_TURN_ON_PRESSURE_POT,       // 3. Turn on pressure pot
        S3_MOVE_TO_SAFE_Z,             // 4. Move to safe Z height
        S3_ROTATE_TO_POSITION,         // 5. Rotate to Side 3 position
        S3_MOVE_TO_START_POSITION,     // 6. Move to start X,Y position
        S3_LOWER_TO_PAINTING_Z,        // 7. Lower to painting Z height
        S3_EXECUTE_FIRST_X_SWEEP,      // 8. Execute 1st X sweep (+X direction)
        S3_WAIT_FOR_MODIFIER_BUTTON_1, // 9. Wait for modifier button
        S3_SHIFT_Y_1,                  // 10. Shift Y for 2nd sweep
        S3_WAIT_FOR_MODIFIER_BUTTON_2, // 11. Wait for modifier button
        S3_EXECUTE_SECOND_X_SWEEP,     // 12. Execute 2nd X sweep (-X direction)
        S3_WAIT_FOR_MODIFIER_BUTTON_3, // 13. Wait for modifier button
        S3_SHIFT_Y_2,                  // 14. Shift Y for 3rd sweep
        S3_WAIT_FOR_MODIFIER_BUTTON_4, // 15. Wait for modifier button
        S3_EXECUTE_THIRD_X_SWEEP,      // 16. Execute 3rd X sweep (+X direction)
        S3_WAIT_FOR_MODIFIER_BUTTON_5, // 17. Wait for modifier button
        S3_SHIFT_Y_3,                  // 18. Shift Y for 4th sweep
        S3_WAIT_FOR_MODIFIER_BUTTON_6, // 19. Wait for modifier button
        S3_EXECUTE_FOURTH_X_SWEEP,     // 20. Execute 4th X sweep (-X direction)
        S3_WAIT_FOR_MODIFIER_BUTTON_7, // 21. Wait for modifier button
        S3_SHIFT_Y_4,                  // 22. Shift Y for 5th sweep
        S3_WAIT_FOR_MODIFIER_BUTTON_8, // 23. Wait for modifier button
        S3_EXECUTE_FIFTH_X_SWEEP,      // 24. Execute 5th X sweep (+X direction, final)
        S3_WAIT_FOR_MODIFIER_BUTTON_FINAL, // 25. Final modifier button wait
        S3_RAISE_TO_SAFE_Z,            // 26. Raise to safe Z height
        S3_MOVE_TO_HOME_POSITION,      // 27. Move to (1,1,0) before homing
        S3_TRANSITION_TO_HOMING        // 28. Transition to homing state
    };
    Side3SubStep currentStep;
    
    // Movement variables
    long currentX;
    long currentY;
    long startX_steps;
    long startY_steps;
    long zPos;
    long sideZPos;
    long sweepX_steps;
    long shiftY_steps;
    long paint_x_speed;
    long paint_y_speed;
    long final_sweep_paint_x_speed_side3;
    bool paintGunActivated;
    bool paintGunDeactivated;
    unsigned long moveStartTime;
    
    // Helper methods
    void performCurrentStep();
    void transitionToNextStep();
    void executeXSweep(bool isNegativeDirection, bool isFinalSweep = false);
    void performYShift();
};

Side3State::Side3State() : currentStep(S3_IDLE), paintGunActivated(false), paintGunDeactivated(false) {
    Serial.println("Side3State: Constructor called");
}

void Side3State::enter() {
    Serial.println("Side3State: Entering Side 3 painting state");
    currentStep = S3_SET_SERVO_ANGLE;
}

void Side3State::update() {
    performCurrentStep();
}

void Side3State::exit() {
    Serial.println("Side3State: Exiting Side 3 painting state");
    paintGun_OFF();
}

const char* Side3State::getName() const {
    return "Side3State";
}

void Side3State::performCurrentStep() {
    switch (currentStep) {
        case S3_IDLE:
            break;
            
        case S3_SET_SERVO_ANGLE:
            Serial.println("Side3State: Setting servo angle");
            {
                float servoAngle = paintingSettings.getServoAngleSide3();
                float currentAngle = myServo.getCurrentAngle();
                Serial.printf("Side3State: Current servo angle: %.1f degrees\n", currentAngle);
                Serial.printf("Side3State: Target servo angle from settings: %.1f degrees\n", servoAngle);
                Serial.printf("Side3State: Angle difference: %.1f degrees\n", abs(servoAngle - currentAngle));
                
                myServo.setAngle(servoAngle);
                Serial.printf("Side3State: Servo command sent to: %.1f degrees for Side 3\n", servoAngle);
                
                // Add a delay to allow servo movement
                delay(1000);
                
                float newAngle = myServo.getCurrentAngle();
                Serial.printf("Side3State: Servo angle after command: %.1f degrees\n", newAngle);
            }
            transitionToNextStep();
            break;
            
        case S3_TURN_ON_PRESSURE_POT:
            Serial.println("Side3State: Turning on pressure pot");
            PressurePot_ON();
            transitionToNextStep();
            break;
            
        case S3_MOVE_TO_SAFE_Z:
            Serial.println("Side3State: Moving to safe Z height");
            sideZPos = (long)(paintingSettings.getSide3SideZHeight() * STEPS_PER_INCH_XYZ);
            moveToXYZ(stepperX->getCurrentPosition(), DEFAULT_X_SPEED,
                      stepperY_Left->getCurrentPosition(), DEFAULT_Y_SPEED,
                      sideZPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S3_ROTATE_TO_POSITION:
            Serial.println("Side3State: Rotating to Side 3 position");
            rotateToAngle(SIDE3_ROTATION_ANGLE);
            transitionToNextStep();
            break;
            
        case S3_MOVE_TO_START_POSITION:
            Serial.println("Side3State: Moving to start position");
            startX_steps = (long)(paintingSettings.getSide3StartX() * STEPS_PER_INCH_XYZ);
            startY_steps = (long)(paintingSettings.getSide3StartY() * STEPS_PER_INCH_XYZ);
            moveToXYZ(startX_steps, DEFAULT_X_SPEED, startY_steps, DEFAULT_Y_SPEED, sideZPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S3_LOWER_TO_PAINTING_Z:
            Serial.println("Side3State: Lowering to painting Z height");
            zPos = (long)(paintingSettings.getSide3ZHeight() * STEPS_PER_INCH_XYZ);
            moveToXYZ(startX_steps, DEFAULT_X_SPEED, startY_steps, DEFAULT_Y_SPEED, zPos, DEFAULT_Z_SPEED);
            // Initialize variables
            currentX = startX_steps;
            currentY = startY_steps;
            sweepX_steps = (long)(paintingSettings.getSide3ShiftX() * STEPS_PER_INCH_XYZ);
            shiftY_steps = (long)(paintingSettings.getSide3SweepY() * STEPS_PER_INCH_XYZ);
            paint_x_speed = paintingSettings.getSide3PaintingXSpeed();
            paint_y_speed = paintingSettings.getSide3PaintingYSpeed();
            final_sweep_paint_x_speed_side3 = (long)(paint_x_speed * 0.75f);
            transitionToNextStep();
            break;
            
        case S3_EXECUTE_FIRST_X_SWEEP:
            executeXSweep(true, false); // -X direction, not final
            transitionToNextStep();
            break;
            
        case S3_WAIT_FOR_MODIFIER_BUTTON_1:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                transitionToNextStep();
            }
            break;
            
        case S3_SHIFT_Y_1:
            performYShift();
            transitionToNextStep();
            break;
            
        case S3_WAIT_FOR_MODIFIER_BUTTON_2:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                transitionToNextStep();
            }
            break;
            
        case S3_EXECUTE_SECOND_X_SWEEP:
            executeXSweep(false, false); // +X direction, not final
            transitionToNextStep();
            break;
            
        case S3_WAIT_FOR_MODIFIER_BUTTON_3:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                transitionToNextStep();
            }
            break;
            
        case S3_SHIFT_Y_2:
            performYShift();
            transitionToNextStep();
            break;
            
        case S3_WAIT_FOR_MODIFIER_BUTTON_4:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                transitionToNextStep();
            }
            break;
            
        case S3_EXECUTE_THIRD_X_SWEEP:
            executeXSweep(true, false); // -X direction, not final
            transitionToNextStep();
            break;
            
        case S3_WAIT_FOR_MODIFIER_BUTTON_5:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                transitionToNextStep();
            }
            break;
            
        case S3_SHIFT_Y_3:
            performYShift();
            transitionToNextStep();
            break;
            
        case S3_WAIT_FOR_MODIFIER_BUTTON_6:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                transitionToNextStep();
            }
            break;
            
        case S3_EXECUTE_FOURTH_X_SWEEP:
            executeXSweep(false, false); // +X direction, not final
            transitionToNextStep();
            break;
            
        case S3_WAIT_FOR_MODIFIER_BUTTON_7:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                transitionToNextStep();
            }
            break;
            
        case S3_SHIFT_Y_4:
            performYShift();
            transitionToNextStep();
            break;
            
        case S3_WAIT_FOR_MODIFIER_BUTTON_8:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                transitionToNextStep();
            }
            break;
            
        case S3_EXECUTE_FIFTH_X_SWEEP:
            executeXSweep(true, true); // -X direction, final sweep
            transitionToNextStep();
            break;
            
        case S3_WAIT_FOR_MODIFIER_BUTTON_FINAL:
            if (digitalRead(MODIFIER_BUTTON_RIGHT) == HIGH) {
                transitionToNextStep();
            }
            break;
            
        case S3_RAISE_TO_SAFE_Z:
            Serial.println("Side3State: Raising to safe Z height");
            moveToXYZ(currentX, DEFAULT_X_SPEED, currentY, DEFAULT_Y_SPEED, sideZPos, DEFAULT_Z_SPEED);
            transitionToNextStep();
            break;
            
        case S3_MOVE_TO_HOME_POSITION:
            Serial.println("Side3State: Moving to home position");
            moveToPositionOneOneBeforeHoming();
            transitionToNextStep();
            break;
            
        case S3_TRANSITION_TO_HOMING:
            Serial.println("Side3State: Side 3 painting completed");
            // Check if we're in "Paint All Sides" mode
            if (stateMachine && stateMachine->isInPaintAllSidesMode()) {
                Serial.println("Side3State: Paint All Sides mode - returning to PaintingState");
                PaintingState* paintingState = static_cast<PaintingState*>(stateMachine->getPaintingState());
                stateMachine->changeState(paintingState);
                paintingState->onSideCompleted(); // Notify that this side is complete
            } else {
                Serial.println("Side3State: Individual side mode - transitioning to homing");
                stateMachine->changeState(stateMachine->getHomingState());
            }
            break;
    }
}

void Side3State::transitionToNextStep() {
    currentStep = static_cast<Side3SubStep>(static_cast<int>(currentStep) + 1);
}

void Side3State::executeXSweep(bool isNegativeDirection, bool isFinalSweep) {
    long finalX;
    long current_paint_x_speed = isFinalSweep ? final_sweep_paint_x_speed_side3 : paint_x_speed;
    
    if (isNegativeDirection) {
        finalX = currentX - sweepX_steps;
        Serial.println("Side3State: X sweep in -X direction with smooth paint gun control");
    } else {
        finalX = currentX + sweepX_steps;
        Serial.println("Side3State: X sweep in +X direction with smooth paint gun control");
    }
    
    if (isFinalSweep) {
        Serial.printf("Side3State: Applying 75%% speed for final sweep: %ld\n", current_paint_x_speed);
    }
    
    float totalDistance = (float)sweepX_steps / STEPS_PER_INCH_XYZ;
    float timeToStart = 0.25f * STEPS_PER_INCH_XYZ / (float)current_paint_x_speed;
    float timeToStop = (totalDistance - 0.5f) * STEPS_PER_INCH_XYZ / (float)current_paint_x_speed;
    
    moveStartTime = millis();
    stepperX->moveTo(finalX);
    stepperX->setSpeedInHz(current_paint_x_speed);
    
    paintGunActivated = false;
    paintGunDeactivated = false;
    
    while(stepperX->isRunning()) {
        unsigned long currentTime = millis();
        float elapsedSeconds = (currentTime - moveStartTime) / 1000.0f;
        
        // Check for pause button press
        if (digitalRead(MODIFIER_BUTTON_RIGHT) == LOW) {
            bool wasGunOn = paintGunActivated && !paintGunDeactivated;
            if (wasGunOn) {
                paintGun_OFF();
                Serial.println("Side3State: Paint gun OFF (paused)");
            }
            stepperX->forceStop();
            Serial.println("Side3State: Movement paused");
            
            // Wait for button release
            while (digitalRead(MODIFIER_BUTTON_RIGHT) == LOW) {
                delay(10);
            }
            
            // Resume movement
            stepperX->moveTo(finalX);
            stepperX->setSpeedInHz(current_paint_x_speed);
            moveStartTime = millis() - (unsigned long)(elapsedSeconds * 1000); // Adjust start time
            if (wasGunOn && elapsedSeconds >= timeToStart && elapsedSeconds < timeToStop) {
                paintGun_ON();
                Serial.println("Side3State: Paint gun ON (resumed)");
            }
            Serial.println("Side3State: Movement resumed");
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
    currentX = finalX;
}

void Side3State::performYShift() {
    Serial.println("Side3State: Shift Y-");
    currentY -= shiftY_steps;
    moveToXYZ(currentX, DEFAULT_X_SPEED, currentY, DEFAULT_Y_SPEED, zPos, DEFAULT_Z_SPEED);
}

//* ************************************************************************
//* **************************** SIDE 3 PAINTING ****************************
//* ************************************************************************
//* SIDE 3 SIDE PAINTING PATTERN (Horizontal Sweeps)
//*
//* P1 (startX,startY)  ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ←  (startX-sweepX,startY)
//*       |                                                      |
//*       | Shift 1 (Y-)                                         |
//*       ↓                                                      |
//* (startX,startY-shiftY)  → → → → → → → → → → → → → → → →  (startX-sweepX,startY-shiftY)
//*       |                                                      |
//*       | Shift 2 (Y-)                                         |
//*       ↓                                                      |
//* (startX,startY-2*shiftY)← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ←  (startX-sweepX,startY-2*shiftY)
//*       |                                                      |
//*       | Shift 3 (Y-)                                         |
//*       ↓                                                      |
//* (startX,startY-3*shiftY)→ → → → → → → → → → → → → → → →  (startX-sweepX,startY-3*shiftY)
//*
//* Sequence: Start → Sweep X- → Shift Y- → Sweep X+ → Shift Y- → Sweep X- → Shift Y- → Sweep X+
//* Paint ON during horizontal (X) sweeps. Start position assumed to be top-right corner.
//*

// Function to paint the side 3 pattern using the new Side3State
void paintSide3Pattern() {
    Serial.println("Starting Side 3 Pattern Painting (Using Side3State)");
    
    // Create and transition to Side3State
    Side3State* side3State = new Side3State();
    
    if (stateMachine) {
        stateMachine->changeState(side3State);
        Serial.println("Transitioned to Side3State for pattern execution");
    } else {
        Serial.println("ERROR: StateMachine not available for Side 3 pattern");
        delete side3State;
    }
} 