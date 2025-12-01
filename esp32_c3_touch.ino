#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include "secrets.h" // Must be created by user!

// ----------------------------
// Pin Definitions
// ----------------------------
#define TFT_CS        7
#define TFT_RST       10
#define TFT_DC        3
#define TFT_MOSI      6
#define TFT_SCK       4
#define TFT_MISO      5

#define TOUCH_CS      1
#define TOUCH_IRQ     0

// ----------------------------
// Objects
// ----------------------------
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
WebServer server(80);

// ----------------------------
// Globals
// ----------------------------
unsigned long lastWifiCheck = 0;
unsigned long lastGuiUpdate = 0;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600; // GMT+1
const int   daylightOffset_sec = 3600; // DST+1

// ----------------------------
// Functions
// ----------------------------

void setupWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  // Non-blocking wait in loop, but here we wait a bit to show status
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Init Time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  } else {
    Serial.println("\nWiFi Failed to connect.");
  }
}

void checkWifi() {
  unsigned long currentMillis = millis();
  // Check every 30 seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - lastWifiCheck >= 30000)) {
    Serial.print(currentMillis);
    Serial.println(" Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    lastWifiCheck = currentMillis;
  }
}

String getSystemInfoJson() {
  float temp = temperatureRead();
  uint32_t heap = ESP.getFreeHeap();
  
  struct tm timeinfo;
  char timeStringBuff[50] = "N/A";
  if(getLocalTime(&timeinfo)){
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
  }

  String json = "{";
  json += "\"temperature\": " + String(temp) + ",";
  json += "\"voltage\": \"N/A\","; // ESP32-C3 internal voltage read isn't standard API
  json += "\"free_heap\": " + String(heap) + ",";
  json += "\"time\": \"" + String(timeStringBuff) + "\"";
  json += "}";
  return json;
}

void handleRoot() {
  server.send(200, "application/json", getSystemInfoJson());
}

void drawDashboardStatic() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("ESP32-C3 Dashboard");
  
  tft.drawLine(0, 35, 320, 35, ILI9341_BLUE);
  
  tft.setTextSize(2);
  
  tft.setCursor(10, 60);
  tft.println("Time:");
  
  tft.setCursor(10, 100);
  tft.println("Temp:");
  
  tft.setCursor(10, 140);
  tft.println("Heap:");
  
  tft.setCursor(10, 180);
  tft.println("IP:");
}

void updateDashboard() {
  // Time
  struct tm timeinfo;
  tft.setCursor(80, 60);
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  if(getLocalTime(&timeinfo)){
    tft.printf("%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    tft.print("Syncing...");
  }
  
  // Temp
  tft.setCursor(80, 100);
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.printf("%.1f C", temperatureRead());
  
  // Heap
  tft.setCursor(80, 140);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.printf("%d B", ESP.getFreeHeap());
  
  // IP
  tft.setCursor(50, 180); // Shift left a bit
  tft.setTextColor(ILI9341_MAGENTA, ILI9341_BLACK);
  if (WiFi.status() == WL_CONNECTED) {
    tft.print(WiFi.localIP());
  } else {
    tft.print("Disconnected");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Dashboard...");

  // Init SPI & Display
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI);
  tft.begin(40000000); 
  tft.setRotation(1);
  
  drawDashboardStatic();
  
  // Init WiFi
  setupWifi();
  
  // Init Server
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  checkWifi();
  
  unsigned long currentMillis = millis();
  if (currentMillis - lastGuiUpdate >= 1000) {
    updateDashboard();
    lastGuiUpdate = currentMillis;
  }
}
