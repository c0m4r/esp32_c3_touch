#!/bin/bash

arduino-cli config set board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli update
arduino-cli upgrade
