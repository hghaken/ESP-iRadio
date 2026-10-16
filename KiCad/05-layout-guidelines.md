# 05 - Layout guidelines

For the integrated board (Rev B, `07-integrated-board.md`). Rewritten 2026-10-15; the carrier
board's version (Olimex outline, PCB-antenna keep-out, 2 layers) is in git history.

## Board and mechanics

- **122 x 76 mm** (Rev C, D15: display, encoder and connectors from top to bottom; 90 x 60 mm in
  Rev B), **4 layers**, 1.6 mm FR-4: layer 2 a solid GND plane, layer 3
  power (VSYS, +3V3) and some signals, top and bottom signals. Stitch GND vias along the edge and
  next to signal vias that change layer.
- **Front (top) side**, from top to bottom (Rev C): the HS20S010B display module centred on two
  1x10 sockets (J7 CN2 left, J3 CN1 right) and four round M2.5 standoffs, ~4 mm across (H1,
  H5-H7), its PCB ~11 mm above the board; the EC11 encoder centred under it; the connectors along
  the bottom edge; the other parts left and right of the encoder and beside the module (D13/D16). Only parts lower than ~10 mm under the module. The case front
  panel lies ~15 mm above the board (module + glass); agree the front with the owner first.
- **Display sockets**: positions and the module holes follow the HS20S010B drawing
  (`tools/placement.py`); the module outline, glass and active area are drawn on User.Drawings.
- **ESP32-WROVER-IE**: no PCB antenna, so no keep-out at a board edge. Keep its **U.FL connector
  reachable** (no tall parts within ~5 mm, room for the plug and the pigtail to the case wall). On
  the 90 x 60 and 122 x 76 boards it lies under the display module: plug the antenna in before
  the module.
  The ground land is the Espressif 3 x 3 array (datasheet figure 15): add the **thermal vias
  between the squares** and do not flood it with one paste opening.
- **Edge connectors** (Rev C: along the bottom edge): RCA L/R (J8), 3.5 mm jack (J5), microSD
  (J4, card from the bottom), USB-C (J201, receptacle front flush with the edge), speaker (J6),
  slide switch (SW203; not next to the USB-C, a plug would cover it). The cell connector J202 sits
  on the right edge (owner, 2026-10-16: the cell is inside the case), beside the cell protection.
- RESET/BOOT buttons and the recovery pads (TP203-TP207, TP4) reachable with the case open.
- The NTC (RT201) next to where the cell lies (it measures the cell temperature).
- 4 board mounting holes (M3, H2-H4, H8) in the corners. 1 mm keep-out around the heads.

## 3D-printed case (decision D3)

The case is designed around the board, so the board files must give the case designer everything:
- A **STEP** of the assembled board (all 3D models; the LCSC models are in `lib/lcsc.3dshapes`,
  STEP files recreated with easyeda2kicad, see NOTES.md) and a **DXF** of Edge.Cuts and holes.
- In `NOTES.md`: board thickness, tallest part per side, display glass position and active area,
  encoder shaft position and knob pocket (D16), openings (USB-C, jack, microSD, switch), the
  speaker (D4, mounted in the case wall, wired to J6), the U.FL pigtail and antenna position.
- Antenna (external, on the pigtail) at a plastic wall, away from the speaker magnet and the cell.
- Room for the Li-Po (5000 mAh pouch, roughly 60 x 50 x 8 mm: measure the owner's cell) and its
  cable to J202.

## Routing

- **USB**: D+/D- as a 90 Ohm pair (JLCPCB impedance calculator for the 4-layer stack-up), from
  J201 through the USBLC6-2SC6 pads (pins 1/6 and 3/4) to the CH340C, short, no stubs; the two
  D+ and the two D- pins of the connector joined right at the connector.
- **Power input**: J201 VBUS -> F201 -> D201 -> C201 -> BQ24074 IN, wide (>= 1 mm) and short; D201
  and C201 close to the charger.
- **BQ24074 (VQFN-16)**: IN, OUT and BAT capacitors right at the pins; the thermal pad with
  **at least 5 vias** into the GND plane and copper around it on top and bottom (up to ~1.3 W while
  charging); ISET/ILIM/TMR resistors close to their pins, away from switching nodes. VSYS, VBAT and
  the cell path (J202 - Q203 - charger; BATN - FS8205A - GND) >= 1 mm.
- **Cell protection**: DW01A and FS8205A next to J202; the FS8205A carries the full cell current
  (short, wide BATN and GND); C206 at the DW01A VCC pin.
- **3.3 V**: AP7361C (SOT-89-5) with copper on its tab; C209 at its output, **C210 22 uF + C211
  100 nF at the WROVER 3V3 pin**; LP5907 (+3V3A) next to the PCM5102A, its output only to AVDD.
- **SPI** (`SPI_SCK`, `SPI_MOSI`, `SPI_MISO`, CS lines): the display runs at 26.7 MHz. Short
  (< 80 mm), over unbroken ground, no stubs; R8 (22 Ohm) and R9 (0 Ohm; 22-33 Ohm if ringing
  shows) at the ESP32 end.
- **I2S** (BCK 1.4-2.8 MHz, LRCK, DATA): short, over ground, away from the analog outputs and the
  speaker traces; to both chips (PCM5102A and MAX98357A) as a short daisy chain or a short T.
- **Analog line out**: short, guarded by ground, away from the amplifier outputs and the SPI clock.
- **Amplifier (MAX98357A, TQFN-16, ThermalVias footprint)**: VSYS and the speaker outputs >= 0.8 mm
  (1 mm preferred), C22 10 uF + C23 100 nF right at the VDD pins, exposed pad to the GND plane,
  outputs straight to J6. Never tie an output to ground.
- Encoder lines can be thin (0.25 mm); keep them off the I2S lines.

## Fab and assembly rules (JLCPCB 4-layer + SMT assembly, D6/D17)

- Track/space 0.15/0.15 mm minimum (0.25 mm signals by default), vias 0.3 mm drill / 0.5-0.55 mm pad; **no
  hole below 0.3 mm** (smaller holes are JLCPCB's high-precision option, ~$37 extra per order).
  The finest pitches on the board are 0.5 mm (USB-C, both QFNs).
- Copper 0.3 mm from the board edge.
- Locating holes of the LCSC footprints are NPTH (fixed in `lib/lcsc.pretty` 2026-10-15); check
  any new EasyEDA footprint for plated holes with drill = pad.
- Silkscreen: "ESP iRadio", revision (Rev B), date; **"+" and "-" at J202 (cell) and J6
  (speaker)** - Li-Po lead colours are not the same between makers; "5V" at USB-C; ON/OFF at the
  slide switch once a sample shows which position is on; names at the recovery pads.
- Run DRC with the JLCPCB 4-layer rule set; export Gerbers and the Excellon drill file.
- Assembly (Rev C, D17): by hand with solder paste, a stencil (paste layer F.Paste, from JLCPCB)
  and a heat gun, then the through-hole parts with an iron; all SMD on the top side, R/C in 1206.
  Parts list `jlcpcb/esp-iradio_lcsc_order.csv`, assembly drawing `jlcpcb/esp-iradio_assembly.pdf`.
  The BOM/CPL for JLCPCB assembly are still generated (and their rotations checked) if that route
  is ever taken again.
- Not on the board: the display module (plugged in), the cell, the speaker, the antenna pigtail.
- 3 fiducials (1 mm copper, 2 mm mask opening) if JLCPCB asks for them; tooling rails if panelized.
