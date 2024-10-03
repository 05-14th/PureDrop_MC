
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

//const char *ssid = "dlink-6988";
//const char *password = "ayxid69359";
const char *ssid = "SiJi";
const char *password = "10072003";
String data;

ESP8266WebServer server(80);
const int relayPin = D4; 

void setup() {
  Serial.begin(9600);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Define HTTP routes
  server.on("/data", HTTP_GET, handleGetData);
  server.on("/get_switch_state", HTTP_POST, handleGetSwitchState);

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

void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><title>ESP8266 Relay Control</title></head>";
  html += "<body><h1>ESP8266 Relay Control</h1>";
  html += "<form action=\"/switch\" method=\"POST\">";
  html += "<input type=\"radio\" name=\"state\" value=\"on\" checked> ON<br>";
  html += "<input type=\"radio\" name=\"state\" value=\"off\"> OFF<br>";
  html += "<input type=\"submit\" value=\"Submit\">";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleGetSwitchState() {
  // Read the data from the request body
  String requestBody = server.arg("plain"); // Get the request body
  Serial.println("Received POST data: " + requestBody);
   // Process the request body to set the relay state
  if (requestBody.indexOf("on") >= 0) {
    digitalWrite(relayPin, HIGH); // Turn ON the relay
    server.send(200, "text/plain", "Relay is ON");
  } else if (requestBody.indexOf("off") >= 0) {
    digitalWrite(relayPin, LOW); // Turn OFF the relay
    server.send(200, "text/plain", "Relay is OFF");
  } else {
    server.send(400, "text/plain", "Invalid state");
  }
}