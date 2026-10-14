#include "encoder.h"
#include <soc/gpio_reg.h>
#include "config.h"
#include "display.h"
#include "player.h"
#include "settings.h"
#include "weblog.h"

// Quadrature decoding in an interrupt on both edges of A and B: a table on (previous state,
// new state) gives +1, -1 or an impossible jump (both pins changed). An EC11 makes 4 steps per
// detent and rests with A and B high. A detent counts when the encoder is back at rest with a net
// of at least 2 steps in one direction since the last rest; bounce and missed edges then cannot
// shift the count (raw / 4 did, in the v1.24 test). Clockwise is positive: on this wiring the
// raw steps run the other way, hence ENC_REVERSE. GPIO32/33 are read straight from
// GPIO_IN1_REG so the handler stays in IRAM (it also runs while the flash cache is off).
#define ENC_REVERSE 1
static volatile int32_t raw = 0;
static volatile int32_t detents = 0;
static volatile int8_t sinceRest = 0;
static volatile uint32_t badSteps = 0;  // impossible transitions (both pins changed at once)
static volatile uint8_t abState = 0;

static inline uint8_t readAB() {
  uint32_t in = REG_READ(GPIO_IN1_REG);  // GPIO32 = bit 0, GPIO33 = bit 1
  return (uint8_t)(((in >> (ENC_A_PIN - 32)) & 1) << 1 | ((in >> (ENC_B_PIN - 32)) & 1));
}

static void IRAM_ATTR onEdge() {
  static const int8_t step[16] = {0, -1, 1, 2, 1, 0, 2, -1, -1, 2, 0, 1, 2, 1, -1, 0};
  uint8_t now = readAB();
  int8_t s = step[(abState << 2) | now];
  abState = now;
  if (s == 2) {
    badSteps++;
  } else {
    if (ENC_REVERSE) s = -s;
    raw += s;
    if (sinceRest > -8 && sinceRest < 8) sinceRest += s;
  }
  if (now == 3) {  // at rest
    if (sinceRest >= 2) detents++;
    else if (sinceRest <= -2) detents--;
    sinceRest = 0;
  }
}

// Button: polled from loop(), counted as changed only after 30 ms stable. Held for LONG_MS it
// is a long press (acted on while still held, so the chooser opens without letting go).
static const uint32_t LONG_MS = 700;
static const int VOLUME_STEP = 1;            // percent per detent
static const uint32_t CHOOSER_TIMEOUT_MS = 10000;
static bool btnDown = false;
static bool btnRawLast = false;
static bool longFired = false;
static uint32_t btnChangedAt = 0, btnDownAt = 0;
static uint32_t presses = 0;
static int32_t seenDetent = 0;
static uint32_t volumeSaveAt = 0;  // 0 = nothing to save; NVS is written once the knob rests
static bool choosing = false;
static int chooserIndex = 0;
static uint32_t chooserLastInput = 0;

void encoderBegin() {
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  pinMode(ENC_SW_PIN, INPUT_PULLUP);
  abState = readAB();
  attachInterrupt(ENC_A_PIN, onEdge, CHANGE);
  attachInterrupt(ENC_B_PIN, onEdge, CHANGE);
  Log.printf("[enc] A GPIO%d, B GPIO%d, button GPIO%d (pins now A%d B%d button %s)\n",
             ENC_A_PIN, ENC_B_PIN, ENC_SW_PIN, abState >> 1, abState & 1,
             digitalRead(ENC_SW_PIN) ? "up" : "down");
}

static void showChooser() {
  displayShowChooser(chooserIndex, settings.stations.size(), settings.stations[chooserIndex].name);
}

static void closeChooser(const char* why) {
  choosing = false;
  displayHideChooser();
  Log.printf("[enc] station chooser closed (%s)\n", why);
}

// Turning: volume, or the next/previous station in the chooser.
static void onTurn(int steps) {
  if (choosing) {
    int n = settings.stations.size();
    chooserIndex = ((chooserIndex + steps) % n + n) % n;
    chooserLastInput = millis();
    showChooser();
    return;
  }
  settings.volume = constrain(settings.volume + steps * VOLUME_STEP, 0, 100);
  playerSetVolume(settings.volume);
  if (playerMuted()) playerSetMuted(false);  // turning means: let me hear it
  volumeSaveAt = millis() + 2000;
  displayShowVolume(settings.volume, false);
}

static void onShortPress() {
  if (choosing) {
    int i = chooserIndex;
    closeChooser("station chosen");
    settings.lastStation = i;
    settings.autoPlay = true;
    settingsSave();
    playerSetMuted(false);  // a new station is meant to be heard
    playerPlay(i);
    Log.printf("[enc] play station %d (%s)\n", i, settings.stations[i].name.c_str());
    return;
  }
  playerSetMuted(!playerMuted());
  displayShowVolume(settings.volume, playerMuted());
}

static void onLongPress() {
  if (choosing) {
    closeChooser("cancelled");
    return;
  }
  if (settings.stations.empty()) return;
  int n = settings.stations.size();
  chooserIndex = settings.lastStation >= 0 && settings.lastStation < n ? settings.lastStation : 0;
  choosing = true;
  chooserLastInput = millis();
  showChooser();
  Log.println("[enc] station chooser open");
}

void encoderLoop() {
  uint32_t now = millis();
  int32_t d = detents;
  if (d != seenDetent) {
    int steps = d - seenDetent;
    seenDetent = d;
    displayWake();
    onTurn(steps);
  }
  if (volumeSaveAt && (int32_t)(now - volumeSaveAt) >= 0) {
    volumeSaveAt = 0;
    settingsSaveVolume();
    Log.printf("[enc] volume %d%%\n", settings.volume);
  }
  if (choosing && now - chooserLastInput > CHOOSER_TIMEOUT_MS) closeChooser("timeout");

  bool rawBtn = digitalRead(ENC_SW_PIN) == LOW;
  if (rawBtn != btnRawLast) {
    btnRawLast = rawBtn;
    btnChangedAt = now;
  } else if (rawBtn != btnDown && now - btnChangedAt >= 30) {
    btnDown = rawBtn;
    if (btnDown) {
      btnDownAt = now;
      longFired = false;
      presses++;
    } else if (!longFired) {
      displayWake();
      if (choosing) chooserLastInput = now;
      onShortPress();
    }
  }
  if (btnDown && !longFired && now - btnDownAt >= LONG_MS) {
    longFired = true;
    displayWake();
    onLongPress();
  }
}

String encoderStatus() {
  int32_t r = raw;
  return "position " + String(detents) + " (raw " + String(r) + ", bad " + String(badSteps) +
         "), button " + (btnDown ? "down" : "up") + ", presses " + String(presses);
}
