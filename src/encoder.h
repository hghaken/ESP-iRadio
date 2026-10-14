// EC11 rotary encoder with push button (A GPIO32, B GPIO33, switch GPIO25, common pins to GND).
// Turning sets the volume (volume bar on the display), a short press mutes or unmutes, a long
// press opens the station chooser: turning browses, a short press plays, a long press or 10 s
// without input cancels.
#pragma once
#include <Arduino.h>

void encoderBegin();
void encoderLoop();       // from loop(): debounces the button and acts on turns and presses
String encoderStatus();   // for /api/status diag.enc
