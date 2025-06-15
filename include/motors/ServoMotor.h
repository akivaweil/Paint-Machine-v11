#ifndef SERVOMOTOR_H
#define SERVOMOTOR_H

#include <Arduino.h>
#include <ESP32Servo.h>

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