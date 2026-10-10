#include "display.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <SPI.h>
#include <time.h>

#include "config.h"
#include "player.h"
#include "sdcard.h"
#include "settings.h"

// 1.69" IPS module (ZJY169S0800TG01, ST7789V2, 240 x 280) used in landscape as 280 x 240.
// Layout, top to bottom: header bar ("ESP iRadio", Bluetooth state), station, artist, song
// (each a strip that scrolls on its own when too wide; a station logo from the SD card sits
// left of station and artist), status line, network line, battery (right). The glass has
// rounded corners, hence the wide side margins.

static const int W = 280, H = 240;
static const uint16_t COL_BG = ST77XX_BLACK;
// High-contrast colours only: full-intensity white, yellow, cyan and green on black.
static const uint16_t COL_HEADER = 0x9000;   // dark red bar, white bold text
static const uint16_t COL_STATION = ST77XX_WHITE;
static const uint16_t COL_ARTIST = 0xFFE0;   // yellow
static const uint16_t COL_SONG = 0x07FF;     // cyan
static const uint16_t COL_STATUS = 0x07E0;   // green
static const uint16_t COL_INFO = ST77XX_WHITE;  // network and battery lines
static const uint16_t COL_ERROR = 0xF800;    // red

static const int BL_CHANNEL = 7;                 // LEDC channel for the backlight PWM
static const int BL_BITS = 10, BL_MAX = 1023;    // 10-bit PWM at 5 kHz: fine steps at low levels
static const uint32_t SPI_HZ = 27000000;         // ESP32 rounds to 26.7 MHz (80 / 3)

// Layout (pixels). Header bar and its text.
static const int HEADER_H = 28, HEADER_BASELINE = 19, MARGIN = 18;
// Station logo (SD card), left of the station and artist strips.
static const int LOGO_X = 14, LOGO_Y = 34, LOGO_SIZE = 64, LOGO_GAP = 10;
// Status, network and battery lines (FreeSans 9 pt baselines) and their side margin.
static const int STATUS_Y = 162, NET_Y = 190, POWER_Y = 216, INFO_MARGIN = 20;

struct Strip {
  int top, bottom, baseline;  // rows of the strip and the text baseline
  const GFXfont* font;
  uint16_t color;
};

static const Strip STRIPS[3] = {
    {32, 64, 56, &FreeSansBold12pt7b, COL_STATION},  // station
    {64, 96, 88, &FreeSans12pt7b, COL_ARTIST},       // artist (or the whole title)
    {98, 130, 122, &FreeSans12pt7b, COL_SONG},       // song
};

static Adafruit_ST7789* tft = nullptr;
static GFXcanvas16* canvas = nullptr;  // full frame, 131 KB, allocated in PSRAM

static SemaphoreHandle_t netLock;
static String netLine = "Starting...";
static String powerText;
static bool flipScreen = false;

struct Screen {
  String station, title, status, net, power;
  bool bt = false;    // headphones connected
  bool btOn = true;   // Bluetooth switched on (web toggle)
  bool error = false;
  bool flip = false;
  bool scroll = true;  // in the key so switching it redraws the lines from the start
  int logo = 0;        // sdLogoGeneration(): 0 = no logo for this station
  String key() const {
    return station + "|" + title + "|" + status + "|" + net + "|" + power + "|" + (bt ? "1" : "0") +
           (btOn ? "b" : "-") + (error ? "E" : "") + (flip ? "F" : "") + (scroll ? "S" : "") + "L" + String(logo);
  }
};

void displaySetNetwork(const String& line) {
  xSemaphoreTake(netLock, portMAX_DELAY);
  netLine = line;
  xSemaphoreGive(netLock);
}

void displaySetFlip(bool flip) {
  xSemaphoreTake(netLock, portMAX_DELAY);
  flipScreen = flip;
  xSemaphoreGive(netLock);
}

void displaySetPower(const String& text) {
  xSemaphoreTake(netLock, portMAX_DELAY);
  powerText = text;
  xSemaphoreGive(netLock);
}

// The GFX fonts only have ASCII: fold common UTF-8 Latin-1 letters, drop the rest.
static String toAscii(const String& in) {
  static const char* from = "ÀÁÂÃÄÅàáâãäåÈÉÊËèéêëÌÍÎÏìíîïÒÓÔÕÖòóôõöÙÚÛÜùúûüÇçÑñÝýÿ";
  static const char* to   = "AAAAAAaaaaaaEEEEeeeeIIIIiiiiOOOOOoooooUUUUuuuuCcNnYyy";
  String out;
  out.reserve(in.length());  // one allocation instead of one per character
  for (size_t i = 0; i < in.length(); i++) {
    uint8_t c = in[i];
    if (c < 0x80) { out += (char)c; continue; }
    if ((c & 0xE0) == 0xC0 && i + 1 < in.length()) {
      const char* p = from;
      for (int k = 0; *p; k++, p += 2) {
        if ((uint8_t)p[0] == c && p[1] == in[i + 1]) { out += to[k]; break; }
      }
      i++;
    } else {
      while (i + 1 < in.length() && ((uint8_t)in[i + 1] & 0xC0) == 0x80) i++;
    }
  }
  return out;
}

// ---------------------------------------------------------------- backlight

// Level = the lowest of: the normal level, the night level (inside the night window, once the
// clock is set) and the idle level (no new title, station or Bluetooth state for idleMinutes).
// The PWM duty follows a square curve (perceived brightness) and fades towards the target.
static volatile uint32_t lastActivity = 0;
static int blDuty = -1;
static volatile int blPercent = 100;

void displayWake() { lastActivity = millis(); }

int displayBacklightPercent() { return blPercent; }

static bool nightNow() {
  if (!settings.nightDim || settings.nightFrom == settings.nightTo) return false;
  time_t now = time(nullptr);
  if (now < 1700000000) return false;  // clock not set yet (SNTP)
  struct tm t;
  localtime_r(&now, &t);
  int m = t.tm_hour * 60 + t.tm_min;
  int from = settings.nightFrom, to = settings.nightTo;
  return from < to ? (m >= from && m < to) : (m >= from || m < to);
}

static int targetPercent() {
  int p = settings.backlight;
  if (nightNow()) p = min(p, settings.nightLevel);
  if (settings.idleMinutes > 0 && millis() - lastActivity > (uint32_t)settings.idleMinutes * 60000)
    p = min(p, settings.idleLevel);
  return p;
}

static int dutyFor(int percent) {
  if (percent <= 0) return 0;
  return max(1, (int)((long)BL_MAX * percent * percent / 10000));
}

// Called every tick (40 ms): a change fades in ~1 s.
static void backlightTick() {
  int p = targetPercent();
  int target = dutyFor(p);
  if (blDuty < 0) blDuty = target;
  int diff = target - blDuty;
  if (diff) blDuty += diff > 0 ? max(1, diff / 6) : min(-1, diff / 6);
  ledcWrite(BL_CHANNEL, blDuty);
  if (blDuty == target) blPercent = p;
}

// ---------------------------------------------------------------- update screen

static volatile UpdatePhase updPhase = UpdatePhase::None;
static volatile uint32_t updBytes = 0, updTotal = 0, updFailedAt = 0;
static const char* volatile updSource = "";

void displaySetUpdate(UpdatePhase phase, uint32_t bytes, uint32_t total, const char* source) {
  updBytes = bytes;
  updTotal = total;
  updSource = source;
  if (phase == UpdatePhase::Failed) updFailedAt = millis();
  updPhase = phase;
  lastActivity = millis();  // an update wakes a dimmed screen
}

// ---------------------------------------------------------------- drawing helpers

static uint16_t textWidth(const String& s) {
  int16_t x, y;
  uint16_t w, h;
  canvas->getTextBounds(s, 0, 0, &x, &y, &w, &h);
  return w;
}

static String fit(String s, int maxW) {
  if (textWidth(s) <= maxW) return s;
  while (s.length() > 1 && textWidth(s + "...") > maxW) s.remove(s.length() - 1);
  s.trim();
  return s + "...";
}

static void drawLeft(const String& text, int y, uint16_t color) {
  canvas->setTextColor(color);
  canvas->setCursor(INFO_MARGIN, y);
  canvas->print(fit(text, W - 2 * INFO_MARGIN));
}

static void drawRight(const String& text, int y, uint16_t color) {
  canvas->setTextColor(color);
  canvas->setCursor(W - INFO_MARGIN - textWidth(text), y);
  canvas->print(text);
}

static bool shownFlip = false;  // orientation of the frame on the panel

static void push(bool flip) {
  shownFlip = flip;
  tft->setRotation(flip ? 3 : 1);
  tft->drawRGBBitmap(0, 0, canvas->getBuffer(), W, H);
}

// Pushes only rows [y, y+h) of the canvas (a canvas row is a full panel row, so the strip is one
// contiguous block): a scroll step costs one strip (~18 KB) instead of the whole frame.
static void pushRows(int y, int h) {
  tft->setRotation(shownFlip ? 3 : 1);
  tft->drawRGBBitmap(0, y, canvas->getBuffer() + y * W, W, h);
}

// ---------------------------------------------------------------- scrolling strips

// Station, artist and song each live in their own horizontal strip. Text that is wider than
// its area scrolls: 2 s pause, slide to the end at SCROLL_STEP px per tick, 2 s pause, slide
// back. A strip keeps its scroll position while other fields change; it restarts only when its
// own text or its area changes. Strips are tall enough for the font's ascenders and
// descenders, so a redraw never touches the neighbours.
struct ScrollLine {
  Strip s;
  int left, width;  // text area; narrower next to a station logo (set by setArea())
  String text;
  int textW;
  int offset;       // pixels scrolled to the left
  int dir;
  uint32_t pauseUntil;
};

static const uint32_t SCROLL_PAUSE_MS = 2000;
static const int SCROLL_STEP = 1;       // px per tick
static const uint32_t TICK_MS = 40;     // ~25 px/s

static ScrollLine lines[3];

static uint16_t* logoPix = nullptr;  // PSRAM, LOGO_SIZE^2 pixels
static bool logoShown = false;

static void setArea(ScrollLine& L, int left, int width) {
  if (L.left == left && L.width == width) return;
  L.left = left;
  L.width = width;
  L.offset = 0;  // the scroll range changed: start over
  L.dir = 1;
  L.pauseUntil = millis() + SCROLL_PAUSE_MS;
}

static void setLine(ScrollLine& L, const String& text) {
  if (text == L.text) return;
  L.text = text;
  canvas->setFont(L.s.font);
  L.textW = textWidth(text);
  L.offset = 0;
  L.dir = 1;
  L.pauseUntil = millis() + SCROLL_PAUSE_MS;
}

static void drawLine(const ScrollLine& L) {
  canvas->fillRect(0, L.s.top, W, L.s.bottom - L.s.top, COL_BG);
  if (!L.text.isEmpty()) {
    canvas->setFont(L.s.font);
    canvas->setTextColor(L.s.color);
    int x = L.textW <= L.width ? L.left + (L.width - L.textW) / 2 : L.left - (settings.scrollTitles ? L.offset : 0);
    canvas->setCursor(x, L.s.baseline);
    canvas->print(L.text);
  }
  // GFX has no clipping: text scrolled to the left ran over the logo column. Clear that
  // column and put the logo back (rows outside this strip get the same pixels again).
  if (logoShown && L.s.top < LOGO_Y + LOGO_SIZE) {
    canvas->fillRect(0, L.s.top, L.left, L.s.bottom - L.s.top, COL_BG);
    canvas->drawRGBBitmap(LOGO_X, LOGO_Y, logoPix, LOGO_SIZE, LOGO_SIZE);
  }
}

// One scroll tick for every strip whose text does not fit; pushes only the strips that moved.
static void scrollTick() {
  uint32_t now = millis();
  for (ScrollLine& L : lines) {
    if (L.textW <= L.width || (int32_t)(now - L.pauseUntil) < 0) continue;
    int maxOffset = L.textW - L.width;
    L.offset += L.dir * SCROLL_STEP;
    if (L.offset >= maxOffset) {
      L.offset = maxOffset;
      L.dir = -1;
      L.pauseUntil = now + SCROLL_PAUSE_MS;
    } else if (L.offset <= 0) {
      L.offset = 0;
      L.dir = 1;
      L.pauseUntil = now + SCROLL_PAUSE_MS;
    }
    drawLine(L);
    pushRows(L.s.top, L.s.bottom - L.s.top);
  }
}

// ---------------------------------------------------------------- screen

static void drawScreen(const Screen& s) {
  canvas->fillScreen(COL_BG);

  // Header bar
  canvas->fillRect(0, 0, W, HEADER_H, COL_HEADER);
  canvas->setTextSize(1);
  canvas->setTextColor(ST77XX_WHITE);
  canvas->setFont(&FreeSansBold9pt7b);
  String btText = !s.btOn ? "BT: off" : s.bt ? "BT: on" : "BT: searching";
  canvas->setCursor(MARGIN, HEADER_BASELINE);
  canvas->print("ESP iRadio");
  canvas->setCursor(W - MARGIN - textWidth(btText), HEADER_BASELINE);
  canvas->print(btText);

  // Station logo (SD card) and the text areas next to it.
  logoShown = s.logo && logoPix && sdCopyLogo(logoPix, LOGO_SIZE);
  int textLeft = LOGO_X + LOGO_SIZE + LOGO_GAP;
  for (int i = 0; i < 2; i++)
    setArea(lines[i], logoShown ? textLeft : MARGIN, logoShown ? W - textLeft - MARGIN : W - 2 * MARGIN);
  if (logoShown) canvas->drawRGBBitmap(LOGO_X, LOGO_Y, logoPix, LOGO_SIZE, LOGO_SIZE);

  // Station, artist and song ("Artist - Song"; without a separator the whole title goes in the
  // artist strip). Each strip scrolls on its own when the text is too wide.
  setLine(lines[0], s.station.length() ? s.station : "No station");
  int sep = s.title.indexOf(" - ");
  String artist = sep < 0 ? s.title : s.title.substring(0, sep);
  String song = sep < 0 ? String("") : s.title.substring(sep + 3);
  artist.trim();
  song.trim();
  setLine(lines[1], artist);
  setLine(lines[2], song);
  for (const ScrollLine& L : lines) drawLine(L);

  // Status, network, power
  canvas->setFont(&FreeSans9pt7b);
  drawLeft(s.status, STATUS_Y, s.error ? COL_ERROR : COL_STATUS);
  drawLeft(s.net, NET_Y, COL_INFO);
  if (s.power.length()) drawRight(s.power, POWER_Y, COL_INFO);

  push(s.flip);
}

static void centered(const String& text, int baseline, const GFXfont* font, uint16_t color) {
  canvas->setFont(font);
  canvas->setTextColor(color);
  canvas->setCursor((W - textWidth(text)) / 2, baseline);
  canvas->print(text);
}

static void drawUpdate(UpdatePhase phase, uint32_t bytes, uint32_t total, int percent, bool flip) {
  canvas->fillScreen(COL_BG);
  canvas->fillRect(0, 0, W, HEADER_H, COL_HEADER);
  canvas->setFont(&FreeSansBold9pt7b);
  canvas->setTextColor(ST77XX_WHITE);
  canvas->setCursor(MARGIN, HEADER_BASELINE);
  canvas->print("ESP iRadio");
  centered("Updating firmware", 72, &FreeSansBold12pt7b, COL_STATION);
  centered(updSource, 100, &FreeSans9pt7b, COL_INFO);
  // Progress bar
  const int bx = 24, by = 118, bw = W - 48, bh = 24;
  canvas->drawRect(bx, by, bw, bh, ST77XX_WHITE);
  int fill = (bw - 4) * percent / 100;
  if (fill > 0) canvas->fillRect(bx + 2, by + 2, fill, bh - 4, phase == UpdatePhase::Failed ? COL_ERROR : COL_STATUS);
  String amount = String(bytes / 1024) + (total ? " / " + String(total / 1024) : String("")) + " kB";
  if (total) amount += "   " + String(percent) + "%";
  centered(amount, 168, &FreeSans9pt7b, COL_INFO);
  if (phase == UpdatePhase::Done) centered("Done - restarting", 204, &FreeSansBold9pt7b, COL_STATUS);
  else if (phase == UpdatePhase::Failed) centered("Failed - radio keeps playing", 204, &FreeSansBold9pt7b, COL_ERROR);
  else centered("Do not switch off", 204, &FreeSans9pt7b, COL_ARTIST);
  push(flip);
}

static Screen current() {
  PlayerStatus p = playerStatus();
  Screen s;
  s.station = toAscii(p.stationName);
  s.title = toAscii(p.title);
  s.bt = p.btConnected;
  s.btOn = p.btEnabled;
  if (p.state == PlayState::Error) {
    s.status = "Error: " + p.error;
    s.error = true;
  } else if (p.state == PlayState::Stopped) {
    s.status = "Stopped";
  } else if (p.muted) {
    s.status = "Muted (play button)";
  } else if (!p.btEnabled || !p.btConnected) {
    s.status = "";  // the header already says off / searching
  } else {
    // Bluetooth link quality (RSSI relative to the golden range) and stream cushion.
    String bt = p.btRssiDelta == 127 ? "BT ?" : p.btRssiDelta >= -5 ? "BT good" : p.btRssiDelta >= -15 ? "BT fair" : "BT weak";
    int kbps = p.bitrateKbps > 0 ? p.bitrateKbps : 96;
    // Rounded to 5 s: the status line is part of the full frame, and an exact value would
    // force a full-frame redraw every second.
    int seconds = (p.bufferKB * 8 / kbps + 2) / 5 * 5;
    s.status = bt + "  buf " + String(seconds) + " s" + (p.codec.length() ? "  " + p.codec : "");
  }
  xSemaphoreTake(netLock, portMAX_DELAY);
  s.net = netLine;
  s.power = powerText;
  s.flip = flipScreen;
  s.scroll = settings.scrollTitles;
  xSemaphoreGive(netLock);
  s.logo = sdLogoGeneration();
  return s;
}

static void displayTask(void*) {
  String shownKey, activityKey;
  uint32_t lastCheck = 0;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(TICK_MS));
    backlightTick();
    if (shownKey.length() && !shownKey.startsWith("U") && settings.scrollTitles) scrollTick();
    if (millis() - lastCheck < 250) continue;  // fields are compared 4 times a second
    lastCheck = millis();
    UpdatePhase phase = updPhase;
    if (phase == UpdatePhase::Failed && millis() - updFailedAt > 10000) updPhase = phase = UpdatePhase::None;
    if (phase != UpdatePhase::None) {
      uint32_t bytes = updBytes, total = updTotal;
      int percent = phase == UpdatePhase::Done ? 100 : total ? (int)min<uint64_t>(100, (uint64_t)bytes * 100 / total) : 0;
      String key = "U" + String((int)phase) + "/" + String(percent) + "/" + String(total ? 0 : bytes / 51200);
      if (key != shownKey) {
        drawUpdate(phase, bytes, total, percent, flipScreen);
        shownKey = key;  // the normal screen redraws afterwards (its key differs); no scrolling meanwhile
      }
      continue;
    }
    if (shownKey.startsWith("U")) shownKey = "";
    Screen s = current();
    // What wakes a dimmed screen: a new title or station, Bluetooth or error state. Not the
    // buffer seconds, WiFi signal or battery, which change on their own.
    String act = s.station + "|" + s.title + "|" + (s.bt ? "1" : "0") + (s.btOn ? "b" : "-") + (s.error ? "E" : "");
    if (act != activityKey) {
      activityKey = act;
      lastActivity = millis();
    }
    String key = s.key();
    if (key == shownKey) continue;
    drawScreen(s);
    shownKey = key;
  }
}

void displayBegin() {
  netLock = xSemaphoreCreateMutex();
  // A microSD card on the same bus (SD_CS) must stay deselected.
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  ledcSetup(BL_CHANNEL, 5000, BL_BITS);
  ledcAttachPin(TFT_BL, BL_CHANNEL);
  lastActivity = millis();
  backlightTick();  // the configured level from the first frame on

  SPI.begin();  // VSPI defaults: SCK 18, MOSI 23 (MISO 19 only for a card reader)
  tft = new Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);
  tft->init(240, 280);  // Adafruit's generic ST7789 init (includes INVON: IPS panels need it), 20-row offset
  tft->setSPISpeed(SPI_HZ);
  canvas = new GFXcanvas16(W, H);  // 131 KB: lands in PSRAM (malloc > 256 bytes)
  canvas->setTextWrap(false);
  logoPix = (uint16_t*)malloc(LOGO_SIZE * LOGO_SIZE * 2);  // PSRAM as well
  for (int i = 0; i < 3; i++) {
    lines[i].s = STRIPS[i];
    lines[i].left = MARGIN;
    lines[i].width = W - 2 * MARGIN;
    lines[i].dir = 1;
  }
  flipScreen = settings.displayFlip;
  tft->setRotation(flipScreen ? 3 : 1);
  tft->fillScreen(COL_BG);

  // First frame at once (boot messages arrive before the task's first round).
  Screen s;
  s.net = netLine;
  s.flip = flipScreen;
  s.scroll = settings.scrollTitles;
  drawScreen(s);
  xTaskCreatePinnedToCore(displayTask, "display", 5120, nullptr, 1, nullptr, 1);
}
