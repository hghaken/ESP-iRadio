#include "player.h"

#include <BluetoothA2DPSource.h>
#include <Preferences.h>
#include <algorithm>
#include <WiFi.h>
#include <esp_a2dp_api.h>
#include <esp_avrc_api.h>
#include <esp_coexist.h>
#include <esp_bt.h>
#include <esp_gap_bt_api.h>

// audio-tools pulls in <iostream> when HAS_IOSTRAM is set (Collections/BitVector.h),
// which costs ~100 KB of flash for the C++ locale machinery we never use - flash is
// tight with two OTA slots. Undefine it right after the config header.
#include "AudioConfig.h"
#undef HAS_IOSTRAM
#include "AudioTools/AudioCodecs/AudioEncoded.h"
#include "AudioTools/AudioCodecs/CodecAACHelix.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/Concurrency/BufferRTOS.h"
#include "AudioTools/CoreAudio/AudioOutput.h"
#include "config.h"
#include "display.h"
#include "icystream.h"
#include "memlog.h"
#include "settings.h"
#include "weblog.h"

using namespace audio_tools;

// Pipeline: netTask reads the HTTP stream into netBuf (PSRAM); decodeTask decodes
// netBuf through Helix into pcmBuf (PSRAM, 44.1 kHz 16-bit stereo); the Bluetooth
// stack pulls PCM from pcmBuf in btData() and gets silence when it is empty.
// A station change bumps `generation`; the decoder acknowledges it before the
// net task resets the shared buffers, so no task is inside a buffer call then.
//
// The library objects allocate from PSRAM in their constructors, and PSRAM is only
// initialised after global constructors have run - so they are created in playerBegin().

// Writes decoded PCM into pcmBuf; used as the decoder's output. A2DP runs at
// 44.1 kHz; streams with another rate (48 kHz MP3) are resampled here with linear
// interpolation (stereo 16-bit), otherwise they play too slow/fast.
class PcmSink : public AudioOutput {
 public:
  explicit PcmSink(BufferRTOS<uint8_t>& buf) : buf(buf) {
    out = (int16_t*)ps_malloc(OUT_FRAMES * 4);
  }
  size_t write(const uint8_t* data, size_t len) override {
    if (inRate == BT_RATE || inRate <= 0 || out == nullptr) {
      push(data, len);
      return len;
    }
    const int16_t* in = (const int16_t*)data;
    int nIn = len / 4;
    // Fixed point (Q16): the ESP32 has no double-precision FPU, so the former double
    // maths ran in software for every output frame. The step is exact to ~3 ppm.
    const int32_t step = (int32_t)(((int64_t)inRate << 16) / BT_RATE);
    const int32_t end = (int32_t)(nIn - 1) << 16;
    int nOut = 0;
    while (pos < end) {
      int i = pos >> 16;                      // arithmetic shift = floor, also for pos < 0
      int32_t frac = (pos & 0xFFFF) >> 1;     // Q15, so diff * frac fits in 32 bits
      int32_t l0 = i < 0 ? lastL : in[2 * i], r0 = i < 0 ? lastR : in[2 * i + 1];
      int32_t l1 = in[2 * (i + 1)], r1 = in[2 * (i + 1) + 1];
      out[2 * nOut] = (int16_t)(l0 + (((l1 - l0) * frac) >> 15));
      out[2 * nOut + 1] = (int16_t)(r0 + (((r1 - r0) * frac) >> 15));
      pos += step;
      if (++nOut == OUT_FRAMES) {
        push((uint8_t*)out, nOut * 4);
        nOut = 0;
      }
    }
    if (nOut) push((uint8_t*)out, nOut * 4);
    if (nIn > 0) {
      pos -= (int32_t)nIn << 16;  // keep the fractional position relative to the next buffer
      lastL = in[2 * (nIn - 1)];
      lastR = in[2 * (nIn - 1) + 1];
    }
    return len;
  }
  void setInputRate(int rate) {
    if (rate != inRate) {
      inRate = rate;
      pos = 0;
      Log.printf("[player] decoder %d Hz -> %s\n", rate, rate == BT_RATE ? "no resampling" : "resampling to 44.1 kHz");
    }
  }
  volatile bool abortWrites = false;

 private:
  static const int BT_RATE = 44100;
  static const int OUT_FRAMES = 1152;
  void push(const uint8_t* data, size_t len) {
    size_t written = 0;
    while (written < len) {
      written += buf.writeArray(data + written, len - written);  // waits up to writeMaxWait
      // Link gone: nobody drains pcmBuf. Drop the rest instead of waiting forever, so
      // the decoder reaches its no-headphones path and stale audio does not pile up.
      if (abortWrites || !playerBtConnected()) break;
    }
  }
  BufferRTOS<uint8_t>& buf;
  int16_t* out = nullptr;
  int inRate = BT_RATE;
  int32_t pos = 0;  // Q16 input position relative to the current buffer
  int16_t lastL = 0, lastR = 0;
};

static BluetoothA2DPSource* a2dp;
static IcyStream* url;
static BufferRTOS<uint8_t>* netBuf;
static BufferRTOS<uint8_t>* pcmBuf;
static PcmSink* sink;
static AACDecoderHelix* aac;
static MP3DecoderHelix* mp3;
static EncodedAudioOutput* decoded;

static SemaphoreHandle_t lock;
static PlayerStatus status;              // guarded by lock
static std::vector<String> seenDevices;  // guarded by lock
struct FoundDevice {
  String name;
  uint8_t bda[6];
};
static std::vector<FoundDevice> foundDevices;  // names with addresses from discovery, guarded by lock
static String btTarget;
static volatile int volumePercent = DEFAULT_VOLUME;
static volatile bool muted = false;  // buttons on the speaker/headphones, see playerLoop()

// Requests from the web/UI side (guarded by lock).
static bool requestPending = false;
static int requestStation = -1;  // -1 = stop

static volatile uint32_t generation = 0;  // bumped by netTask per new stream
static volatile uint32_t decoderAck = 0;  // last generation the decoder has seen
static volatile uint32_t readyGen = 0;    // buffers reset and stream running
static volatile bool useMp3 = false;
static volatile bool decoderActive = false;  // decodeTask has a decoder running

const char* playStateName(PlayState s) {
  switch (s) {
    case PlayState::Stopped: return "stopped";
    case PlayState::Connecting: return "connecting";
    case PlayState::Buffering: return "buffering";
    case PlayState::Playing: return "playing";
    case PlayState::Error: return "error";
  }
  return "?";
}

static void setState(PlayState s, const String& error = "") {
  xSemaphoreTake(lock, portMAX_DELAY);
  status.state = s;
  status.error = error;
  xSemaphoreGive(lock);
}

// "SUPERTRAMP - GIVE A LITTLE BIT" -> "Supertramp - Give A Little Bit". Every word gets a
// capital first letter and lower-case rest; a word that already mixes case ("McCartney",
// "DeWolff") is left alone. Only ASCII letters are touched, UTF-8 bytes pass through.
static String titleCase(const String& in) {
  String out = in;
  int n = out.length();
  int i = 0;
  while (i < n) {
    while (i < n && isspace((unsigned char)out[i])) i++;
    int start = i;
    while (i < n && !isspace((unsigned char)out[i])) i++;
    bool upper = false, lower = false;
    for (int k = start; k < i; k++) {
      if (isupper((unsigned char)out[k])) upper = true;
      else if (islower((unsigned char)out[k])) lower = true;
    }
    if (upper && lower) continue;  // mixed case: intentional, keep
    bool firstLetter = true;
    for (int k = start; k < i; k++) {
      unsigned char c = out[k];
      if (!isalpha(c)) continue;
      out[k] = firstLetter ? toupper(c) : tolower(c);
      firstLetter = false;
    }
  }
  return out;
}

static void onTitle(const String& raw) {
  String title = titleCase(raw);
  title.trim();
  xSemaphoreTake(lock, portMAX_DELAY);
  status.title = title;
  xSemaphoreGive(lock);
  Log.printf("[player] title: %s\n", title.c_str());
}

// Discovery callback: remember every device name, connect to the configured one.
static bool onDeviceFound(const char* name, esp_bd_addr_t addr, int rssi) {
  if (name == nullptr || *name == 0) return false;
  xSemaphoreTake(lock, portMAX_DELAY);
  bool known = false;
  for (const String& d : seenDevices) known |= d == name;
  if (!known && seenDevices.size() < 16) seenDevices.push_back(name);
  bool knownAddr = false;
  for (const FoundDevice& f : foundDevices) knownAddr |= memcmp(f.bda, addr, 6) == 0;
  if (!knownAddr && foundDevices.size() < 16) {
    FoundDevice f;
    f.name = name;
    memcpy(f.bda, addr, 6);
    foundDevices.push_back(f);
  }
  xSemaphoreGive(lock);
  bool match = strncasecmp(name, btTarget.c_str(), btTarget.length()) == 0;
  Log.printf("[bt] found '%s' %02X:%02X:%02X:%02X:%02X:%02X rssi %d%s\n", name, addr[0], addr[1], addr[2],
                addr[3], addr[4], addr[5], rssi, match ? " -> connecting" : "");
  return match;
}

// The library's connection state went stale once (2026-10-08: audio flowing at
// 176 KB/s while is_connected() stayed false, so the radio kept opening pairing
// windows). The A2DP media callback only runs on a live link, so a recent pull is
// the stronger evidence.
static volatile uint32_t btLastPullMs = 0; // when Bluetooth last asked for audio

bool playerBtAudioActive() { return btLastPullMs && millis() - btLastPullMs < 2000; }
static bool btLinkUp() { return a2dp->is_connected() || playerBtAudioActive(); }
bool playerBtConnected() { return a2dp && btLinkUp(); }

static volatile int btFailures = 0;
static volatile int streamFailures = 0;
int playerStreamFailures() { return streamFailures; }
void playerResetStreamFailures() { streamFailures = 0; }
// Set by the GAP callback in the (patched) ESP32-A2DP library copy, see
// scripts/patch_a2dp_reconnect.py. 127 = no reading yet.
extern volatile int8_t g_bt_rssi_delta;
static uint32_t lastRssiRequest = 0;
static volatile bool btPageSeen = false;  // CONNECTING or CONNECTED since boot
static volatile bool btPageDone = false;  // that first page has ended

static volatile bool recordPending = false;  // a device connected; playerLoop() stores it
static uint8_t recordBda[6];

// Automatic device choice: a copy of the addresses in priority order for the Bluetooth task
// (the list itself belongs to the loop task). Guarded by lock.
static const int AUTO_MAX = 4;
static uint8_t autoAddr[AUTO_MAX][6];
static int autoCount = 0;             // 0 = automatic mode off (or fewer than 2 devices)
static int autoIndex = 0;             // candidate being paged
static volatile bool autoHold = false;  // a planned disconnect (shutdown, switch, toggle off): do not move on
static volatile int pageFailures = 0;   // pages that ended without a link since boot
static volatile bool linkWasUp = false;  // the last DISCONNECTED ends a link, not a failed page
static volatile uint32_t btConnects = 0;  // CONNECTED events since boot

// After a failed page (or a dropped link) page the next device in the order. The library's
// heartbeat pages last_connection every 10 s; set_auto_reconnect(addr) changes that address.
static void autoNext() {
  if (autoHold || !lock) return;
  xSemaphoreTake(lock, portMAX_DELAY);
  if (autoCount >= 2) {
    autoIndex = (autoIndex + 1) % autoCount;
    uint8_t addr[6];
    memcpy(addr, autoAddr[autoIndex], 6);
    xSemaphoreGive(lock);
    a2dp->set_auto_reconnect(addr);
    Log.printf("[bt] auto: next try device %d (%02X:%02X:%02X:%02X:%02X:%02X)\n", autoIndex, addr[0], addr[1],
               addr[2], addr[3], addr[4], addr[5]);
    return;
  }
  xSemaphoreGive(lock);
}

static void onBtState(esp_a2d_connection_state_t state, void*) {
  esp_bd_addr_t* peer = a2dp->get_last_peer_address();
  Log.printf("[bt] %s (%s)\n", a2dp->to_str(state), peer ? a2dp->to_str(*peer) : "?");
  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    btConnects = btConnects + 1;
    linkWasUp = true;
    autoHold = false;  // a planned disconnect (e.g. the starvation refresh) is over
    if (peer) {
      memcpy(recordBda, *peer, 6);
      recordPending = true;
    }
    btFailures = 0;
    btPageSeen = true;
    btPageDone = true;
  } else if (state == ESP_A2D_CONNECTION_STATE_CONNECTING) {
    btPageSeen = true;
  } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    btFailures = btFailures + 1;
    if (btPageSeen) pageFailures = pageFailures + 1;
    // Page ended (a bare inquiry never sets this). In automatic mode only once every
    // candidate had its page, so the boot keeps WiFi off for the whole round.
    if (btPageSeen && pageFailures >= (autoCount >= 2 ? autoCount : 1)) btPageDone = true;
    // A link that drops (often WiFi's connect burst right after a fresh link) is paged again
    // first; only a failed page moves on to the next device.
    if (linkWasUp) {
      linkWasUp = false;
      Log.println("[bt] auto: link dropped, trying the same device again first");
    } else {
      autoNext();
    }
  }
}

int playerBtFailures() { return btFailures; }
bool playerBtPageStarted() { return btPageSeen; }
bool playerBtPageDone() { return btPageDone; }

static volatile bool quietPageRequested = false;
static volatile bool btEnabled = true;
static bool btStackStarted = false;  // a2dp->start_raw() done

bool playerBluetoothEnabled() { return btEnabled; }

void playerRequestQuietPage() { quietPageRequested = true; }

bool playerTakeQuietPage() {
  if (!quietPageRequested) return false;
  quietPageRequested = false;
  return true;
}

static volatile uint32_t btPulled = 0;    // bytes handed to Bluetooth (diagnostics)
static volatile uint32_t btSilence = 0;   // bytes of padding because pcmBuf was empty
static volatile int statInKBps = 0, statBtKBps = 0, statSilenceKBps = 0;  // last 10 s averages
static volatile int underrunCount = 0, reconnectCount = 0;
// Stream stalls while the Bluetooth link plays (see checkStarvation()).
static volatile uint32_t stallAt = 0, prevStallAt = 0;
static volatile int btRefreshCount = 0;
static volatile int slowPeriods = 0;      // consecutive 10 s periods of starved input (see netTask)
static volatile uint32_t playStartAt = 0;  // last station request (millis | 1)

// Set when the station changes. btData() (the only reader of pcmBuf) empties the buffer
// itself: a reset from netTask could race with a receive running on core 0 and leave
// a fraction of a second of the previous station behind.
static volatile bool pcmFlush = false;

// Called by the Bluetooth stack (~ every 20 ms) for the next PCM block.
// Must never block: take what is there and pad with silence.
static int32_t btData(uint8_t* data, int32_t len) {
  if (pcmFlush) {
    pcmFlush = false;
    // Bounded: the decoder may already be writing the new station.
    for (int i = 0; i < PCM_BUFFER_BYTES / 256 + 2 && pcmBuf->readArray(data, len) > 0; i++) {
    }
  }
  int got = pcmBuf->readArray(data, len);  // readMaxWait is 0
  btPulled += len;
  btLastPullMs = millis();
  if (got < len) {
    memset(data + got, 0, len - got);
    btSilence += len - got;
  }
  int vol = muted ? 0 : volumePercent;
  if (vol < 100) {
    int16_t* s = (int16_t*)data;
    for (int i = 0; i < len / 2; i++) s[i] = (int32_t)s[i] * vol / 100;
  }
  return len;
}

static bool isMp3(const String& u, const String& mime) {
  if (mime.indexOf("aac") >= 0 || mime.indexOf("mp4") >= 0) return false;
  if (mime.indexOf("mpeg") >= 0 || mime.indexOf("mp3") >= 0) return true;
  String l = u;
  l.toLowerCase();
  return l.indexOf("aac") < 0;  // no usable Content-Type: AAC when the URL says so, else MP3
}

// Starts a new generation and clears the buffers once the decoder has let go.
static void resetPipeline() {
  generation = generation + 1;
  sink->abortWrites = true;
  for (int i = 0; i < 200 && decoderAck != generation; i++) vTaskDelay(pdMS_TO_TICKS(10));
  sink->abortWrites = false;
  netBuf->reset();
  if (playerBtConnected()) pcmFlush = true;  // the reader is live: let it drain
  else pcmBuf->reset();                       // nobody reads: safe to reset here
}

static void netTask(void*) {
  static uint8_t chunk[1024];
  int station = -1;
  String streamUrl;
  bool streaming = false;
  bool fresh = false;  // first connection for this station (vs. a reconnect after a stall)
  uint32_t lastData = 0, retryAt = 0;
  uint32_t connBytes = 0;                       // received on the current connection
  const uint32_t HEALTHY_BYTES = 64 * 1024;     // a connection that delivered this much works
  int retries = 0;

  uint32_t statAt = 0, statIn = 0, statPulled = 0, statSilence = 0, totalIn = 0;

  for (;;) {
    // Diagnostics every 10 s: stream input rate vs. Bluetooth pull rate.
    // Playing 44.1 kHz stereo 16-bit in real time pulls 176.4 KB/s.
    if (millis() - statAt >= 10000) {
      uint32_t dt = millis() - statAt;
      if (statAt) {
        statInKBps = (totalIn - statIn) / dt;
        statBtKBps = (btPulled - statPulled) / dt;
        statSilenceKBps = (btSilence - statSilence) / dt;
        // Starved input next to a playing link: under 70 % of the stream rate with the buffer
        // under half and falling. Three periods in a row (30 s) trigger checkStarvation().
        static int lastBufKB = 0;
        int bufKB = netBuf->available() / 1024, kbps = url->bitrateKbps();
        bool slow = streaming && kbps > 0 && statBtKBps >= 170 && statInKBps * 80 < kbps * 7 &&
                    bufKB < (int)(NET_BUFFER_BYTES / 2048) && bufKB < lastBufKB;
        slowPeriods = slow ? slowPeriods + 1 : 0;
        lastBufKB = bufKB;
        Log.printf("[stat] in %d KB/s, bt pull %d KB/s (silence %d KB/s), buffer %d KB, heap %u\n",
                      statInKBps, statBtKBps, statSilenceKBps, netBuf->available() / 1024, ESP.getFreeHeap());
      }
      statAt = millis();
      statIn = totalIn;
      statPulled = btPulled;
      statSilence = btSilence;
    }

    // 1. New request?
    xSemaphoreTake(lock, portMAX_DELAY);
    bool pending = requestPending;
    int req = requestStation;
    requestPending = false;
    if (pending && req >= 0) streamUrl = settings.stations[req].url;
    xSemaphoreGive(lock);

    if (pending) {
      url->end();
      streaming = false;
      station = req;
      resetPipeline();
      xSemaphoreTake(lock, portMAX_DELAY);
      status.station = station;
      status.stationName = station >= 0 ? settings.stations[station].name : "";
      status.title = "";
      status.codec = "";
      xSemaphoreGive(lock);
      setState(station >= 0 ? PlayState::Connecting : PlayState::Stopped);
      fresh = true;
      retries = 0;
      retryAt = 0;
    }

    // 2. Connect (with back-off on failure).
    if (station >= 0 && !streaming && (int32_t)(millis() - retryAt) >= 0) {  // wrap-safe
      if (fresh && retries == 0) setState(PlayState::Connecting);  // keep showing the error while retrying
      Log.printf("[player] connecting %s\n", streamUrl.c_str());
      if (url->begin(streamUrl)) {
        useMp3 = isMp3(streamUrl, url->contentType());
        xSemaphoreTake(lock, portMAX_DELAY);
        status.codec = useMp3 ? "MP3" : "AAC";
        xSemaphoreGive(lock);
        readyGen = generation;
        streaming = true;
        lastData = millis();
        retries = 0;
        connBytes = 0;
        // streamFailures is cleared once this connection has delivered real data (below), not
        // here: on 2026-10-11 every connect succeeded but stalled after a few KB (WiFi still
        // "connected" at -45 dBm, pings lost), so a reset here kept the zombie check silent.
        if (fresh) setState(PlayState::Buffering);
        else setState(decoderActive ? PlayState::Playing : PlayState::Buffering);  // clears an earlier error
        fresh = false;
      } else {
        url->end();
        retries++;
        // Only count failures while WiFi claims to be up: that is the "zombie" signal
        // (associated, IP, nothing reachable). Failures inside a paging window are expected.
        if (WiFi.status() == WL_CONNECTED) streamFailures = streamFailures + 1;
        retryAt = millis() + min(30000, 2000 * retries);
        setState(PlayState::Error, "connect failed");
      }
    }

    // 3. Copy network data into the buffer.
    if (streaming) {
      size_t n = url->readBytes(chunk, sizeof(chunk));
      if (n > 0) {
        lastData = millis();
        totalIn += n;
        connBytes += n;
        if (connBytes >= HEALTHY_BYTES) streamFailures = 0;  // this connection really works
        size_t written = 0;
        while (written < n) {
          written += netBuf->writeArray(chunk + written, n - written);
          lastData = millis();  // blocked on a full buffer is not a stall: data is arriving
          if (requestPending) break;  // drop the rest, a new request is waiting
        }
      } else if (url->lost()) {
        // IcyStream dropped the connection itself (broken metadata block or read error):
        // the byte stream is out of sync, so reconnect at once instead of after the stall
        // timeout. (WiFiClient::connected() is not used here: it reads a stale errno.)
        Log.printf("[player] stream lost, reconnecting (buffer %d KB)\n", netBuf->available() / 1024);
        reconnectCount = reconnectCount + 1;
        url->end();
        streaming = false;
        retryAt = millis() + 200;
      } else if (millis() - lastData > STALL_TIMEOUT_MS) {
        // Reconnect without touching the decoder or buffers: whatever is buffered
        // keeps playing and the new connection's burst refills the buffer.
        Log.printf("[player] stream stalled, reconnecting (buffer %d KB, %u KB on this connection)\n",
                   netBuf->available() / 1024, (unsigned)(connBytes / 1024));
        reconnectCount = reconnectCount + 1;
        // A connection that stalls before delivering real data counts as a failure: three in a
        // row make loop() re-associate WiFi (the "zombie" check in main.cpp).
        if (connBytes < HEALTHY_BYTES && WiFi.status() == WL_CONNECTED) streamFailures = streamFailures + 1;
        if (btLinkUp()) {
          prevStallAt = stallAt;
          stallAt = millis() | 1;  // never 0: 0 means "none"
        }
        url->end();
        streaming = false;
        retryAt = millis() + 200;
      } else {
        vTaskDelay(pdMS_TO_TICKS(5));
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}

static void decodeTask(void*) {
  static uint8_t chunk[1024];
  uint32_t myGen = 0;
  bool prebuffering = true;
  bool decoderRunning = false;
  size_t threshold = PREBUFFER_MIN_BYTES;

  for (;;) {
    uint32_t gen = generation;
    if (gen != myGen) {
      if (decoderRunning) decoded->end();
      decoderRunning = false;
      decoderActive = false;
      sink->setInputRate(44100);  // reset until the new stream reports its rate
      prebuffering = true;
      threshold = PREBUFFER_MIN_BYTES;  // a new station starts with the normal cushion again
      myGen = gen;
      decoderAck = gen;
    }

    if (readyGen != myGen) {  // no stream for this generation yet
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    if (!btLinkUp()) {
      // No headphones: discard only the overflow, so the buffer stays ~3/4 full and
      // playback can start immediately once they connect.
      if (netBuf->available() > NET_BUFFER_BYTES * 3 / 4) netBuf->readArray(chunk, sizeof(chunk));
      else vTaskDelay(pdMS_TO_TICKS(20));
      prebuffering = true;
      continue;
    }

    if (!decoderRunning) {
      decoded->setDecoder(useMp3 ? (AudioDecoder*)mp3 : (AudioDecoder*)aac);
      decoded->begin();
      decoderRunning = true;
      decoderActive = true;
    }

    if (prebuffering) {
      if (netBuf->available() < (int)threshold) {
        vTaskDelay(pdMS_TO_TICKS(20));
        continue;
      }
      prebuffering = false;
      setState(PlayState::Playing);
    }

    int n = netBuf->readArray(chunk, sizeof(chunk));
    if (n <= 0) {
      threshold = min((size_t)PREBUFFER_MAX_BYTES, threshold * 2);
      underrunCount = underrunCount + 1;
      Log.printf("[player] buffer underrun, re-buffering (threshold now %u KB)\n", threshold / 1024);
      prebuffering = true;
      setState(PlayState::Buffering);
      continue;
    }
    decoded->write(chunk, n);  // blocks while pcmBuf is full
    int rate = (useMp3 ? mp3->audioInfo() : aac->audioInfo()).sample_rate;
    if (rate > 0) sink->setInputRate(rate);
  }
}

// ---------------------------------------------------------------- remembered devices

static const int BT_MAX_DEVICES = 4;
struct BtDeviceRec {
  char name[32];
  uint8_t bda[6];
};
static std::vector<BtDeviceRec> btDevices;  // loop task only (web handlers, playerLoop)
static int btSelected = -1;
static bool btAuto = false;
static bool btPairing = false;  // "Pair new device": this boot discovers by name, no list (NVS `pair`)

static void loadDevices() {
  btDevices.clear();
  btSelected = -1;
  Preferences p;
  if (!p.begin("btdev", true)) return;
  size_t len = p.getBytesLength("list");
  if (len && len % sizeof(BtDeviceRec) == 0 && len <= BT_MAX_DEVICES * sizeof(BtDeviceRec)) {
    btDevices.resize(len / sizeof(BtDeviceRec));
    p.getBytes("list", btDevices.data(), len);
    for (BtDeviceRec& d : btDevices) d.name[sizeof(d.name) - 1] = 0;
  }
  btSelected = p.getInt("sel", -1);
  btAuto = p.getBool("auto", false);
  btPairing = p.getBool("pair", false);
  p.end();
  if (btSelected >= (int)btDevices.size()) btSelected = -1;
}

// Copies the order for the Bluetooth task; automatic mode needs two or more devices.
static void syncAuto() {
  if (!lock) return;
  xSemaphoreTake(lock, portMAX_DELAY);
  autoCount = btAuto && !btPairing && btDevices.size() >= 2 ? min((int)btDevices.size(), AUTO_MAX) : 0;
  for (int i = 0; i < autoCount; i++) memcpy(autoAddr[i], btDevices[i].bda, 6);
  if (autoIndex >= autoCount) autoIndex = 0;
  xSemaphoreGive(lock);
}

static void saveDevices() {
  Preferences p;
  if (!p.begin("btdev", false)) return;
  if (btDevices.empty()) p.remove("list");
  else p.putBytes("list", btDevices.data(), btDevices.size() * sizeof(BtDeviceRec));
  p.putInt("sel", btSelected);
  p.putBool("auto", btAuto);
  p.putBool("pair", btPairing);
  p.end();
  syncAuto();
}

// The library keeps its own copy of the last address in NVS (namespace connected_bda). Clear
// it so the next boot discovers by name instead of paging an old device.
static void clearLibraryAddress() {
  Preferences nvs;
  if (nvs.begin("connected_bda", false)) {
    nvs.clear();
    nvs.end();
  }
}

static String bdaText(const uint8_t* b) {
  char s[18];
  snprintf(s, sizeof(s), "%02X:%02X:%02X:%02X:%02X:%02X", b[0], b[1], b[2], b[3], b[4], b[5]);
  return s;
}

// Before start_raw() (and when the order or mode changes): page the top device of the list.
// Automatic mode moves on down the list after a failed page; manual mode stays on the top one.
// With an empty list the library uses its own stored address, or discovers by name (btTarget).
static void applySelection() {
  syncAuto();
  autoHold = false;
  if (btPairing) {  // discover by name (btTarget); the library address was cleared
    Log.printf("[bt] pairing: looking for a device named '%s'\n", btTarget.c_str());
    return;
  }
  if (btDevices.empty()) return;
  autoIndex = 0;
  uint8_t addr[6];
  memcpy(addr, btDevices[0].bda, 6);
  a2dp->set_auto_reconnect(addr);
  Log.printf("[bt] %s device %s (%s)\n", autoCount >= 2 ? "auto: first try" : "top", btDevices[0].name,
             bdaText(addr).c_str());
}

bool playerBtAuto() { return btAuto; }

int playerBtCandidates() { return autoCount >= 2 ? autoCount : 1; }

void playerBtSetAuto(bool on) {
  if (btAuto == on) return;
  btAuto = on;
  saveDevices();
  Log.printf("[bt] automatic device choice %s\n", on ? "on" : "off");
  // Takes effect on the next page: when the link is down now, start at the top of the list.
  if (btStackStarted && btEnabled && !btLinkUp()) applySelection();
}

bool playerBtMoveUp(int index) {
  if (index <= 0 || index >= (int)btDevices.size()) return false;
  std::swap(btDevices[index], btDevices[index - 1]);
  if (btSelected == index) btSelected--;
  else if (btSelected == index - 1) btSelected++;
  saveDevices();
  return true;
}

// ---------------------------------------------------------------- remote buttons (AVRCP)

// Called by the Bluetooth task (fifth patch in scripts/patch_a2dp_reconnect.py). Only counts;
// playerLoop() acts on it.
static portMUX_TYPE remoteMux = portMUX_INITIALIZER_UNLOCKED;
static int playPresses = 0, nextPresses = 0, prevPresses = 0;
static uint32_t lastPlayPress = 0, lastForward = 0;
static const uint32_t TRIPLE_MS = 900;  // a FORWARD waits this long for a PLAY (= triple press)

void radio_avrc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t* param) {
  // Every target event is logged while the buttons are being brought up (diagnostics).
  switch (event) {
    case ESP_AVRC_TG_CONNECTION_STATE_EVT: {
      Log.printf("[avrc] target %s\n", param->conn_stat.connected ? "connected" : "disconnected");
      // Accept the button commands. The library patch tries this right after
      // esp_avrc_tg_init(), but that init is asynchronous: the filter call then fails with
      // ESP_ERR_INVALID_STATE (259) and the supported set stays empty, so every button was
      // refused (2026-10-10). Once a target connection exists the init is certainly done.
      static bool filterSet = false;
      if (param->conn_stat.connected && !filterSet) {
        esp_avrc_psth_bit_mask_t cmds = {0};
        esp_err_t g = esp_avrc_tg_get_psth_cmd_filter(ESP_AVRC_PSTH_FILTER_ALLOWED_CMD, &cmds);
        esp_err_t st = esp_avrc_tg_set_psth_cmd_filter(ESP_AVRC_PSTH_FILTER_SUPPORTED_CMD, &cmds);
        filterSet = g == ESP_OK && st == ESP_OK;
        Log.printf("[avrc] button filter: get %d, set %d, cmds %04x %04x %04x %04x\n", (int)g, (int)st, cmds.bits[0],
                   cmds.bits[1], cmds.bits[2], cmds.bits[3]);
      }
      return;
    }
    case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
      Log.printf("[avrc] peer features 0x%x, ct features 0x%x\n", (unsigned)param->rmt_feats.feat_mask,
                 (unsigned)param->rmt_feats.ct_feat_flag);
      return;
    case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT:
      Log.printf("[avrc] peer registers notification %d\n", (int)param->reg_ntf.event_id);
      return;
    case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
      Log.printf("[avrc] peer sets absolute volume %d\n", (int)param->set_abs_vol.volume);
      return;
    case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
      Log.printf("[avrc] key 0x%02X %s\n", param->psth_cmd.key_code, param->psth_cmd.key_state ? "released" : "pressed");
      break;
    default:
      Log.printf("[avrc] target event %d\n", (int)event);
      return;
  }
  if (param->psth_cmd.key_state != ESP_AVRC_PT_CMD_STATE_PRESSED) return;
  displayWake();  // every button press brings a dimmed screen back (only sets a timestamp)
  uint8_t key = param->psth_cmd.key_code;
  portENTER_CRITICAL(&remoteMux);
  uint32_t now = millis();
  if (key == ESP_AVRC_PT_CMD_PLAY || key == ESP_AVRC_PT_CMD_PAUSE || key == ESP_AVRC_PT_CMD_STOP) {
    if (nextPresses && now - lastForward < TRIPLE_MS) {
      // JBL Charge 5: a triple press arrives as FORWARD (its double press) plus PLAY.
      nextPresses = 0;
      prevPresses++;
    } else {
      playPresses++;
      lastPlayPress = now;
    }
  } else if (key == ESP_AVRC_PT_CMD_FORWARD) {
    nextPresses++;
    lastForward = now;
  } else if (key == ESP_AVRC_PT_CMD_BACKWARD) {
    prevPresses++;
  }
  portEXIT_CRITICAL(&remoteMux);
}

void playerSetMuted(bool on) {
  muted = on;
  Log.printf("[player] %s\n", on ? "muted" : "unmuted");
}

bool playerMuted() { return muted; }

static void stepStation(int dir) {
  int n = settings.stations.size();
  if (n == 0) return;
  int i = ((settings.lastStation + dir) % n + n) % n;
  settings.lastStation = i;
  settings.autoPlay = true;
  settingsSave();
  muted = false;  // a new station is meant to be heard
  playerPlay(i);
  Log.printf("[player] button: %s station %d (%s)\n", dir > 0 ? "next" : "previous", i, settings.stations[i].name.c_str());
}

static void handleButtons() {
  portENTER_CRITICAL(&remoteMux);
  uint32_t now = millis();
  int play = playPresses, prev = prevPresses;
  int next = nextPresses && now - lastForward > TRIPLE_MS ? nextPresses : 0;  // wait for a PLAY first
  bool settled = play && now - lastPlayPress > 600;  // wait for a second or third press
  prevPresses = 0;
  if (next) nextPresses = 0;
  if (next || prev || settled) playPresses = 0;  // a device's own next/previous replaces the count
  portEXIT_CRITICAL(&remoteMux);
  if (next) stepStation(+1);
  else if (prev) stepStation(-1);
  else if (settled) {
    if (play == 1) playerSetMuted(!muted);
    else if (play == 2) stepStation(+1);
    else stepStation(-1);
  }
}

// Bluedroid internals (not in the public headers, present in both builds).
extern "C" uint8_t BTM_GetRole(uint8_t* remote_bd_addr, uint8_t* p_role);
extern "C" void BTM_SetDefaultLinkPolicy(uint16_t settings);
extern "C" uint8_t L2CA_SetDesireRole(uint8_t new_role);
static const uint16_t POLICY_SNIFF = 0x0004, POLICY_SWITCH = 0x0001;  // HCI link policy bits
static const uint8_t L2CAP_DISALLOW_SWITCH = 0x40, L2CAP_ALLOW_SWITCH = 0x80;

// Role switch (2026-10-11): the JBL Charge 5 took the master role right after the radio paged it,
// and WiFi then collapsed every 5-10 min; the headphones did not. With settings.btLead the radio
// creates connections with "role switch not allowed" (L2CA_SetDesireRole; Bluedroid resets it to
// "allowed" when the last stream closes, so it is set again before every page) and the default
// link policy leaves out master/slave switch, so the controller refuses a later request.
static void applyLinkPolicy() {
  BTM_SetDefaultLinkPolicy(settings.btLead ? POLICY_SNIFF : (POLICY_SNIFF | POLICY_SWITCH));
  L2CA_SetDesireRole(settings.btLead ? L2CAP_DISALLOW_SWITCH : L2CAP_ALLOW_SWITCH);
}

// Called by the library once the stack is up, before the first page (sixth patch).
void radio_bt_stack_ready() {
  applyLinkPolicy();
  Log.printf("[bt] link policy: %s\n", settings.btLead ? "radio stays master (no role switch)" : "role switch allowed");
}

void playerApplyLinkPolicy() {
  if (btStackStarted) applyLinkPolicy();
}
static volatile int btRole = -1;   // 0 = the radio is master, 1 = the peer is master, -1 unknown
static uint32_t roleCheckAt = 0;   // next check; a role switch happens right after connecting

// Measures which side leads the link (WiFi trouble with the JBL, 2026-10-11): logged when it
// changes, shown under Diagnostics. Read-only Bluedroid call from the loop task.
static void checkRole() {
  if (!roleCheckAt || (int32_t)(millis() - roleCheckAt) < 0) return;
  roleCheckAt = millis() + 60000;
  esp_bd_addr_t* peer = a2dp ? a2dp->get_last_peer_address() : nullptr;
  if (!peer || !btLinkUp()) {
    btRole = -1;
    return;
  }
  uint8_t addr[6], role = 0xFF;
  memcpy(addr, *peer, 6);
  int r = BTM_GetRole(addr, &role) == 0 ? role : -1;
  if (r != btRole) Log.printf("[bt] link role: %s\n", r == 0 ? "radio is master" : r == 1 ? "peer is master" : "unknown");
  btRole = r;
}

static volatile bool coexWanted = false;  // web toggle, not stored
static bool coexActive = false;           // PREFER_WIFI currently set

void playerSetCoexWifi(bool wanted) {
  coexWanted = wanted;
  Log.printf("[coex] WiFi priority while streaming: %s\n", wanted ? "wanted" : "off");
}
bool playerCoexWifiWanted() { return coexWanted; }
bool playerCoexWifiActive() { return coexActive; }

// PREFER_WIFI only while streaming: with BALANCE paging works; PREFER_BT before start-up once
// stopped every connection, so the preference is never touched at boot or while paging.
static void updateCoex() {
  if (!a2dp || (!coexWanted && !coexActive)) return;  // setup mode / nothing to do
  bool want = coexWanted && btLinkUp() && playerBtAudioActive();
  if (want == coexActive) return;
  esp_err_t err = esp_coex_preference_set(want ? ESP_COEX_PREFER_WIFI : ESP_COEX_PREFER_BALANCE);
  coexActive = want && err == ESP_OK;
  Log.printf("[coex] preference %s -> %d\n", want ? "WIFI" : "BALANCE", (int)err);
}

static void sourceDisconnect();

// WiFi starvation next to some sinks (JBL Charge 5, 2026-10-11, docs/wifi-stall-investigation.md):
// after minutes to hours on one A2DP link the WiFi path slows to 1-4 KB/s (ping 1-2.5 s) while
// Bluetooth plays on; Bluetooth off cured it at once. Trigger (link up and playing): two stream
// stalls within STARVE_WINDOW_MS, or 30 s of starved input (slowPeriods, netTask). Then drop the
// link and page the same device again in a paging window (WiFi off meanwhile, then a fresh
// association too). Not in the first 3 min after boot or a station change. The cooldown starts
// at 10 min and doubles up to 60 min; 2 h without a refresh resets it. When the device does not
// play again within 60 s (e.g. it refuses the bond), the refresh stays off until a restart.
// Reviewed by the reviewer agent 2026-10-11 (hold kept until the link is back, zombie reset).
static const uint32_t STARVE_WINDOW_MS = 600000;
static const uint32_t STARVE_QUIET_MS = 180000;      // after boot or a station change
static const uint32_t STARVE_COOLDOWN_MS = 600000;   // first cooldown, doubles per refresh
static const uint32_t STARVE_COOLDOWN_MAX_MS = 3600000;
static const uint32_t STARVE_BACK_MS = 60000;        // the device must play again within this

static void checkStarvation() {
  static uint32_t lastRefresh = 0, cooldown = STARVE_COOLDOWN_MS, waitUntil = 0, connectsBefore = 0;
  static bool off = false;
  if (off) return;
  if (waitUntil) {  // a refresh is running: is the device back?
    // A new CONNECTED, then audio: btLinkUp() alone still counts pulls of the old link for 2 s.
    if (btConnects != connectsBefore && btLinkUp() && playerBtAudioActive()) {
      waitUntil = 0;
      Log.printf("[bt] back after the WiFi refresh (%u ms)\n", (unsigned)(millis() - lastRefresh));
    } else if ((int32_t)(millis() - waitUntil) >= 0) {
      waitUntil = 0;
      off = true;
      Log.println("[bt] device not back within 60 s after the WiFi refresh: refresh off until restart");
    }
    return;
  }
  uint32_t a = prevStallAt, b = stallAt;
  bool stalls = a && b && b - a <= STARVE_WINDOW_MS && millis() - b < 10000;
  bool slow = slowPeriods >= 3;
  if (!stalls && !slow) return;
  if (!btStackStarted || !btEnabled || !btLinkUp() || !playerBtAudioActive()) return;
  if (millis() < STARVE_QUIET_MS || (playStartAt && millis() - playStartAt < STARVE_QUIET_MS)) return;
  if (lastRefresh && millis() - lastRefresh > 2 * STARVE_COOLDOWN_MAX_MS) cooldown = STARVE_COOLDOWN_MS;
  if (lastRefresh && millis() - lastRefresh < cooldown) return;
  esp_bd_addr_t* peer = a2dp->get_last_peer_address();
  if (peer == nullptr) return;
  uint8_t addr[6];
  memcpy(addr, *peer, 6);
  if (lastRefresh) cooldown = min(cooldown * 2, STARVE_COOLDOWN_MAX_MS);
  lastRefresh = millis() | 1;
  prevStallAt = stallAt = 0;
  slowPeriods = 0;
  btRefreshCount = btRefreshCount + 1;
  if (stalls)
    Log.printf("[bt] WiFi starved next to the link (2 stalls in %u s): reconnecting %s\n",
               (unsigned)((b - a) / 1000), bdaText(addr).c_str());
  else
    Log.printf("[bt] WiFi starved next to the link (input %d KB/s for 30 s): reconnecting %s\n",
               (int)statInKBps, bdaText(addr).c_str());
  playerResetStreamFailures();      // the zombie rule must not re-associate right before the window
  sourceDisconnect();               // planned: autoHold stays until the link is back (onBtState)
  if (a2dp->get_connection_state() != ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    Log.println("[bt] link did not go down, no paging window");
    return;
  }
  a2dp->set_auto_reconnect(addr);   // the library heartbeat pages this same device
  playerRequestQuietPage();         // loop() opens a paging window: WiFi off, page, WiFi back
  connectsBefore = btConnects;
  waitUntil = millis() + STARVE_BACK_MS;
}

int playerBtRefreshes() { return btRefreshCount; }

void playerLoop() {
  handleButtons();
  checkRole();
  updateCoex();
  checkStarvation();
  static uint32_t lastPolicy = 0;
  if (btStackStarted && settings.btLead && !btLinkUp() && millis() - lastPolicy > 2000) {
    lastPolicy = millis();  // before every page: Bluedroid allows the switch again after a stream
    L2CA_SetDesireRole(L2CAP_DISALLOW_SWITCH);
  }
  if (!recordPending) return;
  recordPending = false;
  roleCheckAt = millis() + 5000;  // after a possible role switch
  uint8_t bda[6];
  memcpy(bda, recordBda, 6);
  String name;
  xSemaphoreTake(lock, portMAX_DELAY);
  for (const FoundDevice& f : foundDevices)
    if (memcmp(f.bda, bda, 6) == 0) name = f.name;
  xSemaphoreGive(lock);
  int idx = -1;
  for (size_t i = 0; i < btDevices.size(); i++)
    if (memcmp(btDevices[i].bda, bda, 6) == 0) idx = i;
  if (name.isEmpty()) name = idx >= 0 ? String(btDevices[idx].name) : btTarget;  // reconnect: no discovery name
  bool changed = false;
  if (idx < 0) {
    BtDeviceRec d = {};
    memcpy(d.bda, bda, 6);
    if ((int)btDevices.size() >= BT_MAX_DEVICES) btDevices.pop_back();  // make room: drop the bottom one
    if (btAuto) {  // automatic: the user's order stays, the new one goes last
      btDevices.push_back(d);
      idx = btDevices.size() - 1;
    } else {  // manual: the top one is the one used, so the new device goes on top
      btDevices.insert(btDevices.begin(), d);
      idx = 0;
    }
    changed = true;
  }
  if (strncmp(btDevices[idx].name, name.c_str(), sizeof(btDevices[idx].name)) != 0) {
    strlcpy(btDevices[idx].name, name.c_str(), sizeof(btDevices[idx].name));
    changed = true;
  }
  if (btSelected != idx) {
    btSelected = idx;
    changed = true;
  }
  if (btPairing) {  // the new device is in: back to the list (and automatic mode) from now on
    btPairing = false;
    changed = true;
  }
  if (changed) {
    saveDevices();
    Log.printf("[bt] remembered device %d: %s (%s)\n", idx, btDevices[idx].name, bdaText(bda).c_str());
  }
}

std::vector<BtDeviceInfo> playerBtDevices() {
  std::vector<BtDeviceInfo> v;
  bool up = a2dp && btLinkUp();
  esp_bd_addr_t* peer = a2dp ? a2dp->get_last_peer_address() : nullptr;
  for (size_t i = 0; i < btDevices.size(); i++) {
    bool con = up && peer && memcmp(*peer, btDevices[i].bda, 6) == 0;
    v.push_back({btDevices[i].name, bdaText(btDevices[i].bda), !btAuto && i == 0, con});
  }
  return v;
}

void playerBegin(const char* btName, int volume) {
  lock = xSemaphoreCreateMutex();
  btTarget = btName;
  volumePercent = constrain(volume, 0, 100);

  url = new IcyStream();
  netBuf = new BufferRTOS<uint8_t>(0);
  pcmBuf = new BufferRTOS<uint8_t>(0);
  sink = new PcmSink(*pcmBuf);
  aac = new AACDecoderHelix();
  mp3 = new MP3DecoderHelix();
  decoded = new EncodedAudioOutput((Print*)sink, aac);

  netBuf->setReadMaxWait(pdMS_TO_TICKS(200));
  netBuf->setWriteMaxWait(pdMS_TO_TICKS(100));
  netBuf->resize(NET_BUFFER_BYTES);
  pcmBuf->setReadMaxWait(0);                  // the Bluetooth callback must not block
  pcmBuf->setWriteMaxWait(pdMS_TO_TICKS(200));
  pcmBuf->resize(PCM_BUFFER_BYTES);
  url->setTitleCallback(onTitle);
  url->setTimeout(8000);

  // Bluetooth is started in dual mode (the Arduino core's btStart() default); no
  // controller memory is released, which keeps Bluedroid happy.
  logMem("before BT");
  a2dp = new BluetoothA2DPSource();
  a2dp->set_local_name("ESP iRadio");
  a2dp->set_ssid_callback(onDeviceFound);
  a2dp->set_on_connection_state_changed(onBtState, nullptr);
  loadDevices();
  btEnabled = settings.btEnabled;
  if (btEnabled) {
    a2dp->set_auto_reconnect(true);
    applySelection();
    a2dp->start_raw(btTarget.c_str(), btData);  // returns at once; discovery runs in the background
    btStackStarted = true;
  } else {
    Log.println("[bt] disabled by the web toggle; stack not started");
  }
  esp_bredr_tx_power_set(ESP_PWR_LVL_P9, ESP_PWR_LVL_P9);  // max BR/EDR TX power for a sturdier link
  logMem("bluetooth");

  xTaskCreatePinnedToCore(netTask, "net", 4096, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(decodeTask, "decode", 8192, nullptr, 4, nullptr, 1);
}

void playerPlay(int stationIndex) {
  if (!lock || stationIndex < 0 || stationIndex >= (int)settings.stations.size()) return;
  xSemaphoreTake(lock, portMAX_DELAY);
  requestStation = stationIndex;
  requestPending = true;
  playStartAt = millis() | 1;
  xSemaphoreGive(lock);
}

void playerStop() {
  if (!lock) return;
  xSemaphoreTake(lock, portMAX_DELAY);
  requestStation = -1;
  requestPending = true;
  xSemaphoreGive(lock);
}

void playerSetVolume(int percent) { volumePercent = constrain(percent, 0, 100); }

// The library's disconnect() calls esp_a2d_sink_disconnect, which fails on this
// source-only stack, so a restart used to drop the ACL. Ask the source profile
// and wait until the link is actually down (about 3 s).
static void sourceDisconnect() {
  if (!a2dp) return;
  autoHold = true;  // planned: do not page the next device when this link goes down
  if (a2dp->get_connection_state() == ESP_A2D_CONNECTION_STATE_DISCONNECTED) return;
  esp_bd_addr_t* peer = a2dp->get_last_peer_address();
  if (peer == nullptr) return;
  esp_bd_addr_t addr;
  memcpy(addr, *peer, ESP_BD_ADDR_LEN);
  bool any = false;
  for (int i = 0; i < ESP_BD_ADDR_LEN; i++) any = any || addr[i] != 0;
  if (!any) {
    Log.println("[bt] source disconnect skipped, no peer address");
    return;
  }
  esp_err_t err = esp_a2d_source_disconnect(addr);
  Log.printf("[bt] source disconnect %s -> %d\n", a2dp->to_str(addr), (int)err);
  for (int i = 0; i < 30 && a2dp->get_connection_state() != ESP_A2D_CONNECTION_STATE_DISCONNECTED; i++)
    delay(100);
  Log.printf("[bt] link %s\n",
                a2dp->get_connection_state() == ESP_A2D_CONNECTION_STATE_DISCONNECTED ? "down" : "still up");
}

void playerSetBluetoothEnabled(bool on) {
  if (!lock) return;
  btEnabled = on;
  if (on) {
    if (!btStackStarted) {
      a2dp->set_auto_reconnect(true);
      applySelection();
      a2dp->start_raw(btTarget.c_str(), btData);
      btStackStarted = true;
    } else {
      a2dp->set_auto_reconnect(true);
      applySelection();  // automatic mode: start at the top of the list again
    }
    playerRequestQuietPage();
    Log.println("[bt] enabled");
  } else {
    if (btStackStarted) {
      a2dp->set_auto_reconnect(false);
      sourceDisconnect();
    }
    Log.println("[bt] disabled");
  }
}

void playerShutdown() {
  if (!lock) return;
  playerStop();
  sourceDisconnect();
}

void playerHoldBluetooth(bool hold) {
  if (!lock || !btStackStarted) return;
  if (hold) {
    autoHold = true;
    a2dp->set_auto_reconnect(false);
    Log.println("[bt] paging held (firmware update)");
  } else if (btEnabled) {
    a2dp->set_auto_reconnect(true);
    applySelection();
    Log.println("[bt] paging resumed");
  }
}

void playerForgetHeadphones() {
  if (!lock) return;
  // Auto-reconnect off first, so the library heartbeat does not page again
  // before the bond is removed. Then drop the source link and wait.
  // a2dp->disconnect() calls the sink API and does not drop this link.
  // The library NVS namespace is wiped directly: its own
  // clean_last_connection()/end() path did not clear it in practice.
  a2dp->set_auto_reconnect(false);
  sourceDisconnect();
  clearLibraryAddress();
  btDevices.clear();  // every bond goes, so the remembered devices go too
  btSelected = -1;
  saveDevices();
  int n = esp_bt_gap_get_bond_device_num();
  if (n > 0) {
    esp_bd_addr_t* list = new esp_bd_addr_t[n];
    if (esp_bt_gap_get_bond_device_list(&n, list) == ESP_OK)
      for (int i = 0; i < n; i++) esp_bt_gap_remove_bond_device(list[i]);
    delete[] list;
  }
  // The Bluetooth address is kept. Bumping btIdentity would look like a new device
  // and fill one of the two headphone slots. NVS is cleared while the in-RAM
  // address stays, so a same-address rewrite before the restart does not put the
  // key back. The headphones still hold their copy until "ESP iRadio" is removed
  // in the Sony app.
  Log.printf("[bt] forgot headphones and %d bond(s), address unchanged\n", n);
}

bool playerBtUseDevice(int index) {
  if (!lock || index < 0 || index >= (int)btDevices.size()) return false;
  // Manual mode uses the top device: move this one there.
  std::rotate(btDevices.begin(), btDevices.begin() + index, btDevices.begin() + index + 1);
  btSelected = 0;
  btPairing = false;  // also ends a pairing attempt that never connected
  saveDevices();
  if (btStackStarted) {
    a2dp->set_auto_reconnect(false);  // no page to the old device before the restart
    sourceDisconnect();
  }
  Log.printf("[bt] switching to %s (now on top)\n", btDevices[0].name);
  return true;
}

void playerBtPairNew() {
  if (!lock) return;
  btPairing = true;
  btSelected = -1;
  saveDevices();
  if (btStackStarted) {
    a2dp->set_auto_reconnect(false);
    sourceDisconnect();
  }
  clearLibraryAddress();  // the next boot discovers by name (btTarget) instead of paging
  Log.println("[bt] pairing a new device after the restart");
}

bool playerBtForgetDevice(int index) {
  if (!lock || index < 0 || index >= (int)btDevices.size()) return false;
  esp_bd_addr_t* peer = a2dp ? a2dp->get_last_peer_address() : nullptr;
  bool connected = btStackStarted && btLinkUp() && peer && memcmp(*peer, btDevices[index].bda, 6) == 0;
  if (connected) {
    a2dp->set_auto_reconnect(false);
    sourceDisconnect();
  }
  if (btStackStarted) {
    uint8_t addr[6];
    memcpy(addr, btDevices[index].bda, 6);
    esp_bt_gap_remove_bond_device(addr);
  }
  Log.printf("[bt] forgot device %d: %s\n", index, btDevices[index].name);
  btDevices.erase(btDevices.begin() + index);
  btSelected = btSelected == index ? -1 : btSelected > index ? btSelected - 1 : btSelected;
  clearLibraryAddress();  // never page the forgotten one from the library's own copy
  saveDevices();
  // Restart when its link was dropped or nothing is left to page (the boot then discovers by
  // name); otherwise page the (new) top of the list from now on.
  if (connected || btDevices.empty()) return true;
  if (btStackStarted && btEnabled && !btLinkUp()) applySelection();
  return false;
}

PlayerStatus playerStatus() {
  // Not started (WiFi setup mode) or still starting: playerBegin() creates the lock first and
  // the Bluetooth source (a2dp) last, and the display task may ask in between (seen 2026-10-10:
  // a LoadProhibited boot loop in btLinkUp() once the display polled every 40 ms).
  if (!lock || !a2dp) return PlayerStatus();
  xSemaphoreTake(lock, portMAX_DELAY);
  PlayerStatus s = status;
  xSemaphoreGive(lock);
  s.btConnected = btLinkUp();
  s.btEnabled = btEnabled;
  s.muted = muted;
  s.btRole = btRole;
  s.btBonds = btStackStarted ? esp_bt_gap_get_bond_device_num() : -1;
  s.bufferPercent = netBuf->available() * 100 / NET_BUFFER_BYTES;
  s.sampleRate = useMp3 ? mp3->audioInfo().sample_rate : aac->audioInfo().sample_rate;
  s.inKBps = statInKBps;
  s.btKBps = statBtKBps;
  s.silenceKBps = statSilenceKBps;
  s.bufferKB = netBuf->available() / 1024;
  s.pcmBufferKB = pcmBuf->available() / 1024;
  s.underruns = underrunCount;
  s.reconnects = reconnectCount;
  s.bitrateKbps = url->bitrateKbps();
  s.btRssiDelta = s.btConnected ? g_bt_rssi_delta : 127;
  // Ask the controller for the link RSSI now and then; the answer lands in g_bt_rssi_delta.
  if (s.btConnected && millis() - lastRssiRequest > 30000) {
    lastRssiRequest = millis();
    esp_bd_addr_t* peer = a2dp->get_last_peer_address();
    if (peer) esp_bt_gap_read_rssi_delta(*peer);
  }
  return s;
}

std::vector<String> playerSeenDevices() {
  if (!lock) return {};
  xSemaphoreTake(lock, portMAX_DELAY);
  std::vector<String> v = seenDevices;
  xSemaphoreGive(lock);
  return v;
}
