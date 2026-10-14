#include "settings.h"
#include <Preferences.h>
#include "config.h"
#include "sdcard.h"

Settings settings;

static Preferences prefs;

bool settingsParseStations(const String& text, std::vector<Station>& out) {
  out.clear();
  int start = 0;
  while (start < (int)text.length() && (int)out.size() < MAX_STATIONS) {
    int end = text.indexOf('\n', start);
    if (end < 0) end = text.length();
    String line = text.substring(start, end);
    start = end + 1;
    line.trim();
    int sep = line.indexOf('|');
    if (line.isEmpty() || sep <= 0) continue;
    Station s;
    s.name = line.substring(0, sep);
    s.url = line.substring(sep + 1);
    s.name.trim();
    s.url.trim();
    if (s.url.startsWith("http://") || s.url.startsWith("https://")) out.push_back(s);
  }
  return !out.empty();
}

String settingsStationsText() {
  String text;
  for (const Station& s : settings.stations) text += s.name + "|" + s.url + "\n";
  return text;
}

void settingsLoad() {
  prefs.begin("radio", true);
  settings.wifiSsid = prefs.getString("ssid", "");
  settings.wifiPass = prefs.getString("pass", "");
  settings.btName = prefs.getString("bt", DEFAULT_BT_NAME);
  settings.volume = prefs.getInt("vol", DEFAULT_VOLUME);
  settings.lastStation = prefs.getInt("station", 0);
  settings.autoPlay = prefs.getBool("autoplay", true);
  settings.btIdentity = prefs.getInt("btid", 0);
  settings.wifiRoamDbm = prefs.getInt("roam", -65);
  settings.wifiAutoScan = prefs.getBool("autoscan", false);
  settings.btEnabled = prefs.getBool("bten", true);
  settings.displayFlip = prefs.getBool("flip", false);
  settings.scrollTitles = prefs.getBool("scroll", true);
  settings.line32 = prefs.getBool("l32", false);
  settings.backlight = constrain(prefs.getInt("bl", 100), 5, 100);
  settings.nightDim = prefs.getBool("ngt", false);
  settings.nightFrom = constrain(prefs.getInt("ngtf", 22 * 60), 0, 24 * 60 - 1);
  settings.nightTo = constrain(prefs.getInt("ngtt", 7 * 60), 0, 24 * 60 - 1);
  settings.nightLevel = constrain(prefs.getInt("ngtl", 10), 0, 100);
  settings.idleMinutes = constrain(prefs.getInt("idlm", 0), 0, 240);
  settings.idleLevel = constrain(prefs.getInt("idll", 0), 0, 100);
  settings.btLead = prefs.getBool("btlead", true);
  // Station list: a blob under "stlist" (NVS strings are capped at 4000 bytes, which 20+
  // StreamTheWorld lines exceed). Older installs stored a string under "stations".
  String stations;
  size_t len = prefs.getBytesLength("stlist");
  if (len > 0) {
    char* buf = (char*)malloc(len + 1);
    if (buf) {
      prefs.getBytes("stlist", buf, len);
      buf[len] = 0;
      stations = buf;
      free(buf);
    }
  } else {
    stations = prefs.getString("stations", DEFAULT_STATIONS);
  }
  prefs.end();

  if (!settingsParseStations(stations, settings.stations))
    settingsParseStations(DEFAULT_STATIONS, settings.stations);
  settings.volume = constrain(settings.volume, 0, 100);
  if (settings.lastStation < 0 || settings.lastStation >= (int)settings.stations.size())
    settings.lastStation = 0;
  if (settings.wifiRoamDbm != 0) settings.wifiRoamDbm = constrain(settings.wifiRoamDbm, -90, -40);
}

void settingsSaveVolume() {
  prefs.begin("radio", false);
  prefs.putInt("vol", settings.volume);
  prefs.end();
  sdScheduleBackup();  // SD card copy, 10 s after the last change
}

void settingsSave() {
  prefs.begin("radio", false);
  prefs.putString("ssid", settings.wifiSsid);
  prefs.putString("pass", settings.wifiPass);
  prefs.putString("bt", settings.btName);
  prefs.putInt("vol", settings.volume);
  prefs.putInt("station", settings.lastStation);
  prefs.putBool("autoplay", settings.autoPlay);
  prefs.putInt("btid", settings.btIdentity);
  prefs.putInt("roam", settings.wifiRoamDbm);
  prefs.putBool("autoscan", settings.wifiAutoScan);
  prefs.putBool("bten", settings.btEnabled);
  prefs.putBool("flip", settings.displayFlip);
  if (prefs.isKey("tftv")) prefs.remove("tftv");  // panel variant, gone since one panel is left
  prefs.putBool("scroll", settings.scrollTitles);
  prefs.putBool("l32", settings.line32);
  prefs.putInt("bl", settings.backlight);
  prefs.putBool("ngt", settings.nightDim);
  prefs.putInt("ngtf", settings.nightFrom);
  prefs.putInt("ngtt", settings.nightTo);
  prefs.putInt("ngtl", settings.nightLevel);
  prefs.putInt("idlm", settings.idleMinutes);
  prefs.putInt("idll", settings.idleLevel);
  prefs.putBool("btlead", settings.btLead);
  String stations = settingsStationsText();
  prefs.putBytes("stlist", stations.c_str(), stations.length());
  if (prefs.isKey("stations")) prefs.remove("stations");  // pre-blob format
  prefs.end();
  sdScheduleBackup();
}
