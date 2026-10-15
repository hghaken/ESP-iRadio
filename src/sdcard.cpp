#include "sdcard.h"
#include <SD.h>
#include <esp_core_dump.h>
#include <esp_system.h>
#include <stdarg.h>
#include <sd_diskio.h>
#include <SPI.h>
#include <Update.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <freertos/semphr.h>
#include <time.h>
#include <algorithm>
#include <vector>
#include "battery.h"
#include "config.h"
#include "display.h"
#include "player.h"
#include "settings.h"
#include "version.h"
#include "weblog.h"

static bool mounted = false;
static const char* resetReasonText(esp_reset_reason_t r);
static const char* BACKUP_SETTINGS = "/backup/settings.txt";
static const char* BACKUP_STATIONS = "/backup/stations.txt";
static String info = "not checked";
static volatile bool formatRequested = false;
static String formatResult;  // see sdFormatResult()

static String sizeText(uint64_t bytes) {
  if (bytes >= 1000ULL * 1000 * 1000) return String(bytes / 1e9, 1) + " GB";
  if (bytes >= 1000ULL * 1000) return String(bytes / 1e6, 1) + " MB";
  return String((uint32_t)(bytes / 1000)) + " kB";
}

// ---------------------------------------------------------------- time

static bool timeKnown() { return time(nullptr) > 1700000000; }  // set by SNTP (main.cpp)

// Fixed formats with snprintf instead of strftime: the prebuilt framework links newlib's
// strftime into IRAM (3 KB), and env `radio` has no IRAM to spare.
enum TimeFmt { DAY, CLOCK, STAMP, STAMP_MIN };  // 2026-10-10 / 15:52:30 / both / without seconds

static String timeText(TimeFmt f, time_t when = 0) {
  time_t now = when ? when : time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  char buf[24];
  if (f == DAY)
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
  else if (f == CLOCK)
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
  else if (f == STAMP)
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec);
  else
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
             t.tm_min);
  return buf;
}

// ---------------------------------------------------------------- files

struct FileEntry {
  String name;
  uint32_t size;
};

static std::vector<FileEntry> listDir(const char* dir) {
  std::vector<FileEntry> out;
  File d = SD.open(dir);
  if (!d || !d.isDirectory()) return out;
  for (File f = d.openNextFile(); f; f = d.openNextFile()) {
    if (!f.isDirectory()) {
      String n = f.name();
      int slash = n.lastIndexOf('/');
      if (slash >= 0) n = n.substring(slash + 1);
      out.push_back({n, (uint32_t)f.size()});
    }
    f.close();
  }
  d.close();
  std::sort(out.begin(), out.end(), [](const FileEntry& a, const FileEntry& b) { return a.name < b.name; });
  return out;
}

// Keeps the newest `keep` files (names sort by date) whose name ends in `ext`.
static void prune(const char* dir, const char* ext, size_t keep) {
  std::vector<FileEntry> files = listDir(dir);
  files.erase(std::remove_if(files.begin(), files.end(), [&](const FileEntry& e) { return !e.name.endsWith(ext); }),
              files.end());
  for (size_t i = 0; i + keep < files.size(); i++) {
    SD.remove(String(dir) + "/" + files[i].name);
    Log.printf("[sd] removed old %s/%s\n", dir, files[i].name.c_str());
  }
}

// ---------------------------------------------------------------- mount / format

// format: let the library run f_mkfs when the card has no valid FAT volume (FatFs error 13).
// A card that already has FAT is only mounted, never wiped. ESP-IDF 4.4's FatFs has no exFAT,
// so it makes FAT32 (FAT16 on small cards).
// Defined in the SD library's sd_diskio.cpp (C++ linkage, not in its header).
unsigned char ff_sd_initialize(uint8_t pdrv);

// After a failed mount: initialise the card by hand and log what it reports and what sector 0
// (and the first partition's boot sector) holds, so "no valid FAT volume" can be told apart
// from a wrong card type, an exFAT card or a bad connection.
static void cardDiag() {
  uint8_t pdrv = sdcard_init(SD_CS, &SPI, 1000000);
  if (pdrv == 0xFF) { Log.println("[sd] diag: no free drive slot"); return; }
  unsigned st = ff_sd_initialize(pdrv);
  Log.printf("[sd] diag: init status %u, card type %u (2 SD, 3 SDHC), %lu sectors\n", st,
             (unsigned)sdcard_type(pdrv), (unsigned long)sdcard_num_sectors(pdrv));
  uint8_t* b = (uint8_t*)malloc(512);
  if (b && st == 0 && sd_read_raw(pdrv, b, 0)) {
    uint32_t lba = b[0x1C6] | b[0x1C7] << 8 | b[0x1C8] << 16 | (uint32_t)b[0x1C9] << 24;
    Log.printf("[sd] diag: sector 0 sig %02X%02X, oem '%.8s', part type %02X at %lu\n",
               b[510], b[511], (const char*)b + 3, b[0x1C2], (unsigned long)lba);
    if (lba && sd_read_raw(pdrv, b, lba))
      Log.printf("[sd] diag: boot sector sig %02X%02X, oem '%.8s', fat32 label '%.8s'\n",
                 b[510], b[511], (const char*)b + 3, (const char*)b + 0x52);
  } else {
    Log.println("[sd] diag: sector 0 not readable");
  }
  free(b);
  sdcard_uninit(pdrv);
}

static bool mount(bool format) {
  // The card shares VSPI with the TFT. Arduino's SPI transactions take the bus lock (HAL
  // locks are on), so the display task and the card take turns. 4 MHz: safe for any breakout
  // and jumper wires (the Robot LCD slot until 2026-10-10 and the breakout since 2026-10-14).
  // Cards that hide the SDHC bit are fixed in the library (scripts/patch_sd_sdhc.py).
  mounted = SD.begin(SD_CS, SPI, 4000000, "/sd", 3, format);
  if (!mounted) {
    info = "no card (or not readable)";
    Log.println("[sd] no card or not readable");
    if (!format) cardDiag();
    return false;
  }
  uint8_t type = SD.cardType();
  const char* typeName = type == CARD_SDHC ? "SDHC/SDXC" : type == CARD_SD ? "SD" : type == CARD_MMC ? "MMC" : "unknown";
  // Free space is not asked for: f_getfree can scan the whole FAT (tens of seconds on a
  // 128 GB card) when the FSInfo sector is stale after a power cut.
  info = String(typeName) + ", " + sizeText(SD.totalBytes());
  for (const char* d : {"/log", "/stats", "/backup", "/logos", "/firmware", "/events", "/stations", "/stations/months",
                        "/crash"})
    if (!SD.exists(d)) SD.mkdir(d);
  Log.printf("[sd] %s card, %llu MB filesystem\n", typeName, SD.totalBytes() / (1024 * 1024));
  return true;
}

bool sdBegin() {
  sdEvent("boot: firmware %s (%s), reset reason: %s", FIRMWARE_VERSION, buildStamp,
          resetReasonText(esp_reset_reason()));
#if !SD_ENABLED
  info = "disabled (no card reader wired yet)";
  Log.println("[sd] disabled (SD_ENABLED 0 in config.h)");
  return false;
#endif
  bool ok = mount(false);
  if (!ok) {
    // A reset in the middle of a card transfer (esptool's hard reset after a USB flash, seen
    // 2026-10-14) leaves the card out of step: GO_IDLE_STATE fails once and a second attempt
    // (the init sends its wake-up clocks again) finds it.
    SD.end();
    delay(200);
    Log.println("[sd] second attempt");
    ok = mount(false);
  }
  if (ok && !SD.exists(BACKUP_SETTINGS)) sdScheduleBackup();
  return ok;
}

String sdInfo() { return info; }

void sdRequestFormat() {
  formatResult = "busy";
  formatRequested = true;
}

String sdFormatResult() { return formatResult; }

// The web server does not answer while f_mkfs runs (~45 s for 128 GB), so the page counts
// the seconds itself and polls for the result afterwards.
static void doFormat() {
#if !SD_ENABLED
  formatResult = "SD support is off (no card reader wired yet).";
  return;
#endif
  info = "formatting...";
  Log.println("[sd] format requested from the web page (only a card without FAT is formatted)");
  SD.end();
  if (mount(false)) {
    formatResult = "The card already has a FAT file system; nothing was erased.";
    Log.println("[sd] card already has FAT, nothing formatted");
    return;
  }
  SD.end();
  uint32_t start = millis();
  bool ok = mount(true);
  uint32_t ms = millis() - start;
  formatResult = ok ? "Formatted (FAT32) in " + String((ms + 500) / 1000) + " s."
                    : String("Formatting failed (no card or a faulty card); see the log.");
  Log.printf("[sd] format %s after %lu ms\n", ok ? "done" : "failed", (unsigned long)ms);
}

// ---------------------------------------------------------------- log copy

static uint32_t logPos = 0;  // position in the web log (bytes since boot) already on the card
static bool bootMarked = false;
static String lastLogDay;

// Appends what the web log gained since the last call. Waits up to 3 min after boot for the
// clock (file names are dates); without a clock it writes to no-date.log.
static void writeLog() {
  bool known = timeKnown();
  if (!known && millis() < 180000) return;
  String day = known ? timeText(DAY) : String("no-date");
  String path = "/log/" + day + ".log";
  if (day != lastLogDay) {
    lastLogDay = day;
    prune("/log", ".log", 90);
  }
  File f = SD.open(path, FILE_APPEND);
  if (!f) return;
  if (!bootMarked) {
    bootMarked = true;
    f.printf("\n===== opgestart, firmware %s =====\n", buildStamp);
  }
  f.printf("--- %s (uptime %lu s) ---\n", known ? timeText(CLOCK).c_str() : "--:--:--",
           (unsigned long)(millis() / 1000));
  char* buf = (char*)malloc(4096);  // PSRAM (malloc above 256 bytes prefers it)
  if (buf) {
    for (int i = 0; i < 32; i++) {  // at most 128 KB per call, the ring holds 64 KB
      size_t len = 0;
      bool skipped = false, restarted = false;
      uint32_t next = weblogRead(logPos, buf, 4096, &len, &skipped, &restarted);
      if (skipped) f.print("[... oudere regels overgeslagen ...]\n");
      if (len == 0) break;
      f.write((const uint8_t*)buf, len);
      logPos = next;
      if (len < 4096) break;
    }
    free(buf);
  }
  f.close();
}

// ---------------------------------------------------------------- measurements

static String lastStatsDay;

static void writeStats() {
  if (!timeKnown()) return;
  String day = timeText(DAY);
  String path = "/stats/" + day + ".csv";
  if (day != lastStatsDay) {
    lastStatsDay = day;
    prune("/stats", ".csv", 365);
  }
  bool fresh = !SD.exists(path);
  File f = SD.open(path, FILE_APPEND);
  if (!f) return;
  if (fresh)
    f.print("time,uptime_s,state,station,codec,in_kBps,buffer_kB,underruns,reconnects,bt_kBps,silence_kBps,"
            "bt,bt_rssi_delta,wifi_rssi,bssid,channel,heap,min_heap,battery_mV,battery_pct,usb,usb_mV\n");
  PlayerStatus p = playerStatus();
  BatteryInfo b = batteryRead();
  static const char* STATE[] = {"stopped", "connecting", "buffering", "playing", "error"};
  String station = p.stationName;
  station.replace(",", ";");  // keep the CSV plain (the page splits on commas)
  bool wifiUp = WiFi.status() == WL_CONNECTED;
  f.printf("%s,%lu,%s,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%d,%u,%u,%d,%d,%d,%d\n",
           timeText(STAMP).c_str(), (unsigned long)(millis() / 1000), STATE[(int)p.state],
           station.c_str(), p.codec.c_str(), p.inKBps, p.bufferKB, p.underruns, p.reconnects, p.btKBps,
           p.silenceKBps, p.btConnected ? 1 : 0, p.btRssiDelta, wifiUp ? (int)WiFi.RSSI() : 0,
           wifiUp ? WiFi.BSSIDstr().c_str() : "", wifiUp ? (int)WiFi.channel() : 0, ESP.getFreeHeap(),
           ESP.getMinFreeHeap(), b.valid ? b.millivolts : 0, b.valid ? b.percent : -1, b.usb ? 1 : 0, b.usbMv);
  f.close();
}

// ---------------------------------------------------------------- settings backup

static uint32_t backupDue = 0;  // millis() deadline, 0 = nothing pending

void sdScheduleBackup() { backupDue = (millis() + 10000) | 1; }

static bool writeText(const char* path, const String& text) {
  File f = SD.open(path, FILE_WRITE);  // truncates
  if (!f) return false;
  size_t n = f.print(text);
  f.close();
  return n == text.length();
}

bool sdBackupNow() {
  backupDue = 0;
  if (!mounted) return false;
  String s = "# ESP iRadio settings";
  if (timeKnown()) s += ", written " + timeText(STAMP_MIN);
  s += "\n# The WiFi password is left out on purpose. A restore leaves WiFi as it is.\n";
  s += "bt_name=" + settings.btName + "\n";
  s += "volume=" + String(settings.volume) + "\n";
  s += "station=" + String(settings.lastStation) + "\n";
  s += "autoplay=" + String(settings.autoPlay ? 1 : 0) + "\n";
  s += "bt_enabled=" + String(settings.btEnabled ? 1 : 0) + "\n";
  s += "roam_dbm=" + String(settings.wifiRoamDbm) + "\n";
  s += "autoscan=" + String(settings.wifiAutoScan ? 1 : 0) + "\n";
  s += "flip=" + String(settings.displayFlip ? 1 : 0) + "\n";
  s += "scroll=" + String(settings.scrollTitles ? 1 : 0) + "\n";
  s += "line32=" + String(settings.line32 ? 1 : 0) + "\n";
  s += "battery_fitted=" + String(settings.batteryFitted ? 1 : 0) + "\n";
  s += "speaker=" + String(settings.speakerOn ? 1 : 0) + "\n";
  s += "backlight=" + String(settings.backlight) + "\n";
  s += "night=" + String(settings.nightDim ? 1 : 0) + "\n";
  s += "night_from=" + String(settings.nightFrom) + "\n";
  s += "night_to=" + String(settings.nightTo) + "\n";
  s += "night_level=" + String(settings.nightLevel) + "\n";
  s += "idle_minutes=" + String(settings.idleMinutes) + "\n";
  s += "idle_level=" + String(settings.idleLevel) + "\n";
  s += "wifi_ssid=" + settings.wifiSsid + "\n";
  bool ok = writeText(BACKUP_SETTINGS, s) && writeText(BACKUP_STATIONS, settingsStationsText());
  Log.printf("[sd] settings backup %s\n", ok ? "written" : "failed");
  return ok;
}

static String readText(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return "";
  String s = f.readString();
  f.close();
  return s;
}

String sdRestoreSettings(bool* ok) {
  *ok = false;
  if (!mounted) return "No SD card.";
  String s = readText(BACKUP_SETTINGS);
  String st = readText(BACKUP_STATIONS);
  if (s.isEmpty()) s = readText("/backup/instellingen.txt");  // Dutch names before 2026-10-11
  if (st.isEmpty()) st = readText("/backup/zenders.txt");
  if (s.isEmpty() || st.isEmpty()) return "No backup on the card.";
  std::vector<Station> stations;
  if (!settingsParseStations(st, stations)) return "stations.txt holds no valid stations.";
  int start = 0;
  while (start < (int)s.length()) {
    int end = s.indexOf('\n', start);
    if (end < 0) end = s.length();
    String line = s.substring(start, end);
    start = end + 1;
    line.trim();
    int eq = line.indexOf('=');
    if (line.startsWith("#") || eq <= 0) continue;
    String k = line.substring(0, eq), v = line.substring(eq + 1);
    k.trim();
    v.trim();
    if (k == "bt_name" && v.length()) settings.btName = v;
    else if (k == "volume") settings.volume = constrain(v.toInt(), 0, 100);
    else if (k == "station") settings.lastStation = v.toInt();
    else if (k == "autoplay") settings.autoPlay = v.toInt() != 0;
    else if (k == "bt_enabled") settings.btEnabled = v.toInt() != 0;
    else if (k == "roam_dbm") settings.wifiRoamDbm = v.toInt() == 0 ? 0 : constrain(v.toInt(), -90, -40);
    else if (k == "autoscan") settings.wifiAutoScan = v.toInt() != 0;
    else if (k == "flip") settings.displayFlip = v.toInt() != 0;
    else if (k == "scroll") settings.scrollTitles = v.toInt() != 0;
    else if (k == "line32") settings.line32 = v.toInt() != 0;
    else if (k == "battery_fitted") settings.batteryFitted = v.toInt() != 0;
    else if (k == "speaker") settings.speakerOn = v.toInt() != 0;
    else if (k == "backlight") settings.backlight = constrain(v.toInt(), 5, 100);
    else if (k == "night") settings.nightDim = v.toInt() != 0;
    else if (k == "night_from") settings.nightFrom = constrain(v.toInt(), 0, 24 * 60 - 1);
    else if (k == "night_to") settings.nightTo = constrain(v.toInt(), 0, 24 * 60 - 1);
    else if (k == "night_level") settings.nightLevel = constrain(v.toInt(), 0, 100);
    else if (k == "idle_minutes") settings.idleMinutes = constrain(v.toInt(), 0, 240);
    else if (k == "idle_level") settings.idleLevel = constrain(v.toInt(), 0, 100);
    // wifi_ssid is informational: WiFi stays as it is.
  }
  settings.stations = stations;
  if (settings.lastStation < 0 || settings.lastStation >= (int)settings.stations.size()) settings.lastStation = 0;
  settingsSave();
  Log.printf("[sd] settings restored, %u stations\n", (unsigned)stations.size());
  *ok = true;
  return "Restored: " + String(stations.size()) + " stations and the settings. The radio restarts.";
}

// ---------------------------------------------------------------- logos

static SemaphoreHandle_t logoLock = nullptr;
static uint16_t* logoPix = nullptr;  // PSRAM, up to SD_LOGO_MAX^2 pixels
static int logoSrc = 0;              // side of the loaded logo (40 or 64)
static volatile int logoGen = 0;     // 0 = no logo for the current station
static int logoCounter = 0;
static String logoFor = "\x01";      // station name the logo state belongs to (never matches at start)
static bool logoDirty = false;

static const size_t LOGO_BYTES = SD_LOGO_MAX * SD_LOGO_MAX * 2;  // 8192
static const size_t LOGO_OLD_BYTES = 40 * 40 * 2;                 // 3200, before 2026-10-10 evening

static String logoPath(const String& name) {
  uint32_t h = 2166136261u;  // FNV-1a over the UTF-8 bytes; the web page computes the same
  for (size_t i = 0; i < name.length(); i++) {
    h ^= (uint8_t)name[i];
    h *= 16777619u;
  }
  char buf[24];
  snprintf(buf, sizeof(buf), "/logos/%08x.565", (unsigned)h);
  return buf;
}

static String stationName(int i) {
  return i >= 0 && i < (int)settings.stations.size() ? settings.stations[i].name : String("");
}

bool sdLogoSave(int station, const uint8_t* data, size_t len) {
  String name = stationName(station);
  if (!mounted || name.isEmpty() || (len != LOGO_BYTES && len != LOGO_OLD_BYTES)) return false;
  File f = SD.open(logoPath(name), FILE_WRITE);
  if (!f) return false;
  bool ok = f.write(data, len) == len;
  f.close();
  logoDirty = true;
  Log.printf("[sd] logo for %s %s\n", name.c_str(), ok ? "saved" : "failed");
  return ok;
}

bool sdLogoDelete(int station) {
  String name = stationName(station);
  if (!mounted || name.isEmpty()) return false;
  bool ok = SD.remove(logoPath(name));
  logoDirty = true;
  return ok;
}

int sdLogoGeneration() { return logoGen; }

// Box filter when shrinking (64 -> 40), nearest pixel when growing (40 -> 64).
bool sdCopyLogo(uint16_t* dst, int size) {
  if (!logoLock || !logoGen || size <= 0 || size > SD_LOGO_MAX) return false;
  xSemaphoreTake(logoLock, portMAX_DELAY);
  int src = logoSrc;
  for (int y = 0; y < size; y++)
    for (int x = 0; x < size; x++) {
      int x0 = x * src / size, x1 = std::max(x0 + 1, (x + 1) * src / size);
      int y0 = y * src / size, y1 = std::max(y0 + 1, (y + 1) * src / size);
      uint32_t r = 0, g = 0, b = 0, n = 0;
      for (int sy = y0; sy < y1; sy++)
        for (int sx = x0; sx < x1; sx++) {
          uint16_t v = logoPix[sy * src + sx];
          r += v >> 11;
          g += (v >> 5) & 63;
          b += v & 31;
          n++;
        }
      dst[y * size + x] = (uint16_t)(((r / n) << 11) | ((g / n) << 5) | (b / n));
    }
  xSemaphoreGive(logoLock);
  return true;
}

// Loads the logo of the current station when the station (or the logo) changed.
static void updateLogo() {
  String name = stationName(settings.lastStation);
  if (name == logoFor && !logoDirty) return;
  logoFor = name;
  logoDirty = false;
  if (!logoLock) {
    logoLock = xSemaphoreCreateMutex();
    logoPix = (uint16_t*)heap_caps_malloc(LOGO_BYTES, MALLOC_CAP_SPIRAM);
  }
  int gen = 0;
  String path = logoPath(name);
  if (name.length() && logoPix && SD.exists(path)) {
    File f = SD.open(path, FILE_READ);
    uint8_t* tmp = (uint8_t*)malloc(LOGO_BYTES);
    size_t len = f ? f.size() : 0;
    if (f && tmp && (len == LOGO_BYTES || len == LOGO_OLD_BYTES) && f.read(tmp, len) == len) {
      xSemaphoreTake(logoLock, portMAX_DELAY);
      memcpy(logoPix, tmp, len);
      logoSrc = len == LOGO_BYTES ? SD_LOGO_MAX : 40;
      xSemaphoreGive(logoLock);
      gen = ++logoCounter;
    }
    free(tmp);
    if (f) f.close();
  }
  logoGen = gen;
}

// ---------------------------------------------------------------- firmware copies

static String fwResult;
static bool fwSaveRequested = false;
static bool fwAutoDone = false;
static String fwRestoreName;
// Copy in progress (one 4 KB chunk per sdLoop pass, so loop() keeps running).
static File fwOut;
static const esp_partition_t* fwPart = nullptr;
static uint32_t fwSize = 0, fwOff = 0;
static uint8_t* fwBuf = nullptr;  // internal RAM: flash reads must not land in PSRAM

String sdFirmwareResult() { return fwResult; }

void sdRequestFirmwareSave() {
  fwResult = "busy";
  fwSaveRequested = true;
}

void sdRequestFirmwareRestore(const String& name) {
  fwResult = "busy";
  fwRestoreName = name;
}

static String fwName() {
  String s = buildStamp;  // "2026-10-10 15:40"
  s.replace(" ", "_");
  s.replace(":", "");
  return s + ".bin";
}

static void fwFinish(bool ok, const String& msg) {
  if (fwOut) fwOut.close();
  if (fwBuf) heap_caps_free(fwBuf);
  fwBuf = nullptr;
  fwPart = nullptr;
  if (ok) {
    String final = "/firmware/" + fwName();
    SD.remove(final);
    SD.rename("/firmware/kopie.tmp", final);
    prune("/firmware", ".bin", 5);
  } else {
    SD.remove("/firmware/kopie.tmp");
  }
  fwResult = msg;
  Log.printf("[sd] firmware copy: %s\n", msg.c_str());
}

static void fwStartCopy() {
  fwPart = esp_ota_get_running_partition();
  fwSize = ESP.getSketchSize();  // the image itself, not the whole 1984 KB slot
  fwOff = 0;
  fwBuf = (uint8_t*)heap_caps_malloc(4096, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  fwOut = SD.open("/firmware/kopie.tmp", FILE_WRITE);
  if (!fwPart || !fwBuf || !fwOut || fwSize == 0 || fwSize > fwPart->size) {
    fwFinish(false, "Making the copy failed.");
    return;
  }
  Log.printf("[sd] copying the running firmware (%u bytes) to the card\n", (unsigned)fwSize);
}

static void fwCopyStep() {
  uint32_t n = std::min<uint32_t>(4096, fwSize - fwOff);
  if (esp_partition_read(fwPart, fwOff, fwBuf, n) != ESP_OK || fwOut.write(fwBuf, n) != n) {
    fwFinish(false, "Making the copy failed (read or write).");
    return;
  }
  fwOff += n;
  if (fwOff >= fwSize) fwFinish(true, "Firmware " + String(buildStamp) + " saved on the card.");
}

// Flashes a copy from the card into the other OTA slot and restarts. Blocks for ~30 s; audio
// stops first, as with an update over WiFi.
static void fwRestore(const String& name) {
  sdEvent("firmware restore from the card: %s", name.c_str());
  if (name.indexOf('/') >= 0 || !name.endsWith(".bin")) {
    fwResult = "Invalid name.";
    return;
  }
  File f = SD.open("/firmware/" + name, FILE_READ);
  if (!f || f.size() < 100000) {
    if (f) f.close();
    fwResult = "File not found.";
    return;
  }
  uint8_t* buf = (uint8_t*)heap_caps_malloc(4096, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!buf) {
    f.close();
    fwResult = "Out of memory.";
    return;
  }
  size_t size = f.size();
  Log.printf("[sd] restoring firmware %s (%u bytes)\n", name.c_str(), (unsigned)size);
  playerShutdown();
  displaySetUpdate(UpdatePhase::Receiving, 0, size, "from the SD card");
  bool ok = Update.begin(size);
  size_t done = 0;
  while (ok && f.available()) {
    size_t n = f.read(buf, 4096);
    ok = n > 0 && Update.write(buf, n) == n;
    done += n;
    displaySetUpdate(UpdatePhase::Receiving, done, size, "from the SD card");
  }
  ok = ok && Update.end(true);
  f.close();
  heap_caps_free(buf);
  if (ok) {
    Log.println("[sd] firmware restored, restarting");
    displaySetUpdate(UpdatePhase::Done, size, size, "from the SD card");
    sdEvent("firmware restored from the card, restarting");
    sdFlushEvents();
    delay(1500);  // let the display show it
    ESP.restart();
  }
  displaySetUpdate(UpdatePhase::Failed, 0, 0, "from the SD card");
  Update.abort();
  fwResult = "Restore failed: " + String(Update.errorString());
  Log.printf("[sd] %s\n", fwResult.c_str());
  playerPlay(settings.lastStation);
}

// ---------------------------------------------------------------- history: shared

// Removes files in `dir` ending in `ext` whose name starts with a date (YYYY-MM-DD or YYYY-MM)
// that sorts before `cutoff` (the same form): a year is kept whatever the gaps in between.
static void pruneBefore(const char* dir, const char* ext, const String& cutoff) {
  for (const FileEntry& e : listDir(dir)) {
    if (!e.name.endsWith(ext) || e.name.length() < cutoff.length() || !isdigit((uint8_t)e.name[0])) continue;
    if (e.name.substring(0, cutoff.length()) < cutoff) {
      SD.remove(String(dir) + "/" + e.name);
      Log.printf("[sd] removed old %s/%s\n", dir, e.name.c_str());
    }
  }
}

static String yearAgo(bool monthOnly) {
  String d = timeText(DAY, time(nullptr) - 365L * 86400);
  return monthOnly ? d.substring(0, 7) : d;
}

// ---------------------------------------------------------------- events

static const size_t EV_BYTES = 8192;
static char* evBuf = nullptr;  // PSRAM, "<millis>\t<text>\n" lines
static size_t evLen = 0;
static uint32_t evDropped = 0;
static portMUX_TYPE evMux = portMUX_INITIALIZER_UNLOCKED;
static String evMonth;

void sdEvent(const char* fmt, ...) {
  if (!evBuf) {  // first call happens at boot, before other tasks log events
    evBuf = (char*)heap_caps_malloc(EV_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!evBuf) return;
  }
  char line[200];
  int n = snprintf(line, sizeof(line), "%lu\t", (unsigned long)millis());
  size_t room = sizeof(line) - n - 1;  // keep one byte for the line end
  va_list ap;
  va_start(ap, fmt);
  int m = vsnprintf(line + n, room, fmt, ap);
  va_end(ap);
  if (m < 0) return;
  n += m < (int)room ? m : (int)room - 1;
  for (int i = 0; i < n; i++)
    if (line[i] == '\n' || line[i] == '\r') line[i] = ' ';
  line[n++] = '\n';
  portENTER_CRITICAL(&evMux);
  if (evLen + n <= EV_BYTES) {
    memcpy(evBuf + evLen, line, n);
    evLen += n;
  } else {
    evDropped++;
  }
  portEXIT_CRITICAL(&evMux);
}

// Writes the waiting events with their wall-clock time (worked out from millis() once the clock
// is set; before that "uptime N s" after a 3-minute wait).
static void writeEvents() {
  if (!evBuf || !evLen) return;
  bool known = timeKnown();
  if (!known && millis() < 180000) return;
  char* copy = (char*)malloc(EV_BYTES + 1);  // PSRAM
  if (!copy) return;
  portENTER_CRITICAL(&evMux);
  size_t len = evLen;
  memcpy(copy, evBuf, len);
  evLen = 0;
  uint32_t dropped = evDropped;
  evDropped = 0;
  portEXIT_CRITICAL(&evMux);
  copy[len] = 0;
  uint32_t nowMs = millis();
  time_t nowT = time(nullptr);
  String month = known ? timeText(DAY).substring(0, 7) : String("no-date");
  if (month != evMonth) {
    evMonth = month;
    if (known) pruneBefore("/events", ".log", yearAgo(true));
  }
  File f = SD.open("/events/" + month + ".log", FILE_APPEND);
  if (f) {
    for (char* p = copy; *p;) {
      char* end = strchr(p, '\n');
      if (!end) break;
      *end = 0;
      char* tab = strchr(p, '\t');
      if (tab) {
        *tab = 0;
        uint32_t ms = strtoul(p, nullptr, 10);
        String ts = known ? timeText(STAMP, nowT - (time_t)((nowMs - ms) / 1000)) : "uptime " + String(ms / 1000) + " s";
        f.printf("%s  %s\n", ts.c_str(), tab + 1);
      }
      p = end + 1;
    }
    if (dropped) f.printf("%s  (%lu events lost: buffer full)\n", known ? timeText(STAMP).c_str() : "?",
                          (unsigned long)dropped);
    f.close();
  }
  free(copy);
}

void sdFlushEvents() {
  if (mounted) writeEvents();
}

static const char* resetReasonText(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON: return "power on";
    case ESP_RST_EXT: return "reset pin";
    case ESP_RST_SW: return "restart";
    case ESP_RST_PANIC: return "CRASH (panic)";
    case ESP_RST_INT_WDT: return "CRASH (interrupt watchdog)";
    case ESP_RST_TASK_WDT: return "CRASH (task watchdog)";
    case ESP_RST_WDT: return "CRASH (watchdog)";
    case ESP_RST_DEEPSLEEP: return "wake from deep sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    default: return "other";
  }
}

// ---------------------------------------------------------------- station statistics

// Per station and day: minutes listened (playing, sound on an output, not muted), minutes
// streamed, bytes in, the stream bitrate, underruns, reconnects and failed connects. Counted
// once a minute from the player's running totals; the day file is rewritten every 10 minutes
// and the month file is summed from that month's day files.
struct StationStat {
  String name;
  uint32_t listenMin = 0, streamMin = 0;
  uint64_t inBytes = 0;
  uint32_t underruns = 0, reconnects = 0, fails = 0;
  int kbps = 0;
};
static std::vector<StationStat> stToday;
static String stDayName;
static bool stBase = false;
static uint32_t stLastIn = 0;
static int stLastUnder = 0, stLastRecon = 0, stLastFail = 0;
static uint32_t stLastWrite = 0;
static bool stDirty = false;

static StationStat& stFind(std::vector<StationStat>& v, const String& name) {
  for (StationStat& s : v)
    if (s.name == name) return s;
  v.push_back(StationStat());
  v.back().name = name;
  return v.back();
}

// Adds the rows of a station CSV (day or month file) to `v`.
static void stParse(const String& text, std::vector<StationStat>& v) {
  int pos = text.indexOf('\n');
  while (pos >= 0 && pos + 1 < (int)text.length()) {
    int end = text.indexOf('\n', pos + 1);
    String line = text.substring(pos + 1, end < 0 ? text.length() : end);
    pos = end;
    line.trim();
    String c[8];
    int n = 0, from = 0;
    while (n < 8) {
      int comma = line.indexOf(',', from);
      c[n++] = line.substring(from, comma < 0 ? line.length() : comma);
      if (comma < 0) break;
      from = comma + 1;
    }
    if (n < 8 || c[0].isEmpty()) continue;
    StationStat& s = stFind(v, c[0]);
    s.listenMin += c[1].toInt();
    s.streamMin += c[2].toInt();
    s.inBytes += (uint64_t)c[3].toInt() * 1024;
    if (c[4].toInt() > 0) s.kbps = c[4].toInt();
    s.underruns += c[5].toInt();
    s.reconnects += c[6].toInt();
    s.fails += c[7].toInt();
  }
}

static String stCsv(std::vector<StationStat> v) {
  std::sort(v.begin(), v.end(), [](const StationStat& a, const StationStat& b) { return a.listenMin > b.listenMin; });
  String t = "station,listen_min,stream_min,in_kB,kbps,underruns,reconnects,connect_fails\n";
  for (const StationStat& s : v)
    t += s.name + "," + String(s.listenMin) + "," + String(s.streamMin) + "," + String((uint32_t)(s.inBytes / 1024)) +
         "," + String(s.kbps) + "," + String(s.underruns) + "," + String(s.reconnects) + "," + String(s.fails) + "\n";
  return t;
}

static void stWriteDay() {
  writeText(("/stations/" + stDayName + ".csv").c_str(), stCsv(stToday));
  String month = stDayName.substring(0, 7);
  std::vector<StationStat> m;
  for (const FileEntry& e : listDir("/stations"))
    if (e.name.startsWith(month) && e.name.endsWith(".csv")) stParse(readText(("/stations/" + e.name).c_str()), m);
  writeText(("/stations/months/" + month + ".csv").c_str(), stCsv(m));
  stDirty = false;
  stLastWrite = millis();
}

static void stMinute() {
  if (!timeKnown()) return;
  String day = timeText(DAY);
  if (day != stDayName) {
    if (stDayName.length() && stDirty) stWriteDay();  // close the previous day
    stToday.clear();
    stDayName = day;
    stParse(readText(("/stations/" + day + ".csv").c_str()), stToday);  // continue after a restart
    pruneBefore("/stations", ".csv", yearAgo(false));
    pruneBefore("/stations/months", ".csv", yearAgo(true));
  }
  PlayerStatus p = playerStatus();
  uint32_t dIn = p.inBytes - stLastIn;
  int dU = p.underruns - stLastUnder, dR = p.reconnects - stLastRecon, dF = p.connectFails - stLastFail;
  stLastIn = p.inBytes;
  stLastUnder = p.underruns;
  stLastRecon = p.reconnects;
  stLastFail = p.connectFails;
  if (!stBase) {  // the first minute only sets the baseline
    stBase = true;
    return;
  }
  if (p.stationName.isEmpty()) return;  // stopped
  String name = p.stationName;
  name.replace(",", ";");  // keep the CSV plain
  StationStat& s = stFind(stToday, name);
  if (p.state == PlayState::Playing || p.state == PlayState::Buffering) s.streamMin++;
  if (p.state == PlayState::Playing && (p.btConnected || p.lineOut) && !p.muted) s.listenMin++;
  s.inBytes += dIn;
  if (dU > 0) s.underruns += dU;
  if (dR > 0) s.reconnects += dR;
  if (dF > 0) s.fails += dF;
  if (p.bitrateKbps > 0) s.kbps = p.bitrateKbps;
  stDirty = true;
  if (millis() - stLastWrite > 600000) stWriteDay();
}

// ---------------------------------------------------------------- crash dumps

// After a crash ESP-IDF leaves an ELF core dump in the coredump partition. The next boot copies
// it to /crash (once the clock is set, or after 3 minutes), writes its summary (task, PC,
// backtrace) to the events and erases it from flash.
static bool crashChecked = false;

static void saveCrashDump() {
  crashChecked = true;
#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH
  size_t addr = 0, size = 0;
  if (esp_core_dump_image_get(&addr, &size) != ESP_OK || size == 0) return;
  const esp_partition_t* part =
      esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, nullptr);
  if (!part || addr < part->address) return;
  String stamp = timeKnown() ? timeText(STAMP) : String("no-date ") + String(millis() / 1000);
  stamp.replace(" ", "_");
  stamp.replace(":", "");
  String path = "/crash/" + stamp + ".elf";
  File f = SD.open(path, FILE_WRITE);
  uint8_t* buf = (uint8_t*)heap_caps_malloc(4096, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  bool ok = f && buf;
  for (size_t off = 0; ok && off < size; off += 4096) {
    size_t n = size - off < 4096 ? size - off : 4096;
    ok = esp_partition_read(part, addr - part->address + off, buf, n) == ESP_OK && f.write(buf, n) == n;
  }
  if (buf) free(buf);
  if (f) f.close();
  String summary = "no summary";
#if CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF
  esp_core_dump_summary_t* sum = (esp_core_dump_summary_t*)malloc(sizeof(esp_core_dump_summary_t));
  if (sum && esp_core_dump_get_summary(sum) == ESP_OK) {
    char pc[12];
    snprintf(pc, sizeof(pc), "0x%08lx", (unsigned long)sum->exc_pc);
    summary = "task " + String(sum->exc_task) + ", PC " + String(pc) + ", cause " + String(sum->ex_info.exc_cause) +
              ", backtrace";
    for (uint32_t i = 0; i < sum->exc_bt_info.depth && i < 8; i++) {
      snprintf(pc, sizeof(pc), " 0x%08lx", (unsigned long)sum->exc_bt_info.bt[i]);
      summary += pc;
    }
    summary += ", elf " + String((const char*)sum->app_elf_sha256).substring(0, 8);
  }
  if (sum) free(sum);
#endif
  if (ok) {
    esp_core_dump_image_erase();
    sdEvent("crash dump saved: %s (%u KB): %s", path.c_str(), (unsigned)(size / 1024), summary.c_str());
    Log.printf("[sd] crash dump saved: %s (%u bytes): %s\n", path.c_str(), (unsigned)size, summary.c_str());
    pruneBefore("/crash", ".elf", yearAgo(false));
  } else {
    SD.remove(path);
    Log.println("[sd] crash dump could not be copied");
  }
#endif
}

// ---------------------------------------------------------------- listing

static String jsonEsc(const String& s) {
  String o;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') o += '\\';
    if ((uint8_t)c >= 0x20) o += c;
  }
  return o;
}

static String filesJson(const char* dir, const char* ext) {
  String j = "[";
  for (const FileEntry& e : listDir(dir)) {
    if (!e.name.endsWith(ext)) continue;
    if (j.length() > 1) j += ",";
    j += "{\"n\":\"" + jsonEsc(e.name) + "\",\"s\":" + String(e.size) + "}";
  }
  return j + "]";
}

String sdListJson() {
  String j = "{\"mounted\":" + String(mounted ? "true" : "false") + ",\"info\":\"" + jsonEsc(info) + "\"";
  j += ",\"time\":" + String(timeKnown() ? "true" : "false");
  if (mounted) {
    j += ",\"log\":" + filesJson("/log", ".log");
    j += ",\"stats\":" + filesJson("/stats", ".csv");
    j += ",\"fw\":" + filesJson("/firmware", ".bin");
    j += ",\"events\":" + filesJson("/events", ".log");
    j += ",\"stdays\":" + filesJson("/stations", ".csv");
    j += ",\"months\":" + filesJson("/stations/months", ".csv");
    j += ",\"crash\":" + filesJson("/crash", ".elf");
    j += ",\"current\":\"" + fwName() + "\"";
    File b = SD.open(BACKUP_SETTINGS);
    j += ",\"backup\":" + String(b ? "true" : "false");
    if (b) {
      time_t t = b.getLastWrite();
      b.close();
      if (t > 1700000000) j += ",\"backupTime\":\"" + timeText(STAMP_MIN, t) + "\"";
    }
    // Stations that have a logo (index list), from one directory listing.
    std::vector<FileEntry> logos = listDir("/logos");
    j += ",\"logos\":[";
    bool first = true;
    for (size_t i = 0; i < settings.stations.size(); i++) {
      String p = logoPath(settings.stations[i].name).substring(7);  // without "/logos/"
      for (const FileEntry& e : logos)
        if (e.name == p) {
          j += String(first ? "" : ",") + String(i);
          first = false;
          break;
        }
    }
    j += "]";
  }
  return j + "}";
}

bool sdPathAllowed(const String& p) {
  if (!mounted || p.indexOf("..") >= 0) return false;
  return p.startsWith("/log/") || p.startsWith("/stats/") || p.startsWith("/backup/") ||
         p.startsWith("/logos/") || p.startsWith("/firmware/") || p.startsWith("/events/") ||
         p.startsWith("/stations/") || p.startsWith("/crash/");
}

// ---------------------------------------------------------------- loop

void sdLoop() {
  if (formatRequested) {
    formatRequested = false;
    doFormat();
    logoDirty = true;
  }
  if (!mounted) return;

  if (fwRestoreName.length()) {
    String n = fwRestoreName;
    fwRestoreName = "";
    fwRestore(n);  // restarts on success
    return;
  }
  if (fwPart) {  // copy in progress
    fwCopyStep();
    return;
  }
  // Keep a copy of every firmware that ran 10 minutes (it booted, played and did not crash).
  if (!fwAutoDone && millis() > 600000) {
    fwAutoDone = true;
    if (!SD.exists("/firmware/" + fwName())) fwSaveRequested = true;
  }
  if (fwSaveRequested) {
    fwSaveRequested = false;
    fwStartCopy();
    return;
  }

  uint32_t now = millis();
  if (backupDue && (int32_t)(now - backupDue) >= 0) sdBackupNow();

  static uint32_t lastLogo = 0;
  if (now - lastLogo > 500) {
    lastLogo = now;
    updateLogo();
  }
  static uint32_t lastLog = 0;
  if (now - lastLog > 30000) {
    lastLog = now;
    writeLog();
    writeEvents();
  }
  static uint32_t lastStats = 0;
  if (now - lastStats > 60000) {
    lastStats = now;
    writeStats();
    stMinute();
  }
  if (!crashChecked && (timeKnown() || now > 180000)) saveCrashDump();
}
