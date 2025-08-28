# OTA Web con ElegantOTA (Rama asyncElegantOTA)# OTA en ESP32 con ArduinoOTA (Rama main)



Esta rama implementa OTA vía navegador usando `ElegantOTA` sobre un servidor web asíncrono en el ESP32. Permite cargar binarios desde `http://<ip>/update` con barra de progreso.Esta rama implementa OTA (Over-The-Air) usando `ArduinoOTA` sobre la red local (LAN). Permite subir firmware sin cable una vez que el dispositivo está conectado al Wi‑Fi.



## ¿Qué es ElegantOTA?## ¿Qué es ArduinoOTA?

ElegantOTA es una librería que agrega una UI web simple para subir firmware y (opcionalmente) el filesystem (LittleFS) desde el navegador. Ventajas:`ArduinoOTA` es una librería oficial del core de Arduino para ESP32/ESP8266 que habilita la actualización de firmware a través de la red local. Funciona con PlatformIO y Arduino IDE. Su principal ventaja es la simplicidad: no requiere servidores externos ni hosting.

- Interfaz web amigable y barra de progreso.

- No depende del soporte mDNS del host.## Requisitos

- Funciona con `ESPAsyncWebServer` (asíncrono) o `WebServer` (sincrónico).- Board: `ESP32-S3-DevKitC-1` (ajustable en `platformio.ini`).

- Wi‑Fi 2.4 GHz accesible para la placa.

## Configuración del proyecto- PlatformIO (recomendado) o Arduino IDE.

Archivo `platformio.ini` (extracto relevante):

## Configuración del proyecto

```iniArchivo `platformio.ini`:

build_flags =

  -DELEGANTOTA_USE_ASYNC_WEBSERVER=1```ini

lib_deps =[env:esp32s3-usb]

  ayushsharma82/ElegantOTA@^3.1.7platform = espressif32

```board = esp32-s3-devkitc-1

framework = arduino

Esto activa el modo servidor asíncrono con `AsyncTCP` + `ESPAsyncWebServer`.monitor_speed = 115200

monitor_filters = esp32_exception_decoder

## Código base (extracto de `src/main.cpp`)board_build.arduino.memory_type = qio_opi

board_build.flash_mode = qio

```cppboard_build.psram_type = opi

#include <WiFi.h>board_build.partitions = partitions.csv

#include <ElegantOTA.h>board_build.filesystem = littlefs

#include <AsyncTCP.h>board_upload.flash_size = 16MB

#include <ESPAsyncWebServer.h>board_upload.maximum_size = 16777216

board_upload.offset_address = 0x10000

AsyncWebServer server(80);build_flags =

  -DBOARD_HAS_PSRAM

void setup() {  -DOTA_HOSTNAME=\"esp32s3-ota\"

  WiFi.mode(WIFI_STA);lib_deps = adafruit/Adafruit NeoPixel@^1.15.1

  WiFi.begin("<SSID>", "<PASSWORD>");

  while (WiFi.status() != WL_CONNECTED) { delay(500); }[env:esp32s3-ota]

extends = env:esp32s3-usb

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){upload_protocol = espota

    request->send(200, "text/plain", "Hola, accede a /update para OTA");upload_port = esp32s3-ota.local

  });upload_flags = 

    --auth=12345678

  ElegantOTA.begin(&server, "admin", "12345678");lib_deps = adafruit/Adafruit NeoPixel@^1.15.1

  server.begin();```

}

- `upload_protocol = espota`: habilita subidas por OTA.

void loop() {- `upload_port`: usa el hostname o la IP del dispositivo (mDNS `.local` o IP directa).

  ElegantOTA.loop();- `upload_flags --auth`: contraseña OTA.

}

```## Código mínimo (extracto de `src/main.cpp`)



- Credenciales web: usuario y contraseña en `ElegantOTA.begin(&server, user, pass)`.```cpp

- Acceso en `http://<IP-ESP32>/update`.#include <WiFi.h>

#include <ArduinoOTA.h>

## Uso paso a paso

1. Flashea por USB la primera vez (`env:esp32s3-usb`).void setup() {

2. Conecta el ESP32 a tu Wi‑Fi; mira la IP en el monitor serie.  WiFi.mode(WIFI_STA);

3. Abre en tu navegador `http://<IP>/update` y sube el `.bin` generado por PlatformIO.  WiFi.begin("<SSID>", "<PASSWORD>");

  while (WiFi.status() != WL_CONNECTED) { delay(250); }

Para compilar y obtener el binario:

  ArduinoOTA.setHostname("esp32s3-ota");

```bash  ArduinoOTA.setPassword("12345678");

pio run -e esp32s3-usb  ArduinoOTA.begin();

# archivo: .pio/build/esp32s3-usb/firmware.bin}

```

void loop() {

## Alternativa 100% free al ecosistema ElegantOTA  ArduinoOTA.handle();

ElegantOTA es gratuito de usar, pero si prefieres evitar dependencias adicionales o licencias, puedes usar:}

- `ArduinoOTA` (rama `main`): OTA por red sin interfaz web, totalmente open source del core Arduino.```

- `AsyncWebServer` + endpoint propio: crear un handler `POST /update` y usar `Update.h` directamente. Ejemplo mínimo:

Asegúrate de llamar a `ArduinoOTA.handle()` en el `loop()`; sin ello no se atienden las conexiones OTA.

```cpp

#include <WiFi.h>## Cómo usarlo (PlatformIO)

#include <AsyncTCP.h>1. Carga por cable una primera vez usando el entorno `env:esp32s3-usb`.

#include <ESPAsyncWebServer.h>2. El ESP32 se conecta al Wi‑Fi y anuncia mDNS con el hostname configurado.

#include <Update.h>3. Cambia a `env:esp32s3-ota` y sube el firmware por red:



AsyncWebServer server(80);```bash

pio run -e esp32s3-ota -t upload

void setup() {```

  // ... conectar Wi‑Fi ...

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){Opcionalmente especifica la IP si mDNS no funciona:

    bool ok = !Update.hasError();

    request->send(ok ? 200 : 500, "text/plain", ok ? "OK" : "FAIL");```bash

    ESP.restart();pio run -e esp32s3-ota -t upload --upload-port 192.168.1.123

  }, [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){```

    if (index == 0) {

      Update.begin(UPDATE_SIZE_UNKNOWN);## Cómo usarlo (Arduino IDE)

    }- Selecciona la placa y el puerto OTA que aparecerá como `esp32s3-ota at <IP>` en Puertos.

    if (Update.write(data, len) != len) {- Usa "Subir mediante red"; si pide contraseña, introduce la configurada (`12345678`).

      // error

    }## Buenas prácticas y consejos

    if (final) {- Reserva un pequeño LED/NeoPixel para señalar estado de OTA (opcional).

      Update.end(true);- Mantén la contraseña OTA y Wi‑Fi en un archivo no versionado para producción.

    }- En redes con mDNS problemático, usa IP fija o DHCP-reservation y `--upload-port <IP>`.

  });- Si recibes `No response from device`, verifica: Wi‑Fi conectado, `ArduinoOTA.begin()` llamado, y que el firewall permite UDP/5353 y TCP.

  server.begin();

}## Ejemplos útiles

```- Cambiar sólo la contraseña OTA:



Esto es 100% libre y usa sólo librerías open source estándar. Como UI puedes servir una página HTML simple con un `<input type="file">`.```bash

pio run -e esp32s3-ota -t upload --upload-port esp32s3-ota.local --upload-flag "--auth=MiPass"

## Seguridad```

- Protege `/update` con credenciales y, si es posible, sirve sobre HTTPS a través de un proxy inverso en tu LAN.

- Cambia credenciales por variables de entorno o secretos en build para producción.- Ver logs de excepción decodificados: abre el monitor serie con filtro `esp32_exception_decoder` ya preconfigurado en `platformio.ini`.



## Solución de problemas## Solución de problemas

- Si la página `/update` no carga, revisa que el server esté iniciado y que no haya conflictos de puertos.- mDNS no resuelve `.local`: instala/activa soporte mDNS en tu sistema o usa la IP.

- Si la carga falla al final, confirma que `Update.end(true)` se llama y que hay espacio suficiente en la partición.- Subida falla al 0%: revisa que el `upload_port` apunta a tu dispositivo correcto y credenciales.

- Bucles de reinicio post-OTA: puede deberse a tamaño de firmware o particiones; revisa `partitions.csv` y tamaños.

## Seguridad
`ArduinoOTA` no cifra tráfico por sí mismo. Usa redes de confianza. Para producción, combina con segmentación de red o túneles seguros; para necesidades avanzadas considera las ramas con HTTPS/GitHub Actions.
