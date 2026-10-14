// EC11 rotary encoder with push button (A GPIO32, B GPIO33, switch GPIO25, common pins to GND).
// Test stage (v1.24): it only counts and logs; nothing in the radio reacts to it yet.
#pragma once
#include <Arduino.h>

void encoderBegin();
void encoderLoop();       // from loop(): debounces the button, logs changes
String encoderStatus();   // for /api/status diag.enc
