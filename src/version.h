// Firmware version and build stamp of the running firmware.
#pragma once

// Raised by hand with every firmware change (v1.01, v1.02, ...): shown bottom left on the
// display and under Diagnostics. The build stamp below changes on every build anyway.
#define FIRMWARE_VERSION "v1.36"

extern const char* const buildStamp;  // e.g. "2026-10-11 09:15"
extern const char* const firmwareVersion;
