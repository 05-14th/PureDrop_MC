#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char *ssid = "dlink-6988";
const char *password = "ayxid69359";
String data;

ESP8266WebServer server(80);

void setup() {
  Serial.begin(9600);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Define HTTP routes
  server.on("/data", HTTP_GET, handleGetData);

  // Start HTTP server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  if (Serial.available() > 0) {
        data = Serial.readStringUntil('\n'); // Read the data until newline character
        Serial.println("Received data: " + data);
    }
}

void handleGetData() {
  if (Serial.available() > 0) {
        data = Serial.readStringUntil('\n'); // Read the data until newline character
        Serial.println("Received data: " + data);
    }
  server.send(200, "text/plain", data);
}
