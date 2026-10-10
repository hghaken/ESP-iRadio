// microSD card on a separate breakout (CS GPIO4, shares VSPI with the display). Off until it is
// wired: SD_ENABLED in config.h.
//
// What lives on the card (all optional; without a card everything else works as before):
//   /log/YYYY-MM-DD.log     copy of the live log, appended every 30 s (90 days kept)
//   /stats/YYYY-MM-DD.csv   one line of measurements per minute (365 days kept)
//   /backup/settings.txt, /backup/stations.txt   settings and station list (no WiFi password)
//   /logos/<hash>.565       64x64 (older: 40x40) station logo, RGB565, made by the web page
//   /firmware/<build>.bin   copies of firmware that ran 10 min without trouble (newest 5)
// All card work runs in loop() (sdLoop) in small steps; no extra task, no internal RAM held.
#pragma once
#include <Arduino.h>

bool sdBegin();          // mount the card at boot; false when there is none (or no FAT)
String sdInfo();         // one line for the web page
void sdLoop();           // call from loop()

// Format (web page): only a card without a FAT volume is formatted.
void sdRequestFormat();
String sdFormatResult();  // "" never asked, "busy" while running, else a sentence for the page

// Listing for the web page (JSON) and the paths it may download.
String sdListJson();
bool sdPathAllowed(const String& path);

// Settings backup: written 10 s after the last change; restore applies it (caller restarts).
void sdScheduleBackup();
bool sdBackupNow();
String sdRestoreSettings(bool* ok);

// Station logos, square RGB565 (64x64 = 8192 bytes; 40x40 files from before still load),
// keyed by the station name.
static const int SD_LOGO_MAX = 64;
bool sdLogoSave(int station, const uint8_t* data, size_t len);
bool sdLogoDelete(int station);
int sdLogoGeneration();  // changes when the logo for the current station changes; 0 = none
// Scaled to size x size pixels (the panel's logo size, at most SD_LOGO_MAX); false when none.
bool sdCopyLogo(uint16_t* dst, int size);

// Firmware copies.
void sdRequestFirmwareSave();
void sdRequestFirmwareRestore(const String& name);
String sdFirmwareResult();  // "" / "busy" / sentence
