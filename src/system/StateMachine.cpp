#include "system/StateMachine.h"
#include "states/IdleState.h"
#include "states/HomingState.h"
#include "states/PaintingState.h"
#include "states/CleaningState.h"
#include "states/PausedState.h"
#include "states/InspectTipState.h"
#include <Arduino.h>
#include "system/machine_state.h"
#include "states/State.h"
#include <WebSocketsServer.h>

//* ************************************************************************
//* ************************* STATE MACHINE *******************************
//* ************************************************************************

// Global state machine pointer
StateMachine* stateMachine = nullptr;

// Extern declaration for the WebSocket server instance
extern WebSocketsServer webSocket;

// Flag to prevent circular state changes
bool inStateTransition = false;

StateMachine::StateMachine() : 
    currentState(nullptr),
    nextStateOverride(nullptr),
    _isTransitioningToPaintAllSides(false),
    _inPaintAllSidesMode(false)
{
    // Initialize all state objects
    idleState = new IdleState();
    homingState = new HomingState();
    paintingState = new PaintingState();
    cleaningState = new CleaningState();
    pausedState = new PausedState();
    inspectTipState = new InspectTipState();
    
    // Set initial state to idle
    currentState = idleState;
    currentState->enter();
    
    // Set the global pointer
    stateMachine = this;
    
    Serial.println("State Machine initialized with Idle state");
}

StateMachine::~StateMachine() {
    // Clean up all state objects
    delete idleState;
    delete homingState;
    delete paintingState;
    delete cleaningState;
    delete pausedState;
    delete inspectTipState;
    
    // Clear the global pointer
    stateMachine = nullptr;
}

void StateMachine::changeState(State* newState) {
    if (newState == nullptr) {
        Serial.println("ERROR: Attempted to change to NULL state!");
        return;
    }
    
    const char* newStateName = newState->getName();
    
    // Check if already in the target state
    if (currentState == newState) {
        Serial.print("INFO: Already in state: ");
        Serial.println(newStateName);
        return;
    }

    // Set flag to prevent circular state changes
    inStateTransition = true;
    
    if (currentState != nullptr) {
        Serial.print("Changing state from ");
        Serial.print(currentState->getName());
        Serial.print(" to ");
        Serial.println(newStateName);
        currentState->exit();
    }
    
    currentState = newState;
    currentState->enter();

    // Broadcast state change
    String stateMessage = "STATE:";
    stateMessage += newStateName;
    webSocket.broadcastTXT(stateMessage);
    Serial.print("Broadcasted state: ");
    Serial.println(stateMessage);
    
    // Clear flag
    inStateTransition = false;
}

void StateMachine::update() {
    if (currentState != nullptr) {
        currentState->update();
    }
}

//* ************************************************************************
//* ********************* NEXT STATE OVERRIDE METHODS ********************
//* ************************************************************************

void StateMachine::setNextStateOverride(State* state) {
    nextStateOverride = state;
    if (state) {
        Serial.printf("StateMachine: Next state override set to: %s\n", state->getName());
    } else {
        Serial.println("StateMachine: Next state override cleared.");
    }
}

State* StateMachine::getNextStateOverrideAndClear() {
    State* temp = nextStateOverride;
    if (temp) {
        Serial.printf("StateMachine: Consuming next state override: %s\n", temp->getName());
    }
    nextStateOverride = nullptr;
    return temp;
}

//* ************************************************************************
//* ***************** PAINT ALL SIDES TRANSITION METHODS ******************
//* ************************************************************************

void StateMachine::setTransitioningToPaintAllSides(bool value) {
    _isTransitioningToPaintAllSides = value;
    if (value) {
        Serial.println("StateMachine: Flag set - transitioning to Paint All Sides sequence.");
    } else {
        Serial.println("StateMachine: Flag cleared - no longer transitioning to Paint All Sides sequence.");
    }
}

bool StateMachine::isTransitioningToPaintAllSides() const {
    return _isTransitioningToPaintAllSides;
}

void StateMachine::clearTransitioningToPaintAllSidesFlag() {
    if (_isTransitioningToPaintAllSides) {
        Serial.println("StateMachine: Paint All Sides transition flag explicitly cleared.");
    }
    _isTransitioningToPaintAllSides = false;
}

//* ************************************************************************
//* ******************* PAINT ALL SIDES MODE METHODS *********************
//* ************************************************************************

void StateMachine::setInPaintAllSidesMode(bool value) {
    _inPaintAllSidesMode = value;
    if (value) {
        Serial.println("StateMachine: Entering Paint All Sides mode.");
    } else {
        Serial.println("StateMachine: Exiting Paint All Sides mode.");
    }
}

bool StateMachine::isInPaintAllSidesMode() const {
    return _inPaintAllSidesMode;
}

//* ************************************************************************
//* ************************* HELPER METHODS ******************************
//* ************************************************************************

const char* StateMachine::getStateName(State* state) {
    if (state != nullptr) {
        return state->getName();
    } else {
        return "Unknown";
    }
} 