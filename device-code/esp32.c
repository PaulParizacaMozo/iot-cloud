#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <time.h>

// ======= CONFIG WIFI =======
const char *WIFI_SSID = "wifi-CsComputacion";
const char *WIFI_PASS = "EPCC2022$";

// ======= CONFIG MQTT =======
const char *MQTT_BROKER = "10.7.135.36"; // IP del broker MQTT
const int MQTT_PORT = 1883;
const char *MQTT_TOPIC = "esp32/temperatura";

// ======= CONFIG DHT =======
#define DHTPIN 26     // GPIO donde está conectado
#define DHTTYPE DHT11 // Sensor
DHT dht(DHTPIN, DHTTYPE);

// ======= CONFIG NTP (Time) =======
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600; // UTC-5
const int daylightOffset_sec = 0;

WiFiClient espClient;
PubSubClient mqtt(espClient);

unsigned long lastPublish = 0;
const unsigned long INTERVAL_MS = 1000; // 1 second

// ---------------------------

void connectWiFi()
{
  Serial.print("Conectando a WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi conectado, IP: ");
  Serial.println(WiFi.localIP());
}

void initTime()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("Sincronizando NTP");
  time_t now = 0;
  int retries = 0;
  while (now < 1700000000 && retries < 20)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    retries++;
  }
  Serial.println("\nHora sincronizada");
}

void connectMQTT()
{
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  Serial.print("Conectando a MQTT");
  while (!mqtt.connected())
  {
    String clientId = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (mqtt.connect(clientId.c_str()))
    {
      Serial.println("\nMQTT conectado");
    }
    else
    {
      Serial.print(".");
      delay(1000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  dht.begin();

  connectWiFi();
  initTime();
  connectMQTT();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
    connectWiFi();
  if (!mqtt.connected())
    connectMQTT();
  mqtt.loop();

  unsigned long nowMs = millis();
  if (nowMs - lastPublish >= INTERVAL_MS)
  {
    lastPublish = nowMs;

    float h = dht.readHumidity();
    float t = dht.readTemperature(); // °C

    if (isnan(h) || isnan(t))
    {
      Serial.println("Error al leer el DHT");
      return;
    }

    unsigned long ts = (unsigned long)time(nullptr) * 1000UL;
    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"temp\":%.2f,\"hum\":%.2f,\"ts\":%lu}",
             t, h, ts);

    if (mqtt.publish(MQTT_TOPIC, payload))
    {
      Serial.print("Publicado: ");
      Serial.println(payload);
    }
    else
    {
      Serial.println("Error publicando en MQTT");
    }
  }
}