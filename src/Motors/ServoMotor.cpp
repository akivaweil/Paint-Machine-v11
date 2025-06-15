#include "motors/ServoMotor.h"
#include <ESP32Servo.h>

//* ************************************************************************
//* **************************** SERVO MOTOR *******************************
//* ************************************************************************

ServoMotor::ServoMotor(int pin) : servoPin(pin), currentAngle(0.0f) {}

void ServoMotor::init(float initialAngle) {
    servo.attach(servoPin);
    setAngle(initialAngle);
    Serial.println("Servo Initialized at: " + String(initialAngle) + " degrees");
}

void ServoMotor::setAngle(float angle) {
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;
    servo.write((int)angle); // ESP32Servo expects int
    currentAngle = angle;
    //Serial.println("Servo moved to: " + String(angle) + " degrees"); // Avoid serial print during potential motor movement
}

float ServoMotor::getCurrentAngle() {
    return currentAngle;
} 