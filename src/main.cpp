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
String currentVersion = "1.0.8"; // Versión actual del firmware

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

// S3 Cert
const char amazonRootCACert[] PROGMEM =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIGhTCCBW2gAwIBAgIRAJB3NEFHMW75lZl2eur98bkwDQYJKoZIhvcNAQELBQAw\n"
    "gY8xCzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAO\n"
    "BgNVBAcTB1NhbGZvcmQxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRlZDE3MDUGA1UE\n"
    "AxMuU2VjdGlnbyBSU0EgRG9tYWluIFZhbGlkYXRpb24gU2VjdXJlIFNlcnZlciBD\n"
    "QTAeFw0yNTAzMDcwMDAwMDBaFw0yNjAzMDcyMzU5NTlaMBYxFDASBgNVBAMMCyou\n"
    "Z2l0aHViLmlvMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxKQLElVm\n"
    "JYKnZ9dmKMWrb4fy4BWFm658EQemS4hJgrt+1NFpL2tGVaFupVyV3vmKorCX3zej\n"
    "c7+gH8Ugpemmj9r5tk1NZ0SXXALTjvT2i03oSqjwCzkn+R1o0TYg+G7PyQ5pE18A\n"
    "E+K3VUcpch1f5RyBTEvE4+HUg4/6OpAIYFVznJ3yk8a+bo1i/HBp2MbtPzssSlT8\n"
    "mPLY76SETtKdwgIdY91MkTiJd1x0KJDM2GPKM7pNTc81NMSw6WBzsxg4PFbR+BCY\n"
    "82/sYu8iMy/AdYcUz72hh2DGXnVypzzV/LLgJ/VAP5m+md0lVH5KIG/cduDrajlo\n"
    "LQ4LKJktO4VmwQIDAQABo4IDUjCCA04wHwYDVR0jBBgwFoAUjYxexFStiuF36Zv5\n"
    "mwXhuAGNYeEwHQYDVR0OBBYEFBLwftAxb+SvNbWJ+0LZ7bcLk80EMA4GA1UdDwEB\n"
    "/wQEAwIFoDAMBgNVHRMBAf8EAjAAMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEF\n"
    "BQcDAjBJBgNVHSAEQjBAMDQGCysGAQQBsjEBAgIHMCUwIwYIKwYBBQUHAgEWF2h0\n"
    "dHBzOi8vc2VjdGlnby5jb20vQ1BTMAgGBmeBDAECATCBhAYIKwYBBQUHAQEEeDB2\n"
    "ME8GCCsGAQUFBzAChkNodHRwOi8vY3J0LnNlY3RpZ28uY29tL1NlY3RpZ29SU0FE\n"
    "b21haW5WYWxpZGF0aW9uU2VjdXJlU2VydmVyQ0EuY3J0MCMGCCsGAQUFBzABhhdo\n"
    "dHRwOi8vb2NzcC5zZWN0aWdvLmNvbTCCAX4GCisGAQQB1nkCBAIEggFuBIIBagFo\n"
    "AHYAlpdkv1VYl633Q4doNwhCd+nwOtX2pPM2bkakPw/KqcYAAAGVbeysdQAABAMA\n"
    "RzBFAiEA+YIgsAqb2cqQVlF4JP2ERIVCH3RXdB7DjIPc6Ch5aK4CIHjqUoV7F5Mk\n"
    "fcIQcmdn7Z5UR8nYtPA2OLvYc3mCFcLuAHcAGYbUxyiqb/66A294Kk0BkarOLXIx\n"
    "D67OXXBBLSVMx9QAAAGVbeysDgAABAMASDBGAiEAjryAbXlHsXj/v4f7CWXJzDUX\n"
    "SUuvA5kRH3doh4WPUQcCIQC+nojCqhCn/ZupbnI50O1T3FSKBQu/LOZ33fApzLJW\n"
    "hQB1AMs49xWJfIShRF9bwd37yW7ymlnNRwppBYWwyxTDFFjnAAABlW3srDcAAAQD\n"
    "AEYwRAIgS98L1D2W8nzV3tIQ0R4UJWxwxb7I/TT6e9ly0nA0QsACIFpl7s/WA1Qm\n"
    "z1Vm8ZtihoNFubO/AiiaVGaeDQiznHFCMHsGA1UdEQR0MHKCCyouZ2l0aHViLmlv\n"
    "ggwqLmdpdGh1Yi5jb22CFyouZ2l0aHVidXNlcmNvbnRlbnQuY29tggpnaXRodWIu\n"
    "Y29tgglnaXRodWIuaW+CFWdpdGh1YnVzZXJjb250ZW50LmNvbYIOd3d3LmdpdGh1\n"
    "Yi5jb20wDQYJKoZIhvcNAQELBQADggEBAHksjTVCptW9CtbBXu+7J2cDDmKRz/EA\n"
    "kUyONuojOnKoI3d2f5DQDkqzu/gSj6B28YO3a4EYFktvwq3KnXAu9KzSM1ehlhtA\n"
    "lxlvjjGUgXvux7DjnBH40ItKiE723opeWVbm2WExdRPSckm/CDwshz2U3Sl3M3Wt\n"
    "v0xPuZJrg1tMIL58RqrS5PpFlAIIlEUC6dr+xVQrwLNcYXVVgvZsRSX/YbrzboLM\n"
    "gWhuDSQPcaeDGHcy7NxRZHmlpHz+/Ot067VuxjGqm9veKNGZMUdroS+ocxAJBXv3\n"
    "Z1NCCowvpZazNxKccQg7izYwd6HL70WMxCWFU0e70uw9KZqteG7SVcQ=\n"
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

void performOTAUpdate(String binURL)
{
  WiFiClientSecure client;
  HTTPClient http;
  int redirects = 0;
  const int maxRedirects = 5; // Límite para evitar loops

  String currentURL = binURL;
  while (redirects < maxRedirects)
  {
    Serial.print("Iniciando descarga desde: ");
    Serial.println(currentURL);
    if (currentURL.startsWith("https://github.com"))
    {
      client.setCACert(rootCACert); // Usa S3 Cert
    }
    else
    {
      client.setCACert(amazonRootCACert); // Usa Amazon Cert
    }
    if (http.begin(client, currentURL))
    {
      Serial.println("Conexión HTTPS establecida");
      int httpCode = http.GET();
      Serial.print("HTTP Code: ");
      Serial.println(httpCode);
      if (httpCode == HTTP_CODE_OK)
      { // 200: Éxito, procede
        int contentLength = http.getSize();
        Serial.print("Tamaño del firmware: ");
        Serial.println(contentLength);
        if (Update.begin(contentLength))
        {
          Serial.println("Update.begin OK");
          size_t written = Update.writeStream(http.getStream());
          Serial.print("Bytes escritos: ");
          Serial.println(written);
          if (written == contentLength)
          {
            Serial.println("Escritura completa");
            if (Update.end(true))
            {
              Serial.println("Update exitoso! Rebooting...");
              ESP.restart();
            }
            else
            {
              Serial.print("Update.end falló. Error: ");
              Serial.println(Update.getError());
            }
          }
          else
            Serial.println("Escritura incompleta");
        }
        else
          Serial.println("Update.begin falló (tamaño inválido?)");
        http.end();
        return; // Update completado, sal
      }
      else if (httpCode == 302 || httpCode == 301)
      {                                  // Redirect: Sigue
        currentURL = http.getLocation(); // Obtiene nueva URL de headers
        Serial.print("Redirect detectado a: ");
        Serial.println(currentURL);
        http.end();
        redirects++;
      }
      else
      {
        Serial.print("HTTP GET falló con code: ");
        Serial.println(httpCode);
        http.end();
        return; // Falla definitiva
      }
    }
    else
    {
      Serial.println("Fallo en http.begin (cert/red?)");
      return;
    }
  }
  Serial.println("Demasiados redirects, abortando");
}

void checkForUpdates()
{
  WiFiClientSecure client;
  client.setCACert(rootCACert);
  HTTPClient http;
  String url = "https://" + String(githubHost) + repoPath;
  Serial.print("Chequeando updates en: ");
  Serial.println(url);
  if (http.begin(client, url))
  {
    Serial.println("Conexión HTTPS establecida para chequeo");
    http.addHeader("User-Agent", "ESP32-OTA"); // Para API GitHub
    int httpCode = http.GET();
    Serial.print("HTTP Code chequeo: ");
    Serial.println(httpCode);
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      Serial.print("Payload recibido (longitud): ");
      Serial.println(payload.length());
      DynamicJsonDocument doc(4096); // Tamaño para JSON
      DeserializationError error = deserializeJson(doc, payload);
      if (error)
      {
        Serial.print("deserializeJson falló: ");
        Serial.println(error.c_str());
        return;
      }
      String latestVersion = doc["tag_name"].as<String>(); // Ej. "v1.0.4"
      Serial.print("Versión latest: ");
      Serial.println(latestVersion.substring(1));
      Serial.print("Versión current: ");
      Serial.println(currentVersion);
      if (latestVersion.substring(1) > currentVersion)
      { // Quita 'v' y compara (ej. "1.0.4" > "1.0.3")
        firmwareURL = doc["assets"][0]["browser_download_url"].as<String>();
        Serial.print("URL del bin: ");
        Serial.println(firmwareURL);
        performOTAUpdate(firmwareURL);
      }
      else
        Serial.println("Firmware actualizado");
    }
    else
      Serial.println("HTTP GET chequeo falló");
    http.end();
  }
  else
    Serial.println("Fallo en http.begin chequeo (cert/red?)");
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
  if (WiFi.status() != WL_CONNECTED)
    connectWiFi();
  if (millis() - lastCheck > checkInterval)
  {
    lastCheck = millis();
    checkForUpdates(); // Chequea updates
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
