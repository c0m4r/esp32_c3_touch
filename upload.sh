#!/bin/bash

#LOG_LEVEL="debug"
LOG_LEVEL="info"

arduino-cli board attach \
    --fqbn "esp32:esp32:esp32c3:UploadSpeed=921600,CDCOnBoot=cdc" \
    -p /dev/ttyACM0 \
    --programmer esptool

arduino-cli upload /home/c0m4r/Arduino/esp32_c3_touch/esp32_c3_touch.ino \
    -v \
    --fqbn "esp32:esp32:esp32c3:UploadSpeed=921600,CDCOnBoot=cdc" \
    --log \
    --log-level "$LOG_LEVEL" \
    -p /dev/ttyACM0
