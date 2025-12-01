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
#define TS_MINY 300 // Adjusted to fix offset
#define TS_MAXX 3800
#define TS_MAXY 3800

// ----------------------------
// State Variables
// ----------------------------
uint16_t currentBgColor = ILI9341_BLACK;
uint16_t paintColor = ILI9341_BLUE;

// Palette Colors
uint16_t palette[] = {ILI9341_RED, ILI9341_GREEN, ILI9341_BLUE, ILI9341_YELLOW, ILI9341_CYAN, ILI9341_MAGENTA, ILI9341_BLACK};
int paletteCount = 7;

void drawUI() {
  // Top Bar Background
  tft.fillRect(0, 0, 320, 40, ILI9341_DARKGREY);
  
  // Clear Button (Left, 50px)
  tft.drawRect(0, 0, 50, 40, ILI9341_WHITE);
  tft.setCursor(8, 12);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.println("CLR");

  // Color Palette (Remaining 270px)
  int startX = 50;
  int boxWidth = (320 - 50) / paletteCount;
  
  for (int i = 0; i < paletteCount; i++) {
    uint16_t color = palette[i];
    // If eraser (last item), use current BG color for visual, but logic uses palette color
    if (i == paletteCount - 1) color = currentBgColor; 
    
    tft.fillRect(startX + (i * boxWidth), 0, boxWidth, 40, color);
    tft.drawRect(startX + (i * boxWidth), 0, boxWidth, 40, ILI9341_WHITE);
    
    // Highlight selected color
    if (palette[i] == paintColor) {
       tft.drawRect(startX + (i * boxWidth) + 2, 2, boxWidth - 4, 36, ILI9341_WHITE);
       tft.drawRect(startX + (i * boxWidth) + 3, 3, boxWidth - 6, 34, ILI9341_BLACK);
    }
  }

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
  
  // Update Eraser color in palette to match BG
  palette[paletteCount - 1] = currentBgColor;
  
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
    
    // Pressure check to reduce noise (High Sensitivity)
    if (p.z < 10) return;

    // Map touch coordinates to screen coordinates
    // Landscape Mapping: Swap X and Y
    int16_t x = map(p.y, TS_MINY, TS_MAXY, 0, 320);
    int16_t y = map(p.x, TS_MAXX, TS_MINX, 0, 240);

    // Check if touch is within Top Bar (0-40px)
    if (y < 40) {
      // Debounce slightly for buttons
      delay(150); 
      
      if (x < 50) {
        // Left Button: CLEAR
        Serial.println("Action: Clear");
        tft.fillRect(0, 40, 320, 200, currentBgColor);
        tft.drawRect(0, 40, 320, 200, ILI9341_WHITE);
      } else {
        // Palette Selection
        int startX = 50;
        int boxWidth = (320 - 50) / paletteCount;
        int index = (x - startX) / boxWidth;
        
        if (index >= 0 && index < paletteCount) {
           paintColor = palette[index];
           Serial.print("Action: Color Selected "); Serial.println(index);
           drawUI(); // Redraw to show selection
        }
      }
      
      // Wait for release to prevent repeated triggering
      while (ts.touched()) {
        delay(10);
      }
    } else {
      // Drawing Area
      tft.fillCircle(x, y, 2, paintColor);
    }
  }
}
