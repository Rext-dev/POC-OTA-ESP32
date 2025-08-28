#include <WiFi.h>              // WiFi STA
#include <WiFiClientSecure.h>  // Cliente WiFi seguro
#include <HTTPClient.h>        // Cliente HTTP
#include <Update.h>            // Actualizaciones OTA
#include <ArduinoJson.h>       // JSON
#include <Adafruit_NeoPixel.h> // NeoPixel (instalar librería)

#ifndef LED_PIN
#define LED_PIN 48 // Cambia a 38 si tu DevKitC-1 v1.1 marca RGB@IO38
#endif
#define NUM_PIXELS 1

// Ajusta credenciales
const char *ssid = "Totalplay-F4A3";
const char *password = "F4A3D4DB82qNSUMy";

Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// GITHUB
const char *githubHost = "api.github.com";
const char *repoPath = "/repos/Rext-Dev/POC-OTA-ESP32/releases/latest"; // Cambia a tu repo
const char *firmwareURL = "";
String currentVersion = "1.0.0"; // Versión actual del firmware

const char *githubCert =
  "-----BEGIN PUBLIC KEY-----\n"
  "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEZ7HRH5V+m4grbVZ9nyYbEiwrjPOD\n"
  "DQ+6roX6ktxGp7fz/jsxJ4MAYMnFsPXpx6+uENqtB4wsyhRVryWI63ecpA==\n"
  "-----END PUBLIC KEY-----\n";

// chequeo 10 minutos
const unsigned long checkInterval = 10 * 60 * 1000;
unsigned long lastCheck = 0;

void connectWiFi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado, IP: " + WiFi.localIP().toString());
}

void performOTAUpdate(String binURL) {
  WiFiClientSecure client;
  client.setCACert(githubCert);
  HTTPClient http;
  if (http.begin(client, binURL)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      int contentLength = http.getSize();
      if (Update.begin(contentLength)) {
        size_t written = Update.writeStream(http.getStream());
        if (written == contentLength) {
          if (Update.end(true)) {
            Serial.println("Update exitoso! Rebooting...");
            ESP.restart();
          } else Serial.println("Update falló");
        }
      }
    }
    http.end();
  }
}

void checkForUpdates() {
  WiFiClientSecure client;
  client.setCACert(githubCert);
  HTTPClient http;
  String url = "https://" + String(githubHost) + repoPath;
  if (http.begin(client, url)) {
    http.addHeader("User-Agent", "ESP32-OTA");  // Para API GitHub
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument doc;  // Tamaño para JSON
      deserializeJson(doc, payload);
      String latestVersion = doc["tag_name"];  // Ej. "v1.0.1"
      if (latestVersion > currentVersion) {  // Compara versiones
        firmwareURL = doc["assets"][0]["browser_download_url"];  // URL del .bin
        Serial.println("Nueva versión: " + latestVersion + ". Descargando...");
        performOTAUpdate(firmwareURL);
      } else Serial.println("Firmware actualizado");
    }
    http.end();
  }
}

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  connectWiFi();

  strip.begin();
  strip.clear();
  strip.show();
}

void loop()
{
  static uint32_t t0 = 0;
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (millis() - lastCheck > checkInterval) {
    lastCheck = millis();
    checkForUpdates();  // Chequea updates
  }

  if (millis() - t0 > 1000)
  {
    t0 = millis();
    static bool on = false;
    on = !on;
    // Cambio: rojo en lugar de azul (0,0,255 -> 255,0,0)
    uint32_t color = on ? strip.Color(0, 255, 0) : strip.Color(0, 0, 0);
    strip.setPixelColor(0, color);
    strip.show();
  }
}
