#!/bin/bash

#LOG_LEVEL="debug"
LOG_LEVEL="info"

arduino-cli compile \
    -j 0 -v \
    --fqbn "esp32:esp32:esp32c3:UploadSpeed=921600,CDCOnBoot=cdc" \
    --log --log-level "$LOG_LEVEL" \
    /home/c0m4r/Arduino/esp32_c3_touch/esp32_c3_touch.ino
