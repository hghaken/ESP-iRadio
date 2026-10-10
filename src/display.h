// Status screen on the 1.69" ST7789 IPS (240x280, used as 280x240).
// Drawn in its own task into a full-frame buffer in PSRAM and pushed to the panel in one go
// whenever something changed, so there is no flicker.
#pragma once
#include <Arduino.h>

void displayBegin();
// Network line shown at the bottom, e.g. "192.168.1.50  WiFi -50 dBm" or setup AP info.
void displaySetNetwork(const String& line);
// Power text shown at the bottom right, e.g. "3.9V 60% USB"; empty = unknown.
void displaySetPower(const String& text);
// Rotate the screen 180 degrees.
void displaySetFlip(bool flip);
// Firmware update screen (instead of the normal one) while an update runs: progress bar with
// bytes and percent, then "restarting" or "failed" (that one goes away after 10 s).
enum class UpdatePhase { None, Receiving, Done, Failed };
void displaySetUpdate(UpdatePhase phase, uint32_t bytes, uint32_t total, const char* source);
// Backlight: counts as activity (back to the normal level, the idle timer restarts).
void displayWake();
int displayBacklightPercent();  // level the backlight is at right now (0..100)
