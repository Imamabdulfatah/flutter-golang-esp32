
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>

#define DHTPIN 2       // Pin untuk DHT11
#define DHTTYPE DHT11  // Tipe sensor DHT
#define FAN_PIN 14     // Pin untuk kipas

DHT dht(DHTPIN, DHTTYPE);

// Konfigurasi WiFi
const char* ssid = "IA";
const char* password = "imam887588";

// Membuat instance server
WebServer server(80);

// Variabel data integer dan status perangkat
// int dataInteger = 42;
bool deviceState = false; // Untuk mencatat status ON/OFF

// Fungsi untuk menangani permintaan data integer
void handleGetInteger() {
  float suhu = dht.readTemperature();

  Serial.println("Request received: /getInteger");

  // Log data ke Serial Monitor
  Serial.print("Data from ESP32: ");
  Serial.println(suhu);

  // Kirim data integer sebagai respons JSON
  StaticJsonDocument<100> jsonResponse;
  jsonResponse["data"] = suhu;
  String response;
  serializeJson(jsonResponse, response);
  server.send(200, "application/json", response);
}

// Fungsi untuk menangani tombol ON/OFF dari Flutter
void handlePostState() {
  float suhu = dht.readTemperature();
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"error\":\"No data received\"}");
    return;
  }

  String body = server.arg("plain");

  // Parsing JSON dari Flutter
  StaticJsonDocument<100> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  const char* state = doc["state"];

  // Ubah status perangkat berdasarkan data dari Flutter
  if (strcmp(state, "ON") == 0) {
    deviceState = true;
    Serial.println("Command received: TURN ON");
    digitalWrite(FAN_PIN, LOW);
    Serial.println("Kipas dinyalakan secara manual.");
  } else if (strcmp(state, "OFF") == 0) {
    deviceState = false;
    Serial.println("Command received: TURN OFF");
    digitalWrite(FAN_PIN, HIGH);
      Serial.println("Kipas dimatikan secara manual.");
  } else {
    Serial.println("Invalid command received.");
  }

  // Log status ke Serial Monitor
  Serial.print("Device state: ");
  Serial.println(deviceState ? "ON" : "OFF");
  Serial.print("Current dataInteger: ");
  Serial.println(suhu);

  // Kirim respons
  StaticJsonDocument<100> responseJson;
  responseJson["message"] = "State updated";
  responseJson["state"] = deviceState ? "ON" : "OFF";
  String response;
  serializeJson(responseJson, response);
  server.send(200, "application/json", response);
}

void setup() {
  Serial.begin(115200);

  // Menghubungkan ke WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, HIGH); // Matikan kipas pada awalnya

  dht.begin();


  // Menyiapkan endpoint
  server.on("/getInteger", HTTP_GET, handleGetInteger); // Endpoint untuk data integer
  server.on("/setState", HTTP_POST, handlePostState);  // Endpoint untuk tombol ON/OFF

  // Memulai server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
