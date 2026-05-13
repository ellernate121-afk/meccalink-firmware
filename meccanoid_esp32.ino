// ============================================================
//  MeccaLink Firmware — meccanoid_esp32.ino
//
//  Bridges the MeccaLink AI app (Classic Bluetooth SPP) to the
//  Meccanoid G15KS over UART2 at 115200 baud.
//  Also runs a WiFi AP web server for RF 433MHz transmit commands.
//
//  Repo   : meccalink-firmware
//  Board  : ESP32 (any standard variant)
//  IDE    : Arduino IDE 2.x + esp32 board package by Espressif
//
//  Required libraries (install via Arduino Library Manager):
//    - ArduinoJson  (by Benoit Blanchon)
// ============================================================

#include <BluetoothSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "packet.h"

// --- Bluetooth serial instance ------------------------------
BluetoothSerial SerialBT;

// --- WiFi web server (port 80) ------------------------------
WebServer wifiServer(80);

// --- State --------------------------------------------------
unsigned long lastKeepalive = 0;
String        btBuffer      = "";

// ============================================================
//  RF transmit (433MHz ASK) — WiFi web server handlers
// ============================================================

void transmitASK(String hexData, int durationMs = 200) {
  digitalWrite(RF_TX_PIN, HIGH);
  delay(durationMs);
  digitalWrite(RF_TX_PIN, LOW);
  delay(50);
  Serial.println("[RF] Transmitted: " + hexData);
}

// POST /transmit  { "frequency":"433e6", "modulation":"ASK", "data":"..." }
void handleTransmit() {
  if (!wifiServer.hasArg("plain")) {
    wifiServer.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, wifiServer.arg("plain"));
  if (err) {
    wifiServer.send(400, "application/json", "{\"error\":\"Bad JSON\"}");
    return;
  }
  String frequency  = doc["frequency"]  | "433e6";
  String modulation = doc["modulation"] | "ASK";
  String data       = doc["data"]       | "";

  Serial.printf("[RF] TX → freq:%s mod:%s data:%s\n",
    frequency.c_str(), modulation.c_str(), data.c_str());

  transmitASK(data);
  wifiServer.send(200, "application/json",
    "{\"success\":true,\"transmitted\":true,\"data\":\"" + data + "\"}");
}

// GET /  — health check
void handleHealth() {
  wifiServer.send(200, "application/json",
    "{\"status\":\"ok\",\"device\":\"meccalink-esp32\",\"bt\":\"" +
    String(BT_DEVICE_NAME) + "\"}");
}

// ============================================================
//  setup
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("[MeccaLink] Booting...");

  // UART2 → Meccanoid
  Serial2.begin(MECCA_BAUD_RATE, SERIAL_8N1, MECCA_RX_PIN, MECCA_TX_PIN);
  Serial.println("[MeccaLink] UART2 ready on RX=" + String(MECCA_RX_PIN) +
                 " TX=" + String(MECCA_TX_PIN));

  // RF TX pin
  pinMode(RF_TX_PIN, OUTPUT);
  digitalWrite(RF_TX_PIN, LOW);
  Serial.println("[MeccaLink] RF TX pin " + String(RF_TX_PIN) + " ready");

  // WiFi — Access Point mode (ESP32 is its own hotspot, no router needed)
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println("[MeccaLink] Hotspot started!");
  Serial.println("[MeccaLink] Connect your phone to WiFi: " + String(AP_SSID));
  Serial.println("[MeccaLink] Web server always at: http://192.168.4.1");

  // Web server routes
  wifiServer.on("/",         HTTP_GET,  handleHealth);
  wifiServer.on("/transmit", HTTP_POST, handleTransmit);
  wifiServer.begin();
  Serial.println("[MeccaLink] RF web server running on port 80");

  // Classic Bluetooth SPP
  SerialBT.begin(BT_DEVICE_NAME);
  Serial.println("[MeccaLink] Bluetooth ready — pair with: " + String(BT_DEVICE_NAME));

  // First keepalive + boot LED
  delay(500);
  sendKeepalive();
  lastKeepalive = millis();
  setLEDColor(LED_BOOT_R, LED_BOOT_G, LED_BOOT_B);
  Serial.println("[MeccaLink] Fully ready.");
}

// ============================================================
//  loop
// ============================================================
void loop() {

  // --- 0. Handle incoming WiFi/RF web requests --------------
  wifiServer.handleClient();

  // --- 1. Keepalive (prevents Meccanoid staling out) --------
  if (millis() - lastKeepalive >= KEEPALIVE_INTERVAL_MS) {
    sendKeepalive();
    lastKeepalive = millis();
  }

  // --- 2. Read and process Bluetooth commands ---------------
  while (SerialBT.available()) {
    char c = (char)SerialBT.read();
    if (c == '\n') {
      btBuffer.trim();
      if (btBuffer.length() > 0) handleCommand(btBuffer);
      btBuffer = "";
    } else {
      btBuffer += c;
    }
  }

  // --- 3. Echo debug input from USB monitor (dev only) ------
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      btBuffer.trim();
      if (btBuffer.length() > 0) handleCommand(btBuffer);
      btBuffer = "";
    } else {
      btBuffer += c;
    }
  }
}

// ============================================================
//  handleCommand
//  S<n>:<pos>            — single servo (index 0-3, pos 0-255)
//  SA:<s0>,<s1>,<s2>,<s3> — all four servos at once
//  LED:<r>,<g>,<b>       — chest LED color (0-255 each)
//  SND:<index>           — play built-in sound (1-99)
//  RST                   — reset the Meccanoid
//  PING                  — health check, replies PONG
// ============================================================
void handleCommand(const String& cmd) {
  Serial.println("[CMD] " + cmd);

  if (cmd == "PING") {
    SerialBT.println("PONG");
    return;
  }
  if (cmd == "RST") {
    resetMeccanoid();
    SerialBT.println("OK:RST");
    return;
  }
  if (cmd.startsWith("SA:")) {
    String args = cmd.substring(3);
    uint8_t vals[4] = { SERVO_CTR, SERVO_CTR, SERVO_CTR, SERVO_CTR };
    uint8_t idx = 0;
    int start = 0;
    for (int i = 0; i <= (int)args.length() && idx < 4; i++) {
      if (i == (int)args.length() || args[i] == ',') {
        vals[idx++] = (uint8_t)constrain(args.substring(start, i).toInt(), 0, 255);
        start = i + 1;
      }
    }
    setAllServos(vals[0], vals[1], vals[2], vals[3]);
    SerialBT.println("OK:SA");
    return;
  }
  if (cmd.startsWith("S") && cmd.indexOf(':') > 0) {
    int colon = cmd.indexOf(':');
    uint8_t servoIdx = (uint8_t)cmd.substring(1, colon).toInt();
    uint8_t position = (uint8_t)constrain(cmd.substring(colon + 1).toInt(), 0, 255);
    if (servoIdx > 3) {
      SerialBT.println("ERR:servo index out of range (0-3)");
      return;
    }
    setServo(servoIdx, position);
    SerialBT.println("OK:S" + String(servoIdx) + "=" + String(position));
    return;
  }
  if (cmd.startsWith("LED:")) {
    String args = cmd.substring(4);
    int c1 = args.indexOf(',');
    int c2 = args.lastIndexOf(',');
    if (c1 < 0 || c2 < 0 || c1 == c2) {
      SerialBT.println("ERR:LED format is LED:R,G,B");
      return;
    }
    uint8_t r = (uint8_t)constrain(args.substring(0, c1).toInt(), 0, 255);
    uint8_t g = (uint8_t)constrain(args.substring(c1 + 1, c2).toInt(), 0, 255);
    uint8_t b = (uint8_t)constrain(args.substring(c2 + 1).toInt(), 0, 255);
    setLEDColor(r, g, b);
    SerialBT.println("OK:LED");
    return;
  }
  if (cmd.startsWith("SND:")) {
    uint8_t sndIdx = (uint8_t)constrain(cmd.substring(4).toInt(), 1, 99);
    playSound(sndIdx);
    SerialBT.println("OK:SND=" + String(sndIdx));
    return;
  }

  SerialBT.println("ERR:unknown command — " + cmd);
}
