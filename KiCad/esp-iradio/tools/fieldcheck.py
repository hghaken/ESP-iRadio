"""Rough readability check for a schgen Sheet: visible field texts that overlap each other,
cross a wire, or sit on another symbol's body. Geometry is approximate (stroke font)."""

CW, CH = 1.15, 1.6     # mm per character, text height incl. margin (1.27 mm font)


def text_box(txt, x, y, angle, justify):
    w = len(txt) * CW
    horiz = angle % 180 == 0
    if horiz:
        if justify == 'left':
            x0 = x
        elif justify == 'right':
            x0 = x - w
        else:
            x0 = x - w / 2
        return (x0, y - CH / 2, x0 + w, y + CH / 2)
    if justify == 'left':
        y1 = y
    elif justify == 'right':
        y1 = y + w
    else:
        y1 = y + w / 2
    return (x - CH / 2, y1 - w, x + CH / 2, y1)


def inter(a, b):
    return a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]


def seg_hits(box, a, b):
    lo_x, hi_x = sorted((a[0], b[0])); lo_y, hi_y = sorted((a[1], b[1]))
    return inter(box, (lo_x - 0.05, lo_y - 0.05, hi_x + 0.05, hi_y + 0.05))


def run(sheet):
    texts = []
    bodies = []
    for ref, s in sheet.symbols.items():
        x, y, r = s['x'], s['y'], s['r']
        f = s['fields']
        for key, txt, hide, default in (('ref', ref, s['hide_ref'], (x + 3.81, y - 1.27, 0, 'left')),
                                        ('val', s['value'], s['hide_value'], (x + 3.81, y + 1.27, 0, 'left'))):
            if hide or ref.startswith('#FLG'):
                continue
            fx, fy, fa, fj = f.get(key, default)
            disp = (fa + r) % 180          # field angle is relative to the symbol
            texts.append(('%s.%s' % (ref, key), txt, text_box(txt, fx, fy, disp, fj)))
        pts = [s['pins'][k] for k in s['nums']]
        if len(pts) >= 2 and not ref.startswith('#'):
            xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
            x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
            if len(pts) == 2:          # two-pin part: body in the middle half
                if x1 - x0 < 0.1:
                    y0, y1 = y0 + (y1 - y0) * 0.25, y1 - (y1 - y0) * 0.25
                    x0, x1 = x0 - 1.3, x1 + 1.3
                else:
                    x0, x1 = x0 + (x1 - x0) * 0.25, x1 - (x1 - x0) * 0.25
                    y0, y1 = y0 - 1.3, y1 + 1.3
            else:
                x0 += 1.5; x1 -= 1.5; y0 += 0.5; y1 -= 0.5
                if x1 < x0: x0, x1 = x - 1.5, x + 1.5
                if y1 < y0: y0, y1 = y - 1.5, y + 1.5
            bodies.append((ref, (x0, y0, x1, y1)))
    n = 0
    for i, (na, ta, ba) in enumerate(texts):
        for nb, tb, bb in texts[i + 1:]:
            if inter(ba, bb):
                print('TEXT %s "%s" overlaps %s "%s"' % (na, ta, nb, tb)); n += 1
        for a, b in sheet.segs:
            if seg_hits(ba, a, b):
                print('WIRE %s "%s" crosses wire %s-%s' % (na, ta, a, b)); n += 1
                break
        for ref, bb in bodies:
            if ref != na.split('.')[0] and inter(ba, bb):
                print('BODY %s "%s" on %s' % (na, ta, ref)); n += 1
    print('fieldcheck: %d issues' % n)
    return n
