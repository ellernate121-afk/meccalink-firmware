#pragma once

// ============================================================
//  MeccaLink Firmware — config.h
//  Edit this file to match your wiring and preferences.
// ============================================================

// --- WiFi Access Point (ESP32 hosts its own hotspot) --------
// No router needed — your phone connects directly to the ESP32.
// Once connected, the web server is always at: http://192.168.4.1
#define AP_SSID      "MeccaLink-RF"    // hotspot name your phone sees
#define AP_PASSWORD  "meccanoid123"    // change if you want (min 8 chars)

// --- Serial (ESP32 UART2 → Meccanoid) -----------------------
#define MECCA_RX_PIN     16       // GPIO connected to Meccanoid data line (receive)
#define MECCA_TX_PIN     17       // GPIO connected to Meccanoid data line (send)
#define MECCA_BAUD_RATE  115200   // Must match your app setting

// --- RF 433MHz transmitter ----------------------------------
// !! IMPORTANT: Must NOT be 17 — that pin is used by MECCA_TX_PIN !!
#define RF_TX_PIN  4              // GPIO wired to 433MHz transmitter DATA pin

// --- Bluetooth ----------------------------------------------
#define BT_DEVICE_NAME   "MeccaLink-ESP32"   // Name visible when pairing

// --- Keepalive timing ---------------------------------------
// The Meccanoid drops into safe/idle mode if it stops receiving
// packets. Send a keepalive at this interval to prevent that.
#define KEEPALIVE_INTERVAL_MS  100   // milliseconds (do not exceed 200ms)

// --- Servo limits -------------------------------------------
#define SERVO_MIN   0x00   // Full reverse / minimum position
#define SERVO_CTR   0x80   // Center / neutral position
#define SERVO_MAX   0xFF   // Full forward / maximum position

// --- Servo index labels (G15KS) -----------------------------
#define SERVO_LEFT_SHOULDER   0
#define SERVO_RIGHT_SHOULDER  1
#define SERVO_LEFT_ELBOW      2
#define SERVO_RIGHT_ELBOW     3

// --- LED ----------------------------------------------------
// Default boot color (R, G, B) — feel free to change
#define LED_BOOT_R  0
#define LED_BOOT_G  80
#define LED_BOOT_B  255
