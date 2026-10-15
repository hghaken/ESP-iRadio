# 07 - Integrated board (phase 2, chosen 2026-10-15)

The owner wants everything on one board: no Olimex dev board, no display module, no breakouts,
and **every part assembled by JLCPCB from stock** (D17). This replaces the phase 1 carrier board
(the A3 schematic of 2026-10-12/15 stays in git as the reference for every function that is
already verified on the breadboard).

None of the module schematics are needed: every block comes from a chip datasheet or reference
design. The Olimex ESP32-DevKit-LiPo is open hardware; its Rev.D KiCad files are in
`esp-iradio/ref/` (git-ignored) and show the proven USB, charger and regulator circuits.

Status: design input, reviewed 2026-10-15 (reviewer agent: architecture sound; its must-fix items
are worked in below). Schematic Rev B done the same evening (two A3 sheets, see
`esp-iradio/NOTES.md`); two values were corrected against the BQ24074 datasheet while drawing it
(ILIM, ITERM/SYSOFF, see Power). Reviewed by Grok and Claude the same evening
(`grok-bevindingen.md`, `claude-bevindingen.md` in the repository root); the agreed changes are
worked in: no bulk electrolytic on VSYS, polyfuse 2 A before the TVS, ILIM 1.13k, a high-ohmic USB
divider, recovery pads, NPTH locating holes, no VIN-DPM in ILIM mode (power the board from a
source of at least 1.5 A). Next: the checks listed in NOTES.md, then the layout.

## Owner decisions (2026-10-15)

| # | Decision | Choice |
|---|---|---|
| D8 | Display | the bare 1.69" IPS panel (ST7789V2, 240x280) soldered by its FPC to the board; backlight driver on the board |
| D9 | ESP32 | **ESP32-WROVER-IE-N16R8** module (16 MB flash, 8 MB PSRAM, U.FL connector for an external 2.4 GHz antenna; owner's choice 2026-10-15, was the -E with PCB antenna); a chip-down ESP32 is not wanted (antenna and RF tuning, no module certification, extended-part fees) |
| D10 | Battery charging | **power-path charger** (TI BQ24074): the radio runs from USB while the cell charges separately |
| D11 | Speaker | also on battery: the MAX98357A runs from the system rail (4.4 V on USB, the cell voltage on battery) |
| D12 | On/off switch | **slide switch on the 3.3 V LDO's enable**: off = the radio is off, also on USB; the cell still charges on USB. (The BQ24075/79 have a SYSOFF pin that would also stop charging; the BQ24074 has none.) |
| D13 | Display position | on the front of the board: panel on ~5 mm spacers (see D16), FPC folded to pads on the same side |
| D14 | Charge LEDs | red on CHG (charging), green on PGOOD (USB power); they light also while the radio is off |
| D15 | Board size | as small as reasonable, about 60 x 80 mm; the case is designed around the board |
| D16 | Encoder | **recessed**: nothing sticks out of the front panel. EC11 with push switch and a short shaft, a low knob (~15-18 mm across, ~6 mm high) in a round pocket (~22 mm) of the front panel, its top flush with the panel. The EC11 body is ~6.5 mm high, so the front panel sits at least that far above the board and the display goes on ~5 mm spacers to bring its glass right behind the window. |
| D17 | Assembly | **every part from the JLCPCB stock**, the whole board assembled by JLCPCB (SMT and THT). Only exceptions: the display panel (soldered by its FPC afterwards), the cell and the speaker |
| D18 | Battery protection | **on the board**: DW01A + FS8205A (over-discharge, over-current, short) and a P-MOSFET against a reversed cell; works also with a pack that has its own protection |

Earlier decisions that stay: one mono speaker 4-8 Ohm (D4), USB-C 5 V without PD (D5), JLCPCB with
SMT assembly (D6), 3D-printed case around the board, PCM5102A line out, MAX98357A speaker
amplifier, microSD, EC11 encoder.

## Block diagram

```
USB-C --+-- CC1, CC2: 5.1k each (sink)
        +-- D+/D- (A6/B6, A7/B7 tied) -- USBLC6-2 -- CH340C -- TXD0/RXD0 (GPIO1/3)
        |                                         +-- DTR/RTS -- auto-reset (2x NPN) -- EN, GPIO0
        +-- VBUS -- polyfuse -- SMAJ5.0A -- BQ24074 IN
                                              |  OUT = VSYS (4.4 V on USB, VBAT on battery)
                                              |    +-- AP7361C-33Y5 (EN = slide switch) -- +3V3
                                              |    |      ESP32-WROVER-IE, CH340C, display VDD + LEDA,
                                              |    |      PCM5102A DVDD/CPVDD, microSD, pull-ups
                                              |    +-- LP5907-3.3 -- +3V3A (PCM5102A AVDD)
                                              |    +-- MAX98357A VDD (10 uF + 100 nF)
                                              |    +-- charge LEDs (anodes)
                                              |  BAT -- P-FET (reverse cell) -- JST-PH 2 -- cell
                                              |         cell negative -- FS8205A (DW01A) -- GND
                                              |  CHG -> GPIO34, PGOOD -> GPIO36 (via BAT54)
        VIN -- 330k/220k (+100 nF) -> GPIO39 (USB voltage); VBAT -- 470k/470k (+100 nF) -> GPIO35
```

## Power

- **USB-C** (TYPE-C-31-M-12, 16 pins): one 5.1k per CC pin (never shared), D+/D- pairs tied
  (A6/B6, A7/B7), a polyfuse **2 A hold** (1206L200/16NR) and behind it the **SMAJ5.0A**
  (unidirectional: BQ24074 IN must not go below -0.3 V). Fuse first: a wrong adapter (above the
  TVS's ~6.4 V) then opens the fuse instead of burning the TVS into a short. One connector
  powers, charges and flashes.
- **USB source of at least 1.5 A (owner's decision 2026-10-15, option a).** In ILIM mode the
  BQ24074 has **no input-voltage regulation**: VIN-DPM only works in the USB100/USB500 modes
  (EN2 = 0, SLUS810N 9.3.4.1.1), and DPPM only cuts the charge current to keep OUT up. The board
  can therefore draw up to ~1.37 A (radio ~0.3 A + 0.8 A charging an empty cell); a 500 mA PC or
  laptop port may switch off or brown out, also while flashing. Charge and flash from a power bank
  or charger of at least 1.5 A (the power bank in use today). A USB500 option would need two
  jumpers switched together (EN1 to VSYS and EN2 to GND); not on this board.
- **Charger BQ24074RGTR** (VQFN-16 3x3):
  - OUT regulated to 4.4 V while USB is present (4.3-4.5 V), battery 4.2 V, input OVP 10.5 V;
    the system load comes first, the cell supplements above the input limit.
  - **EN1 to GND, EN2 to OUT** = ILIM mode (0/0 would be 100 mA, 1/1 USB suspend; EN2 not to IN:
    7 V abs max; not to +3V3: that rail can be off).
  - **ISET 1.1k** -> ~0.8 A charge (K_ISET ~890 A*Ohm): a 5000 mAh cell in ~6-7 h. 1 A would
    heat the VQFN by ~65-70 C in a closed case (input FET 5.0 -> 4.4 V plus charge FET).
  - **ILIM 1.13k** -> ~1.37 A input limit (K_ILIM ~1550 A*Ohm; the datasheet allows 1.1k-8k:
    1.05k was below it, and 1.1k at 1 % tolerance could be too).
  - **ITERM open** -> termination at the default 10 % (pin 15 of the BQ24074 is ITERM).
  - **TMR 68k** -> fast-charge safety timer ~9 h (open = 5 h default, which a 5000 mAh cell
    would hit: charging stops with a fault until USB is replugged).
  - **TS: 10k NTC (B ~3435) on the board where the cell lies** (with a 3-wire pack: its NTC).
    A plain 10k resistor would mean no temperature protection at all.
  - **CE to GND** (charging enabled). The BQ24074 has **no SYSOFF pin** (only the BQ24075/79);
    D12 does not need it.
  - Caps (datasheet): IN 1-10 uF -> 4.7 uF 25 V; OUT and BAT 4.7-47 uF -> 10 uF each.
  - Thermal pad with >= 5 vias into the GND plane; copper around it.
  - CHG and PGOOD are open-drain: LEDs (red CHG, green PGOOD) with their anodes on VSYS through
    1k, so they also light while the radio is off; each pin also reaches its GPIO through a
    **BAT54** (cathode at the charger pin) with **100k to +3V3** at the GPIO side, so an off
    +3V3 rail is not back-fed and the GPIO reads low when the charger pulls low.
- **3.3 V: AP7361C-33Y5-13** (1 A, SOT-89-5, **with EN**), from VSYS. EN = slide switch MSK12C02
  (SPDT: ON to VSYS, OFF to GND) plus 100k to GND. Dropout ~0.36 V at 1 A: a 3.45 V cell at a
  0.5 A WiFi/BT peak still gives ~3.15 V (ESP32 min 3.0 V; the firmware's power guard stops at
  3.45 V). ~0.3 W at 4.4 V in: SOT-89 with copper. Capacitors: 10 uF in, 10 uF at the LDO out,
  22 uF + 100 nF at the WROVER 3V3 pin. A buck-boost is not needed.
- **+3V3A for the DAC: LP5907-3.3** (TPLP5907MFX-3.3, 300 mA, SOT-23-5, low noise) from VSYS, EN
  tied to the LDO's EN net (off with the switch). Replaces the ferrite from the WiFi rail.
- **Speaker supply** = VSYS directly (MAX98357A 2.5-5.5 V): 4.4 V on USB (~2.4 W in 4 Ohm),
  3.5-4.2 V on battery (~1.3-1.8 W). 10 uF + 100 nF at the pins, **no bulk electrolytic**: VSYS
  is the BQ24074 OUT (4.7-47 uF allowed, ~31 uF in total now), and a large bulk would have to be
  charged through the FS8205A when a cell is plugged in (the DW01A may read that as a short). The
  cell supplements the peaks. Its SD_MODE pull-up
  (1M) goes to **+3V3** (not VSYS): with the switch off the amplifier is then really off
  (VSYS would leave ~0.3 V on SD_MODE and keep it on). 1M against the internal 100k gives 0.30 V,
  inside the (L+R)/2 window (0.16-0.77 V); GPIO2 low = off.
- **Battery protection (D18)**: DW01A + FS8205A in the cell's negative lead (standard one-cell
  protection: over-discharge ~2.4-2.5 V, over-charge 4.3 V, over-current, short). Reverse cell:
  a P-MOSFET (DMG2305UX) in the positive lead between the JST connector and the charger's BAT
  (gate to GND through 10k-100k, conducts both ways with a correct cell, blocks a reversed one).
  Big +/- on the silkscreen next to the JST-PH (pin order is not standardised between vendors).
- **Measurement**: VBAT -> 470k/470k -> GPIO35 with **100 nF** at the pin (the source is too
  high-ohmic for the ADC sample capacitor; the Olimex board has the same). VIN (after the fuse) ->
  **330k/220k with 100 nF** -> GPIO39 (x0.4: 2.0 V at 5 V, inside the linear 11 dB range).
  High-ohmic on purpose: with the switch off and USB in, the ESP32 is unpowered and the divider
  would push current through the pin's protection diode into the dead 3V3 rail (33k/22k: ~0.1 mA;
  now ~10 uA). Firmware: `PWR_CAL_PERMILLE` ~2500 (was 1457), `BAT_CAL_PERMILLE` stays.
  PGOOD and CHG give "on USB" and "charging" without the ADC.

## USB serial and boot

- **CH340C** (SOP-16, internal clock) on D+/D- (after the USBLC6-2), supplied from +3V3 with V3
  tied to VCC plus 100 nF; its TXD to GPIO3 (RXD0), RXD to GPIO1 (TXD0); 3.3 V levels, so no
  isolation diodes (the Olimex needed them because its CH340X ran from 5 V). With the switch off
  the CH340C is unpowered too (flashing needs the radio switched on). The PC sees the same CH340
  as today: `scripts/flash_usb.ps1` and the firmware's USB update keep working.
- **Auto-reset**: the standard two-NPN circuit (DTR/RTS -> EN and GPIO0, MMBT3904); EN with 10k
  pull-up and 1 uF to GND; GPIO0 with 10k pull-up.
- **Buttons**: RESET (EN) and BOOT (GPIO0), TS-1187A SMD tact switches (recovery only).
- **Recovery pads** TP203-TP207 (EN, IO0, TXD0, RXD0, GND; +3V3 is TP4): flash the module with a
  3.3 V serial adapter when the USB-C connector or the CH340C is dead (slide switch on).

## ESP32-WROVER-IE-N16R8

- Project symbol `ESP32-WROVER-IE_iRadio`, footprint `lcsc:ESP32-WROVER-IE_SMD` (LCSC C701352;
  same pads and outline as the WROVER-E). No PCB antenna, so no antenna keep-out at a board edge:
  a 2.4 GHz antenna with an IPEX/U.FL pigtail (bought separately, not assembled) plugs into the
  module and goes to the case wall. Keep the U.FL connector reachable (no tall parts next to it,
  room for the plug); never transmit without an antenna connected.
- Strapping (reviewed): GPIO12 (MTDI) **must be low at boot** (high selects 1.8 V flash): nothing
  connected (optionally burn the VDD_SDIO eFuse to 3.3 V). GPIO0 10k up; GPIO2 low at reset (its
  internal pull-down beats the MAX98357A network, also in download mode); GPIO5 10k up (LCD_CS);
  GPIO15 open.
- PSRAM: 8 MB on the module, the ESP32 maps 4 MB directly (the rest only through the himem API):
  same as today for the firmware. Flash: 16 MB -> a new partition table with larger OTA slots
  (one full flash over USB on a new board; settings start empty).
- **PCB: 4 layers** (layer 2 a solid GND plane): audio and the charger's heat. Cheap at JLCPCB.

## Display: bare panel on the board (D8)

Panel class: 1.69" IPS, ST7789V2, 240x280, 4-wire SPI, **12-pin FPC, 0.7 mm pitch, soldered**
(not a ZIF connector). Reference: Unvision UN169T-IG01 (the panel of the lcdwiki MSP1691 module,
same pin order as the owner's ZJY169S0800TG01 module; confirm on the panel that is ordered).

| FPC pin | Signal | Board net |
|---|---|---|
| 1 | GND | GND |
| 2 | LEDK | backlight cathode -> driver |
| 3 | LEDA | `+3V3` (+ 10 uF) |
| 4 | VDD | `+3V3` (2.5-3.3 V, ~8 mA) + 100 nF + 10 uF |
| 5, 6 | GND | GND |
| 7 | D/C | `LCD_DC` GPIO22 |
| 8 | CS | `LCD_CS` GPIO5 (10k pull-up) |
| 9 | SCL | `SPI_SCK_BUS` (after R8 22 Ohm) GPIO18 |
| 10 | SDA | `SPI_MOSI_BUS` (after R9) GPIO23 |
| 11 | RESET | `LCD_RST` GPIO21 |
| 12 | GND | GND |

- Backlight: 3 white LEDs in parallel, Vf 2.8-3.2 V, 60 mA typical (90 mA max). Low-side driver as
  on the module but with a logic-level N-MOSFET: LEDK -> R_BL -> AO3400A drain, source GND, gate =
  `LCD_BL` GPIO26 (PWM) with 100k to GND (dark until the firmware runs). **R_BL in 0603, value
  from the real panel's Vf** (5.6 Ohm on the module; from 3.3 V that gives 18-89 mA across the
  Vf spread): measure the delivered panel before ordering. Firmware: backlight PWM to >= 20 kHz
  (5 kHz current pulses on +3V3 can be heard through the amplifier).
- Pads: 12 x 0.7 mm pitch, ~0.4 x 2.0 mm, in a row matching the FPC end (11.4 mm wide), soldered
  by hand (or hot-bar) after the assembly. A panel on the front with its FPC folded back mirrors
  the contact side: **check pin 1 and the contact side with a paper mock-up** before the pads are
  placed. Panel 30.07 x 37.43 x 1.56 mm, active area 27.97 x 32.63 mm, FPC bend zone 10 mm (see
  the drawing in the panel spec).
- Sources: lcdwiki "1.69inch IPS Module" (MSP1691) with the panel spec SPEC_UN169T-IG01 and the
  module schematic (QDtech), 2026-10-15.

## Unchanged blocks (verified on the breadboard)

- PCM5102APWR line out (I2S BCK GPIO27, LRCK GPIO14, DATA GPIO13; SCK to GND, XSMT pull-up, FMT/
  FLT/DEMP low; 470R + 2.2 nF per output; 3.5 mm jack), AVDD now from the LP5907 (`+3V3A`).
- MAX98357A speaker amplifier on the same I2S nets; SD_MODE = `AMP_SD` GPIO2 with 1M to +3V3,
  GAIN open (9 dB); speaker on a JST-PH 2.
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
| 26 | `LCD_BL` | backlight PWM (>= 20 kHz) |
| 34 | `CHG` (BQ24074, via BAT54) | input only, 100k pull-up to +3V3 |
| 35 | battery voltage | 470k/470k + 100 nF |
| 36 | `PGOOD` (BQ24074, via BAT54) | input only, 100k pull-up; ESP32 errata: glitches low ~80 ns while ADC/WiFi run -> poll with debounce, no interrupt |
| 39 | USB voltage | 330k/220k + 100 nF |

Firmware changes for this board: `PWR_CAL_PERMILLE` ~2500, charge state from GPIO34/36 (polled),
backlight PWM >= 20 kHz, a 16 MB partition table; pins, I2S, SPI and encoder stay.

## Parts and JLCPCB stock (D17; JLCPCB Tools database of 2026-10-11)

| Part | LCSC | Stock | Note |
|---|---|---|---|
| ESP32-WROVER-IE-N16R8 | C701352 | 1970 | extended; U.FL antenna (was -E C529589) |
| BQ24074RGTR | C54313 | 816 | extended; fallback BQ24073RGTR C15220 (33782 in stock, same 4.4 V OUT, 6.6 V OVP, TD pin instead of ITERM) |
| CH340C | C84681 | 143702 | |
| MAX98357AETE+T | C910544 | 28887 | |
| PCM5102APWR | C107671 | 1570 | |
| AP7361C-33Y5-13 (SOT-89-5, EN) | C460397 | 5968 | 3.3 V 1 A |
| TPLP5907MFX-3.3 | C5370990 | 194731 | DAC supply |
| DW01A | C351410 | 117507 | cell protection |
| FS8205A | C2830320 | 55880 | protection FETs |
| DMG2305UX-7 | C5261054 | 9017 | reverse-cell P-FET |
| BAT54WS (SOD-323) | C7502694 | 405154 | preferred; 2x, CHG/PGOOD isolation |
| LED red / green 0603 | C2286 / C12624 | 3941052 / 354847 | red basic, green extended |
| polyfuse 1206L200/16NR (2 A hold, 16 V) | C22374899 | 34396 | VBUS, before the TVS |
| AO3400A | C20917 | 1071707 | basic; backlight |
| MMBT3904 | C7420353 | 5024877 | preferred; auto-reset |
| USBLC6-2SC6 | C2827654 | 105627 | USB ESD |
| SMAJ5.0A | C113952 | 602831 | VBUS TVS |
| NTC 10k B3435 0603 (NCP18XH103F03RB) | C13564 | 206858 | TS |
| TYPE-C-31-M-12 | C165948 | 90528 | USB-C 16 pins |
| microSD push-push (TF-01A) | C91145 | 205737 | footprint to match |
| EC11E with switch, short shaft | e.g. C470754 EC11E15244B2 | 6196 | THT; confirm shaft length/type for D16 |
| MSK12C02 slide switch | C431540 | 166028 | on/off |
| TS-1187A-B-A-B | C318884 | 498821 | basic; RESET, BOOT |
| JST S2B-PH-K-S (side) / B2B-PH-K-S (top) | C173752 / C131337 | 193749 / 87147 | cell, speaker |
| PJ-320D 3.5 mm jack (SMD) | C431535 | 60364 | line out |
| R/C | basic parts | | LCSC numbers from JLCPCB Tools when the BOM is made |

Stock is a snapshot; JLCPCB Tools checks it again when the BOM is made. Not assembled: the display
panel (hand-soldered FPC), the cell, the speaker.

## Sources

- TI BQ24072/3/4/5/9 datasheet (ZHCSIF0N / SLUSA19): BQ24074 OUT 4.4 V typ (4.3-4.5 V), OVP 10.5 V;
  ISET/ILIM/TMR/ITERM/EN1-EN2/TS from the same datasheet (SLUS810N, checked in the schematic step).
- Panel: lcdwiki.com/1.69inch_IPS_Module (SPEC_UN169T-IG01.pdf, 1.69IPS_MSP1691_Schematic.pdf).
- Olimex ESP32-DevKit-LiPo Rev.D hardware files (github.com/OLIMEX/ESP32-DevKit-LiPo): CH340X,
  BL4054B charger, HX6211 3.3 V LDO.
- Espressif ESP32-WROVER-E / -IE datasheet and hardware design guidelines (strapping, antenna).
- JLCPCB parts: the JLCPCB Tools plugin database (`current-parts-fts5.db`, 2026-10-11).
