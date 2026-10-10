// Configuration / control web page served by the ESP32.
#pragma once
#include <stdint.h>

void webBegin(bool setupMode);  // setupMode: captive portal on the setup access point
void webLoop();
// A firmware upload is running: main.cpp opens no Bluetooth paging window (WiFi must stay on).
bool webUploadActive();

// Implemented in main.cpp: hop to this access point of the configured network on the next
// loop() pass (after the HTTP answer has gone out).
void wifiRequestRoam(const uint8_t bssid[6], int channel);
