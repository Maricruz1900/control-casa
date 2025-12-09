import { useEffect, useState } from "react";
import client from "../mqtt/mqttClient";

export default function SensorStatus({ name, topic }) {
  const [status, setStatus] = useState("0"); // 0 = abierto, 1 = cerrado

  // 🔥 Elegir imagen según el nombre
  const getImage = () => {
    switch (name.toLowerCase()) {
      case "puerta 1":
        return "/img/puerta1.jpg";
      case "puerta 2":
        return "/img/puerta2.jpg";
      case "ventana 1":
        return "/img/ventana1.jpg";
      case "ventana 2":
        return "/img/ventana2.jpg";
      default:
        return "/img/default.jpg";
    }
  };

  useEffect(() => {
    client.subscribe(topic);

    const handler = (t, msg) => {
      if (t === topic) {
        setStatus(msg.toString());
      }
    };

    client.on("message", handler);

    return () => {
      client.unsubscribe(topic);
      client.off("message", handler);
    };
  }, [topic]);

  return (
    <div className="device-card">
      <div className="device-caption">{name}</div>

      {/* IMAGEN */}
      <img src={getImage()} alt={name} className="sensor-image" />

      <div className="device-subtitle">
        {status === "1" ? "Cerrado" : "Abierto"}
      </div>
    </div>
  );
}
