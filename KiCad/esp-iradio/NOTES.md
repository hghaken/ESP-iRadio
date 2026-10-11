# esp-iradio KiCad project - notes

Design input: `../README.md` and `../01-..06-*.md`. This file records decisions, deviations and
the revision history of the actual KiCad project.

## Status

- 2026-10-11: empty KiCad 9.0.9 project created by the owner; JLCPCB Tools database downloaded.
  Olimex Rev.D hardware files fetched into `ref/` (git-ignored, re-fetch with
  `git clone --depth 1 --filter=blob:none --sparse https://github.com/OLIMEX/ESP32-DevKit-LiPo.git`
  + `git sparse-checkout set HARDWARE` inside `ref/`). Header pinout, row spacing and the +5V power
  path verified against the Rev.D netlist (see `../02-pinout-and-nets.md`, `../04-power.md`).

- 2026-10-11 evening: **schematic Rev A draft done** with the KiCad MCP server: root sheet +
  5 sub-sheets (`power`, `socket`, `display_sd`, `controls`, `audio` `.kicad_sch`), 73 parts, all
  with footprints, connected by net labels (global labels for nets that cross sheets, local labels
  inside a sheet). Verified with `kicad-cli` 9.0.9: ERC 0 violations; the exported netlist was
  checked by a script against `../02-pinout-and-nets.md` (every J1/J2 header pin, SPI bus, DAC
  strapping, amplifier, USB-C, ideal diode, encoder): all correct. Layout of the sheets is
  functional, not yet tidy (some field/label overlaps, a few diagonal wire stubs): cosmetic
  clean-up still to do. No `LCSC`/`MPN` fields yet (next: JLCPCB Tools).

## Schematic plan (Rev A, done as a draft)

Hierarchical, one sheet per block; net names exactly as in `../02-pinout-and-nets.md`:

1. **Root**: sheet symbols, title block "ESP iRadio carrier", Rev A, date.
2. **Power**: J_USBC (CC1/CC2 5.1k each), TVS, polyfuse, `+5V_USBC`, ideal diode U3 -> `+5V_OLI`,
   bulk caps, test points, `+VAMP` (= `+5V_USBC`).
3. **Olimex socket**: J1/J2 1x19 sockets with every pin labelled; unused pins with no-connect flags
   (GPIO0/1/3/6-12/15/16/17/35/39, ESP_EN); `+3V3`, `+5V_OLI`, GND.
4. **Display + microSD**: J3 (1x8 display), microSD socket with pull-ups and decoupling, series
   0 Ohm on `SPI_SCK`/`SPI_MOSI`, pull-ups on `LCD_CS`/`SD_CS`.
5. **Controls**: EC11 with pull-ups and RC, spare header J8 with DNP pull-ups.
6. **Audio**: PCM5102APWR per the TI datasheet (SCK to GND, XSMT pull-up, FMT/FLT/DEMP to GND),
   3.5 mm jack, mono sum resistors, PAM8302A per the Diodes datasheet, `AMP_SD` 100k pull-down,
   speaker connector J6.

Symbols: KiCad standard library first; project library `lib/esp-iradio.kicad_sym` for parts that
are missing (PAM8302A, maybe PCM5102A); every symbol gets `LCSC`, `MPN` and footprint fields. Run
ERC after each sheet.

## Decisions and deviations

- References: J1/J2 Olimex rows, J3 display, J4 microSD, J5 line jack, J6 speaker, J7 USB-C,
  J8 spare header, U1 PCM5102APWR, U2 PAM8302AASCR (SO-8), U3 LM66100DCK, SW1 EC11, F1 polyfuse,
  D1 TVS, FB1/FB2 ferrites, H1-H4 M3 holes, TP1-TP8 test points.
- Passives in 0603 (10 uF in 0805), JLCPCB basic-part sizes.
- Power: TVS **SMAJ5.0CA** (bidirectional, KiCad `Device:D_TVS`) on `VBUS_C`, polyfuse 2 A hold to
  `+5V_USBC`, 10 uF; LM66100 `+5V_USBC` -> `+5V_OLI` (CE to GND, ST open) with 1 uF out; ferrite FB2
  (600 R @ 100 MHz, 2 A) from `+5V_USBC` to `+VAMP` (keeps amplifier switching noise off the
  radio's 5 V).
- Display/SD: 0 Ohm R8/R9 at the ESP32 side of SCK/MOSI; bus side nets `SPI_SCK_BUS`,
  `SPI_MOSI_BUS`; pull-ups 10k on LCD_CS, SD_CS, MISO, DAT1, DAT2.
- DAC (TI PCM510xA datasheet SLAS859C, Figure 33): 0.1 uF + 10 uF on DVDD, CPVDD, AVDD; 2.2 uF
  flying (CAPP-CAPM) and on VNEG; 0.1 uF on LDOO; 470 Ohm + 2.2 nF per output; AVDD through
  ferrite FB1 as `+3V3A`; SCK/FMT/FLT/DEMP to GND, XSMT 10k to 3V3.
- Amplifier (Diodes PAM8302A datasheet DS41333): gain = 150k / (10k + Rin), 23.5 dB with no
  external Rin. The DAC gives 2.1 Vrms full scale, so the mono sum is **LINE_L and LINE_R each
  through 180k into one node (Thevenin 90k), 100 nF to IN+; IN- through 91k + 100 nF to GND**:
  about x1.5 overall (2.1 Vrms -> ~3.2 Vrms, ~2.5 W in 4 Ohm, ~1.3 W in 8 Ohm) instead of the
  10k summing resistors first written in `../02-pinout-and-nets.md`. Supply 10 uF + 1 uF + 220 uF.
  `AMP_SD` 100k pull-down (GPIO2 strapping).
- PWR_FLAGs on `+5V_USBC`, `+VAMP`, `GND`, `+3V3`, `+3V3A` (sources come through passive pins).

## Tooling pitfall

The KiCad MCP server (v2.8.2) created new sheets in the **KiCad 10 format** (`version 20260101`,
`(body_style ...)`, `(in_pos_files ...)` in every placed symbol). KiCad 9.0.9 then silently skips
those sheets: ERC "0 violations" and an empty netlist. Fixed by rewriting the headers to
`20250114` / `"9.0"` and removing those two tokens (the server only writes them into files that
declare KiCad 10), and by patching the server's `python/templates/*.kicad_sch` headers to KiCad 9
(redo after updating the server). Always check `kicad-cli sch export netlist` for a non-empty
component list, not only the ERC count.

## Revision history

(none yet)
