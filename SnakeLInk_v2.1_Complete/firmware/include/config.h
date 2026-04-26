/**
 * @file config.h
 * @brief SnakeLink v2.0 Configuration & Pin Definitions
 * @description Master configuration adopting Corrected Guide v2 pinout.
 *              All values are compile-time defaults; runtime overrides via NVS.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <cstdint>

// ============================================================================
// VERSION
// ============================================================================
#define SNAKELINK_VERSION_MAJOR 2
#define SNAKELINK_VERSION_MINOR 0
#define SNAKELINK_VERSION_PATCH 0

// ============================================================================
// HARDWARE PIN DEFINITIONS (Corrected Guide v2 — LOCKED)
// ============================================================================

// FSO (Free-Space Optical) — UART2
#define PIN_FSO_RX          GPIO_NUM_16     // UART2 RX (from LM393 OUT)
#define PIN_FSO_TX          GPIO_NUM_17     // UART2 TX (to 2N7000 Gate)
#define PIN_FSO_ADC         GPIO_NUM_34     // ADC1_CH6 — FSO envelope signal strength
#define PIN_FSO_ALIGN_BTN   GPIO_NUM_32     // Button 1 — FSO alignment mode (active-low, pull-up)

// LoRa (EMERGENCY FALLBACK) (SX1278 via Ra-02 module) — VSPI
#define PIN_LORA_SCK        GPIO_NUM_18     // SPI Clock
#define PIN_LORA_MISO       GPIO_NUM_19     // SPI MISO
#define PIN_LORA_MOSI       GPIO_NUM_23     // SPI MOSI
#define PIN_LORA_NSS        GPIO_NUM_5      // SPI Chip Select
#define PIN_LORA_DIO0       GPIO_NUM_4      // Packet reception interrupt
#define PIN_LORA_RST        GPIO_NUM_14     // Module reset (NEW in v2)

// OLED Display (SSD1306) — I2C
#define PIN_OLED_SDA        GPIO_NUM_21     // I2C Data
#define PIN_OLED_SCL        GPIO_NUM_22     // I2C Clock
#define OLED_I2C_ADDR       0x3C
#define OLED_WIDTH          128
#define OLED_HEIGHT         64

// Battery Monitoring
#define PIN_BATTERY_ADC     GPIO_NUM_35     // ADC1_CH7 — 2:1 divider (100k/100k)

// Feedback Outputs
#define PIN_BUZZER          GPIO_NUM_25     // PWM/LEDC — active buzzer
#define PIN_LED_GREEN       GPIO_NUM_26     // PWM — power/health
#define PIN_LED_BLUE        GPIO_NUM_27     // PWM — active link indicator
#define PIN_LED_RED         GPIO_NUM_33     // PWM — alert/data

// Buttons
#define PIN_MODE_BTN        GPIO_NUM_13     // Button 2 — mode switch / emergency (active-low, pull-up)

// ============================================================================
// POWER & BATTERY CONFIGURATION
// ============================================================================
#define BATTERY_DIVIDER_RATIO   2.0f        // 100k/100k divider
#define BATTERY_ADC_ATTEN       ADC_ATTEN_DB_12
#define BATTERY_ADC_WIDTH       ADC_WIDTH_BIT_12
#define BATTERY_VOLTAGE_MAX     4.20f
#define BATTERY_VOLTAGE_MIN     3.10f
#define BATTERY_VOLTAGE_WARN    3.50f       // 25% — triggers warning
#define BATTERY_VOLTAGE_CRIT    3.30f       // 10% — triggers critical

static inline uint8_t voltageToPercent(float v) {
    if (v >= 4.20f) return 100;
    if (v >= 3.90f) return 75 + (uint8_t)((v - 3.90f) / 0.30f * 25.0f);
    if (v >= 3.70f) return 50 + (uint8_t)((v - 3.70f) / 0.20f * 25.0f);
    if (v >= 3.50f) return 25 + (uint8_t)((v - 3.50f) / 0.20f * 25.0f);
    if (v >= 3.30f) return 10 + (uint8_t)((v - 3.30f) / 0.20f * 15.0f);
    return 0;
}

// ============================================================================
// COMMUNICATION CONFIGURATION
// ============================================================================

// Wi-Fi / ESP-NOW (BACKUP LINK)
#define WIFI_AP_SSID_PREFIX     "SnakeLink"
#define WIFI_AP_PASS            "Tactical2026"
#define WIFI_AP_CHANNEL         6
#define WIFI_AP_MAX_CONN        1
#define ESPNOW_PEER_MAX         1
#define ESPNOW_CHANNEL          6

// UDP (Laptop ↔ ESP32)
#define UDP_LOCAL_PORT          4210
#define UDP_REMOTE_PORT         4210
#define UDP_BUFFER_SIZE         512
#define UDP_PACKET_TIMEOUT_MS   3000

// LoRa (EMERGENCY FALLBACK)
#define LORA_FREQUENCY          433E6
#define LORA_SPREADING_FACTOR   7           // SF7 = fast, SF12 = long range
#define LORA_SIGNAL_BANDWIDTH   125E3
#define LORA_CODING_RATE        5           // 4/5
#define LORA_TX_POWER           20          // dBm
#define LORA_PREAMBLE_LEN       8

// FSO UART (PRIMARY LINK)
#define FSO_UART_NUM            UART_NUM_2
#define FSO_BAUD_RATE           115200
#define FSO_BUFFER_SIZE         256

// ============================================================================
// H-LINK PROTOCOL CONFIGURATION
// ============================================================================
#define HLINK_SYNC_BYTE_1       0xAA
#define HLINK_SYNC_BYTE_2       0x55
#define HLINK_MAX_PAYLOAD       240
#define HLINK_MAX_PACKET_SIZE   (2 + 1 + 2 + 2 + HLINK_MAX_PAYLOAD + 1) // 248 bytes
#define HLINK_SEQ_MAX           65535

// ARQ Timeouts
#define ARQ_TIMEOUT_FSO_MS      200
#define ARQ_TIMEOUT_WIFI_MS     200
#define ARQ_TIMEOUT_LORA_MS     2000
#define ARQ_MAX_RETRIES         3

// CRC-8 (ITU polynomial 0x07)
#define CRC8_POLYNOMIAL         0x07
#define CRC8_INITIAL            0x00

// ============================================================================
// FSM THRESHOLDS & HYSTERESIS
// ============================================================================

// Wi-Fi RSSI thresholds (dBm)
#define WIFI_RSSI_DEGRADE       -80         // Switch AWAY from Wi-Fi
#define WIFI_RSSI_RECOVER       -75         // Switch BACK to Wi-Fi
#define WIFI_RSSI_STABLE_MS     5000        // 5 seconds continuous

// FSO ADC voltage thresholds (Volts)
#define FSO_VOLTAGE_DEGRADE     1.0f        // Switch AWAY from FSO
#define FSO_VOLTAGE_RECOVER     1.5f        // Switch BACK to FSO
#define FSO_VOLTAGE_STABLE_MS   5000

// CRC Error Rate thresholds (%)
#define CRC_ERROR_DEGRADE       30          // Switch away if >30%
#define CRC_ERROR_RECOVER       5           // Switch back if <5%
#define CRC_ERROR_WINDOW        20          // Sliding window size

// Latency / Timeout thresholds
#define ACK_TIMEOUT_CONSECUTIVE 3           // 3 consecutive ACK timeouts = degradation

// Make-Before-Break
#define MBB_WARNING_DURATION_MS 3000        // Duplication window: 3 seconds max
#define MBB_DUPLICATE_MAX_AGE_MS 500        // Drop duplicates older than 500ms

// ============================================================================
// TELEMETRY SCHEDULE
// ============================================================================
#define TELEM_HEARTBEAT_MS      5000
#define TELEM_GPS_MS            30000
#define TELEM_BATTERY_MS        60000       // 60-90s range, we use 60s
#define TELEM_LINK_STATUS_MS    10000

// ============================================================================
// DEVICE & SECURITY
// ============================================================================
#define DEVICE_ID_LEN           8
#define DEVICE_KEY_LEN          32          // 256 bits
#define DEVICE_KEY_FINGER_LEN   2
#define ANTI_REPLAY_WINDOW_MS   5000
#define MAX_DEVICE_NAME_LEN     16

// ============================================================================
// DISPLAY & FEEDBACK
// ============================================================================
#define DISPLAY_UPDATE_MS       100
#define LED_PWM_FREQ            5000
#define LED_PWM_RES             8
#define BUZZER_PWM_FREQ         2000
#define BUZZER_PWM_RES          8
#define BUTTON_DEBOUNCE_MS      50
#define BUTTON_LONG_PRESS_MS    1500

// ============================================================================
// TASK CONFIGURATION (FreeRTOS)
// ============================================================================
#define TASK_DATA_STACK         8192
#define TASK_DATA_PRIORITY      configMAX_PRIORITIES - 1  // Highest
#define TASK_DATA_CORE          1

#define TASK_BG_STACK           4096
#define TASK_BG_PRIORITY        5
#define TASK_BG_CORE            0

#define TASK_UI_STACK           4096
#define TASK_UI_PRIORITY        3
#define TASK_UI_CORE            0

// ============================================================================
// LOGGING
// ============================================================================
#define LOG_LEVEL_NONE          0
#define LOG_LEVEL_ERROR         1
#define LOG_LEVEL_WARN          2
#define LOG_LEVEL_INFO          3
#define LOG_LEVEL_DEBUG         4
#define LOG_LEVEL_VERBOSE       5

#ifndef LOG_LEVEL
#define LOG_LEVEL               LOG_LEVEL_INFO
#endif

#define LOG_BUFFER_SIZE         256
#define LOG_FILE_PATH           "/spiffs/system.log"
#define LOG_FILE_MAX_SIZE       32768       // 32KB max log file

// ============================================================================
// UTILITY MACROS
// ============================================================================
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define CONSTRAIN(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))
#define MAP_RANGE(x, in_min, in_max, out_min, out_max)     (((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min))

#endif // CONFIG_H
