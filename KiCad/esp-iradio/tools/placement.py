"""Placement of the integrated board: ref -> (x, y, rotation in degrees), board coordinates in mm with the
origin at the top-left corner of the 80 x 60 mm outline, seen from the front (top side). Every part
is on the top side.

Floor plan (see NOTES.md "Layout Rev B"):
- display panel 37.43 x 30.07 mm on spacers over x 7.0-44.4, y 4.0-34.1; its FPC leaves on the right
  and folds down onto J3; low parts only under the panel (< 4.5 mm);
- WROVER-IE horizontal under the panel, its U.FL end at the left edge, outside the panel;
- encoder top right, microSD at the right edge, jack at the left edge, USB-C, slide switch, cell
  and speaker connectors at the bottom edge; charge LEDs at the bottom edge between the switch
  and the cell connector."""

W, H = 80.0, 60.0
PANEL = (7.0, 4.0, 44.43, 34.07)          # display panel outline (x0, y0, x1, y1), drawn on User.Drawings

P = {
    # mechanics and connectors
    'H1': (3.5, 3.5, 0), 'H2': (76.5, 3.5, 0), 'H3': (3.5, 56.5, 0), 'H4': (76.5, 56.5, 0),
    'U3': (19.6, 17.6, 90),                  # WROVER-IE, U.FL end left (x 0.7-7)
    'J3': (47.0, 19.0, 90),                  # panel FPC pads (pin 1 end: check with the mock-up)
    'SW1': (54.5, 12.5, 0),                  # EC11, shaft at (62, 15)
    'J4': (70.5, 32.0, 90),                  # microSD, card from the right edge
    'J5': (9.4, 47.5, 0),                    # 3.5 mm jack, plug from the left edge
    'J201': (35.5, 55.9, 0),                 # USB-C at the bottom edge (KiCad HRO footprint)
    'SW203': (48.5, 56.62, 0),               # on/off, actuator at the bottom edge
    'J202': (56.0, 56.6, 0),                 # cell (JST-PH)
    'J6': (63.5, 56.6, 0),                   # speaker (JST-PH)
    'D202': (42.6, 56.9, 0), 'D203': (42.6, 58.9, 0),       # charge LEDs (red CHG, green PGOOD)
    'R212': (51.4, 52.6, 0), 'R214': (51.4, 50.4, 0),
    'SW202': (4.1, 30.5, 0), 'SW201': (4.1, 37.2, 0),       # BOOT, RESET at the left edge
    # 3.3 V regulator at the WROVER 3V3 pin (bottom row, x 9.3, y 26.6), under the panel
    'C210': (8.9, 31.2, 90), 'C211': (11.0, 28.6, 0),
    'U206': (14.0, 32.0, 0), 'C209': (18.5, 29.4, 0), 'C208': (18.5, 32.4, 0), 'R221': (17.4, 58.4, 0),
    'TP4': (22.0, 31.6, 0), 'C25': (22.0, 28.8, 0),
    # recovery pads, below the panel
    'TP203': (9.5, 38.0, 0), 'TP204': (12.3, 38.0, 0), 'TP205': (15.1, 38.0, 0), 'TP206': (17.9, 38.0, 0),
    'TP207': (20.7, 38.0, 0),
    # DAC next to the jack
    'U1': (22.4, 46.6, 0),
    'R16': (16.1, 43.4, 90), 'R17': (16.1, 46.6, 90), 'C18': (16.1, 49.8, 90), 'C19': (16.1, 53.0, 90),
    'C15': (27.5, 43.6, 90), 'C16': (27.5, 46.8, 90), 'C17': (27.5, 50.0, 90), 'R15': (27.0, 40.6, 0),
    'C9': (18.4, 52.0, 0), 'C11': (21.4, 52.0, 0), 'C13': (24.4, 52.0, 0),
    'C10': (18.6, 54.3, 0), 'C12': (22.2, 54.3, 0), 'C14': (25.8, 54.3, 0),
    'U207': (21.0, 58.0, 0), 'C212': (25.4, 57.4, 0), 'C213': (13.0, 55.6, 0),
    'TP6': (18.6, 40.8, 0), 'TP7': (21.4, 40.8, 0), 'TP8': (24.2, 40.8, 0),
    # USB: ESD, fuse, TVS, CC, sense divider
    'U202': (33.6, 47.6, 90), 'R201': (30.0, 49.0, 0), 'R202': (30.0, 46.6, 0),
    'F201': (37.4, 47.4, 90), 'TP201': (37.4, 43.6, 0),
    'D201': (45.6, 51.0, 0),
    'R203': (40.4, 46.2, 90), 'R204': (40.4, 42.8, 270), 'C214': (40.4, 39.4, 90),
    # USB serial and auto-reset
    'U203': (47.6, 41.0, 0), 'C204': (41.8, 35.6, 0),
    'Q201': (34.0, 38.6, 0), 'Q202': (34.0, 43.2, 0), 'R205': (37.8, 37.6, 90), 'R206': (37.8, 40.8, 90),
    'R207': (29.6, 36.6, 0), 'C205': (30.0, 40.0, 90), 'R208': (30.0, 43.2, 90),
    # charger
    'U201': (58.2, 45.6, 0), 'C201': (54.0, 41.6, 0), 'C202': (62.6, 42.2, 90), 'C203': (62.6, 46.8, 90),
    'R209': (54.0, 44.4, 0), 'R210': (54.0, 46.4, 0), 'R211': (54.0, 48.4, 0),
    'D204': (53.8, 39.2, 0), 'D205': (57.4, 39.2, 0), 'R213': (55.0, 36.8, 0), 'R215': (58.4, 36.8, 0),
    'TP202': (73.0, 47.8, 0), 'TP2': (63.0, 38.2, 0),
    # cell path: reverse P-FET, NTC, protection, battery divider
    'Q203': (58.5, 51.2, 0), 'R216': (55.0, 51.0, 90), 'RT201': (62.6, 51.6, 90),
    'U204': (67.0, 43.0, 0), 'U205': (67.0, 47.0, 0), 'R219': (71.6, 41.2, 0), 'C206': (70.6, 43.6, 0),
    'R220': (70.6, 45.6, 0),
    'R217': (75.6, 42.6, 90), 'R218': (75.6, 45.8, 90), 'C207': (75.6, 49.0, 90),
    # amplifier next to the speaker connector
    'U2': (70.4, 55.4, 0), 'C22': (67.0, 52.0, 0), 'C23': (70.4, 52.0, 0), 'R21': (73.8, 51.0, 90),
    # display: backlight driver, decoupling, CS pull-up, SPI series resistors (around J3)
    'Q1': (50.8, 22.6, 90), 'R22': (50.8, 18.4, 90), 'R23': (50.8, 26.6, 90),
    'C5': (44.6, 8.6, 0), 'R3': (48.4, 6.0, 0), 'R8': (44.6, 11.0, 0), 'R9': (48.4, 8.4, 0), 'TP5': (52.0, 4.0, 0),
    # encoder RC filters (below the encoder)
    'R10': (54.0, 28.4, 0), 'R11': (54.0, 30.4, 0), 'R12': (54.0, 32.4, 0),
    'C6': (57.6, 28.4, 0), 'C7': (57.6, 30.4, 0), 'C8': (57.6, 32.4, 0),
    # microSD pull-ups and decoupling (left of J4)
    'R4': (61.2, 27.2, 90), 'R5': (63.0, 34.6, 90), 'R6': (61.2, 34.6, 90), 'R7': (63.0, 27.2, 90),
    'C3': (61.2, 31.0, 90), 'C4': (63.2, 31.0, 90),
}
