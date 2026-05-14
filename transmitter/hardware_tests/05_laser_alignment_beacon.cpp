/* * FSO ALIGNMENT BEACON (NODE A)
 * Modulates the bare laser diode with standard UART serial data.
 * Broadcasts repeatedly to allow for physical receiver alignment.
 */

#include <Arduino.h>

#define RX2_PIN 16
#define TX2_PIN 17 // Your laser MOSFET gate

void setup() {
  // 1. Initialize your PC serial monitor for debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== FSO ALIGNMENT BEACON STARTING ===");

  // 2. Initialize the Laser Serial Port (Hardware Serial 2)
  // Baud rate: 1200 (We use a very slow speed for maximum reliability during physical alignment)
  // Format: SERIAL_8N1 (Standard 8 data bits, no parity, 1 stop bit)
  // Invert: 'true' - CRITICAL to keep the laser idling in simmer (LOW) instead of full brightness (HIGH)
  Serial2.begin(1200, SERIAL_8N1, RX2_PIN, TX2_PIN, true);
  
  Serial.println("Transmitting 'HELLO WORLD' at 1200 baud...");
}

void loop() {
  // Send the data string over the laser beam
  Serial2.println("HELLO WORLD - ALIGNMENT BEACON");
  
  // Print to your PC terminal just so you know it's working
  Serial.println("Pulsing optical data...");
  
  // Wait half a second before sending the next burst
  delay(500); 
}