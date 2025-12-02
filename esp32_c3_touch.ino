#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include "secrets.h"
#include "image_data.h"

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

// Double buffering - one canvas at a time
GFXcanvas16 *canvas = nullptr;

// Helper to draw canvas to screen at position
void flushCanvas(int x, int y, int w, int h) {
  if (canvas && canvas->getBuffer()) {
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
  server.send(200, "application/json", getSystemInfoJson() + "\n");
}

void drawUsageBar(Adafruit_GFX* gfx, int x, int y, int w, int h, int percentage) {
  if (!gfx) return;
  gfx->fillRect(x, y, w, h, GRAPH_BG);
  gfx->drawRect(x, y, w, h, BORDER_COLOR);
  int fillWidth = (w - 2) * percentage / 100;
  if (fillWidth > 0) {
    gfx->fillRect(x + 1, y + 1, fillWidth, h - 2, GRAPH_FILL);
  }
}

void drawWiFiIcon(Adafruit_GFX* gfx, int x, int y, int rssi) {
  if (!gfx) return;
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
      int x1 = x + radius * cos(rad);
      int y1 = y + radius * sin(rad);
      gfx->drawPixel(x1, y1, color);
    }
  }
  gfx->fillCircle(x, y, 2, bars > 0 ? TEXT_COLOR : 0x4208);
}

void drawMemoryBox(int x, int y, int w, int h, bool expanded) {
  // Placeholder for fallback
}

void drawWiFiBox(int x, int y, int w, int h, bool expanded) {
  // Placeholder for fallback
}

void drawTimeBox(int x, int y, int w, int h, bool expanded) {
  // Placeholder for fallback
}

void drawSystemBox(int x, int y, int w, int h, bool expanded) {
  // Placeholder for fallback
}

void drawDashboard() {
  // Always use a fixed 320x120 buffer (75KB) to prevent OOM
  int requiredHeight = 120;
  
  // Recreate canvas only if it doesn't exist or has wrong size
  if (!canvas || canvas->height() != requiredHeight) {
    if (canvas) {
      delete canvas;
      canvas = nullptr;
    }
    
    // Wait for memory to stabilize
    delay(20);
    
    // Allocate new canvas
    Serial.printf("Allocating canvas: 320x%d... ", requiredHeight);
    canvas = new GFXcanvas16(320, requiredHeight);
    
    if (!canvas || !canvas->getBuffer()) {
      Serial.println("FAILED! Using direct draw fallback");
      if (canvas) { delete canvas; canvas = nullptr; }
    } else {
      Serial.printf("Success! (%d KB free)\n", ESP.getFreeHeap() / 1024);
    }
  }
  
  // Determine if we can use the buffer
  bool useBuffer = (canvas != nullptr);
  Adafruit_GFX* target = useBuffer ? (Adafruit_GFX*)canvas : (Adafruit_GFX*)&tft;
  
  // If direct drawing, clear screen once at start
  if (!useBuffer) {
    tft.fillScreen(BG_COLOR);
  }

  if (expandedBox == -1) {
    // === NORMAL 4-BOX VIEW ===
    // This fits perfectly in our 2-pass logic naturally
    
    // Pass 1: Top Half (0-120)
    if (useBuffer) canvas->fillScreen(BG_COLOR);
    
    uint32_t heapUsed = ESP.getHeapSize() - ESP.getFreeHeap();
    uint32_t heapTotal = ESP.getHeapSize();
    uint32_t flashSize = ESP.getFlashChipSize();
    uint32_t flashUsed = ESP.getSketchSize();
    int heapPct = (heapUsed * 100) / heapTotal;
    int flashPct = (flashUsed * 100) / flashSize;
    bool connected = (WiFi.status() == WL_CONNECTED);
    int rssi = connected ? WiFi.RSSI() : -100;
    
    // Memory box
    target->fillRect(0, 0, 160, 120, BOX_BG);
    target->drawRect(0, 0, 160, 120, BORDER_COLOR);
    target->setTextColor(TEXT_COLOR);
    target->setTextSize(1);
    target->setCursor(5, 5);
    target->print("MEMORY");
    //target->setCursor(5, 20);
    //target->print("RAM:");
    target->setCursor(5, 32);
    target->printf("RAM: %d/%dKB (%d%%)", heapUsed/1024, heapTotal/1024, heapPct);
    //target->setCursor(5, 44);
    //target->printf("", heapPct);
    drawUsageBar(target, 5, 44, 150, 8, heapPct);
    target->setCursor(5, 72);
    target->print("Flash:");
    target->setCursor(5, 84);
    target->printf("%d/%dKB (%d%%)", flashUsed/1024, flashSize/1024, flashPct);
    //target->setCursor(5, 96);
    //target->printf("", );
    drawUsageBar(target, 5, 96, 150, 8, flashPct);
    
    // WiFi box
    target->fillRect(160, 0, 160, 120, BOX_BG);
    target->drawRect(160, 0, 160, 120, BORDER_COLOR);
    target->setCursor(165, 5);
    target->print("WiFi");
    drawWiFiIcon(target, 240, 35, rssi);
    target->setCursor(165, 65);
    target->print(connected ? "Connected" : "Disconnected");
    if (connected) {
      target->setCursor(165, 80);
      String ssid = WiFi.SSID();
      if (ssid.length() > 16) ssid = ssid.substring(0, 13) + "...";
      target->print(ssid);
      target->setCursor(165, 95);
      target->printf("%d dBm", rssi);
    }
    
    if (useBuffer) flushCanvas(0, 0, 320, 120);
    
    // Pass 2: Bottom Half (120-240)
    if (useBuffer) canvas->fillScreen(BG_COLOR);
    
    // For normal view, we draw to (0,0) in buffer, but it represents (0,120) on screen
    // If direct draw, we offset by 120
    int yOffset = useBuffer ? 0 : 120;
    
    struct tm timeinfo;
    bool ntpOk = getLocalTime(&timeinfo);
    float temp = temperatureRead();
    unsigned long uptimeSeconds = millis() / 1000;
    
    // Time box
    target->fillRect(0, yOffset, 160, 120, BOX_BG);
    target->drawRect(0, yOffset, 160, 120, BORDER_COLOR);
    target->setCursor(5, yOffset + 5);
    target->print("TIME");
    target->setCursor(5, yOffset + 25);
    target->print("NTP:");
    target->setCursor(5, yOffset + 40);
    target->print(ntpOk ? "Synced" : "Not synced");
    if (ntpOk) {
      char dateBuff[20];
      strftime(dateBuff, sizeof(dateBuff), "%Y-%m-%d", &timeinfo);
      target->setCursor(5, yOffset + 60);
      target->print(dateBuff);
      target->setCursor(5, yOffset + 75);
      target->setTextSize(2);
      char timeBuff[10];
      strftime(timeBuff, sizeof(timeBuff), "%H:%M:%S", &timeinfo);
      target->print(timeBuff);
      target->setTextSize(1);
    }
    
    // System box
    target->fillRect(160, yOffset, 160, 120, BOX_BG);
    target->drawRect(160, yOffset, 160, 120, BORDER_COLOR);
    target->setCursor(165, yOffset + 5);
    target->print("SYSTEM");
    target->setCursor(165, yOffset + 25);
    target->print("Temp:");
    target->setCursor(165, yOffset + 40);
    target->setTextSize(2);
    target->printf("%.1f C", temp);
    target->setTextSize(1);
    target->setCursor(165, yOffset + 70);
    target->print("Uptime:");
    target->setCursor(165, yOffset + 85);
    unsigned long hours = (uptimeSeconds % 86400) / 3600;
    unsigned long minutes = (uptimeSeconds % 3600) / 60;
    if (hours > 0) target->printf("%luh %lum", hours, minutes);
    else target->printf("%lum", minutes);
    
    if (useBuffer) flushCanvas(0, 120, 320, 120);
    
  } else {
    // === EXPANDED VIEW (Split Buffering) ===
    // We render the full screen in TWO passes of 320x120 each
    
    for (int pass = 0; pass < 2; pass++) {
      int virtualY = pass * 120; // 0 for top half, 120 for bottom half
      
      if (useBuffer) {
        canvas->fillScreen(BG_COLOR);
      } else {
        // Direct draw doesn't need passes in the same way, but we'll stick to the loop structure
        // to share the drawing code. We just don't flush.
      }
      
      // Helper to adjust Y coordinates
      // If direct draw: we draw at actual Y
      // If buffered: we draw at (Y - virtualY)
      // BUT: The drawing logic below assumes we are drawing the WHOLE screen.
      // So we need to shift everything up by virtualY when drawing to buffer.
      
      int drawOffsetY = useBuffer ? -virtualY : 0;
      
      // We clip output to the buffer size automatically by GFX
      
      switch(expandedBox) {
        case 0: { // Memory
          uint32_t heapUsed = ESP.getHeapSize() - ESP.getFreeHeap();
          uint32_t heapTotal = ESP.getHeapSize();
          uint32_t flashSize = ESP.getFlashChipSize();
          uint32_t flashUsed = ESP.getSketchSize();
          int heapPct = (heapUsed * 100) / heapTotal;
          int flashPct = (flashUsed * 100) / flashSize;
          
          target->fillRect(0, 0 + drawOffsetY, 320, 240, BOX_BG);
          target->drawRect(0, 0 + drawOffsetY, 320, 240, BORDER_COLOR);
          target->setTextColor(TEXT_COLOR);
          target->setTextSize(2);
          target->setCursor(10, 10 + drawOffsetY);
          target->print("MEMORY");
          target->setTextSize(1);
          target->setCursor(10, 40 + drawOffsetY);
          target->print("RAM:");
          target->setCursor(10, 55 + drawOffsetY);
          target->printf("%d / %d KB (%d%%)", heapUsed/1024, heapTotal/1024, heapPct);
          drawUsageBar(target, 10, 70 + drawOffsetY, 300, 20, heapPct);
          target->setCursor(10, 110 + drawOffsetY);
          target->print("Flash:");
          target->setCursor(10, 125 + drawOffsetY);
          target->printf("%d / %d KB (%d%%)", flashUsed/1024, flashSize/1024, flashPct);
          drawUsageBar(target, 10, 140 + drawOffsetY, 300, 20, flashPct);
          break;
        }
        
        case 1: { // WiFi
          bool connected = (WiFi.status() == WL_CONNECTED);
          int rssi = connected ? WiFi.RSSI() : -100;
          
          target->fillRect(0, 0 + drawOffsetY, 320, 240, BOX_BG);
          target->drawRect(0, 0 + drawOffsetY, 320, 240, BORDER_COLOR);
          target->setTextColor(TEXT_COLOR);
          target->setTextSize(2);
          target->setCursor(10, 10 + drawOffsetY);
          target->print("WiFi");
          drawWiFiIcon(target, 160, 60 + drawOffsetY, rssi);
          target->setTextSize(1);
          target->setCursor(10, 100 + drawOffsetY);
          target->print("Status:");
          target->setCursor(10, 115 + drawOffsetY);
          target->print(connected ? "Connected" : "Disconnected");
          if (connected) {
            target->setCursor(10, 135 + drawOffsetY);
            target->print("SSID:");
            target->setCursor(10, 150 + drawOffsetY);
            target->print(WiFi.SSID());
            target->setCursor(10, 170 + drawOffsetY);
            target->printf("Signal: %d dBm", rssi);
            target->setCursor(10, 190 + drawOffsetY);
            target->print("IP:");
            target->setCursor(10, 205 + drawOffsetY);
            target->print(WiFi.localIP());
          }
          break;
        }
        
        case 2: { // Time
          struct tm timeinfo;
          bool ntpOk = getLocalTime(&timeinfo);
          
          target->fillRect(0, 0 + drawOffsetY, 320, 240, BOX_BG);
          target->drawRect(0, 0 + drawOffsetY, 320, 240, BORDER_COLOR);
          target->setTextColor(TEXT_COLOR);
          target->setTextSize(2);
          target->setCursor(10, 10 + drawOffsetY);
          target->print("TIME");
          target->setTextSize(1);
          target->setCursor(10, 40 + drawOffsetY);
          target->print("NTP: ");
          target->print(ntpOk ? "Synced" : "Not synced");
          if (ntpOk) {
            char dateBuff[20];
            strftime(dateBuff, sizeof(dateBuff), "%Y-%m-%d", &timeinfo);
            target->setCursor(10, 70 + drawOffsetY);
            target->setTextSize(2);
            target->print(dateBuff);
            target->setCursor(10, 120 + drawOffsetY);
            target->setTextSize(3);
            char timeBuff[10];
            strftime(timeBuff, sizeof(timeBuff), "%H:%M:%S", &timeinfo);
            target->print(timeBuff);
          }
          break;
        }
        
        case 3: { // System
          float temp = temperatureRead();
          unsigned long uptimeSeconds = millis() / 1000;
          
          target->fillRect(0, 0 + drawOffsetY, 320, 240, BOX_BG);
          target->drawRect(0, 0 + drawOffsetY, 320, 240, BORDER_COLOR);
          target->setTextColor(TEXT_COLOR);
          target->setTextSize(2);
          target->setCursor(10, 10 + drawOffsetY);
          target->print("SYSTEM");
          target->setTextSize(1);
          target->setCursor(10, 50 + drawOffsetY);
          target->print("Temperature:");
          target->setCursor(10, 80 + drawOffsetY);
          target->setTextSize(4);
          target->printf("%.1f C", temp);
          target->setTextSize(1);
          target->setCursor(10, 140 + drawOffsetY);
          target->print("Uptime:");
          target->setCursor(10, 160 + drawOffsetY);
          target->setTextSize(2);
          unsigned long hours = (uptimeSeconds % 86400) / 3600;
          unsigned long minutes = (uptimeSeconds % 3600) / 60;
          unsigned long seconds = uptimeSeconds % 60;
          if (hours > 0) target->printf("%luh %lum", hours, minutes);
          else target->printf("%lum %lus", minutes, seconds);
          break;
        }
      }
      
      if (useBuffer) {
        flushCanvas(0, virtualY, 320, 120);
      } else {
        // If direct draw, we only need one pass
        break;
      }
    }
  }
}

void handleTouch() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    
    Serial.printf("Touch: z=%d ", p.z);
    
    if (p.z < 10) {
      Serial.println("(too weak)");
      return;
    }
    
    int16_t x = map(p.y, TS_MINY, TS_MAXY, 0, 320);
    int16_t y = map(p.x, TS_MAXX, TS_MINX, 0, 240);
    
    Serial.printf("x=%d y=%d ", x, y);
    
    if (expandedBox == -1) {
      int8_t touchedBox = -1;
      if (x < 160 && y < 120) touchedBox = 0;
      else if (x >= 160 && y < 120) touchedBox = 1;
      else if (x < 160 && y >= 120) touchedBox = 2;
      else if (x >= 160 && y >= 120) touchedBox = 3;
      
      if (touchedBox >= 0) {
        Serial.printf("-> Expand box %d\n", touchedBox);
        
        // DISABLE DOUBLE BUFFER: Free canvas memory before expanding
        if (canvas) {
          Serial.println("Freeing canvas before expand...");
          delete canvas;
          canvas = nullptr;
        }
        delay(50); // Allow memory to stabilize
        
        // Change state
        expandedBox = touchedBox;
        
        // Redraw from scratch
        drawDashboard();
      } else {
        Serial.println("(invalid)");
      }
    } else {
      Serial.println("-> Collapse");
      
      // DISABLE DOUBLE BUFFER: Free canvas memory before collapsing
      if (canvas) {
        Serial.println("Freeing canvas before collapse...");
        delete canvas;
        canvas = nullptr;
      }
      delay(50); // Allow memory to stabilize
      
      // Clear screen to ensure clean slate
      tft.fillScreen(BG_COLOR);
      
      // Change state
      expandedBox = -1;
      
      // Redraw main screen from scratch
      drawDashboard();
    }
    
    delay(100);
    while(ts.touched()) delay(10);
  }
}

void drawSplashScreen() {
  tft.fillScreen(BG_COLOR);
  
  // Draw Title
  tft.setTextColor(TEXT_COLOR);
  tft.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds("ESP32-C3", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((320 - w) / 2, 80);
  tft.print("ESP32-C3");
  
  tft.setTextSize(1);
  tft.getTextBounds("Starting...", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((320 - w) / 2, 110);
  tft.print("Starting...");
  
  // Draw Progress Bar
  int barW = 200;
  int barH = 12;
  int barX = (320 - barW) / 2;
  int barY = 140;
  
  tft.drawRect(barX, barY, barW, barH, BORDER_COLOR);
  
  // Animate progress
  for(int i=0; i<=100; i+=2) {
    int fillW = (barW - 4) * i / 100;
    if (fillW > 0) {
      tft.fillRect(barX + 2, barY + 2, fillW, barH - 4, HIGHLIGHT);
    }
    delay(15); // Simulate loading time (total ~750ms)
  }
  
  delay(200);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-C3 Dashboard");

  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI);
  tft.begin(40000000);
  tft.setRotation(1);
  
  if (!ts.begin()) {
    Serial.println("Touch failed!");
  }

  // Draw Image Splash
  tft.drawRGBBitmap(0, 0, splash_image_rgb565, 320, 240);
  delay(3000);

  drawSplashScreen();

  setupWifi();
  
  server.on("/", handleRoot);
  server.begin();
  
  drawDashboard();
}

void loop() {
  server.handleClient();
  checkWifi();
  handleTouch();
  
  if (millis() - lastUpdate > 1000) {
    drawDashboard();
    lastUpdate = millis();
  }
  
  delay(10);
}
