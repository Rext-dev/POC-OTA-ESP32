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
String firmwareURL = "";
String currentVersion = "1.0.4"; // Versión actual del firmware

const char rootCACert[] PROGMEM =
  "-----BEGIN CERTIFICATE-----\n"
  "MIID0zCCArugAwIBAgIQVmcdBOpPmUxvEIFHWdJ1lDANBgkqhkiG9w0BAQwFADB7\n"
  "MQswCQYDVQQGEwJHQjEbMBkGA1UECAwSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYD\n"
  "VQQHDAdTYWxmb3JkMRowGAYDVQQKDBFDb21vZG8gQ0EgTGltaXRlZDEhMB8GA1UE\n"
  "AwwYQUFBIENlcnRpZmljYXRlIFNlcnZpY2VzMB4XDTE5MDMxMjAwMDAwMFoXDTI4\n"
  "MTIzMTIzNTk1OVowgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpOZXcgSmVyc2V5\n"
  "MRQwEgYDVQQHEwtKZXJzZXkgQ2l0eTEeMBwGA1UEChMVVGhlIFVTRVJUUlVTVCBO\n"
  "ZXR3b3JrMS4wLAYDVQQDEyVVU0VSVHJ1c3QgRUNDIENlcnRpZmljYXRpb24gQXV0\n"
  "aG9yaXR5MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEGqxUWqn5aCPnetUkb1PGWthL\n"
  "q8bVttHmc3Gu3ZzWDGH926CJA7gFFOxXzu5dP+Ihs8731Ip54KODfi2X0GHE8Znc\n"
  "JZFjq38wo7Rw4sehM5zzvy5cU7Ffs30yf4o043l5o4HyMIHvMB8GA1UdIwQYMBaA\n"
  "FKARCiM+lvEH7OKvKe+CpX/QMKS0MB0GA1UdDgQWBBQ64QmG1M8ZwpZ2dEl23OA1\n"
  "xmNjmjAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zARBgNVHSAECjAI\n"
  "MAYGBFUdIAAwQwYDVR0fBDwwOjA4oDagNIYyaHR0cDovL2NybC5jb21vZG9jYS5j\n"
  "b20vQUFBQ2VydGlmaWNhdGVTZXJ2aWNlcy5jcmwwNAYIKwYBBQUHAQEEKDAmMCQG\n"
  "CCsGAQUFBzABhhhodHRwOi8vb2NzcC5jb21vZG9jYS5jb20wDQYJKoZIhvcNAQEM\n"
  "BQADggEBABns652JLCALBIAdGN5CmXKZFjK9Dpx1WywV4ilAbe7/ctvbq5AfjJXy\n"
  "ij0IckKJUAfiORVsAYfZFhr1wHUrxeZWEQff2Ji8fJ8ZOd+LygBkc7xGEJuTI42+\n"
  "FsMuCIKchjN0djsoTI0DQoWz4rIjQtUfenVqGtF8qmchxDM6OW1TyaLtYiKou+JV\n"
  "bJlsQ2uRl9EMC5MCHdK8aXdJ5htN978UeAOwproLtOGFfy/cQjutdAFI3tZs4RmY\n"
  "CV4Ks2dH/hzg1cEo70qLRDEmBDeNiXQ2Lu+lIg+DdEmSx/cQwgwp+7e9un/jX9Wf\n"
  "8qn0dNW44bOwgeThpWOjzOoEeJBuv/c=\n"
  "-----END CERTIFICATE-----\n";

// chequeo 1 minuto -> para testar rapido
const unsigned long checkInterval = 1 * 60 * 1000;
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
  client.setCACert(rootCACert);
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
  client.setCACert(rootCACert);
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
        firmwareURL = doc["assets"][0]["browser_download_url"].as<String>();  // URL del .bin
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
