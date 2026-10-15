"""Check the integrated-board netlist against KiCad/07-integrated-board.md: every expected connection
(a set of ref.pin that must share one net) and nets that must stay apart."""
import re, sys
t = open(sys.argv[1], encoding='utf-8').read()
nets = {}
for chunk in t.split('(net (code ')[1:]:
    name = re.search(r'\(name "([^"]*)"\)', chunk).group(1)
    nets[name] = set('%s.%s' % (a, b) for a, b in re.findall(r'\(node \(ref "([^"]+)"\) \(pin "([^"]+)"\)', chunk))
where = {}
for n, nodes in nets.items():
    for x in nodes:
        where[x] = n
print(len(nets), 'nets')

E = [
    # USB-C
    ('J201.A4', 'J201.A9', 'J201.B4', 'J201.B9', 'F201.1'),                                 # VBUS (connector side of the fuse)
    ('J201.A6', 'J201.B6', 'U202.1'), ('U202.6', 'U203.5'),         # USBLC6 passes D+ through inside (1-6)
    ('J201.A7', 'J201.B7', 'U202.3'), ('U202.4', 'U203.6'),
    ('J201.A5', 'R201.1'), ('J201.B5', 'R202.1'),
    ('J201.A1', 'J201.A12', 'J201.B1', 'J201.B12', 'J201.S1', 'R201.2', 'R202.2', 'D201.2', 'U202.2',
     'U203.1', 'U201.8', 'U201.17', 'U201.4', 'U201.6', 'U3.1', 'U3.15', 'U3.38', 'U205.3', 'R220.2', 'RT201.2', 'C214.2', 'TP207.1'),
    ('F201.2', 'D201.1', 'U202.5', 'U201.13', 'R203.1', 'C201.1', 'TP201.1'),  # VIN: every clamp behind the fuse
    ('R203.2', 'R204.1', 'C214.1', 'U3.5'),            # VBUS_SENSE -> IO39
    # charger
    ('U201.10', 'U201.11', 'U201.5', 'C202.1', 'U206.4', 'U207.1', 'SW203.1', 'U2.7', 'R212.1', 'R214.1'),  # VSYS
    ('U201.14', 'R209.1'), ('U201.12', 'R210.1'), ('U201.16', 'R211.1'),
    ('U201.1', 'RT201.1'),
    ('U201.2', 'U201.3', 'Q203.2', 'C203.1', 'R217.1', 'TP202.1'),          # VBAT
    ('Q203.3', 'J202.1', 'R219.1'),                                       # cell +
    ('Q203.1', 'R216.1'),
    ('J202.2', 'U204.6', 'C206.2', 'U205.1'),                             # BATN
    ('U204.5', 'R219.2', 'C206.1'),
    ('U204.1', 'U205.6'), ('U204.3', 'U205.4'), ('U204.2', 'R220.1'),
    ('R217.2', 'R218.1', 'C207.1', 'U3.7'),                               # VBAT_SENSE -> IO35
    ('U201.9', 'D202.1', 'D204.1'), ('U201.7', 'D203.1', 'D205.1'),
    ('D204.2', 'R213.2', 'U3.6'),                                         # CHG -> IO34
    ('D205.2', 'R215.2', 'U3.4'),                                         # PGOOD -> IO36
    # regulators
    ('U206.5', 'U3.2', 'U203.16', 'U203.4', 'C209.1', 'C210.1', 'C211.1', 'R213.1', 'R215.1', 'R207.1', 'R208.1', 'U1.1', 'U1.20'),
    ('U206.1', 'SW203.2', 'R221.2', 'U207.3'),                            # EN_3V3
    ('U207.5', 'C213.1', 'U1.8'),                                         # +3V3A -> DAC AVDD
    # USB serial, auto-reset
    ('U203.2', 'U3.34', 'TP206.1'), ('U203.3', 'U3.35', 'TP205.1'),       # TXD->RXD0, RXD<-TXD0, recovery pads
    ('U203.13', 'R205.1', 'Q202.2'), ('U203.14', 'R206.1', 'Q201.2'),
    ('R205.2', 'Q201.1'), ('R206.2', 'Q202.1'),
    ('Q201.3', 'R207.2', 'C205.1', 'SW201.4', 'U3.3', 'TP203.1'),         # EN + recovery pad
    ('Q202.3', 'R208.2', 'SW202.4', 'U3.25', 'TP204.1'),                  # GPIO0 + recovery pad
    # radio sheet: display backlight, amp supply
    ('J3.2', 'R22.2'), ('R22.1', 'Q1.3'), ('Q1.1', 'R23.2', 'U3.11'),
    ('J3.3', 'J3.4', 'C5.1'),
    ('U2.4', 'R21.1', 'U3.24'),
]
bad = 0
for grp in E:
    ns = set(where.get(x, '<unconnected %s>' % x) for x in grp)
    if len(ns) != 1:
        bad += 1
        print('SPLIT', grp, '->', {x: where.get(x) for x in grp})
# nets that must not exist any more (review 2026-10-15): no bulk electrolytic on VSYS
for ref in ('C24',):
    if any(x.startswith(ref + '.') for x in where):
        bad += 1
        print('STILL THERE', ref)
PS = '/Power, charger and USB/'
APART = [(PS + 'VBUS', PS + 'VIN'), ('VSYS', PS + 'VBAT'), ('VSYS', '+3V3'), ('GND', PS + 'BATN')]
for a, b in APART:
    if a not in nets or b not in nets:
        bad += 1
        print('NET MISSING', a if a not in nets else b)
    elif nets[a] & nets[b]:
        bad += 1
        print('SHORT', a, b)
print('pin count of the WROVER: %d connected pins' % sum(1 for x in where if x.startswith('U3.')))
print('OK' if not bad else '%d problems' % bad)
for n in sorted(nets):
    if len(nets[n]) == 1:
        print('single-node net', n, nets[n])
