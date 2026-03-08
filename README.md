# EspClock

EspClock is a WiFi-enabled LED matrix clock built for the ESP32 platform. It fetches the NTP time as well as information about upcoming rocket launches from The Space Devs API, displaying a countdown on a HUB75 LED matrix panel.

## Hardware Required
- Board: ESP32 development board (e.g., Adafruit Feather ESP32-S3).
- Display: HUB75 LED Matrix Panel (e.g., 64x32 or 64x64).
- Appropriate wiring between the ESP32 and the HUB75 panel.

## Dependencies

The project is built using PlatformIO and the Arduino framework. The libraries required (automatically managed in `platformio.ini`) are:
- [ESP32 HUB75 LED MATRIX PANEL DMA Display](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA) (by mrfaptastic)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [ArduinoJson](https://arduinojson.org/)

## Setup

Before you can build and flash the project, you need to create a `privateinfo.h` file in the `src/` directory to store your Wi-Fi credentials.

1. Create a file named `src/privateinfo.h`
2. Add your Wi-Fi SSID and password as macros:
   ```cpp
   #pragma once
   #define SSID "your_wifi_ssid"
   #define PASSWORD "your_wifi_password"
   ```

## Build and Flash

This project uses [PlatformIO](https://platformio.org/). You can build and upload the code via the PlatformIO CLI or using the PlatformIO extension for your IDE (like VSCode).

### Using the PlatformIO CLI:

1. **Build the project:**
   ```bash
   platformio run
   ```

2. **Upload to the ESP32:**
   Connect your ESP32 board to your computer via USB and run:
   ```bash
   platformio run --target upload
   ```

3. **Monitor serial output (optional for debugging):**
   ```bash
   platformio device monitor
   ```

## Display Configuration

By default, the display is configured for a 64x32 matrix. If you have a different size or multiple panels chained together, adjust the definitions in `src/main.cpp`:
```cpp
#define PANEL_RES_X 64      // Panel width
#define PANEL_RES_Y 32      // Panel height
#define PANEL_CHAIN 2       // Number of chained panels
```
