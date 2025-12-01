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
