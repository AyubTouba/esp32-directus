#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "DHT.h"

/////////////////////////////////////
////// USER DEFINED VARIABLES //////
///////////////////////////////////
/// WIFI Settings ///
const char* ssid = "ssid";
const char* password = "password";

/// Socket.IO Settings ///
char host[] = "IP";    // Socket.IO Server Address
int port = 8055;                  // Socket.IO Port Address
char path[] = "/websocket";       // Socket.IO Base Path

const char* access_token = "DirctusToken";
const char* collection = "dht_sensor";

/// Pin Settings ///
#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// time excuting
unsigned long startTime;
const int delayTime = 5000;
unsigned long currentTime;
bool isReadyToSend = false;

/////////////////////////////////////
////// ESP32 Socket.IO Client //////
///////////////////////////////////

WebSocketsClient webSocket;
WiFiClient client;


void authenticate() {
  JsonDocument doc;
  doc["type"] = "auth";
  doc["access_token"] = access_token;

  String message;
  serializeJson(doc, message);
  webSocket.sendTXT(message);
}

void traitPayload(uint8_t* payload) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  const char* typeValue = doc["type"];
  const char* statusValue = doc["status"];
  const char* event = doc["event"];

  if (strcmp(typeValue, "auth") == 0 && strcmp(statusValue, "ok") == 0) {
    Serial.println("[EVENT] SUBSCRIBE");
    subscribe();
  }
  if (strcmp(typeValue, "subscription") == 0 && strcmp(event, "init") == 0) {
    Serial.println("[EVENT] INIT TO SEND");
    isReadyToSend = true;
  }
  if (strcmp(typeValue, "ping") == 0) {
    Serial.println("[EVENT] SEND PONG");
    pong();
  }
}

void subscribe() {
  JsonDocument doc;
  doc["type"] = "subscribe";
  doc["collection"] = collection;
  doc["query"]["fields"] = doc["query"]["fields"].to<JsonArray>();
  doc["query"]["fields"].add("*");

  String message;
  serializeJson(doc, message);

  Serial.println("[EVENT] SEND SUBSCRIBE");
  Serial.println(message);

  webSocket.sendTXT(message);
}
void pong() {
  JsonDocument doc;
  doc["type"] = "pong";
  String message;
  serializeJson(doc, message);
  webSocket.sendTXT(message);
}
void sendData() {
  float humidity = dht.readHumidity();
  float temperatureC = dht.readTemperature();

  if (isnan(humidity) || isnan(temperatureC)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("Temperature C: ");
  Serial.println(temperatureC);

  JsonDocument doc;
  doc["type"] = "items";
  doc["collection"] = collection;
  doc["action"] = "create";
  doc["data"]["humidity"] = humidity;
  doc["data"]["temperatureC"] = temperatureC;

  String message;
  serializeJson(doc, message);

  Serial.println("[EVENT] SEND DATA");

  webSocket.sendTXT(message);
}
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);

      // send Auth to server when Connected
      authenticate();
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      traitPayload(payload);
      break;
  }
}
void setup() {
  startTime = millis();

  Serial.begin(115200);
  delay(10);


  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  webSocket.onEvent(webSocketEvent);
  // Setup Connection
  webSocket.begin(host, port, path);
}

void loop() {
  webSocket.loop();
  currentTime = millis();

  if (isReadyToSend == true && currentTime - startTime >= delayTime) {
    sendData();
    startTime = currentTime;  // Reset start time for next execution
  }
}
