#include "encoder.h"
#include <soc/gpio_reg.h>
#include "config.h"
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

// Button: polled from loop(), counted as changed only after 30 ms stable.
static bool btnDown = false;
static bool btnRawLast = false;
static uint32_t btnChangedAt = 0, btnDownAt = 0;
static uint32_t presses = 0;
static int32_t loggedDetent = 0;
static uint32_t loggedBad = 0;

void encoderBegin() {
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  pinMode(ENC_SW_PIN, INPUT_PULLUP);
  abState = readAB();
  attachInterrupt(ENC_A_PIN, onEdge, CHANGE);
  attachInterrupt(ENC_B_PIN, onEdge, CHANGE);
  Log.printf("[enc] test mode: A GPIO%d, B GPIO%d, button GPIO%d (pins now A%d B%d button %s)\n",
             ENC_A_PIN, ENC_B_PIN, ENC_SW_PIN, abState >> 1, abState & 1,
             digitalRead(ENC_SW_PIN) ? "up" : "down");
}

void encoderLoop() {
  uint32_t now = millis();
  int32_t r = raw;
  int32_t detent = detents;
  if (detent != loggedDetent) {
    Log.printf("[enc] %s, position %ld (raw %ld, bad steps %lu)\n",
               detent > loggedDetent ? "right" : "left", (long)detent, (long)r, (unsigned long)badSteps);
    loggedDetent = detent;
    loggedBad = badSteps;
  } else if (badSteps != loggedBad && badSteps - loggedBad >= 10) {
    Log.printf("[enc] %lu bad steps (bounce)\n", (unsigned long)badSteps);
    loggedBad = badSteps;
  }

  bool rawBtn = digitalRead(ENC_SW_PIN) == LOW;
  if (rawBtn != btnRawLast) {
    btnRawLast = rawBtn;
    btnChangedAt = now;
  } else if (rawBtn != btnDown && now - btnChangedAt >= 30) {
    btnDown = rawBtn;
    if (btnDown) {
      btnDownAt = now;
      presses++;
      Log.printf("[enc] button down (press %lu)\n", (unsigned long)presses);
    } else {
      Log.printf("[enc] button up after %lu ms\n", (unsigned long)(now - btnDownAt));
    }
  }
}

String encoderStatus() {
  int32_t r = raw;
  return "position " + String(detents) + " (raw " + String(r) + ", bad " + String(badSteps) +
         "), button " + (btnDown ? "down" : "up") + ", presses " + String(presses);
}
