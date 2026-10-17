"""Simple VRML model of the 2-pole 5.0 mm screw terminal block J6 (speaker; Ningbo Kangnex WJ126V-5.0-02P,
LCSC C8404, the 126 form factor of KiCad's MaiXu MX126-5.0-02P footprint), 2026-10-17. KiCad's own
STEP model for that footprint is not installed on this PC (no TerminalBlock.3dshapes folder).
Body over the footprint's F.Fab outline (x -3..7.5, y -4..3.8 from pin 1), 10 mm high, wire openings
on the +y face (KiCad's terminal blocks: wire entry towards +y; checked on the Phoenix MKDS model),
a screw head over each pin. Units of 0.1 inch as KiCad's VRML models; 3D Y points up (footprint y
down). A picture, not a fit check.
Writes lib/esp-iradio.3dshapes/TerminalBlock_MX126-5.0-02P.wrl. Run with any Python 3."""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), 'lib', 'esp-iradio.3dshapes', 'TerminalBlock_MX126-5.0-02P.wrl')

MATS = {
    'body': (0.10, 0.55, 0.25),
    'hole': (0.03, 0.03, 0.03),
    'metal': (0.70, 0.70, 0.72),
}
H = 10.0


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


parts = [box(-3.0, -4.0, 0, 7.5, 3.8, H, 'body')]
for px in (0.0, 5.0):
    parts.append(box(px - 1.6, 3.8, 2.0, px + 1.6, 3.85, 5.0, 'hole'))          # wire opening, +y face
    parts.append(box(px - 1.5, -1.5, H, px + 1.5, 1.5, H + 0.05, 'metal'))       # screw head
    parts.append(box(px - 1.4, -0.15, H + 0.05, px + 1.4, 0.15, H + 0.1, 'hole'))  # slot
    parts.append(box(px - 0.5, -0.5, -3.5, px + 0.5, 0.5, 0, 'metal'))           # pin
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
    f.write('#VRML V2.0 utf8\n# 2-pole 5.0 mm screw terminal block (tools/terminal3d.py)\n')
    f.writelines(parts)
print('written', OUT)
