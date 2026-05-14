#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// FSO Pin Definitions
#define FSO_RX_PIN 34
#define FSO_TX_PIN 17

// FSO Protocol Configuration
#define BAUD 300
#define BIT_TIME (1000000 / BAUD)
#define THRESHOLD 1100

// LoRa Pin Definitions
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_NSS 5
#define LORA_RST 14
#define LORA_DIO0 4

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // Configure FSO RX
  analogReadResolution(12);
  
  // Configure FSO TX
  pinMode(FSO_TX_PIN, OUTPUT);
  digitalWrite(FSO_TX_PIN, LOW); // Ensure laser is OFF at boot

  // Initialize LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa initialization failed. Check wiring.");
    while (1);
  }
  LoRa.setSyncWord(0xF3);
  
  Serial.println("Hybrid Transceiver Node Ready.");
}

// --- FSO Transmit Protocol ---
void writeFSOByte(char data) {
  // 1. Start Bit (Logic 0) -> Turn Laser ON
  // RX node sees a voltage drop <= THRESHOLD, triggering the read sequence.
  digitalWrite(FSO_TX_PIN, HIGH);
  delayMicroseconds(BIT_TIME);

  // 2. Data Bits (LSB first)
  for (int i = 0; i < 8; i++) {
    if (data & (1 << i)) {
      // Transmit 1 -> Turn Laser OFF -> RX sees > THRESHOLD
      digitalWrite(FSO_TX_PIN, LOW); 
    } else {
      // Transmit 0 -> Turn Laser ON -> RX sees <= THRESHOLD
      digitalWrite(FSO_TX_PIN, HIGH);
    }
    delayMicroseconds(BIT_TIME);
  }

  // 3. Stop Bit (Logic 1) -> Turn Laser OFF (Return to Idle)
  digitalWrite(FSO_TX_PIN, LOW);
  delayMicroseconds(BIT_TIME);
}

void transmitFSOMessage(const char* message) {
  while (*message) {
    writeFSOByte(*message++);
  }
}

// --- FSO Receive Protocol ---
char readFSOByte() {
  // Idle state is voltage > THRESHOLD
  if (analogRead(FSO_RX_PIN) > THRESHOLD) return 0; 

  // Half-bit delay to confirm it's a real Start Bit and not noise
  delayMicroseconds(BIT_TIME / 2);
  if (analogRead(FSO_RX_PIN) > THRESHOLD) return 0; 

  char data = 0;
  for (int i = 0; i < 8; i++) {
    delayMicroseconds(BIT_TIME);
    if (analogRead(FSO_RX_PIN) > THRESHOLD) {
      data |= (1 << i); // If voltage is high (laser off), record a 1
    }
  }
  
  // Wait out the Stop Bit before returning
  delayMicroseconds(BIT_TIME); 
  return data;
}

void loop() {
  // 1. Listen for incoming FSO data (Blocking)
  char c = readFSOByte();
  if (c > 0) {
    Serial.print(c);
  }

  // 2. Listen for incoming LoRa data (Non-blocking)
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("\n[LoRa RX]: ");
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    Serial.println();
  }

  // 3. Serial Monitor Passthrough -> Transmit via FSO
  // Type a message in the Serial monitor and hit enter to fire the laser
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    if (msg.length() > 0) {
      Serial.print("\n[FSO TX]: ");
      Serial.println(msg);
      transmitFSOMessage(msg.c_str());
    }
  }
}