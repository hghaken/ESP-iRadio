// Prints internal-RAM and PSRAM figures; used to track the tight internal heap
// (WiFi + Bluetooth + decoder must all fit next to each other).
#pragma once
#include <Arduino.h>
#include <esp_heap_caps.h>

#include "weblog.h"

inline void logMem(const char* stage) {
  Log.printf("[mem] %-14s internal free %6u, largest %6u | psram free %7u\n", stage,
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
