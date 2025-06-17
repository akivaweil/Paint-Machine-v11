#include "motors/ServoMotor.h"
#include <ESP32Servo.h>

//* ************************************************************************
//* **************************** SERVO MOTOR *******************************
//* ************************************************************************
// ESP32-S3 Servo Motor Implementation with proper PWM configuration
// Provides smooth servo control with float precision and bounds checking

// Servo configuration constants
const float ServoMotor::MIN_ANGLE = 0.0f;
const float ServoMotor::MAX_ANGLE = 180.0f;
const int ServoMotor::SERVO_MIN_PULSE_WIDTH = 500;   // 0.5ms pulse width for 0°
const int ServoMotor::SERVO_MAX_PULSE_WIDTH = 2500;  // 2.5ms pulse width for 180°

ServoMotor::ServoMotor(int pin) : servoPin(pin), currentAngle(0.0f), attached(false) {}

void ServoMotor::init(float initialAngle) {
    // Configure ESP32 servo with proper PWM settings
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    
    // Attach servo with custom pulse width range for better precision
    servo.setPeriodHertz(50);    // Standard 50 Hz servo frequency
    servo.attach(servoPin, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);
    attached = true;
    
    // Set initial angle with validation
    setAngle(initialAngle);
    Serial.printf("Servo Initialized on pin %d at: %.1f degrees\n", servoPin, currentAngle);
}

void ServoMotor::setAngle(float angle) {
    if (!attached) {
        Serial.println("WARNING: Servo not attached, cannot set angle");
        return;
    }
    
    // Constrain angle to valid range
    float constrainedAngle = constrainAngle(angle);
    
    Serial.printf("ServoMotor: setAngle called - requested: %.1f°, constrained: %.1f°, current: %.1f°\n", 
                  angle, constrainedAngle, currentAngle);
    
    // Always move the servo and update position (removed the threshold check for debugging)
    servo.write(constrainedAngle);
    currentAngle = constrainedAngle;
    Serial.printf("ServoMotor: Servo commanded to move to %.1f degrees\n", constrainedAngle);
    
    // Debug output if angle was constrained
    if (abs(constrainedAngle - angle) > 0.1f) {
        Serial.printf("ServoMotor: Angle constrained from %.1f to %.1f degrees\n", angle, constrainedAngle);
    }
}

float ServoMotor::getCurrentAngle() {
    return currentAngle;
}

bool ServoMotor::isAttached() {
    return attached && servo.attached();
}

void ServoMotor::detach() {
    if (attached) {
        servo.detach();
        attached = false;
        Serial.printf("Servo detached from pin %d\n", servoPin);
    }
}

void ServoMotor::reattach() {
    if (!attached) {
        servo.attach(servoPin, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);
        attached = true;
        Serial.printf("Servo reattached to pin %d\n", servoPin);
    }
}

// Private helper methods
bool ServoMotor::isValidAngle(float angle) {
    return (angle >= MIN_ANGLE && angle <= MAX_ANGLE);
}

float ServoMotor::constrainAngle(float angle) {
    if (angle < MIN_ANGLE) return MIN_ANGLE;
    if (angle > MAX_ANGLE) return MAX_ANGLE;
    return angle;
} 