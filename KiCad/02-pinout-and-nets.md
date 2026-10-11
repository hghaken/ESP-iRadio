# 02 - Pinout and nets

All GPIO numbers are the ones the firmware uses (`src/config.h`, `docs/speaker-output.md`,
`docs/display-research.md`). Net names are suggestions; keep them in the schematic.

## Olimex ESP32-DevKit-LiPo headers

Two 1x19 rows, 2.54 mm pitch: EXT1 (left) and EXT2 (right) seen from the top with the antenna
up and the USB connector down (pinout drawing `docs/ESP32-DevKit-Lipo-GPIOs.png`). **Verify the
order, the row spacing and pin 1 against the Olimex Rev.D KiCad files before routing**: the
drawing in the repository shows an older revision with a WROOM module; the radio has the WROVER
(rev D), where GPIO16/17 belong to the PSRAM.

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
| 19 | +5V | `+5V_USB` | USB 5 V from the Olimex board (only present when USB is powered) |

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
| 15 | GPIO2 | `AMP_SD` | amplifier shutdown, high = amplifier on. Strapping pin: **must be low or floating at boot** -> 100k pull-down only, never a pull-up |
| 16 | GPIO15 | - | **leave open**: strapping pin (boot log) |
| 17 | GPIO8 | - | not connected (internal flash) |
| 18 | GPIO7 | - | not connected (internal flash) |
| 19 | GPIO6 | - | not connected (internal flash) |

Notes:
- The Olimex drawing says GPIO0-GPIO3 connect to the CH340 USB serial converter. GPIO2 is used here
  only as an output after boot; with the 100k pull-down it stays low during reset, which is what
  the boot ROM needs for serial download. Check the Olimex Rev.D schematic that nothing else
  drives GPIO2.
- No free GPIO is left apart from GPIO34/36 (inputs). Any new function needs a firmware change and
  a pin from someone else.

## Connectors on the carrier

### J_LCD: display module (1x8, 2.54 mm, order as printed on the module)

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

### U_DAC: PCM5102A

Module variant (GY-PCM5102 "purple board"; verify its header order on the actual module):

| Module pin | Net | Note |
|---|---|---|
| VIN | `+3V3` (see `04-power.md`: `+5V_USB` is absent on battery) | module has its own 3.3 V LDO; at ~15 mA its dropout is a few tens of mV |
| GND | `GND` | |
| LCK | `I2S_LRCK` | |
| DIN | `I2S_DATA` | |
| BCK | `I2S_BCK` | |
| SCK | `GND` | **system clock unused: must be tied to GND, otherwise no sound** |
| FLT / DEMP / XSMT / FMT | L / L / **H** / L | solder jumpers on the module, check them |
| Out L / R / G | `LINE_L` / `LINE_R` / `AGND` | to J_LINE and to the amplifier input |

Chip variant (PCM5102APWR, TSSOP-20): follow the reference circuit in the TI PCM510xA datasheet
(take every value from there): SCK to GND, XSMT to 3V3 via 10k (no GPIO is free for a mute),
FMT/FLT/DEMP to GND, the charge-pump capacitor on CAPP/CAPM, decoupling on AVDD/DVDD/CPVDD, the
LDOO capacitor, and an RC on each output to the jack.

### J_LINE: 3.5 mm stereo jack (line out)

Tip = `LINE_L`, ring = `LINE_R`, sleeve = `AGND`. Optional: switched jack whose switch mutes the
amplifier (not needed, the firmware decides).

### U_AMP: PAM8403

| Pin | Net | Note |
|---|---|---|
| VDD / PVDD | `+VAMP` | see `04-power.md` (5 V USB, or battery) |
| GND / PGND | `GND` (power ground, see grounding) | |
| INL / INR | `LINE_L` / `LINE_R` via 1 uF coupling caps (modules have them) | |
| SHDN (or SD) | `AMP_SD` | 100k pull-down to GND; high = on |
| MUTE (chip only) | `+VAMP` via 100k (not muted) | |
| OUTL+ / OUTL- | `SPK_L+` / `SPK_L-` | bridged: neither side may touch GND |
| OUTR+ / OUTR- | `SPK_R+` / `SPK_R-` | |

### J_SPK_L, J_SPK_R: speaker outputs

2-pin each (screw terminal 5.0/3.5 mm or JST-PH 2-pin): pin 1 = `SPK_x+`, pin 2 = `SPK_x-`.

### J_SPARE: spare header (1x5, 2.54 mm)

`+3V3`, `SPARE_IN34`, `SPARE_IN36`, `GND`, `GND`. Footprints for 10k pull-ups on both inputs
(do not populate by default).
