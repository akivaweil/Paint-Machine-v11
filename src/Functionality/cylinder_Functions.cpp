#include "utils/settings.h"
#include <Arduino.h>

//* ************************************************************************
//* ************************* CYLINDER CONTROL ***************************
//* ************************************************************************

/**
 * @brief Lower the cylinder (extend)
 * 
 * This function extends the cylinder by setting the pin HIGH
 */
void cylinderDown() {
    digitalWrite(PICK_CYLINDER_PIN, HIGH);
    Serial.println("Cylinder: EXTENDING (DOWN) - Pin set HIGH");
}

/**
 * @brief Raise the cylinder (retract)
 * 
 * This function retracts the cylinder by setting the pin LOW
 */
void cylinderUp() {
    digitalWrite(PICK_CYLINDER_PIN, LOW);
    Serial.println("Cylinder: RETRACTING (UP) - Pin set LOW");
} 
