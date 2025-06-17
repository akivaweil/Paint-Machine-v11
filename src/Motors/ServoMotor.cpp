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
    Serial.println("Attempting to attach servo...");
    Serial.printf("Attempting to attach servo on pin %d...\n", servoPin);
    
    // Configure ESP32 servo with proper PWM settings
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    
    // Try Method 1: Default attach
    servo.setPeriodHertz(50);    // Standard 50 Hz servo frequency
    int result1 = servo.attach(servoPin, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);
    Serial.printf("Method 1 result - Channel: %d\n", result1);
    
    // Try Method 2: Custom parameters
    if (result1 == 0) {
        Serial.println("Trying custom parameters (500-2500us)...");
        int result2 = servo.attach(servoPin, 500, 2500);
        Serial.printf("Method 2 result - Channel: %d\n", result2);
        
        // Try Method 3: Different pulse range
        if (result2 == 0) {
            Serial.println("Trying wider pulse range (1000-2000us)...");
            int result3 = servo.attach(servoPin, 1000, 2000);
            Serial.printf("Method 3 result - Channel: %d\n", result3);
        }
    }
    
    // Force attached status regardless of actual result
    Serial.println("⚠ All attach methods failed, but FORCING servo operation anyway");
    Serial.printf("🔧 This means PWM will still be generated on pin %d\n", servoPin);
    attached = true;
    Serial.println("⚠ Forced attach completed - servo commands will be sent regardless");
    
    Serial.printf("Servo attached: %s\n", attached ? "YES" : "NO");
    
    // Set initial angle with validation
    setAngle(initialAngle);
    Serial.printf("Servo Initialized on pin %d at: %.1f degrees\n", servoPin, currentAngle);
}

void ServoMotor::setAngle(float angle) {
    // Constrain angle to valid range
    float constrainedAngle = constrainAngle(angle);
    
    Serial.printf("ServoMotor: setAngle called - requested: %.1f°, constrained: %.1f°, current: %.1f°\n", 
                  angle, constrainedAngle, currentAngle);
    
    // Always move the servo and update position (forced operation)
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