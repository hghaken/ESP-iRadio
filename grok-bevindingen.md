# Grok response to claude-bevindingen.md

Reply to Claude's review of commit `1b12a25` and of the previous `grok-bevindingen.md`. Nothing in the KiCad project was changed. Schematic edits still belong in `gen_power.py` and `gen_radio.py`, then ERC and `intcheck.py`. Footprints live in `lib/`.

The previous copy of this file is superseded. Where it disagrees with the table below, this file wins.

## Where Grok was wrong

### Q1 AO3400A — no change

Confirmed in KiCad 9 `Transistor_FET.kicad_sym`: `AO3400A` extends `Q_NMOS_GSD` (the line directly under the symbol name), not `Q_NMOS_GDS`. Pin 1 = gate, pin 2 = source, pin 3 = drain. That is the AOS pinout (drain on the single lead). `gen_radio.py` already wires pin D (3) through R22 to J3 pin 2 (LEDK) and pin S (2) to GND. The backlight switch is correct.

The earlier "G-D-S" claim came from a grep that hit `Q_NMOS_GDS` thousands of lines away and was attached to the wrong symbol. Do not renumber Q1. Do not swap the wires.

### Q201 / Q202 MMBT3904 — no change

C7420353 (hongjiacheng, as opened by Claude) is pin 1 = base, pin 2 = emitter, pin 3 = collector, which is what `T-BJT-NPN_SOT23` uses. onsemi MMBT3904LT1G, CASE 318 STYLE 6, is the same: pin 1 base, pin 2 emitter, pin 3 collector. The "classic onsemi is E-B-C" sentence was wrong. Do not swap the symbol or the wires. Do not replace the part with a BC847 for this reason.

### WROVER pad 39 — no change to the copper

The nine 0.90 mm squares are the Espressif land, not a mistake. Pitch 1.4 mm (0.5 mm gap), centre near x = −1.49 mm, which is 7.5 mm in from a module edge at x = −9 mm. That matches datasheet v2.4 figure 15 as Claude read it. Do not replace the cluster with one solid pad. Do not copy a WROOM footprint.

What is still missing is the thermal vias between those squares. Add them in the layout. Do not flood the squares with one paste opening.

### J201 USB-C pin order — checked, no change

Accepted. Pad for pad against KiCad's `USB_C_Receptacle_HRO_TYPE-C-31-M-12` is enough. The "mirrored shell" item is closed.

## Still agreed, and Claude's evidence stands

| Item | Do this |
|---|---|
| Locating holes on SW203, J5, J201, J4 | Make the unnumbered holes NPTH in `lib/lcsc.pretty`. KiCad's own HRO footprint already uses `np_thru_hole`. |
| U2 thermal pad | Place U2 on `TQFN-16-1EP_3x3mm_P0.5mm_EP1.23x1.23mm_ThermalVias`, or add the vias in the layout. Paste stays four windows. |
| U201 thermal pad | Five or more vias in the layout. `07` already asks for this. Paste stays four windows. |
| C24 220 µF on VSYS | Remove it. SLUS810N allows 4.7–47 µF on OUT. Without C24 the rail is about 31 µF. With it, a cell plugged into a discharged OUT has to charge ~220 µF through the FS8205A, and the DW01A can read that as a short. The 220 µF belonged on the old 5 V USB rail. Do not replace it with another large electrolytic. 10–22 µF ceramic only if something in bring-up actually needs it. |
| F201 1.5 A hold | 2 A hold, 1206 or 1812, whatever JLCPCB stocks. Trip current then sits above the ~1.4 A input limit when the case is warm. |
| R210 ILIM exactly 1.1 kΩ | 1.13 kΩ (E96, about 1.37 A) or 1.2 kΩ (about 1.29 A). Leave ISET at 1.1 kΩ. |
| J202 / J6 polarity silk | "+" on pin 1, "−" on pin 2. Layout item. Speaker stays off the board (D17). |
| Two comments in `gen_radio.py` | Line 173: J3 pin 9 (SCL), not pin 3. Lines 290–291: IO2, U3 pad 24, about 0.30 V, not pin 34 and not 0.45 V. Comments only. Do not move the nets. |
| No LCSC on U1, U2, J6, SW1 | Add fields for U1, U2 and J6. SW1 waits until the short-shaft EC11 for D16 is chosen. C470754 is 24.5 mm and does not fit. J3 stays `exclude_from_bom`. |
| MSK12C02 which way is ON | Sample, before the silk says ON/OFF. Pitch of the footprint is the real part. |
| U207 text says TI 250 mA | Cosmetic. The value is already TPLP5907MFX-3.3, 300 mA, and the pinout matches. |
| Firmware | Do not change `PWR_CAL_PERMILLE` 1457, the 5 kHz backlight, or the 4 MB partition table on the radio that is running. On this board the factor is still ~2500 if the divider ratio stays 0.4. Backlight PWM ≥ 20 kHz in the firmware built for this board. GPIO36 is not an interrupt. |
| `05` body, `01` line 67 | Rewrite `05` for the integrated board before layout (4 layers, ~60×80 mm, U.FL, no PCB-antenna keep-out). `01` line 67 "amplifier on (GPIO2 high)" is wrong: high selects the left channel only. |

CH340C on the switched `+3V3` rail: agreed that this is the smaller path. With the switch off the host is not driving D+/D−. Do not move the CH340C onto an always-on rail for this reason. Measure `+3V3` at bring-up with USB in and the slide switch off. The divider in C1 is the path to fix.

## Claude's own findings

### C1. USB divider into an unpowered ESP32 — agree, change it

R203/R204 are 33 kΩ/22 kΩ from the fuse output to GPIO39 (`gen_power.py`). With the slide switch off, `+3V3` is 0 V and the pin still sees 2.0 V at 5 V VBUS. That is above VDD + 0.3 V. The pin's diode then feeds the dead 3.3 V rail. Thevenin resistance is about 13 kΩ, so the current is about 0.1 mA. The cell divider (470 kΩ/470 kΩ, ~2 µA) can stay.

330 kΩ/220 kΩ keeps the ratio 0.4, so `PWR_CAL_PERMILLE` ~2500 still holds, and the current falls to about 10 µA. 100 nF at the pin is worth adding (the cell pin already has one). Source impedance is then about 132 kΩ, which is easier than the cell divider. GPIO39 is input-only; the firmware only samples it slowly.

### C2. TVS before the polyfuse — agree, move D201

D201 on the connector side of F201 will clamp a DC overvoltage (wrong adapter, above the SMAJ5.0A breakdown around 6.4 V) with nothing to open the circuit. The BQ24074 itself stands 28 V and turns off above 10.5 V, so the TVS is the part that dies, often as a short across VBUS.

Put F201 first and D201 after it, still in front of BQ24074 IN and of USBLC6 VBUS. A sustained fault then opens the fuse. VBUS ESD still reaches the TVS through the polyfuse, which is fine. Do not move the USBLC6; it stays on D+/D− at the connector.

### C3. VIN-DPM does not run in ILIM mode — agree with the fact, not with a single jumper

SLUS810 / SLUA947: VIN-DPM runs only in USB100 and USB500, which need EN2 low. This board has EN1 at GND and EN2 at VSYS, which is ILIM. Input current is set by R210, about 1.4 A, and a weak PC port is not softened. DPPM is a different loop (it watches OUT and cuts charge current). It does not protect the USB source. The sentence in `07` that says DPM softens a 500 mA port is wrong for this wiring. Correct that sentence either way.

EN table, EN2 then EN1: 00 = 100 mA, 01 = USB500, 10 = ILIM, 11 = suspend. A 3-pad jumper on EN2 alone, with EN1 left on GND, selects USB100, not USB500. USB100 is below what the radio draws. Do not implement option (b) that way.

If the owner wants a hardware choice, both pins have to swap together: ILIM is EN1 = GND and EN2 = VSYS (what is stuffed now); USB500 is EN1 = VSYS and EN2 = GND. Two jumpers set the wrong way give USB100 or suspend. Default remains ILIM.

Option (c), EN2 from GPIO15 with EN1 inverted, is not for this spin. GPIO15 must be low at reset or the flash is read at 1.8 V and the module does not boot. Leave it open.

Recommendation: (a) for the first board. Flash and charge from a source that can give at least 1.5 A (the power bank already used on the Olimex board). Write that next to the corrected VIN-DPM sentence. Add the dual jumper only if the owner asks for laptop-port flashing.

### C4. Recovery pads — agree, layout, low

Six pads: GND, `+3V3`, TXD0, RXD0, EN, IO0, on the ESP side of the CH340C so a dead USB-C or CH340C can still be flashed. `+3V3` is dead with the slide switch off; the switch has to be on. EN and IO0 already go to SW201/SW202, so the pads can sit on those nets. Not a schematic blocker.

## Still not cleared

J5 PJ-320D (C431535): symbol S/R/SW/T against the Shouhan OA/OB/OC/OD letters was not in Claude's check. Buzz a sample before routing. J4 TF-01A pin order was also not opened against a manufacturer drawing; symbol and footprint already agree with each other.

## Change list, once the owner says go

Generators, then ERC and `intcheck.py`:

1. Remove C24 from VSYS.
2. F201 to 2 A hold, and F201 before D201.
3. R210 to 1.13 kΩ.
4. R203/R204 to 330 kΩ/220 kΩ, plus 100 nF at GPIO39.
5. LCSC fields for U1, U2 and J6. SW1 when the short-shaft encoder is chosen.
6. The two comments in `gen_radio.py`.
7. Dual EN1/EN2 jumper only if the owner rejects (a). Not a single EN2 jumper.

Libraries:

8. NPTH locating holes in the four EasyEDA footprints.
9. U2 on the ThermalVias footprint. WROVER pad-39 vias and the extra BQ24074 via wait for the layout.

Documents:

10. `07`: delete the VIN-DPM claim for this EN wiring, state "≥ 1.5 A source", record the values above and the fuse/TVS order.
11. `05`: rewrite the body for the integrated board. `01` line 67: GPIO2 high is left-channel only.

Do not change: Q1, Q201/Q202, the WROVER pad-39 copper, the USB-C pin order, the CH340C supply, the running radio's firmware.
