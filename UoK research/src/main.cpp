#include <Arduino.h>

const int SENSOR_PIN = 34;
const int THRESHOLD = 525; // Keep your calibrated threshold here
const int BIT_TIME = 100;

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT);
  analogReadResolution(12);
  Serial.println("\n--- FSO RECEIVER: PERFECT ALIGNMENT ---");
  Serial.println("Waiting for light...");
}

void loop() {
  // 1. Detect the Start Bit (Laser turns ON, voltage drops)
  if (analogRead(SENSOR_PIN) < THRESHOLD) { 
    String bits = "";
    
    // 2. THE CRITICAL FIX: Skip the Start Bit!
    // Wait 1.5x the bit time to land perfectly in the middle of Data Bit 0
    delay(BIT_TIME + (BIT_TIME / 2)); 

    // 3. Read the 8 actual data bits
    for (int i = 0; i < 8; i++) {
      if (analogRead(SENSOR_PIN) < THRESHOLD) {
        bits += "1";
      } else {
        bits += "0";
      }
      delay(BIT_TIME); // Move forward exactly one bit
    }

    // 4. Decode and Print
    if (bits != "00000000") {
      char c = (char)strtol(bits.c_str(), NULL, 2);
      Serial.print("[DECODED] Bits: "); Serial.print(bits);
      Serial.print(" -> Letter: "); Serial.println(c);
    }
    
    // 5. Cooldown before looking for the next character
    delay(BIT_TIME);
  }
}