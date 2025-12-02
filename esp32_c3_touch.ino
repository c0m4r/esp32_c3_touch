#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include "secrets.h"

// Pin Definitions
#define TFT_CS        7
#define TFT_RST       10
#define TFT_DC        3
#define TFT_MOSI      6
#define TFT_SCK       4
#define TFT_MISO      5
#define TOUCH_CS      1
#define TOUCH_IRQ     0

// Touch Calibration
#define TS_MINX 200
#define TS_MINY 300
#define TS_MAXX 3800
#define TS_MAXY 3800

// Objects
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
WebServer server(80);

// Globals
unsigned long lastWifiCheck = 0;
unsigned long lastUpdate = 0;
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// UI State
int8_t expandedBox = -1; // -1 = none, 0-3 = expanded box

// Colors
#define BG_COLOR      0x0000    // Black
#define TEXT_COLOR    0xFFFF    // White
#define BORDER_COLOR  0x39C7    // Gray
#define HIGHLIGHT     0xFD20    // Orange
#define BOX_BG        0x1082    // Dark blue
#define GRAPH_BG      0x2145    // Dark gray
#define GRAPH_FILL    0x07E0    // Green

// Double buffering - canvas for smooth rendering
GFXcanvas16 *canvas = nullptr;

// Helper to draw canvas to screen at position
void flushCanvas(int x, int y, int w, int h) {
  if (canvas) {
    tft.drawRGBBitmap(x, y, canvas->getBuffer(), w, h);
  }
}

void setupWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.println(WiFi.localIP());
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
  } else {
    Serial.println("\nWiFi Failed. Will retry later.");
  }
}

void checkWifi() {
  unsigned long currentMillis = millis();
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - lastWifiCheck >= 30000)) {
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    lastWifiCheck = currentMillis;
  }
}

String getSystemInfoJson() {
  float temp = temperatureRead();
  uint32_t heapFree = ESP.getFreeHeap();
  uint32_t flashSize = ESP.getFlashChipSize();
  uint32_t flashFree = flashSize - ESP.getSketchSize();
  unsigned long uptimeSeconds = millis() / 1000;
  int wifiStrength = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : -100;
  
  struct tm timeinfo;
  char timeStringBuff[50] = "N/A";
  if(getLocalTime(&timeinfo)){
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
  }

  String json = "{";
  json += "\"temperature\": " + String(temp) + ",";
  json += "\"free_ram\": " + String(heapFree / 1024) + ",";
  json += "\"free_flash\": " + String(flashFree / 1024) + ",";
  json += "\"uptime\": " + String(uptimeSeconds) + ",";
  json += "\"wifi_strength\": " + String(wifiStrength) + ",";
  json += "\"time\": \"" + String(timeStringBuff) + "\"";
  json += "}";
  return json;
}

void handleRoot() {
  server.send(200, "application/json", getSystemInfoJson());
}

void drawUsageBar(int x, int y, int w, int h, int percentage) {
  // Draw background
  tft.fillRect(x, y, w, h, GRAPH_BG);
  tft.drawRect(x, y, w, h, BORDER_COLOR);
  
  // Draw filled portion
  int fillWidth = (w - 2) * percentage / 100;
  if (fillWidth > 0) {
    tft.fillRect(x + 1, y + 1, fillWidth, h - 2, GRAPH_FILL);
  }
}

void drawWiFiIcon(int x, int y, int rssi) {
  // WiFi signal strength: -30 = excellent, -67 = good, -70 = fair, -80 = weak
  int bars = 0;
  if (rssi > -50) bars = 4;
  else if (rssi > -60) bars = 3;
  else if (rssi > -70) bars = 2;
  else if (rssi > -80) bars = 1;
  
  // Draw WiFi symbol (curved arcs)
  for (int i = 0; i < 4; i++) {
    uint16_t color = (i < bars) ? TEXT_COLOR : 0x4208;
    int radius = (i + 1) * 6;
    
    // Draw arc segments
    for (int angle = 210; angle <= 330; angle += 3) {
      float rad = angle * PI / 180.0;
      int x1 = x + radius * cos(rad);
      int y1 = y + radius * sin(rad);
      tft.drawPixel(x1, y1, color);
    }
  }
  
  // Draw center dot
  tft.fillCircle(x, y, 2, bars > 0 ? TEXT_COLOR : 0x4208);
}

void drawMemoryBox(int x, int y, int w, int h, bool expanded) {
  tft.fillRect(x, y, w, h, BOX_BG);
  tft.drawRect(x, y, w, h, BORDER_COLOR);
  
  uint32_t heapUsed = ESP.getHeapSize() - ESP.getFreeHeap();
  uint32_t heapTotal = ESP.getHeapSize();
  uint32_t flashSize = ESP.getFlashChipSize();
  uint32_t flashUsed = ESP.getSketchSize();
  
  int heapPct = (heapUsed * 100) / heapTotal;
  int flashPct = (flashUsed * 100) / flashSize;
  
  tft.setTextColor(TEXT_COLOR);
  
  if (expanded) {
    // Expanded view
    tft.setTextSize(2);
    tft.setCursor(x + 10, y + 10);
    tft.print("MEMORY");
    
    tft.setTextSize(1);
    // RAM
    tft.setCursor(x + 10, y + 40);
    tft.print("RAM:");
    tft.setCursor(x + 10, y + 55);
    tft.printf("%d / %d KB (%d%%)", heapUsed/1024, heapTotal/1024, heapPct);
    drawUsageBar(x + 10, y + 70, w - 20, 20, heapPct);
    
    // Flash
    tft.setCursor(x + 10, y + 110);
    tft.print("Flash:");
    tft.setCursor(x + 10, y + 125);
    tft.printf("%d / %d KB (%d%%)", flashUsed/1024, flashSize/1024, flashPct);
    drawUsageBar(x + 10, y + 140, w - 20, 20, flashPct);
  } else {
    // Compact view
    tft.setTextSize(1);
    tft.setCursor(x + 5, y + 5);
    tft.print("MEMORY");
    
    // RAM
    tft.setCursor(x + 5, y + 20);
    tft.print("RAM:");
    tft.setCursor(x + 5, y + 32);
    tft.printf("%d/%dKB", heapUsed/1024, heapTotal/1024);
    tft.setCursor(x + 5, y + 44);
    tft.printf("(%d%%)", heapPct);
    drawUsageBar(x + 5, y + 56, w - 10, 8, heapPct);
    
    // Flash
    tft.setCursor(x + 5, y + 72);
    tft.print("Flash:");
    tft.setCursor(x + 5, y + 84);
    tft.printf("%d/%dKB", flashUsed/1024, flashSize/1024);
    tft.setCursor(x + 5, y + 96);
    tft.printf("(%d%%)", flashPct);
    drawUsageBar(x + 5, y + 108, w - 10, 8, flashPct);
  }
}

void drawWiFiBox(int x, int y, int w, int h, bool expanded) {
  tft.fillRect(x, y, w, h, BOX_BG);
  tft.drawRect(x, y, w, h, BORDER_COLOR);
  
  bool connected = (WiFi.status() == WL_CONNECTED);
  int rssi = connected ? WiFi.RSSI() : -100;
  
  tft.setTextColor(TEXT_COLOR);
  
  if (expanded) {
    // Expanded view
    tft.setTextSize(2);
    tft.setCursor(x + 10, y + 10);
    tft.print("WiFi");
    
    // WiFi Icon (larger)
    drawWiFiIcon(x + w/2, y + 60, rssi);
    
    tft.setTextSize(1);
    tft.setCursor(x + 10, y + 100);
    tft.print("Status:");
    tft.setCursor(x + 10, y + 115);
    tft.print(connected ? "Connected" : "Disconnected");
    
    if (connected) {
      tft.setCursor(x + 10, y + 135);
      tft.print("SSID:");
      tft.setCursor(x + 10, y + 150);
      tft.print(WiFi.SSID());
      
      tft.setCursor(x + 10, y + 170);
      tft.printf("Signal: %d dBm", rssi);
      
      tft.setCursor(x + 10, y + 190);
      tft.print("IP:");
      tft.setCursor(x + 10, y + 205);
      tft.print(WiFi.localIP());
    }
  } else {
    // Compact view
    tft.setTextSize(1);
    tft.setCursor(x + 5, y + 5);
    tft.print("WiFi");
    
    // WiFi Icon
    drawWiFiIcon(x + w/2, y + 35, rssi);
    
    tft.setCursor(x + 5, y + 65);
    tft.print(connected ? "Connected" : "Disconnected");
    
    if (connected) {
      tft.setCursor(x + 5, y + 80);
      String ssid = WiFi.SSID();
      if (ssid.length() > 16) ssid = ssid.substring(0, 13) + "...";
      tft.print(ssid);
      
      tft.setCursor(x + 5, y + 95);
      tft.printf("%d dBm", rssi);
    }
  }
}

void drawTimeBox(int x, int y, int w, int h, bool expanded) {
  tft.fillRect(x, y, w, h, BOX_BG);
  tft.drawRect(x, y, w, h, BORDER_COLOR);
  
  struct tm timeinfo;
  bool ntpOk = getLocalTime(&timeinfo);
  
  tft.setTextColor(TEXT_COLOR);
  
  if (expanded) {
    // Expanded view
    tft.setTextSize(2);
    tft.setCursor(x + 10, y + 10);
    tft.print("TIME");
    
    tft.setTextSize(1);
    tft.setCursor(x + 10, y + 40);
    tft.print("NTP: ");
    tft.print(ntpOk ? "Synced" : "Not synced");
    
    if (ntpOk) {
      // Date
      tft.setCursor(x + 10, y + 70);
      char dateBuff[20];
      strftime(dateBuff, sizeof(dateBuff), "%Y-%m-%d", &timeinfo);
      tft.setTextSize(2);
      tft.print(dateBuff);
      
      // Time (large)
      tft.setCursor(x + 10, y + 120);
      tft.setTextSize(3);
      char timeBuff[10];
      strftime(timeBuff, sizeof(timeBuff), "%H:%M:%S", &timeinfo);
      tft.print(timeBuff);
    }
  } else {
    // Compact view
    tft.setTextSize(1);
    tft.setCursor(x + 5, y + 5);
    tft.print("TIME");
    
    tft.setCursor(x + 5, y + 25);
    tft.print("NTP:");
    tft.setCursor(x + 5, y + 40);
    tft.print(ntpOk ? "Synced" : "Not synced");
    
    if (ntpOk) {
      tft.setCursor(x + 5, y + 60);
      char dateBuff[20];
      strftime(dateBuff, sizeof(dateBuff), "%Y-%m-%d", &timeinfo);
      tft.print(dateBuff);
      
      tft.setCursor(x + 5, y + 75);
      tft.setTextSize(2);
      char timeBuff[10];
      strftime(timeBuff, sizeof(timeBuff), "%H:%M:%S", &timeinfo);
      tft.print(timeBuff);
    }
  }
}

void drawSystemBox(int x, int y, int w, int h, bool expanded) {
  tft.fillRect(x, y, w, h, BOX_BG);
  tft.drawRect(x, y, w, h, BORDER_COLOR);
  
  float temp = temperatureRead();
  unsigned long uptimeSeconds = millis() / 1000;
  
  tft.setTextColor(TEXT_COLOR);
  
  if (expanded) {
    // Expanded view
    tft.setTextSize(2);
    tft.setCursor(x + 10, y + 10);
    tft.print("SYSTEM");
    
    tft.setTextSize(1);
    tft.setCursor(x + 10, y + 50);
    tft.print("Temperature:");
    
    tft.setCursor(x + 10, y + 80);
    tft.setTextSize(4);
    tft.printf("%.1f C", temp);
    
    tft.setTextSize(1);
    tft.setCursor(x + 10, y + 140);
    tft.print("Uptime:");
    
    tft.setCursor(x + 10, y + 160);
    tft.setTextSize(2);
    unsigned long days = uptimeSeconds / 86400;
    unsigned long hours = (uptimeSeconds % 86400) / 3600;
    unsigned long minutes = (uptimeSeconds % 3600) / 60;
    unsigned long seconds = uptimeSeconds % 60;
    
    if (days > 0) {
      tft.printf("%lud %luh", days, hours);
    } else if (hours > 0) {
      tft.printf("%luh %lum", hours, minutes);
    } else {
      tft.printf("%lum %lus", minutes, seconds);
    }
  } else {
    // Compact view
    tft.setTextSize(1);
    tft.setCursor(x + 5, y + 5);
    tft.print("SYSTEM");
    
    tft.setCursor(x + 5, y + 25);
    tft.print("Temp:");
    tft.setCursor(x + 5, y + 40);
    tft.setTextSize(2);
    tft.printf("%.1f C", temp);
    
    tft.setTextSize(1);
    tft.setCursor(x + 5, y + 70);
    tft.print("Uptime:");
    tft.setCursor(x + 5, y + 85);
    
    unsigned long days = uptimeSeconds / 86400;
    unsigned long hours = (uptimeSeconds % 86400) / 3600;
    unsigned long minutes = (uptimeSeconds % 3600) / 60;
    
    if (days > 0) {
      tft.printf("%lud %luh", days, hours);
    } else if (hours > 0) {
      tft.printf("%luh %lum", hours, minutes);
    } else {
      tft.printf("%lum", minutes);
    }
  }
}

void drawDashboard() {
  // Use double buffering for smooth updates
  
  if (expandedBox == -1) {
    // Normal 4-box view - use half-screen buffering
    
    // Create canvas for top half (320x120) if needed
    if (!canvas || canvas->width() != 320 || canvas->height() != 120) {
      if (canvas) delete canvas;
      canvas = new GFXcanvas16(320, 120);
      if (!canvas) {
        Serial.println("Failed to allocate canvas!");
        // Fallback to direct drawing
        drawMemoryBox(0, 0, 160, 120, false);
        drawWiFiBox(160, 0, 160, 120, false);
        drawTimeBox(0, 120, 160, 120, false);
        drawSystemBox(160, 120, 160, 120, false);
        return;
      }
    }
    
    // Draw top half to canvas
    canvas->fillScreen(BG_COLOR);
    
    // Draw Memory box (0,0)
    uint32_t heapUsed = ESP.getHeapSize() - ESP.getFreeHeap();
    uint32_t heapTotal = ESP.getHeapSize();
    uint32_t flashSize = ESP.getFlashChipSize();
    uint32_t flashUsed = ESP.getSketchSize();
    int heapPct = (heapUsed * 100) / heapTotal;
    int flashPct = (flashUsed * 100) / flashSize;
    
    canvas->fillRect(0, 0, 160, 120, BOX_BG);
    canvas->drawRect(0, 0, 160, 120, BORDER_COLOR);
    canvas->setTextColor(TEXT_COLOR);
    canvas->setTextSize(1);
    canvas->setCursor(5, 5);
    canvas->print("MEMORY");
    canvas->setCursor(5, 20);
    canvas->print("RAM:");
    canvas->setCursor(5, 32);
    canvas->printf("%d/%dKB", heapUsed/1024, heapTotal/1024);
    canvas->setCursor(5, 44);
    canvas->printf("(%d%%)", heapPct);
    // Draw usage bar for RAM
    canvas->fillRect(5, 56, 150, 8, GRAPH_BG);
    canvas->drawRect(5, 56, 150, 8, BORDER_COLOR);
    int fillW = (148 * heapPct) / 100;
    if (fillW > 0) canvas->fillRect(6, 57, fillW, 6, GRAPH_FILL);
    
    canvas->setCursor(5, 72);
    canvas->print("Flash:");
    canvas->setCursor(5, 84);
    canvas->printf("%d/%dKB", flashUsed/1024, flashSize/1024);
    canvas->setCursor(5, 96);
    canvas->printf("(%d%%)", flashPct);
    // Draw usage bar for Flash
    canvas->fillRect(5, 108, 150, 8, GRAPH_BG);
    canvas->drawRect(5, 108, 150, 8, BORDER_COLOR);
    fillW = (148 * flashPct) / 100;
    if (fillW > 0) canvas->fillRect(6, 109, fillW, 6, GRAPH_FILL);
    
    // Draw WiFi box (160,0)
    bool connected = (WiFi.status() == WL_CONNECTED);
    int rssi = connected ? WiFi.RSSI() : -100;
    
    canvas->fillRect(160, 0, 160, 120, BOX_BG);
    canvas->drawRect(160, 0, 160, 120, BORDER_COLOR);
    canvas->setCursor(165, 5);
    canvas->print("WiFi");
    
    // WiFi Icon
    int iconX = 160 + 80;
    int iconY = 35;
    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -60) bars = 3;
    else if (rssi > -70) bars = 2;
    else if (rssi > -80) bars = 1;
    
    for (int i = 0; i < 4; i++) {
      uint16_t color = (i < bars) ? TEXT_COLOR : 0x4208;
      int radius = (i + 1) * 6;
      for (int angle = 210; angle <= 330; angle += 3) {
        float rad = angle * PI / 180.0;
        int x1 = iconX + radius * cos(rad);
        int y1 = iconY + radius * sin(rad);
        canvas->drawPixel(x1, y1, color);
      }
    }
    canvas->fillCircle(iconX, iconY, 2, bars > 0 ? TEXT_COLOR : 0x4208);
    
    canvas->setCursor(165, 65);
    canvas->print(connected ? "Connected" : "Disconnected");
    
    if (connected) {
      canvas->setCursor(165, 80);
      String ssid = WiFi.SSID();
      if (ssid.length() > 16) ssid = ssid.substring(0, 13) + "...";
      canvas->print(ssid);
      canvas->setCursor(165, 95);
      canvas->printf("%d dBm", rssi);
    }
    
    // Flush top half to screen
    flushCanvas(0, 0, 320, 120);
    
    // Draw bottom half to canvas
    canvas->fillScreen(BG_COLOR);
    
    // Draw Time box (0,0 in canvas, maps to 0,120 on screen)
    struct tm timeinfo;
    bool ntpOk = getLocalTime(&timeinfo);
    
    canvas->fillRect(0, 0, 160, 120, BOX_BG);
    canvas->drawRect(0, 0, 160, 120, BORDER_COLOR);
    canvas->setCursor(5, 5);
    canvas->print("TIME");
    canvas->setCursor(5, 25);
    canvas->print("NTP:");
    canvas->setCursor(5, 40);
    canvas->print(ntpOk ? "Synced" : "Not synced");
    
    if (ntpOk) {
      canvas->setCursor(5, 60);
      char dateBuff[20];
      strftime(dateBuff, sizeof(dateBuff), "%Y-%m-%d", &timeinfo);
      canvas->print(dateBuff);
      canvas->setCursor(5, 75);
      canvas->setTextSize(2);
      char timeBuff[10];
      strftime(timeBuff, sizeof(timeBuff), "%H:%M:%S", &timeinfo);
      canvas->print(timeBuff);
      canvas->setTextSize(1);
    }
    
    // Draw System box (160,0 in canvas, maps to 160,120 on screen)
    float temp = temperatureRead();
    unsigned long uptimeSeconds = millis() / 1000;
    
    canvas->fillRect(160, 0, 160, 120, BOX_BG);
    canvas->drawRect(160, 0, 160, 120, BORDER_COLOR);
    canvas->setCursor(165, 5);
    canvas->print("SYSTEM");
    canvas->setCursor(165, 25);
    canvas->print("Temp:");
    canvas->setCursor(165, 40);
    canvas->setTextSize(2);
    canvas->printf("%.1f C", temp);
    canvas->setTextSize(1);
    canvas->setCursor(165, 70);
    canvas->print("Uptime:");
    canvas->setCursor(165, 85);
    
    unsigned long days = uptimeSeconds / 86400;
    unsigned long hours = (uptimeSeconds % 86400) / 3600;
    unsigned long minutes = (uptimeSeconds % 3600) / 60;
    
    if (days > 0) {
      canvas->printf("%lud %luh", days, hours);
    } else if (hours > 0) {
      canvas->printf("%luh %lum", hours, minutes);
    } else {
      canvas->printf("%lum", minutes);
    }
    
    // Flush bottom half to screen
    flushCanvas(0, 120, 320, 120);
    
  } else {
    // Expanded view - draw directly (single box, less flicker)
    tft.fillScreen(BG_COLOR);
    switch(expandedBox) {
      case 0: drawMemoryBox(0, 0, 320, 240, true); break;
      case 1: drawWiFiBox(0, 0, 320, 240, true); break;
      case 2: drawTimeBox(0, 0, 320, 240, true); break;
      case 3: drawSystemBox(0, 0, 320, 240, true); break;
    }
  }
}

void handleTouch() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    
    if (p.z < 10) return;
    
    // Map coordinates
    int16_t x = map(p.y, TS_MINY, TS_MAXY, 0, 320);
    int16_t y = map(p.x, TS_MAXX, TS_MINX, 0, 240);
    
    if (expandedBox == -1) {
      // Determine which box was touched
      int8_t touchedBox = -1;
      if (x < 160 && y < 120) touchedBox = 0;
      else if (x >= 160 && y < 120) touchedBox = 1;
      else if (x < 160 && y >= 120) touchedBox = 2;
      else if (x >= 160 && y >= 120) touchedBox = 3;
      
      if (touchedBox >= 0) {
        expandedBox = touchedBox;
        tft.fillScreen(BG_COLOR); // Clear only when expanding
        drawDashboard();
        Serial.printf("Expanded box %d\n", expandedBox);
      }
    } else {
      // Collapse back to normal view
      expandedBox = -1;
      tft.fillScreen(BG_COLOR); // Clear only when collapsing
      drawDashboard();
      Serial.println("Collapsed to normal view");
    }
    
    delay(100);
    while(ts.touched()) delay(10);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting 4-Box Dashboard...");

  // Init SPI & Display
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI);
  tft.begin(40000000);
  tft.setRotation(1);
  
  // Init Touch
  if (!ts.begin()) {
    Serial.println("Touchscreen failed!");
  }

  // Init WiFi
  setupWifi();
  
  // Init Server
  server.on("/", handleRoot);
  server.begin();
  
  // Draw initial UI (canvas will handle clearing)
  drawDashboard();
}

void loop() {
  server.handleClient();
  checkWifi();
  handleTouch();
  
  // Update display every 1 second (in both normal and expanded views)
  if (millis() - lastUpdate > 1000) {
    drawDashboard();
    lastUpdate = millis();
  }
  
  delay(10);
}
