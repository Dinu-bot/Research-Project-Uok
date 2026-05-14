/* * TEST C: LoRa SPI Handshake
 * WHAT IT CHECKS: Confirms the digital connection between the ESP32 and the Ra-02 module[cite: 256].
 * HOW IT WORKS: The ESP32 uses the SPI protocol to "knock on the door" of the LoRa chip's 
 * internal registers[cite: 256]. If the wires for data (MISO/MOSI) and timing (SCK) are correct 
 * and the chip has 3.3V power, it will respond with a "Success" message[cite: 257, 266].
 */

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// Pins based on your Phase 7 Wiring Guide
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SCK  18
#define LORA_NSS  5
#define LORA_RST  14
#define LORA_DIO0 4

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n--- LoRa SPI Communication Test ---");

  // Step 1: Initialize the SPI bus with your specific pins
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  
  // Step 2: Tell the LoRa library which pins control the module's state
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  Serial.println("Attempting to initialize Ra-02 module...");

  // Step 3: Attempt the handshake at 433 MHz (standard for Ra-02)
  if (!LoRa.begin(433E6)) { 
    Serial.println("CRITICAL FAILURE: LoRa Handshake failed!");
    Serial.println("Check MISO (19), MOSI (23), SCK (18), and NSS (5) wiring.");
    Serial.println("Also verify the Ra-02 is connected to the 3.3V rail, NOT 5V.");
    while (1); // Halt the code here forever
  }

  Serial.println("SUCCESS: LoRa chip responded perfectly!");
  Serial.println("The SPI bus is correctly wired.");
}

void loop() {
  // Keep the board idle
  delay(10000);
}