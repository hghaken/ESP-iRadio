"""Project copies of two KiCad footprints whose KiCad 3D models are not installed on the owner's PC
(the Connector_USB and Rotary_Encoder 3D folders are missing): the pads stay KiCad's, the model is
the EasyEDA one already in lib/lcsc.3dshapes, aligned on the pads (2026-10-16). Run with any
Python 3; writes lib/esp-iradio.pretty/<name>.kicad_mod.

Alignment, measured on the pads of both footprints (tools: fplib + pcbnew):
- USB-C HRO TYPE-C-31-M-12: the EasyEDA footprint (lcsc:USB-C_SMD-TYPE-C-31-M-12_1) has its pegs and
  shell tabs 1.40 mm lower than KiCad's (pegs y -1.21 vs -2.60, tabs -1.71 vs -3.13), same x, same
  orientation: the model moves 1.40 mm up (3D offset y +1.40; 3D Y points up), its own 180 degree z
  turn stays.
- EC11E encoder: the EasyEDA footprint (lcsc:SW-TH_EC11XXXXXXXX) has A/C/B in a row at the bottom
  (y +7.25) and the shaft at its origin; KiCad's has A/C/B in a column on the left and the shaft at
  (7.5, 2.5): the model turns -90 degrees about z and moves to (7.5, 2.5) (3D offset y -2.5)."""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
LIB = os.path.join(os.path.dirname(HERE), 'lib')
KICAD_FP = 'C:/Program Files/KiCad/9.0/share/kicad/footprints'

PARTS = [
    ('Connector_USB', 'USB_C_Receptacle_HRO_TYPE-C-31-M-12', 'USB-C_SMD-TYPE-C-31-M-12_1.wrl', (0, 1.40, 0), (0, 0, 180)),
    ('Rotary_Encoder', 'RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm',
     'SW-TH_EC11XXXXXXXX-L11.7-W12.0-H24.5-P2.5-LS14.5.wrl', (7.5, -2.5, 0), (0, 0, -90)),
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

# the EasyEDA model of the PJ-320D jack came with every material fully transparent (transparency 1.0):
# make it solid (the other EasyEDA models have 0.0)
p = os.path.join(LIB, 'lcsc.3dshapes', 'AUDIO-SMD_PJ-320D-1.wrl')
w = open(p, encoding='utf-8').read()
if 'transparency 1.0' in w:
    open(p, 'w', encoding='utf-8', newline='\n').write(w.replace('transparency 1.0', 'transparency 0.0'))
    print('PJ-320D model made solid')
