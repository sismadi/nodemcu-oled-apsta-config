/*
  NodeMCU ESP8266 + OLED SSD1306
  AP (Setup) + STA (Normal) — Simpan SSID, Password, API GET/POST
  HTTP/HTTPS via HTTPClient (WiFiClientSecure untuk HTTPS)

  Wiring OLED:
    VCC -> 3V3
    GND -> GND
    SCL -> D4 (GPIO2)
    SDA -> D3 (GPIO0)  <-- hati-hati: jangan LOW saat boot
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// ===================== OLED =====================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void showOLED(const String &l1, const String &l2 = "", const String &l3 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);  display.println(l1);
  if (l2.length()) { display.setCursor(0, 10); display.println(l2); }
  if (l3.length()) { display.setCursor(0, 20); display.println(l3); }
  display.display();
}

// ===================== CONFIG / EEPROM =====================
struct Config {
  uint32_t magic;           // "CFG1"
  char ssid[32];
  char pass[32];
  char apiGet[128];         // simpan URL lengkap, contoh: https://api.sismadi.com/iot
  char apiPost[128];        // simpan URL lengkap POST, contoh: https://api.sismadi.com/iot/post
};
Config config;
const uint32_t CFG_MAGIC = 0x43464731;

void saveConfig() { EEPROM.put(0, config); EEPROM.commit(); }
void loadConfig() {
  EEPROM.get(0, config);
  if (config.magic != CFG_MAGIC) {
    memset(&config, 0, sizeof(config));
    config.magic = CFG_MAGIC;
    // nilai default (opsional)
    strcpy(config.apiGet,  "https://jsonplaceholder.typicode.com/users/1");
    strcpy(config.apiPost, "");
    saveConfig();
  }
}
bool hasWiFiCreds() { return strlen(config.ssid) > 0; }

// ===================== WEB SERVER (AP Setup) =====================
ESP8266WebServer server(80);

String htmlHeader() {
  return F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
           "<style>body{font-family:Arial;padding:16px}input{width:100%;padding:8px;margin:6px 0}"
           "label{display:block;margin-top:8px}</style></head><body>");
}
String htmlFooter() { return F("</body></html>"); }

void handleRoot() {
  String h = htmlHeader();
  h += F("<h2>Setup WiFi & API</h2><form action='/save' method='GET'>");
  h += String("SSID:<input name='ssid' value='") + String(config.ssid) + "'><br>";
  h += String("Password:<input name='pass' type='password' value='") + String(config.pass) + "'><br>";
  h += String("API GET URL:<input name='get' value='") + String(config.apiGet) + "' "
       "placeholder='https://api.sismadi.com/iot'><br>";
  h += String("API POST URL:<input name='post' value='") + String(config.apiPost) + "' "
       "placeholder='https://api.sismadi.com/iot/post'><br>";
  h += F("<input type='submit' value='Save'></form><hr>");
  h += "<p>STA IP: " + WiFi.localIP().toString() + " | AP IP: " + WiFi.softAPIP().toString() + "</p>";
  h += htmlFooter();
  server.send(200, "text/html", h);
}

void handleSave() {
  if (server.hasArg("ssid")) server.arg("ssid").toCharArray(config.ssid, sizeof(config.ssid));
  if (server.hasArg("pass")) server.arg("pass").toCharArray(config.pass, sizeof(config.pass));
  if (server.hasArg("get"))  server.arg("get").toCharArray(config.apiGet, sizeof(config.apiGet));
  if (server.hasArg("post")) server.arg("post").toCharArray(config.apiPost, sizeof(config.apiPost));
  config.magic = CFG_MAGIC;
  saveConfig();
  server.send(200, "text/html", "Saved! Restarting...");
  delay(700);
  ESP.restart();
}

// ===================== WIFI (AP + STA) =====================
void startAP_STA() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("SetupNodeMCU", "12345678");  // AP selalu aktif
  delay(100);

  if (hasWiFiCreds()) {
    WiFi.begin(config.ssid, config.pass);
    showOLED("Connecting...", config.ssid, "AP: " + WiFi.softAPIP().toString());
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
      delay(300); Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      showOLED("WiFi Connected!", WiFi.localIP().toString(), "AP: " + WiFi.softAPIP().toString());
    } else {
      showOLED("AP Setup Mode", "SSID: SetupNodeMCU", "IP: " + WiFi.softAPIP().toString());
    }
  } else {
    showOLED("AP Setup Mode", "SSID: SetupNodeMCU", "IP: " + WiFi.softAPIP().toString());
  }

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
  Serial.println("Setup page: http://192.168.4.1/");
}

// ===================== UTIL URL =====================
bool isHttpsUrl(const String &url) { return url.startsWith("https://"); }

// ===================== GET JSON (HTTP/HTTPS) =====================
bool httpGET_JSON(const String &url, String &payloadOut, int &httpCodeOut) {
  payloadOut = ""; httpCodeOut = 0;
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  bool ok = false;

  if (isHttpsUrl(url)) {
    WiFiClientSecure cli; cli.setInsecure();
    if (http.begin(cli, url)) {
      http.addHeader("Accept", "application/json");
      http.addHeader("Accept-Encoding", "identity");   // cegah kompresi
      http.setTimeout(15000);
      httpCodeOut = http.GET();
      if (httpCodeOut > 0) {
        payloadOut = http.getString();
        ok = (httpCodeOut == HTTP_CODE_OK || httpCodeOut == HTTP_CODE_MOVED_PERMANENTLY);
      }
      http.end();
    }
  } else {
    WiFiClient cli;
    if (http.begin(cli, url)) {
      http.addHeader("Accept", "application/json");
      http.addHeader("Accept-Encoding", "identity");
      http.setTimeout(15000);
      httpCodeOut = http.GET();
      if (httpCodeOut > 0) {
        payloadOut = http.getString();
        ok = (httpCodeOut == HTTP_CODE_OK || httpCodeOut == HTTP_CODE_MOVED_PERMANENTLY);
      }
      http.end();
    }
  }
  return ok;
}

// ===================== POST JSON (HTTP/HTTPS) =====================
bool httpPOST_JSON(const String &url, const String &jsonPayload, String &respOut, int &httpCodeOut) {
  respOut = ""; httpCodeOut = 0;
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  bool ok = false;

  if (isHttpsUrl(url)) {
    WiFiClientSecure cli; cli.setInsecure();
    if (http.begin(cli, url)) {
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Accept-Encoding", "identity");
      http.setTimeout(15000);
      httpCodeOut = http.POST(jsonPayload);
      if (httpCodeOut > 0) { respOut = http.getString(); ok = true; }
      http.end();
    }
  } else {
    WiFiClient cli;
    if (http.begin(cli, url)) {
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Accept-Encoding", "identity");
      http.setTimeout(15000);
      httpCodeOut = http.POST(jsonPayload);
      if (httpCodeOut > 0) { respOut = http.getString(); ok = true; }
      http.end();
    }
  }
  return ok;
}

// ===================== LOGIKA UTAMA =====================
unsigned long lastGet = 0;
unsigned long lastPost = 0;

void fetchAndDisplayJson() {
  String url = String(config.apiGet);
  if (url.length() == 0) { showOLED("GET not set", "Open: 192.168.4.1"); return; }

  showOLED("Fetching...", url.substring(0, 18), "");
  String payload; int code;
  bool ok = httpGET_JSON(url, payload, code);

  if (!ok) {
    showOLED("GET Error", "HTTP: " + String(code));
    Serial.printf("[HTTP] GET failed code=%d\n", code);
    return;
  }

  Serial.println("\n--- RAW JSON BODY (GET) ---");
  Serial.println(payload);
  Serial.println("---------------------------");

  // Parse JSON dan ambil 'name' (dengan fallback)
  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    showOLED("JSON parse", err.c_str());
    Serial.printf("JSON parse failed: %s\n", err.c_str());
    return;
  }

  String name = "No Name";
  JsonVariantConst root = doc.as<JsonVariantConst>();
  if (root.is<JsonObjectConst>()) {
    JsonObjectConst o = root.as<JsonObjectConst>();
    if (o["name"].is<const char*>())        name = o["name"].as<const char*>();
    else if (o["nama"].is<const char*>())   name = o["nama"].as<const char*>();
    else if (o["message"].is<const char*>())name = o["message"].as<const char*>();
    else if (o["title"].is<const char*>())  name = o["title"].as<const char*>();
  }
  if (name == "No Name" && root.is<JsonArrayConst>()) {
    JsonArrayConst a = root.as<JsonArrayConst>();
    if (a.size() > 0 && a[0]["name"].is<const char*>()) name = a[0]["name"].as<const char*>();
  }

  showOLED("GET OK", name, "IP: " + WiFi.localIP().toString());
  Serial.printf("Name: %s\n", name.c_str());
}

void maybePostJson() {
  if (strlen(config.apiPost) == 0) return;  // POST opsional
  StaticJsonDocument<256> p;
  p["device"] = "NodeMCU";
  p["rssi"] = WiFi.RSSI();
  p["uptime_ms"] = millis();
  String payload; serializeJson(p, payload);

  String resp; int code;
  bool ok = httpPOST_JSON(String(config.apiPost), payload, resp, code);
  if (ok) {
    Serial.printf("[POST] code=%d resp.len=%u\n", code, (unsigned)resp.length());
  } else {
    Serial.printf("[POST] failed code=%d\n", code);
  }
}

// ===================== SETUP / LOOP =====================
void setup() {
  Serial.begin(115200);

  // >>>>>>> PIN I2C DIPINDAHKAN: SDA=D3, SCL=D4 <<<<<<<
  Wire.begin(D3, D4);  // SDA, SCL

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED not found at 0x3C"));
    while (true) { delay(1000); }
  }
  display.clearDisplay(); display.display();

  EEPROM.begin(512);
  loadConfig();
  startAP_STA();
}

void loop() {
  server.handleClient();

  // GET setiap 10s
  if (millis() - lastGet > 10000) {
    lastGet = millis();
    if (WiFi.status() == WL_CONNECTED) fetchAndDisplayJson();
    else showOLED("WiFi not conn.", "Open: 192.168.4.1");
  }

  // POST setiap 30s bila diisi
  if (millis() - lastPost > 30000) {
    lastPost = millis();
    if (WiFi.status() == WL_CONNECTED) maybePostJson();
  }
}
