// Persistent settings (NVS / Preferences).
#pragma once
#include <Arduino.h>
#include <vector>

struct Station {
  String name;
  String url;
};

struct Settings {
  String wifiSsid;
  String wifiPass;  // never printed or sent to the web page
  String btName;
  int volume = 0;        // 0..100
  int lastStation = 0;
  bool autoPlay = true;  // resume the last station after boot
  bool displayFlip = false; // screen rotated 180 degrees (web toggle, NVS `flip`)
  bool scrollTitles = true; // scroll station/artist/song lines that do not fit (NVS `scroll`)
  bool line32 = false;      // line output with 32-bit I2S slots instead of 16 (NVS `l32`)
  bool batteryFitted = true; // a Li-Po is connected; off = no battery reading (NVS `batf`)
  bool speakerOn = true;     // MAX98357A speaker plays with the line output (NVS `spk`)
  // Backlight (display.cpp picks the lowest that applies, fading ~1 s). Percent of full.
  int backlight = 100;      // normal level, 5..100 (NVS `bl`)
  bool nightDim = false;    // night mode on (NVS `ngt`), needs the SNTP clock
  int nightFrom = 22 * 60;  // minutes after midnight (NVS `ngtf`); the window may cross midnight
  int nightTo = 7 * 60;     // (NVS `ngtt`)
  int nightLevel = 10;      // 0..100, 0 = off (NVS `ngtl`)
  int idleMinutes = 0;      // dim after this many minutes without a new title/station/BT change, 0 = never (NVS `idlm`)
  int idleLevel = 0;        // 0..100, 0 = off (NVS `idll`)
  bool btLead = true;       // the radio stays master of the Bluetooth link: no role switch (NVS `btlead`)
  bool btEnabled = true; // Bluetooth on: connect to the headphones (web toggle). Off: no paging,
                         // no paging windows, link dropped.
  int btIdentity = 0;    // >0: locally administered base MAC (last octet + this value).
                         // Changed only via /api/btidentity. "Erase all pairings" does not
                         // bump it: each new address uses one of the headphone's two slots.
  int wifiRoamDbm = -65; // While the headphones are connected, scan for a stronger mesh
                         // point below this RSSI. 0 = off. Set via /api/roam.
  bool wifiAutoScan = false; // Web page scans for networks on open. Off by default:
                              // that scan crackles A2DP audio. Set via /api/autoscan.
                              // Setup mode always scans, this flag does not apply there.
  std::vector<Station> stations;
};

extern Settings settings;

void settingsLoad();
void settingsSave();
// Writes only the volume key: the slider must not rewrite every key and the station blob.
void settingsSaveVolume();
// Parses "Name|URL" lines; returns false if no valid station was found.
bool settingsParseStations(const String& text, std::vector<Station>& out);
String settingsStationsText();
