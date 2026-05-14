#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

#define FSO_PIN 34
#define BAUD 300
#define BIT_TIME (1000000 / BAUD)
#define THRESHOLD 1100

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_NSS 5
#define LORA_RST 14
#define LORA_DIO0 4

void setup() {
  Serial.begin(115200);
  delay(2000);
  analogReadResolution(12);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    while (1);
  }
  LoRa.setSyncWord(0xF3);
}

char readFSOByte() {
  if (analogRead(FSO_PIN) > THRESHOLD) return 0;

  delayMicroseconds(BIT_TIME / 2);
  if (analogRead(FSO_PIN) > THRESHOLD) return 0;

  char data = 0;
  for (int i = 0; i < 8; i++) {
    delayMicroseconds(BIT_TIME);
    if (analogRead(FSO_PIN) > THRESHOLD) {
      data |= (1 << i);
    }
  }
  delayMicroseconds(BIT_TIME);
  return data;
}

void loop() {
  char c = readFSOByte();
  if (c > 0) {
    Serial.print(c);
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    Serial.println();
  }
}