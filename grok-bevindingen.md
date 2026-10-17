# Grok review, round 10 (edge spread, 2026-10-17)

Measured commit `6b0a565` on the placed board with KiCad's pcbnew, and re-ran DRC with schematic parity. The board file was not changed. Outline 122.000 x 76.000 mm, origin (87.5, 67.0), 122 footprints. In1.Cu has no signal tracks. The schematic is unchanged.

DRC: 0 errors, 0 unconnected, 0 schematic-parity issues, 20 warnings. Seventeen are silkscreen clipped by the board edge, on the connectors that sit on the outline: SW203 x2, J8 x4, J6 x2, J4 x4, J201 x2, J5 x3. Two are the expected QFN thermal-via library mismatches (U201 VQFN, U2 TQFN; the drills stay 0.3 mm). One is J4's own outline on copper. The round 9 warnings for CELL and SPK on a JST outline are gone.

One thing to fix before an order. It is copper under the USB-C shell. No part has to move.

## 1. USB-C — vias under the shell

J201 is at (81.75, 72.85), rotation 0. The opening faces the bottom edge. The front render from the same minute as this commit shows that opening, and the two wire holes of the green screw terminal, toward the viewer.

There is no keep-out on the shell. Freerouting and `stitch.py` both treat that rectangle as empty board: the stitch only skips other copper, holes, and a 1 mm margin from the edge, on a 2.5 mm grid. Ten vias sit inside the F.Fab rectangle. The board tents vias on both sides.

In the silk gap the footprint leaves for the mid-mount shell (local y 0.1 to 2.0), three GND stitch vias:

| Via | Board | Local |
|---|---|---|
| GND | (78.95, 73.95) | (−2.80, +1.10) |
| GND | (81.45, 73.95) | (−0.30, +1.10) |
| GND | (83.95, 73.95) | (+2.20, +1.10) |

Each is 0.55 / 0.30 mm. That row is where the shell has to sit on the board. A via there can hold the shell off the tabs.

Further back, still inside the body, three signal vias and four more GND vias:

| Net | Board | Local |
|---|---|---|
| USB_D− | (79.92, 70.08) | (−1.83, −2.77) |
| CC1 | (80.93, 70.34) | (−0.82, −2.51) |
| CC2 | (83.47, 69.99) | (+1.72, −2.86) |
| GND | (78.95, 71.45), (81.45, 71.45), (83.95, 71.45) | y −1.40 |
| GND | (78.43, 69.39), width 0.50 | (−3.32, −3.46), on the rear fab edge |

D+ does not do this. It leaves the pads at y 68.81 and runs to U202 pin 1 entirely on F.Cu: 12.4 mm, 0.20 mm, no via. D− is 13.7 mm, also 0.20 mm, and about 7 mm of that is on B.Cu after the via under the body. A worn tent on D− or either CC pin shorts that pin to the shell. A GND via shorted to the shell is only a mechanical bump.

Fix, local, no new placement: a keep-out on the shell outline, so the next stitch and the next route stay out; drop the vias inside it; bring D− and both CC back behind the body the way D+ already leaves. The SMT pads are at y 68.81, 6.47 mm inside the edge, so the escape has room.

## 2. The overhang and the 1.05 mm note

The front of the fab outline is at y 76.50, so 0.50 mm past the edge. That is the owner's flush-with-the-wall choice. The HRO page did not return a drawing (HTTP 403), so nothing measured here contradicts the 0.5 mm. A case wall thicker than that lip needs a notch around the face. That is a case cut, not a board move.

The front shell tabs are the S1 pads at (77.43, 73.90) and (86.07, 73.90), 1.00 x 1.60 mm, drill 0.60 x 1.20. Their copper ends at y 74.70, which is 1.30 mm inside the edge. The "1.05 mm inside" in the notes is the pad centre's local Y, measured from the footprint origin, not from the board edge. The copper is further inside than that sentence says, and well clear of the 0.3 mm copper-to-edge rule. Do not push the connector out to "fix" the 1.05.

The rear shell pegs are at y 70.25. U202 is at (79.25, 62.00): pins 1 and 3 are the connector-side USB nets, pins 6 and 4 are UD+ and UD− toward the CH340, pin 5 is VIN. Two net names on one USBLC6 line is the chip. Same-layer distance from the USB nets to the microSD nets is 4.95 mm (DAT2 on F.Cu). Nothing of the speaker nets comes within 15 mm of them on the same layer.

## 3. Cell path

J202 is at (119.00, 27.50). KiCad reports the rotation as −90°, which is placement 270. Pin 1, `Net-(J202-Pin_1)`, is at (119.00, 27.50). Pin 2, BATN, is at (119.00, 29.50). Pin 1 is on top.

The housing's right edge is at x 120.70, so 1.30 mm of board remains. The courtyard ends at x 121.25, 0.75 mm inside the edge. The socket is vertical and fully on the board. Pad copper is 2.12 mm from the right edge.

Silk: "+" at (119.00, 24.00), above pin 1; "−" at (119.00, 33.00), below pin 2; "CELL" at (119.00, 34.59). The reference "J202" sits to the left of the plus, at (115.47, 24.52).

Cell positive is 38.8 mm, all 0.50 mm. On F.Cu it runs (119.0, 27.5) to (116.0, 30.5) to Q203's pad at (109.44, 30.50), about 11 mm. Q203 is at (108.5, 30.5). A second run of 0.50 mm already exists on B.Cu (26 mm of the 38.8). The F.Cu piece at y 30.5 passes between R216's pads. R216 is at (113.00, 30.50), rotation 90, pads at y 29.04 and y 31.96. The 0.50 mm track clears each pad by 0.65 mm. DRC is clean. That track can stay.

BATN is 33.4 mm, 17 segments, all 0.50 mm, all on F.Cu. It steps out to x 120.32 (copper edge at 120.57, 1.43 mm from the board edge) and comes back along y 26.45 to U205 and U204. Courtyard gap R216 to J202 is 1.48 mm. The cell path is short, wide, and the right polarity.

## 4. Speaker, switch, and the gaps along the edge

J6 pin 1 is at (93.20, 72.20), pin 2 at (98.20, 72.20). Pin 1 is `Net-(J6-Pin_1)`, the "+" silk at the same x. The 2.8 mm pads end at y 73.6, 2.40 mm inside the edge. The fab front is at y 76.00, flush. The model puts the wire holes on that front face at z 2.0 to 5.0, above the 1.6 mm board, and the front render shows the two openings toward the edge. The flush front does not cover the holes. A case wall still needs a slot for the wires.

Pin 1 is 22.1 mm at 0.30 mm (In2.Cu, then F.Cu to U2 pin 9). Pin 2 is 15.1 mm. The long run is 0.30 mm on In2.Cu; two F.Cu pieces of 0.4 mm and 0.1 mm at 0.25 mm sit on U2 pin 10. That is the 0.5 mm-pitch escape. U2 is still at 180°, and pins 9 and 10 are the two speaker nets. Under the terminal body the only extra copper is three GND vias.

SW203 is at (108.15, 72.62). The actuator courtyard reaches y 76.00. The anchor pads at y 74.32 are 1.33 mm inside the edge. Courtyard gap J6 to SW203 is 3.56 mm. USB centre to switch centre is 26.4 mm, so a plug does not cover the slide. J201's fab ends at x 86.22 and J6's fab starts at x 90.20 (3.98 mm). A USB plug about 14 mm wide reaches about x 88.8, roughly 1.5 mm short of the terminal body. Tight for a fat cable, and still a gap. D202 and D203 sit above the switch: D202's courtyard ends at y 67.18, the switch courtyard starts at y 71.66, 4.48 mm between them. Which way the slide is ON is still unknown. The silk still says "ON/OFF ?".

Courtyard gaps along the bottom, H3 to H4:

| Pair | Gap |
|---|---|
| H3 to J8 | 2.58 mm |
| C4 to J4 | 2.53 mm |
| J4 to J201 | 3.61 mm |
| J201 to J6 | 2.54 mm |
| J6 to SW203 | 3.56 mm |
| SW203 to H4 | 3.51 mm |

J5's footprint returns no courtyard. Its pads are at x 41.85 and 48.85, and J8's courtyard ends at x 37.67, so that gap is about the same size. "Equal 2.6 mm" is 2.5 to 3.6 mm on the board. Nothing along this edge is under 2.5 mm. J6's courtyard reaches y 76.56, inside the 0.6 mm the placement check allows past the edge.

The tightest soldered pair on the whole board is still C214 against D202, now 0.88 mm of courtyard in x (round 9 was a 0.64 mm corner). D202 to D203 is 1.15 mm. DRC is clean. No move.

The vertical reference "SW203" is at (113.56, 72.84), 0.9 mm, 90°. Its text box runs x 112.77 to 114.35. The switch courtyard ends at x 111.50 and H4's starts at x 115.01, so the ink clears the switch by 1.27 mm and the hole by 0.66 mm. The centre is nearer the hole than the switch. H4 has no reference, so the label is not ambiguous. Leave it unless a silk pass runs anyway.

## 5. USB, microSD, power, and the ADC pin

Connector to the USBLC6: D+ 12.4 mm on F.Cu, D− 13.7 mm with the via in section 1. Skew about 1.3 mm, both 0.20 mm. Full speed. The pair does not need to be routed as a pair.

The microSD socket stayed mouth-out. Its courtyard reaches y 76.01. Shared with the display: CLK `Net-(J3-Pin_3)` 94.5 mm, CMD `Net-(J3-Pin_4)` 104.5 mm, DAT0 67.8 mm, DAT1 10.0 mm, DAT2 10.2 mm, DAT3/CD 88.7 mm, LCD_BL 37.7 mm all on F.Cu. Line-out to I2S is 8.87 mm on the same layer (round 8 was 5.7 mm). Line-out to the speaker nets, and line-out to the card, have no same-layer approach inside 15 mm. The 0.80 mm between `Net-(J6-Pin_2)` and LRCK is the F.Cu fanout at U2 pin 10. The long speaker run is on In2.Cu.

Power width, whole net:

| Net | Length | Width |
|---|---|---|
| +3V3 | 343.4 mm | 0.50 |
| +3V3A | 55.4 mm | 0.30 (its class) |
| VSYS | 230.1 mm | 0.50, pad escapes under 0.6 mm at 0.25 |
| VIN | 92.4 mm | 0.50, same short escapes |
| VBAT | 42.5 mm | 0.50, one 0.4 mm piece at 0.25 on the charger pad |
| VBUS | 35.0 mm | 0.50 |
| BATN | 33.4 mm | 0.50 |
| cell positive | 38.8 mm | 0.50 |
| speaker pin 1 / pin 2 | 22.1 / 15.1 mm | 0.30, and the 0.25 mm pieces on pin 2 are the QFN escape |

No power run is both under 0.45 mm and longer than 0.6 mm, apart from +3V3A, which is a 0.30 mm class.

C207 is still at (49.50, 29.50), its VBAT_SENSE pad at (48.03, 29.50), U3 pin 7 at (49.19, 26.60). The pin-end copper is 1.65 mm plus 1.73 mm, 3.38 mm at 0.20 mm (round 9 was 3.39). The whole net is 77.3 mm (was 79). The long part is on B.Cu. Same-layer neighbours of the pin stub, inside 3 mm: the encoder RC `Net-(C6-+)` at 0.53 mm (round 9 was 0.96 mm) and CHG at 1.27 mm. LCD_BL and the SPI nets are not among them. The 100 nF is on the pin, the read is averaged, and the nearer neighbour is a slow encoder edge.

Four 0.30 mm ground vias sit in the gaps of the WROVER pad-39 land, at (48.68, 18.39), (48.68, 19.79), (50.08, 18.39) and (50.08, 19.79). Round 7 said two are enough. J5 was not rotated. Its mouth stays toward the edge.

`renders/bottom.png` is from 2026-10-16 14:43, before this spread. The front, iso and top renders are from 13:03–13:04, the same minute as `6b0a565`.

## Before the order

One copper change: a keep-out on the J201 shell, and D− / CC1 / CC2 brought out behind the body, with the stitch vias inside the shell removed. No placement change and no full re-route.

Still on a sample, unchanged: PJ-320D pin order, TF-01A drawing, which side of SW203 is ON, EC11 shaft length, and the HS20S010B pin-1 / pin-7 / pin-8 check.

---

# Grok review, round 9 (C207, charge LEDs, silk, 3D, 2026-10-16)

Measured commit `e6b5f01` (changes since `f7bd61e`) on the placed board with KiCad's pcbnew. The board file was not changed. DRC was not re-run; the recorded result stands (0 errors, 0 unconnected, 0 parity, 18 warnings). The board was re-routed, so the track lengths in round 8 do not describe this copper. This round measured the four checks below.

Nothing has to move before an order. The one optional item is a silk nudge, and it does not need a new route.

## 1. C207 is on the ADC pin

C207 sits at (49.5, 29.5), under the module. Its VBAT_SENSE pad is at (48.03, 29.50) and U3 pin 7 (IO35) is at (49.19, 26.60). The copper between them is two F.Cu segments, both 0.20 mm wide:

- 1.65 mm, (48.03, 29.50) to (49.19, 28.34)
- 1.74 mm, (49.19, 28.34) to (49.19, 26.60)

That is 3.39 mm. The 100 nF is on the pin side of the incoming run. R217 and R218 stay at (112, 41) and (116, 41). The whole VBAT_SENSE net is now 79 mm (it was 86 mm with the capacitor at the divider).

Same-layer neighbours of those two segments, closer than 2 mm: an encoder RC net at 0.96 mm (`Net-(C6-+)`), CHG at 1.27 mm, and I2S BCK at 1.91 mm. LCD_BL and the SPI nets are not among them. The backlight still passes the long incoming run, at about (58.6, 24.6), before the pin. The capacitor is downstream of that, which is what the move was for.

## 2. Charge LEDs — the 0.64 mm is a courtyard corner

D202 is at (104.30, 66.60) and D203 at (110.30, 66.60), side by side above SW203. They have left the corner by H4. Courtyard gap D202 to D203 is 1.35 mm. Each LED clears SW203 by 3.89 mm.

C214 (100.50, 62.50, rotated 90) meets D202 at 0.64 mm, and that number is the courtyard corners: 0.28 mm in x and 0.59 mm in y. The nearest pads are C214.1 (VBUS_SENSE) and D202.1 (CHG_N), 3.56 mm centre to centre, different nets. The bodies are about 2 mm apart. Hand soldering does not bridge that.

R212 stays at (116.50, 64.00) and R214 at (116.50, 60.00). The anode net of D202 is 13.5 mm (6.6 on F.Cu, 6.8 on In2.Cu). D203's anode net is 13.0 mm, all on F.Cu. CHG_N is 66 mm and PGOOD_N is 61 mm, almost all of it on In2.Cu up to the charger. Both are slow.

"C" and "P" are 0.8 mm text at the LED y, anchored just left of each body. The ink clears D202 pad 1 and D203 pad 1 by 0.20 mm and does not touch either pad, so it is not on copper JLCPCB would clip. CELL and SPK on the JST outlines are the warnings already recorded.

## 3. Silkscreen — references are off the pads, and a few sit in the gap

Footprint references: none on a pad, none left on a body, none within 0.15 mm of the board edge, and none nearer another part than its own by the check in `place_refs()`. Recovery pads still carry signal names.

The vertical references in the 4.0 mm columns are the weak spot. A text whose centre is 1.98 mm to one side of a part on that pitch lands on the midpoint of the gap. The checker still calls it the owner's, by about 0.03 mm (0.79 mm to the own courtyard, 0.82 to 0.86 mm to the neighbour). A person cannot see that.

Measured centres, reference against the part:

| Ref | Part | Reference | Faces |
|---|---|---|---|
| C10 | (29.5, 40.5) | (27.52, 40.50), 90 deg | the gap with C9 |
| C15 | (12.5, 46.8) | (10.52, 46.80), 90 deg | the gap with C16 |
| R10 | (49.5, 40.5) | (47.54, 40.50), 90 deg | the gap with C6 |
| R11 | (49.5, 46.8) | (47.54, 46.80), 90 deg | the gap with C7 |
| R12 | (49.5, 53.1) | (47.54, 53.10), 90 deg | the gap with C8 |
| R207 | (103.0, 41.0) | (101.04, 41.00), 90 deg | the gap with R205 |
| R208 | (107.0, 41.0) | (105.04, 41.00), 90 deg | the gap with R207 |

R218 against R217 has the same 0.03 mm margin. Both are 470k. R205, R207 and R208 are all 10k. C15 and C16 are both 2.2 uF. R10, R11 and R12 are 10k next to 100 nF capacitors, and the letter tells them apart.

The pair that matters for hand assembly is C9 and C10. Both are 1206. C9 is 100 nF and C10 is 10 uF, and "C10" sits between them. C9's own reference is on the outside, at (23.29, 40.50), horizontal. Nudging the gap texts to the outside of each column is silk only.

## 4. 3D — pads match KiCad, the encoder picture is the 24.5 mm part

The three project footprints differ from KiCad in the model only. Pad count, number, type, shape, position, size, drill and layers match:

- J201, the HRO USB-C receptacle, 22 pads. Model is the EasyEDA WRL, offset (0, +1.40, 0), turned 180 deg about z.
- SW1, the Alps EC11E H20 footprint, 7 pads. Model is `SW-TH_EC11XXXXXXXX-L11.7-W12.0-H24.5-P2.5-LS14.5.wrl`, offset (7.5, -2.5, 0), turned -90 deg about z, scale 1.
- J3, the 1x10 socket, 10 pads. It keeps KiCad's socket model and adds `HS20S010B_on_sockets.wrl`.

These EasyEDA models are in the usual 0.1 inch units (a USB-C body in the same library spans 8.9 x 7.9 x 4.1 mm once multiplied by 2.54, at scale 1). The encoder's highest vertex is 9.6457, which is 24.50 mm above the board. That is the 24.5 mm overall part. The footprint is the 20 mm shaft, and `07-integrated-board.md` already says the 24.5 mm part is not the choice. Shaft length for the case is still open. The pads are the same for those heights, so the copper is unchanged.

The display model puts the module PCB at z 11.0 and the glass front at 13.9 (1.6 mm PCB plus a 1.3 mm body). The drawing says the module PCB is about 11 mm up and the glass front about 14 mm, with the module 2.5 mm thick. The script calls the model a picture, not a fit check. The front face matches the drawing.

## Before the order

No placement change and no new route. Optional, silk only: move the vertical references in the table above out of the shared gap, C10 first. Still on a sample, unchanged: PJ-320D pin order, TF-01A drawing, which side of SW203 is ON, EC11 shaft length, and the HS20S010B pin-1 / pin-7 / pin-8 check. J5 stays mouth-out.

---

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
