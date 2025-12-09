// src/mqtt/mqttClient.js
import mqtt from "mqtt";

const MQTT_URL = "ws://192.168.100.38:9001";

const client = mqtt.connect(MQTT_URL, {
  reconnectPeriod: 1000,
  connectTimeout: 4000,
});

client.on("connect", () => {
  console.log("🔌 Conectado a MQTT");
});

client.on("error", (err) => {
  console.error("❌ Error MQTT:", err);
});

export default client;
