# esp-iradio KiCad project - notes

Design input: `../README.md` and `../01-..06-*.md`. This file records decisions, deviations and
the revision history of the actual KiCad project.

## Status

- 2026-10-11: empty KiCad 9.0.9 project created by the owner; JLCPCB Tools database downloaded.
  Olimex Rev.D hardware files fetched into `ref/` (git-ignored, re-fetch with
  `git clone --depth 1 --filter=blob:none --sparse https://github.com/OLIMEX/ESP32-DevKit-LiPo.git`
  + `git sparse-checkout set HARDWARE` inside `ref/`). Header pinout, row spacing and the +5V power
  path verified against the Rev.D netlist (see `../02-pinout-and-nets.md`, `../04-power.md`).

## Schematic plan (next step, Rev A)

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

(none yet)

## Revision history

(none yet)
