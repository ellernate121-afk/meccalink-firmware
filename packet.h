#pragma once
#include <Arduino.h>
#include "config.h"

// ============================================================
//  MeccaLink Firmware — packet.h
//  Packet construction, checksum, and Meccanoid command helpers.
//
//  Packet format:
//    [ 0xFF ][ LENGTH ][ CMD ][ DATA... ][ CHECKSUM ]
//
//  Checksum = XOR of all bytes after the 0xFF start byte.
// ============================================================

// --- Command bytes ------------------------------------------
#define CMD_SERVO_UPDATE   0x01
#define CMD_LED_COLOR      0x05
#define CMD_SOUND_PLAY     0x06
#define CMD_RESET          0x0F

// --- Max packet payload size --------------------------------
#define MAX_PAYLOAD  16

// ------------------------------------------------------------
//  calcChecksum
//  XOR all bytes in the slice [data, data+len)
// ------------------------------------------------------------
inline uint8_t calcChecksum(const uint8_t* data, uint8_t len) {
  uint8_t chk = 0;
  for (uint8_t i = 0; i < len; i++) chk ^= data[i];
  return chk;
}

// ------------------------------------------------------------
//  sendPacket
//  Builds and transmits a complete Meccanoid packet over Serial2.
//  cmd      — command byte (see CMD_* above)
//  payload  — pointer to data bytes
//  payloadLen — number of data bytes
// ------------------------------------------------------------
inline void sendPacket(uint8_t cmd, const uint8_t* payload, uint8_t payloadLen) {
  // Guard: refuse oversized payloads
  if (payloadLen > MAX_PAYLOAD) return;

  uint8_t packet[MAX_PAYLOAD + 4];
  packet[0] = 0xFF;                        // start byte
  packet[1] = payloadLen + 1;              // length = cmd byte + data
  packet[2] = cmd;
  memcpy(&packet[3], payload, payloadLen);

  // Checksum covers everything after 0xFF (index 1 onward)
  packet[3 + payloadLen] = calcChecksum(&packet[1], payloadLen + 2);

  Serial2.write(packet, 4 + payloadLen);
}

// ------------------------------------------------------------
//  sendKeepalive
//  Holds all servos at center. Must be called every
//  KEEPALIVE_INTERVAL_MS to prevent the Meccanoid staling out.
// ------------------------------------------------------------
inline void sendKeepalive() {
  uint8_t servos[4] = {
    SERVO_CTR,   // servo 0
    SERVO_CTR,   // servo 1
    SERVO_CTR,   // servo 2
    SERVO_CTR    // servo 3
  };
  sendPacket(CMD_SERVO_UPDATE, servos, 4);
}

// ------------------------------------------------------------
//  setServo
//  Move one servo to a position (0x00–0xFF).
//  All other servos stay at center.
//  servoIndex — 0–3
//  position   — SERVO_MIN / SERVO_CTR / SERVO_MAX or any byte
// ------------------------------------------------------------
inline void setServo(uint8_t servoIndex, uint8_t position) {
  uint8_t servos[4] = { SERVO_CTR, SERVO_CTR, SERVO_CTR, SERVO_CTR };
  if (servoIndex < 4) servos[servoIndex] = position;
  sendPacket(CMD_SERVO_UPDATE, servos, 4);
}

// ------------------------------------------------------------
//  setAllServos
//  Set all four servos in one packet.
// ------------------------------------------------------------
inline void setAllServos(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3) {
  uint8_t servos[4] = { s0, s1, s2, s3 };
  sendPacket(CMD_SERVO_UPDATE, servos, 4);
}

// ------------------------------------------------------------
//  setLEDColor
//  Set the Meccanoid's chest LED to an RGB color.
// ------------------------------------------------------------
inline void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t colors[3] = { r, g, b };
  sendPacket(CMD_LED_COLOR, colors, 3);
}

// ------------------------------------------------------------
//  playSound
//  Trigger a built-in sound by index (0x01–0x63 on G15KS).
// ------------------------------------------------------------
inline void playSound(uint8_t soundIndex) {
  uint8_t data[1] = { soundIndex };
  sendPacket(CMD_SOUND_PLAY, data, 1);
}

// ------------------------------------------------------------
//  resetMeccanoid
//  Send a full reset packet — use sparingly.
// ------------------------------------------------------------
inline void resetMeccanoid() {
  uint8_t data[1] = { 0x00 };
  sendPacket(CMD_RESET, data, 1);
}
