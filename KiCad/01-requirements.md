# 01 - Requirements

> **Status 2026-10-15:** written for phase 1, the carrier board for the Olimex module. The owner
> chose the integrated board instead (`07-integrated-board.md`, decisions D8-D18). Still valid
> here: the product description and decisions D1-D4 and D6 (chips, display on the board, case,
> one mono speaker, JLCPCB assembly); D5/D7 (USB-C feeding the Olimex board) are replaced by the
> power design in 07.

## Product

A battery-powered internet radio. The ESP32 streams internet radio over WiFi and plays it on
Bluetooth headphones/speakers (A2DP) or, with this board, on a line output and a built-in mono
speaker. It is controlled from a web page and from a rotary knob. Power: a 3.7 V Li-Po (5000 mAh,
JST-PH on the Olimex board), charged over USB.

## Owner decisions (2026-10-11)

| # | Question | Decision |
|---|---|---|
| D1 | Modules or chips | **Chips on the board** (PCM5102A, class-D amplifier), no plug-in audio modules |
| D2 | Display | **On the board** (the 1.69" module mounted on the carrier) |
| D3 | Enclosure | **3D-printed case designed around the board** (the KiCad STEP model is its input) |
| D4 | Speaker | **One mono speaker, 4 or 8 Ohm at the user's choice**: the board must work with both (4 Ohm ~2.5 W, 8 Ohm ~1.5 W at 5 V) |
| D5 | Amplifier supply | **USB-C 5 V** input on the carrier (plain USB-C sink, no PD negotiation) |
| D7 | USB-C also powers the radio | **Yes**: USB-C runs the radio and charges the Li-Po through the Olimex +5V pin, via an ideal diode (one cable for everything) |
| D6 | Fab | **JLCPCB with SMT assembly** (parts from the JLCPCB/LCSC library, prefer "basic" parts) |

## Phase 1: carrier board (design this)

### Must have

| # | Function | Part | Interface |
|---|---|---|---|
| R1 | Socket for the Olimex ESP32-DevKit-LiPo Rev.D | 2 x 1x19 female headers, 2.54 mm (through-hole, hand-soldered or JLCPCB THT assembly) | all nets via the headers |
| R2 | Display 1.69" IPS 240x280, ST7789V2 (module ZJY169S0800TG01, 8-pin 2.54 mm: GND VCC SCL SDA RES DC CS BLK) on the board | 1x8 female header (module plugs in) or the module soldered on a 1x8 pin header; plus standoffs/holes if the module has them | VSPI + 4 GPIO |
| R3 | Rotary encoder with push button, through the front of the case | EC11 (20 or 24 detents) | 3 GPIO, RC debounce |
| R4 | microSD card socket, reachable from outside the case | push-push microSD socket | VSPI shared with the display, own CS |
| R5 | Stereo line output ~2.1 Vrms | **PCM5102APWR** chip (TSSOP-20) | I2S: 3 GPIO |
| R6 | 3.5 mm stereo jack for the line output | PCB jack at a board edge | analog |
| R7 | Mono speaker amplifier ~3 W at 5 V | **MAX98357A** (I2S input with its own DAC, mono (L+R)/2, TQFN-16) on the same I2S lines as the PCM5102A; since 2026-10-15, replaces the analog PAM8302A route (tested on a breakout board on the radio, firmware v1.37) | I2S `I2S_BCK`/`I2S_LRCK`/`I2S_DATA`, `AMP_SD` (GPIO2) |
| R8 | Speaker connection, bridged output | JST-PH 2-pin (or 2-pin terminal); speaker 4 or 8 Ohm, 2-3 W | never connect a speaker pin to GND |
| R9 | USB-C power input (5 V, up to 3 A) for the amplifier, the radio and Li-Po charging | USB-C receptacle, 5.1k CC pull-downs (sink), polyfuse, TVS, ideal diode to the Olimex +5V pin | power only, no data |
| R10 | Mounting holes for the 3D-printed case | 4 x M2.5 or M3 | |

### Should have

- Spare pins on a header: GPIO34 and GPIO36 (input only, no internal pull-up: add pull-up
  footprints), 3V3, GND.
- Test points for 3V3, 5V_USBC, VAMP, GND, I2S BCK/LRCK/DATA, SPI SCK.
- Silkscreen labels for every connector pin, board name "ESP iRadio", revision, date.

### Constraints

- **Pins are fixed by the firmware.** Use exactly the GPIOs in `02-pinout-and-nets.md`. A change
  means a firmware change: avoid it.
- The Olimex board's micro-USB connector (USB updates from the PC), Li-Po JST connector, reset
  button and the WROVER antenna must stay reachable / free (see `05-layout-guidelines.md`).
- The Olimex board's own battery sensing (GPIO35, GPIO39 via its solder jumpers) stays on the
  Olimex board: leave those header pins unconnected on the carrier.
- 2-layer board, JLCPCB standard rules, 1.6 mm FR-4. SMD parts on the top side only (one-sided
  assembly is cheaper); through-hole connectors and headers.
- Size: as small as the display + encoder + jack allow; target under 100 x 70 mm.

### Use cases the board must support

1. Bluetooth on: audio to headphones/speaker over Bluetooth; amplifier off (GPIO2 low), DAC idle.
2. Bluetooth off: audio over I2S to the DAC; line out on the jack; amplifier on (GPIO2 released,
   the 1M pull-up gives (L+R)/2; never drive GPIO2 high, that selects the left channel only) to
   the speaker (the I2S firmware exists since v1.28/v1.37).
3. On battery only: radio, display, Bluetooth and line out work; on the carrier the speaker did
   not (amplifier on USB-C only, D5); on the integrated board it does (VSYS, D11).
4. USB-C plugged in: the radio runs from USB-C and the Li-Po charges (decision D7); the Olimex
   micro-USB may be plugged in at the same time (PC for USB updates) without either source feeding
   the other.
5. Encoder: turn = volume, press = mute, hold = station chooser (firmware exists, v1.26).
6. microSD: log files, settings backup, station logos, firmware copies, history (firmware exists,
   `SD_ENABLED 1`).

## Phase 2: integrated board (chosen 2026-10-15)

The owner skipped phase 1: the integrated board is the current design. Its requirements and
decisions D8-D18 are in `07-integrated-board.md` (WROVER-IE-N16R8, USB-C + CH340C, BQ24074 power
path with cell protection, 3.3 V LDO with the on/off switch, bare display panel, everything
assembled by JLCPCB). The early notes that stood here (10k/22k USB divider, "after phase 1") are
superseded.

## Still open

Nothing from the owner (all answered 2026-10-11). To verify by the designer: how the Olimex +5V
pin connects to its USB VBUS, charger input and 3.3 V regulator (Rev.D schematic), see
`04-power.md`.
