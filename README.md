# OTA Web con ElegantOTA (Rama `asyncElegantOTA`)

Esta rama implementa OTA vía navegador usando `ElegantOTA` sobre un servidor web asíncrono en el ESP32. Permite cargar binarios desde `http://<IP>/update` con barra de progreso y también actualizar el filesystem (LittleFS).

## ¿Qué es ElegantOTA?
ElegantOTA es una librería que agrega una UI web simple para subir firmware y, opcionalmente, el filesystem desde el navegador.

Beneficios:
- Interfaz web amigable y barra de progreso en tiempo real.
- No depende del soporte mDNS del host (navegas por IP directa).
- Funciona con `ESPAsyncWebServer` (asíncrono) o `WebServer` (sincrónico).

## Configuración del proyecto
Archivo `platformio.ini` (extracto relevante):

```ini
build_flags =
  -DELEGANTOTA_USE_ASYNC_WEBSERVER=1
lib_deps =
  ayushsharma82/ElegantOTA@^3.1.7
  me-no-dev/AsyncTCP@^1.1.1
  me-no-dev/ESP Async WebServer@^1.2.4
```

- La flag `ELEGANTOTA_USE_ASYNC_WEBSERVER` activa el modo asíncrono.
- Asegúrate de incluir explícitamente `AsyncTCP` y `ESP Async WebServer` en `lib_deps` si el resolver de dependencias no los instala automáticamente.

## Código base (extracto de `src/main.cpp`)

```cpp
#include <WiFi.h>
#include <ElegantOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("<SSID>", "<PASSWORD>");
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hola, accede a /update para OTA");
  });

  ElegantOTA.begin(&server, "admin", "12345678"); // user, pass
  server.begin();
}

void loop() {
  ElegantOTA.loop();
}
```

- Credenciales web: usuario y contraseña en `ElegantOTA.begin(&server, user, pass)`.
- Acceso en `http://<IP-ESP32>/update`.

## Uso paso a paso (firmware)
1. Flashea por USB la primera vez (`env:esp32s3-usb`).
2. Conecta el ESP32 a tu Wi‑Fi; mira la IP en el monitor serie.
3. Compila el firmware:

```bash
pio run -e esp32s3-usb
# artefacto: .pio/build/esp32s3-usb/firmware.bin
```

4. Abre en tu navegador `http://<IP>/update` y sube el `.bin`.

## Actualizar filesystem (LittleFS)
ElegantOTA soporta actualizar el filesystem. Para generar la imagen LittleFS:

```bash
pio run -e esp32s3-usb -t buildfs
# artefacto: .pio/build/esp32s3-usb/littlefs.bin
```

Luego, desde la UI de `ElegantOTA` selecciona la opción de filesystem y sube `littlefs.bin`.

## Alternativa 100% free al ecosistema ElegantOTA
ElegantOTA es de uso gratuito. Si prefieres evitar cualquier dependencia adicional, tienes opciones 100% libres basadas sólo en componentes del core/librerías estándar:
- `ArduinoOTA` (ver rama `main`): OTA por red sin interfaz web, del core de Arduino.
- `AsyncWebServer` + endpoint propio: crear un handler `POST /update` y usar `Update.h` directamente. Ejemplo mínimo:

```cpp
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>

AsyncWebServer server(80);

void setup() {
  // ... conectar Wi‑Fi ...
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    bool ok = !Update.hasError();
    request->send(ok ? 200 : 500, "text/plain", ok ? "OK" : "FAIL");
    if (ok) ESP.restart();
  }, [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    if (index == 0) {
      Update.begin(UPDATE_SIZE_UNKNOWN);
    }
    if (Update.write(data, len) != len) {
      // manejar error de escritura
    }
    if (final) {
      Update.end(true);
    }
  });
  server.begin();
}
```

Esto es 100% libre y usa sólo librerías open source estándar. Como UI puedes servir una página HTML simple con un `<input type="file">`.

## Seguridad
- Protege `/update` con credenciales y, si es posible, sirve sobre HTTPS detrás de un proxy inverso en tu LAN.
- Cambia credenciales por variables de entorno o secretos de build para producción.
- En producción, limita el acceso a la red de gestión (VLAN, VPN, etc.).

## Solución de problemas
- Si la página `/update` no carga, revisa que el server esté iniciado y que no haya conflictos de puertos.
- Si la compilación falla por cabeceras de `AsyncTCP` o `ESPAsyncWebServer`, añade esas libs a `lib_deps` como se indica arriba.
- Si la carga falla al final, confirma que `Update.end(true)` se llama y que hay espacio suficiente en la partición.
- Si tras actualizar no arranca, revisa `partitions.csv` y el tamaño de firmware/FS.
