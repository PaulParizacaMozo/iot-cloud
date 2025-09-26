import express from "express";
import http from "http";
import { Server as SocketIOServer } from "socket.io";
import mqtt from "mqtt";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// === Broker en tu LAN ===
const MQTT_URL = process.env.MQTT_URL || "mqtt://10.7.135.36:1883";
const MQTT_USER = process.env.MQTT_USER || "";
const MQTT_PASS = process.env.MQTT_PASS || "";
const MQTT_TOPIC = process.env.MQTT_TOPIC || "esp32/temperatura";
// ========================

const app = express();
const server = http.createServer(app);
const io = new SocketIOServer(server, { cors: { origin: "*" } });

// Memoria simple de últimas lecturas
const MAX_READINGS = 200;
const readings = []; // { ts, temp, hum, raw }
const pushReading = (r) => {
  readings.push(r);
  if (readings.length > MAX_READINGS) readings.shift();
};

// Conexión MQTT
const mqttClient = mqtt.connect(MQTT_URL, {
  username: MQTT_USER || undefined,
  password: MQTT_PASS || undefined
});

mqttClient.on("connect", () => {
  console.log("[MQTT] Conectado a", MQTT_URL);
  mqttClient.subscribe(MQTT_TOPIC, (err) => {
    if (err) console.error("[MQTT] Error suscripción:", err.message);
    else console.log(`[MQTT] Suscrito a ${MQTT_TOPIC}`);
  });
});

mqttClient.on("error", (err) => console.error("[MQTT] Error:", err.message));

mqttClient.on("message", (_topic, payload) => {
  try {
    const data = JSON.parse(payload.toString());
    const reading = {
      ts: Number.isFinite(data.ts) ? data.ts : Date.now(),
      temp: typeof data.temp === "number" ? data.temp : null,
      hum: typeof data.hum === "number" ? data.hum : null,
      raw: data
    };
    pushReading(reading);
    io.emit("reading", reading); // broadcast al navegador
  } catch (e) {
    console.warn("[MQTT] Mensaje inválido:", payload.toString());
  }
});

// Estáticos + API
app.use(express.static(path.join(__dirname, "public")));
app.get("/api/readings", (_req, res) => res.json(readings));

// HTTP server
const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`Dashboard en  http://10.7.135.36:${PORT}`);
  console.log(`(o http://localhost:${PORT} desde este PC)`);
});
