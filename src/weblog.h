// Copy of everything the radio logs, for the live log window on the web page (/api/log).
//
// A ring buffer in PSRAM (WEBLOG_BYTES, config.h) always runs, so the log also holds what
// happened before the window was opened (boot, paging windows). Three sources feed it:
//  - own messages: write them with `Log.printf(...)` / `Log.println(...)` instead of Serial;
//    Log writes to the USB serial port and to the buffer;
//  - Arduino and library log_x()/ESP_LOGx (the Bluetooth "(diag)" lines): they end in
//    ets_printf, which also feeds ROM output channel 2, hooked here;
//  - ESP-IDF's own ESP_LOGx (e.g. "E (123) wifi: ..."): esp_log_set_vprintf hook.
// The buffer is lost on a restart.
#pragma once
#include <Arduino.h>

void weblogBegin();  // call right after Serial.begin()

// Copies log text from absolute position `from` (bytes since boot) into `out` (at most
// `maxBytes`), sets *len and returns the position after the copied text. When `from` is
// older than the buffer, the copy starts at the oldest byte and *skipped is set; when `from`
// lies beyond the end (the radio restarted), it starts at the oldest byte and *restarted is set.
uint32_t weblogRead(uint32_t from, char* out, size_t maxBytes, size_t* len, bool* skipped,
                    bool* restarted);
uint32_t weblogEnd();  // current end position

// Print target that writes to Serial and to the log buffer.
class LogTee : public Print {
 public:
  size_t write(uint8_t c) override;
  size_t write(const uint8_t* data, size_t len) override;
};
extern LogTee Log;
