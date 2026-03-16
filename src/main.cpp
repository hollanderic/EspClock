#include "privateinfo.h" // Contains WiFi credentials and other private info
#include "time.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Pin signal connections:
#define R1_PIN 18
#define G1_PIN 25
#define B1_PIN 5
#define R2_PIN 17
#define G2_PIN 33
#define B2_PIN 16
#define A_PIN 4
#define B_PIN 3
#define C_PIN 0
#define D_PIN 21
#define E_PIN                                                                  \
  32 // required for 1/32 scan panels, like 64x64px. Any available pin would do,
     // i.e. IO32
#define LAT_PIN 19
#define OE_PIN 15
#define CLK_PIN 2

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

HUB75_I2S_CFG::i2s_pins _pins = {R1_PIN, G1_PIN,  B1_PIN, R2_PIN, G2_PIN,
                                 B2_PIN, A_PIN,   B_PIN,  C_PIN,  D_PIN,
                                 E_PIN,  LAT_PIN, OE_PIN, CLK_PIN};

// WiFi credentials (replace with your actual network)
const char *ssid = MYSSID;
const char *password = MYPASSWORD;

// NTP Server settings
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000; // -5 hours for EST
const int daylightOffset_sec = 3600;

// LED Matrix settings
#define PANEL_RES_X 64 // Number of pixels wide of each INDIVIDUAL panel module.
#define PANEL_RES_Y 64 // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 4  // Total number of panels chained one to another

MatrixPanel_I2S_DMA *dma_display = nullptr;

// Rocket launch state
time_t nextLaunchEpoch = 0;
char launchName[43] = ""; // 42 chars + null terminator
unsigned long lastLaunchFetch = 0;
const unsigned long LAUNCH_FETCH_INTERVAL = 1 * 60 * 60 * 1000; // 1 hour in ms
bool fetchingLaunch = false;

void fetchNextLaunch() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // Allow connections without verifying the SSL
                          // certificate
    HTTPClient http;
    Serial.println("Fetching next rocket launch...");
    dma_display->setCursor(2, 16);
    dma_display->setTextColor(dma_display->color565(150, 150, 150),
                              0); // Gray text on black background
    dma_display->setTextSize(1);
    dma_display->print("Fetching             ");
    dma_display->setCursor(2, 24);
    dma_display->print("                     "); // clear line 3
    String url = "https://ll.thespacedevs.com/2.2.0/launch/upcoming/"
                 "?limit=1&location__ids=12,27&window_start__gt=";
    time_t now;
    time(&now);
    struct tm tm_utc;
    gmtime_r(&now, &tm_utc);
    char timeStr[30];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
    url += timeStr;
    Serial.print("doing url: ");
    Serial.println(url);
    http.begin(client, url.c_str());
    http.setTimeout(10000);
    Serial.println("doing get");
    int httpCode = http.GET();
    Serial.printf("HTTP GET code: %d\n", httpCode);
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          const char *net = doc["results"][0]["net"];
          if (net) {
            struct tm tm;
            // Example: 2026-03-10T03:14:00Z
            if (strptime(net, "%Y-%m-%dT%H:%M:%SZ", &tm) != NULL) {
              // Convert UTC tm to epoch
              nextLaunchEpoch =
                  mktime(&tm) + gmtOffset_sec + daylightOffset_sec;
              Serial.printf("Next Launch: %s, Epoch: %ld\n", net,
                            nextLaunchEpoch);
            } else {
              Serial.println("Failed to parse net string");
            }
          }
          const char *name = doc["results"][0]["name"];
          if (name) {
            strncpy(launchName, name, 41);
            launchName[41] = '\0';
            Serial.printf("Launch Name: %s\n", launchName);
          }
        } else {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
        }
      } else {
        Serial.printf("HTTP GET failed, error: %s\n",
                      http.errorToString(httpCode).c_str());
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

  dma_display->setCursor(16, 0); // (x,y)
  dma_display->setTextColor(dma_display->color565(255, 255, 255),
                            0); // White text on black bg
  dma_display->setTextSize(2);

  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  dma_display->print(timeStr);

  time_t now;
  time(&now);

  // Launch countdown display
  if (nextLaunchEpoch > 0) {
    // Line 2: launch name
    dma_display->setCursor(2, 16);
    dma_display->setTextColor(dma_display->color565(0, 255, 255),
                              0); // Cyan text on black bg
    dma_display->setTextSize(1);
    char paddedName[44];
    snprintf(paddedName, sizeof(paddedName), "%-41s", launchName);
    dma_display->print(paddedName);

    // Line 3: countdown
    dma_display->setCursor(2, 24);
    dma_display->setTextSize(1);

    long diff = nextLaunchEpoch - now;
    if (diff > 0) {
      dma_display->setTextColor(dma_display->color565(255, 165, 0),
                                0); // Orange text on black bg
      int days = diff / 86400;
      int hours = (diff % 86400) / 3600;
      int minutes = (diff % 3600) / 60;
      int seconds = diff % 60;
      char cndStr[25];
      snprintf(cndStr, sizeof(cndStr), "T-%dd:%02dh:%02dm:%02ds", days, hours,
               minutes, seconds);
      char paddedCndStr[22];
      snprintf(paddedCndStr, sizeof(paddedCndStr), "%-21s", cndStr);
      dma_display->print(paddedCndStr);

    } else if (diff > -3600) {
      // Just launched (within 1 hour)
      dma_display->setTextColor(dma_display->color565(0, 255, 0),
                                0); // Green text on black bg
      dma_display->print("Launched!            ");
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
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, // module width
                         PANEL_RES_Y, // module height
                         PANEL_CHAIN, // chain length
                         _pins);
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
    delay(2000);
    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Initial fetch of next launch
    fetchNextLaunch();

    dma_display->clearScreen(); // Clear setup messages like "NTP..." before
                                // normal loop
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
  if (WiFi.status() == WL_CONNECTED &&
      (millis() - lastLaunchFetch > LAUNCH_FETCH_INTERVAL ||
       nextLaunchEpoch == 0)) {
    fetchNextLaunch();
  }
}
