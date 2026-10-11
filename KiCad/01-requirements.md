# 01 - Requirements

## Product

A battery-powered internet radio. The ESP32 streams internet radio over WiFi and plays it on
Bluetooth headphones/speakers (A2DP) or, with the planned extension, on a line output and a small
speaker. It is controlled from a web page and, once the encoder is fitted, from a rotary knob.
Power: a 3.7 V Li-Po (5000 mAh, JST-PH on the Olimex board), charged over USB; in daily use a USB
power bank feeds the USB connector.

## Phase 1: carrier board (design this)

### Must have

| # | Function | Part | Interface |
|---|---|---|---|
| R1 | Socket for the Olimex ESP32-DevKit-LiPo Rev.D | 2 x 1x19 female headers, 2.54 mm | all nets via the headers |
| R2 | Display 1.69" IPS 240x280, ST7789V2 (module ZJY169S0800TG01, 8-pin 2.54 mm: GND VCC SCL SDA RES DC CS BLK) | module on a 1x8 female header, or a cable + connector | VSPI + 4 GPIO |
| R3 | Rotary encoder with push button | EC11 (20 or 24 detents) | 3 GPIO, RC debounce |
| R4 | microSD card socket | push-push microSD socket | VSPI shared with the display, own CS |
| R5 | Line output (stereo, ~2.1 Vrms) | PCM5102A (module GY-PCM5102 or chip) | I2S: 3 GPIO |
| R6 | 3.5 mm stereo jack for the line output | PCB jack | analog |
| R7 | Speaker amplifier 2 x 3 W class D | PAM8403 (module or chip) | analog in from the DAC, shutdown GPIO |
| R8 | Speaker connection(s), bridged (BTL) outputs | 2-pin screw terminal or JST-PH per channel | never connect a speaker pin to GND |
| R9 | Mounting holes | 4 x M2.5 or M3 | enclosure |

### Should have

- Spare pins on a header: GPIO34 and GPIO36 (input only, no internal pull-up: add pull-up
  footprints), 3V3, GND. For later buttons or sensors.
- A power switch for the amplifier supply (or a jumper), see `04-power.md`.
- Test points for 3V3, 5V, VBAT (if routed), GND, I2S BCK/LRCK/DATA, SPI SCK.
- Silkscreen labels for every connector pin, board name "ESP iRadio", revision, date.

### Constraints

- **Pins are fixed by the firmware.** Use exactly the GPIOs in `02-pinout-and-nets.md`. A change
  means a firmware change: avoid it.
- The Olimex board's USB connector, Li-Po JST connector, reset button and the WROVER antenna must
  stay reachable / free (see `05-layout-guidelines.md`).
- The Olimex board's own battery sensing (GPIO35, GPIO39 via its solder jumpers) stays on the
  Olimex board: leave those header pins unconnected on the carrier.
- 2-layer board, standard fab rules (JLCPCB/PCBWay class: 0.15 mm track/space, 0.3 mm drill),
  1.6 mm FR-4, HASL or ENIG. Hand-solderable parts preferred (0805 passives, through-hole
  connectors); SMD chips only if the board is assembled by the fab.
- Size: as small as the display + encoder + jack allow; target under 100 x 70 mm. Owner decides
  the enclosure (see open questions).

### Use cases the board must support

1. Bluetooth on: audio to headphones/speaker over Bluetooth; amplifier off (GPIO2 low), DAC idle.
2. Bluetooth off: audio over I2S to the DAC; line out on the jack; amplifier on (GPIO2 high) to
   the speaker. (Firmware for the I2S path is still to be written.)
3. Encoder: turn = scroll stations / volume, press = menu (firmware to be written).
4. microSD: log files, settings backup, station logos, firmware copies (firmware exists,
   `SD_ENABLED` in `src/config.h` switches it on).

## Phase 2: integrated board (later, notes only)

- ESP32-WROVER-E (8 MB PSRAM variant fine; the firmware uses 4 MB PSRAM, 4 MB flash minimum).
- USB-C with a USB-UART (CH340C or CP2102N) and the usual auto-reset circuit (DTR/RTS -> EN/GPIO0).
- Li-Po charger (e.g. TP4056-class or BQ24075 with power path), 3.3 V LDO or buck (>= 600 mA peaks:
  WiFi + Bluetooth), battery and USB voltage dividers on GPIO35 / GPIO39 like the Olimex
  (470k/470k for the battery; 10k/22k for USB, see `AGENTS.md` "Battery measurement").
- Then the Olimex reference design (its KiCad files) is the starting point. Do not start phase 2
  before phase 1 has run for a while.

## Open questions for the owner (owner decides)

1. Modules on headers (cheap, replaceable, hand-solderable) or chips on the board (PCM5102A in
   TSSOP-20, PAM8403 in SOP-16: smaller, cleaner, needs assembly)? Recommendation: modules for
   Rev A, chips in a later revision.
2. Display on the board (module plugged on top of the carrier) or on a cable to the front panel?
3. Enclosure: existing box or a 3D-printed case designed around the board? Dimensions?
4. Speaker: one mono speaker (only the left channel) or two? Impedance (4 or 8 Ohm) and size?
5. Amplifier supply: USB 5 V only, or also from the battery (needs the battery wire routed through
   the carrier, see `04-power.md`)?
6. Fab and assembly: JLCPCB (with or without SMT assembly), or hand assembly?
