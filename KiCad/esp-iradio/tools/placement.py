"""Placement of the integrated board, Rev C (hand assembly, 2026-10-16): ref -> (x, y, rotation in
degrees), board coordinates in mm with the origin at the top-left corner of the 122 x 76 mm outline,
seen from the front (top side). Every part is on the top side.

Floor plan, top to bottom (owner's wish): display, encoder, connectors.
- display module HS20S010B (63.2 x 36.2 mm PCB, 320 px axis horizontal) centred at the top, plugged
  into two 1x10 sockets (J7 = CN2 left, J3 = CN1 right, 8.5 mm high); its four M2.5 holes on H1, H5,
  H6, H7. Under it (~11 mm free): the WROVER-IE (U.FL end left), its decoupling, the SPI series
  resistors, the display decoupling;
- left strip beside the display: 3.3 V regulator, RESET/BOOT buttons, recovery pads;
- right strip beside the display: charger BQ24074 and the cell protection;
- encoder SW1 centred under the display; left of it the DAC with its filters and LDO and the encoder
  RC filters, right of it USB/CH340 with auto-reset, the amplifier and the charge LEDs; the microSD
  pull-ups between the encoder and the card socket;
- bottom edge from left: RCA L/R (J8), 3.5 mm jack (J5), microSD (J4, under the encoder), USB-C
  (J201, its front flush with the edge), speaker (J6), on/off switch (SW203); M3 holes in the four
  corners;
- right edge: the cell connector J202 (the cell sits inside the case), next to the cell protection."""

W, H = 122.0, 76.0               # 76 since 2026-10-16: room between the parts for hand soldering
# top-left corner of the outline in KiCad's page coordinates: the board centred in the A4 drawing
# frame (10 mm margins, frame centre 148.5 / 105; owner, 2026-10-17; was 100 / 100)
OX, OY = 148.5 - W / 2, 105.0 - H / 2
MOD_X, MOD_Y = (W - 63.2) / 2, 1.0             # display module PCB top-left corner on the board
# outlines drawn on User.Drawings: module PCB, glass, active area (datasheet HS20S010B, mechanical drawing)
OUTLINES = [
    ('display module HS20S010B 63.2 x 36.2 (on 8.5 mm sockets)', (MOD_X, MOD_Y, MOD_X + 63.2, MOD_Y + 36.2)),
    ('glass 51.8', (MOD_X + 5.94, MOD_Y, MOD_X + 5.94 + 51.8, MOD_Y + 36.2)),
    ('active area 40.86 x 30.6', (MOD_X + 63.2 - 8.04 - 40.86, MOD_Y + 2.8, MOD_X + 63.2 - 8.04, MOD_Y + 2.8 + 30.6)),
]
TITLE = (17.0, 3.6)              # the title text (silk.py), top of the left strip
ENC_SHAFT = (W / 2, 45.0)        # encoder shaft centre (case: knob hole)

# Spacing (2026-10-16, owner: hand soldering is easier with room): ~1.5 mm between courtyards where
# possible; 1206 columns (rotated) on a 4.0 mm pitch, rows on 6.3 mm.
P = {
    # mechanics: H2-H4 and H8 fix the board (M3, corners), H1/H5/H6/H7 carry the display module (M2.5)
    'H1': (MOD_X + 3.0, MOD_Y + 2.8, 0), 'H5': (MOD_X + 60.2, MOD_Y + 2.8, 0),
    'H6': (MOD_X + 3.0, MOD_Y + 33.4, 0), 'H7': (MOD_X + 60.2, MOD_Y + 33.4, 0),
    'H8': (3.5, 3.5, 0), 'H2': (W - 3.5, 3.5, 0), 'H3': (3.5, H - 3.5, 0), 'H4': (W - 3.5, H - 3.5, 0),
    # display module sockets: CN2 pin 1 at the bottom (pins upwards), CN1 pin 1 at the top
    'J7': (MOD_X + 2.0, MOD_Y + 29.53, 180), 'J3': (MOD_X + 61.2, MOD_Y + 6.67, 0),
    'C5': (MOD_X + 56.1, 10.0, 90), 'C25': (MOD_X + 56.1, 16.3, 90), 'R3': (MOD_X + 56.1, 22.6, 90),
    # WROVER-IE under the module, U.FL end left; SPI series resistors above it, 3V3 decoupling at pad 2
    'U3': (53.0, 17.6, 90),
    'R9': (42.8, 5.6, 0), 'R8': (51.7, 5.6, 0), 'TP5': (57.5, 5.6, 0),
    'C211': (43.5, 28.9, 0), 'C210': (40.0, 33.6, 90),
    # left strip: recovery buttons and pads, 3.3 V regulator
    'SW202': (9.0, 12.0, 0), 'SW201': (9.0, 19.0, 0),                     # BOOT, RESET
    'TP203': (4.0, 25.5, 0), 'TP204': (8.5, 25.5, 0), 'TP205': (13.0, 25.5, 0), 'TP206': (17.5, 25.5, 0),
    'TP207': (22.0, 25.5, 0),
    'TP4': (9.0, 31.8, 0), 'R221': (13.0, 31.8, 90), 'C208': (17.0, 31.8, 90), 'C209': (21.0, 31.8, 90),
    'U206': (25.5, 31.8, 0),
    # encoder centred under the display; RC filters left of it
    'SW1': (ENC_SHAFT[0] - 7.5, ENC_SHAFT[1] - 2.5, 0),
    'R10': (49.5, 40.5, 90), 'R11': (49.5, 46.8, 90), 'R12': (49.5, 53.1, 90),
    'C6': (45.5, 40.5, 90), 'C7': (45.5, 46.8, 90), 'C8': (45.5, 53.1, 90),
    # DAC left of the encoder, output filters above the RCA jack, DAC LDO, I2S test pads
    'U1': (19.0, 46.8, 0),
    'C11': (12.5, 40.5, 90), 'C12': (8.5, 40.5, 90), 'C15': (12.5, 46.8, 90), 'C16': (8.5, 46.8, 90),
    'C13': (12.5, 53.1, 90), 'C14': (8.5, 53.1, 90),
    'C9': (25.5, 40.5, 90), 'C10': (29.5, 40.5, 90), 'C17': (25.5, 46.8, 90), 'R15': (29.5, 46.8, 90),
    'R16': (8.0, 58.8, 0), 'C18': (14.3, 58.8, 0), 'R17': (20.6, 58.8, 0), 'C19': (26.9, 58.8, 0),
    'U207': (35.5, 46.8, 0), 'C212': (35.5, 40.5, 0), 'C213': (35.5, 53.1, 0),
    'TP6': (41.0, 41.0, 0), 'TP7': (41.0, 45.0, 0), 'TP8': (41.0, 49.0, 0),
    # microSD: decoupling left of the socket, pull-ups in a row between the encoder and the socket
    'C3': (54.2, 64.5, 90), 'C4': (54.2, 71.0, 90),
    'R4': (55.0, 56.5, 0), 'R5': (61.3, 56.5, 0), 'R6': (67.6, 56.5, 0), 'R7': (73.9, 56.5, 0),
    # bottom edge connectors, spread over the width (owner, 2026-10-17): courtyards from H3 to H4 with
    # equal gaps of 2.6 mm, pads included (J8 | J5 | C3/C4 + J4 | J201 | J6 | SW203); the switch stays away from the
    # USB-C (a plug covered it, round 7)
    'J8': (25.75, H - 8.2, 0), 'J5': (45.35, H - 9.5, 90), 'J4': (65.35, H - 9.6, 0),
    # USB-C: the receptacle front (F.Fab 3.65 mm below the origin) 0.5 mm past the board edge (owner,
    # flush with the case wall); the front shell-tab pad copper ends 1.30 mm inside the edge (Grok, round 10)
    'J201': (81.75, H - 3.65 + 0.5, 0),
    # J6: 2-pole 5.0 mm screw terminal (owner, 2026-10-17), wire openings at the edge, its front (F.Fab
    # 3.8 mm in front of the pins) flush with the edge; pin 1 (+) on the left
    'J6': (93.2, H - 3.8, 0), 'SW203': (108.15, H - 3.38, 0),
    # cell connector on the right edge (owner: the cell is inside the case), pin 1 (+) on top, beside
    # the protection U204/U205/Q203
    'J202': (119.0, 27.5, 270),
    # USB input, ESD, CC, USB sense, CH340C and auto-reset right of the encoder
    'D201': (81.5, 53.3, 0), 'F201': (80.5, 58.0, 0), 'U202': (79.25, 62.0, 0), 'TP201': (88.0, 57.0, 0),
    'R201': (84.5, 62.5, 90), 'R202': (88.5, 62.5, 90),
    'R203': (92.5, 62.5, 90), 'R204': (96.5, 62.5, 90), 'C214': (100.5, 62.5, 90),
    'U203': (82.0, 45.0, 0), 'C204': (88.5, 41.0, 90),
    'Q201': (94.0, 41.0, 0), 'Q202': (94.0, 47.3, 0), 'R205': (99.0, 41.0, 90), 'R206': (99.0, 47.3, 90),
    'R207': (103.0, 41.0, 90), 'C205': (103.0, 47.3, 90), 'R208': (107.0, 41.0, 90),
    # battery divider and test pads (moved out of the right strip for room)
    'R217': (112.0, 41.0, 90), 'R218': (116.0, 41.0, 90),
    'C207': (49.5, 29.5, 0),     # VBAT_SENSE filter at the ADC pin (U3 pin 7, IO35), round 8
    'TP202': (113.0, 47.3, 0), 'TP2': (117.5, 47.3, 0),
    # amplifier next to the speaker connector, charge LEDs at the right edge
    'U2': (108.0, 61.5, 180), 'C22': (101.7, 56.5, 0), 'C23': (108.0, 56.5, 0), 'R21': (114.3, 56.5, 0),
    'R214': (116.5, 60.0, 0), 'R212': (116.5, 64.0, 0),
    'D202': (104.9, H - 10.0, 0), 'D203': (110.7, H - 10.0, 0),    # side by side above the switch
    # right strip: charger and cell path
    'U201': (106.0, 16.0, 0),
    'R209': (96.0, 11.0, 90), 'R210': (96.0, 17.3, 90), 'R211': (96.0, 23.6, 90),
    'C201': (100.0, 11.0, 90), 'C202': (112.0, 11.0, 90), 'C203': (112.0, 17.3, 90),
    'D204': (118.0, 9.5, 0), 'D205': (118.0, 13.5, 0), 'R213': (118.0, 17.5, 0), 'R215': (118.0, 21.5, 0),
    'RT201': (106.0, 23.0, 90),
    'U204': (97.5, 30.5, 0), 'U205': (103.0, 30.5, 0), 'Q203': (108.5, 30.5, 0), 'R216': (113.0, 30.5, 90),
    'R219': (96.0, 35.5, 0), 'C206': (102.3, 35.5, 0), 'R220': (108.6, 35.5, 0),
}
