# 02 - Pinout and nets

> **Status 2026-10-15:** written for phase 1 (Olimex headers EXT1/EXT2). The GPIO numbers and
> functions stay the same on the integrated board; its full GPIO map (incl. the new GPIO0, EN,
> UART, CHG/PGOOD and sense inputs) is in `07-integrated-board.md` and the schematic Rev B.

All GPIO numbers are the ones the firmware uses (`src/config.h`, `docs/speaker-output.md`,
`docs/display-research.md`). Net names are suggestions; keep them in the schematic.

## Olimex ESP32-DevKit-LiPo headers

Two 1x19 rows, 2.54 mm pitch: EXT1 (left) and EXT2 (right) seen from the top with the antenna
up and the USB connector down. **Verified 2026-10-11 against the Olimex Rev.D netlist and board
file** (`ESP32-DevKit-Lipo_Rev_D.net` / `.kicad_pcb`, fetched into the git-ignored
`KiCad/esp-iradio/ref/`): the pin order below is exact; the rows are **25.40 mm apart** (centre to
centre); each row runs the full board length, **pin 1 at the antenna end**, pin 19 at the USB end.
The radio has the WROVER (GPIO16/17 belong to the PSRAM).

### EXT1 (left), top to bottom

| Pin | Signal | Carrier net | Use |
|---|---|---|---|
| 1 | +3.3V | `+3V3` | supply for display, encoder pull-ups, microSD (out of the Olimex regulator) |
| 2 | ESP_EN | - | not connected (reset is on the Olimex board) |
| 3 | GPI36 | `SPARE_IN36` | spare, input only, no pull-up |
| 4 | GPI39 | - | **leave open**: USB voltage sense on the Olimex board |
| 5 | GPI34 | `SPARE_IN34` | spare, input only, no pull-up |
| 6 | GPI35 | - | **leave open**: battery voltage sense on the Olimex board |
| 7 | GPIO32 | `ENC_A` | encoder A |
| 8 | GPIO33 | `ENC_B` | encoder B |
| 9 | GPIO25 | `ENC_SW` | encoder push button (to GND when pressed) |
| 10 | GPIO26 | `LCD_BL` | display backlight (PWM, high = on) |
| 11 | GPIO27 | `I2S_BCK` | I2S bit clock to the DAC |
| 12 | GPIO14 | `I2S_LRCK` | I2S word select to the DAC |
| 13 | GPIO12 | - | **leave open**: strapping pin (flash voltage), must be low at boot |
| 14 | GND | `GND` | |
| 15 | GPIO13 | `I2S_DATA` | I2S data to the DAC |
| 16 | GPIO9 | - | not connected (internal flash) |
| 17 | GPIO10 | - | not connected (internal flash) |
| 18 | GPIO11 | - | not connected (internal flash) |
| 19 | +5V | `+5V_OLI` | Olimex 5 V rail: from its micro-USB, or fed by the carrier's USB-C through the ideal diode U3 (decision D7) |

### EXT2 (right), top to bottom

| Pin | Signal | Carrier net | Use |
|---|---|---|---|
| 1 | GND | `GND` | |
| 2 | GPIO23 | `SPI_MOSI` | VSPI MOSI: display SDA + microSD DI |
| 3 | GPIO22 | `LCD_DC` | display data/command |
| 4 | GPIO1 | - | not connected (USB serial TX) |
| 5 | GPIO3 | - | not connected (USB serial RX) |
| 6 | GPIO21 | `LCD_RST` | display reset |
| 7 | GND | `GND` | |
| 8 | GPIO19 | `SPI_MISO` | VSPI MISO: microSD DO only (the display has no MISO) |
| 9 | GPIO18 | `SPI_SCK` | VSPI clock: display SCL + microSD CLK |
| 10 | GPIO5 | `LCD_CS` | display chip select (strapping pin, high at boot: add 10k pull-up) |
| 11 | GPIO17 | - | **leave open**: PSRAM on the WROVER |
| 12 | GPIO16 | - | **leave open**: PSRAM on the WROVER |
| 13 | GPIO4 | `SD_CS` | microSD chip select (10k pull-up so the card is deselected at boot) |
| 14 | GPIO0 | - | **leave open**: boot strapping, used by the USB auto-reset |
| 15 | GPIO2 | `AMP_SD` | MAX98357A SD_MODE: firmware drives it **low = amplifier off** and releases it (input) = on. Strapping pin: must be low or floating at boot - the 1M pull-up to `+VAMP` against the chip's internal 100k pull-down gives ~0.45 V, which reads low |
| 16 | GPIO15 | - | **leave open**: strapping pin (boot log) |
| 17 | GPIO8 | - | not connected (internal flash) |
| 18 | GPIO7 | - | not connected (internal flash) |
| 19 | GPIO6 | - | not connected (internal flash) |

Notes:
- On the Olimex board GPIO0 and GPIO2 go to the anodes of a BAT54C (D3, common cathode via 220 Ohm
  to the auto-reset circuit): the USB-serial converter can pull both low for serial download but
  never drives them high (verified in the Rev.D netlist). GPIO2 is used here only as an output
  after boot; the 100k pull-down keeps it low during reset, as the boot ROM needs.
- No free GPIO is left apart from GPIO34/36 (inputs). Any new function needs a firmware change and
  a pin from someone else.

## Connectors on the carrier

### J_LCD: display module on the board (1x8, 2.54 mm, order as printed on the module)

| Pin | Module | Net |
|---|---|---|
| 1 | GND | `GND` |
| 2 | VCC | `+3V3` (3.3 V logic module; do not use 5 V) |
| 3 | SCL | `SPI_SCK` |
| 4 | SDA | `SPI_MOSI` |
| 5 | RES | `LCD_RST` |
| 6 | DC | `LCD_DC` |
| 7 | CS | `LCD_CS` |
| 8 | BLK | `LCD_BL` |

### SW_ENC: EC11 rotary encoder with switch

| Pin | Net | Circuit |
|---|---|---|
| A | `ENC_A` | 10k pull-up to `+3V3`, 10k series + 10 nF to GND (or 100 nF directly, as in `docs/display-research.md`) |
| B | `ENC_B` | same as A |
| C (common) | `GND` | |
| S1 / S2 (switch) | `ENC_SW` / `GND` | 10k pull-up to `+3V3`, 100 nF to GND |

The firmware reads A/B with the ESP32 PCNT unit (hardware glitch filter), so light RC filtering is
enough. Keep the encoder traces away from the I2S lines.

### J_SD: microSD socket (SPI mode)

| Card pin (SPI mode) | Net |
|---|---|
| CS (DAT3) | `SD_CS` (10k pull-up to `+3V3`) |
| DI (CMD) | `SPI_MOSI` |
| CLK | `SPI_SCK` |
| DO (DAT0) | `SPI_MISO` (10k pull-up to `+3V3`) |
| VDD | `+3V3` (10 uF + 100 nF close to the socket) |
| VSS | `GND` |
| DAT1, DAT2 | 10k pull-ups to `+3V3` (unused in SPI mode, must not float) |
| Card detect (if the socket has it) | spare: `SPARE_IN34` via a jumper, or leave open |

### U_DAC: PCM5102APWR (TSSOP-20, on the board)

Follow the reference circuit in the TI PCM510xA datasheet; take every component value from there.

| Pin / function | Net | Note |
|---|---|---|
| BCK | `I2S_BCK` | |
| LRCK | `I2S_LRCK` | |
| DIN | `I2S_DATA` | |
| SCK (system clock) | `GND` | **unused: must be tied to GND**, the PLL then runs from BCK (otherwise no sound) |
| XSMT | `+3V3` via 10k | soft mute off (no GPIO free to drive it) |
| FMT, FLT, DEMP | `GND` | I2S format, normal filter, de-emphasis off |
| AVDD, DVDD, CPVDD | `+3V3` (own decoupling per pin; a ferrite bead or 10 Ohm from `+3V3` to AVDD is a good idea) | ~15 mA |
| CAPP / CAPM, VNEG, LDOO | capacitors per datasheet | charge pump for the ground-centred output |
| OUTL / OUTR | `LINE_L` / `LINE_R` via the datasheet output RC | 2.1 Vrms, ground-centred: no DC-blocking caps needed to the jack |
| AGND / DGND / CPGND | `GND` (see grounding in `04-power.md`) | |

### J_LINE: 3.5 mm stereo jack (line out)

Tip = `LINE_L`, ring = `LINE_R`, sleeve = `GND` (at the DAC's ground). No switch needed: the
firmware decides between speaker and line out.

### U_AMP: mono class-D amplifier (MAX98357A, since 2026-10-15)

**MAX98357AETE+T** (Analog Devices/Maxim, TQFN-16 3x3 mm, KiCad `Audio:MAX98357A`): I2S input with
its own DAC, mono (L+R)/2, 3.2 W into 4 Ohm at 5 V, 2.5-5.5 V. It listens to the same I2S lines as
the PCM5102A (one transmitter, two receivers), so the line output and the speaker play the same;
the firmware's software volume sets both. Replaces the analog PAM8302A route of the first draft
(that needed the mono sum and input network below the DAC). Tested 2026-10-15 on a breakout board
(firmware v1.37, web checkbox "Speaker").

| Pin | Net | Note |
|---|---|---|
| DIN (1) | `I2S_DATA` (GPIO13) | shared with the PCM5102A |
| BCLK (16) | `I2S_BCK` (GPIO27) | shared |
| LRCLK (14) | `I2S_LRCK` (GPIO14) | shared |
| SD_MODE (4) | `AMP_SD` (GPIO2) + 1M to `+VAMP` | low = shutdown; released = (L+R)/2 (the 1M against the internal 100k pull-down gives ~0.45 V); never drive it to 3.3 V (that selects the left channel only) |
| GAIN_SLOT (2) | open | 9 dB (to GND 12 dB, to VDD 6 dB, 100k to GND 15 dB, 100k to VDD 3 dB) |
| VDD (7, 8) | `+VAMP` | 10 uF + 100 nF at the pins, 220 uF bulk nearby |
| GND (3, 11, 15) + EP (17) | `GND` | exposed pad to the ground plane with vias |
| OUTP / OUTN (9, 10) | `SPK+` / `SPK-` | bridged (BTL): neither side may touch GND; short-circuit and thermal protection in the chip; optional ferrite + capacitor per output for EMI with long speaker leads |
| NC (5, 6, 12, 13) | - | not connected |

### J_SPK: speaker output

JST-PH 2-pin (or a 2-pin terminal): pin 1 = `SPK+`, pin 2 = `SPK-`. Speaker 4 or 8 Ohm at the
user's choice (decision D4), rated >= 3 W; the amplifier drives both (4 Ohm louder, more current).

### J_USBC: USB-C power input

Power-only USB-C receptacle (6-pin power type or a 16-pin USB 2.0 type with the data pins left
open).

| Pin | Net | Note |
|---|---|---|
| VBUS (all) | `VBUS_C` | through a polyfuse (2-3 A hold) to `+5V_USBC`; TVS diode (5 V class) at the connector. `+5V_USBC` feeds the amplifier and, via the ideal diode U3, the Olimex +5V pin (`+5V_OLI`): radio + Li-Po charging (decision D7) |
| CC1, CC2 | `GND` via **5.1k each** (separate resistors) | identifies the board as a sink: a USB-C source then gives 5 V, up to 3 A if it can |
| GND, shell | `GND` | shell to GND directly or via 1 M // 4.7 nF |
| D+ / D- | not connected | firmware updates stay on the Olimex micro-USB |

### J_SPARE: spare header (1x5, 2.54 mm)

`+3V3`, `SPARE_IN34`, `SPARE_IN36`, `GND`, `GND`. Footprints for 10k pull-ups on both inputs
(do not populate by default).
