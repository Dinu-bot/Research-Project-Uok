/* * TEST B: Wi-Fi Power Brownout (SoftAP)
 * WHAT IT CHECKS: Verifies that your custom 5V power architecture (the MT3608 
 * and battery pack) can survive sudden, massive current spikes.
 * HOW IT WORKS: When the ESP32 is commanded to broadcast a Wi-Fi network, it 
 * instantly fires up its internal 2.4GHz radio amplifier. This pulls 
 * a sudden transient spike of up to 500mA. If the power regulator is 
 * weak or wiring is loose, the voltage collapses and the ESP32 
 * hardware reboots itself (a brownout).
 */
#include <Arduino.h>
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n--- Wi-Fi Access Point (SoftAP) Test ---");
  Serial.println("Firing up the Wi-Fi Radio...");
  
  // Set the ESP32 to broadcast its own network
  // Syntax: WiFi.softAP(SSID, Password)
  // Note: Password must still be at least 8 characters!
  bool success = WiFi.softAP("ESP32_FSO_Test", "12345678");
  
  if (success) {
    Serial.println("\nSuccess! The power supply held stable.");
    Serial.println("The ESP32 is now broadcasting a Wi-Fi network.");
    Serial.print("ESP32 Local IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("\nFailed to start the Access Point.");
  }
}

void loop() {
  // Keep the board idle while you check your phone
  delay(10000); 
}
