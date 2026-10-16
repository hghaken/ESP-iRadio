# 06 - Review checklist and bring-up

## Integrated board (Rev B, 2026-10-15)

Before the layout (details in `esp-iradio/NOTES.md`):
- [x] Schematic Rev B: ERC 1 known warning (MAX98357A exposed pad), netlist checked with
      `esp-iradio/tools/intcheck.py` against `07-integrated-board.md`, GPIOs compared with Rev A.
- [x] Review by Grok and Claude (2026-10-15, `grok-bevindingen.md` / `claude-bevindingen.md`),
      agreed changes in the schematic and the libraries.
- [ ] PJ-320D (J5) pin order S/R/T against a sample (the Shouhan drawing only says OA-OD).
- [ ] MSK12C02 (SW203): which slide position is ON, on a sample (for the silk).
- [ ] EC11 shaft length for the front panel ~15 mm above the board (D16); update SW1's part.
- [ ] Display module HS20S010B (C5329586): CN1 pin 1 position on the delivered module; pin
      headers included or two C492409 ordered with it; standoff height (~11 mm) measured;
      GPIO5 (pin 7) selects the display and GPIO26 (pin 8) switches the backlight.
- [ ] TS-1187A pads 1 and 4 diagonal (LCSC drawing).

Layout:
- [x] First layout (Rev B draft, 2026-10-15): DRC 0 errors, 0 unconnected, 0 schematic parity
      issues; "+"/"-" silk at J202 and J6; recovery pads labelled; U.FL at the left edge.
- [x] Layout reviewed by Grok and Claude (round 4-5): placement, U.FL at the left edge, power
      widths, USB lengths, I2S detours accepted.
- [x] BQ24074 5 thermal vias, WROVER ground-land vias (`tools/touchup.py`); LED labels "C"/"P".
- [x] 2026-10-16: board 90 x 60 mm for the HS20S010B module on sockets, encoder beside it, U2
      turned 180 degrees; DRC 0 errors, 0 unconnected, 0 parity issues.
- [x] Review of the 90 x 60 board by Grok and Claude (round 6): no layout change.
- [x] 2026-10-16: **Rev C for hand assembly**: 1206 passives, RCA L/R (J8), H8, 122 x 76 mm with
      display / encoder / connectors from top to bottom; DRC 0 errors, 0 unconnected, 0 parity.
- [x] Review of the Rev C board by Grok and Claude (round 7): switch moved off the USB-C, two
      1206 pairs opened, F201 nudged, RCA pinout confirmed from the maker's drawing; DRC clean.
- [ ] RCA J8 pinout (1/3 = shell/GND, 2/4 = centre) checked with a meter on the delivered part.
- [x] Parts spread for hand soldering (>= 1.0 mm between courtyards, board 122 x 76 mm); DRC clean.
- [x] Review of the spread board by Grok (round 8): nothing to move; optional: C207 to the ADC pin.
- [ ] A last look at the routing in KiCad by the owner.

Before ordering:
- [x] DRC clean with the JLCPCB 4-layer rules after the hand changes.
- [x] BOM, CPL and Gerbers in `esp-iradio/jlcpcb/` (`tools/jlcfab.py`, 2026-10-16): every placed
      part has an LCSC number and is in stock; plain R/C mapped to basic parts in the script.
- [ ] SW1: the EC11 part (shaft length) chosen and its `LCSC` field set (not placed until then).
- [x] The rotation of every part checked in JLCPCB's placement preview (2026-10-16, six part
      types corrected in `tools/jlcfab.py`, `PART_CORRECTIONS`).
- [x] Assembly scope decided 2026-10-16: hand assembly by the owner (paste, stencil, heat gun).
- [ ] Order: bare PCBs + stencil at JLCPCB (Gerber zip), parts at LCSC
      (`esp-iradio/jlcpcb/esp-iradio_lcsc_order.csv` + the loose parts in the README there).
- [ ] STEP and DXF exported for the case; knob, display window and connectors line up.

Bring-up (new board):
0. Power the board from a source of at least 1.5 A (power bank or charger; not a laptop port:
   ILIM mode has no VIN-DPM).
1. Without cell, switch off: USB-C in, measure VBUS, VSYS (~4.4 V), +3V3 near 0 V (no back-feed
   through the CH340C or GPIO39); green LED on.
2. Switch on: +3V3 and +3V3A at 3.3 V, current below ~50 mA without firmware.
3. PC sees the CH340; `esptool flash-id` works (auto-reset), 16 MB flash.
4. Full flash with a 16 MB partition table (ask the owner: new board, settings start empty),
   then the firmware with `PWR_CAL_PERMILLE` ~2500 and the GPIO34/36 charge state.
5. Display, backlight PWM (>= 20 kHz), microSD, encoder, line out, speaker, WiFi with the antenna.
6. Cell (watch polarity; the P-FET blocks a reversed cell): plugging it in must not trip the
   protection (VSYS ~31 uF); charging with the red LED, about 0.8 A, the radio running from USB;
   then battery only and the switch.

## Carrier board (phase 1, superseded)

### Before ordering (design review)

- [ ] Every net in `02-pinout-and-nets.md` exists in the schematic with the same GPIO; compare with
      `src/config.h` (`TFT_CS 5`, `TFT_DC 22`, `TFT_RST 21`, `TFT_BL 26`, `SD_CS 4`) and
      `docs/speaker-output.md` (I2S 27/14/13, amplifier shutdown 2).
- [ ] Header pin 1, row order and row spacing checked against the Olimex Rev.D KiCad file (not
      only against the PNG drawing).
- [ ] GPIO35, GPIO39, GPIO12, GPIO0, GPIO15, GPIO16, GPIO17, GPIO6-11, GPIO1, GPIO3 and ESP_EN are
      not connected on the carrier.
- [ ] GPIO2 (`AMP_SD`) only has the 1M to `+VAMP` (with the MAX98357A's internal 100k it reads low
      at boot); GPIO5 (`LCD_CS`) and GPIO4 (`SD_CS`) have pull-ups.
- [ ] PCM5102A SCK tied to GND; XSMT high; FMT, FLT, DEMP low; every value from the TI datasheet.
- [ ] Amplifier (MAX98357A): DIN/BCLK/LRCLK on the same I2S nets as the PCM5102A, SD_MODE on
      `AMP_SD` with 1M to `+VAMP`, GAIN_SLOT open (9 dB), exposed pad to GND; speaker outputs not
      connected to GND anywhere.
- [ ] The amplifier is supplied from `+5V_USBC` only, not from `+3V3`.
- [ ] USB-C: 5.1k on CC1 and CC2 separately (not one shared resistor), polyfuse, TVS, ideal diode
      U3 to the Olimex +5V pin; Olimex Rev.D schematic checked: its USB VBUS has a diode to the +5V
      rail and the +5V pin feeds the charger and the 3.3 V regulator.
- [ ] Speaker path dimensioned for 4 Ohm (the heavier case): `+VAMP` and speaker traces >= 0.8 mm.
- [ ] No copper/parts under the WROVER antenna (or the antenna overhangs the edge).
- [ ] USB connector, Li-Po JST and reset button reachable with the Olimex board plugged in.
- [ ] ERC clean (or every remaining warning explained in `NOTES.md`), DRC clean with the fab rules.
- [ ] 3D view: heights under the Olimex board, display and encoder shaft against the enclosure.
- [ ] BOM: every SMD part has an `LCSC` code that is in stock at JLCPCB; CPL rotations checked in
      the JLCPCB preview.
- [ ] STEP export complete (all 3D models present) and DXF outline handed to the case design.
- [ ] Owner has answered the open questions in `01-requirements.md` and approved the layout
      (screenshot of the 3D view and the front panel positions).

### Bring-up (first board)

1. Visual check, then measure resistance `+3V3`-`GND`, `+5V_USBC`-`GND`, `+VAMP`-`GND`: no short.
2. Without the Olimex board: apply 3.3 V from a lab supply with current limit (100 mA) to `+3V3`;
   current should be < 5 mA (pull-ups only).
3. Plug in the Olimex board, display, nothing else. Power over USB. Expect the normal boot: the
   display shows the radio screen, the web page answers (`/api/status`), audio over Bluetooth.
4. microSD: insert a FAT32 card, set `SD_ENABLED 1` in `src/config.h`, update the firmware, check
   the web page's SD card section (`AGENTS.md`, `src/sdcard.*`).
5. Encoder: once the firmware reads it, turn and press; until then, check with a multimeter that
   A/B/SW pull to GND.
6. USB-C: plug in a USB-C charger without the Li-Po and without the micro-USB; the radio must
   boot from USB-C alone. Then with the Li-Po: the Olimex charge LED lights. Then with the PC on
   the micro-USB as well: measure that no current flows back into either port (USB meter).
7. DAC + amplifier: switch Bluetooth off on the web page (firmware v1.37+); line out first
   (headphones in the jack, low volume), then the speaker (USB-C plugged in, web checkbox
   "Speaker"). Check that `AMP_SD` is low while Bluetooth is on or "Speaker" is off (no hiss).
8. Long run: 1 h with the speaker on USB-C and 1 h on the battery (Bluetooth), watch `/api/status` (heap, underruns,
   brownouts in `diag`).

Record results and any rework (cut traces, bodge wires) in `KiCad/esp-iradio/NOTES.md`; fold
fixes into the next revision.
