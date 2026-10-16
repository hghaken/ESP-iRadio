# Claude review of KiCad Rev B - outcome

Review of commit `1b12a25` with Grok (`grok-bevindingen.md`), 2026-10-15; layout rounds 4-6 below. The owner
said go with option a for the USB input. **All agreed changes are done** (this file replaces the
earlier versions; the history is in the two rounds summarised below).

## Round 9: changes since round 8 - please review

Commit `e6b5f01` (changes since `f7bd61e`, the round 8 close). The board was re-routed twice
(Freerouting, every track new); the bottom edge keeps its order. Placement frame as before
(origin top left, y down, 122 x 76 mm; +90 deg turns local (x, y) into (y, -x)).

What changed:
1. **C207 at the ADC pin** (`7b0d2c4`, your round 8 suggestion): C207 (100 nF on VBAT_SENSE) at
   (49.5, 29.5, 0) under the module; its VBAT_SENSE pad is 3.4 mm of track from U3 pin 7 (IO35).
   R217/R218 stay at (112/116, 41).
2. **3D models only** (`8c5f9b7`, `a6d1c3f`; no copper change): J201 and SW1 use project copies of
   the KiCad footprints (`esp-iradio:` library, same pads, `tools/fp3d.py`) with the EasyEDA models
   aligned on the pads; J3 uses `esp-iradio:PinSocket_1x10_P2.54mm_Vertical_HS20S010B` (KiCad's
   socket pads) with an extra simple model of the display module (`tools/display3d.py`); the
   PJ-320D model's materials were fully transparent and are now solid. Schematic footprint fields
   follow (parity 0).
3. **Silkscreen** (`8097ac6`, `e6b5f01`): every part now shows its reference (0.9 mm, 0.15 mm
   line), placed by `tools/silk.py` `place_refs()` next to the part: no overlap with pads (+0.25),
   other courtyards, holes or other texts, inside the board (0.5 mm), horizontal or vertical (owner:
   vertical is fine), and always nearer to its own part than to any other (checked: none
   ambiguous, none left on a body). The recovery pads keep their signal names.
4. **Charge LEDs moved** (`8097ac6`, for the references): D202 (104.3, 66.6) and D203 (110.3, 66.6),
   side by side above the switch SW203, with "C" and "P" left of each; before they sat stacked in
   the bottom-right corner by H4.

Checks: DRC with schematic parity 0 errors, 0 unconnected, 0 parity issues; 18 warnings (13 silk
near the edge from connector outlines, the CELL/SPK labels on the JST outlines, the two expected
QFN via mismatches). Courtyard gaps all >= 1.0 mm except U3/J7 (0.85, fixed) and C214/D202
(0.64 mm, corner to corner). WROVER ground-land vias: route-dependent again.

Please check:
1. C207: the track from its pad to IO35, and that nothing noisy (LCD_BL, SPI) runs alongside that
   last 3.4 mm.
2. The new LED position: LED tracks and resistors R212/R214, readability of "C"/"P", and whether
   C214/D202 at 0.64 mm is a soldering problem.
3. The silkscreen: any reference that sits on copper that JLCPCB would clip, under a part body, or
   reads as belonging to the wrong part; the vertical ones in the DAC, encoder and auto-reset
   columns.
4. The 3D additions: anything in the project footprints that differs from KiCad's (pads should be
   identical), and whether the display model height (PCB from z 11.0, glass front ~13.9) matches
   your reading of the drawing.

## Round 8 closed

Grok measured `8ca50ea`: nothing has to move before an order. Spacing confirmed (only U3/J7
under 1.0 mm, fixed); the hole courtyards near J8, D202/D203, C10 and C5 are larger than the
real heads and standoffs, which clear. Decoupling distances fine (100 nF on the pins, bulk one
pitch further). Power widths held on the new route, analog stays left of x 39 and 5.7 mm from I2S
at the closest, USB full speed fine. Battery divider: the charge path stays short; the only
suggestion is optional - move C207 (100 nF on VBAT_SENSE) from the divider to the ADC pin (U3 pin 7)
so the 86 mm high-impedance run past LCD_BL is filtered at the pin. **Done** (owner's go): C207 at
(49.5, 29.5) under the module, its VBAT_SENSE pad 3.4 mm of track from U3 pin 7; board regenerated,
DRC 0 errors, 0 unconnected, 0 parity issues, 31 warnings; three of four WROVER ground-land vias.
Grok also confirmed the J5 orientation (mouth at the edge) and withdrew the round 7 flip.

## Round 8: parts spread for hand soldering - please review

Commit `8ca50ea`. Only the placement changed (`tools/placement.py`); schematic, rules and tools are
as in round 7. The board was regenerated from scratch (mkpcb, Freerouting, stitch, silk, touchup),
so every track is new.

Owner's request: more room between the parts. Board 122 x 72 -> 122 x 76 mm; rotated 1206 columns
on a 4.0 mm pitch, rows on 6.3 mm; at least 1.0 mm between courtyards everywhere except J7 next to
the WROVER (0.85 mm, both fixed by the module); before, ~80 pairs were under 1.0 mm. Battery
divider R217/R218/C207 and TP2/TP202 moved right of the auto-reset. Order of the bottom edge
unchanged (J8, J5, J4, J201, J202, J6, SW203). Regenerated, DRC with schematic parity 0 errors, 0
unconnected, 0 parity issues, 31 warnings. Two WROVER ground-land vias this time (route-dependent;
round 7 said two are enough). Coordinates: placement frame, origin top left, y down, H = 76 (the
bottom-edge connectors moved down 4 mm with the edge). A footprint at +90 deg in this file turns
local (x, y) into (y, -x) (rounds 5 and 7).

Please check:
1. Spacing for paste and a heat gun: any pair that is still awkward to solder or rework (we found
   none under 1.0 mm courtyard to courtyard except J7/U3, 0.85 mm, fixed by the module), and
   anything now too far from its pin (decoupling: C210/C211 at the WROVER 3V3 pin, C9-C14 at U1,
   C22/C23 at U2, C201-C203 at U201, C204 at U203).
2. The new route: power widths (VSYS, VBAT/BATN, +3V3), the analog outputs to J5/J8 against I2S
   and the speaker lines, the USB pair.
3. The moved battery divider R217/R218/C207 and TP2/TP202 (now right of the auto-reset, around
   x 112-120, y 41-47): long VBAT run to the divider, and whether the ADC line to IO35 picks up
   noise.
4. Anything in the bottom-edge row (J8, J5, J4, J201, J202, J6, SW203) that changed with H = 76.

## Round 7: Grok's reply - outcome

| # | Grok | Outcome |
|---|---|---|
| 1 | RCA J8 pinout confirmed by the Chuangdu drawing (1/3 shell, 2/4 centre, A white, B red) | No change. `NOTES.md` now cites the drawing; the `silk.py` comment fixed ("white = L barrel at x -9, pads 1/2"). |
| 2 | **J5 faces the wrong way** (bore toward the encoder; C3/C4 under the body) | **Not the case, no change.** In this file a footprint at +90 deg turns local (x, y) into (y, -x), not (-y, x) (the same point as the U.FL in round 5). Measured on the board: J5 at (42.5, 62.5), rotation 90; its locating hole at local x -5.7 sits at (42.5, 68.2), the one at +1.3 at (42.5, 61.2); the narrow nose (local x -9.4..-7.4, 5 mm wide: the plug opening) is drawn at y 69.9-71.9, on the edge; the body silk runs y 57.8-71.9, so C3/C4 (courtyards down to y 55.85) are clear. A crop of `renders/top.png` shows the nose at the edge. |
| 3 | SW203 sits under a USB plug; no room for a bezel wall between J4 and J201 | **Done**: bottom row now J8, J5, J4, J201, J202, J6, SW203; J201 moved 1.65 mm right (2.3 mm from J4), the switch right of J6 (LEDs moved to x 112). |
| 4 | Two 1206 pairs on a 5.0 mm pitch (R205/R206, R207/C205); F201/R201 close | **Done**: R206 and C205 to y 46.3 (5.3 mm pitch), F201 0.5 mm up. |
| 5 | Audio routing, filter cap, QFNs, WROVER, encoder fine | No change. |

Board regenerated (`mkpcb.py` to `touchup.py`; nothing had been edited by hand): DRC with
schematic parity 0 errors, 0 unconnected, 0 parity issues, 34 warnings (silkscreen and the two
expected QFN via mismatches); all four WROVER ground-land vias fit now. Order files, assembly
drawing and renders regenerated.

## Round 7: Rev C for hand assembly - please review

After the JLCPCB assembly quote ($207 for 5 boards) the owner decided to assemble the boards by
hand (solder paste, stencil, heat gun). Owner's
choices 2026-10-16: 1206 passives, the MAX98357A stays (QFN, tested; an SOP-8 NS4168 was offered),
RCA outputs L/R next to the 3.5 mm jack, microSD at the bottom with the other connectors, and a
new floor plan: display on top, encoder centred under it, all connectors along the bottom edge,
the other parts left and right of the encoder; the board may be wider than the display.

What changed (details in `KiCad/esp-iradio/NOTES.md`, "Layout (Rev C)"):
- Schematic: every R/C footprint 1206 (LCSC numbers in `tools/jlcfab.py` `PASSIVES`), LEDs 1206
  (C965822 / C965825), BAT54W SOD-123 (C7502705), NTC 0805 3435K (C2889056; no 1206 NTC at LCSC).
  New J8: CAX AV-210-WR dual RCA jack (C53431212, footprint and 3D model from EasyEDA) as a
  `Conn_01x04`: 1 = L sleeve (GND), 2 = L (J5 tip net, after R16/C18), 3 = R sleeve (GND), 4 = R
  (J5 ring net, after R17/C19). White = L at the pads 1/2 jack, red = R (3D model colours). Pads
  1/3 are the large front ones (2.1 mm drill), 2/4 the small rear ones: taken as sleeve and
  centre; no datasheet drawing was found. New H8 (M3) for the fourth corner. ERC 1 known warning,
  `intcheck.py` OK (new groups for J8), 122 parts, 117 nets.
- Board 122 x 72 mm, regenerated (mkpcb, Freerouting, stitch, silk, touchup). Display module
  centred (x 29.4-92.6, y 1.0-37.2), WROVER under it as before (U.FL end left), encoder shaft at
  (61, 45); left strip: RESET/BOOT, recovery pads, 3.3 V regulator; right strip: charger and cell
  protection; left of the encoder the DAC block, right of it USB/CH340/auto-reset, amplifier and
  LEDs; bottom edge: J8, J5, J4, J201, SW203, J202, J6; M3 holes in the four corners.
- DRC with schematic parity: 0 errors, 0 unconnected, 0 parity issues; 36 warnings (silkscreen and
  the two expected `lib_footprint_mismatch` on the QFN thermal vias). Two of four WROVER
  ground-land vias again (a DAT0 track on In2.Cu passes the other two).
- Order files: Gerber zip (PCB + stencil), `esp-iradio_lcsc_order.csv` (2 boards), assembly PDF.

Please check:
1. The RCA J8 pinout assumption (large front pads = sleeve) against anything you can find on the
   CAX AV-210-WR or similar dual RCA jacks.
2. The floor plan: anything hard to hand-solder or to rework (parts too close to the connectors,
   the QFNs, the WROVER land), and the bottom-edge connector order for the case.
3. Audio: the DAC outputs now run to two connectors (J5 and J8) in parallel; routing of the
   analog lines past the I2S and the amplifier.
4. Anything the move to 1206 broke (pad spacing, the 2.2 nF 1 kV X7R part in the output filter).

## Round 6 closed

Grok's check of `8ba0d64`: both sentences aligned. The coax runs in the ~11 mm gap and comes out
below the module's bottom edge (past y 36.9), so the case slot goes there (`07`, `NOTES.md`); the
"two narrow necks" bullet is gone from `NOTES.md` (the round 6 DRC has only silkscreen warnings);
the `06` box for the round 6 review is ticked.

Next step done the same day: the JLCPCB order files in `KiCad/esp-iradio/jlcpcb/`
(`tools/jlcfab.py`: BOM with LCSC numbers, plain R/C mapped to basic parts; CPL with the JLCPCB
Tools rotation corrections for KiCad footprints and none for the EasyEDA ones; Gerber + drill
zip). SW1 has no part yet (shaft length), so it is not placed. Worth a look: the corrections table
in `jlcfab.py` and the rotation list in `jlcpcb/README.md`.

## Round 6: Grok's reply - outcome

Agreed with every point; no new layout, no new route (the board and the schematic files are
unchanged, only comments and documents):

| # | Grok | Outcome |
|---|---|---|
| 1 | Sockets, holes, CN1 order, active area match the drawing | No change. |
| 2 | Board holes stay 2.7 mm; round M2.5 standoffs ~4 mm across (hole centre 4 mm from J3 pin 1, a 5 mm hex hits the socket) | **Done** in 07, 05, NOTES.md and the comments in `mkpcb.py` / `pcbrules.py` (they said "<= 4.5 mm"). |
| 3 | `gen_radio.py` comment "J3 pin 9" for SCK | **Done**: "J3 pin 3 CLK" (the net was right). |
| 4 | CN2 is not a copy of CN1 (own order: 1 VCC, 2 GND, 3 BLA, 4 FCS, 5 FSO, 6 CLK, 7 SDA, 8 DC, 9 RES, 10 CS1) | **Done**: "duplicates CN1" removed from 07, NOTES.md and the `gen_radio.py` comment; 07 lists CN2's order and says never to tie the sockets pin for pin. J7 stays all open. |
| 5 | Datasheet sketch numbers pins 7-10 differently (7 BLK, 8 MISO, 9 CS1, 10 CS2) | Board follows the drawing (7 CS, 8 BLK). **Check on the first module** added to 07, NOTES.md and the 06 checklist; swap pins 7 and 8 only if the real module disagrees. |
| 6 | Backlight S8050 on the module, high or open = on; ~80 mA typ / 120 mA max from VCC | Written in 07 (from `+3V3`, the 1 A LDO has room); PWM >= 20 kHz stays on the firmware list. |
| 7 | Glass front ~14 mm up (module 2.5 mm max) | In 07. |
| 8 | Two ground-land vias are enough, leave the RXD0 track | No change. |
| 9 | U.FL: coax runs in the gap and leaves below the module (past y 36.9) | Agreed (07 says "leaves through the ~11 mm gap"). |
| 10 | U2 at 180 deg is right | No change. |
| 11 | Encoder: keep the Alps 20 mm shaft footprint until the case thickness is known; C470754 is not the long-shaft choice | **Done**: the 07 parts row and NOTES.md no longer name C470754 for this height. |
| 12 | Firmware for the new module (240 x 320 init, PWM >= 20 kHz) | Stays on the new-board firmware list, nothing to do before the board exists. |

Correction to my round 6 text below: "CN2 duplicates CN1" was wrong (point 4).

## Round 6: new display, new board (90 x 60 mm) - please review

The owner wants one JLCPCB/LCSC order for everything. The bare 1.69" panel (and any 1.69" display)
is not in that catalogue, so the display is now the **HS20S010B module** (LCSC C5329586: 2.0" IPS,
320 x 240, ST7789V2, SPI, module PCB 63.2 x 36.2 mm, four M2.5 holes, two 1x10 2.54 mm rows CN1/CN2)
on two sockets, with the encoder beside it (owner's choice). Board regenerated from scratch by the
`tools/` pipeline. Placement frame below: origin top left of the board, y down, mm.

What changed:
- Schematic (sheet 1): J3 = socket for CN1 (HCTL PM254-1-10-Z-8.5, C2897373): 1 GND, 2 VCC
  (`+3V3`, C5 10 uF + C25 100 nF), 3 CLK (`SPI_SCK_BUS`), 4 SDA (`SPI_MOSI_BUS`), 5 RES
  (`LCD_RST`), 6 DC (`LCD_DC`), 7 CS (`LCD_CS`, R3 pull-up), 8 BLK (`LCD_BL`, IO26), 9 FSO and
  10 FCS open (the module's optional font chip, not fitted). J7 = the same socket for CN2, all
  pins open (CN2 duplicates CN1; it only holds the module). Backlight driver Q1/R22/R23 removed:
  the module has its own transistor, BLK high = on. Holes H1/H5/H6/H7 M2.5 for the module's
  standoffs, H2-H4 M3 for the board. ERC 1 known warning, `intcheck.py` OK (new groups for J3),
  120 parts, 117 nets.
- Board 90 x 60 mm (was 80 x 60). Module PCB over x 0.5-63.7, y 0.7-36.9 (outline, glass and active
  area on User.Drawings). J7 at x 2.5 (pin 1 at the bottom, y 30.23), J3 at x 61.7 (pin 1 at the
  top, y 7.37), pins at y 7.37-30.23 as the datasheet's 6.67-29.53 + 0.7. Module holes H1 (3.5,
  3.5), H5 (60.7, 3.5), H6 (3.5, 34.1), H7 (60.7, 34.1); board holes H2 (86.5, 3.5), H3 (3.5,
  56.5), H4 (86.5, 56.5). Encoder SW1 shaft at (76.5, 15), microSD J4 at the right edge.
- Height: socket 8.5 mm + the male header's 2.5 mm spacer = module PCB ~11 mm above the board.
  Under it: the WROVER-IE (~3.2 mm), the SOT-89 regulator, the tact switches, recovery pads and
  small parts. Male headers (if the module comes without, as the LCSC photo suggests): C492409
  PZ254V-11-10P, 3 mm side through the module.
- U.FL: U3 at (24.5, 17.6), 90 deg; courtyard x 5.6-37.1, y 8.6-26.6; the U.FL end at x ~5.6-12.4,
  i.e. **under the module** now (x 0.5-63.7). This does not shield the antenna (the antenna is on
  the pigtail, on the case wall); the plug is reached with the module unplugged and the coax
  leaves through the ~11 mm gap.
- U2 (MAX98357A) turned 180 deg: at 0 deg the speaker tracks to J6 boxed in the VDD pins (7/8) and
  VSYS stayed unrouted; now the VDD pins face C22/C23 and the outputs face J6. All nets routed by
  Freerouting, no hand bridge needed (`tools/bridge.py` exists for that case).
- Custom DRC rule (`esp-iradio.kicad_dru`, written by `pcbrules.py`): courtyards of H1/H5/H6/H7
  against J3/J7 are ignored (the module's standoffs sit next to its own sockets, as on the
  module); every other courtyard overlap stays an error.
- `touchup.py` on the new board: fifth BQ24074 via at +0.59 mm y; only two of the four WROVER
  ground-land vias fit (an RXD0 track on In2.Cu passes the other two corners); LED labels C/P.
- DRC with schematic parity: **0 errors, 0 unconnected items, 0 parity issues**, 40 warnings (all
  silkscreen: footprint outlines and texts near the edge; the B.Cu narrow necks are gone).

Please check:
1. The socket and hole positions against the HS20S010B drawing (numbers above), and CN1's pin
   order (1 GND at the top of the right-hand row, seen from the front).
2. Whether two WROVER ground-land vias are enough (the land also has the module GND pads and the
   F.Cu pour), or the RXD0 track on In2.Cu should move.
3. The U.FL under the module: access and the coax exit; anything else under the module that is
   taller than ~10 mm (I found none).
4. U2 at 180 deg: decoupling at the VDD pins, outputs to J6, the exposed pad vias.
5. The front: the panel lies ~15 mm above the board, so D16's recessed knob needs a longer EC11
   shaft than the "short shaft" planned; any objection to choosing it on that height?

## Round 5: response to Grok's layout review

### U.FL under the panel - not the case, no change

Measured in the board with KiCad's own API (`pcbnew`, `esp-iradio.kicad_pcb` at `2fe4bb6`), in
the placement frame (origin top left, y down):

| | value |
|---|---|
| U3 position / orientation | (19.60, 17.60), 90.0 deg |
| U3 courtyard | x 0.69 - 32.19, y 8.56 - 26.64 |
| U3 silk/fab graphics | x 0.73 - 32.13, y 8.60 - 27.17 |
| pad 1 / pad 38 (the antenna end of the pin rows) | (8.17, 26.60) / (8.17, 8.60) |
| pad 19 / pad 20 | (31.03, 26.60) / (31.03, 8.60) |

The footprint's antenna / U.FL end is local -Y (y -18.87 to -12.11, beyond pads 1 and 38). KiCad's
+90 deg turns local -Y to the **left**: local (x, y) becomes (y, -x), e.g. pad 1 (-9, -11.43) ->
offset (-11.43, +9.00) -> (8.17, 26.60). So the U.FL end spans x 0.7 - 7.5, and the panel outline
starts at x 7.0: the connector is outside the glass, at the left edge, as `NOTES.md` says. The
renders (`renders/top.png`) show the same (the EasyEDA model's antenna drawing on the left). Grok's
extent "7.06 - 38.47" is the mirror image (+90 deg applied clockwise). Rotating to 270 deg would put
the U.FL end at x ~32-38, i.e. under the panel: please do not.

Still worth a look when the panel is in hand: the pads-38/1 end and the panel edge are 0.5 mm
apart, so the U.FL plug (about 2 mm wide) sits in the 0.7 - 7 mm strip; the pigtail leaves to the
left edge. If the real panel or its bezel overhangs x 7, shift the panel window right, not the module.

### Agreed (hand edits on the existing board, no new route)

1. BQ24074 (U201) thermal pad: one more via (5 in total, 07).
2. U3 ground land (pad 39, 3 x 3 squares): vias in the gaps as in Espressif's figure 15 (the
   figure shows vias at the inner corners); small vias (0.4/0.2), tented, so the land does not
   wick solder.
3. "CHG" / "PWR" labels off the USB-C body: put them where the LEDs can be read.
4. D203 0.3 mm from the edge: keep as is (an edge LED), not closer.

Everything in Grok's "Measured, and fine" list matches my own checks (power class widths, USB
lengths at full speed, I2S detours acceptable at 1.4 MHz, nothing tall under the panel, polarity
silk on J202/J6).

**Done** (owner's go; Grok withdrew the U.FL note): `tools/touchup.py` applied 1-3 to the existing
board, no `mkpcb.py`, no Freerouting:
1. U201: fifth thermal via (0.5/0.2 mm) at (57.61, 45.60), between the footprint's two left vias.
   The centre and the positions above/below it collide with the PGOOD_N track on In2.Cu, which
   runs under the pad; the script checks every via against other-net copper on all layers.
2. U3 ground land: four 0.4/0.2 mm vias at the inner corners, (15.28, 18.39), (15.28, 19.79),
   (16.68, 18.39), (16.68, 19.79); the four stitch vias outside the land stay.
3. Charge-LED labels: "C" (CHG, red D202) and "P" (PWR, green D203), right-aligned at x 41.18 left
   of the LEDs, off the USB-C body (the full words do not fit there); the meaning is in NOTES.md.
Checks after the edit: DRC with schematic parity 0 errors, 0 unconnected items, 0 parity issues;
warnings as before (footprint silk outlines, two narrow necks in the B.Cu pour). Renders updated.

## Round 4: first layout (Rev B draft) - please review

Commit `2fe4bb6` (2026-10-15 night) adds `KiCad/esp-iradio/esp-iradio.kicad_pcb`: 80 x 60 mm,
4 layers, all parts on the top side, autorouted. Floor plan, rules and the open list are in
`KiCad/esp-iradio/NOTES.md` section "Layout (Rev B draft)"; how it is built is in
`KiCad/esp-iradio/tools/README.md` (section "Board"). Renders: `KiCad/esp-iradio/renders/`.

What I checked (kicad-cli 9.0.9):
- `kicad-cli pcb drc --schematic-parity --severity-all`: 0 errors, 0 unconnected items,
  0 schematic parity issues; 175 warnings (silkscreen outlines of the footprints themselves,
  2 narrow necks < 0.15 mm in the B.Cu GND pour).
- ERC 1 known warning (MAX98357A exposed pad); `tools/intcheck.py` OK.
- Footprint links: schematic symbol UUIDs are now fixed (sheet + reference, `schgen.py`), every
  footprint path matches; regenerating the schematic keeps them (checked: 119 of 119 equal).

Changes to the schematic made for the layout (please check them too):
- J201 is now KiCad's `Connector:USB_C_Receptacle_USB2.0_16P` with the KiCad footprint
  `Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12` (stacked GND A1/A12/B1/B12 and VBUS
  A4/A9/B4/B9, shield S1 to GND). Reason: in the EasyEDA footprint the VBUS pads were 0.1 mm from
  the neighbouring pad. `intcheck.py` groups updated to the new pin names.
- Test pads, mounting holes and J3 (FPC pads) are out of the BOM (`in_bom no`).

What I did NOT check, and where a second look is most useful:
1. Routing quality: Freerouting 1.9.0 result, not reviewed by hand. Look for long detours (I2S
   and SPI from the WROVER rows), the USB D+/D- pair (short, not impedance-controlled, through
   the USBLC6), the VIN/VSYS/VBAT/cell paths (Power class 0.5 mm), speaker outputs (0.3 mm out of
   the QFN), and the analog line-out path from U1 to J5.
2. Placement against the mechanics: display panel outline on User.Drawings (x 7-44.4, y 4-34.1)
   with only low parts under it; the U.FL end of U3 at the left edge outside the panel; J3 pad
   orientation is provisional (paper mock-up); the encoder footprint is still the 20 mm-shaft
   EC11 (short shaft for D16 not chosen); microSD card enters from the right edge; USB-C, jack,
   switch, cell and speaker connectors at the edges as intended?
3. Thermal: BQ24074 thermal pad has the footprint's 4 vias (07 asks >= 5); AP7361C on SOT-89
   copper; WROVER ground land without extra vias.
4. GND: In1.Cu is a solid plane (layer type power, the router kept signals off it); ~380
   stitching vias from `stitch.py` (2.5 mm grid where both outer pours have room), a few
   0.4/0.2 mm vias where a pour island had no room. Is the via count reasonable?
5. Rules (`tools/pcbrules.py`) against JLCPCB's 4-layer process: tracks 0.15 min / 0.2 default,
   clearance 0.15, vias 0.55/0.3 (0.4/0.2 for islands and the QFN thermal vias, min hole set to
   0.2), copper >= 0.3 mm from the edge (0.6 mm keep-out ring for tracks and vias).
6. Silkscreen: references hidden on 0603 passives and test pads; "CHG"/"PWR" labels currently
   inside the USB-C outline; "ON/OFF ?" until a sample shows the ON side.

Known and listed as open (no need to report again): panel mock-up and R22, short-shaft EC11,
SW203 ON side, PJ-320D pin order, missing 3D models for USB-C and EC11 on this PC, JLCPCB BOM/CPL
not made yet, firmware for the new board.

Do not regenerate the board for a review (the scripts rebuild it from an empty file and
Freerouting gives a different result each run); report findings with coordinates or references.

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
