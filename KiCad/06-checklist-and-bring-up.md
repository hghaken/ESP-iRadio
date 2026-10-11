# 06 - Review checklist and bring-up

## Before ordering (design review)

- [ ] Every net in `02-pinout-and-nets.md` exists in the schematic with the same GPIO; compare with
      `src/config.h` (`TFT_CS 5`, `TFT_DC 22`, `TFT_RST 21`, `TFT_BL 26`, `SD_CS 4`) and
      `docs/speaker-output.md` (I2S 27/14/13, amplifier shutdown 2).
- [ ] Header pin 1, row order and row spacing checked against the Olimex Rev.D KiCad file (not
      only against the PNG drawing).
- [ ] GPIO35, GPIO39, GPIO12, GPIO0, GPIO15, GPIO16, GPIO17, GPIO6-11, GPIO1, GPIO3 and ESP_EN are
      not connected on the carrier.
- [ ] GPIO2 (`AMP_SD`) has a pull-down only; GPIO5 (`LCD_CS`) and GPIO4 (`SD_CS`) have pull-ups.
- [ ] PCM5102A SCK tied to GND; XSMT high; FMT, FLT, DEMP low; every value from the TI datasheet.
- [ ] Amplifier: SD pin pulled down (off at boot), mono sum of `LINE_L`/`LINE_R` into its input,
      gain low; speaker outputs not connected to GND anywhere.
- [ ] The amplifier is supplied from `+5V_USBC` only, not from `+3V3`.
- [ ] USB-C: 5.1k on CC1 and CC2 separately (not one shared resistor), polyfuse, TVS; if USB-C
      also feeds the Olimex +5V pin, the ideal diode blocks backfeed both ways (checked against
      the Olimex Rev.D schematic).
- [ ] No copper/parts under the WROVER antenna (or the antenna overhangs the edge).
- [ ] USB connector, Li-Po JST and reset button reachable with the Olimex board plugged in.
- [ ] ERC clean (or every remaining warning explained in `NOTES.md`), DRC clean with the fab rules.
- [ ] 3D view: heights under the Olimex board, display and encoder shaft against the enclosure.
- [ ] BOM: every SMD part has an `LCSC` code that is in stock at JLCPCB; CPL rotations checked in
      the JLCPCB preview.
- [ ] STEP export complete (all 3D models present) and DXF outline handed to the case design.
- [ ] Owner has answered the open questions in `01-requirements.md` and approved the layout
      (screenshot of the 3D view and the front panel positions).

## Bring-up (first board)

1. Visual check, then measure resistance `+3V3`-`GND`, `+5V_USBC`-`GND`, `+VAMP`-`GND`: no short.
2. Without the Olimex board: apply 3.3 V from a lab supply with current limit (100 mA) to `+3V3`;
   current should be < 5 mA (pull-ups only).
3. Plug in the Olimex board, display, nothing else. Power over USB. Expect the normal boot: the
   display shows the radio screen, the web page answers (`/api/status`), audio over Bluetooth.
4. microSD: insert a FAT32 card, set `SD_ENABLED 1` in `src/config.h`, update the firmware, check
   the web page's SD card section (`AGENTS.md`, `src/sdcard.*`).
5. Encoder: once the firmware reads it, turn and press; until then, check with a multimeter that
   A/B/SW pull to GND.
6. USB-C: plug in a USB-C charger; measure `+5V_USBC` and `+VAMP` (~5 V); with the Olimex
   micro-USB also plugged in, check that no current flows back into either port.
7. DAC + amplifier: with the I2S firmware (planned), switch Bluetooth off on the web page; line out
   first (headphones in the jack, low volume), then the speaker (USB-C plugged in). Check that
   `AMP_SD` is low while Bluetooth is on (no hiss from the speaker).
8. Long run: 1 h with the speaker on USB-C and 1 h on the battery (Bluetooth), watch `/api/status` (heap, underruns,
   brownouts in `diag`).

Record results and any rework (cut traces, bodge wires) in `KiCad/esp-iradio/NOTES.md`; fold
fixes into the next revision.
