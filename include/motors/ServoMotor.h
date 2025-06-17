#ifndef SERVOMOTOR_H
#define SERVOMOTOR_H

#include <Arduino.h>
#include <ESP32Servo.h>

//* ************************************************************************
//* ************************* SERVO MOTOR CLASS ****************************
//* ************************************************************************
// Servo motor control class for ESP32-S3 with proper PWM configuration
// Uses float precision for smooth servo movements and accurate positioning

class ServoMotor {
public:
    ServoMotor(int pin);
    void init(float initialAngle);
    void setAngle(float angle);
    float getCurrentAngle();

private:
    Servo servo;
    int servoPin;
    float currentAngle;
};

#endif // SERVOMOTOR_H 