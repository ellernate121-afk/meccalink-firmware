# MeccaLink Firmware 🤖

ESP32 Bluetooth firmware bridge for the **Meccanoid G15KS** — part of the
[MeccaLink AI](https://mecca-bot-link.base44.app/) project.

This firmware runs on an ESP32 and acts as the hardware layer between the
MeccaLink AI app and the robot, handling:

- **Bluetooth SPP** (Classic Bluetooth serial) — receives commands from the app
- **UART serial** — translates and forwards packets to the Meccanoid at 115200 baud
- **Keepalive loop** — prevents the Meccanoid from staling out mid-session

---

## Repo Structure

```
meccalink-firmware/
├── firmware/
│   └── meccanoid_esp32/
│       ├── meccanoid_esp32.ino   ← Main sketch (open this in Arduino IDE)
│       ├── packet.h              ← Packet builder, checksum, command helpers
│       └── config.h              ← Pins, baud rate, timing — edit this first
├── docs/
│   ├── protocol.md               ← Full packet format & command reference
│   └── wiring.md                 ← ESP32 → Meccanoid wiring diagram
└── README.md
```

---

## Quick Start

### 1. Requirements

- [Arduino IDE 2.x](https://www.arduino.cc/en/software)
- [ESP32 board package by Espressif](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)
  - Board manager URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- Select board: **ESP32 Dev Module**

### 2. Wire it up

See [`docs/wiring.md`](docs/wiring.md) for the full diagram. Short version:

```
ESP32 GPIO 17 (TX2) ──[1kΩ]──┬── Meccanoid Data Line
ESP32 GPIO 16 (RX2) ──────────┘
ESP32 GND ───────────────────── Meccanoid GND
```

### 3. Configure

Open `firmware/meccanoid_esp32/config.h` and check:

```cpp
#define MECCA_RX_PIN     16       // adjust if you used different pins
#define MECCA_TX_PIN     17
#define MECCA_BAUD_RATE  115200   // must match your app
#define BT_DEVICE_NAME   "MeccaLink-ESP32"
```

### 4. Flash

1. Open `firmware/meccanoid_esp32/meccanoid_esp32.ino` in Arduino IDE
2. Select your ESP32 COM port
3. Click **Upload**
4. Open Serial Monitor at 115200 to see boot messages

### 5. Pair

On your phone/device, scan for Bluetooth devices and pair with **`MeccaLink-ESP32`**.
No PIN required on most devices.

---

## Command Reference

Commands are sent as plain text strings ending with `\n` (newline).

| Command                     | Effect                                 |
|-----------------------------|----------------------------------------|
| `PING`                      | Health check — replies `PONG`          |
| `S<n>:<pos>`                | Servo n (0–3) to position (0–255)      |
| `SA:<s0>,<s1>,<s2>,<s3>`    | Set all four servos at once            |
| `LED:<r>,<g>,<b>`           | Set chest LED color (0–255 per channel)|
| `SND:<index>`               | Play built-in sound (1–99)             |
| `RST`                       | Reset the Meccanoid                    |

**Examples:**
```
PING
S0:128
S1:200
SA:128,200,60,180
LED:255,0,128
SND:3
RST
```

---

## How the Keepalive Works

The Meccanoid enters a **stale/safe mode** (~200ms timeout) if it stops
receiving serial packets. The firmware automatically sends a neutral servo
packet every **100ms** in the background to keep the connection alive —
you don't need to do anything, it's handled in the main loop.

See [`docs/protocol.md`](docs/protocol.md) for the full packet format.

---

## Related

- **MeccaLink AI App** — [mecca-bot-link.base44.app](https://mecca-bot-link.base44.app/)
- Meccanoid G15KS official product page — [Meccano](https://www.meccano.com)

---

## License

MIT — do whatever you want, just don't make the robot evil.
