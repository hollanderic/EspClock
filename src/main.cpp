#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "privateinfo.h" // Contains WiFi credentials and other private info

/*  Pin signal connections:
 *  R1_PIN: 25   G1_PIN: 26   B1_PIN: 27
 *  R2_PIN: 14   G2_PIN: 12   B2_PIN: 13
 *  A_PIN:  23   B_PIN:  19   C_PIN:  5
 *  D_PIN:  17   E_PIN:  -1
 *  LAT_PIN: 4   OE_PIN: 15   CLK_PIN: 16
 *
 *  E_PIN is required for 1/32 scan panels, like 64x64px.
 *  Any available pin would do, i.e. IO32.
 */

// Display libraries


#define R1_PIN 1
#define G1_PIN 2
#define B1_PIN 3
#define R2_PIN 4
#define G2_PIN 5
#define B2_PIN 6
#define A_PIN 7
#define B_PIN 8
#define C_PIN 9
#define D_PIN 10
#define E_PIN -1 // required for 1/32 scan panels, like 64x64px. Any available pin would do, i.e. IO32
#define LAT_PIN 11
#define OE_PIN 12
#define CLK_PIN 13

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

HUB75_I2S_CFG::i2s_pins _pins={R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};


// WiFi credentials (replace with your actual network)
const char* ssid     = SSID;
const char* password = PASSWORD;

// NTP Server settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000; // -5 hours for EST
const int   daylightOffset_sec = 3600;

// LED Matrix settings
#define PANEL_RES_X 64      // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 32      // Number of pixels tall of each INDIVIDUAL panel module. 
#define PANEL_CHAIN 1       // Total number of panels chained one to another

MatrixPanel_I2S_DMA *dma_display = nullptr;

void printLocalTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

    dma_display->clearScreen();
    dma_display->setCursor(2, 0); // (x,y)
    dma_display->setTextColor(dma_display->color565(255, 255, 255)); // White text
    dma_display->setTextSize(1);
    
    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    dma_display->print(timeStr);
}

void setup() {
    Serial.begin(115200);

    // Initialize display
    HUB75_I2S_CFG mxconfig(
        PANEL_RES_X,   // module width
        PANEL_RES_Y,   // module height
        PANEL_CHAIN,    // chain length
        _pins       
    );
    // Optional: override default pins if needed
    // mxconfig.gpio.e = 18;
    
    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    dma_display->begin();
    dma_display->setBrightness8(90); // 0-255
    dma_display->clearScreen();
    dma_display->setTextColor(dma_display->color565(255, 255, 255));
    dma_display->setCursor(0, 4);
    dma_display->print("WiFi...");

    // Connect to WiFi
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println("");
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected.");
      dma_display->clearScreen();
      dma_display->setCursor(0, 4);
      dma_display->setTextColor(dma_display->color565(0, 0, 255));
      dma_display->print("NTP...");

      // Init and get the time
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      printLocalTime();
    } else {
      Serial.println("WiFi connection failed.");
      dma_display->clearScreen();
      dma_display->setCursor(0, 0);
      dma_display->setTextColor(dma_display->color565(255, 0, 0));
      dma_display->print("No WiFi");
    }

    // Disconnect WiFi as it's no longer needed, optional
    // WiFi.disconnect(true);
    // WiFi.mode(WIFI_OFF);
}

void loop() {
    // Only update the display approximately every second instead of constantly
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        if (WiFi.status() == WL_CONNECTED) {
            printLocalTime();
        }
    }
}
