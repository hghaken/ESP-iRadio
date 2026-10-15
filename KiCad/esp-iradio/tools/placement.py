"""Placement of the integrated board: ref -> (x, y, rotation in degrees), board coordinates in mm with the
origin at the top-left corner of the 90 x 60 mm outline, seen from the front (top side). Every part
is on the top side.

Floor plan (NOTES.md "Layout"):
- display module HS20S010B (63.2 x 36.2 mm PCB, 2.0in glass) top left, plugged into two 1x10 sockets
  (J7 = CN2 left, J3 = CN1 right, 8.5 mm high); its four M2.5 holes sit on H1, H5, H6, H7. Only parts
  lower than ~7 mm under the module (everything except the connectors and the encoder);
- WROVER-IE horizontal under the module, U.FL end left (reachable with the module unplugged);
- encoder right of the module, microSD at the right edge, jack at the left edge, USB-C, charge LEDs,
  slide switch, cell and speaker connectors at the bottom edge."""

W, H = 90.0, 60.0
MOD_X, MOD_Y = 0.5, 0.7                        # display module PCB top-left corner on the board
# outlines drawn on User.Drawings: module PCB, glass, active area (datasheet HS20S010B, mechanical drawing)
OUTLINES = [
    ('display module HS20S010B 63.2 x 36.2 (on 8.5 mm sockets)', (MOD_X, MOD_Y, MOD_X + 63.2, MOD_Y + 36.2)),
    ('glass 51.8', (MOD_X + 5.94, MOD_Y, MOD_X + 5.94 + 51.8, MOD_Y + 36.2)),
    ('active area 40.86 x 30.6', (MOD_X + 63.2 - 8.04 - 40.86, MOD_Y + 2.8, MOD_X + 63.2 - 8.04, MOD_Y + 2.8 + 30.6)),
]

P = {
    # mechanics: H2-H4 fix the board (M3), H1/H5/H6/H7 carry the display module (M2.5)
    'H1': (MOD_X + 3.0, MOD_Y + 2.8, 0), 'H5': (MOD_X + 60.2, MOD_Y + 2.8, 0),
    'H6': (MOD_X + 3.0, MOD_Y + 33.4, 0), 'H7': (MOD_X + 60.2, MOD_Y + 33.4, 0),
    'H2': (86.5, 3.5, 0), 'H3': (3.5, 56.5, 0), 'H4': (86.5, 56.5, 0),
    # display module sockets: CN2 pin 1 at the bottom (pins upwards), CN1 pin 1 at the top
    'J7': (MOD_X + 2.0, MOD_Y + 29.53, 180), 'J3': (MOD_X + 61.2, MOD_Y + 6.67, 0),
    'C5': (58.0, 9.6, 90), 'C25': (58.0, 13.0, 90), 'R3': (58.0, 22.6, 90),
    # WROVER-IE under the module, U.FL end left
    'U3': (24.5, 17.6, 90),
    'R8': (23.2, 5.6, 0), 'R9': (14.3, 5.6, 0), 'TP5': (26.6, 5.6, 0),
    # 3.3 V regulator at the WROVER 3V3 pin (bottom row x 14.3, y 26.6)
    'C210': (15.0, 31.4, 90), 'C211': (15.9, 28.6, 0),
    'U206': (18.9, 32.0, 0), 'C209': (23.4, 29.4, 0), 'C208': (23.4, 32.4, 0), 'R221': (17.4, 58.4, 0),
    'TP4': (26.9, 31.6, 0),
    'SW202': (10.2, 30.4, 0), 'SW201': (10.2, 35.8, 0),        # BOOT, RESET (under the module edge)
    # encoder right of the module, its RC filters below it; microSD at the right edge
    'SW1': (69.0, 12.5, 0),                                   # EC11, shaft at (76.5, 15)
    'R10': (65.4, 31.0, 0), 'R11': (65.4, 33.0, 0), 'R12': (65.4, 35.0, 0),
    'C6': (68.6, 31.0, 0), 'C7': (68.6, 33.0, 0), 'C8': (68.6, 35.0, 0),
    'J4': (80.5, 32.0, 90),                                   # card from the right edge
    'R4': (71.2, 27.2, 90), 'R5': (73.0, 34.6, 90), 'R6': (71.2, 34.6, 90), 'R7': (73.0, 27.2, 90),
    'C3': (71.2, 31.0, 90), 'C4': (73.2, 31.0, 90),
    'TP2': (73.0, 38.4, 0),
    # recovery pads, below the module
    'TP203': (15.2, 38.2, 0), 'TP204': (18.0, 38.2, 0), 'TP205': (20.8, 38.2, 0), 'TP206': (23.6, 38.2, 0),
    'TP207': (26.4, 38.2, 0),
    # jack at the left edge, DAC next to it
    'J5': (9.4, 47.5, 0),
    'U1': (22.4, 46.6, 0),
    'R16': (16.1, 43.4, 90), 'R17': (16.1, 46.6, 90), 'C18': (16.1, 49.8, 90), 'C19': (16.1, 53.0, 90),
    'C15': (27.5, 43.6, 90), 'C16': (27.5, 46.8, 90), 'C17': (27.5, 50.0, 90), 'R15': (27.0, 40.6, 0),
    'C9': (18.4, 52.0, 0), 'C11': (21.4, 52.0, 0), 'C13': (24.4, 52.0, 0),
    'C10': (18.6, 54.3, 0), 'C12': (22.2, 54.3, 0), 'C14': (25.8, 54.3, 0),
    'U207': (21.0, 58.0, 0), 'C212': (25.4, 57.4, 0), 'C213': (13.0, 55.6, 0),
    'TP6': (18.6, 40.9, 0), 'TP7': (21.4, 40.9, 0), 'TP8': (24.2, 40.9, 0),
    # USB-C, ESD, fuse, TVS, CC, sense divider
    'J201': (35.5, 55.9, 0),
    'U202': (33.6, 47.6, 90), 'R201': (30.0, 49.0, 0), 'R202': (30.0, 46.6, 0),
    'F201': (37.4, 47.4, 90), 'TP201': (37.4, 43.6, 0),
    'D201': (45.6, 51.0, 0),
    'R203': (40.4, 46.2, 90), 'R204': (40.4, 42.8, 270), 'C214': (40.4, 39.4, 90),
    # USB serial and auto-reset
    'U203': (47.6, 41.0, 0), 'C204': (41.8, 35.6, 0),
    'Q201': (34.0, 38.6, 0), 'Q202': (34.0, 43.2, 0), 'R205': (37.8, 37.6, 90), 'R206': (37.8, 40.8, 90),
    'R207': (29.6, 36.6, 0), 'C205': (30.0, 40.0, 90), 'R208': (30.0, 43.2, 90),
    # bottom edge: charge LEDs, switch, cell, speaker
    'D202': (42.6, 56.9, 0), 'D203': (42.6, 58.9, 0),
    'SW203': (48.5, 56.62, 0),
    'J202': (61.0, 56.6, 0), 'J6': (68.5, 56.6, 0),
    'R212': (56.4, 52.6, 0), 'R214': (56.4, 50.4, 0),
    # charger
    'U201': (63.2, 45.6, 0), 'C201': (59.0, 41.6, 0), 'C202': (67.6, 42.2, 90), 'C203': (67.6, 46.8, 90),
    'R209': (59.0, 44.4, 0), 'R210': (59.0, 46.4, 0), 'R211': (59.0, 48.4, 0),
    'D204': (58.8, 39.2, 0), 'D205': (62.4, 39.2, 0), 'R213': (54.6, 38.0, 0), 'R215': (54.6, 39.8, 0),
    # cell path: reverse P-FET, NTC, protection, battery divider
    'Q203': (63.5, 51.2, 0), 'R216': (60.0, 51.0, 90), 'RT201': (67.6, 51.6, 90),
    'U204': (72.0, 43.0, 0), 'U205': (72.0, 47.0, 0), 'R219': (76.6, 41.2, 0), 'C206': (75.6, 43.6, 0),
    'R220': (75.6, 45.6, 0),
    'R217': (80.6, 42.6, 90), 'R218': (80.6, 45.8, 90), 'C207': (80.6, 49.0, 90),
    'TP202': (78.0, 47.8, 0),
    # amplifier next to the speaker connector
    'U2': (75.4, 55.4, 180), 'C22': (72.0, 52.0, 0), 'C23': (75.4, 52.0, 0), 'R21': (78.8, 51.0, 90),
}
