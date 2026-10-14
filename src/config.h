// Hardware pins and compile-time defaults.
#pragma once

// Display: 1.69" ST7789 IPS 240x280 (GND VCC SCL SDA RES DC CS BLK, VCC on 3V3) on the VSPI bus
// (SCK GPIO18, MOSI GPIO23; MISO GPIO19 only for a card reader - the SPI library defaults).
// Since 2026-10-10 evening; before it the Arduino Robot LCD and the WeAct 2.13" e-paper used
// the same CS/DC/RST/BL pins (see git history).
#define TFT_CS   5
#define TFT_DC   22
#define TFT_RST  21
#define TFT_BL   26   // backlight via a transistor on the module: high = on, PWM for dimming
#define SD_CS    4    // microSD on the same bus, see sdcard.cpp
// 0 while no card reader is wired (the ST7789 module has no slot; a separate microSD breakout
// on SD_CS, wired since 2026-10-14): sdBegin() then skips the card and every SD feature stays idle.
#define SD_ENABLED 1

// EC11 rotary encoder (common pins to GND, internal pull-ups; on the PCB 10k + 100 nF each).
#define ENC_A_PIN  32
#define ENC_B_PIN  33
#define ENC_SW_PIN 25

// Olimex battery sense (470k/470k divider, needs BAT_SENS_E1 + BAT_PWR_E1 bridged) and
// external power sense (needs PWR_SENS_E1 bridged). Both ADC1, input-only pins.
#define BAT_SENSE_PIN 35
#define PWR_SENSE_PIN 39
// Calibration: multimeter 4.09 V at the Li-Po while the pin read 2067 mV (x2 = 4134 mV),
// 2026-10-09 on USB. Was 986 (3.900 V vs 3.957 V, 2026-10-06, on battery).
#define BAT_CAL_PERMILLE 989
// USB input voltage = GPIO39 pin voltage x divider x calibration, as one factor: multimeter
// 4.54 V (power bank) while the pin read ~3117 mV -> 1457 permille. That matches a 10k/22k
// divider (x1.4545). Above ~4.6-4.8 V the pin reaches the top of the ADC range (11 dB
// attenuation) and the reading flattens off; a real 5 V supply reads too low.
#define PWR_CAL_PERMILLE 1457

// Access point used when no WiFi network is configured (or it cannot be reached).
#define SETUP_AP_NAME "ESP-iRadio-Setup"
#define MDNS_NAME     "radio"  // -> http://radio.local/

#define DEFAULT_BT_NAME "WH-1000XM6"
#define DEFAULT_VOLUME  60  // percent

// "Name|URL" per line.
#define DEFAULT_STATIONS \
  "NPO Radio 2|http://icecast.omroep.nl/radio2-sb-mp3\n" \
  "Arrow Classic Rock|http://stream.player.arrow.nl/arrowcr\n"

// Live log on the web page: ring buffer in PSRAM (power of two), ~500-1000 log lines.
#define WEBLOG_BYTES (64 * 1024)

#define MAX_STATIONS 60  // list is stored as an NVS blob (~90 bytes per line)

// Buffers (allocated in PSRAM).
#define NET_BUFFER_BYTES  (256 * 1024)  // compressed stream: ~21 s AAC 96k, ~6 s MP3 320k
#define PCM_BUFFER_BYTES  (64 * 1024)   // decoded 44.1 kHz stereo 16-bit: ~0.37 s
// Start decoding once this much is buffered (Icecast bursts ~64 KB on connect, so
// playback starts almost at once). After an underrun the threshold doubles, up to
// PREBUFFER_MAX_BYTES, trading start-up delay for a bigger cushion against WiFi stalls
// (WiFi shares the radio with Bluetooth and loses airtime while audio streams).
#define PREBUFFER_MIN_BYTES (64 * 1024)
#define PREBUFFER_MAX_BYTES (192 * 1024)
// No stream data for this long -> reconnect (Icecast then bursts ~64 KB again). The
// decoder keeps running on the buffered data and resyncs on the new frames.
#define STALL_TIMEOUT_MS 5000
