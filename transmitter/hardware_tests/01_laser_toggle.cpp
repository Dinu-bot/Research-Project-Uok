/* * TEST A: Laser & MOSFET Driver
 * WHAT IT CHECKS: Verifies that the ESP32 can switch the laser to full brightness 
 * without drawing too much current and crashing the system.
 * HOW IT WORKS: We send a 3.3V HIGH signal from GPIO 17 to the MOSFET gate. 
 * This opens the MOSFET's drain-source channel, allowing the 5V rail 
 * to push ~28mA through the laser diode and the 100-ohm limit resistor. 
 * When LOW, it drops back down to the 1.3mA "simmer" bypass circuit.
 */

// Laser Toggle Unit Test
// Target: ESP32 GPIO 17 (TX2)
#include <Arduino.h>
#define LASER_PIN 17

void setup() {
  // Initialize the serial monitor for debugging
  Serial.begin(115200);
  delay(1000); 
  Serial.println("\n--- Laser Toggle Test Initialized ---");

  // Configure the laser pin as an output
  pinMode(LASER_PIN, OUTPUT);
  
  // Ensure we start in the OFF (simmer) state
  digitalWrite(LASER_PIN, LOW);
}

void loop() {
  Serial.println("Command: HIGH (Laser Full Brightness)");
  digitalWrite(LASER_PIN, HIGH);
  delay(2000); // Hold for 2 seconds

  Serial.println("Command: LOW (Laser Simmer State)");
  digitalWrite(LASER_PIN, LOW);
  delay(2000); // Hold for 2 seconds
}