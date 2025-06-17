#include <Arduino.h>
#include "utils/settings.h" // Include settings for pin definitions
#include "hardware/paintGun_Functions.h" // Corrected path
// #include "web_command_adapter.h" // Removing this include
#include <WebSocketsServer.h>

// External reference to the WebSocket instance in webserver.cpp
extern WebSocketsServer webSocket;

// Track the paint gun state
bool isPaintGun_ON = false;
unsigned long lastPaintGunToggleTime = 0;
const unsigned long PAINT_GUN_DEBOUNCE_MS = 50; // Minimum 50ms between toggles

// Function Implementations
/* REMOVED - Logic moved to Setup.cpp
void setupPaintGun() {
    Serial.println("Setting up Paint Gun...");
    pinMode(PAINT_GUN_PIN, OUTPUT);
    digitalWrite(PAINT_GUN_PIN, LOW); // Ensure it's off initially
}
*/

// Moved from web_command_adapter.cpp
void sendWebStatus(WebSocketsServer* webSocket, const char* message) {
    if (webSocket) {
        // Basic implementation: broadcast the message to all connected clients
        webSocket->broadcastTXT(message);
        Serial.print("Sent to WebSocket: "); // Debug output
        Serial.println(message);
    }
}

void paintGun_OFF() {
    // Debounce: Check if enough time has passed since last toggle
    unsigned long currentTime = millis();
    if (currentTime - lastPaintGunToggleTime < PAINT_GUN_DEBOUNCE_MS) {
        // Too soon since last toggle, skip this command
        Serial.println("Paint gun OFF command debounced (too soon)");
        return;
    }
    
    // Only proceed if paint gun is currently ON
    if (!isPaintGun_ON) {
        // Already OFF, no need to send command again
        return;
    }
    
    pinMode(PAINT_GUN_PIN, OUTPUT);
    digitalWrite(PAINT_GUN_PIN, LOW);
    Serial.println("Paint gun OFF");
    Serial.printf("Paint gun pin %d set to LOW\n", PAINT_GUN_PIN);
    
    // Update the state and timing
    isPaintGun_ON = false;
    lastPaintGunToggleTime = currentTime;
    
    // Send status update to web clients to sync UI toggle
    sendWebStatus(&webSocket, "PAINT_GUN_STATUS:OFF");
}

void paintGun_ON() {
    // Debounce: Check if enough time has passed since last toggle
    unsigned long currentTime = millis();
    if (currentTime - lastPaintGunToggleTime < PAINT_GUN_DEBOUNCE_MS) {
        // Too soon since last toggle, skip this command
        Serial.println("Paint gun ON command debounced (too soon)");
        return;
    }
    
    // Only proceed if paint gun is currently OFF
    if (isPaintGun_ON) {
        // Already ON, no need to send command again
        return;
    }
    
    pinMode(PAINT_GUN_PIN, OUTPUT);
    digitalWrite(PAINT_GUN_PIN, HIGH);
    Serial.println("Paint gun ON");
    Serial.printf("Paint gun pin %d set to HIGH\n", PAINT_GUN_PIN);
    
    // Update the state and timing
    isPaintGun_ON = true;
    lastPaintGunToggleTime = currentTime;
    
    // Send status update to web clients to sync UI toggle
    sendWebStatus(&webSocket, "PAINT_GUN_STATUS:ON");
}