import { useEffect, useState } from "react";
import client from "../mqtt/mqttClient";

export default function LedControl({ id }) {
  const topic = `domotica/leds/l${id}`;
  const [state, setState] = useState(false);

  useEffect(() => {
    client.subscribe(topic);

    const handler = (t, msg) => {
      if (t === topic) {
        setState(msg.toString() === "1");
      }
    };

    client.on("message", handler);

    return () => {
      client.unsubscribe(topic);
      client.off("message", handler);
    };
  }, [topic]);

  // ⭐ FUNCIÓN PARA ELEGIR LA IMAGEN SEGÚN EL FOCO
  const getImage = () => {
    switch (id) {
      case 1:
        return "/img/bano.jpg";
      case 2:
        return "/img/cocina.jpg";
      case 3:
        return "/img/sala.jpg";
      case 4:
        return "/img/cochera.jpg";
      case 5:
        return "/img/pasillo.jpg";
      default:
        return "/img/cochera.jpg";
    }
  };

  const toggleLed = () => {
    const newState = !state;

    // 🔥 Actualiza el estado inmediatamente
    setState(newState);

    // 🔥 Enviar por MQTT
    client.publish(topic, newState ? "1" : "0");
  };

  return (
    <div className="device-card">

      {/* ⭐ IMAGEN DEL FOCO */}
      <img src={getImage()} className="device-icon" alt={`Foco ${id}`} />

      <div className="device-caption">Foco {id}</div>

      <button
        className={`device-toggle ${state ? "active" : ""}`}
        onClick={toggleLed}
      >
        {state ? "Apagar" : "Encender"}
      </button>
    </div>
  );
}
