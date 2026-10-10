#include "weblog.h"

#include <esp_log.h>
#include <esp_rom_sys.h>
#include <esp_spi_flash.h>

#include "config.h"

static_assert((WEBLOG_BYTES & (WEBLOG_BYTES - 1)) == 0, "WEBLOG_BYTES must be a power of two");

static char* buf = nullptr;              // PSRAM
static volatile uint32_t total = 0;      // bytes written since boot (also the end position)
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static vprintf_like_t origVprintf = nullptr;

LogTee Log;

static inline void IRAM_ATTR putLocked(char c) {
  buf[total & (WEBLOG_BYTES - 1)] = c;
  total = total + 1;
}

// ROM output channel 2: every character of ets_printf (Arduino log_x, library ESP_LOGx).
// Can run in an ISR and while the flash cache is off (flash writes): PSRAM is not reachable
// then, so those characters are dropped.
static void IRAM_ATTR romPutc(char c) {
  if (buf == nullptr || !spi_flash_cache_enabled()) return;
  portENTER_CRITICAL_SAFE(&mux);
  putLocked(c);
  portEXIT_CRITICAL_SAFE(&mux);
}

static void append(const char* s, size_t n) {
  if (buf == nullptr || n == 0) return;
  portENTER_CRITICAL_SAFE(&mux);  // one log line: a few microseconds
  for (size_t i = 0; i < n; i++) putLocked(s[i]);
  portEXIT_CRITICAL_SAFE(&mux);
}

// ESP-IDF's own log output (esp_log_write -> vprintf -> stdout).
static int logVprintf(const char* fmt, va_list args) {
  char line[160];
  va_list copy;
  va_copy(copy, args);
  int n = vsnprintf(line, sizeof(line), fmt, copy);
  va_end(copy);
  if (n > 0) append(line, min((size_t)n, sizeof(line) - 1));
  return origVprintf ? origVprintf(fmt, args) : vprintf(fmt, args);
}

void weblogBegin() {
  buf = (char*)ps_malloc(WEBLOG_BYTES);
  if (buf == nullptr) {
    Serial.println("[log] no PSRAM for the web log");
    return;
  }
  esp_rom_install_channel_putc(2, romPutc);
  origVprintf = esp_log_set_vprintf(logVprintf);
}

size_t LogTee::write(uint8_t c) {
  append((const char*)&c, 1);
  return Serial.write(c);
}

size_t LogTee::write(const uint8_t* data, size_t len) {
  append((const char*)data, len);
  return Serial.write(data, len);
}

uint32_t weblogEnd() { return total; }

uint32_t weblogRead(uint32_t from, char* out, size_t maxBytes, size_t* len, bool* skipped,
                    bool* restarted) {
  *len = 0;
  *skipped = *restarted = false;
  if (buf == nullptr) return 0;
  uint32_t end = total;
  uint32_t oldest = end > WEBLOG_BYTES ? end - WEBLOG_BYTES : 0;
  if (from > end) {
    from = oldest;
    *restarted = true;
  } else if (from < oldest) {
    from = oldest;
    *skipped = true;
  }
  uint32_t n = min((uint32_t)maxBytes, end - from);
  // Copy in small pieces so the lock is never held for long; the writer cannot overrun a
  // 64 KB buffer during the few milliseconds this takes.
  uint32_t done = 0;
  while (done < n) {
    uint32_t chunk = min((uint32_t)256, n - done);
    portENTER_CRITICAL(&mux);
    for (uint32_t i = 0; i < chunk; i++) out[done + i] = buf[(from + done + i) & (WEBLOG_BYTES - 1)];
    portEXIT_CRITICAL(&mux);
    done += chunk;
  }
  *len = n;
  return from + n;
}
