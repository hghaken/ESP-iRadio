#include "battery.h"

#include "config.h"

static const int SAMPLES = 16;

void batteryBegin() {
  pinMode(BAT_SENSE_PIN, INPUT);
  pinMode(PWR_SENSE_PIN, INPUT);
  analogSetPinAttenuation(BAT_SENSE_PIN, ADC_11db);  // up to ~3.1 V at the pin
  analogSetPinAttenuation(PWR_SENSE_PIN, ADC_11db);
}

static int readMilliVolts(int pin) {
  uint32_t sum = 0;
  for (int i = 0; i < SAMPLES; i++) sum += analogReadMilliVolts(pin);  // eFuse-calibrated
  return sum / SAMPLES;
}

// Li-Po open-circuit voltage -> rough percentage (resting curve, no load compensation).
static int percentFromMillivolts(int mv) {
  static const int table[][2] = {{4200, 100}, {4100, 90}, {4000, 78}, {3900, 62}, {3800, 45},
                                 {3700, 25},  {3600, 12}, {3500, 5},  {3400, 2},  {3300, 0}};
  if (mv >= table[0][0]) return 100;
  for (size_t i = 1; i < sizeof(table) / sizeof(table[0]); i++) {
    if (mv >= table[i][0]) {
      int v0 = table[i][0], p0 = table[i][1], v1 = table[i - 1][0], p1 = table[i - 1][1];
      return p0 + (mv - v0) * (p1 - p0) / (v1 - v0);
    }
  }
  return 0;
}

BatteryInfo batteryRead() {
  BatteryInfo b;
  b.rawBatMv = readMilliVolts(BAT_SENSE_PIN);
  b.rawPwrMv = readMilliVolts(PWR_SENSE_PIN);
  b.millivolts = b.rawBatMv * 2 * BAT_CAL_PERMILLE / 1000;  // 470k/470k divider + calibration
  b.valid = b.millivolts >= 2800 && b.millivolts <= 4350;
  b.percent = b.valid ? percentFromMillivolts(b.millivolts) : -1;
  if (!b.valid) b.millivolts = 0;
  b.usb = b.rawPwrMv > 1000;
  b.usbMv = b.usb ? b.rawPwrMv * PWR_CAL_PERMILLE / 1000 : 0;
  return b;
}
