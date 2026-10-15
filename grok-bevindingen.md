# Grok review of the first layout

Checked commit `2fe4bb6` (notes at `56d6e6f`). The board file was measured directly. It was not regenerated, and DRC was not run again. Coordinates are the placement frame from `tools/placement.py`: origin at the top-left, y downward.

The floor plan, the layer stack and the power-track class are in good shape. One placement is wrong enough to fix before a case is drawn: the WROVER's U.FL sits under the display glass.

## Fix before the next route: U.FL is under the panel

`lcsc:ESP32-WROVER-IE_SMD` on U3 is at (19.6, 17.6), rotation 90. The module body (the graphics extent, 31.4 x 18 mm) runs from (7.06, 8.18) to (38.47, 26.60).

On that footprint the antenna / U.FL end is local −Y. Rotation 90 maps that end to the right, at x ≈ 38.5, y ≈ 17. The display outline is x 7.0–44.4, y 4.0–34.1, so the connector is about 6 mm inside the glass, mid-height. The note in `NOTES.md` ("U.FL end at the left edge, x 0.7–7, outside the panel") does not match the board.

A pigtail cannot be mated once the panel is on, and the cable has no path out to the case wall. The EasyEDA 3D model draws a PCB antenna on this module; that drawing is not a keep-out. This part is the U.FL module. Do not add the old PCB-antenna keep-out.

Change in `placement.py` only, then a new route (FreeRouting will not reproduce this one):

- Rotation −90° (270°), so local −Y points to the left.
- Shift the module so the U.FL end is left of the panel, with a few millimetres for the plug. The right-hand end can stay under the glass.
- H1 at (3.5, 3.5) stays clear of a connector at mid-height.

That swaps the two long pad rows, so the fan-out around U3 has to be routed again. Nothing else on this list needs a new route.

## Measured, and fine

- 80 x 60 mm, 1.6 mm, 4 copper layers. In1.Cu is a power layer and has no signal tracks. Every via is a through via, F.Cu to B.Cu, so the inner plane is tied in. 478 vias: 467 are 0.55 / 0.3 mm, 379 of those are GND. 11 vias are 0.7 / 0.35 mm, all on +3V3, VSYS, VBAT, BATN or the cell positive, which is a better size for those nets. The stitch count is reasonable; JLCPCB does not charge per via.
- No track endpoint comes within 0.3 mm of the board edge (copper half-width included).
- Power runs are 0.5 mm: VIN 38.7 mm, VSYS 142 mm, VBAT 46.7 mm, +3V3 214 mm, VBUS 17 mm, cell positive and BATN 0.5 mm. The pieces below that are pad escapes, not necks in the run: VIN 0.8 mm at the BQ24074 (59.0, 43.4), VBAT 0.5 mm on the same package, VSYS under 2 mm at U201 and U2, +3V3 1.1 mm at 0.40 mm near (20, 44), speaker pin 2 a 0.7 mm escape at U2. Leave them. The QFN pitch will not take 0.5 mm into the pad.
- Speaker outputs are 0.3 mm and about 11 mm long, from U2 (70.4, 55.4) to J6. +3V3A is 0.3 mm, 28 mm, at the DAC.
- USB D+/D− on the connector side of the USBLC6: 7.2 mm and 6.6 mm. On the CH340C side: 14.9 mm and 12.1 mm. The CH340C is full speed. That skew does not need a 90 Ω pair.
- I2S is the long detour: BCK 97 mm, LRCK 73 mm, DIN 73 mm, against about 30 mm between U3 and U1. Fine at 1.4 MHz. SPI to J3 is about 40 mm. Line-out from U1 to the filters is 3–5 mm. None of these are worth a reroute on their own.
- J201 on KiCad's `USB_C_Receptacle_HRO_TYPE-C-31-M-12` is the right swap. The EasyEDA footprint's 0.1 mm VBUS gap was a real clearance miss. `intcheck.py` now names A4/A9/B4/B9 and S1 separately. Not re-checked with a fresh netlist in this pass.
- "+" / "−" silk is on the board: J202 at (56, 54) / (58, 54), J6 at (63.5, 54) / (65.5, 54), pin 1 positive. That old item is done.
- Parts under the panel are the module, the SOT-89 (U206) and 0603/0805. Nothing there is taller than the module (3.3 mm) under the 5 mm spacers. The encoder body is to the right of x 44.

## Still open, already on Claude's list

- Four thermal vias in the BQ24074 footprint (pad 17). `07` asks for at least five. Add one in the pad; no reroute.
- The nine squares of U3 pad 39 still have no via in the gaps. Four GND stitch vias sit just outside that land, at (21.5, 13.9), (24.0, 13.9), (21.5, 16.5) and (24.0, 16.5). One via in the middle of the 3 x 3 is enough. Also by hand.
- "CHG" at (39.2, 56.9) and "PWR" at (39.2, 58.9) sit on the USB-C body (J201 pads reach x 39.8, y 57.0). "ON/OFF ?" at (48.5, 53.4) is right until a sample says which side is on.
- D203's pads are at y 58.9, so the 0603 body ends about 0.3 mm from the edge. Acceptable if the LED is meant to be seen on the edge; do not push it any closer.
- Unchanged, and not layout defects: J5 pin order on a sample, J4 against a drawing, J3 pin 1 and the contact side, R22 from the panel Vf, short-shaft EC11 (the footprint is still the 20 mm shaft), firmware for this board only.

Do not run `mkpcb.py` for these notes. A new FreeRouting pass is only for the U.FL rotation.
