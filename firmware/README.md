# Firmware

This directory contains the ESP8266 NodeMCU firmware for the
GWID (General-purpose WiFi-connected Indicator DeviCe).

- Entry point: `src/` - contains the Arduino firmware file for the NodeMCU board

## Building with Arduino IDE

1. Install the **ESP8266** core via the Boards Manager.
2. Ensure required libraries are installed
    - string.h
    - Preferences.h
    - ESP8266WiFi.h
    - WiFiClient.h
    - DNSServer.h
    - WiFiManager.h
    - ESP8266HTTPClient.h
    - ESP8266WebServer.h
    - ArduinoOTA.h
    - ArduinoJson.h
    - Adafruit_NeoPixel.h
4. Copy the latest .ino sketch from the `src/` directory into the Arduino IDE editor.
5. Select the correct ESP8266 NodeMCU board and upload.

Though the initial software upload must be done via NodeMCU serial port, the GWID does have an Over-the-Air (OTA) update capability. Once the GWID is connected to a WiFi network, new software releases can be uploaded over WiFi using the Arduino IDE.
