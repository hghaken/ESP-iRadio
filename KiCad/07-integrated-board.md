# 07 - Integrated board (phase 2, chosen 2026-10-15)

The owner wants everything on one board: no Olimex dev board, no display module, no breakouts.
This replaces the phase 1 carrier board (the A3 schematic of 2026-10-12/15 stays in git as the
reference for every function that is already verified on the breadboard).

None of the module schematics are needed: every block comes from a chip datasheet or reference
design. The Olimex ESP32-DevKit-LiPo is open hardware; its Rev.D KiCad files are in
`esp-iradio/ref/` (git-ignored) and show the proven USB, charger and regulator circuits.

## Owner decisions (2026-10-15)

| # | Decision | Choice |
|---|---|---|
| D8 | Display | the bare 1.69" IPS panel (ST7789V2, 240x280) soldered by its FPC to the board; backlight driver on the board |
| D9 | ESP32 | **ESP32-WROVER-E-N16R8** module (16 MB flash, 8 MB PSRAM); a chip-down ESP32 is not wanted (antenna and RF tuning) |
| D10 | Battery charging | **power-path charger** (TI BQ24074): the radio runs from USB while the cell charges separately |
| D11 | Speaker | also on battery: the MAX98357A runs from the system rail (4.4 V on USB, the cell voltage on battery) |

Earlier decisions that stay: one mono speaker 4-8 Ohm (D4), USB-C 5 V without PD (D5), JLCPCB with
SMT assembly (D6), 3D-printed case around the board, PCM5102A line out, MAX98357A speaker
amplifier, microSD, EC11 encoder.

## Block diagram

```
USB-C --+-- CC1/CC2 5.1k (sink)
        +-- D+/D- -- ESD (USBLC6-2) -- CH340C -- TXD0/RXD0 (GPIO1/3)
        |                                 +-- DTR/RTS -- auto-reset (2x NPN) -- EN, GPIO0
        +-- VBUS -- TVS -- polyfuse -- BQ24074 IN ---- OUT = VSYS (4.4 V on USB, VBAT on battery)
                                         |  BAT -- Li-Po (JST-PH 2)          |
                                         |  CHG -> GPIO34, PGOOD -> GPIO36   +-- MAX98357A VDD (+ 220 uF)
                                         |                                   +-- LDO 3.3 V 1 A -- +3V3
        VBUS divider -> GPIO39 (USB voltage)                                     |
        VBAT divider 470k/470k -> GPIO35 (battery voltage)                       +-- ESP32-WROVER-E
                                                                                 +-- PCM5102A (AVDD via ferrite)
                                                                                 +-- display VDD + backlight (LEDA)
                                                                                 +-- microSD, encoder pull-ups
```

## Power

- **USB-C input** as in the carrier design: 5.1k on CC1 and CC2, TVS (SMAJ5.0CA) and a polyfuse
  (1.5-2 A hold). One connector now powers, charges and flashes.
- **Charger BQ24074** (TI, VQFN-16 3x3): input OVP 10.5 V, OUT regulated to 4.4 V while USB is
  present, battery regulation 4.2 V. Settings by resistors: ISET for the charge current
  (~1 A for the 5000 mAh cell), ILIM for the input limit (1.5 A), EN1/EN2 = ILIM mode, CE low.
  TS: the cell has no thermistor wire -> 10k to GND (as the datasheet allows; with a 3-wire pack
  the NTC goes here). SYSOFF: slide switch (D12). CHG and PGOOD are open-drain: pull-ups to
  +3V3, read on GPIO34 and GPIO36 (input-only pins, free on this board), plus a red (CHG) and a
  green (PGOOD) LED (D14).
- **3.3 V: LDO, 1 A class** (e.g. Diodes AP7361C-33 or TI TLV75733P), fed from VSYS. Olimex uses
  a 500 mA LDO (HX6211) and the radio ran 14 h on battery with it; 1 A gives margin for WiFi and
  Bluetooth peaks plus the backlight (60 mA). A switcher would gain a little runtime between
  3.5 V and 3.3 V of the cell but puts switching noise near the DAC; the firmware already stops at
  3.45 V (power guard). Input 10 uF, output 22 uF + 100 nF near the WROVER.
- **Speaker supply** = VSYS directly (MAX98357A 2.5-5.5 V): 4.4 V on USB (~2.4 W in 4 Ohm),
  3.5-4.2 V on battery (~1.3-1.8 W). 10 uF + 100 nF at the pins, 220 uF bulk. The BQ24074 input
  limit (1.5 A) covers ESP32 + backlight + speaker peaks; above that the cell supplements.
- **Measurement**: VBAT -> 470k/470k -> GPIO35 (as on the Olimex board, the firmware calibration
  stays). VBUS -> 22k/22k -> GPIO39 (x2; the Olimex had x1.4545 on its diode-fed 5 V rail):
  firmware constant `PWR_CAL_PERMILLE` changes. PGOOD and CHG give "on USB" and "charging"
  without the ADC.

## USB serial and boot

- **CH340C** (SOP-16, internal clock; or the CH340X/CH340K of the Olimex boards) on D+/D-, its
  TXD to GPIO3 (RXD0) and RXD to GPIO1 (TXD0); supplied from +3V3 (V3 pin tied to VCC at 3.3 V).
  The PC sees the same CH340 as today, so `scripts/flash_usb.ps1` and the USB update in the
  firmware keep working.
- **Auto-reset**: the usual two-NPN circuit (DTR/RTS -> EN and GPIO0) as on every ESP32 dev board;
  EN with 10k pull-up and 1 uF to GND (clean power-on reset); GPIO0 with 10k pull-up.
- **Buttons**: RESET (EN) and BOOT (GPIO0), small SMD tact switches (needed only for recovery).

## ESP32-WROVER-E-N16R8

- KiCad symbol and footprint: `RF_Module:ESP32-WROVER-E` (check the library), antenna at a board
  edge with the keep-out from the Espressif hardware design guidelines (no copper, no parts, no
  case metal under and beside the antenna).
- Strapping: GPIO12 (MTDI) must be **low at boot** (high selects 1.8 V flash and the module will
  not boot): no pull-up, nothing that drives it at reset. GPIO0 high (boot), GPIO2 low or floating
  (the MAX98357A SD_MODE network reads low), GPIO5 high (LCD_CS pull-up), GPIO15 open.
- PSRAM: 8 MB on the module, the ESP32 maps 4 MB directly (the rest only through the himem API):
  same as today for the firmware. Flash: 16 MB -> a new partition table with larger OTA slots
  (one full flash over USB when the board is new; the radio's settings start empty).
- Decoupling: 22 uF + 100 nF at the 3V3 pin.

## Display: bare panel on the board (D8)

Panel class: 1.69" IPS, ST7789V2, 240x280, 4-wire SPI, **12-pin FPC, 0.7 mm pitch, soldered**
(not a ZIF connector). Reference: Unvision UN169T-IG01 (the panel of the lcdwiki MSP1691 module,
same pin order as the owner's ZJY169S0800TG01 module; confirm on the panel that is ordered).

| FPC pin | Signal | Board net |
|---|---|---|
| 1 | GND | GND |
| 2 | LEDK | backlight cathode -> driver |
| 3 | LEDA | `+3V3` |
| 4 | VDD | `+3V3` (2.5-3.3 V, ~8 mA) + 100 nF + 10 uF |
| 5, 6 | GND | GND |
| 7 | D/C | `LCD_DC` GPIO22 |
| 8 | CS | `LCD_CS` GPIO5 (10k pull-up) |
| 9 | SCL | `SPI_SCK_BUS` (after R8 22 Ohm) GPIO18 |
| 10 | SDA | `SPI_MOSI_BUS` (after R9) GPIO23 |
| 11 | RESET | `LCD_RST` GPIO21 |
| 12 | GND | GND |

- Backlight: 3 white LEDs in parallel, Vf 2.8-3.2 V, 60 mA typical (90 mA max). Driver as on the
  module (low side), but with a logic-level N-MOSFET: LEDK -> 5.6 Ohm -> AO3400A drain, source GND,
  gate = `LCD_BL` GPIO26 (PWM, the firmware dims it) with 100k to GND (dark until the firmware
  runs). The datasheet recommends a constant-current driver (AW9364); the resistor works on the
  module and is cheaper.
- Pads: 12 x 0.7 mm pitch, ~0.4 x 2.0 mm, in a row matching the FPC end (11.4 mm wide); the FPC is
  hand-soldered (or hot-bar) after the SMT assembly. Mechanics: FPC bend zone 10 mm, panel
  30.07 x 37.43 x 1.56 mm, active area 27.97 x 32.63 mm (see the drawing in the panel spec).
  The panel sits on the front of the board (D13).
- Sources: lcdwiki "1.69inch IPS Module" (MSP1691) with the panel spec SPEC_UN169T-IG01 and the
  module schematic (QDtech), 2026-10-15.

## Unchanged blocks (verified on the breadboard)

- PCM5102APWR line out (I2S BCK GPIO27, LRCK GPIO14, DATA GPIO13; SCK to GND, XSMT pull-up, FMT/
  FLT/DEMP low; 470R + 2.2 nF per output; 3.5 mm jack J5), AVDD through ferrite FB1 (`+3V3A`).
- MAX98357A speaker amplifier on the same I2S nets; SD_MODE = `AMP_SD` GPIO2 with 1M to its VDD
  (now VSYS), GAIN open (9 dB); speaker J6.
- microSD socket (CS GPIO4, MISO GPIO19, shared SCK/MOSI), pull-ups, 10 uF + 100 nF.
- EC11 encoder: A GPIO32, B GPIO33, switch GPIO25, each 10k pull-up + 100 nF (tested smooth).

## GPIO map (firmware stays compatible)

| GPIO | Use | Note |
|---|---|---|
| 0 | BOOT button / auto-reset | 10k pull-up |
| 1, 3 | TXD0 / RXD0 to CH340C | USB serial, log and updates |
| 2 | `AMP_SD` | MAX98357A SD_MODE (low = off, released = on) |
| 4 | `SD_CS` | 10k pull-up |
| 5 | `LCD_CS` | 10k pull-up (strapping) |
| 12 | free, **must stay low at boot** | nothing connected |
| 13, 14, 27 | I2S DATA, LRCK, BCK | PCM5102A + MAX98357A |
| 15 | free (strapping) | nothing connected |
| 16, 17 | PSRAM | inside the module, not available |
| 18, 19, 23 | SPI SCK, MISO, MOSI | display + microSD |
| 21, 22 | `LCD_RST`, `LCD_DC` | |
| 25, 32, 33 | encoder switch, A, B | |
| 26 | `LCD_BL` | backlight PWM |
| 34 | `CHG` (BQ24074) | input only, pull-up to +3V3 |
| 35 | battery voltage | 470k/470k divider |
| 36 | `PGOOD` (BQ24074) | input only, pull-up to +3V3 |
| 39 | USB voltage | 22k/22k divider |

Firmware changes for this board: `PWR_CAL_PERMILLE` (new divider), charge status from GPIO34/36,
a 16 MB partition table; everything else (pins, I2S, SPI, encoder) stays.

## Owner decisions on the details (2026-10-15)

| # | Question | Choice |
|---|---|---|
| D12 | On/off switch | **yes**: a slide switch on the BQ24074 SYSOFF pin (off = the cell is cut from the system, no drain; with USB plugged in the radio still runs and the cell still charges) |
| D13 | Display position | **on the front of the board**: panel on spacers or double-sided tape, FPC folded to the pads on the same side |
| D14 | Charge LEDs | **yes**: red on CHG (charging), green on PGOOD (USB power), besides the screen |
| D15 | Board size | as small as reasonable, about **60 x 80 mm**; the case is designed around the board |
| D16 | Encoder | **recessed**: nothing may stick out of the front panel. EC11 with push switch and a shorter shaft (the carrier footprint is for H20 mm), a low knob (about 15-18 mm across, ~6 mm high) in a round pocket in the front panel (about 22 mm across) with its top flush with the panel. The EC11 body is ~6.5 mm high, so the front panel sits at least that far above the board and the display goes on ~5 mm spacers to bring its glass right behind the window. Alternative if the owner prefers a flat front: a thumbwheel encoder on a case edge. |

## Sources

- TI BQ24072/3/4/5/9 datasheet (ZHCSIF0N / SLUSA19): BQ24074 OUT 4.4 V typ (4.3-4.5 V), OVP 10.5 V.
- Panel: lcdwiki.com/1.69inch_IPS_Module (SPEC_UN169T-IG01.pdf, 1.69IPS_MSP1691_Schematic.pdf).
- Olimex ESP32-DevKit-LiPo Rev.D hardware files (github.com/OLIMEX/ESP32-DevKit-LiPo): CH340X,
  BL4054B charger, HX6211 3.3 V LDO.
- Espressif ESP32-WROVER-E datasheet and hardware design guidelines (antenna keep-out, strapping).
