/**
 * @file fso_link.cpp
 * @brief FSO Laser Link Implementation
 */

#include "fso_link.h"
#include <Arduino.h>
#include <driver/uart.h>

namespace links {

FSOLink::FSOLink() : rxIndex(0) {
    memset(txBuffer, 0, sizeof(txBuffer));
    memset(rxBuffer, 0, sizeof(rxBuffer));
}

bool FSOLink::begin() {
    // Configure UART2 for FSO
    uart_config_t uart_config = {
        .baud_rate = FSO_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(FSO_UART_NUM, FSO_BUFFER_SIZE * 2, FSO_BUFFER_SIZE * 2, 0, NULL, 0);
    if (err != ESP_OK) {
        Serial.println("[FSO] UART driver install failed");
        return false;
    }

    err = uart_param_config(FSO_UART_NUM, &uart_config);
    if (err != ESP_OK) {
        Serial.println("[FSO] UART config failed");
        return false;
    }

    err = uart_set_pin(FSO_UART_NUM, PIN_FSO_TX, PIN_FSO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        Serial.println("[FSO] UART pin setup failed");
        return false;
    }

    // Configure ADC for envelope monitoring
    analogSetAttenuation(ADC_11db);
    analogReadResolution(12);

    // Ensure laser is OFF at boot (GPIO 17 low)
    pinMode(PIN_FSO_TX, OUTPUT);
    digitalWrite(PIN_FSO_TX, LOW);

    Serial.printf("[FSO] UART2 initialized at %d baud\n", FSO_BAUD_RATE);
    ready = true;
    return true;
}

void FSOLink::end() {
    uart_driver_delete(FSO_UART_NUM);
    digitalWrite(PIN_FSO_TX, LOW);
    ready = false;
}

bool FSOLink::send(const hlink::Packet& pkt) {
    if (!ready) return false;

    int len = pkt.serialize(txBuffer, sizeof(txBuffer));
    if (len <= 0) return false;

    // Write to UART2 (drives laser via 2N7000)
    int written = uart_write_bytes(FSO_UART_NUM, (const char*)txBuffer, len);
    if (written == len) {
        stats.txPackets++;
        stats.txBytes += len;
        stats.lastActivity = millis();
        uart_wait_tx_done(FSO_UART_NUM, pdMS_TO_TICKS(100));
        return true;
    }
    stats.errors++;
    return false;
}

void FSOLink::update() {
    if (!ready) return;

    // Read available bytes from UART2
    size_t available = 0;
    uart_get_buffered_data_len(FSO_UART_NUM, &available);

    while (available > 0 && rxIndex < sizeof(rxBuffer)) {
        uint8_t b;
        int read = uart_read_bytes(FSO_UART_NUM, &b, 1, 0);
        if (read <= 0) break;

        rxBuffer[rxIndex++] = b;
        available--;

        // Check for complete packet
        if (rxIndex >= HLINK_HEADER_SIZE + 1) {
            if (hlink::Packet::isSync(rxBuffer)) {
                processRX();
            } else {
                // Shift buffer to find sync
                memmove(rxBuffer, rxBuffer + 1, rxIndex - 1);
                rxIndex--;
            }
        }
    }

    // Prevent buffer overflow
    if (rxIndex >= sizeof(rxBuffer) - 1) {
        rxIndex = 0;
        stats.dropped++;
    }
}

void FSOLink::processRX() {
    hlink::Packet pkt;
    int consumed = pkt.deserialize(rxBuffer, rxIndex);
    if (consumed > 0) {
        stats.rxPackets++;
        stats.rxBytes += consumed;
        stats.lastActivity = millis();

        // Shift remaining data
        if (rxIndex > (size_t)consumed) {
            memmove(rxBuffer, rxBuffer + consumed, rxIndex - consumed);
        }
        rxIndex -= consumed;

        if (packetHandler) packetHandler(pkt);
    }
}

float FSOLink::readADC() const {
    int raw = analogRead(PIN_FSO_ADC);
    // Convert to voltage (12-bit, 3.3V reference, with 11dB attenuation)
    // With 11dB attenuation, full scale is ~3.3V (actually ~2.6V effective)
    // The bias divider sets idle at 1.65V
    return (raw / 4095.0f) * 3.3f;
}

int8_t FSOLink::getRSSI() const {
    float adc = readADC();
    // Map 1.0V-2.0V range to RSSI-like scale
    // Idle is ~1.65V, strong signal peaks toward 3.3V (but clamped by divider)
    // Actually: signal rides on 1.65V bias, so peak-to-peak matters
    // Simplified: use instantaneous voltage above idle
    float signal = adc - 1.65f;
    if (signal < 0) signal = 0;
    // Map 0-1.65V swing to -90 to -30 dBm equivalent
    int rssi = -90 + (int)(signal / 1.65f * 60.0f);
    if (rssi > -30) rssi = -30;
    return (int8_t)rssi;
}

float FSOLink::getSignalQuality() const {
    float adc = readADC();
    float signal = adc - 1.65f;
    if (signal < 0) signal = 0;
    float quality = signal / 1.0f;  // 1.0V above idle = max
    if (quality > 1.0f) quality = 1.0f;
    return quality;
}

bool FSOLink::isAligned() const {
    return readADC() > (1.65f + 0.2f);  // 200mV above idle = signal detected
}

void FSOLink::setLaser(bool on) {
    digitalWrite(PIN_FSO_TX, on ? HIGH : LOW);
}

void FSOLink::sendRawByte(uint8_t b) {
    uart_write_bytes(FSO_UART_NUM, (const char*)&b, 1);
}

} // namespace links
