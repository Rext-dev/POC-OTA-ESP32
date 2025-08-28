# OTA desde servidor y GitHub Releases (Rama `update-from-server`)

Esta rama implementa un flujo OTA donde el ESP32 consulta la API de GitHub Releases, sigue redirecciones HTTPS y descarga el binario para actualizarse usando `HTTPClient` + `Update.h` con `WiFiClientSecure`.

- Código principal: `src/main.cpp`
- Configuración: `platformio.ini`

## 1) Certificados y firma de la build

Esta rama incluye un certificado raíz de ejemplo embebido (constante `rootCACert`) para validar TLS hacia GitHub. Además, en la raíz del repo verás:
- `my_key.pem`: clave privada (para firmar en CI)
- `public_key.pem`: clave pública (para verificación del firmware si decides implementarla en el dispositivo)

Actualmente el código valida TLS por CA (`setCACert`) y NO valida la firma del binario. Puedes extenderlo para validar una firma `RSA/ECDSA` con mbedTLS si tu amenaza lo requiere.

### 1.1 Generar par de claves para firmar

Usa OpenSSL (RSA 2048 como ejemplo):

```bash
# Clave privada
openssl genrsa -out my_key.pem 2048

# Clave pública
openssl rsa -in my_key.pem -pubout -out public_key.pem
```

En CI, firma el binario y publica junto con él la firma `.sig` y, si no está embebida, la `public_key.pem`.

### 1.2 Obtener el certificado de confianza para GitHub (TLS)

Tienes dos opciones:
- Usar el bundle de CAs que trae el core ESP32 (más mantenimiento a futuro).
- Incrustar una CA raíz/intermedia válida para `api.github.com`/`github.com`.

Para extraer la cadena desde tu equipo:

```bash
# Descarga la cadena de certs (puede variar por CDN/SNI)
openssl s_client -showcerts -connect api.github.com:443 </dev/null 2>/dev/null \
  | sed -n '/BEGIN CERTIFICATE/,/END CERTIFICATE/p' > github_chain.pem
```

Luego copia el certificado raíz apropiado y embébelo en el firmware como C-string:

```c
const char rootCACert[] PROGMEM = "-----BEGIN CERTIFICATE-----\n"
"...base64...\n"
"-----END CERTIFICATE-----\n";
```

Para pruebas o ambientes no productivos, puedes desactivar la verificación TLS del lado del ESP32:

```cpp
WiFiClientSecure client;
client.setInsecure(); // PRUEBAS solamente: desactiva validación TLS
```

En producción, evita `setInsecure()` y usa `client.setCACert(rootCACert)`.

## 2) URL(es) de descarga y redirecciones

El código consulta:

```
https://api.github.com/repos/<owner>/<repo>/releases/latest
```

Obtiene `assets[0].browser_download_url` y sigue redirecciones (`301/302`) hasta una respuesta `200 OK` para leer el binario. Si quieres usar una segunda URL (p. ej. servidor propio) puedes pasar directamente la URL final de tu archivo `.bin`:

```cpp
String firmwareURL = "https://tu-servidor/firmware/esp32s3/firmware.bin";
// Para pruebas con servidor propio sin TLS válido:
client.setInsecure(); // SOLO pruebas; en producción usa CA propia: client.setCACert(miCA)
```

## 3) Pipeline con GitHub Actions: cómo funciona y cómo crear uno

Objetivo: compilar el firmware con PlatformIO, firmarlo con `my_key.pem`, crear un Release y adjuntar artefactos (`firmware.bin`, `littlefs.bin`, `firma .sig`).

Pasos típicos del workflow:
1. Disparador: `push` a `main` o lanzamiento manual (`workflow_dispatch`).
2. Checkout del repo.
3. Instalar PlatformIO.
4. Build de `firmware.bin` y (opcional) `littlefs.bin`.
5. Firmar el binario con `my_key.pem`.
6. Crear/actualizar Release y subir artefactos.

Ejemplo de `/.github/workflows/release.yml`:

```yaml
name: Build and Release ESP32
on:
  push:
    branches: [ main ]
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'

      - name: Install PlatformIO
        run: pip install -U platformio

      - name: Build firmware and FS
        run: |
          pio run -e esp32s3-usb
          pio run -e esp32s3-usb -t buildfs

      - name: Write private key from secret
        run: |
          echo "$FIRMWARE_PRIVATE_KEY" > my_key.pem
          chmod 600 my_key.pem
        env:
          FIRMWARE_PRIVATE_KEY: ${{ secrets.FIRMWARE_PRIVATE_KEY }}

      - name: Sign firmware
        run: |
          openssl dgst -sha256 -sign my_key.pem \
            -out firmware.bin.sig .pio/build/esp32s3-usb/firmware.bin

      - name: Create GitHub Release
        uses: softprops/action-gh-release@v2
        with:
          generate_release_notes: true
          files: |
            .pio/build/esp32s3-usb/firmware.bin
            .pio/build/esp32s3-usb/littlefs.bin
            firmware.bin.sig
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

Notas:
- Guarda la clave privada en `secrets` como `FIRMWARE_PRIVATE_KEY`. No la subas al repo.
- Puedes adjuntar `public_key.pem` o embutir su contenido en el firmware para validación futura de firmas.

## 4) Uso en el dispositivo

En `src/main.cpp` se configura:
- `githubHost` y `repoPath` del Release a consultar.
- `rootCACert` para `client.setCACert(rootCACert)` cuando `currentURL` apunta a GitHub.
- `client.setInsecure()` como fallback para hosts no GitHub (sólo pruebas).
- Manejo de redirecciones hasta 5 saltos y escritura con `Update.writeStream()`.

Ciclo de vida:
- Chequea cada `checkInterval` (1 minuto por defecto). Ajusta `checkInterval` si lo necesitas.
- Al detectar versión más nueva (comparando `tag_name` vs `currentVersion`) descarga e instala el binario y reinicia (`ESP.restart()`).

## 5) Seguridad
- Evita `setInsecure()` en producción; usa siempre una CA válida (`setCACert`).
- Firma los binarios en CI y considera verificar la firma en el dispositivo.
- Usa Releases privados o un servidor con TLS vigente. Sincroniza el reloj (NTP) para que TLS funcione.

## 6) Solución de problemas
- `HTTP GET` falla: verifica hora/NTP, conectividad y la CA embebida.
- `deserializeJson` falla: aumenta `DynamicJsonDocument` si la respuesta de GitHub crece.
- "Demasiados redirects": revisa que la URL del asset sea válida y accesible.
- OTA incompleto: confirma `contentLength`, particiones (`partitions.csv`) y espacio disponible.
- El dispositivo no ve nuevas versiones: asegura que el `tag_name` del Release sea mayor (ej. `v1.1.0` > `v1.0.9`).
