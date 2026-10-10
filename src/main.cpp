// ESP32 internet radio: WiFi stream -> Bluetooth A2DP headphones, e-paper status, web config.
#include <Arduino.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_sleep.h>
#include <esp_system.h>

#include "battery.h"
#include "config.h"
#include "display.h"
#include "memlog.h"
#include "player.h"
#include "power.h"
#include "settings.h"
#include "sdcard.h"
#include "web.h"
#include "weblog.h"

static bool setupMode = false;
static String shownIp;
static int shownRssi = 0;  // WiFi signal as last drawn on the e-paper

static uint8_t lastBssid[6];
static int lastChannel = 0;
static int sameApMisses = 0;             // same-AP connects that skipped the scan
static bool lastConnectSkippedScan = false;
static bool mdnsStarted = false;
// Settle check: one roam scan a minute after each successful connect (see loop()).
static bool settleRoamPending = false;
static uint32_t settleRoamAt = 0;
// Hop chosen on the web page ("Switch to this access point"), done in loop().
static uint8_t pendingRoamBssid[6];
static int pendingRoamChannel = 0;

// Remember the associated access point so the next connect can skip a scan.
static void rememberAp() {
  if (WiFi.status() != WL_CONNECTED || WiFi.BSSID() == nullptr) return;
  if (memcmp(lastBssid, WiFi.BSSID(), 6) == 0 && lastChannel == WiFi.channel()) return;
  memcpy(lastBssid, WiFi.BSSID(), 6);
  lastChannel = WiFi.channel();
  Preferences p;
  if (p.begin("radio", false)) {
    p.putBytes("bssid", lastBssid, 6);
    p.putInt("chan", lastChannel);
    p.end();
  }
}

// sameAp: reconnect straight to the access point used before (no scan burst, which
// disturbs a fresh Bluetooth link). The all-channel fallback is skipped while the
// headphones are not connected, for a few tries: that scan takes the radio the
// page needs. After that, one scan is allowed so a moved access point does not
// leave the radio unreachable. No remembered AP yet: the scan is the only way on.
// A fresh A2DP link is fragile until the media stream runs: WiFi's connect burst in
// that gap dropped the link with disc_rsn=1 three times in a row (2026-10-09). Wait
// until the Bluetooth stack pulls audio (plus 1 s), at most maxMs.
static void waitForBtAudio(uint32_t maxMs) {
  uint32_t start = millis();
  while (millis() - start < maxMs && !playerBtAudioActive()) delay(100);
  bool active = playerBtAudioActive();
  if (active) delay(1000);
  Log.printf("[bt] audio %s after %lu ms\n", active ? "running" : "not started",
                (unsigned long)(millis() - start));
}

static bool connectWifi(bool sameAp = false) {
  if (settings.wifiSsid.isEmpty()) return false;
  lastConnectSkippedScan = false;
  playerResetStreamFailures();
  Log.printf("[wifi] connecting to %s%s\n", settings.wifiSsid.c_str(), sameAp ? " (same AP)" : "");
  WiFi.useStaticBuffers(false);  // 4 instead of 8 static RX buffers: internal RAM is scarce
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(MDNS_NAME);
  WiFi.setAutoReconnect(true);
  // Mesh networks: several access points share the SSID. When a scan does run,
  // look at all channels and pick the strongest instead of the first one found.
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
  bool triedSame = false;
  if (sameAp && lastChannel > 0) {
    triedSame = true;
    WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPass.c_str(), lastChannel, lastBssid);
    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) delay(500);
  }
  bool headphonesUp = playerBtConnected();
  bool skipScan = triedSame && !headphonesUp && sameApMisses < 3;
  if (WiFi.status() != WL_CONNECTED && !skipScan) {
    WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPass.c_str());  // also clears any BSSID lock
    for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) delay(500);
  } else if (skipScan && WiFi.status() != WL_CONNECTED) {
    lastConnectSkippedScan = true;
    sameApMisses++;
    Log.printf("[wifi] same AP failed, scan skipped (%d)\n", sameApMisses);
  }
  if (WiFi.status() != WL_CONNECTED) return false;
  sameApMisses = 0;
  rememberAp();
  static bool clockStarted = false;
  if (!clockStarted) {  // SNTP keeps the clock from here on (SD card file names and log stamps)
    clockStarted = true;
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "nl.pool.ntp.org", "pool.ntp.org", "fritz.box");
  }
  settleRoamPending = true;
  settleRoamAt = millis() + 60000;
  // Modem sleep must stay on (Bluetooth coexistence). WIFI_PS_MAX_MODEM was tried:
  // fine at -56 dBm, but at -72 dBm the link became unusable (75 % packet loss,
  // stream could not connect). MIN_MODEM is the sturdier choice.
  WiFi.setSleep(WIFI_PS_MIN_MODEM);
  return true;
}

// Headphones are connected and the signal has stayed under the threshold.
// One scan of our SSID. Switch only when another mesh point is clearly stronger
// (12 dB): a smaller gap is scan noise and would flap. The scan itself crackles
// the audio for a few seconds. Returns true when the association moved.
static bool hopTo(const uint8_t* bssid, int channel);

static bool roamIfBetter() {
  if (WiFi.status() != WL_CONNECTED || WiFi.BSSID() == nullptr) return false;
  int current = WiFi.RSSI();
  uint8_t currentBssid[6];
  memcpy(currentBssid, WiFi.BSSID(), 6);
  int n = WiFi.scanNetworks(false, false, false, 300, 0, settings.wifiSsid.c_str());
  int best = -127;
  int bestIdx = -1;
  for (int i = 0; i < n; i++) {
    if ((int)WiFi.RSSI(i) > best) {
      best = WiFi.RSSI(i);
      bestIdx = i;
    }
  }
  uint8_t bestBssid[6] = {};
  int bestChannel = 0;
  if (bestIdx >= 0 && WiFi.BSSID(bestIdx) != nullptr) {
    memcpy(bestBssid, WiFi.BSSID(bestIdx), 6);
    bestChannel = WiFi.channel(bestIdx);
  }
  WiFi.scanDelete();
  bool same = bestIdx >= 0 && memcmp(bestBssid, currentBssid, 6) == 0;
  Log.printf("[wifi] roam check: current %d dBm, best %d dBm (%d APs)\n", current, best, n);
  if (bestIdx < 0 || same || bestChannel <= 0 || best < current + 12) return false;

  Log.printf("[wifi] roaming to a stronger access point (%d dBm, channel %d)\n", best, bestChannel);
  return hopTo(bestBssid, bestChannel);
}

void wifiRequestRoam(const uint8_t bssid[6], int channel) {
  memcpy(pendingRoamBssid, bssid, 6);
  pendingRoamChannel = channel;
}

// Associate with one access point of our SSID; falls back to the previous one when
// that fails. Returns true when the association moved.
static bool hopTo(const uint8_t* bssid, int channel) {
  if (WiFi.status() != WL_CONNECTED || WiFi.BSSID() == nullptr) return false;
  uint8_t currentBssid[6];
  memcpy(currentBssid, WiFi.BSSID(), 6);
  if (memcmp(currentBssid, bssid, 6) == 0) return false;
  uint8_t target[6];
  memcpy(target, bssid, 6);  // bssid may point into WiFi's own state
  WiFi.disconnect(false, false);
  delay(100);
  WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPass.c_str(), channel, target);
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) delay(500);
  if (WiFi.status() != WL_CONNECTED) {
    Log.println("[wifi] roam failed, returning to the previous access point");
    if (lastChannel > 0)
      WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPass.c_str(), lastChannel, lastBssid);
    else
      WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPass.c_str());
    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) delay(500);
  }
  WiFi.setSleep(WIFI_PS_MIN_MODEM);  // begin() must not leave modem sleep off
  if (WiFi.status() == WL_CONNECTED) {
    rememberAp();
    Log.printf("[wifi] roam done, %d dBm\n", WiFi.RSSI());
    return memcmp(currentBssid, WiFi.BSSID(), 6) != 0;
  }
  return false;
}

static void startMdns() {
  if (mdnsStarted) return;
  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    mdnsStarted = true;
  }
}

static void startSetupAp();

// WiFi did not come up. A skipped scan means Bluetooth still needs the radio:
// drop the station interface and let the paging window try the same AP later.
// A scan that ran and failed means the credentials or the network are really gone.
static void wifiConnectFailed() {
  if (lastConnectSkippedScan) {
    Log.println("[wifi] staying off so Bluetooth can page; same AP is retried later");
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
    mdnsStarted = false;  // begin again once the station interface is back
    return;
  }
  Log.println("[wifi] cannot reach the configured network, starting setup access point");
  startSetupAp();
}

static void startSetupAp() {
  setupMode = true;
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) { Log.println("[wifi] client joined setup AP"); },
               ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SETUP_AP_NAME);  // open network, only used for the first configuration
  Log.printf("[wifi] setup AP '%s' at %s\n", SETUP_AP_NAME, WiFi.softAPIP().toString().c_str());
  displaySetNetwork("WiFi: " SETUP_AP_NAME "  ->  " + WiFi.softAPIP().toString());
}

// Power guard, run before the radios start (their start-up is the biggest current
// spike). Two situations seen in practice:
//  - Battery empty after a long run: every boot browns out when Bluetooth starts and
//    the board restarts every 10 s (flashing the e-paper each time). On battery only,
//    sleep deeply and re-check later instead.
//  - Charging an empty battery from a weak USB source (PC port, 500 mA): charger +
//    radios exceed the budget -> brownout loop. After a brownout reset, wait with the
//    radios so the charger can fill the cell a little first.
static RTC_DATA_ATTR int brownouts = 0;

int powerBrownouts() { return brownouts; }

static void powerGuard() {
  BatteryInfo b = batteryRead();
  bool brownout = esp_reset_reason() == ESP_RST_BROWNOUT;
  if (brownout) brownouts++; else brownouts = 0;
  Log.printf("[power] reset reason %d, brownouts in a row %d, battery %d mV, usb %d\n",
                (int)esp_reset_reason(), brownouts, b.millivolts, b.usb);

  if (!b.usb && b.valid && b.millivolts < 3450) {
    Log.println("[power] battery empty, deep sleep 15 min");
    displaySetNetwork("Battery empty - charge it");
    delay(20000);  // let the display task draw the message (15 s refresh)
    esp_sleep_enable_timer_wakeup(15ULL * 60 * 1000000);
    esp_deep_sleep_start();
  }
  // On USB with a deeply discharged cell: charge first, radios later. Starting them
  // now would brown out (seen at 3.2 V on a PC port), and every brownout is a hard
  // reset plus a 15 s e-paper flash.
  if (b.usb && b.valid && b.millivolts < 3500) {
    Log.println("[power] battery low while charging: waiting until it reaches 3.55 V");
    for (int i = 0; i < 120 && b.millivolts < 3550; i++) {  // at most 60 min
      displaySetNetwork("Charging battery, please wait: " + String(b.millivolts / 1000.0f, 2) + " V");
      for (int k = 0; k < 30; k++) delay(1000);
      b = batteryRead();
      Log.printf("[power] charging, battery %d mV\n", b.millivolts);
    }
  } else if (brownout) {
    int wait = min(brownouts, 3) * 60;  // 1, 2, 3 minutes
    Log.printf("[power] brownout reset: waiting %d s before starting the radios\n", wait);
    displaySetNetwork("Waiting after a brownout (" + String(wait / 60) + " min)");
    for (int i = 0; i < wait; i++) delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  weblogBegin();  // from here on every log line is also kept for the web page
  delay(200);
  Log.printf("\n[boot] ESP iRadio, PSRAM %u kB\n", ESP.getPsramSize() / 1024);

  // The prebuilt Arduino core keeps every malloc() below 4 KB in internal RAM, so
  // Bluedroid (~90 KB) and WiFi/lwIP (~55 KB) exhaust the ~180 KB internal heap.
  // Lower that limit at runtime: allocations above 256 bytes now prefer PSRAM
  // (what squeezelite-esp32 does at build time with SPIRAM_MALLOC_ALWAYSINTERNAL).
  // DMA and task-stack allocations ask for internal memory explicitly, so they are
  // unaffected. Must run before the Bluetooth and WiFi stacks are started.
  heap_caps_malloc_extmem_enable(256);
  // Note: esp_coex_preference_set(ESP_COEX_PREFER_BT) was tried here; with it the
  // headphones could no longer connect at all. Leave the coexistence at BALANCE.

  logMem("boot");
  settingsLoad();
  if (settings.btIdentity > 0) {
    // Optional address override from /api/btidentity, not from "Erase all pairings".
    // Headphones that still hold a link key for the previous address refuse a new
    // one until that device is removed on their side. IDF 4.4 can only change the
    // base MAC, so the WiFi MAC changes along (new DHCP lease). Must run before
    // WiFi and Bluetooth are initialised.
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    mac[0] |= 0x02;  // locally administered
    mac[5] += settings.btIdentity;
    esp_base_mac_addr_set(mac);
    Log.printf("[bt] identity %d, base MAC %02X:%02X:%02X:%02X:%02X:%02X\n", settings.btIdentity,
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
  batteryBegin();
  displayBegin();
  displaySetFlip(settings.displayFlip);
  sdBegin();
  powerGuard();
  logMem("display");

  // Bluetooth first, then WiFi. Starting WiFi first (tried 2026-10-07) made the
  // stream starve to 1-6 KB/s as soon as A2DP audio ran - the WiFi/Bluetooth
  // coexistence apparently initialises differently.
  // WiFi and Bluetooth share one radio. An inquiry is 12.8 s; the page after it
  // (or a direct reconnect to the stored headphones) needs the radio until it
  // finishes. WiFi stays off until then. The cap is so missing headphones do not
  // hold the network off. Bluetooth discovery starves the soft AP, so in setup
  // mode (no credentials yet) Bluetooth stays off.
  if (settings.wifiSsid.isEmpty()) {
    Log.println("[wifi] no network configured, starting setup access point");
    startSetupAp();
  } else {
    playerBegin(settings.btName.c_str(), settings.volume);
    logMem("player");
    uint32_t btWaitStart = millis();
    bool connected = false;
    // Automatic device choice pages each device in turn (one per 10 s heartbeat): allow
    // 12 s more per extra candidate.
    uint32_t btWaitMax = 36000 + 12000 * (playerBtCandidates() - 1);
    while (settings.btEnabled && millis() - btWaitStart < btWaitMax) {
      if (playerBtConnected()) { connected = true; break; }
      if (playerBtPageDone()) break;  // first page ended without a link
      // No page yet: the first inquiry (12.8 s) is over and found nothing.
      if (!playerBtPageStarted() && millis() - btWaitStart > 18000) break;
      delay(100);
    }
    Log.printf("[bt] boot wait %lu ms, %s\n", (unsigned long)(millis() - btWaitStart),
                  connected ? "connected" : "not connected");
    if (connected) waitForBtAudio(15000);  // let the A2DP stream start before WiFi
    {
      Preferences p;  // last access point: connect without a scan burst
      if (p.begin("radio", true)) {
        if (p.getBytesLength("bssid") == 6) p.getBytes("bssid", lastBssid, 6);
        lastChannel = p.getInt("chan", 0);
        p.end();
      }
    }
    if (connectWifi(true)) {
      shownIp = WiFi.localIP().toString();
      Log.printf("[wifi] connected, IP %s\n", shownIp.c_str());
      startMdns();
      shownRssi = WiFi.RSSI();
      displaySetNetwork(shownIp + "  WiFi " + String(shownRssi) + " dBm");
    } else {
      wifiConnectFailed();
    }
    logMem("wifi");
  }
  webBegin(setupMode);

  if (!setupMode && settings.autoPlay) playerPlay(settings.lastStation);
}

void loop() {
  webLoop();
  playerLoop();  // remembers a Bluetooth device that just connected

  // Battery: read every 30 s; the display text shows the voltage and the percentage.
  static uint32_t lastBattery = 0;
  static String shownPower;
  if (millis() - lastBattery > 30000 || lastBattery == 0) {
    lastBattery = millis();
    BatteryInfo b = batteryRead();
    String text;
    if (b.valid) text = String(b.millivolts / 1000.0f, 1) + "V " + String(b.percent) + "%";
    if (b.usb) text += text.length() ? " USB" : "USB";
    Log.printf("[battery] %d mV, %d %%, usb %d (pins: bat %d mV, pwr %d mV)\n", b.millivolts, b.percent,
                  b.usb, b.rawBatMv, b.rawPwrMv);
    if (text != shownPower) {
      shownPower = text;
      displaySetPower(text);
    }
  }

  // Bluetooth paging window: give Bluetooth the whole radio for 30 s (WiFi off)
  // while the headphones are still not connected. A failed inquiry never
  // increments the disconnect counter, so this does not wait for 3 DISCONNECTED
  // events. "Try again" opens a window at once. Nothing is wiped.
  // Automatic windows stay 45 s apart. The stream reconnects once WiFi is back.
  static uint32_t wifiOffAt = 0, windowEnd = 0, downSince = 0;
  if (!setupMode) {
    int failures = playerBtFailures();
    bool btUp = playerBtConnected();
    if (btUp) downSince = 0;
    else if (downSince == 0) downSince = millis();
    if (!playerBluetoothEnabled() || webUploadActive()) downSince = 0;  // Bluetooth off or an update running: no window
    bool manual = (wifiOffAt == 0 && !btUp && !webUploadActive()) ? playerTakeQuietPage() : false;
    bool stillDown = downSince != 0 && (millis() - downSince >= 15000 || failures >= 3);
    if (wifiOffAt == 0 && !btUp && (manual || (stillDown && millis() - windowEnd > 45000))) {
      Log.printf("[wifi] off for 30 s: Bluetooth paging window (%s)\n",
                    manual ? "requested" : "not connected");
      WiFi.disconnect(true, false);
      WiFi.mode(WIFI_OFF);
      mdnsStarted = false;  // begin again once WiFi returns
      wifiOffAt = millis();
    } else if (wifiOffAt && (btUp || millis() - wifiOffAt > 30000)) {
      wifiOffAt = 0;
      windowEnd = millis();
      Log.printf("[wifi] back on (%s)\n", btUp ? "headphones connected" : "window over");
      if (btUp) waitForBtAudio(15000);  // let the A2DP stream start before WiFi's connect burst
      if (!connectWifi(true)) wifiConnectFailed();
    }
    if (wifiOffAt) {
      delay(50);
      return;  // nothing else to do without WiFi
    }
  }

  // WiFi left off after a failed reconnect (kept off so Bluetooth could page):
  // retry every 20 s regardless of Bluetooth. Without this a connected link (no
  // more paging windows) left WiFi off for good: silence, page unreachable
  // (seen 2026-10-09).
  static uint32_t wifiRetryAt = 0, wifiLogAt = 0;
  bool wifiUp = WiFi.status() == WL_CONNECTED && WiFi.localIP()[0] != 0;
  // "Zombie" WiFi: the driver still reports an association, but the stream cannot
  // reach anything (errno 118, host unreachable) several times in a row. Seen
  // 2026-10-09 right after an A2DP link drop; the old retry never fired because
  // status() stayed WL_CONNECTED. Force a fresh association then.
  bool zombie = wifiUp && playerStreamFailures() >= 3;
  if (!setupMode && wifiOffAt == 0 && millis() - wifiLogAt > 10000) {
    wifiLogAt = millis();
    Log.printf("[wifi] status %d, ip %s, rssi %d, stream failures %d\n", (int)WiFi.status(),
                  WiFi.localIP().toString().c_str(), wifiUp ? WiFi.RSSI() : 0, playerStreamFailures());
  }
  if (!setupMode && wifiOffAt == 0 && (!wifiUp || zombie) && (int32_t)(millis() - wifiRetryAt) > 0) {  // wrap-safe
    wifiRetryAt = millis() + 20000;
    Log.printf("[wifi] %s, reconnecting\n", zombie ? "associated but unreachable" : "not connected");
    if (zombie) {
      WiFi.disconnect(true, false);
      delay(200);
    }
    if (!connectWifi(true)) wifiConnectFailed();
  }

  // Mesh roam. RSSI is checked every 30 s. After it has stayed under the
  // configured threshold twice, one scan runs. Only while the headphones are
  // connected: a scan during pairing takes the radio the page needs. A scan
  // that finds nothing better waits 5 min; a successful hop waits 2 min.
  static uint32_t lastRoamCheck = 0;
  static uint32_t nextRoamScan = 0;
  static int weakStreak = 0;
  if (!setupMode && settings.wifiRoamDbm < 0 && WiFi.status() == WL_CONNECTED &&
      millis() - lastRoamCheck > 30000) {
    lastRoamCheck = millis();
    if ((playerBtConnected() || !playerBluetoothEnabled()) && WiFi.RSSI() < settings.wifiRoamDbm) weakStreak++;
    else weakStreak = 0;
    if (weakStreak >= 2 && (int32_t)(millis() - nextRoamScan) >= 0) {
      weakStreak = 0;
      bool moved = roamIfBetter();
      nextRoamScan = millis() + (moved ? 120000 : 300000);
    }
  }

  // Settle check. WiFi reconnects to the remembered access point without a scan, so
  // after the radio has been moved it can stay on a far mesh point whose signal never
  // drops under the threshold (seen 2026-10-10: -58..-66 dBm on one point, -46 on
  // another). A minute after each connect, once the headphones are up (or Bluetooth is
  // off), one roam scan runs; at most every 30 min, since it crackles the audio.
  static uint32_t lastSettleScan = 0;
  static bool settleScanned = false;
  if (settleRoamPending && !setupMode && WiFi.status() == WL_CONNECTED &&
      (int32_t)(millis() - settleRoamAt) >= 0 &&
      (playerBtConnected() || !playerBluetoothEnabled())) {
    settleRoamPending = false;
    if (settings.wifiRoamDbm < 0 && (!settleScanned || millis() - lastSettleScan > 1800000)) {
      settleScanned = true;
      lastSettleScan = millis();
      Log.println("[wifi] settle check after connect");
      bool moved = roamIfBetter();
      nextRoamScan = millis() + (moved ? 120000 : 300000);
    }
  }

  sdLoop();  // SD format requested on the web page

  // Hop chosen on the web page (scan list, "Switch to this access point").
  if (pendingRoamChannel > 0 && !setupMode) {
    int ch = pendingRoamChannel;
    pendingRoamChannel = 0;
    delay(300);  // let the HTTP answer go out first
    Log.printf("[wifi] hop requested from the web page (channel %d)\n", ch);
    if (hopTo(pendingRoamBssid, ch)) nextRoamScan = millis() + 120000;
    else Log.println("[wifi] hop not done (same point or not connected)");
  }

  // Keep the network line on the display current: IP and WiFi signal (checked every 5 s,
  // redrawn when the signal moved 3 dB or more; the TFT redraws without flicker).
  static uint32_t lastCheck = 0;
  if (!setupMode && millis() - lastCheck > 5000) {
    lastCheck = millis();
    bool up = WiFi.status() == WL_CONNECTED;
    String ip = up ? WiFi.localIP().toString() : String("");
    int rssi = up ? WiFi.RSSI() : 0;
    int delta = abs(rssi - shownRssi);
    bool rssiDue = up && (delta >= 3 || shownRssi == 0);
    if (ip != shownIp || rssiDue) {
      shownIp = ip;
      shownRssi = rssi;
      displaySetNetwork(ip.length() ? ip + "  WiFi " + String(rssi) + " dBm" : String("WiFi connection lost"));
    }
    if (ip.length()) startMdns();  // WiFi may have come up only after the boot page
  }
  delay(2);
}
