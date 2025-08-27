#include <WiFi.h>              // WiFi STA
#include <ArduinoOTA.h>        // OTA por LAN
#include <Adafruit_NeoPixel.h> // NeoPixel (instalar librería)

#ifndef LED_PIN
  #define LED_PIN 48           // Cambia a 38 si tu DevKitC-1 v1.1 marca RGB@IO38
#endif
#define NUM_PIXELS 1

// Ajusta credenciales
const char *ssid = "Totalplay-F4A3";
const char *password = "F4A3D4DB82qNSUMy";

Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(250); }
  Serial.println("");
  Serial.print("Conectado a ");
  Serial.println(ssid);
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // OTA básico: hostname y password
  ArduinoOTA.setHostname("esp32s3-ota");
  ArduinoOTA.setPassword("12345678");
  ArduinoOTA.begin(); // inicia servicio OTA

  strip.begin();
  strip.clear();
  strip.show();
}

void loop() {
  static uint32_t t0 = 0;
  ArduinoOTA.handle(); // atiende peticiones OTA

  if (millis() - t0 > 1000) {
    t0 = millis();
    static bool on = false;
    on = !on;
    // Cambio: rojo en lugar de azul (0,0,255 -> 255,0,0)
    uint32_t color = on ? strip.Color(255, 0, 0) : strip.Color(0, 0, 0);
    strip.setPixelColor(0, color);
    strip.show();
    }
  }
