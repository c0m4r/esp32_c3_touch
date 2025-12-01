# ESP32-C3 Super Mini Pinout

| Pin     | Function |
|---------|----------------|
| 3V3     | 3.3V output/input (outputs 3.3V from the onboard regulator, or it is a input for external 3.3V power supply) |
| 5V      | 5V input/output (connects to the USB-C 5V or external 5V supply) |
| GND     | GND pin |
| GPIO 0  | GPIO I/O, ADC1, PWM |
| GPIO 1  | GPIO I/O, ADC1, PWM |
| GPIO 2  | GPIO I/O ADC1, Strapping Pin (Boot Mode) (avoid for general use) |
| GPIO 3  | GPIO I/O, PWM |
| GPIO 4  | GPIO I/O, PWM, default SPI SCK pin |
| GPIO 5  | GPIO I/O, PWM, default SPI MISO pin |
| GPIO 6  | GPIO I/O, PWM, default SPI MOSI pin |
| GPIO 7  | GPIO I/O, PWM, default SPI SS pin |
| GPIO 8  | Connected to the onboard LED (active low); Strapping Pin (avoid for GPIO use); Default I2C SDA pin |
| GPIO 9  | Connected to BOOT Button (LOW to enter bootloader), Strapping Pin (avoid for GPIO use); Default I2C SCL pin |
| GPIO 10 | GPIO I/O, PWM |
| GPIO 20 | GPIO I/O, PWM, default UART RX Pin |
| GPIO 21 | GPIO I/O, PWM, default UART TX Pin |

# 2.4" TFT SPI 240x320 Touch Display Pinout

| Pin       | Function | ESP32-C3 Pin |
|-----------|----------------|--------------|
| T_IRQ     | Touch interrupt | GPIO 0 |
| T_DO      | Touch data output | GPIO 5 (Shared MISO) |
| T_DIN     | Touch data input | GPIO 6 (Shared MOSI) |
| T_CS      | Touch chip select | GPIO 1 |
| T_CLK     | Touch clock | GPIO 4 (Shared SCK) |
| SDO(MISO) | SPI master in slave out | GPIO 5 |
| LED       | Backlight control | 3.3V or PWM |
| SCK       | SPI clock | GPIO 4 |
| SDI(MOSI) | SPI master out slave in | GPIO 6 |
| DC        | SPI data command | GPIO 3 |
| RST       | SPI reset | GPIO 10 |
| CS        | SPI chip select | GPIO 7 |
| GND       | GND pin | GND |
| VCC       | 3.3V power supply | 3V3 |

# Display SD Card Pinout

| Pin       | Function |
|-----------|----------------|
| SD_SCK    | SPI clock |
| SD_MISO   | SPI master in slave out |
| SD_MOSI   | SPI master out slave in |
| SD_CS     | SPI chip select |
