"""Simple VRML model of the HS20S010B display module as it sits on the board (2026-10-16), for the
3D view and the STEP-less case check: module PCB 63.2 x 36.2 x 1.6 mm, the LCD body over the glass
outline, the active area, and the two 1x10 male header spacers it stands on. Origin = pin 1 of CN1
(the J3 socket footprint origin), units of 0.1 inch as KiCad's VRML models; 3D Y points up (footprint
y down). Heights: socket 8.5 mm + header spacer 2.5 mm = module PCB from z 11.0; glass front ~13.9.
Dimensions from the HS20S010B drawing (see 07-integrated-board.md); a picture, not a fit check.
Writes lib/esp-iradio.3dshapes/HS20S010B_on_sockets.wrl. Run with any Python 3."""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), 'lib', 'esp-iradio.3dshapes', 'HS20S010B_on_sockets.wrl')

# module PCB corner relative to CN1 pin 1 (footprint coordinates, y down)
MX, MY = -61.2, -6.67
Z_PCB = 11.0

MATS = {
    'pcb': (0.05, 0.16, 0.42),
    'lcd': (0.08, 0.08, 0.09),
    'active': (0.02, 0.05, 0.13),
    'header': (0.06, 0.06, 0.06),
}


def box(x0, y0, z0, x1, y1, z1, mat):
    """Axis-aligned box from footprint coordinates (mm, y down) to a VRML shape."""
    s = 1 / 2.54
    xa, xb = x0 * s, x1 * s
    ya, yb = -y1 * s, -y0 * s
    za, zb = z0 * s, z1 * s
    pts = [(xa, ya, za), (xb, ya, za), (xb, yb, za), (xa, yb, za),
           (xa, ya, zb), (xb, ya, zb), (xb, yb, zb), (xa, yb, zb)]
    faces = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    r, g, b = MATS[mat]
    return ('Shape {\n appearance Appearance { material Material { diffuseColor %g %g %g specularColor 0.2 0.2 0.2 '
            'ambientIntensity 0.3 shininess 0.3 transparency 0 } }\n geometry IndexedFaceSet {\n  coord Coordinate { point [ %s ] }\n'
            '  coordIndex [ %s ]\n }\n}\n') % (r, g, b, ', '.join('%.4f %.4f %.4f' % p for p in pts),
                                               ', '.join('%d, %d, %d, %d, -1' % f for f in faces))


parts = [
    # module PCB
    box(MX, MY, Z_PCB, MX + 63.2, MY + 36.2, Z_PCB + 1.6, 'pcb'),
    # LCD body over the glass outline (51.8 wide from x 5.94), 1.3 mm
    box(MX + 5.94, MY + 0.4, Z_PCB + 1.6, MX + 5.94 + 51.8, MY + 36.2 - 0.4, Z_PCB + 2.9, 'lcd'),
    # active area 40.86 x 30.6, 8.04 mm from the CN1 end, 2.8 mm from the top
    box(MX + 63.2 - 8.04 - 40.86, MY + 2.8, Z_PCB + 2.9, MX + 63.2 - 8.04, MY + 2.8 + 30.6, Z_PCB + 2.95, 'active'),
    # male header spacers (2.5 mm) on CN1 (x 0) and CN2 (x -59.2), pins y 0..22.86
    box(-1.27, -1.27, 8.5, 1.27, 24.13, Z_PCB, 'header'),
    box(-59.2 - 1.27, -1.27, 8.5, -59.2 + 1.27, 24.13, Z_PCB, 'header'),
]
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
    f.write('#VRML V2.0 utf8\n# HS20S010B display module on its sockets (tools/display3d.py)\n')
    f.writelines(parts)
print('written', OUT)
