/**
 * @file lora_link.cpp
 * @brief LoRa Link Implementation
 */

#include "lora_link.h"
#include <Arduino.h>

namespace links {

LoRaLink::LoRaLink() : txPending(false), lastRSSI(-120.0f), lastSNR(-20.0f), rxIndex(0) {
    memset(txBuffer, 0, sizeof(txBuffer));
    memset(rxBuffer, 0, sizeof(rxBuffer));
}

bool LoRaLink::begin() {
    // Configure SPI pins
    SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);

    // Reset LoRa module
    pinMode(PIN_LORA_RST, OUTPUT);
    digitalWrite(PIN_LORA_RST, LOW);
    delay(10);
    digitalWrite(PIN_LORA_RST, HIGH);
    delay(10);

    // Initialize LoRa
    if (!LoRa.begin(LORA_FREQUENCY)) {
        Serial.println("[LoRa] Module init failed");
        return false;
    }

    LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
    LoRa.setSignalBandwidth(LORA_SIGNAL_BANDWIDTH);
    LoRa.setCodingRate4(LORA_CODING_RATE);
    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.setPreambleLength(LORA_PREAMBLE_LEN);
    LoRa.enableCrc();

    // Set callback for receive
    LoRa.onReceive([](int size) {
        // This is tricky with ESP32 LoRa library - we poll instead
    });

    Serial.printf("[LoRa] Initialized at %.0f MHz, SF%d, BW%.0f kHz\n",
                  LORA_FREQUENCY/1E6, LORA_SPREADING_FACTOR, LORA_SIGNAL_BANDWIDTH/1E3);

    ready = true;
    return true;
}

void LoRaLink::end() {
    LoRa.end();
    ready = false;
}

bool LoRaLink::send(const hlink::Packet& pkt) {
    if (!ready || txPending) return false;

    int len = pkt.serialize(txBuffer, sizeof(txBuffer));
    if (len <= 0) return false;

    // LoRa has limited payload - check size
    if (len > 255) {
        stats.errors++;
        return false;
    }

    LoRa.beginPacket();
    LoRa.write(txBuffer, len);
    if (LoRa.endPacket()) {
        txPending = true;
        stats.txPackets++;
        stats.txBytes += len;
        stats.lastActivity = millis();

        // Wait for TX done (blocking, but LoRa is slow anyway)
        // In production, use non-blocking with DIO0 interrupt
        delay(10);
        txPending = false;
        return true;
    }

    stats.errors++;
    return false;
}

void LoRaLink::update() {
    if (!ready) return;

    // Poll for received packets
    int packetSize = LoRa.parsePacket();
    if (packetSize > 0) {
        lastRSSI = LoRa.packetRssi();
        lastSNR = LoRa.packetSnr();

        rxIndex = 0;
        while (LoRa.available() && rxIndex < sizeof(rxBuffer)) {
            rxBuffer[rxIndex++] = LoRa.read();
        }

        if (rxIndex > 0) {
            processRX();
        }
    }
}

void LoRaLink::processRX() {
    // Scan for sync bytes
    size_t i = 0;
    while (i < rxIndex - 1) {
        if (hlink::Packet::isSync(&rxBuffer[i])) {
            hlink::Packet pkt;
            int consumed = pkt.deserialize(&rxBuffer[i], rxIndex - i);
            if (consumed > 0) {
                stats.rxPackets++;
                stats.rxBytes += consumed;
                stats.lastActivity = millis();

                if (packetHandler) packetHandler(pkt);
                i += consumed;
                continue;
            }
        }
        i++;
    }
}

int8_t LoRaLink::getRSSI() const {
    return static_cast<int8_t>(lastRSSI);
}

float LoRaLink::getSignalQuality() const {
    // Map RSSI -120 to -30 to 0.0-1.0
    float rssi = lastRSSI;
    if (rssi >= -50) return 1.0f;
    if (rssi <= -120) return 0.0f;
    return (rssi + 120) / 70.0f;
}

} // namespace links
