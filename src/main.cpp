#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
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

// Rocket launch state
time_t nextLaunchEpoch = 0;
unsigned long lastLaunchFetch = 0;
const unsigned long LAUNCH_FETCH_INTERVAL = 4 * 60 * 60 * 1000; // 4 hours in ms
bool fetchingLaunch = false;

void fetchNextLaunch() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        Serial.println("Fetching next rocket launch...");
        char timeStr[9];
        strcpy(timeStr, "Fetching");
        dma_display->setCursor(2, 16);
        dma_display->setTextColor(dma_display->color565(150, 150, 150));
        dma_display->setTextSize(1);
        dma_display->print(timeStr);
        http.begin("https://ll.thespacedevs.com/2.2.0/launch/upcoming/?limit=1&location__ids=12,27");
        int httpCode = http.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, payload);
                if (!error) {
                    const char* net = doc["results"][0]["net"];
                    if (net) {
                        struct tm tm;
                        // Example: 2026-03-10T03:14:00Z
                        if (strptime(net, "%Y-%m-%dT%H:%M:%SZ", &tm) != NULL) {
                            // Convert UTC tm to epoch
                            nextLaunchEpoch = mktime(&tm) - gmtOffset_sec - daylightOffset_sec;
                            Serial.printf("Next Launch: %s, Epoch: %ld\n", net, nextLaunchEpoch);
                        } else {
                            Serial.println("Failed to parse net string");
                        }
                    }
                } else {
                    Serial.print("deserializeJson() failed: ");
                    Serial.println(error.c_str());
                }
            } else {
                 Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
            }
        } else {
            Serial.printf("HTTP GET failed, code: %d\n", httpCode);
        }
        http.end();
        lastLaunchFetch = millis();
    }
}

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

    time_t now;
    time(&now);
    
    // Launch countdown display
    if (nextLaunchEpoch > 0) {
        dma_display->setCursor(2, 16);
        dma_display->setTextSize(1);
        
        long diff = nextLaunchEpoch - now;
        if (diff > 0) {
            dma_display->setTextColor(dma_display->color565(255, 165, 0)); // Orange
            if (diff > 86400) {
                // More than 24 hours
                int days = diff / 86400;
                int hours = (diff % 86400) / 3600;
                char cndStr[20];
                sprintf(cndStr, "T-%dd %02dh", days, hours);
                dma_display->print(cndStr);
            } else {
                // Less than 24 hours
                int hours = diff / 3600;
                int minutes = (diff % 3600) / 60;
                int seconds = diff % 60;
                char cndStr[20];
                sprintf(cndStr, "T-%02d:%02d:%02d", hours, minutes, seconds);
                dma_display->print(cndStr);
            }
        } else if (diff > -3600) {
             // Just launched (within 1 hour)
             dma_display->setTextColor(dma_display->color565(0, 255, 0)); // Green
             dma_display->print("Launched!");
        } else {
            // Force fetch on next loop
            nextLaunchEpoch = 0;
            lastLaunchFetch = millis() - LAUNCH_FETCH_INTERVAL;
        }
    }
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
      
      // Initial fetch of next launch
      fetchNextLaunch();
      
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
    
    // Periodic launch fetch
    if (WiFi.status() == WL_CONNECTED && (millis() - lastLaunchFetch > LAUNCH_FETCH_INTERVAL || nextLaunchEpoch == 0)) {
        fetchNextLaunch();
    }
}
