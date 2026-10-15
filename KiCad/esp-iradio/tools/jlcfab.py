"""JLCPCB fabrication files for the integrated board (KiCad's Python):
    "C:/Program Files/KiCad/9.0/bin/python.exe" jlcfab.py
Writes ../jlcpcb/: esp-iradio_bom.csv, esp-iradio_cpl.csv and, with kicad-cli, esp-iradio_gerbers.zip
(Gerbers of the four copper layers, mask, paste, silk and outline plus the Excellon drill files).

- BOM: one row per LCSC part; the LCSC number comes from the footprint's `LCSC` field, else from
  PASSIVES below (JLCPCB basic parts for plain R/C, chosen from the JLCPCB Tools database). Parts
  excluded from the BOM (test pads, holes) are left out; a part with no LCSC number is listed with
  an empty number and reported (JLCPCB then leaves it unplaced).
- CPL: centre of the pads, in mm from the board's auxiliary origin, Y up (as the Gerbers), rotation
  in degrees. KiCad's own footprints get the rotation corrections of the JLCPCB Tools plugin
  (CORRECTIONS, same rules); footprints from the `lcsc` library come from EasyEDA, which is what
  JLCPCB places from, so they get none. Always check every part in JLCPCB's placement preview.
"""
import csv
import os
import re
import subprocess
import sys
import zipfile
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
PRJ = os.path.dirname(HERE)
PCB = os.path.join(PRJ, 'esp-iradio.kicad_pcb')
OUT = os.path.join(PRJ, 'jlcpcb')
CLI = r'C:\Program Files\KiCad\9.0\bin\kicad-cli.exe'

# plain R/C without an LCSC field: (footprint name, value) -> LCSC (JLCPCB basic unless noted)
PASSIVES = {
    ('R_0603_1608Metric', '0R'): 'C21189',
    ('R_0603_1608Metric', '22R'): 'C23345',
    ('R_0603_1608Metric', '100R'): 'C22775',
    ('R_0603_1608Metric', '470R'): 'C23179',
    ('R_0603_1608Metric', '1k'): 'C21190',
    ('R_0603_1608Metric', '1.1k'): 'C22764',        # preferred
    ('R_0603_1608Metric', '1.13k'): 'C22833',       # extended (BQ24074 ILIM, 1 %)
    ('R_0603_1608Metric', '5.1k'): 'C23186',
    ('R_0603_1608Metric', '10k'): 'C25804',
    ('R_0603_1608Metric', '68k'): 'C23231',
    ('R_0603_1608Metric', '100k'): 'C25803',
    ('R_0603_1608Metric', '220k'): 'C22961',
    ('R_0603_1608Metric', '330k'): 'C23137',
    ('R_0603_1608Metric', '470k'): 'C23178',
    ('R_0603_1608Metric', '1M'): 'C22935',
    ('C_0603_1608Metric', '2.2nF'): 'C1604',        # 50 V X7R
    ('C_0603_1608Metric', '100nF'): 'C14663',       # 50 V X7R
    ('C_0603_1608Metric', '1uF'): 'C15849',         # 50 V X5R
    ('C_0603_1608Metric', '2.2uF'): 'C23630',       # 16 V X5R (DAC, 3.3 V)
    ('C_0805_2012Metric', '4.7uF 25V'): 'C1779',    # 25 V X5R
    ('C_0805_2012Metric', '10uF'): 'C15850',        # 25 V X5R
    ('C_0805_2012Metric', '22uF'): 'C45783',        # 25 V X5R
}

# rotation corrections of the JLCPCB Tools plugin (jlcpcb/corrections.db) that apply to KiCad's own
# footprints on this board, plus TQFN (the plugin's QFN rule misses the T prefix; same pin-1 corner)
CORRECTIONS = [
    (r'^SOT-23', -90),                       # SOT-23, SOT-23-5, SOT-23-6 (plugin rule)
    (r'^SOIC-', 270),
    (r'^TSSOP-', 270),
    (r'^(.*?_|V|T)?QFN-(16|20|24|28|40)(-|_|$)', 270),
    (r'^USB_C_Receptacle_HRO_TYPE-C-31-M-12', 180),
]
NO_CORRECTION_LIBS = ('lcsc', 'esp-iradio')

# per LCSC part, from JLCPCB's placement preview (2026-10-16): their 3D model of these parts sits in
# another base orientation than the footprint rule assumes; this correction replaces the rule
PART_CORRECTIONS = {
    'C2897373': 90,     # PM254 1x10 socket (J3, J7): the rule-less 0 put the row across the module rows
    'C165948': 0,       # USB-C TYPE-C-31-M-12 (J201): the plugin's 180 turned the opening into the board
    'C2861252': 0,      # BQ24074RGTT (U201): the QFN 270 put pin 1 top right instead of top left
    'C7420353': 180,    # MMBT3904 (Q201, Q202): SOT-23 -90 left the body across the pads
    'C5261054': 180,    # DMG2305UX (Q203): same
    'C5370990': 180,    # TPLP5907 SOT-23-5 (U207): same
}


def correction(fp):
    lcsc = lcsc_of(fp)
    if lcsc in PART_CORRECTIONS:
        return PART_CORRECTIONS[lcsc]
    if fp.GetFPID().GetLibNickname().wx_str() in NO_CORRECTION_LIBS:
        return 0
    name = fp.GetFPID().GetLibItemName().wx_str()
    for rx, rot in CORRECTIONS:
        if re.search(rx, name):
            return rot
    return 0


def lcsc_of(fp):
    if fp.HasFieldByName('LCSC') and fp.GetFieldText('LCSC').strip():
        return fp.GetFieldText('LCSC').strip()
    return PASSIVES.get((fp.GetFPID().GetLibItemName().wx_str(), fp.GetValue()), '')


def centre(fp):
    pads = list(fp.Pads())
    if not pads:
        return fp.GetPosition()
    bb = pads[0].GetBoundingBox()
    for p in pads[1:]:
        bb.Merge(p.GetBoundingBox())
    return bb.GetCenter()


def refkey(r):
    m = re.match(r'([A-Z]+)(\d+)', r)
    return (m.group(1), int(m.group(2))) if m else (r, 0)


board = pcbnew.LoadBoard(PCB)
os.makedirs(OUT, exist_ok=True)
aux = board.GetDesignSettings().GetAuxOrigin()
groups, cpl, missing = {}, [], []
for fp in sorted(board.GetFootprints(), key=lambda f: refkey(f.GetReference())):
    ref = fp.GetReference()
    if fp.IsExcludedFromBOM() or fp.IsDNP():
        continue
    lcsc = lcsc_of(fp)
    if not lcsc:
        missing.append(ref)
    name = fp.GetFPID().GetLibItemName().wx_str()
    # one BOM line per LCSC part: JLCPCB flags two lines that match the same part
    key = (lcsc, name) if lcsc else ('', name, fp.GetValue())
    groups.setdefault(key, {'vals': [], 'refs': []})
    if fp.GetValue() not in groups[key]['vals']:
        groups[key]['vals'].append(fp.GetValue())
    groups[key]['refs'].append(ref)
    if fp.IsExcludedFromPosFiles() or not lcsc:
        continue
    c = centre(fp)
    rot = (fp.GetOrientationDegrees() + correction(fp)) % 360
    if fp.GetLayer() != pcbnew.F_Cu:
        sys.exit('%s is on the bottom side: not handled' % ref)
    cpl.append([ref, '%.4f' % pcbnew.ToMM(c.x - aux.x), '%.4f' % -pcbnew.ToMM(c.y - aux.y), 'Top',
                '%g' % rot])

with open(os.path.join(OUT, 'esp-iradio_bom.csv'), 'w', newline='', encoding='utf-8') as f:
    w = csv.writer(f)
    w.writerow(['Comment', 'Designator', 'Footprint', 'LCSC Part #', 'Quantity'])
    for key, g in sorted(groups.items(), key=lambda kv: refkey(kv[1]['refs'][0])):
        w.writerow([' / '.join(g['vals']), ','.join(g['refs']), key[1], key[0], len(g['refs'])])
with open(os.path.join(OUT, 'esp-iradio_cpl.csv'), 'w', newline='', encoding='utf-8') as f:
    w = csv.writer(f)
    w.writerow(['Designator', 'Mid X', 'Mid Y', 'Layer', 'Rotation'])
    w.writerows(cpl)
print('BOM: %d lines, %d parts; CPL: %d placements' % (len(groups), sum(len(g['refs']) for g in groups.values()), len(cpl)))
if missing:
    print('no LCSC number (not placed by JLCPCB):', ' '.join(missing))

# Gerbers + drill files with kicad-cli, zipped for the upload
gdir = os.path.join(OUT, 'gerbers')
os.makedirs(gdir, exist_ok=True)
for fn in os.listdir(gdir):
    os.remove(os.path.join(gdir, fn))
layers = 'F.Cu,In1.Cu,In2.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts'
subprocess.run([CLI, 'pcb', 'export', 'gerbers', '--layers', layers, '--subtract-soldermask',
                '--no-x2', '-o', gdir + os.sep, PCB], check=True, capture_output=True)
subprocess.run([CLI, 'pcb', 'export', 'drill', '--format', 'excellon', '--excellon-separate-th',
                '--excellon-units', 'mm', '--generate-map', '--map-format', 'gerberx2',
                '-o', gdir + os.sep, PCB], check=True, capture_output=True)
zp = os.path.join(OUT, 'esp-iradio_gerbers.zip')
with zipfile.ZipFile(zp, 'w', zipfile.ZIP_DEFLATED) as z:
    for fn in sorted(os.listdir(gdir)):
        z.write(os.path.join(gdir, fn), fn)
print('Gerbers: %d files in %s' % (len(os.listdir(gdir)), os.path.relpath(zp, PRJ)))
