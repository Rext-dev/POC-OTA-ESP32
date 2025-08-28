// Se puede combinar con ARDUINO OTA de la rama main
#include <WiFi.h>       // WiFi STA
#include <ElegantOTA.h> // OTA por LAN
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_NeoPixel.h> // NeoPixel

#ifndef LED_PIN
#define LED_PIN 48 // Cambia a 38 si tu DevKitC-1 v1.1 marca RGB@IO38
#endif
#define NUM_PIXELS 1


// Ajusta credenciales
const char *ssid = "Totalplay-F4A3";
const char *password = "F4A3D4DB82qNSUMy";

AsyncWebServer server(80);

Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void connectWiFi()
{ // De plantilla
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a " + String(ssid) + " IP: " + WiFi.localIP().toString());
}

void setup()
{
  Serial.begin(115200);
  connectWiFi();

  // Ruta raíz simple (opcional: para testear server)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              request->send(200, "text/plain", "Hola, accede a /update para OTA"); // Mensaje básico
            });

  ElegantOTA.begin(&server, "admin", "12345678"); // Credenciales OTA
  server.begin();

  strip.begin();
  strip.clear();
  strip.show();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  static uint32_t t0 = 0;
  ElegantOTA.loop();

  if (millis() - t0 > 1000)
  {
    t0 = millis();
    static bool on = false;
    on = !on;
    // Cambio: rojo en lugar de azul (0,0,255 -> 255,0,0)
    uint32_t color = on ? strip.Color(0, 0, 255) : strip.Color(0, 0, 0);
    strip.setPixelColor(0, color);
    strip.show();
  }
}
