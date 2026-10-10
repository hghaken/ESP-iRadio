// Audio pipeline: HTTP stream -> PSRAM buffer -> AAC/MP3 decoder -> Bluetooth A2DP.
#pragma once
#include <Arduino.h>
#include <vector>

enum class PlayState { Stopped, Connecting, Buffering, Playing, Error };

struct PlayerStatus {
  PlayState state = PlayState::Stopped;
  bool btConnected = false;
  bool btEnabled = true;   // Bluetooth switched on (web toggle)
  int station = -1;  // index into settings.stations
  String stationName;
  String title;      // ICY StreamTitle
  String codec;      // "AAC" / "MP3"
  String error;
  int bufferPercent = 0;
  int sampleRate = 0;
  // Diagnostics (rates are averages over the last 10 s).
  int inKBps = 0;        // compressed stream input
  int btKBps = 0;        // PCM pulled by Bluetooth (176 = real time)
  int silenceKBps = 0;   // PCM padded with silence (buffer was empty)
  int bufferKB = 0;
  int underruns = 0;     // decoder found the net buffer empty
  int reconnects = 0;    // stream stalls -> reconnect
  int pcmBufferKB = 0;
  int bitrateKbps = 0;     // stream bitrate (icy-br), 0 if unknown
  int btRssiDelta = 127;
  int btBonds = -1;        // Bluetooth bonds stored on the radio (-1 = stack not running)
  bool muted = false;      // play/pause on the speaker or headphones (or the web page)
  int btRole = -1;         // link role: 0 = the radio is master, 1 = the peer is master, -1 unknown   // Bluetooth link RSSI relative to the golden range (dB); 127 = unknown
};

// Starts Bluetooth (non-blocking) and the audio tasks.
void playerBegin(const char* btName, int volumePercent);
void playerPlay(int stationIndex);
void playerStop();
void playerSetVolume(int percent);
// Drops the stored headphone address and all Bluetooth bonds on this radio only.
// The Bluetooth address (btIdentity) stays. The headphones still remember the old
// link key until the user removes "ESP iRadio" on their side. After a restart the
// radio discovers and pairs afresh (headphones must be in pairing mode, one slot free).
void playerForgetHeadphones();
// Next loop turns WiFi off for one paging window and pages the stored headphones.
// Does not clear bonds, the stored address, or btIdentity, and does not restart.
void playerRequestQuietPage();
bool playerTakeQuietPage();
// Source-profile disconnect, then wait until the link is down, before a planned
// restart. The library disconnect() calls the sink API and does not drop this link.
void playerShutdown();
// During a firmware update: no paging at all (auto-reconnect off). false resumes paging the
// device list as at boot (only when Bluetooth is switched on).
void playerHoldBluetooth(bool hold);
// Web toggle. Off: auto-reconnect off and the source link dropped, so no paging and
// no paging windows. On: the stack is started if it never was, then one quiet page.
void playerSetBluetoothEnabled(bool on);
bool playerBluetoothEnabled();
// Consecutive failed Bluetooth connection attempts (0 once connected).
// A failed inquiry does not increment this: inquiry never raises DISCONNECTED.
int playerBtFailures();
// Consecutive stream connection failures (0 after a successful connect). main.cpp uses
// this to detect a WiFi that claims to be associated but has no working route.
int playerStreamFailures();
bool playerBtAudioActive();  // the Bluetooth stack pulled audio in the last 2 s
// Cheap link check for hot paths (loop() runs hundreds of times a second): no
// status copy, no Bluedroid call. Same answer as playerStatus().btConnected.
bool playerBtConnected();
void playerResetStreamFailures();  // call on every fresh WiFi association
// True once a page (CONNECTING) has been seen since boot.
bool playerBtPageStarted();
// True once that first page has ended (connected, or disconnected after a page).
// Stays false during the opening inquiry, which raises no connection event.
bool playerBtPageDone();
PlayerStatus playerStatus();
// Bluetooth device names seen during discovery (for the web page).
std::vector<String> playerSeenDevices();

// Remembered Bluetooth devices (headphones, speakers; up to 4, NVS namespace `btdev`), in
// order. Manual mode pages the top one; automatic mode goes down the list. A device is added
// when it connects (manual: on top, automatic: last). Switching or pairing a new one is done with a restart, which reuses the
// proven boot path (Bluetooth first, WiFi held off until the page is done).
struct BtDeviceInfo {
  String name, addr;
  bool selected;   // manual mode: the device the radio pages
  bool connected;  // the link is up to this one now
};
// Automatic mode (NVS `btdev`/`auto`): page the devices in list order, top first; a failed
// page (or a dropped link) moves on to the next, round and round, until one connects.
bool playerBtAuto();
void playerBtSetAuto(bool on);
bool playerBtMoveUp(int index);       // one place higher in the order
int playerBtCandidates();             // devices the boot wait must give a page (>= 1)
std::vector<BtDeviceInfo> playerBtDevices();
bool playerBtUseDevice(int index);    // move it to the top (manual mode uses the top one); caller restarts
void playerBtPairNew();               // no selection, library address cleared; caller restarts
bool playerBtForgetDevice(int index); // bond and entry removed; true when the caller should restart
void playerLoop();                    // call from loop(): stores a device that just connected,
                                      // handles the speaker/headphone buttons
// Buttons on the Bluetooth device (AVRCP): play/pause once = mute on/off, twice = next station,
// three times = previous station (counted within 0.6 s); a device's own next/previous command
// does the same. A station change unmutes.
void playerSetMuted(bool on);
bool playerMuted();
// Coexistence test (H4, docs/wifi-stall-investigation.md): when wanted, WiFi gets priority
// (ESP_COEX_PREFER_WIFI) only while the A2DP link is up and audio is pulled; BALANCE otherwise
// (paging needs Bluetooth). Not stored: a restart is back to BALANCE.
void playerSetCoexWifi(bool wanted);
bool playerCoexWifiWanted();
bool playerCoexWifiActive();
// settings.btLead changed: apply the link policy (takes effect for the next connection).
void playerApplyLinkPolicy();
const char* playStateName(PlayState s);
