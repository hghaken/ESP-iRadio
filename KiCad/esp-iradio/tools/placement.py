"""Placement of the integrated board, Rev C (hand assembly, 2026-10-16): ref -> (x, y, rotation in
degrees), board coordinates in mm with the origin at the top-left corner of the 122 x 72 mm outline,
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
  (J201), on/off switch (SW203), cell (J202), speaker (J6); M3 holes in the four corners."""

W, H = 122.0, 72.0
MOD_X, MOD_Y = (W - 63.2) / 2, 1.0             # display module PCB top-left corner on the board
# outlines drawn on User.Drawings: module PCB, glass, active area (datasheet HS20S010B, mechanical drawing)
OUTLINES = [
    ('display module HS20S010B 63.2 x 36.2 (on 8.5 mm sockets)', (MOD_X, MOD_Y, MOD_X + 63.2, MOD_Y + 36.2)),
    ('glass 51.8', (MOD_X + 5.94, MOD_Y, MOD_X + 5.94 + 51.8, MOD_Y + 36.2)),
    ('active area 40.86 x 30.6', (MOD_X + 63.2 - 8.04 - 40.86, MOD_Y + 2.8, MOD_X + 63.2 - 8.04, MOD_Y + 2.8 + 30.6)),
]
TITLE = (17.0, 3.6)              # the title text (silk.py), top of the left strip
ENC_SHAFT = (W / 2, 45.0)        # encoder shaft centre (case: knob hole)

P = {
    # mechanics: H2-H4 and H8 fix the board (M3, corners), H1/H5/H6/H7 carry the display module (M2.5)
    'H1': (MOD_X + 3.0, MOD_Y + 2.8, 0), 'H5': (MOD_X + 60.2, MOD_Y + 2.8, 0),
    'H6': (MOD_X + 3.0, MOD_Y + 33.4, 0), 'H7': (MOD_X + 60.2, MOD_Y + 33.4, 0),
    'H8': (3.5, 3.5, 0), 'H2': (W - 3.5, 3.5, 0), 'H3': (3.5, H - 3.5, 0), 'H4': (W - 3.5, H - 3.5, 0),
    # display module sockets: CN2 pin 1 at the bottom (pins upwards), CN1 pin 1 at the top
    'J7': (MOD_X + 2.0, MOD_Y + 29.53, 180), 'J3': (MOD_X + 61.2, MOD_Y + 6.67, 0),
    'C5': (MOD_X + 56.1, 10.0, 90), 'C25': (MOD_X + 56.1, 15.3, 90), 'R3': (MOD_X + 56.1, 20.6, 90),
    # WROVER-IE under the module, U.FL end left; SPI series resistors above it, 3V3 decoupling at pad 2
    'U3': (53.0, 17.6, 90),
    'R9': (42.8, 5.6, 0), 'R8': (51.7, 5.6, 0), 'TP5': (57.5, 5.6, 0),
    'C211': (43.5, 28.9, 0), 'C210': (40.0, 32.5, 90),
    # left strip: recovery buttons and pads, 3.3 V regulator
    'SW202': (9.0, 12.0, 0), 'SW201': (9.0, 19.0, 0),                     # BOOT, RESET
    'TP203': (4.5, 25.5, 0), 'TP204': (8.0, 25.5, 0), 'TP205': (11.5, 25.5, 0), 'TP206': (15.0, 25.5, 0),
    'TP207': (18.5, 25.5, 0),
    'R221': (16.5, 31.0, 90), 'C208': (19.5, 31.0, 90), 'C209': (22.5, 31.0, 90), 'U206': (26.6, 31.0, 0),
    'TP4': (24.5, 35.8, 0),
    # encoder centred under the display; RC filters left of it
    'SW1': (ENC_SHAFT[0] - 7.5, ENC_SHAFT[1] - 2.5, 0),
    'R10': (49.5, 40.5, 90), 'R11': (49.5, 45.8, 90), 'R12': (49.5, 51.1, 90),
    'C6': (46.5, 40.5, 90), 'C7': (46.5, 45.8, 90), 'C8': (46.5, 51.1, 90),
    # DAC left of the encoder, output filters above the RCA jack, DAC LDO
    'U1': (17.0, 46.6, 0),
    'C11': (11.5, 41.3, 90), 'C12': (8.5, 41.3, 90), 'C15': (11.5, 46.6, 90), 'C16': (8.5, 46.6, 90),
    'C13': (11.5, 51.9, 90), 'C14': (8.5, 51.9, 90),
    'C9': (22.5, 41.3, 90), 'C10': (25.5, 41.3, 90), 'C17': (22.5, 46.6, 90), 'R15': (25.5, 46.6, 90),
    'R16': (8.0, 57.0, 0), 'C18': (13.3, 57.0, 0), 'R17': (18.6, 57.0, 0), 'C19': (23.9, 57.0, 0),
    'U207': (31.5, 46.6, 0), 'C212': (31.5, 41.3, 0), 'C213': (31.5, 51.9, 0),
    'TP6': (36.5, 41.3, 0), 'TP7': (36.5, 44.8, 0), 'TP8': (36.5, 48.3, 0),
    # microSD: decoupling left, pull-ups in a row between the encoder and the socket
    'C3': (40.5, 53.5, 90), 'C4': (43.5, 53.5, 90),
    'R4': (55.0, 54.75, 0), 'R5': (60.2, 54.75, 0), 'R6': (65.4, 54.75, 0), 'R7': (70.6, 54.75, 0),
    # bottom edge connectors
    'J8': (23.7, H - 8.2, 0), 'J5': (42.5, H - 9.5, 90), 'J4': (W / 2, H - 9.6, 0),
    'J201': (75.35, H - 4.1, 0), 'SW203': (85.8, H - 3.38, 0), 'J202': (93.5, H - 3.8, 0), 'J6': (102.5, H - 3.8, 0),
    # USB input, ESD, CC, USB sense, CH340C and auto-reset right of the encoder
    'TP201': (72.5, 49.0, 0), 'D201': (79.5, 52.0, 0), 'F201': (79.0, 56.3, 0), 'U202': (74.5, 59.5, 0),
    'R201': (81.5, 60.0, 90), 'R202': (84.5, 60.0, 90),
    'R203': (88.0, 60.0, 90), 'R204': (91.0, 60.0, 90), 'C214': (94.0, 60.0, 90),
    'U203': (80.0, 44.0, 0), 'C204': (86.5, 40.0, 90),
    'Q201': (91.0, 41.0, 0), 'Q202': (91.0, 46.0, 0), 'R205': (96.0, 41.0, 90), 'R206': (96.0, 46.0, 90),
    'R207': (99.5, 41.0, 90), 'C205': (99.5, 46.0, 90), 'R208': (102.5, 41.0, 90),
    # amplifier next to the speaker connector, charge LEDs at the right edge
    'U2': (104.0, 58.0, 180), 'C22': (100.0, 58.0, 90), 'C23': (108.5, 58.0, 90), 'R21': (111.5, 58.0, 90),
    'D202': (110.5, 65.9, 0), 'D203': (110.5, 69.4, 0), 'R212': (116.0, 59.5, 90), 'R214': (119.0, 59.5, 90),
    # right strip: charger and cell path
    'U201': (104.0, 18.0, 0),
    'R209': (95.5, 13.0, 90), 'R210': (95.5, 18.3, 90), 'R211': (95.5, 23.6, 90),
    'C201': (98.5, 13.0, 90), 'C202': (110.0, 15.0, 90), 'C203': (110.0, 21.0, 90),
    'D204': (115.5, 12.0, 0), 'D205': (115.5, 15.5, 0), 'R213': (115.5, 19.0, 0), 'R215': (115.5, 22.5, 0),
    'RT201': (106.5, 25.5, 90), 'TP2': (119.5, 26.0, 0), 'TP202': (119.5, 29.5, 0),
    'U204': (100.0, 30.0, 0), 'U205': (104.5, 30.0, 0), 'Q203': (110.0, 30.0, 0), 'R216': (114.5, 30.0, 90),
    'R219': (96.0, 34.5, 0), 'C206': (101.5, 34.5, 0), 'R220': (107.0, 34.5, 0), 'R217': (112.5, 34.5, 0),
    'R218': (118.0, 34.5, 0), 'C207': (98.5, 25.0, 90),
}
