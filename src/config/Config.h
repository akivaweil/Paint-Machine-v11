#ifndef CONFIG_H
#define CONFIG_H

//* ************************************************************************
//* ************************* MACHINE CONFIGURATION ***********************
//* ************************************************************************
// This file contains the main configuration settings for the Paint Machine v11
// All core settings and machine-specific parameters are defined here

// --- Machine Identification ---
#define MACHINE_NAME "Paint Machine v11"
#define FIRMWARE_VERSION "1.0.0"
#define BUILD_DATE __DATE__

// --- Network Configuration ---
#define WIFI_SSID "Everwood"
#define WIFI_PASSWORD "Everwood-Staff"
#define OTA_PORT 3232
#define WEBSOCKET_PORT 81
#define WEB_SERVER_PORT 80

// --- Serial Communication ---
#define SERIAL_BAUD_RATE 115200

// --- Timing Configuration ---
#define DEBOUNCE_DELAY 10
#define MAIN_LOOP_DELAY 1

// --- State Machine Configuration ---
#define DEFAULT_STATE_TRANSITION_DELAY 50

// --- Motor Configuration ---
#define STEPPER_ENABLE_PIN_ACTIVE LOW
#define SERVO_DEFAULT_ANGLE 180.0f

// --- Safety Configuration ---
#define EMERGENCY_STOP_ENABLED true
#define MAX_EMERGENCY_STOP_TIME 5000  // 5 seconds
// Note: HOMING_TIMEOUT_MS already defined in include/settings/homing.h

// Note: PnP defaults already defined in include/settings/motion.h

// --- Paint System Configuration ---
#define PAINT_GUN_ACTIVATION_DELAY 100    // ms
#define PRESSURE_POT_ACTIVATION_DELAY 50  // ms
#define VACUUM_ACTIVATION_DELAY 25        // ms

// --- Debug Configuration ---
#define DEBUG_SERIAL_ENABLED true
#define DEBUG_STATE_TRANSITIONS true
#define DEBUG_MOTOR_MOVEMENTS false
#define DEBUG_SENSOR_READINGS false

#endif // CONFIG_H 