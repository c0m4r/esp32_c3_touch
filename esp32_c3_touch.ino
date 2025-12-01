#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// ----------------------------
// Pin Definitions for ESP32-C3 Super Mini
// ----------------------------
#define TFT_CS        7
#define TFT_RST       10
#define TFT_DC        3
#define TFT_MOSI      6
#define TFT_SCK       4
#define TFT_MISO      5
#define TFT_LED       21 // Optional: Connect to 3.3V or a PWM pin. Using 21 (TX) as placeholder or unused if hardwired.

#define TOUCH_CS      1
#define TOUCH_IRQ     0

// ----------------------------
// Objects
// ----------------------------
// Use hardware SPI (VSPI/HSPI default pins might need remapping if not using default)
// For ESP32-C3, we can map SPI pins.
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// ----------------------------
// Calibration & Settings
// ----------------------------
// Touch screen calibration values (may need adjustment for your specific module)
#define TS_MINX 200
#define TS_MINY 200
#define TS_MAXX 3800
#define TS_MAXY 3800

// ----------------------------
// State Variables
// ----------------------------
uint16_t currentBgColor = ILI9341_BLACK;
uint16_t paintColor = ILI9341_BLUE;

void drawUI() {
  // Top Bar Background
  tft.fillRect(0, 0, 320, 40, ILI9341_DARKGREY);
  
  // Clear Button (Left)
  tft.drawRect(0, 0, 160, 40, ILI9341_WHITE);
  tft.setCursor(50, 12);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Clear");

  // BG Toggle Button (Right)
  tft.drawRect(160, 0, 160, 40, ILI9341_WHITE);
  tft.setCursor(200, 12);
  tft.println(currentBgColor == ILI9341_BLACK ? "BG: Wht" : "BG: Blk");

  // Drawing Area Border
  tft.drawRect(0, 40, 320, 200, ILI9341_WHITE);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-C3 Touch Paint Test");

  // Initialize SPI
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI);

  // Initialize Display
  // Increase SPI frequency for faster display updates (40MHz)
  tft.begin(40000000); 
  tft.setRotation(1); // Landscape
  tft.fillScreen(currentBgColor);
  
  drawUI();

  // Initialize Touch
  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
  } else {
    Serial.println("Touchscreen started");
  }
  // ts.setRotation(1); // REMOVED: We will map raw coordinates manually
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    
    // Pressure check to reduce noise
    if (p.z < 200) return;

    // Debug Raw Values
    Serial.print("Raw X: "); Serial.print(p.x);
    Serial.print(" Raw Y: "); Serial.print(p.y);
    Serial.print(" Z: "); Serial.println(p.z);

    // Map touch coordinates to screen coordinates
    // Landscape Mapping: Swap X and Y
    // Note: Depending on your specific panel, you might need to invert MIN/MAX for one or both.
    // Here we assume:
    // Screen X (0-320) <-> Raw Y (200-3800)
    // Screen Y (0-240) <-> Raw X (3800-200) [Inverted]
    int16_t x = map(p.y, TS_MINY, TS_MAXY, 0, 320);
    int16_t y = map(p.x, TS_MAXX, TS_MINX, 0, 240);

    Serial.print("Mapped X: "); Serial.print(x);
    Serial.print(" Mapped Y: "); Serial.println(y);

    // Check if touch is within Top Bar (0-40px)
    if (y < 40) {
      // Debounce slightly for buttons
      delay(150); 
      
      if (x < 160) {
        // Left Button: CLEAR
        Serial.println("Action: Clear");
        tft.fillRect(0, 40, 320, 200, currentBgColor);
        tft.drawRect(0, 40, 320, 200, ILI9341_WHITE);
      } else {
        // Right Button: TOGGLE BG
        Serial.println("Action: Toggle BG");
        currentBgColor = (currentBgColor == ILI9341_BLACK) ? ILI9341_WHITE : ILI9341_BLACK;
        tft.fillScreen(currentBgColor);
        drawUI();
      }
      
      // Wait for release to prevent repeated triggering
      while (ts.touched()) {
        delay(10);
      }
    } else {
      // Drawing Area
      // Draw Blue Circle
      tft.fillCircle(x, y, 2, paintColor);
    }
  }
}
