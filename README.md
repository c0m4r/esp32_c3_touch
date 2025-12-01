# Wiring Summary

| ---- | ---- | ---- |
| Display	| ESP32-C3	| Note |
| VCC	| 3V3	| |
| GND	| GND	| |
| SCK	| GPIO 4	| Shared with T_CLK |
| MOSI	| GPIO 6	| Shared with T_DIN |
| MISO	| GPIO 5	| Shared with T_DO |
| CS	| GPIO 7	| |
| DC	| GPIO 3	| |
| RST	| GPIO 10	| |
| T_CS	| GPIO 1	| |
| T_IRQ	| GPIO 0	| |
| T_CLK	| GPIO 4	| Connect to SCK |
| T_DIN	| GPIO 6	| Connect to MOSI |
| T_DO	| GPIO 5	| Connect to MISO |
