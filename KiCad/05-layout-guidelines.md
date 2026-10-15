# 05 - Layout guidelines

## Board and mechanics

- Start from the Olimex outline (from their Rev.D KiCad file) placed on a fab layer, so the
  carrier's headers line up exactly. Measured from the Rev.D board file: board **27.94 x 48.26 mm**,
  header rows **25.40 mm** apart, each 1x19 row spanning the full 48.26 mm length (pin 1 at the
  antenna end); the WROVER module sticks out ~6 mm past the board's antenna edge; the micro-USB,
  the reset button and the Li-Po JST (DW02S) sit at the opposite (bottom) edge. Place the Olimex
  board so that:
  - its **USB connector** and **Li-Po JST connector** point to a board edge and stay reachable
    (the owner plugs a power bank into the USB connector daily, and the PC for USB updates);
  - its **reset button** stays reachable (or leave a hole above it);
  - its **WROVER antenna** sticks out past the carrier edge, or the carrier has **no copper on
    any layer** (and no parts) under the antenna and ~10 mm around it. The radio already fights
    for WiFi throughput next to Bluetooth: any loss at the antenna hurts (see `AGENTS.md`, "WiFi
    and Bluetooth share one 2.4 GHz radio").
- Display on the board (D2): the module sits on the front side, its glass parallel to the board,
  next to (not over) the Olimex board, so the case front shows the display and the encoder knob.
  Agree the front layout with the owner first (screenshot of the 3D view).
- USB-C, 3.5 mm jack and speaker connector on one board edge (the back of the case); the Olimex
  micro-USB on an edge too (USB updates from the PC).
- microSD socket at an edge where the card can be inserted with the board in the case.
- 4 mounting holes, one per corner, clear of tracks (keep-out 1 mm around the screw head); sized
  for M2.5/M3 screws into heat-set inserts in the printed case.
- Height: the Olimex board on female headers sits ~8.5-11 mm above the carrier; parts under it
  must stay below that (only 0805 passives there).

## 3D-printed case (decision D3)

The case is designed around the board, so the board files must give the case designer everything:
- Export a **STEP** of the assembled board (File -> Export -> STEP, with all 3D models, including
  the Olimex board and the display module models from `03-parts-and-footprints.md`).
- Export the board outline and mounting holes as **DXF** (Edge.Cuts + holes) for 2D sketches.
- Document in `NOTES.md`: board thickness, tallest part per side, display glass position and active
  area, encoder shaft position and length, connector openings (USB-C, micro-USB, jack, microSD),
  speaker size and the space it needs (D4: one speaker, mounted in the case wall with a grille,
  wired to J6).
- Keep the WROVER antenna end of the case free of metal and of the speaker magnet; plastic only.
- Leave room for the Li-Po (5000 mAh pouch, roughly 60 x 50 x 8 mm: measure the owner's cell) and
  its JST cable to the Olimex board.

## Routing

- 2 layers: top for signals and short power, bottom as solid ground plane. Stitch ground vias
  around the board edge and next to signal vias.
- SPI (`SPI_SCK`, `SPI_MOSI`, `SPI_MISO`, CS lines): the display runs at 26.7 MHz. Keep them short
  (< 80 mm), over unbroken ground, no stubs; series resistors at the ESP32 end of `SPI_SCK`
  (R8, 22 Ohm) and `SPI_MOSI` (R9, 0 Ohm; 22-33 Ohm if ringing shows).
- I2S (`I2S_BCK` 1.4-2.8 MHz, `I2S_LRCK`, `I2S_DATA`): short, over ground, away from the analog
  output and the speaker traces. They go to two chips (PCM5102A and MAX98357A): route them as a
  short daisy chain or a short T, not as two long branches.
- Analog line out (`LINE_L`, `LINE_R`, `AGND`): short, guarded by ground, away from the amplifier
  output and the SPI clock.
- Amplifier (MAX98357A, TQFN-16): `+VAMP` and the speaker outputs >= 0.8 mm (1 mm preferred), the
  10 uF + 100 nF right at the VDD pins, exposed pad to the ground plane with several vias,
  outputs straight to the terminals.
- Encoder lines can be thin (0.25 mm) but keep them off the I2S lines.

## Fab and assembly rules (JLCPCB standard 2-layer + SMT assembly, decision D6)

- Track/space 0.15/0.15 mm minimum (use 0.25 mm signals by default), via 0.3 mm drill / 0.6 mm pad.
- Board edge clearance 0.3 mm for copper.
- Silkscreen: connector pin names, "ESP iRadio", revision (Rev A), date, "+" on the speaker
  connector, "5V" at the USB-C connector.
- Run DRC with the fab's rule set; export Gerbers (F.Cu, B.Cu, F/B.Mask, F/B.Silkscreen,
  Edge.Cuts, F/B.Paste) and the Excellon drill file.
- Assembly: all SMD parts on the top side; BOM with the `LCSC` field and a CPL (position) file in
  JLCPCB's format (columns Designator, Mid X, Mid Y, Layer, Rotation). Check the part rotations in
  JLCPCB's preview before ordering (TSSOP/SOIC pin 1 and the USB-C connector are often rotated).
- Add 3 fiducials (1 mm copper, 2 mm mask opening) if JLCPCB asks for them, and tooling holes or
  rails if the board is panelized.
