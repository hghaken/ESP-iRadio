# Grok review, reply to round 6

Checked the HS20S010B datasheet (Hansheng, mechanical drawing 2019-07-20, the JLCPCB file for C5329586) against `tools/placement.py` and `esp-iradio.kicad_pcb`. The board was not regenerated. Coordinates are the placement frame: origin top-left, y downward.

The 90 x 60 mm board matches that drawing. No new route.

## Sockets and holes

The drawing shows the module portrait: PCB 63.2 mm on the long side, 36.2 mm on the short side, CN1 on one short end and CN2 on the other. Turned so the long side lies along X, CN1 is the right-hand row and its pin 1 is the top pin. That is what the board has.

J3 is at (61.7, 7.37), rotation 0. Pin 1 is (61.7, 7.37) on GND, pin 10 is at y 30.23. The drawing puts the end pins 6.67 mm from each side of the 36.2 mm edge (span 22.86 mm). With the module origin at (0.5, 0.7) that is y 7.37 to 30.23. J7's holes cover the same y range at x 2.5, and its pin 1 is the bottom pin, which is where the drawing puts CN2 pin 1.

CN1, drawing table and the silkscreen on that drawing:

| pin | drawing | board |
|---|---|---|
| 1 | GND | GND |
| 2 | VCC | +3V3, C5 10 uF, C25 100 nF |
| 3 | CLK | SPI_SCK after R8, GPIO18 |
| 4 | SDA | SPI_MOSI after R9, GPIO23 |
| 5 | RES | LCD_RST, GPIO21 |
| 6 | DC | LCD_DC, GPIO22 |
| 7 | CS1 | LCD_CS, GPIO5, R3 pull-up |
| 8 | BLK | LCD_BL, GPIO26 |
| 9 | FSO | open |
| 10 | FCS | open |

Holes: drawing Ø3.2 at 3.0 and 60.2 mm on the long side, 2.8 and 33.4 mm on the short side. On the board that is H1 (3.5, 3.5), H5 (60.7, 3.5), H6 (3.5, 34.1), H7 (60.7, 34.1). The board holes are the KiCad M2.5 clearance, drill 2.7 mm. Leave them at 2.7. The 3.2 mm figure is the hole in the module, which an M2.5 screw passes through. Use a round M2.5 standoff about 4 mm across: the hole centre is only 4 mm from J3 pin 1, and a 5 mm hex hits the socket.

Active area on the drawing is 40.86 mm on the long side and 30.60 mm on the short side, so in this orientation the 320-pixel axis is horizontal. The outline in `placement.py` follows that.

One comment in `gen_radio.py` still says the SCK wire goes to J3 pin 9. The wire goes to pin 3. The net is right.

## CN2 is not a second CN1

The drawing's CN2 table is a different order: 1 VCC, 2 GND, 3 BLA, 4 FCS, 5 FSO, 6 CLK, 7 SDA, 8 DC, 9 RES, 10 CS1. Pin 1 sits at the other end from a mirrored CN1. J7 is all open, and the Chinese note on the drawing says FSO and FCS stay open when the font chip is absent, so the board is right. The sentence "CN2 duplicates CN1" should go, so a later edit does not tie the two sockets pin for pin.

## The datasheet disagrees with itself on pins 7-10

The mechanical drawing and the English pin table say pin 7 is CS and pin 8 is BLK. The application sketch on the next page numbers the same 10-pin plug as 7 BLK, 8 MISO, 9 CS1, 10 CS2. The board follows the drawing, including the silkscreen printed on that drawing (GND, VCC, SCL, SDA, RES, DC, CS, LED, FSO, FCS).

Do not respin for the sketch. On the first module, CS (GPIO5) should select the glass and BLK (GPIO26) should switch the backlight. If those two are swapped on the real PCB, exchange pins 7 and 8.

The same sketch shows the backlight transistor (S8050, collector to the LED cathodes, base pulled to VCC). BLK high, or open, is on; low is off. GPIO26 as a PWM logic pin is the right drive, same polarity as the old MOSFET. The LED current, about 80 mA typical and 120 mA max for four LEDs, comes from the module VCC pin, so from +3V3. The 1 A regulator has room. Keep the PWM at 20 kHz or above. C5 at 10 uF is in place of the 4.7 uF on that sketch.

Module thickness on the drawing is 2.5 mm max. Socket 8.5 mm plus the header spacer 2.5 mm puts the module PCB near 11 mm and the front of the glass near 14 mm. Nothing else under the module is near that: the WROVER is about 3 mm, the tact switches a few millimetres more.

## Two ground-land vias are enough

The two 0.4 / 0.2 mm vias that fit are at (21.58, 18.39) and (21.58, 19.79), in the pad-39 gaps, on GND. The board tents vias on both sides. The other two corners are blocked by the RXD0 track on In2.Cu. Leave that track. The land is also tied by the module's GND pads and the F.Cu pour. This pad is not the charger's heat pad.

The fifth BQ24074 via is on the board at (63.2, 46.19), 0.5 / 0.2 mm, 0.59 mm below the pad centre.

## U.FL

U3 at (24.5, 17.6), rotation 90, keeps the antenna end on the left. It sits under the module. The radiating element is the pigtail on the case wall, so the module PCB is not a shield for it. Mate the plug before the module goes on. The module outline starts at x 0.5, so the coax does not leave past the left edge; it runs in the 11 mm gap and comes out below the module (past y 36.9) on its way to the case.

## U2 at 180 degrees

U2 is at (75.4, 55.4), rotation 180. VDD pins 7 and 8 land at about y 54, toward C22 and C23 at y 52. Speaker pads 9 and 10 land at about x 74, toward J6 at x 68.5. The exposed pad still has the footprint's thermal vias. The 180 degree turn is the right one.

## Encoder shaft

No objection to a longer shaft than the short one planned for the bare panel. That plan assumed a panel about 5 mm up. The glass is about 14 mm up, and the case front sits on top of that.

The footprint on the board is still the Alps 20 mm shaft. Keep it until the case thickness is known, then pick the shaft so the knob sits just outside the case. C470754 is a 24.5 mm overall part, not a long shaft; the parts table in `07` should not send that one out as the choice for this height.

## Not this layout

Firmware for this module is still the old 280 x 240 panel: `init` for 240 x 320 (the 320-pixel axis horizontal), backlight PWM at 20 kHz or above, and the rest of the new-board list. Pins on the header match the GPIOs already in `config.h`.
