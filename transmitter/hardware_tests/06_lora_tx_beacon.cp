#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// --- PIN CONFIGURATION (Per Guide v2) ---
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SCK  18
#define LORA_NSS  5
#define LORA_RST  14
#define LORA_DIO0 4

void setup() {
  // Serial Monitor at 115200 for VS Code PlatformIO
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== SnakeLink: Transmitter Beacon ===");

  // 1. Initialize SPI with specific pins
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  // 2. Start LoRa at 433 MHz
  if (!LoRa.begin(433E6)) { 
    Serial.println("CRITICAL FAILURE: Ra-02 not found!");
    while (1); 
  }
  
  // 3. APPLY FIXES FOR RECEIVER SYNC
  LoRa.setSyncWord(0xF3);           // CRITICAL: Must match Receiver exactly
  LoRa.setTxPower(17);              // Increased power for link stability
  
  Serial.println("SUCCESS: Link Established.");
  Serial.println("Sync Word: 0xF3 | Freq: 433MHz");
  Serial.println("====================================\n");
}

void loop() {
  static int counter = 1;
  
  // Create payload string
  String payload = "Packet #" + String(counter);

  Serial.print("Broadcasting: ");
  Serial.println(payload);

  // --- TRANSMISSION ---
  LoRa.beginPacket();        
  LoRa.print(payload);       
  // Include some padding to help the receiver differentiate noise
  LoRa.print(" | HELLO WORLD"); 
  LoRa.endPacket();          

  counter++;
  
  // 2-second delay gives the Receiver's serial monitor 
  // enough time to handle FSO diagnostics without missing packets.
  delay(2000); 
}