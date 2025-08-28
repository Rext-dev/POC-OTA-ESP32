# OTA en ESP32 con ArduinoOTA (Rama main)

Esta rama implementa OTA (Over-The-Air) usando `ArduinoOTA` sobre la red local (LAN). Permite subir firmware sin cable una vez que el dispositivo está conectado al Wi‑Fi.

## ¿Qué es ArduinoOTA?
`ArduinoOTA` es una librería oficial del core de Arduino para ESP32/ESP8266 que habilita la actualización de firmware a través de la red local. Funciona con PlatformIO y Arduino IDE. Su principal ventaja es la simplicidad: no requiere servidores externos ni hosting.

## Requisitos
- Board: `ESP32-S3-DevKitC-1` (ajustable en `platformio.ini`).
- Wi‑Fi 2.4 GHz accesible para la placa.
- PlatformIO (recomendado) o Arduino IDE.

## Configuración del proyecto
Archivo `platformio.ini`:

```ini
[env:esp32s3-usb]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
monitor_filters = esp32_exception_decoder
board_build.arduino.memory_type = qio_opi
board_build.flash_mode = qio
board_build.psram_type = opi
board_build.partitions = partitions.csv
board_build.filesystem = littlefs
board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216
board_upload.offset_address = 0x10000
build_flags =
  -DBOARD_HAS_PSRAM
  -DOTA_HOSTNAME=\"esp32s3-ota\"
lib_deps = adafruit/Adafruit NeoPixel@^1.15.1

[env:esp32s3-ota]
extends = env:esp32s3-usb
upload_protocol = espota
upload_port = esp32s3-ota.local
upload_flags = 
    --auth=12345678
lib_deps = adafruit/Adafruit NeoPixel@^1.15.1
```

- `upload_protocol = espota`: habilita subidas por OTA.
- `upload_port`: usa el hostname o la IP del dispositivo (mDNS `.local` o IP directa).
- `upload_flags --auth`: contraseña OTA.

## Código mínimo (extracto de `src/main.cpp`)

```cpp
#include <WiFi.h>
#include <ArduinoOTA.h>

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("<SSID>", "<PASSWORD>");
  while (WiFi.status() != WL_CONNECTED) { delay(250); }

  ArduinoOTA.setHostname("esp32s3-ota");
  ArduinoOTA.setPassword("12345678");
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
}
```

Asegúrate de llamar a `ArduinoOTA.handle()` en el `loop()`; sin ello no se atienden las conexiones OTA.

## Cómo usarlo (PlatformIO)
1. Carga por cable una primera vez usando el entorno `env:esp32s3-usb`.
2. El ESP32 se conecta al Wi‑Fi y anuncia mDNS con el hostname configurado.
3. Cambia a `env:esp32s3-ota` y sube el firmware por red:

```bash
pio run -e esp32s3-ota -t upload
```

Opcionalmente especifica la IP si mDNS no funciona:

```bash
pio run -e esp32s3-ota -t upload --upload-port 192.168.1.123
```

## Cómo usarlo (Arduino IDE)
- Selecciona la placa y el puerto OTA que aparecerá como `esp32s3-ota at <IP>` en Puertos.
- Usa "Subir mediante red"; si pide contraseña, introduce la configurada (`12345678`).

## Buenas prácticas y consejos
- Reserva un pequeño LED/NeoPixel para señalar estado de OTA (opcional).
- Mantén la contraseña OTA y Wi‑Fi en un archivo no versionado para producción.
- En redes con mDNS problemático, usa IP fija o DHCP-reservation y `--upload-port <IP>`.
- Si recibes `No response from device`, verifica: Wi‑Fi conectado, `ArduinoOTA.begin()` llamado, y que el firewall permite UDP/5353 y TCP.

## Ejemplos útiles
- Cambiar sólo la contraseña OTA:

```bash
pio run -e esp32s3-ota -t upload --upload-port esp32s3-ota.local --upload-flag "--auth=MiPass"
```

- Ver logs de excepción decodificados: abre el monitor serie con filtro `esp32_exception_decoder` ya preconfigurado en `platformio.ini`.

## Solución de problemas
- mDNS no resuelve `.local`: instala/activa soporte mDNS en tu sistema o usa la IP.
- Subida falla al 0%: revisa que el `upload_port` apunta a tu dispositivo correcto y credenciales.
- Bucles de reinicio post-OTA: puede deberse a tamaño de firmware o particiones; revisa `partitions.csv` y tamaños.

## Seguridad
`ArduinoOTA` no cifra tráfico por sí mismo. Usa redes de confianza. Para producción, combina con segmentación de red o túneles seguros; para necesidades avanzadas considera las ramas con HTTPS/GitHub Actions.
