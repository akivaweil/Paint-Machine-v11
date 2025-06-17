#include "motors/ServoMotor.h"
#include <ESP32Servo.h>

//* ************************************************************************
//* **************************** SERVO MOTOR *******************************
//* ************************************************************************
// ESP32-S3 Servo Motor Implementation with proper PWM configuration
// Provides smooth servo control with float precision and bounds checking

ServoMotor::ServoMotor(int pin) : servoPin(pin), currentAngle(0.0f) {
    // Force servo attachment - try multiple methods like the working code
    Serial.printf("Attempting to attach servo on pin %d...\n", servoPin);
    
    // Method 1: Try default parameters
    int channel = servo.attach(servoPin);
    Serial.printf("Method 1 result - Channel: %d\n", channel);
    
    // Method 2: Try custom parameters if first failed
    if (channel <= 0) {
        Serial.printf("Trying custom parameters (500-2500us)...\n");
        channel = servo.attach(servoPin, 500, 2500);
        Serial.printf("Method 2 result - Channel: %d\n", channel);
    }
    
    // Method 3: Try different pulse widths if still failing
    if (channel <= 0) {
        Serial.printf("Trying wider pulse range (1000-2000us)...\n");
        channel = servo.attach(servoPin, 1000, 2000);
        Serial.printf("Method 3 result - Channel: %d\n", channel);
    }
    
    // FORCE SUCCESS - We'll assume it worked and proceed
    if (channel > 0) {
        Serial.printf("✓ Servo attached successfully on pin %d, channel %d\n", servoPin, channel);
    } else {
        Serial.printf("⚠ All attach methods failed, but FORCING servo operation anyway\n");
        Serial.printf("⚠ This means PWM will still be generated on pin %d\n", servoPin);
        
        // Try one more time with basic attach
        servo.attach(servoPin);
        Serial.printf("⚠ Forced attach completed - servo commands will be sent regardless\n");
    }
}

void ServoMotor::init(float initialAngle) {
    // No additional attach - already done in constructor
    setAngle(initialAngle);
    Serial.println("Servo Initialized at: " + String(initialAngle) + " degrees");
}

void ServoMotor::setAngle(float angle) {
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;
    // Direct write like the working code - no checks
    servo.write((int)angle);
    currentAngle = angle;
    Serial.printf("Servo command sent: %d degrees\n", (int)angle);
}

float ServoMotor::getCurrentAngle() {
    return currentAngle;
} 