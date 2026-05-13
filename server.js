// ============================================================
//  MeccaLink Relay Server — server.js
//
//  Runs on Render. Acts as the middleman between the Base44
//  app and the MeccaLink ESP32 on the local hotspot.
//
//  Base44 → POST https://your-render-url.onrender.com/transmit
//         → relays to ESP32 at http://ESP32_IP/transmit
// ============================================================

const express = require("express");
const axios   = require("axios");
const cors    = require("cors");

const app  = express();
const PORT = process.env.PORT || 3000;

// --- ESP32 address ------------------------------------------
// When ESP32 is in AP mode:       http://192.168.4.1
// When ESP32 is in Station mode:  http://192.168.x.x (check Serial Monitor)
const ESP32_IP = process.env.ESP32_IP || "192.168.4.1";
const ESP32_URL = `http://${ESP32_IP}`;

// --- Middleware ----------------------------------------------
app.use(cors());
app.use(express.json());

// ============================================================
//  GET /
//  Health check — also pings the ESP32 and reports its status
// ============================================================
app.get("/", async (req, res) => {
  let esp32Status = "unreachable";
  let esp32Data   = null;

  try {
    const response = await axios.get(ESP32_URL, { timeout: 3000 });
    esp32Status = "online";
    esp32Data   = response.data;
  } catch (err) {
    esp32Status = "offline — " + err.message;
  }

  res.json({
    status:      "ok",
    relay:       "meccalink-render-relay",
    esp32_ip:    ESP32_IP,
    esp32_status: esp32Status,
    esp32_data:  esp32Data
  });
});

// ============================================================
//  POST /transmit
//  Accepts a command from Base44 and forwards it to the ESP32.
//
//  Body (JSON):
//    { "frequency": "433e6", "modulation": "ASK", "data": "..." }
//
//  Also accepts MeccaLink direct commands:
//    { "command": "S1:200" }   ← forwarded as BT command string
// ============================================================
app.post("/transmit", async (req, res) => {
  const body = req.body;

  if (!body || Object.keys(body).length === 0) {
    return res.status(400).json({ error: "No body provided" });
  }

  console.log("[Relay] Forwarding to ESP32:", body);

  try {
    const response = await axios.post(`${ESP32_URL}/transmit`, body, {
      headers: { "Content-Type": "application/json" },
      timeout: 5000
    });

    console.log("[Relay] ESP32 response:", response.data);
    res.json({
      success:  true,
      relayed:  true,
      esp32_ip: ESP32_IP,
      result:   response.data
    });

  } catch (err) {
    console.error("[Relay] ESP32 unreachable:", err.message);
    res.status(502).json({
      success:  false,
      error:    "ESP32 unreachable",
      esp32_ip: ESP32_IP,
      detail:   err.message
    });
  }
});

// ============================================================
//  POST /command
//  Shorthand for sending a raw MeccaLink BT command string.
//  Body: { "command": "LED:255,0,0" }
// ============================================================
app.post("/command", async (req, res) => {
  const { command } = req.body;

  if (!command) {
    return res.status(400).json({ error: "No command provided" });
  }

  console.log("[Relay] BT command:", command);

  try {
    const response = await axios.post(`${ESP32_URL}/transmit`, {
      modulation: "BT",
      data: command
    }, {
      headers: { "Content-Type": "application/json" },
      timeout: 5000
    });

    res.json({ success: true, command, result: response.data });

  } catch (err) {
    res.status(502).json({
      success: false,
      error:   "ESP32 unreachable",
      detail:  err.message
    });
  }
});

// ============================================================
//  Start
// ============================================================
app.listen(PORT, () => {
  console.log(`[MeccaLink Relay] Server running on port ${PORT}`);
  console.log(`[MeccaLink Relay] Forwarding to ESP32 at: ${ESP32_URL}`);
  console.log(`[MeccaLink Relay] Set ESP32_IP env var on Render to override`);
});
