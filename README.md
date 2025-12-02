# Wiring Summary

<img width="4096" height="2310" alt="image" src="https://github.com/user-attachments/assets/29775ba1-9322-4025-8b6b-a36ed4d06c88" />

| | | | | | | |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- |
| <img width="2310" height="4096" alt="image" src="https://github.com/user-attachments/assets/42f62da8-d320-411d-90e7-d386e64dc026" /> | <img width="2310" height="4096" alt="image" src="https://github.com/user-attachments/assets/9fdd19b5-c36c-491c-ab8d-4aa956a1b1f6" /> |<img width="4096" height="2310" alt="image" src="https://github.com/user-attachments/assets/ad8c8c45-9a2a-4492-97a5-020221220a16" /> |<img width="2310" height="4096" alt="image" src="https://github.com/user-attachments/assets/aa82d49e-520b-4906-bb81-4a1a4a1624e4" /> | <img width="2310" height="4096" alt="image" src="https://github.com/user-attachments/assets/802d0b75-c224-4bf0-98c5-f54e76014a48" /> |<img width="2310" height="4096" alt="image" src="https://github.com/user-attachments/assets/dc61adba-52ab-484a-8965-bde0e620b000" /> |


| Display	| ESP32-C3	| Note | Wire color |
| ---- | ---- | ---- | ---- |
| VCC	| 3V3	| | Red |
| GND	| GND	| | Black |
| SCK	| GPIO 4	| Shared with T_CLK | brown |
| MOSI	| GPIO 6	| Shared with T_DIN | orange |
| MISO	| GPIO 5	| Shared with T_DO | yellow |
| CS	| GPIO 7	| | green |
| DC	| GPIO 3	| | blue |
| RST	| GPIO 10	| | purple |
| T_CS	| GPIO 1	| | white |
| T_IRQ	| GPIO 0	| | gray |
| T_CLK	| GPIO 4	| Connect to SCK | brown |
| T_DIN	| GPIO 6	| Connect to MOSI | orange |
| T_DO	| GPIO 5	| Connect to MISO | yellow |

## Function Descriptions

### `esp32_c3_touch.ino`

This is the main Arduino sketch for the ESP32-C3 Touch project. It handles WiFi connectivity, the web server, and the touchscreen dashboard interface.

- **`setupWifi()`**: Initializes the WiFi connection using credentials from `secrets.h`.
- **`checkWifi()`**: Checks the WiFi connection status and attempts to reconnect if disconnected.
- **`getSystemInfoJson()`**: Collects system information (uptime, heap memory, WiFi signal strength) and returns it as a JSON string.
- **`handleRoot()`**: Handles HTTP requests to the root URL ("/") and serves a simple "Hello from ESP32-C3!" message.
- **`drawUsageBar(Adafruit_GFX* gfx, int x, int y, int w, int h, int percentage)`**: Draws a graphical usage bar at the specified coordinates with the given percentage fill.
- **`drawWiFiIcon(Adafruit_GFX* gfx, int x, int y, int rssi)`**: Draws a WiFi signal strength icon based on the RSSI value.
- **`drawMemoryBox(int x, int y, int w, int h, bool expanded)`**: Renders the memory usage section of the dashboard.
- **`drawWiFiBox(int x, int y, int w, int h, bool expanded)`**: Renders the WiFi status section of the dashboard.
- **`drawTimeBox(int x, int y, int w, int h, bool expanded)`**: Renders the time display section of the dashboard.
- **`drawSystemBox(int x, int y, int w, int h, bool expanded)`**: Renders the system information section of the dashboard.
- **`drawDashboard()`**: Orchestrates the drawing of the entire dashboard, including all the boxes (Memory, WiFi, Time, System).
- **`handleTouch()`**: Detects and processes touch input from the screen, allowing interaction with the dashboard elements (e.g., expanding/collapsing boxes).
- **`drawSplashScreen()`**: Displays the splash screen image stored in `image_data.h` on startup.
- **`setup()`**: The standard Arduino setup function. It initializes the serial communication, display, touchscreen, WiFi, and time sync.
- **`loop()`**: The standard Arduino loop function. It handles the main program logic, including WiFi checks, web server handling, touch input processing, and dashboard updates.

### `convert_image.py`

This Python script is a utility to convert an image file into a C header file usable by the Arduino sketch.

- **`convert_image(input_path, output_path)`**: Reads an image file (e.g., PNG), resizes it to 320x240, converts it to RGB format, and then writes the pixel data as a C array of RGB565 values to a header file (`image_data.h`). This allows the image to be stored in the ESP32's program memory (PROGMEM) and displayed on the screen.
