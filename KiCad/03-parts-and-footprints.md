# 03 - Parts and footprints

KiCad 8/9 standard library names where a fitting footprint exists. Everything marked **create**
needs a project-local footprint, made from the part's datasheet or measured from the module the
owner has. Do not guess module dimensions: measure or look them up, and record the source in
`KiCad/esp-iradio/NOTES.md`.

## Bill of materials (phase 1, module variant)

| Ref | Qty | Part | KiCad symbol | KiCad footprint | Notes |
|---|---|---|---|---|---|
| J1, J2 | 2 | Female header 1x19, 2.54 mm | `Connector_Generic:Conn_01x19` | `Connector_PinSocket_2.54mm:PinSocket_1x19_P2.54mm_Vertical` | socket for the Olimex board; row spacing from the Olimex Rev.D files |
| J3 | 1 | Female header 1x8, 2.54 mm (display) | `Connector_Generic:Conn_01x08` | `Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical` | or a JST/FFC connector if the display goes on a cable |
| SW1 | 1 | Rotary encoder EC11 with switch, vertical | `Device:RotaryEncoder_Switch` | `Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` | shaft length to suit the enclosure; check the footprint against the bought part |
| J4 | 1 | microSD socket, push-push | `Connector:Micro_SD_Card` (or `_Det`) | per socket, e.g. `Connector_Card:microSD_HC_Hirose_DM3AT-SF-PEJM5` or `Connector_Card:microSD_HC_Molex_104031-0811` | pick the socket, then its footprint |
| U1 | 1 | PCM5102A module (GY-PCM5102) | create (or `Connector_Generic:Conn_01x06` + `Conn_01x03`) | **create** from the module: header positions + outline | verify pin order on the real module |
| U2 | 1 | PAM8403 module | create | **create** from the module | many variants: measure the owner's module |
| J5 | 1 | 3.5 mm stereo jack, PCB | `Connector_Audio:AudioJack3` | e.g. `Connector_Audio:Jack_3.5mm_PJ320D_Horizontal` or `Jack_3.5mm_CUI_SJ1-3523N_Horizontal` | pick the part, then its footprint |
| J6, J7 | 2 | Speaker terminal 2-pin | `Connector_Generic:Conn_01x02` | `TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal` or `Connector_JST:JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical` | one per channel; J7 may be left off for mono |
| J8 | 1 | Spare header 1x5, 2.54 mm | `Connector_Generic:Conn_01x05` | `Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical` | |
| R1-R4 | 4 | 10k, 0805 | `Device:R` | `Resistor_SMD:R_0805_2012Metric` | pull-ups ENC_A, ENC_B, ENC_SW, LCD_CS |
| R5-R8 | 4 | 10k, 0805 | `Device:R` | as above | pull-ups SD_CS, SPI_MISO, SD DAT1, SD DAT2 |
| R9 | 1 | 100k, 0805 | `Device:R` | as above | pull-down AMP_SD (GPIO2 strapping: never a pull-up) |
| R10, R11 | 2 | 10k, 0805, DNP | `Device:R` | as above | optional pull-ups SPARE_IN34/36 |
| C1-C3 | 3 | 100 nF, 0805 | `Device:C` | `Capacitor_SMD:C_0805_2012Metric` | debounce ENC_A, ENC_B, ENC_SW |
| C4 | 1 | 10 uF, 0805 or 1206 | `Device:C` | `Capacitor_SMD:C_0805_2012Metric` | microSD supply |
| C5 | 1 | 100 nF, 0805 | `Device:C` | as above | microSD supply |
| C6 | 1 | 10 uF | `Device:C` | as above | display supply, at J3 |
| C7 | 1 | 470 uF / 10 V electrolytic | `Device:C_Polarized` | e.g. `Capacitor_THT:CP_Radial_D8.0mm_P3.50mm` | amplifier supply bulk (see `04-power.md`) |
| C8 | 1 | 10 uF, 0805 or 1206 | `Device:C` | `Capacitor_SMD:C_1206_3216Metric` | amplifier supply, close to U2 |
| C9 | 1 | 1 uF + 100 nF | `Device:C` | 0805 | amplifier supply, close to U2 |
| TP1-TP8 | 8 | Test points | `Connector:TestPoint` | `TestPoint:TestPoint_Pad_D1.5mm` | 3V3, 5V, VAMP, GND, I2S_BCK, I2S_LRCK, I2S_DATA, SPI_SCK |
| H1-H4 | 4 | Mounting holes M3 | `Mechanical:MountingHole` | `MountingHole:MountingHole_3.2mm_M3` | or M2.5 to suit the enclosure |
| (power) | - | Amplifier supply switch / jumper / fuse | - | - | see `04-power.md` |

## Chip variant (later revision, assembled by the fab)

| Ref | Part | Footprint | Notes |
|---|---|---|---|
| U1 | PCM5102APWR | `Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm` | reference circuit from the TI PCM510xA datasheet: charge-pump cap, LDOO cap, decoupling, output RC (take the values from the datasheet) |
| U2 | PAM8403 (SOP-16) | `Package_SO:SOIC-16_3.9x9.9mm_P1.27mm` | Diodes Inc. datasheet application circuit: 1 uF input caps, 10 uF + 1 uF supply caps, optional ferrite bead + 220 pF per output for EMI |

## Footprints to create

- **Olimex ESP32-DevKit-LiPo outline** on a documentation/fab layer (`User.Drawings` or
  `F.Fab`): board outline, the two header rows, the USB and JST connector positions, the reset
  button, the antenna area. Take every coordinate from the Olimex Rev.D KiCad board file
  (`ESP32-DevKit-LiPo_Rev_D.kicad_pcb` or similar in their HARDWARE folder).
- **Display module outline** (1.69" module): panel area, header position, mounting holes if any.
  Measure the owner's module (ZJY169S0800TG01).
- **PCM5102A and PAM8403 modules**: header positions + outline, measured.

## Not on the carrier

- Li-Po battery (JST-PH on the Olimex board), the charger, USB: stay on the Olimex board.
- Bluetooth headphones/speakers: wireless.
