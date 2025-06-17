#include "states/IdleState.h"
#include <Arduino.h>
// #include <Bounce2.h> // No longer needed here, included via GlobalDebouncers.h
#include "utils/settings.h" // Include for PNP_CYCLE_SENSOR_PIN
#include "system/StateMachine.h" // Include for state machine access
#include "motors/ServoMotor.h" // Include for servo control
#include "system/GlobalState.h" // Include for isPaused global variable
// PnP functionality now handled by PnPState in the state machine
// GlobalDebouncers.h is already included via IdleState.h

// Reference to the global state machine instance
extern StateMachine* stateMachine;
extern ServoMotor myServo; // Declare myServo as extern

//* ************************************************************************
//* ***************************** IDLE STATE ******************************
//* ************************************************************************
// This file defines the IdleState class, which represents the state
// where the machine is waiting for user input or commands.
// It handles initialization of sensors like the PnP cycle switch and
// transitions to other states based on events.

// Bounce pnpCycleSwitch; // REMOVED: This global declaration conflicts with the class member in IdleState.h

IdleState::IdleState() {
    // Constructor implementation
}

void IdleState::enter() {
    Serial.println("Entering Idle State");
    // Set machine status or perform actions specific to entering idle
    // setMachineState(MachineState::IDLE); // REMOVED

    // Clear any lingering pause state when returning to idle
    isPaused = false;
    Serial.println("IdleState: Cleared pause state on entry");

    // Stop motors if they were moving (safety measure)
    // stopAllMotors(); // Example function call

    // Ensure any indicators (like LEDs) show idle status
    // setIdleLED(true);

    // Initialize the PnP cycle sensor pin for direct read with internal pull-up - moved to main setup
    // pinMode(PNP_CYCLE_SENSOR_PIN, INPUT_PULLUP);

    // Setup debouncer for PNP Cycle Sensor
    // pnpCycleSwitch.attach(PNP_CYCLE_SENSOR_PIN); // REMOVED: Using global g_pnpCycleSensorDebouncer
    // pnpCycleSwitch.interval(20); // REMOVED
    Serial.println("PNP Cycle Sensor debouncer is global and initialized in Setup.");

    // Set servo to 180 degrees
    myServo.setAngle(180);
    Serial.println("Servo set to 180 degrees in Idle State.");

    Serial.println("Idle state active. Press PnP cycle sensor to enter PnP mode."); 
}

void IdleState::update() {
    // Monitor for events that trigger state changes
    // e.g., check for button presses, web commands, sensor triggers

    // Update the debouncer
    g_pnpCycleSensorDebouncer.update(); // MODIFIED: Update the global debouncer

    // Example: Check for a start button press
    // if (digitalRead(START_BUTTON_PIN) == HIGH) {
    //     stateMachine->changeState(stateMachine->getHomingState()); 
    // }

    // Example: Check for incoming web command (handled elsewhere, but could be checked here)

    // Keep machine state updated if necessary (REMOVED - StateMachine is source of truth)
    // if (getMachineState() != MachineState::IDLE) {
    //      setMachineState(MachineState::IDLE); 
    // }

    // No continuous actions typically occur in Idle

    // Update the debouncer
    // pnpCycleSensor.update(); // REMOVED for direct read

    // Debug: Print sensor value every second
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 1000) {
        int sensorValue = g_pnpCycleSensorDebouncer.read(); // MODIFIED: Use global debounced read
        // Serial.printf("PNP Cycle Sensor (debounced) value in IdleState: %d (Active when LOW)\n", sensorValue); // REMOVED
        lastDebugTime = millis();
    }

    // Check if the PnP cycle sensor is pressed (active LOW, detected by rising edge)
    if (g_pnpCycleSensorDebouncer.rose()) { // MODIFIED: Check for rising edge on global debouncer
        Serial.println("PnP Cycle Sensor activated (rising edge) in IdleState.");
        
        // Wait for sensor to be released to prevent accidental double triggering
        Serial.println("Waiting for sensor to be released...");
        unsigned long releaseStartTime = millis();
        const unsigned long RELEASE_TIMEOUT_MS = 5000; // 5 second timeout
        
        while (millis() - releaseStartTime < RELEASE_TIMEOUT_MS) {
            g_pnpCycleSensorDebouncer.update();
            if (g_pnpCycleSensorDebouncer.read() == LOW) {
                Serial.println("Sensor released. Starting PnP Full Cycle...");
                delay(100); // Small delay to ensure clean release
                break;
            }
            delay(10);
        }
        
        // Check if we timed out
        if (millis() - releaseStartTime >= RELEASE_TIMEOUT_MS) {
            Serial.println("WARNING: Sensor release timeout. Starting PnP cycle anyway...");
        }
        
        // Transition to PnP state
        Serial.println("Transitioning to PnP state...");
        if (stateMachine) {
            stateMachine->changeState(stateMachine->getPnpState());
        } else {
            Serial.println("ERROR: StateMachine not available for PnP transition.");
        }
        return; // Exit update early after PnP completion
    }
}

void IdleState::exit() {
    Serial.println("Exiting Idle State");
    // Cleanup actions when leaving idle state
    // e.g., turn off idle indicators
    // setIdleLED(false);
}

const char* IdleState::getName() const {
    return "IDLE";
} 