/*
 * ESP32 Control Hub
 * =================
 * Full-featured control panel with WiFiManager, WebServer, and PWA.
 *
 * Features:
 * - Auto WiFi configuration via WiFiManager
 * - Web-based control panel (LED, Relay, Motor)
 * - Sensor readings (DHT11 temperature/humidity)
 * - Connected devices tracking
 * - JSON config in LittleFS
 * - PWA support (install on mobile)
 *
 * Hardware:
 * - ESP32 DevKit
 * - 2x LED (GPIO 2, 4)
 * - 2x Relay (GPIO 5, 18)
 * - 1x Motor (GPIO 19)
 * - 1x DHT11 (GPIO 34) [optional]
 *
 * Libraries needed (install via Arduino IDE Library Manager):
 * - WiFiManager by tzapu
 * - ArduinoJson by Benoit Blanchon (v6.x)
 * - DHT sensor library by Adafruit
 * - Adafruit Unified Sensor
 *
 * Board: ESP32 Dev Module
 * Partition: Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ===== Configuration =====
#define DEVICE_NAME "ESP32-Control-Hub"
#define AP_SSID "ESP32-Control"
#define AP_PASS "12345678"
#define FW_VERSION "1.0.0"

// ===== Pin Definitions =====
#define PIN_LED1    2
#define PIN_LED2    4
#define PIN_RELAY1  5
#define PIN_RELAY2  18
#define PIN_MOTOR   19
#define PIN_DHT     34
#define DHT_TYPE    DHT11

// ===== Objects =====
WebServer server(80);
WiFiManager wifiManager;
DHT dht(PIN_DHT, DHT_TYPE);

// ===== State =====
DynamicJsonDocument config(4096);
DynamicJsonDocument pinStates(2048);

// ===== Pin Mapping =====
int pinMap[5] = {PIN_LED1, PIN_LED2, PIN_RELAY1, PIN_RELAY2, PIN_MOTOR};
String pinNames[5] = {"led1", "led2", "relay1", "relay2", "motor"};

// ===== Connected Devices =====
struct Device {
  String ip;
  String userAgent;
  unsigned long lastSeen;
};
Device devices[10];
int deviceCount = 0;

// ===== Load Config from LittleFS =====
bool loadConfig() {
  if (!LittleFS.exists("/config.json")) {
    Serial.println("config.json not found, using defaults");
    return false;
  }
  File f = LittleFS.open("/config.json", "r");
  if (!f) { Serial.println("Failed to open config.json"); return false; }
  DeserializationError err = deserializeJson(config, f);
  f.close();
  if (err) {
    Serial.print("JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }
  Serial.println("Config loaded successfully");
  return true;
}

// ===== Initialize Pins =====
void initPins() {
  for (int i = 0; i < 5; i++) {
    pinMode(pinMap[i], OUTPUT);
    digitalWrite(pinMap[i], LOW);
    pinStates[pinNames[i]] = 0;
  }
  dht.begin();
  Serial.println("Pins initialized");
}

// ===== Serve File from LittleFS =====
bool serveFile(String path, String contentType) {
  if (path.endsWith("/")) path += "index.html";
  if (!LittleFS.exists(path)) return false;
  File f = LittleFS.open(path, "r");
  server.streamFile(f, contentType);
  f.close();
  return true;
}

// ===== Track Connected Device =====
void trackDevice(String ip, String ua) {
  for (int i = 0; i < deviceCount; i++) {
    if (devices[i].ip == ip) {
      devices[i].lastSeen = millis();
      devices[i].userAgent = ua;
      return;
    }
  }
  if (deviceCount < 10) {
    devices[deviceCount].ip = ip;
    devices[deviceCount].userAgent = ua;
    devices[deviceCount].lastSeen = millis();
    deviceCount++;
    Serial.print("New device: ");
    Serial.println(ip);
  }
}

// ===== HTTP Handlers =====

void handleRoot() {
  String ip = server.client().remoteIP().toString();
  String ua = server.header("User-Agent");
  trackDevice(ip, ua);
  if (!serveFile("/index.html", "text/html")) {
    server.send(500, "text/plain", "index.html not found. Upload data folder to LittleFS.");
  }
}

void handleConfig() {
  String out;
  serializeJson(config, out);
  server.send(200, "application/json", out);
}

void handleState() {
  String out;
  serializeJson(pinStates, out);
  server.send(200, "application/json", out);
}

void handleToggle() {
  if (!server.hasArg("pin")) {
    server.send(400, "text/plain", "Missing pin parameter");
    return;
  }
  String pinName = server.arg("pin");
  int foundIdx = -1;
  for (int i = 0; i < 5; i++) {
    if (pinName == pinNames[i]) {
      foundIdx = i;
      break;
    }
  }
  if (foundIdx == -1) {
    server.send(404, "text/plain", "Unknown pin");
    return;
  }
  int newState = pinStates[pinName].as<int>() ? 0 : 1;
  digitalWrite(pinMap[foundIdx], newState);
  pinStates[pinName] = newState;
  String resp = "{\"pin\":\"" + pinName + "\",\"state\":" + String(newState) + "}";
  server.send(200, "application/json", resp);
  Serial.print("Toggled ");
  Serial.print(pinName);
  Serial.print(" -> ");
  Serial.println(newState ? "ON" : "OFF");
}

void handleSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t)) t = 0;
  if (isnan(h)) h = 0;
  String resp = "{\"temperature\":" + String(t, 1) + ",\"humidity\":" + String(h, 1) + ",\"unit_temp\":\"C\",\"unit_hum\":\"%\"}";
  server.send(200, "application/json", resp);
}

void handleDevices() {
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();
  unsigned long now = millis();
  for (int i = 0; i < deviceCount; i++) {
    if (now - devices[i].lastSeen > 60000) continue;
    JsonObject o = arr.createNestedObject();
    o["ip"] = devices[i].ip;
    o["userAgent"] = devices[i].userAgent;
  }
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleInfo() {
  String resp = "{\"device\":\"" + String(DEVICE_NAME) + "\",\"version\":\"" + String(FW_VERSION) + "\",\"ip\":\"" + WiFi.localIP().toString() + "\",\"uptime\":" + String(millis() / 1000) + "}";
  server.send(200, "application/json", resp);
}

void handleStatic() {
  String path = server.uri();
  String ct = "text/plain";
  if (path.endsWith(".css")) ct = "text/css";
  else if (path.endsWith(".js")) ct = "application/javascript";
  else if (path.endsWith(".json")) ct = "application/json";
  else if (path.endsWith(".png")) ct = "image/png";
  else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) ct = "image/jpeg";
  else if (path.endsWith(".svg")) ct = "image/svg+xml";
  else if (path.endsWith(".ico")) ct = "image/x-icon";
  if (!serveFile(path, ct)) {
    server.send(404, "text/plain", "Not found: " + path);
  }
}

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== ESP32 Control Hub ===");
  Serial.println(FW_VERSION);

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount FAILED");
    return;
  }
  Serial.println("LittleFS mounted");

  loadConfig();
  initPins();

  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setAPCallback([](WiFiManager *wm) {
    Serial.println("Config portal started");
    Serial.print("Connect to: ");
    Serial.println(AP_SSID);
  });

  Serial.println("Connecting to WiFi...");
  if (!wifiManager.autoConnect(AP_SSID, AP_PASS)) {
    Serial.println("Failed to connect, restarting...");
    delay(3000);
    ESP.restart();
  }
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/api/config", handleConfig);
  server.on("/api/state", handleState);
  server.on("/api/toggle", handleToggle);
  server.on("/api/sensors", handleSensors);
  server.on("/api/devices", handleDevices);
  server.on("/api/info", handleInfo);
  server.onNotFound(handleStatic);

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("Open: http://");
  Serial.println(WiFi.localIP());
  Serial.println("============================\n");
}

// ============================================================
// Loop
// ============================================================
void loop() {
  server.handleClient();
  delay(2);
}
