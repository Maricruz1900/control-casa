#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP32Servo.h>

//------------------------------------------
// CONFIG WIFI
//------------------------------------------
#define WIFI_SSID "Totalplay-C2A5" //"Totalplay-C2A5" //"UTMA-RECTORIA"
#define WIFI_PASSWORD "C2A598DByA3CwDeX" //"C2A598DByA3CwDeX"//"RECT2022@utma"

//------------------------------------------
// CONFIG MQTT
//------------------------------------------
const char* mqtt_server = "192.168.100.38";
WiFiClient espClient;
PubSubClient client(espClient);

//------------------------------------------
// TOPICOS
//------------------------------------------
#define TOPIC_LED_BASE "domotica/leds/"
#define TOPIC_SENSOR_BASE "domotica/sensores/"
#define TOPIC_DHT_TEMP "domotica/dht/temperatura"
#define TOPIC_DHT_HUM "domotica/dht/humedad"
#define TOPIC_SERVO "domotica/servo/pos"

//------------------------------------------
// HARDWARE
//------------------------------------------
const uint8_t ledPins[5] = {23, 22, 21, 19, 18};
const uint8_t buttonPins[5] = {32, 33, 25, 26, 27};
const uint8_t sensorPins[4] = {34, 35, 36, 39};
const uint8_t servoPin = 15;

bool ledState[5] = {0};
bool lastSensorState[4] = {0};

unsigned long lastDebounce[5] = {0};
bool lastReading[5] = {LOW};
bool stableState[5] = {LOW};
const unsigned long debounceDelay = 50;

//------------------------------------------
// DHT11
//------------------------------------------
#define DHTPIN 16
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//------------------------------------------
// SERVO
//------------------------------------------
Servo myServo;
int currentServoAngle = 0;

//------------------------------------------
// MANEJO DE MENSAJES MQTT
//------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;

  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  // Control de LEDs desde dashboard
 if (String(topic).startsWith(TOPIC_LED_BASE)) {
    // topic = domotica/leds/l1 → queremos extraer "1"
    String sub = String(topic).substring(strlen(TOPIC_LED_BASE)); // "l1"
    sub.remove(0, 1); // quitamos "l" → queda "1"

    int idx = sub.toInt() - 1;  
    int v = msg.toInt();

    ledState[idx] = v;
    digitalWrite(ledPins[idx], v ? HIGH : LOW);

    Serial.print("LED ");
    Serial.print(idx + 1);
    Serial.print(" → ");
    Serial.println(v ? "ENCENDIDO" : "APAGADO");
}

  // Control del servo desde dashboard
  if (String(topic) == TOPIC_SERVO) {
    int angle = msg.toInt();
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    myServo.write(angle);
    currentServoAngle = angle;

    // * PUBLICA SIEMPRE SU ESTADO *
    client.publish(TOPIC_SERVO, String(angle).c_str());

    Serial.print("Servo movido por dashboard → ");
    Serial.println(angle);
  }
}

//------------------------------------------
// MQTT RECONNECT
//------------------------------------------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando a MQTT... ");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado!");

      // Suscribirse a LEDs 1-5
      for (int i = 1; i <= 5; i++) {
        client.subscribe((TOPIC_LED_BASE + String("l") + i).c_str());
      }

      // Suscribirse al servo
      client.subscribe(TOPIC_SERVO);

      // PUBLICA EL ESTADO DEL SERVO PARA QUE APAREZCA EL TOPICO
      client.publish(TOPIC_SERVO, String(currentServoAngle).c_str());

    } else {
      Serial.print("Fallo: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

//------------------------------------------
// SETUP
//------------------------------------------
void setup() {
  Serial.begin(115200);

  // LEDs
  for (int i = 0; i < 5; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  // Botones
  for (int i = 0; i < 5; i++) {
    pinMode(buttonPins[i], INPUT);
  }

  // Sensores
  for (int i = 0; i < 4; i++) {
    pinMode(sensorPins[i], INPUT_PULLUP);
    lastSensorState[i] = digitalRead(sensorPins[i]);
  }

  // DHT
  dht.begin();

  // SERVO
  myServo.attach(servoPin);
  myServo.write(0);
  delay(500);
  myServo.write(90);
  delay(500);
  myServo.write(180);
  delay(500);
  myServo.write(0);
  delay(500);

  currentServoAngle = 0; // Ángulo inicial

  Serial.println("Servo OK.");

  // WIFI
  Serial.print("Intentando conectar a: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi conectado.");
  Serial.print("IP asignada a la ESP32: ");
  Serial.println(WiFi.localIP());

  // MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
}

//------------------------------------------
// LOOP PRINCIPAL
//------------------------------------------
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

  //-----------------------
  // BOTONES -> LEDS
  //-----------------------
  for (int i = 0; i < 5; i++) {
    bool reading = digitalRead(buttonPins[i]);

    if (reading != lastReading[i]) {
      lastDebounce[i] = now;
    }

    if ((now - lastDebounce[i]) > debounceDelay) {
      if (reading != stableState[i]) {
        stableState[i] = reading;

        if (stableState[i] == HIGH) {

          // Toggle local
          ledState[i] = !ledState[i];
          digitalWrite(ledPins[i], ledState[i] ? HIGH : LOW);

          // Publicar estado
          client.publish(
            (TOPIC_LED_BASE + String("l") + (i + 1)).c_str(),
            ledState[i] ? "1" : "0"
          );

          Serial.print("BOTÓN ");
          Serial.print(i + 1);
          Serial.print(" → LED ");
          Serial.print(i + 1);
          Serial.print(" = ");
          Serial.println(ledState[i] ? "ENCENDIDO" : "APAGADO");
        }
      }
    }

    lastReading[i] = reading;
  }

  //-----------------------
  // SENSORES MAGNÉTICOS
  //-----------------------
  for (int i = 0; i < 4; i++) {
    int lectura = digitalRead(sensorPins[i]);
    int estado = (lectura == LOW) ? 0 : 1;

    const char* topics[4] = {
      "domotica/sensores/p1",
      "domotica/sensores/p2",
      "domotica/sensores/v1",
      "domotica/sensores/v2"
    };

    client.publish(topics[i], estado ? "1" : "0");

    if (estado != lastSensorState[i]) {
      lastSensorState[i] = estado;

      Serial.print("Sensor ");
      Serial.print(i + 1);
      Serial.print(" → ");
      Serial.println(estado ? "Cerrado" : "Abierto");
    }
  }

  //-----------------------
  // DHT11 cada 10s
  //-----------------------
  static unsigned long lastDHT = 0;

  if (millis() - lastDHT > 10000) {
    lastDHT = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      Serial.print("Temp: ");
      Serial.print(t);
      Serial.print(" °C   |   Hum: ");
      Serial.print(h);
      Serial.println(" %");

      client.publish(TOPIC_DHT_TEMP, String(t).c_str());
      client.publish(TOPIC_DHT_HUM, String(h).c_str());
    } else {
      Serial.println("⚠ Error leyendo DHT11");
    }
  }
}