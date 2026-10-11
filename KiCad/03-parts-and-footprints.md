# 03 - Parts and footprints

Assembly: **JLCPCB SMT assembly** (decision D6). Every SMD part needs an LCSC part number; prefer
JLCPCB "basic" parts (no extended-part fee) for passives, accept "extended" for the DAC, amplifier,
USB-C connector and microSD socket. **Look the LCSC codes up in the JLCPCB parts library and check
stock; do not copy codes from memory.** Put them in a symbol field named `LCSC` so the BOM export
carries them.

KiCad 8/9 standard library names where a fitting footprint exists. **Create** = project-local
footprint, from the datasheet or measured from the owner's part; record the source in
`KiCad/esp-iradio/NOTES.md`.

## Bill of materials (phase 1)

### Connectors and mechanical (through-hole: hand-solder, or JLCPCB THT assembly)

| Ref | Qty | Part | KiCad symbol | KiCad footprint | Notes |
|---|---|---|---|---|---|
| J1, J2 | 2 | Female header 1x19, 2.54 mm | `Connector_Generic:Conn_01x19` | `Connector_PinSocket_2.54mm:PinSocket_1x19_P2.54mm_Vertical` | socket for the Olimex board; positions from the Olimex Rev.D files |
| J3 | 1 | Display connection 1x8, 2.54 mm | `Connector_Generic:Conn_01x08` | `Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical` (plug-in) or `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` (module soldered) | display on the board (D2); add the module's mounting holes/standoffs if it has them |
| SW1 | 1 | Rotary encoder EC11 with switch, vertical | `Device:RotaryEncoder_Switch` | `Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` | shaft length to suit the case; check the footprint against the bought part |
| J4 | 1 | microSD socket, push-push, SMD | `Connector:Micro_SD_Card` (or `_Det`) | per socket, e.g. `Connector_Card:microSD_HC_Hirose_DM3AT-SF-PEJM5` or `Connector_Card:microSD_HC_Molex_104031-0811` | choose one that JLCPCB stocks, then its footprint |
| J5 | 1 | 3.5 mm stereo jack | `Connector_Audio:AudioJack3` | e.g. `Connector_Audio:Jack_3.5mm_PJ320D_Horizontal` | choose a stocked part, then its footprint |
| J6 | 1 | Speaker connector JST-PH 2-pin | `Connector_Generic:Conn_01x02` | `Connector_JST:JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical` | mono speaker (D4) |
| J7 | 1 | USB-C receptacle, power only (or USB 2.0 16-pin with data open) | `Connector:USB_C_Receptacle_PowerOnly_6P` (or `USB_C_Receptacle_USB2.0_16P`) | per part, e.g. `Connector_USB:USB_C_Receptacle_GCT_USB4125-xx-x_6P_TopMnt_Horizontal` | choose a stocked part; at a board edge |
| J8 | 1 | Spare header 1x5, 2.54 mm | `Connector_Generic:Conn_01x05` | `Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical` | may stay unpopulated |
| H1-H4 | 4 | Mounting holes M2.5 or M3 | `Mechanical:MountingHole` | `MountingHole:MountingHole_2.7mm_M2.5` or `_3.2mm_M3` | for heat-set inserts in the 3D-printed case |

### Active parts (SMD, assembled)

| Ref | Part | KiCad symbol | Footprint | Notes |
|---|---|---|---|---|
| U1 | PCM5102APWR (TI) | check the KiCad `Audio` library for a PCM510x symbol; else a project symbol | `Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm` | reference circuit and values from the TI PCM510xA datasheet |
| U2 | PAM8302AASCR (Diodes Inc., SOP-8) - or PAM8403 SOP-16 | project symbol if none in the library | `Package_SO:SOIC-8_3.9x4.9mm_P1.27mm` (PAM8403: `Package_SO:SOIC-16_3.9x9.9mm_P1.27mm`) | application circuit from the Diodes Inc. datasheet |
| U3 | Ideal diode / power mux for `+5V_USBC` -> Olimex `+5V` (only if open question 3 is "yes") | e.g. TI LM66100 (SC-70-6) or a P-MOSFET ideal-diode circuit | per part | blocks backfeed between the Olimex micro-USB and the USB-C input; see `04-power.md` |
| D1 | TVS 5 V (unidirectional) at VBUS | `Device:D_TVS` | SOD-123 / SMA per part | USB-C input protection |
| F1 | Polyfuse 2-3 A hold, 1206/1812 | `Device:Polyfuse` | `Fuse:Fuse_1812_4532Metric` (or 1206) | USB-C input |
| FB1 | Ferrite bead 600 Ohm @ 100 MHz, 0805 (optional) | `Device:FerriteBead` | `Inductor_SMD:L_0805_2012Metric` | `+3V3` -> DAC AVDD |

### Passives (0603 or 0805, JLCPCB basic parts where possible)

| Ref | Value | Use |
|---|---|---|
| R_ENC_A, R_ENC_B, R_ENC_SW | 10k | encoder pull-ups to `+3V3` |
| C_ENC_A, C_ENC_B, C_ENC_SW | 100 nF | encoder debounce to GND |
| R_LCD_CS, R_SD_CS, R_MISO, R_DAT1, R_DAT2 | 10k | pull-ups (LCD_CS strapping GPIO5; SD lines) |
| R_AMP_SD | 100k | pull-down on `AMP_SD` (GPIO2 strapping: never a pull-up) |
| R_XSMT | 10k | PCM5102A XSMT to `+3V3` |
| R_CC1, R_CC2 | 5.1k 1 % | USB-C CC pull-downs (sink) |
| R_SUM_L, R_SUM_R | 10k | mono sum `LINE_L`/`LINE_R` -> `AMP_IN` |
| R_SPI_SCK, R_SPI_MOSI | 0 Ohm (22-33 Ohm if ringing) | series at the ESP32 end |
| R_SPARE34, R_SPARE36 | 10k, DNP | optional pull-ups |
| DAC caps | per TI datasheet | decoupling, charge pump, LDOO, output RC |
| Amp caps | per Diodes datasheet | input coupling, 10 uF + 1 uF supply |
| C_BULK | 220-470 uF / 10 V (electrolytic SMD or polymer) | `+VAMP` bulk near the amplifier |
| C_SD | 10 uF + 100 nF | microSD supply |
| C_LCD | 10 uF | display supply at J3 |
| C_VBUS | 10 uF | USB-C input after the fuse |

### Test points

`Connector:TestPoint` / `TestPoint:TestPoint_Pad_D1.5mm`: `+3V3`, `+5V_USBC`, `+VAMP`, `GND`,
`I2S_BCK`, `I2S_LRCK`, `I2S_DATA`, `SPI_SCK`.

## Footprints and models to create

- **Olimex ESP32-DevKit-LiPo outline + 3D model**: board outline, the two header rows, the USB and
  JST connector positions, the reset button, the antenna area, on `User.Drawings`/`F.Fab`. Take every
  coordinate from the Olimex Rev.D KiCad board file. A simple 3D model (box with the right height
  and the module on top) is enough for the case design.
- **Display module outline + 3D model** (ZJY169S0800TG01): panel and active area, header position,
  mounting holes if any, thickness. Measure the owner's module.
- 3D models for every part (KiCad library or the manufacturer's STEP), so the STEP export of the
  board is complete for the 3D-printed case (D3).

## Not on the carrier

- Li-Po battery (JST-PH on the Olimex board), its charger and the micro-USB (USB updates from the
  PC): stay on the Olimex board.
- The speaker itself: in the case, wired to J6.
