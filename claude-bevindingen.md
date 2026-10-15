# Claude review of KiCad Rev B - outcome

Review of commit `1b12a25` with Grok (`grok-bevindingen.md`), two rounds, 2026-10-15. The owner
said go with option a for the USB input. **All agreed changes are done** (this file replaces the
earlier versions; the history is in the two rounds summarised below).

## Round 3: response to Grok's check of the outcome

Grok's "accepted as done" list matches what was changed; nothing to add there. GPIO12/GPIO15:
closed, option a stands.

### USBLC6 VBUS pin before the fuse - agree, move it

Confirmed in `tools/gen_power.py`: U202 pin 5 gets the local label `VBUS` (line 140-141), which is
the connector side of F201, and `intcheck.py` expects `U202.5` together with `F201.1`. The
USBLC6-2SC6 has its own clamp from VBUS (pin 5) to GND; a sustained overvoltage at the connector
would flow through that clamp with no fuse in front of it. Moving pin 5 to the fused net (with
D201 pin 1, U201 IN, R203, C201) puts every clamp behind F201. The data pins stay as they are
(1/3 connector side, 6/4 CH340C side); the 2 A polyfuse drops only tens of mV.

One limit to write down (not a reason to do it differently): the USBLC6 clamp starts lower than
the SMAJ5.0A (ST: VBR 6 V min at 1 mA; SMAJ5.0A: VBR 6.40-7.07 V at 10 mA). With a wrong adapter the
small USBLC6 may still take the first current and fail before the polyfuse trips (trip time is in
seconds); after the move that current at least runs through the fuse, so the board is no longer
fed through a shorted part. A USB-C source without PD never exceeds 5.25 V, so this only matters
for a broken or wrong adapter; it does not need more parts.

**Done** (owner's go):
1. `gen_power.py`: the fused net is named `VIN` (label on the rail behind F201); U202 pin 5 is on
   `VIN`. The `VBUS` net now holds only J201 and F201 pin 1.
2. `intcheck.py`: `U202.5` is in the VIN group. While doing that, a weakness in the checker itself
   showed up: its "must stay apart" test compared `VBAT` and `BATN` by their short names, but
   local nets carry the sheet path (`/Power, charger and USB/VBAT`), so those two checks were
   skipped without a message. They now use the full names, VBUS/VIN was added, and a missing net
   counts as an error.
3. `07` (USB-C bullet, block diagram), NOTES.md and `tools/README.md` updated.
Checks: field check 0 issues, ERC 1 known warning (MAX98357A exposed pad), `intcheck.py` OK,
the corner of sheet 2 checked in the PDF render.

### Still open (agreed)

Unchanged: J5 pin order and SW203 ON position on samples, J4 drawing, short-shaft EC11, FPC pin 1
and R22, thermal vias (BQ24074, WROVER land) and "+"/"-" silk in the layout; firmware for the new
board only.

## Status of every point

| # | Point | Outcome |
|---|---|---|
| - | Q1 AO3400A pinout | No change: KiCad `AO3400A` = `Q_NMOS_GSD` (G 1, S 2, D 3), as the AOS datasheet (C20917). |
| - | Q201/Q202 MMBT3904 pinout | No change: C7420353 drawing B 1, E 2, C 3, as `T-BJT-NPN_SOT23`. |
| - | WROVER-IE pad 39 | No change: Espressif's 3 x 3 land (datasheet v2.4, fig. 15). Thermal vias in the layout. |
| - | J201 USB-C pin order | No change: same as KiCad's HRO TYPE-C-31-M-12 footprint. |
| - | CH340C on the switched +3V3 | No change; measure +3V3 at bring-up (06, step 1). |
| 1 | C24 220 uF on VSYS (BQ24074 OUT allows 4.7-47 uF) | **Done**: removed; VSYS ~31 uF. |
| 2 | Polyfuse 1.5 A hold, after the TVS | **Done**: F201 1206L200/16NR (C22374899, 2 A hold, 16 V), now VBUS -> F201 -> D201 -> IN. |
| 3 | ILIM exactly 1.1k | **Done**: R210 1.13k (1.37 A). ISET stays 1.1k. |
| 4 | USB divider 33k/22k feeds an unpowered ESP32 | **Done**: R203/R204 330k/220k + C214 100 nF (ratio 0.4, `PWR_CAL_PERMILLE` ~2500 on the new board). |
| 5 | No LCSC on U1, U2, J6, SW1 | **Done** for U1 (C107671), U2 (C910544), J6 (C131337). SW1 waits for the short-shaft EC11. |
| 6 | Two wrong comments in `gen_radio.py` | **Done** (J3 pin 9 SCL; IO2 / U3 pad 24, ~0.30 V). |
| 7 | VIN-DPM does not work in ILIM mode | **Option a** (owner): power and flash from a source of at least 1.5 A; no jumper. Written in 07, 06 (bring-up step 0), NOTES.md and AGENTS.md. |
| 8 | Plated locating holes | **Done**: NPTH in the PJ-320D, MSK12C02, TF-01A and TYPE-C-31-M-12 footprints. |
| 9 | U2 exposed pad without vias | **Done**: `TQFN-16-1EP_3x3mm_P0.5mm_EP1.23x1.23mm_ThermalVias`. BQ24074 (>= 5) and WROVER land vias: layout. |
| 10 | `07` | **Done**: VIN-DPM, >= 1.5 A source, fuse before TVS, new values, recovery pads, parts table. |
| 11 | `05` body, `01` use cases | **Done**: `05` rewritten for the integrated board; `01` use cases corrected (GPIO2 released = on, never high; speaker on battery). |
| 12 | Recovery pads | **Done** in the schematic: TP203 EN, TP204 IO0, TP205 TXD0, TP206 RXD0, TP207 GND (+3V3 = TP4). |
| - | J202/J6 "+"/"-" silk | Layout item, in `05` and the 06 checklist. |
| - | GPIO15 as a charger-mode output | Not used. (Correction to Grok's first answer: the flash-voltage strap is GPIO12/MTDI; GPIO15/MTDO only switches the boot log and SDIO timing.) |

## Checks after the changes

- `tools/gen_power.py` and `tools/gen_radio.py` regenerated both sheets; field check 0 issues.
- kicad-cli 9.0.9 ERC: 1 warning (the known MAX98357A exposed pad), 0 errors.
- Netlist: `tools/intcheck.py` OK (107 nets, 39 WROVER pins; VBUS/VIN split at the fuse, C214 on
  VBUS_SENSE, recovery pads on EN/IO0/TXD0/RXD0/GND, C24 gone); values, footprints and LCSC fields
  of the changed parts read back from the netlist; 119 parts.
- The changed corner of sheet 2 checked in the PDF render.

## Still open (on samples or for the layout)

- J5 PJ-320D pin order (Shouhan drawing only says OA-OD): buzz a sample.
- SW203 MSK12C02: which slide position is ON.
- J4 TF-01A: not yet compared with a manufacturer drawing.
- SW1: short-shaft EC11 for D16 (C470754 is 24.5 mm).
- Display panel: FPC pin 1 / contact side (paper mock-up), R22 from its Vf.
- Firmware for the new board only (not on the running radio): `PWR_CAL_PERMILLE` ~2500, charge
  state on GPIO34/36 (polled), backlight PWM >= 20 kHz, 16 MB partition table.
