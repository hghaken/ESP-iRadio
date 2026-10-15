"""ESP iRadio integrated board, sheet 2: USB-C, USB serial (CH340C) with auto-reset, BQ24074 power-path
charger, cell protection, 3.3 V and 3.3 V analog regulators with the on/off switch. Elektuur style.
Connects to the radio sheet through global labels and the global power nets (VSYS, +3V3, +3V3A, GND)."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from schgen import Libs, Sheet

PRJ = sys.argv[1]
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(PRJ, 'power_usb.kicad_sch')
ROOT_UUID = '41c4da12-e59e-4675-bbd9-2603c7cfe6e6'
POWER_SHEET_UUID = 'b7d1c2e4-5a3f-4e2b-9c11-2f6a8d0e4b21'
FILE_UUID = '6c0e9a51-3d2b-4f7a-8e19-5b4d2c7a9f30'
libs = Libs({'Elektuur': os.path.join(PRJ, 'lib', 'Elektuur.kicad_sym'),
             'esp-iradio': os.path.join(PRJ, 'lib', 'esp-iradio.kicad_sym'),
             'lcsc': os.path.join(PRJ, 'lib', 'lcsc.kicad_sym')})
s = Sheet(libs, ROOT_UUID, 'esp-iradio', 'A3', sheet_uuid=POWER_SHEET_UUID, uuid=FILE_UUID, pwr_start=200)
W = s.wire
P = s.P

FR = 'Resistor_SMD:R_0603_1608Metric'
FC6 = 'Capacitor_SMD:C_0603_1608Metric'
FC8 = 'Capacitor_SMD:C_0805_2012Metric'
FTP = 'TestPoint:TestPoint_Pad_D1.5mm'
SOT23 = 'Package_TO_SOT_SMD:SOT-23'


def horiz(x, y):
    return {'ref': (x, y - 3.81, 90, None), 'val': (x, y + 3.81, 90, None)}


def lside(x, y):
    return {'ref': (x - 3.81, y - 1.27, 0, 'right'), 'val': (x - 3.81, y + 1.27, 0, 'right')}


def R(ref, x, y, val, r=0, fp=FR, fields=None, lcsc=None):
    s.add('Elektuur:R', ref, x, y, r, val, fp, fields=fields or (horiz(x, y) if r % 180 else None),
          extra={'LCSC': lcsc} if lcsc else None)


def C(ref, x, y, val, r=0, fp=FC6, fields=None):
    s.add('Elektuur:C', ref, x, y, r, val, fp, fields=fields or (horiz(x, y) if r % 180 else None))


def up(net, at, show=True):
    return s.add_power('Elektuur:V-+', net, at, 180, 1, (at[0], at[1] - 6.35, 0, None), show)


def down(net, at, show=True):
    return s.add_power('Elektuur:V-+', net, at, 0, 1, (at[0], at[1] + 6.35, 0, None), show)


def left(net, at, show=True):
    return s.add_power('Elektuur:V-+', net, at, 0, 2, (at[0] - 4.83, at[1], 0, 'right'), show)


def right(net, at, show=True):
    return s.add_power('Elektuur:V-+', net, at, 180, 2, (at[0] + 6.35, at[1], 0, 'left'), show)


def gnd(at, d='down'):
    r = {'down': 0, 'up': 180, 'left': 270, 'right': 90}[d]
    return s.add_power('Elektuur:V-GND', 'GND', at, r, 1, (at[0], at[1], 0, None), False)


def flag(ref, at, r=0):
    s.add('power:PWR_FLAG', ref, at[0], at[1], r, 'PWR_FLAG', hide_ref=True, hide_value=True)


def tp(ref, at, val, side='right'):
    s.add('Connector:TestPoint', ref, at[0], at[1], 0, val, FTP, hide_value=True,
          fields={'ref': (at[0] - 1.27, at[1] - 3.81, 0, 'right') if side == 'left' else (at[0] + 1.27, at[1] - 3.81, 0, 'left')})


# ================================================================ USB-C input (top left)
s.text('USB-C INPUT: 5 V, no PD; ESD and surge protection', (20.32, 30.48), 1.8, True)
JX, JY = 35.56, 63.5
s.add('lcsc:TYPE-C-31-M-12', 'J201', JX, JY, 0, 'USB-C TYPE-C-31-M-12', 'lcsc:USB-C_SMD-TYPE-C-31-M-12_1',
      mirror='y', extra={'LCSC': 'C165948'}, fields={'ref': (JX, JY - 19.05, 0, None), 'val': (JX, JY - 16.51, 0, None)})
# shield tabs (left after the mirror) to GND
EH = [P('J201', n) for n in ('1', '2', '3', '4')]
for p in EH:
    W(p, (21.59, p[1]))
W((21.59, EH[3][1]), (21.59, EH[2][1]), (21.59, EH[1][1]), (21.59, EH[0][1]))
gnd((21.59, EH[0][1]))
flag('#FLG204', (21.59, EH[2][1]), 90)
gnd_pins = [P('J201', 'A1B12'), P('J201', 'B1A12')]
for p in gnd_pins:
    W(p, (p[0] + 2.54, p[1]))
    gnd((p[0] + 2.54, p[1]), 'right')
for pn in ('A4B9', 'B4A9'):
    p = P('J201', pn)
    W(p, (p[0] + 5.08, p[1]))
    s.label('VBUS', (p[0] + 5.08, p[1]))
for pn in ('A8', 'B8'):
    s.nc(P('J201', pn))
for pn, net in (('A6', 'USB_D+'), ('B6', 'USB_D+'), ('A7', 'USB_D-'), ('B7', 'USB_D-')):
    p = P('J201', pn)
    W(p, (p[0] + 5.08, p[1]))
    s.label(net, (p[0] + 5.08, p[1]))
# CC pull-downs: one 5.1k per pin
for pn, ref in (('A5', 'R201'), ('B5', 'R202')):
    p = P('J201', pn)
    R(ref, p[0] + 20.32, p[1], '5.1k', 90)
    W(p, P(ref, 1))
    gnd(P(ref, 2), 'right')

# VBUS -> polyfuse -> TVS -> VIN rail (y 40.64): fuse first, so a wrong adapter opens the fuse
# instead of burning the TVS
VR = 40.64
s.label('VBUS', (66.04, VR))
s.add('Elektuur:F', 'F201', 73.66, VR, 90, '2A hold 16V', 'Fuse:Fuse_1206_3216Metric', extra={'LCSC': 'C22374899'},
      fields=horiz(73.66, VR))
W((66.04, VR), P('F201', 1))
flag('#FLG201', (66.04, VR), 0)
s.add('Elektuur:D-Zener', 'D201', 83.82, 48.26, 180, 'SMAJ5.0A', 'Diode_SMD:D_SMA', extra={'LCSC': 'C113952'},
      fields={'ref': (80.01, 46.99, 0, 'left'), 'val': (80.01, 49.53, 0, 'left')})   # 180 deg: 'left' shows right-aligned
W(P('F201', 2), (83.82, VR), P('D201', 1))
gnd(P('D201', 2))
# USB voltage sense: 330k/220k (x0.4) + 100 nF -> GPIO39; high-ohmic, so an unpowered ESP32 (switch
# off, USB in) gets only ~10 uA through the pin's protection diode
R('R203', 91.44, 48.26, '330k')
R('R204', 91.44, 60.96, '220k', fields=lside(91.44, 60.96))
W((83.82, VR), (91.44, VR), P('R203', 1))
W(P('R203', 2), P('R204', 1))
W(P('R203', 2), (99.06, P('R203', 2)[1]), (104.14, P('R203', 2)[1]))
C('C214', 99.06, P('R203', 2)[1] + 5.08, '100nF',
  fields={'ref': (99.06, P('R203', 2)[1] + 13.97, 0, None), 'val': (99.06, P('R203', 2)[1] + 16.51, 0, None)})
gnd(P('C214', 2))
s.glabel('VBUS_SENSE', (104.14, P('R203', 2)[1]), 0)
gnd(P('R204', 2))

# USB ESD: USBLC6-2SC6
UX, UY = 60.96, 99.06
s.add('Power_Protection:USBLC6-2SC6', 'U202', UX, UY, 0, 'USBLC6-2SC6', 'Package_TO_SOT_SMD:SOT-23-6',
      extra={'LCSC': 'C2827654'}, fields={'ref': (UX + 5.08, UY - 8.89, 0, 'left'), 'val': (UX + 5.08, UY - 6.35, 0, 'left')})
W(P('U202', '1'), (53.34, P('U202', '1')[1]))
s.label('USB_D+', (53.34, P('U202', '1')[1]), 180)
W(P('U202', '3'), (53.34, P('U202', '3')[1]))
s.label('USB_D-', (53.34, P('U202', '3')[1]), 180)
W(P('U202', '5'), (UX, UY - 7.62))
s.label('VIN', (UX, UY - 7.62), 90)          # its VBUS clamp behind the fuse too
gnd(P('U202', '2'))

# ================================================================ USB serial CH340C + auto-reset (bottom left)
s.text('USB SERIAL: CH340C, auto-reset (EN, GPIO0), RESET and BOOT buttons', (20.32, 185.42), 1.8, True)
CX, CY = 88.9, 134.62
s.add('esp-iradio:CH340C_3V3', 'U203', CX, CY, 0, 'CH340C', 'Package_SO:SOIC-16_3.9x9.9mm_P1.27mm',
      extra={'LCSC': 'C84681'}, fields={'ref': (CX - 5.08, CY + 20.32, 0, 'right'), 'val': (CX - 5.08, CY + 22.86, 0, 'right')})
# D+/D- from the ESD chip (its right pins) to the CH340C
dp, dm = P('U202', '6'), P('U202', '4')
W(dp, (71.12, dp[1]), (71.12, P('U203', 'UD+')[1]), P('U203', 'UD+'))
W(dm, (68.58, dm[1]), (68.58, P('U203', 'UD-')[1]), P('U203', 'UD-'))
s.nc(P('U203', 'R232'))
for pn in ('7', '8'):
    s.nc(P('U203', pn))
for pn in ('~{CTS}', '~{DSR}', '~{RI}', '~{DCD}'):
    s.nc(P('U203', pn))
vcc, v3 = P('U203', 'VCC'), P('U203', 'V3')
RY = vcc[1] - 2.54
W(vcc, (vcc[0], RY))
W(v3, (v3[0], RY))
W((vcc[0], RY), (v3[0], RY), (96.52, RY))
up('+3V3', (vcc[0], RY))
C('C204', 96.52, RY - 5.08, '100nF')
gnd(P('C204', 1), 'up')
gnd(P('U203', 'GND'))
txd, rxd = P('U203', 'TXD'), P('U203', 'RXD')
W(txd, (109.22, txd[1]))
s.glabel('RXD0', (109.22, txd[1]), 0)      # CH340 TXD -> ESP32 RXD0 (GPIO3)
W(rxd, (109.22, rxd[1]))
s.glabel('TXD0', (109.22, rxd[1]), 0)      # ESP32 TXD0 (GPIO1) -> CH340 RXD
dtr, rts = P('U203', '~{DTR}'), P('U203', '~{RTS}')
# Q201: base <- DTR (10k), emitter -> RTS, collector -> EN
# Q202: base <- RTS (10k), emitter -> DTR, collector -> GPIO0
QX = 127.0
QY1, QY2 = dtr[1] - 5.08, rts[1] + 17.78
s.add('esp-iradio:T-BJT-NPN_SOT23', 'Q201', QX, QY1, 0, 'MMBT3904', SOT23, extra={'LCSC': 'C7420353'},
      fields={'ref': (QX + 3.81, QY1 - 1.27, 0, 'left'), 'val': (QX + 3.81, QY1 + 1.27, 0, 'left')})
s.add('esp-iradio:T-BJT-NPN_SOT23', 'Q202', QX, QY2, 0, 'MMBT3904', SOT23, extra={'LCSC': 'C7420353'},
      fields={'ref': (QX + 3.81, QY2 - 1.27, 0, 'left'), 'val': (QX + 3.81, QY2 + 1.27, 0, 'left')})
R('R205', 114.3, QY1, '10k', 90)
R('R206', 114.3, QY2, '10k', 90)
W(P('R205', 2), P('Q201', 'B'))
W(P('R206', 2), P('Q202', 'B'))
W(dtr, (104.14, dtr[1]), (104.14, QY1), P('R205', 1))
W((104.14, dtr[1]), (104.14, QY2 + 10.16), (P('Q202', 'E')[0], QY2 + 10.16), P('Q202', 'E'))
W(rts, (106.68, rts[1]), (106.68, QY2), P('R206', 1))
W((106.68, rts[1]), (P('Q201', 'E')[0], rts[1]), P('Q201', 'E'))
# EN: 10k up, 1 uF, RESET button
ENY = P('Q201', 'C')[1] - 5.08
W(P('Q201', 'C'), (P('Q201', 'C')[0], ENY), (162.56, ENY))
R('R207', 139.7, ENY - 5.08, '10k')
up('+3V3', P('R207', 1), False)
C('C205', 147.32, ENY + 5.08, '1uF', fields=lside(147.32, ENY + 5.08))
gnd(P('C205', 2))
s.add('Elektuur:S-make-push', 'SW201', 154.94, ENY + 5.08, 0, 'RESET', 'esp-iradio:SW_TS-1187A',
      extra={'LCSC': 'C318884'})
gnd(P('SW201', '3'))
s.glabel('ESP_EN', (162.56, ENY), 0)
# GPIO0: 10k up, BOOT button
IOY = P('Q202', 'C')[1] - 5.08
W(P('Q202', 'C'), (P('Q202', 'C')[0], IOY), (162.56, IOY))
R('R208', 139.7, IOY - 5.08, '10k')
up('+3V3', P('R208', 1), False)
s.add('Elektuur:S-make-push', 'SW202', 154.94, IOY + 5.08, 0, 'BOOT', 'esp-iradio:SW_TS-1187A',
      extra={'LCSC': 'C318884'})
gnd(P('SW202', '3'))
s.glabel('ESP_IO0', (162.56, IOY), 0)

# recovery pads: flash the ESP32 with a serial adapter when USB-C or the CH340C is dead (switch on;
# +3V3 is TP4 on sheet 1)
s.text('RECOVERY PADS: flash the ESP32 without USB-C / CH340C (switch on, +3V3 = TP4)', (20.32, 205.74), 1.8, True)
for i, (ref, net) in enumerate((('TP203', 'ESP_EN'), ('TP204', 'ESP_IO0'), ('TP205', 'TXD0'), ('TP206', 'RXD0'))):
    at = (30.48 + 20.32 * i, 220.98)
    tp(ref, at, net)
    s.glabel(net, at, 270)
tp('TP207', (111.76, 220.98), 'GND')
gnd((111.76, 220.98))

# ================================================================ charger BQ24074 (top middle)
s.text('CHARGER: BQ24074 power path (OUT = VSYS), 0.8 A charge, 1.37 A input', (119.38, 30.48), 1.8, True)
BX, BY = 175.26, 63.5
s.add('Battery_Management:BQ24074RGT', 'U201', BX, BY, 0, 'BQ24074RGTR',
      'Package_DFN_QFN:VQFN-16-1EP_3x3mm_P0.5mm_EP1.68x1.68mm_ThermalVias', extra={'LCSC': 'C54313'},
      fields={'ref': (BX - 2.54, BY - 19.05, 0, 'right'), 'val': (BX - 2.54, BY - 16.51, 0, 'right')})
# IN from the VIN rail, 4.7 uF
W((91.44, VR), (BX, VR), P('U201', 'IN'))
s.label('VIN', (116.84, VR))                  # fused rail: D201, U202 VBUS clamp, R203, BQ24074 IN
C('C201', 152.4, VR + 5.08, '4.7uF 25V', fp=FC8, fields=lside(152.4, VR + 5.08))
gnd(P('C201', 2))
flag('#FLG203', (BX, VR), 0)
tp('TP201', (101.6, VR), 'VIN')
s.nc(P('U201', 'ITERM'))         # open = termination at 10 % (BQ24074 has no SYSOFF pin)
# programming pins fan out to the left, each its own column down to y 88.9
COL = {'~{CE}': 106.68, 'TMR': 116.84, 'EN1': 127.0, 'EN2': 137.16, 'ILIM': 147.32, 'ISET': 160.02}
BOT = 88.9
for pn, x in COL.items():
    p = P('U201', pn)
    W(p, (x, p[1]), (x, BOT))
gnd((COL['~{CE}'], BOT))                     # CE low = charging enabled
gnd((COL['EN1'], BOT))                       # EN1 low, EN2 high = ILIM mode
s.add_power('Elektuur:V-+', 'VSYS', (COL['EN2'], BOT), 0, 1, (COL['EN2'] - 2.54, BOT + 5.08, 0, 'right'), True)
R('R209', COL['TMR'], BOT + 5.08, '68k')
R('R210', COL['ILIM'], BOT + 5.08, '1.13k')
R('R211', COL['ISET'], BOT + 5.08, '1.1k', fields={'ref': (COL['ISET'] + 3.81, BOT + 1.27, 0, 'left'),
                                                  'val': (COL['ISET'] + 3.81, BOT + 3.81, 0, 'left')})
for r_ in ('R209', 'R210', 'R211'):
    gnd(P(r_, 2))
s.text('TMR 68k: safety timer ~9 h', (106.68, 106.68), 1.27, False)
s.text('ILIM 1.13k: input 1.37 A, ISET 1.1k: charge 0.8 A', (106.68, 109.22), 1.27, False)
gnd(P('U201', 'VSS'))
# OUT -> VSYS
o = P('U201', 'OUT')
W(o, (190.5, o[1]), (190.5, 35.56), (205.74, 35.56))
right('VSYS', (205.74, 35.56))
C('C202', 198.12, 40.64, '10uF', fp=FC8)
W((198.12, 35.56), P('C202', 1))
gnd(P('C202', 2))
# status outputs: stubs with labels
for pn, net in (('~{PGOOD}', 'PGOOD_N'), ('~{CHG}', 'CHG_N')):
    p = P('U201', pn)
    W(p, (193.04, p[1]))
    s.label(net, (193.04, p[1]))
# TS: NTC on the board next to the cell
ts = P('U201', 'TS')
s.add('Elektuur:R-nonlin', 'RT201', 213.36, ts[1] + 5.08, 0, 'NTC 10k', FR, extra={'LCSC': 'C13564'},
      fields={'ref': (215.9, ts[1] + 3.81, 0, 'left'), 'val': (215.9, ts[1] + 6.35, 0, 'left')})
W(ts, (213.36, ts[1]), P('RT201', 1))
gnd(P('RT201', 2))

# status LEDs + GPIO isolation (BAT54WS)
for i, (nm, led, col, rl, d, rp, glab, ly) in enumerate((
        ('CHG_N', 'D202', 'red', 'R212', 'D204', 'R213', 'CHG', 101.6),
        ('PGOOD_N', 'D203', 'green', 'R214', 'D205', 'R215', 'PGOOD', 116.84))):
    R(rl, 175.26, ly, '1k', 90)
    up('VSYS', (170.18, ly), i == 0)
    W((170.18, ly), P(rl, 1))
    s.add('Elektuur:D-LED', led, 185.42, ly, 90, 'LED ' + col, 'LED_SMD:LED_0603_1608Metric',
          extra={'LCSC': 'C2286' if col == 'red' else 'C12624'}, fields=horiz(185.42, ly))
    W(P(rl, 2), P(led, 2))
    s.add('Elektuur:D-Schottky', d, 203.2, ly, 270, 'BAT54WS', 'Diode_SMD:D_SOD-323', extra={'LCSC': 'C7502694'},
          fields=horiz(203.2, ly))
    W(P(led, 1), P(d, 1))
    s.label(nm, (193.04, ly))
    W(P(d, 2), (220.98, ly))
    R(rp, 213.36, ly - 5.08, '100k')
    up('+3V3', P(rp, 1), False)
    s.glabel(glab, (220.98, ly), 0)
s.text('LEDs on VSYS: they light also with the radio off', (167.64, 123.19), 1.27, False)

# ================================================================ cell: reverse P-FET, protection (top right)
s.text('CELL: 1S Li-Po, reverse-polarity P-FET, DW01A + FS8205A protection', (243.84, 30.48), 1.8, True)
b = P('U201', 'BAT')
QPX = 248.92
s.add('esp-iradio:T-IGFET-P_SOT23', 'Q203', QPX, b[1] + 5.08, 90, 'DMG2305UX-7', SOT23, extra={'LCSC': 'C5261054'},
      fields={'ref': (QPX, b[1] - 5.08, 90, None), 'val': (QPX, b[1] - 2.54, 90, None)})
W(b, P('Q203', 'S'))
s.label('VBAT', (200.66, b[1]))
R('R216', P('Q203', 'G')[0], P('Q203', 'G')[1] + 5.08, '100k', fields=lside(P('Q203', 'G')[0], P('Q203', 'G')[1] + 5.08))
W(P('R216', 1), P('Q203', 'G'))
gnd(P('R216', 2))
C('C203', 223.52, b[1] + 5.08, '10uF', fp=FC8)
gnd(P('C203', 2))
# battery voltage sense: 470k/470k + 100 nF -> GPIO35 (drawn apart, on the VBAT label)
VX = 340.36
R('R217', VX, 60.96, '470k', fields=lside(VX, 60.96))
R('R218', VX, 73.66, '470k', fields=lside(VX, 73.66))
W(P('R217', 1), (VX, 50.8))
s.label('VBAT', (VX, 50.8), 90)
W(P('R217', 2), P('R218', 1))
tap = P('R218', 1)
C('C207', VX + 7.62, 73.66, '100nF')
W(tap, P('C207', 1))
gnd(P('R218', 2))
gnd(P('C207', 2))
W(P('C207', 1), (VX + 15.24, tap[1]))
s.glabel('VBAT_SENSE', (VX + 15.24, tap[1]), 0)
s.text('battery voltage -> GPIO35', (VX - 7.62, 91.44), 1.27, False)
tp('TP202', (226.06, 50.8), 'VBAT')
W((226.06, 50.8), (226.06, b[1]))
# JST-PH cell connector
JCX, JCY = 289.56, b[1]
s.add('Connector_Generic:Conn_01x02', 'J202', JCX, JCY, 0, 'Li-Po cell (JST-PH)',
      'Connector_JST:JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical', extra={'LCSC': 'C131337'},
      fields={'ref': (JCX + 3.81, JCY - 1.27, 0, 'left'), 'val': (JCX + 3.81, JCY + 1.27, 0, 'left')})
W(P('Q203', 'D'), P('J202', 1))
W(P('J202', 2), (281.94, P('J202', 2)[1]), (281.94, P('J202', 2)[1] + 5.08))
s.label('BATN', (281.94, P('J202', 2)[1] + 5.08), 270)
flag('#FLG206', (281.94, P('J202', 2)[1] + 2.54), 90)
s.text('cell +', (JCX + 3.81, JCY - 3.81), 1.27, False)
# DW01A + FS8205A in the negative lead
DX, DY = 271.78, 93.98
s.add('Battery_Management:DW01A', 'U204', DX, DY, 0, 'DW01A', 'Package_TO_SOT_SMD:SOT-23-6', extra={'LCSC': 'C351410'},
      fields={'ref': (DX, DY - 7.62, 0, None), 'val': (DX, DY - 5.08, 0, None)})
vcc = P('U204', 'VCC')
R('R219', 261.62, b[1] + 15.24, '100R')
W(P('R219', 1), (261.62, b[1]))
W(P('R219', 2), vcc)
flag('#FLG205', (vcc[0], vcc[1] - 5.08), 90)
C('C206', 254.0, vcc[1] + 5.08, '100nF', fields=lside(254.0, vcc[1] + 5.08))
W(vcc, P('C206', 1))
gd = P('U204', 'GND')
W(gd, (259.08, gd[1]), (259.08, 106.68))
W(P('C206', 2), (254.0, 106.68), (259.08, 106.68))
s.label('BATN', (254.0, 106.68), 180)
s.nc(P('U204', 'TD'))
FX, FY = 271.78, 124.46
s.add('lcsc:FS8205A_C2830320', 'U205', FX, FY, 0, 'FS8205A', 'lcsc:SOT-23-6_L2.9-W1.6-P0.95-LS2.8-BR',
      extra={'LCSC': 'C2830320'}, fields={'ref': (FX, FY - 7.62, 0, None), 'val': (FX, FY - 5.08, 0, None)})
s1, s2 = P('U205', '1'), P('U205', '3')
W((259.08, 106.68), s1)
W(s2, (254.0, s2[1]))
gnd((254.0, s2[1]), 'left')
s.nc(P('U205', '2'))
s.nc(P('U205', '5'))
od, oc = P('U204', 'OD'), P('U204', 'OC')
g1, g2 = P('U205', '6'), P('U205', '4')
W(od, (od[0], 109.22), (289.56, 109.22), (289.56, g1[1]), g1)
W(oc, (oc[0], 106.68), (292.1, 106.68), (292.1, g2[1]), g2)
cs = P('U204', 'CS')
R('R220', 292.1, cs[1], '1k', 90)
W(cs, P('R220', 1))
gnd(P('R220', 2), 'right')
s.text('BATN = cell -, GND = board ground (P-)', (243.84, 137.16), 1.27, False)

# ================================================================ regulators + on/off switch (bottom right)
s.text('3.3 V: AP7361C (EN = on/off switch) and LP5907 for the DAC (+3V3A)', (190.5, 154.94), 1.8, True)
AX, AY = 251.46, 180.34          # AP7361C: pins at x +-11.43
s.add('lcsc:AP7361C-33Y5-13', 'U206', AX + 1.27, AY, 0, 'AP7361C-33Y5-13', 'lcsc:SOT-89-5_L4.5-W2.5-P1.50-LS4.5-BR',
      extra={'LCSC': 'C460397'}, fields={'ref': (AX + 1.27, AY - 11.43, 0, None), 'val': (AX + 1.27, AY - 8.89, 0, None)})
en, ag, adj = P('U206', 'EN'), P('U206', 'GND'), P('U206', 'ADJ/NC')
ain, aout = P('U206', 'IN'), P('U206', 'OUT')
s.nc(adj)
W(ag, (ag[0] - 2.54, ag[1]))
gnd((ag[0] - 2.54, ag[1]), 'left')
# IN from VSYS, 10 uF
W(ain, (279.4, ain[1]), (284.48, ain[1]))
right('VSYS', (284.48, ain[1]))
C('C208', 279.4, ain[1] + 5.08, '10uF', fp=FC8)
gnd(P('C208', 2))
# OUT -> +3V3, 10 uF at the LDO, 22 uF + 100 nF at the WROVER 3V3 pin
W(aout, (266.7, aout[1]), (266.7, 167.64), (320.04, 167.64))
up('+3V3', (320.04, 167.64))
for i, (ref, val, fp) in enumerate((('C209', '10uF', FC8), ('C210', '22uF', FC8), ('C211', '100nF', FC6))):
    x = 281.94 + 10.16 * i
    C(ref, x, 172.72, val, fp=fp)
    gnd(P(ref, 2))
s.text('C210, C211 at the WROVER 3V3 pin', (287.02, 193.04), 1.27, False)
# on/off switch: common -> EN_3V3; one throw VSYS, the other GND; 100k pull-down
SWX, SWY = 223.52, 194.31
s.add('lcsc:MSK12C02', 'SW203', SWX, SWY, 0, 'ON/OFF MSK12C02', 'lcsc:SW-TH_MSK12C02', extra={'LCSC': 'C431540'},
      fields={'ref': (SWX + 5.08, SWY + 2.54, 0, 'left'), 'val': (SWX + 5.08, SWY + 5.08, 0, 'left')})
p1, p2, p3 = P('SW203', '1'), P('SW203', '2'), P('SW203', '3')
s.nc(P('SW203', '4'))
W(p1, (p1[0], 182.88))
up('VSYS', (p1[0], 182.88))
W(p3, (p3[0], 185.42), (p3[0] + 5.08, 185.42))
gnd((p3[0] + 5.08, 185.42), 'right')
W(p2, (p2[0], 175.26), (233.68, 175.26), (233.68, en[1]), en)
s.label('EN_3V3', (226.06, 175.26))
R('R221', 236.22, 167.64, '100k', fields=lside(236.22, 167.64))
W((233.68, 175.26), (236.22, 175.26), P('R221', 2))
gnd(P('R221', 1), 'up')
s.text('Slide switch: off = radio off, the cell still charges on USB', (190.5, 205.74), 1.27, False)
# LP5907 for the DAC
LX, LY = 251.46, 220.98
s.add('Regulator_Linear:LP5907MFX-3.3', 'U207', LX, LY, 0, 'TPLP5907MFX-3.3', 'Package_TO_SOT_SMD:SOT-23-5',
      extra={'LCSC': 'C5370990'}, fields={'ref': (LX, LY - 11.43, 0, None), 'val': (LX, LY - 8.89, 0, None)})
lin, len_, lout = P('U207', 'IN'), P('U207', 'EN'), P('U207', 'OUT')
s.nc(P('U207', 'NC'))
gnd(P('U207', 'GND'))
W(lin, (236.22, lin[1]))
up('VSYS', (236.22, lin[1]), False)
C('C212', 236.22, lin[1] + 5.08, '1uF', fields=lside(236.22, lin[1] + 5.08))
gnd(P('C212', 2))
W(len_, (241.3, len_[1]), (241.3, len_[1] + 5.08))
s.label('EN_3V3', (241.3, len_[1] + 5.08), 270)
W(lout, (269.24, lout[1]))
C('C213', 264.16, lout[1] + 5.08, '1uF')
gnd(P('C213', 2))
up('+3V3A', (269.24, lout[1]))

warn = s.finalize()
import fieldcheck
fieldcheck.run(s)
s.write(OUT, ('ESP iRadio integrated board - power and USB', '2026-10-15', 'B', 'hghaken'))
print('written', OUT, 'warnings', warn)
