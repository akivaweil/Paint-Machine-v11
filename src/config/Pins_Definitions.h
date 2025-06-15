#ifndef PINS_DEFINITIONS_H
#define PINS_DEFINITIONS_H

//* ************************************************************************
//* ************************** PIN DEFINITIONS *****************************
//* ************************************************************************
// This file contains all hardware pin definitions for the Paint Machine v11
// Organized by subsystem for easy maintenance and reference

// ==========================================================================
//                            STEPPER MOTOR PINS
// ==========================================================================

// --- X Axis Stepper ---
#define X_STEP_PIN 48
#define X_DIR_PIN 45
#define X_HOME_SWITCH 18

// --- Y Axis Left Stepper ---
#define Y_LEFT_STEP_PIN 39
#define Y_LEFT_DIR_PIN 40
#define Y_LEFT_HOME_SWITCH 9

// --- Y Axis Right Stepper ---
#define Y_RIGHT_STEP_PIN 41
#define Y_RIGHT_DIR_PIN 42
#define Y_RIGHT_HOME_SWITCH 15

// --- Z Axis Stepper ---
#define Z_STEP_PIN 35
#define Z_DIR_PIN 36
#define Z_HOME_SWITCH 46

// --- Rotation Axis Stepper ---
#define ROTATION_STEP_PIN 37
#define ROTATION_DIR_PIN 38

// ==========================================================================
//                            PNEUMATIC SYSTEM PINS
// ==========================================================================

// --- Pneumatic Cylinders ---
#define PICK_CYLINDER_PIN 12       // Pick and place cylinder

// --- Pneumatic Solenoids ---
#define SUCTION_PIN 10             // Vacuum suction control
#define PAINT_GUN_PIN 11           // Paint gun solenoid
#define PRESSURE_POT_PIN 13        // Pressure pot solenoid

// ==========================================================================
//                            SERVO MOTOR PINS
// ==========================================================================

#define SERVO_PIN 4                // Servo motor control pin

// ==========================================================================
//                            SENSOR PINS
// ==========================================================================

// --- Cycle Sensors ---
#define PNP_CYCLE_SENSOR_PIN 21    // Pick and place cycle sensor (Active LOW)

// ==========================================================================
//                            CONTROL PANEL PINS
// ==========================================================================

// --- Main Power ---
#define ON_OFF_SWITCH_PIN 2        // Main power switch

// --- Modifier Buttons (Active HIGH - Input Pulldown) ---
#define MODIFIER_BUTTON_LEFT 44    // Left modifier button
#define MODIFIER_BUTTON_CENTER 1   // Center modifier button  
#define MODIFIER_BUTTON_RIGHT 43   // Right modifier button

// --- Action Buttons (Active LOW - Input Pullup) ---
#define ACTION_BUTTON_LEFT 17      // Left action button
#define ACTION_BUTTON_CENTER 7     // Center action button
#define ACTION_BUTTON_RIGHT 16     // Right action button

// ==========================================================================
//                            COMMUNICATION PINS
// ==========================================================================

// --- Serial Communication ---
// Uses default UART pins (TX/RX on ESP32-S3)

// --- I2C Communication (if needed) ---
// #define SDA_PIN 21
// #define SCL_PIN 22

// --- SPI Communication (if needed) ---
// #define MOSI_PIN 23
// #define MISO_PIN 19
// #define SCK_PIN 18
// #define CS_PIN 5

// ==========================================================================
//                            RELAY PINS (5V Relays)
// ==========================================================================

// --- 5V Relay Control Pins ---
// Add relay pins here as needed for high-power devices

// ==========================================================================
//                            RESERVED PINS
// ==========================================================================

// --- ESP32-S3 Reserved Pins ---
// GPIO 0: Boot mode selection (do not use)
// GPIO 19-20: USB communication (do not use)
// GPIO 26-32: Not available on ESP32-S3
// GPIO 33-39: Input only pins (some variants)

#endif // PINS_DEFINITIONS_H 