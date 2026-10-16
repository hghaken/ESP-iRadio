# Grok review, round 8 (parts spread, 122 x 76 mm, 2026-10-16)

Measured commit `8ca50ea` on the placed board with KiCad's pcbnew (pad positions, courtyards, tracks, zones). The board file was not changed. DRC was not re-run; the recorded result stands (0 errors, 0 unconnected, 0 parity, 31 warnings).

One correction to the round 7 text below, confirmed on this board. A footprint at +90 deg turns local (x, y) into (y, -x). J5's locating hole at local x -5.7 sits at (42.5, 72.2), and pad 1 at local (4.20, 3.50) sits at (46.0, 62.3). The plug bore (local x -9.4) is the silk end at y 75.9, on the edge. Round 7 had that turn mirrored and asked for a 180 deg flip. The jack was already mouth-out. C3 and C4 now sit at x 50.5, 3.8 mm clear of the jack silk.

Nothing in this spread has to move before an order.

## 1. Spacing — the 1 mm claim holds for soldered parts

Edge cuts are 122 x 76 mm. Courtyard to courtyard, the only soldered pair under 1.0 mm is U3 against J7 at 0.85 mm. Both are fixed by the module. The next soldered pairs are U3/C211 1.06 mm, F201/R201 1.12 mm, R21/R214 1.15 mm, D202/D203 1.15 mm and C210/C211 1.16 mm. The 4.0 mm column pitch and the 6.3 mm row pitch do not produce a closer 1206 pair. F201/R201 at 1.12 mm is the round 7 nudge, and a heat gun on the fuse no longer sits on R201.

Four more gaps under 1.0 mm are mounting-hole courtyards, which are circles larger than the hardware:

- H3/J8 0.53 mm. An M3 head about 5.5 mm across, centred at (3.5, 72.5), reaches x 6.3. The jack courtyard starts at x 7.5, so the head clears it by about 1.3 mm.
- H4 against D202 and D203, 0.68 mm. The same head clears the LED courtyards (they end at x 114.3, the head starts near x 115.8).
- H6/C10 0.76 mm and H5/C5 0.86 mm. A 4 mm round standoff on those holes clears both capacitors; the tight number is the hole courtyard, not the spacer.

Decoupling, pad to the pin it serves:

| Cap | At | Distance |
|---|---|---|
| C211 100 nF | U3 pin 2 (3V3) | 2.4 mm |
| C210 22 uF | U3 pin 2 | 8.9 mm |
| C9, C11 100 nF | U1 pins 20 and 1 | 4.1 mm |
| C10, C12 10 uF | the nearer of those pins | 7.9 mm |
| C13 100 nF, C14 10 uF | U1 pin 8 (+3V3A) | 7.2 mm, 9.8 mm |
| C23 100 nF, C22 10 uF | U2 pin 7 (VDD) | 3.8 mm, 8.3 mm |
| C201 4.7 uF | U201 IN | 7.1 mm |
| C202 10 uF | U201 OUT | 5.6 mm |
| C203 10 uF | U201 BAT | 7.9 mm |
| C204 100 nF | U203 V3 | 4.2 mm |
| C15 charge pump | U1 CAPP/CAPM | 3.7 and 5.2 mm |
| C17 LDO, C16 VNEG | U1 pins 18 and 5 | 4.8 mm, 7.9 mm |

The 100 nF parts are on the pin. The bulk parts sit one pitch further, which is what the 6.3 mm row is for. None of these wants to come back in.

## 2. The new route — widths held, analog stays on the left

Power nets are on their class for the whole run. VSYS is 216 mm at 0.5 mm (about 1 mm of 0.25 mm where it leaves a pad) and seven 0.7/0.35 vias. +3V3 is 313 mm at 0.5 mm. VBAT, BATN, the cell positive, VBUS and VIN are 0.5 mm throughout (44, 63, 66, 16 and 88 mm). +3V3A is 40 mm at 0.3 mm. The speaker outputs are 15 and 16 mm at 0.3 mm. GND zones are on F.Cu, B.Cu and the In1 plane; the four empty zones are the edge keepout on all four copper layers.

Analog (OUTL, OUTR and the two filtered nets) stays at x under 39. I2S crosses the board to the amplifier (BCK, LRCK and DIN, each about 120 mm, on F.Cu, B.Cu and In2.Cu). The closest analog-to-I2S copper is 5.7 mm, which is OUTL and LRCK on opposite sides of U1. The routed wires do not cross in x/y. OUTP and OUTN stay at x 93-100, 56 mm from the analog. The 470 ohm series resistors are unchanged.

USB full speed, two hops. Connector to the USBLC6: D+ 10.4 mm (6.8 on F.Cu, 3.5 on In2.Cu), D- 15.3 mm on F.Cu. USBLC6 to the CH340C: 18.3 and 18.8 mm, both on F.Cu. The 5 mm skew is on the connector hop. At 12 Mbit that is irrelevant, and the pair does not need to be routed as a differential pair.

## 3. Battery divider — the charge path stays short

VBAT (U201 BAT, Q203, C203, R217, TP202) is 44 mm of 0.5 mm track, all of it in x 104-113, y 16-47. The charge current runs between the FET at y 30 and the charger at y 16. The divider at (112, 41) and TP202 at (113, 47.3) are a spur off that net, still at 0.5 mm, carrying microamps. The cell positive and BATN go down to J202 at 0.5 mm and do not detour out to the divider.

VBAT_SENSE is 86 mm of 0.2 mm track from C207/R218 at x 120 to U3 pin 7 at (49.2, 26.6): 64 mm on F.Cu, 22 mm on In2.Cu. C207 is at the divider, so this whole run is the high-impedance side of the 470k/470k. On the same layer the nearest neighbours are VBUS_SENSE for 22 mm (0.38 mm away; the other slow ADC line) and LCD_BL for 18 mm (0.35 mm away). The backlight is the one that toggles. With the 100 nF at the far end, an edge on that 18 mm can tick an unfiltered sample. The firmware averages the pin, and the divider's own pole is about 7 Hz, so the percentage will not wander by a tenth. If that corner is touched anyway, move C207 next to U3 pin 7 so the capacitor sits on the ADC pin. That is one 1206 and its track. It does not need a new Freerouting run, and it does not hold the order.

## 4. Bottom edge — same offsets, switch clear of the USB plug

The row moved down 4 mm with the new edge. Relative to y = 76 the connectors are the ones in `placement.py`: J8 at H-8.2, J5 at H-9.5, J4 at H-9.6, then J201, J202, J6, SW203.

- J8 silk runs to y 83.4, so the barrels still stick out 7.4 mm. J5 silk runs y 61.8-75.9; the bore is the end at 75.9. A flat rear wall still has the 3.5 mm hole on the board edge and the RCA holes 7 mm further out.
- Courtyard gaps along the row: J8 to J5 silk 3.9 mm, J5 to J4 8.1 mm, J4 to J201 3.2 mm, J201 to J202 1.6 mm, J202 to J6 2.0 mm, J6 to SW203 1.7 mm. J202 and J6 are wires. J4 to J201 at 3.2 mm is a thin rib at best; a USB plug and a card can both be used.
- SW203 courtyard is x 101.7-108.3. J201 ends at x 82.4, so the switch is 19 mm clear of the USB connector. A cable leaving the jack no longer covers the switch.
- C3 and C4 are left of the card socket (x 49.3-51.7), clear of J5 and of J4.

## Before the order

No placement change and no new route. The optional C207 move is the only improvement this round suggests. Still on a sample, unchanged: PJ-320D pin order, TF-01A drawing, which side of SW203 is ON, EC11 shaft length, and the HS20S010B pin-1 / pin-7 / pin-8 check.

---

# Grok review, round 7 (Rev C, hand assembly, 2026-10-16)

Measured the placed board (`esp-iradio.kicad_pcb`, 122 x 72 mm) and the Chuangdu drawing for the CAX AV-210-WR. The board file, both schematic sheets and the tools were not changed. No new route, and none is proposed: `mkpcb.py` starts from an empty board and Freerouting is not deterministic. The recorded DRC (0 errors, 0 unconnected, 0 parity, 36 silkscreen warnings plus the two expected QFN via mismatches) was not re-run.

Placement frame below: origin top left, y down, display at the top, connector edge at y = 72. A footprint at +90° in this file turns (lx, ly) into (−ly, lx). Checked on J8 (rotation 0, barrels leave through the bottom edge) and on J5 pad 2 (silver in `renders/top.png` at the predicted spot).

## 1. RCA J8 pinout — no change

The assumption holds. Zhejiang Chuangdu, "RCA PIN JACK", 11/13/24 rev B (the PDF linked from the EasyEDA footprint of C53431212):

- The schematic on that sheet ties the shell bracket to pins 1 and 3. Pins 2 and 4 are the centre contacts.
- PCB layout, top view: pins 1 and 3 are the large holes (2-φ2.1), pins 2 and 4 the small ones (2-φ1.3). Barrel centres 14 mm apart, the small holes 4 mm behind the large ones, the two rows 4.8 mm apart (5.6 vs 10.4).
- Front view: left barrel A, right barrel B, 14 mm centres. The colour table on the same sheet is A = white, B = red.

The footprint matches those drills and those offsets. The schematic and `intcheck.py` put J8.1 and J8.3 on GND, J8.2 with J5 tip (after R16/C18) and J8.4 with J5 ring (after R17/C19). Silk "L" sits on the white barrel and "R" on the red one. With the display up and the connectors toward you, white/L is on the left.

A meter on the delivered part is still sensible. The drawing is no longer missing: `NOTES.md` can drop "no datasheet drawing was found". The comment in `tools/silk.py` ("white = L at pad 2") names the wrong pad; the letters on the board are on the right barrels. Comment only.

PJ-320D pin order (S = 1, R = 2, T = 4) stays a check on a sample. If that sample is swapped, the 3.5 mm jack and both RCA centres swap together, because they share the nets.

## 2. Floor plan — one fix before ordering, two nudges worth doing

Paste and a heat gun can build this board. The QFNs, the WROVER land and the encoder are acceptable as placed.

**J5 faces the wrong way.** This is the item to fix before the order. The PJ-320D footprint has no courtyard, so DRC cannot see it. The 3D model (`AUDIO-SMD_PJ-320D-1.wrl`, scale 1, no offset) spans the silk exactly, local x −9.40 to +4.70, and the plug bore is the ring at the −9.40 end (a 3.5 mm hole). At rotation 90 that end lands at placement y = 53.1, toward the encoder. The other end is at y = 67.2, 4.8 mm inside the edge. `renders/top.png` shows the hole on the encoder side; only the RCA barrels cross y = 72. `NOTES.md` says the mouth is at the edge. The placed part has it toward the encoder.

C3 and C4 (microSD decoupling, centres (40.5, 53.5) and (43.5, 53.5)) sit under that body. Their courtyards run y 51.2–55.8 and overlap the jack silk (y 53.1–67.2) by about 2.7 mm. Same cause.

Fix in place: set J5's angle from 90 to 270 and shove the four pad tracks. The mouth then lands at y ≈ 71.9, on the edge, and the body comes off C3/C4 (the back of the jack lands near y 57.8). Do not regenerate the board. After that turn the 3.5 mm hole is flush with the edge while the RCA barrels stick out about 7.4 mm, so a flat rear wall has the two openings at two depths. The housings themselves clear each other (about 3.8 mm between J8 and J5).

**The power switch sits under a USB plug.** Along the bottom edge, left to right: J8, J5, J4, J201, SW203, J202, J6. Courtyard gaps: J4 to J201 is 1.6 mm, J201 to SW203 is 1.8 mm. A USB cable covers the switch, and there is no room for a bezel wall between the card slot and USB-C. From J6 to the H4 screw head is about 9 mm. Slide SW203 to the right of J6 (its courtyard is 6.6 mm wide; it fits before the screw) and shift J202 and J6 left into the gap the switch leaves. Cell and speaker are wires, so they do not need a bezel opening. Local nudge and a re-route of those nets, not a new Freerouting run.

**A few 1206s are close enough to bridge.** Clearance 0.15 mm is met. The tight pairs are the auto-reset column placed on a 5.0 mm pitch along the long axis of the 1206 (R205/R206 at x 96, R207/C205 at x 99.5, y 41 and 46): 0.29 mm and 0.33 mm of copper between the ends. The other 1206 columns use 5.3 mm and have 0.55 mm. Open those two pairs to 5.3 mm. F201 and R201 have 0.40 mm of copper and 0.3 mm between courtyards; a heat gun on the fuse reflows R201. Nudge one of them about 0.5 mm. C4 is 0.34 mm from a J5 mounting hole; the rotation above removes that.

The rest of the hand-soldering picture is fine:

- U201 (BQ24074) at (104, 18) is beside the module. Solder it before the display is plugged in. Thermal vias are 0.3 mm and the board tents both sides. Do not update the QFN footprints from the library, or the 0.2 mm holes come back.
- U2 (MAX98357A) at (104, 58), turned 180°, shares a nozzle pass with C22/C23. J6 is through-hole, soldered after reflow.
- U1 (TSSOP-20, 0.65 mm pitch) with 1206s on a 3.0 mm side pitch is the fiddly paste part. The pad gaps there are the 0.55 mm ones.
- U3 lies fully under the module (module PCB x 29.4–92.6, y 1.0–37.2). The U.FL end is still under the module; the coax leaves below it, past y 36.9. Mate the coax before the module goes on. Two of the four ground-land vias are fitted (microSD DAT0 takes the other two on In2.Cu). The land is paste and hot air only. C211 is 0.64 mm from the module pads: solder the module and the 1206s under it in one pass. Parts under the module: R9, R8, TP5, J3, C5, C25, U3, R3, C211, J7, C210.
- SW1 courtyard ends at y 37.9; the module PCB ends at y 37.2. The shaft at y 45 is below the module, and the EC11 body fits in the 11 mm under it.
- H3 clears the RCA courtyard. D203 at the edge is the agreed power LED.

## 3. Audio — no routing change

The two connectors are in parallel after the filter, not on the raw DAC pins. R16/R17 are 470 Ω, C18/C19 are 2.2 nF to ground, so the corner is about 154 kHz. J5's switch pad is unconnected.

The analog copper stays on the left (x below about 39). OUTL and the filtered left net are on F.Cu; OUTR and the filtered right net use F.Cu and In2.Cu. The closest analog-to-I2S copper is 14.3 mm (the filtered left net against BCK, near (29.7, 59.7)). The analog tracks do not cross an I2S track in x/y, and they do not cross the speaker outputs. OUTP and OUTN stay on the right (x about 101–105, about 12 mm long). A shorted plug in one jack loads the other through the shared 470 Ω node. That is acceptable for a line output.

## 4. The 1206 move did not break the filter cap

C9191 (C18, C19) is a 1206 2.2 nF ±10 % X7R, 1 kV, on the standard `C_1206_3216Metric` courtyard. LCSC lists it as that body. Line level is about 2 V, so the 1 kV rating is spare and the X7R bias is negligible. The filter row (R16, C18, R17, C19 at y = 57, pitch 5.3 mm) sits about 3 mm above the RCA body; those four are through-hole soldered after reflow, the caps with the rest of the paste.

What the larger body did tighten is the 5.0 mm columns in section 2, not this capacitor. The NTC staying 0805 (C2889056; LCSC has no 1206 NTC) is already recorded and is fine.

## Before the order

1. Rotate J5 by 180° and shove its pads. That also clears C3/C4.
2. Worth doing in the same local edit: move SW203 to the right of J6, and open the two 5.0 mm 1206 pairs (and F201/R201) to the 5.3 mm pitch.
3. Leave the RCA wiring, the analog routing, the QFN vias and the WROVER where they are.

Still on a sample, unchanged by this round: PJ-320D pin order, TF-01A drawing, which side of SW203 is ON, EC11 shaft length, and the HS20S010B pin-1 / pin-7 / pin-8 check. Firmware for this board waits until a board exists.

---

# Grok review, reply to the round 6 outcome

Checked commit `8ba0d64` (docs and comments only: `07`, `05`, `06`, `NOTES.md`, `gen_radio.py`, `mkpcb.py`, `pcbrules.py`). The board file and both schematic sheets are not in that commit. No new route.

The twelve points match the files:

- CN1 order, hole drill 2.7 mm, and round M2.5 standoffs about 4 mm across are in `07`, `05`, `NOTES.md`, `mkpcb.py` and `pcbrules.py`.
- `gen_radio.py` now says J3 pin 3 CLK.
- CN2's own order is in `07`, with the warning never to tie the sockets pin for pin. J7 stays open. The old "duplicates CN1" sentence is gone from the live docs (it remains only in the archived round 6 text, with the correction above it).
- The first-module check (GPIO5 selects the glass, GPIO26 is the backlight; swap pins 7 and 8 only if the module disagrees) is in `07`, `NOTES.md` and the `06` checklist.
- Backlight current from `+3V3`, glass front about 14 mm, and PWM at 20 kHz or above are in `07`.
- The encoder row no longer offers C470754 as the shaft for this height. The footprint stays the Alps 20 mm shaft until the case thickness is known.

Two sentences to align, still not a layout change:

1. The coax exit. `07` and `NOTES.md` say the cable "leaves through the ~11 mm gap". The module outline starts at x 0.5, so the cable cannot leave past the left edge. It runs in that gap and comes out below the module, past y 36.9. Add that, so the case slot is cut under the bottom edge of the module.
2. `NOTES.md` still lists two narrow necks in the B.Cu pour under "still to do". The round 6 DRC said those necks are gone and the remaining warnings are silkscreen. Drop that bullet unless a fresh DRC shows them again. The `06` checkbox "review of the 90 x 60 board (round 6)" can be ticked: this reply closes it.
