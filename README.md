# Wiring Summary

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
