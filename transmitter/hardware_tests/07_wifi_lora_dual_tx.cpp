#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// --- PIN CONFIG ---
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SCK  18
#define LORA_NSS  5
#define LORA_RST  14
#define LORA_DIO0 4

// --- Wi-Fi Settings ---
const char* ssid = "SnakeLink_Node";
const char* password = "password123";
WiFiUDP udp;
const char* ipAddress = "192.168.4.1"; // Receiver's Default AP IP
unsigned int port = 4210;

void setup() {
  Serial.begin(115200);
  
  // 1. Start LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) { Serial.println("LoRa Fail!"); while(1); }
  LoRa.setSyncWord(0xF3);
  LoRa.setTxPower(17);

  // 2. Connect to Receiver Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Receiver Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected!");
}

void loop() {
  static int counter = 1;
  String message = "Packet #" + String(counter);
  
  // --- Send via Wi-Fi ---
  udp.beginPacket(ipAddress, port);
  udp.print(message + " (via Wi-Fi)");
  udp.endPacket();

  // --- Send via LoRa ---
  LoRa.beginPacket();
  LoRa.print(message + " (via LoRa)");
  LoRa.endPacket();

  Serial.println("Sent: " + message);
  counter++;
  delay(2000); 
}