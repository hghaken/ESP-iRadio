// Battery / power sensing on the Olimex ESP32-DevKit-LiPo.
// GPIO35 (ADC1) reads the battery through a 470k/470k divider -> Vbat = 2 x Vpin.
// GPIO39 (ADC1) senses external (USB) power through a divider.
// Both need their SMT solder jumpers bridged (BAT_SENS_E1 and BAT_PWR_E1 for the
// battery, PWR_SENS_E1 for USB sense); with an open jumper the pin floats and the
// reading is garbage - batteryValid() reports that.
#pragma once
#include <Arduino.h>

struct BatteryInfo {
  int millivolts = 0;   // battery voltage, 0 if not valid
  int percent = -1;     // rough state of charge from the voltage, -1 if not valid
  bool valid = false;   // reading looks like a Li-Po (2.8 .. 4.35 V)
  bool usb = false;     // external power present (GPIO39)
  int usbMv = 0;        // USB input voltage (calibrated, flattens above ~4.6 V), 0 without USB
  int rawBatMv = 0;     // voltage at the sense pins (diagnostics; ~0 = jumper open)
  int rawPwrMv = 0;
};

void batteryBegin();
BatteryInfo batteryRead();  // takes ~16 ADC samples, a few ms
