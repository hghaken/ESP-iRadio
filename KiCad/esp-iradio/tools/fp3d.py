"""Project copies of two KiCad footprints whose KiCad 3D models are not installed on the owner's PC
(the Connector_USB and Rotary_Encoder 3D folders are missing): the pads stay KiCad's, the model is
the EasyEDA one already in lib/lcsc.3dshapes, aligned on the pads (2026-10-16). Run with any
Python 3; writes lib/esp-iradio.pretty/<name>.kicad_mod.

Alignment, measured on the pads of both footprints (tools: fplib + pcbnew):
- USB-C HRO TYPE-C-31-M-12: aligned on the pegs inside the model itself (2026-10-16, corrected):
  after its own 180 degree z turn the model's pegs sit 3.40 mm below its origin, KiCad's holes 2.60
  mm, so the model moves 0.80 mm down (3D offset y -0.80; 3D Y points up). Its front then lands on
  KiCad's F.Fab front (y +3.65) and its tab legs on the shell pads. (Aligning on the EasyEDA
  footprint's pegs, +1.40, left the model 2.2 mm behind the real part.)
- EC11E encoder: the EasyEDA footprint (lcsc:SW-TH_EC11XXXXXXXX) has A/C/B in a row at the bottom
  (y +7.25) and the shaft at its origin; KiCad's has A/C/B in a column on the left and the shaft at
  (7.5, 2.5): the model turns -90 degrees about z and moves to (7.25, 2.5) (3D offset y -2.5; x 7.25,
  not 7.5: the model's pins then sit in the holes, 2026-10-17).
Also (2026-10-17): a project copy of KiCad's MaiXu MX126-5.0-02P screw terminal footprint (J6) with
the simple model from `tools/terminal3d.py`, and offset fixes in `lib/lcsc.pretty` footprints whose
EasyEDA model sits beside its pads (checked by the model's pins or pegs against the holes)."""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
LIB = os.path.join(os.path.dirname(HERE), 'lib')
KICAD_FP = 'C:/Program Files/KiCad/9.0/share/kicad/footprints'

PARTS = [
    ('Connector_USB', 'USB_C_Receptacle_HRO_TYPE-C-31-M-12', 'USB-C_SMD-TYPE-C-31-M-12_1.wrl', (0, -0.80, 0), (0, 0, 180)),
    ('Rotary_Encoder', 'RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm',
     'SW-TH_EC11XXXXXXXX-L11.7-W12.0-H24.5-P2.5-LS14.5.wrl', (7.25, -2.5, 0), (0, 0, -90)),
]

for lib, name, model, off, rot in PARTS:
    s = open(os.path.join(KICAD_FP, lib + '.pretty', name + '.kicad_mod'), encoding='utf-8').read()
    i = s.index('\t(model ')
    j = s.rindex(')')                      # closing bracket of the footprint
    block = ('\t(model "${KIPRJMOD}/lib/lcsc.3dshapes/%s"\n\t\t(offset\n\t\t\t(xyz %g %g %g)\n\t\t)\n'
             '\t\t(scale\n\t\t\t(xyz 1 1 1)\n\t\t)\n\t\t(rotate\n\t\t\t(xyz %g %g %g)\n\t\t)\n\t)\n' % ((model,) + off + rot))
    s = s[:i] + block + s[j:]
    s = re.sub(r'\(descr "', '(descr "Project copy with the EasyEDA 3D model (tools/fp3d.py). ', s, count=1)
    out = os.path.join(LIB, 'esp-iradio.pretty', name + '.kicad_mod')
    open(out, 'w', encoding='utf-8', newline='\n').write(s)
    print('written', os.path.relpath(out, LIB))

# J3 (display CN1 socket): KiCad's socket model plus the display module (tools/display3d.py), so the
# 3D view shows the module where it will sit; J7 keeps the plain KiCad socket.
name = 'PinSocket_1x10_P2.54mm_Vertical'
s = open(os.path.join(KICAD_FP, 'Connector_PinSocket_2.54mm.pretty', name + '.kicad_mod'), encoding='utf-8').read()
j = s.rindex(')')
s = (s[:j] + '\t(model "${KIPRJMOD}/lib/esp-iradio.3dshapes/HS20S010B_on_sockets.wrl"\n\t\t(offset\n\t\t\t(xyz 0 0 0)\n'
     '\t\t)\n\t\t(scale\n\t\t\t(xyz 1 1 1)\n\t\t)\n\t\t(rotate\n\t\t\t(xyz 0 0 0)\n\t\t)\n\t)\n' + s[j:])
s = s.replace('(footprint "%s"' % name, '(footprint "%s_HS20S010B"' % name, 1)
s = re.sub(r'\(descr "', '(descr "Project copy with the HS20S010B display module model (tools/fp3d.py). ', s, count=1)
out = os.path.join(LIB, 'esp-iradio.pretty', name + '_HS20S010B.kicad_mod')
open(out, 'w', encoding='utf-8', newline='\n').write(s)
print('written', os.path.relpath(out, LIB))

# J6 (speaker): KiCad's MX126 2-pole 5.0 mm screw terminal footprint with the own simple model
name = 'TerminalBlock_MaiXu_MX126-5.0-02P_1x02_P5.00mm'
s = open(os.path.join(KICAD_FP, 'TerminalBlock.pretty', name + '.kicad_mod'), encoding='utf-8').read()
i = s.index('\t(model ')
j = s.rindex(')')
s = (s[:i] + '\t(model "${KIPRJMOD}/lib/esp-iradio.3dshapes/TerminalBlock_MX126-5.0-02P.wrl"\n\t\t(offset\n'
     '\t\t\t(xyz 0 0 0)\n\t\t)\n\t\t(scale\n\t\t\t(xyz 1 1 1)\n\t\t)\n\t\t(rotate\n\t\t\t(xyz 0 0 0)\n\t\t)\n\t)\n' + s[j:])
s = re.sub(r'\(descr "', '(descr "Project copy with a simple own 3D model (tools/fp3d.py, tools/terminal3d.py). ', s, count=1)
out = os.path.join(LIB, 'esp-iradio.pretty', name + '.kicad_mod')
open(out, 'w', encoding='utf-8', newline='\n').write(s)
print('written', os.path.relpath(out, LIB))

# EasyEDA models that sit beside their pads in the lcsc footprints (easyeda2kicad output): 3D offset
# (x, y up) that puts the model's pins or pegs into the holes
LCSC_OFFSETS = {
    'SW-TH_MSK12C02': (0, -1.55, 0),     # SW203: pegs 1.55 mm behind the holes (owner saw it, 2026-10-17)
}
for name, off in LCSC_OFFSETS.items():
    p = os.path.join(LIB, 'lcsc.pretty', name + '.kicad_mod')
    w = open(p, encoding='utf-8').read()
    w2 = re.sub(r'\(offset \(xyz [^)]*\)\)', '(offset (xyz %g %g %g))' % off, w, count=1)
    if w2 != w:
        open(p, 'w', encoding='utf-8', newline='\n').write(w2)
        print('model offset set:', name, off)

# the EasyEDA model of the PJ-320D jack came with every material fully transparent (transparency 1.0):
# make it solid (the other EasyEDA models have 0.0)
p = os.path.join(LIB, 'lcsc.3dshapes', 'AUDIO-SMD_PJ-320D-1.wrl')
w = open(p, encoding='utf-8').read()
if 'transparency 1.0' in w:
    open(p, 'w', encoding='utf-8', newline='\n').write(w.replace('transparency 1.0', 'transparency 0.0'))
    print('PJ-320D model made solid')
