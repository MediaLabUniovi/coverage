#include "uploader.h"
#include <LittleFS.h>
#include <WiFiClientSecure.h>

Uploader uploader;

bool Uploader::uploadCSV() {   // <- definición (tiene que coincidir con .h)
  // 1) Abre el fichero con los datos (LittleFS)
  File csvFile = LittleFS.open("/cov.csv", FILE_READ);
  if (!csvFile) {
    Serial.println("Error abriendo /cov.csv");
    return false;
  }
  size_t fileSize = csvFile.size();

  // 2) Prepara delimitadores y C-strings
  static const char host[]     = "medialab-uniovi.es";
  static const uint16_t port   = 443;
  static const char path[]     = "/cobertura/upload.php";
  static const char boundary[] = "----WebKitFormBoundary7MA4YWxkTrZu0gW";

  char preamble[256];
  int plen = snprintf(preamble, sizeof(preamble),
      "--%s\r\n"
      "Content-Disposition: form-data; name=\"csv\"; filename=\"cov.csv\"\r\n"
      "Content-Type: text/csv\r\n\r\n",
      boundary);
  if (plen < 0 || plen >= (int)sizeof(preamble)) {
    Serial.println("Error construyendo preamble");
    csvFile.close();
    return false;
  }

  char ending[64];
  int elen = snprintf(ending, sizeof(ending),
      "\r\n--%s--\r\n",
      boundary);
  if (elen < 0 || elen >= (int)sizeof(ending)) {
    Serial.println("Error construyendo ending");
    csvFile.close();
    return false;
  }

  size_t contentLength = (size_t)plen + fileSize + (size_t)elen;

  // 4) Conecta vía TLS
  WiFiClientSecure client;
  client.setInsecure();
  Serial.printf("Conectando a %s:%u…\n", host, port);
  if (!client.connect(host, port)) {
    Serial.println("Error al conectar TLS");
    csvFile.close();
    return false;
  }

  // 5) Envía petición HTTP/1.1
  client.print("POST ");
  client.print(path);
  client.print(" HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\r\n");
  client.print("User-Agent: ESP32\r\n");
  client.print("Connection: close\r\n");
  client.print("Content-Type: multipart/form-data; boundary=");
  client.print(boundary);
  client.print("\r\n");
  client.print("Content-Length: ");
  client.print(contentLength);
  client.print("\r\n\r\n");

  // 6) Stream del cuerpo
  client.write((const uint8_t*)preamble, plen);

  uint8_t buf[256];
  while (csvFile.available()) {
    size_t n = csvFile.read(buf, sizeof(buf));
    client.write(buf, n);
  }
  csvFile.close();

  client.write((const uint8_t*)ending, elen);

  // 7) Lee respuesta
  unsigned long timeout = millis() + 8000;
  while (!client.available() && millis() < timeout) {
    delay(10);
  }
  if (!client.available()) {
    Serial.println("Timeout leyendo respuesta");
    client.stop();
    return false;
  }

  // Leer la primera linea entera para parsear el codigo de estado HTTP (aprox 15-20 bytes)
  char statusLine[64] = {0};
  int idx = 0;
  while (client.available() && idx < (sizeof(statusLine) - 1)) {
    char c = client.read();
    statusLine[idx++] = c;
    if (c == '\n') break;
  }
  Serial.print(statusLine);

  // Descartar el resto de la respuesta del servidor bloque a bloque para no saturar memoria
  uint8_t discardBuf[128];
  while (client.available()) {
    client.read(discardBuf, sizeof(discardBuf));
  }

  client.stop();
  return (strncmp(statusLine, "HTTP/1.1 200", 12) == 0);
}
