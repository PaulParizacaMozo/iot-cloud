
#include <Arduino_MKRIoTCarrier.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>

// ---------------- CONFIG WIFI ----------------
const char *WIFI_SSID = "wifi-CsComputacion";
const char *WIFI_PASS = "EPCC2022$";

// ---------------- CONFIG MQTT ----------------
const char *MQTT_BROKER = "3.133.89.120"; // misma IP que en ESP32
const int MQTT_PORT = 1883;
const char *MQTT_TOPIC = "mkr/humedad"; // tópico para MKR

// ---------------- CONFIG NTP -----------------
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600; // UTC-5

// ---------------- HARDWARE -------------------
MKRIoTCarrier carrier;
const int pinSensor = A6; // pin sensor

// ---------------- NETWORK / MQTT --------------------
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// ======= CONFIG NTP =======
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, 60000);

unsigned long lastPublish = 0;
const unsigned long INTERVAL_MS = 2000; // 2 seconds

// ---------------------------
void connectWiFi()
{
  Serial.print("Conectando a WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000)
    { // 20s timeout
      Serial.println("\nTimeout WiFi, reintentando...");
      start = millis();
    }
  }
  Serial.print("\nWiFi conectado, IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT()
{
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  Serial.print("Conectando a MQTT");
  while (!mqtt.connected())
  {
    String clientId = "MKR-" + String(millis());
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

void initTime()
{
  timeClient.begin();
  Serial.print("Sincronizando NTP");
  int retries = 0;
  while (timeClient.getEpochTime() < 1600000000UL && retries < 20)
  {
    timeClient.update();
    delay(500);
    Serial.print(".");
    retries++;
  }
  if (timeClient.getEpochTime() >= 1600000000UL)
  {
    Serial.println("\nHora sincronizada (NTP).");
  }
  else
  {
    Serial.println("\nNo se pudo sincronizar hora (NTP).");
  }
}

// ---------------- SETUP ----------------------
void setup()
{
  Serial.begin(115200);

  CARRIER_CASE = true;
  carrier.begin();

  connectWiFi();
  initTime();
  connectMQTT();

  // Pantalla inicial
  carrier.display.fillScreen(ST77XX_BLACK);
  carrier.display.setTextSize(2);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setCursor(20, 60);
  carrier.display.print("Iniciando...");
  delay(1500);
}

// ---------------- LOOP -----------------------
void loop()
{
  if (WiFi.status() != WL_CONNECTED)
    connectWiFi();
  if (!mqtt.connected())
    connectMQTT();
  mqtt.loop();
  timeClient.update(); // NTP Correct Time

  unsigned long nowMs = millis();
  if (nowMs - lastPublish >= INTERVAL_MS)
  {
    lastPublish = nowMs;

    int valor = analogRead(pinSensor);
    int humedad = map(valor, 1023, 200, 0, 100);
    if (humedad < 0)
      humedad = 0;
    if (humedad > 100)
      humedad = 100;

    float humedad_gm3 = humedad * 30.0 / 100.0;

    String estado;
    if (humedad < 30)
      estado = "Seco";
    else if (humedad < 70)
      estado = "Humedo";
    else
      estado = "Muy humedo";

    unsigned long ts_sec = timeClient.getEpochTime();

    // JSON payload (ts en segundos)
    char payload[160];
    snprintf(payload, sizeof(payload),
             "{\"temp\":%d,\"hum\":%.2f,\"ts\":%lu}",
             humedad, humedad_gm3, ts_sec);

    // Publicar en MQTT
    if (mqtt.publish(MQTT_TOPIC, payload))
    {
      Serial.print("Publicado: ");
      Serial.println(payload);
    }
    else
    {
      Serial.println("Error publicando en MQTT");
    }

    // Mostrar en pantalla
    carrier.display.fillScreen(ST77XX_BLACK);
    carrier.display.setTextSize(2);
    carrier.display.setTextColor(ST77XX_WHITE);
    carrier.display.setCursor(20, 20);
    carrier.display.print("Humedad suelo:");
    carrier.display.setCursor(40, 60);
    carrier.display.print(humedad);
    carrier.display.print(" %");
    carrier.display.setCursor(40, 90);
    carrier.display.print(humedad_gm3, 1);
    carrier.display.print(" g/m3");
    carrier.display.setCursor(40, 120);
    carrier.display.print(estado);
  }
}
