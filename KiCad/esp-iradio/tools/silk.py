"""Silkscreen of esp-iradio.kicad_pcb (KiCad's Python): board markings (title, cell/speaker polarity,
LED meaning, recovery pad names, USB, switch, RCA L/R) and, since 2026-10-16 (hand assembly), the
reference of every part, each moved next to its part where it touches no pad, no other part's
courtyard, no hole, no other text and stays on the board (`place_refs()`; the recovery pads carry
their signal name instead; a part boxed in by its neighbours gets its reference on its own body,
across it between the pads). Saves the board."""
import os
import pcbnew
import placement

HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(os.path.dirname(HERE), 'esp-iradio.kicad_pcb')
mm = pcbnew.FromMM
OX, OY = 100.0, 100.0

board = pcbnew.LoadBoard(PCB)
NAMED = ('TP203', 'TP204', 'TP205', 'TP206', 'TP207')    # recovery pads: signal names instead
REF_SIZE, REF_THICK = 0.9, 0.15
for fp in board.GetFootprints():
    ref = fp.GetReference()
    rt = fp.Reference()
    rt.SetVisible(not ref.startswith('H') and ref not in NAMED)
    rt.SetTextSize(pcbnew.VECTOR2I(mm(REF_SIZE), mm(REF_SIZE)))
    rt.SetTextThickness(mm(REF_THICK))
    rt.SetKeepUpright(True)
    fp.Value().SetVisible(False)             # values live on F.Fab / in the BOM
    for f in fp.GetFields():                 # LCSC and other fields: data only, never silkscreen
        if not f.IsReference():
            f.SetVisible(False)


old = [d for d in board.GetDrawings() if d.GetLayer() == pcbnew.F_SilkS and isinstance(d, pcbnew.PCB_TEXT)]
for d in old:                                    # remove the markings of an earlier run (after the
    board.Remove(d)                              # footprint loop: removing first crashed GetFootprints)


def text(s, x, y, size=1.0, rot=0, just=pcbnew.GR_TEXT_H_ALIGN_CENTER):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(s)
    t.SetPosition(pcbnew.VECTOR2I(mm(OX + x), mm(OY + y)))
    t.SetLayer(pcbnew.F_SilkS)
    t.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
    t.SetTextThickness(mm(0.15 if size >= 1 else 0.12))
    t.SetHorizJustify(just)
    t.SetTextAngleDegrees(rot)
    board.Add(t)


def pad_xy(ref, num):
    fp = board.FindFootprintByReference(ref)
    for p in fp.Pads():
        if p.GetNumber() == num:
            q = p.GetPosition()
            return pcbnew.ToMM(q.x) - OX, pcbnew.ToMM(q.y) - OY
    raise KeyError((ref, num))


def fp_xy(ref):
    q = board.FindFootprintByReference(ref).GetPosition()
    return pcbnew.ToMM(q.x) - OX, pcbnew.ToMM(q.y) - OY


# cell and speaker polarity (pin 1 = +)
x1, y1 = pad_xy('J6', '1')
x2, y2 = pad_xy('J6', '2')
text('+', x1, y1 - 2.6, 1.4)
text('-', x2, y2 - 2.6, 1.4)
# J202 stands on the right edge, pin 1 on top: + above, - and CELL below the connector
cb = board.FindFootprintByReference('J202').GetCourtyard(pcbnew.F_CrtYd).BBox()
x1, y1 = pad_xy('J202', '1')
text('+', x1, pcbnew.ToMM(cb.GetTop()) - OY - 1.0, 1.4)
text('-', x1, pcbnew.ToMM(cb.GetBottom()) - OY + 1.0, 1.4)
text('CELL', x1, pcbnew.ToMM(cb.GetBottom()) - OY + 2.6, 0.8)
x, y = pad_xy('J6', '1')
text('SPK', x + 1.0, y + 2.9, 0.8)
# charge LEDs, USB, switch
for ref, lab in (('D202', 'C'), ('D203', 'P')):   # CHG (red) and PWR (green): letters left of the LED
    led = board.FindFootprintByReference(ref)
    p1 = [p for p in led.Pads() if p.GetNumber() == '1'][0]
    x, y = pad_xy(ref, '1')
    text(lab, x - pcbnew.ToMM(p1.GetSize().x) / 2 - 0.2, y, 0.8, 0, pcbnew.GR_TEXT_H_ALIGN_RIGHT)
x, y = fp_xy('J201')
text('5V', x, y - 6.6, 0.8)
x, y = fp_xy('SW203')
text('ON/OFF ?', x, y - 3.2, 0.8)                # ON side: check on a sample (NOTES.md)
# RCA jacks: white = L barrel at x -9 from the origin (pads 1/2), red = R barrel at x +5 (pads 3/4)
x, y = fp_xy('J8')
text('L', x - 9.0, y - 3.5, 1.0)
text('R', x + 5.0, y - 3.5, 1.0)
# recovery pads
for ref, name in (('TP203', 'EN'), ('TP204', 'IO0'), ('TP205', 'TX'), ('TP206', 'RX'), ('TP207', 'GND')):
    x, y = pad_xy(ref, '1')
    text(name, x, y + 1.9, 0.8)
# title
text('ESP iRadio Rev C', placement.TITLE[0], placement.TITLE[1], 0.9)


def box_of(item, grow=0.0):
    bb = item.GetBoundingBox()
    g = mm(grow)
    return (bb.GetLeft() - g, bb.GetTop() - g, bb.GetRight() + g, bb.GetBottom() + g)


def hit(a, b):
    return a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]


def rect_dist(px, py, r):
    dx = max(r[0] - px, 0, px - r[2])
    dy = max(r[1] - py, 0, py - r[3])
    return (dx * dx + dy * dy) ** 0.5


def place_refs(first=()):
    """Move each visible reference next to its part: candidates around the courtyard (above, below,
    left, right, shifted along the side, then turned 90 degrees), nearest valid one wins."""
    fps = list(board.GetFootprints())
    pads = [(fp.GetReference(), box_of(p, 0.15)) for fp in fps for p in fp.Pads()]   # JLCPCB: 0.15 mm silk to pad
    # the parts' own silkscreen outlines and pin-1 marks (some reach past the courtyard)
    pads += [(fp.GetReference(), box_of(g, 0.1)) for fp in fps for g in fp.GraphicalItems()
             if g.GetLayer() == pcbnew.F_SilkS and not isinstance(g, pcbnew.PCB_TEXT)]
    crt = {}
    for fp in fps:
        bb = fp.GetCourtyard(pcbnew.F_CrtYd).BBox()
        if bb.GetWidth() == 0:
            bb = fp.GetBoundingBox(False)
        crt[fp.GetReference()] = (bb.GetLeft(), bb.GetTop(), bb.GetRight(), bb.GetBottom())
    taken = [box_of(d, 0.2) for d in board.GetDrawings() if d.GetLayer() == pcbnew.F_SilkS and isinstance(d, pcbnew.PCB_TEXT)]
    edge = board.GetBoardEdgesBoundingBox()
    lim = (edge.GetLeft() + mm(0.5), edge.GetTop() + mm(0.5), edge.GetRight() - mm(0.5), edge.GetBottom() - mm(0.5))
    order = sorted(fps, key=lambda f: (f.GetReference() not in first, (crt[f.GetReference()][2] - crt[f.GetReference()][0]) * (crt[f.GetReference()][3] - crt[f.GetReference()][1])))
    onbody = []
    for fp in order:                         # boxed in last time first, then small parts: fewest places
        ref = fp.GetReference()
        rt = fp.Reference()
        if not rt.IsVisible():
            continue
        x0, y0, x1, y1 = crt[ref]
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        best = None
        for size, thick in ((REF_SIZE, REF_THICK), (0.8, 0.12)):   # a smaller font only where tight
            if best is not None and not best[5]:
                break
            best = None
            rt.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
            rt.SetTextThickness(mm(thick))
            for ang in (0, 90):
                rt.SetTextAngleDegrees(ang)
                rt.SetPosition(pcbnew.VECTOR2I(int(cx), int(cy)))
                # the drawn letters, not GetBoundingBox() (that adds line spacing: 1.58 vs 1.05 mm tall)
                tb = rt.GetEffectiveTextShape().BBox()
                hw, hh = tb.GetWidth() / 2, tb.GetHeight() / 2
                ox, oy = tb.GetCenter().x - cx, tb.GetCenter().y - cy
                for gap in (0.0, 0.15, 0.3, 0.8, 1.5, 2.5, 3.5, 4.5):
                    g = mm(gap)
                    for sh in (0, -1, 1, -2, 2, -3, 3, -4, 4, -5, 5, -6, 6):
                        s_ = mm(sh)
                        for px, py in ((cx + s_, y0 - g - hh), (cx + s_, y1 + g + hh),
                                       (x0 - g - hw, cy + s_), (x1 + g + hw, cy + s_)):
                            b = (px - hw, py - hh, px + hw, py + hh)
                            if b[0] < lim[0] or b[1] < lim[1] or b[2] > lim[2] or b[3] > lim[3]:
                                continue
                            cost = abs(px - cx) + abs(py - cy) + (mm(1) if ang else 0)   # vertical is fine (owner)
                            # a text nearer to another part than to its own is ambiguous: avoid
                            own = rect_dist(px, py, crt[ref])
                            near = min((rect_dist(px, py, cb) for r, cb in crt.items()
                                        if r != ref and r[0] != 'H'), default=mm(99))
                            amb = near < own + mm(0.3)          # clearly nearer to its own part (Grok, round 9)
                            if amb:
                                cost += mm(6) + (own - near) * 3
                            else:                       # prefer the outside of a column to a shared gap
                                cost += max(0, mm(1.5) - (near - own)) * 2
                            if best and cost >= best[0]:
                                continue
                            if any(hit(b, pb) for r, pb in pads):
                                continue
                            if any(hit(b, cb) for r, cb in crt.items() if r != ref):
                                continue
                            if any(hit(b, tb_) for tb_ in taken):
                                continue
                            best = (cost, px - ox, py - oy, ang, b, amb, size, thick)
        if best is None or best[5]:
            # boxed in (no spot clearly nearer to this part than to a neighbour): the reference goes
            # on the part's own body, across it between the two pads (read while placing the part)
            onbody.append(ref)
            rt.SetTextSize(pcbnew.VECTOR2I(mm(0.8), mm(0.8)))
            rt.SetTextThickness(mm(0.12))
            rt.SetTextAngleDegrees(0 if (y1 - y0) > (x1 - x0) else 90)
            rt.SetPosition(pcbnew.VECTOR2I(int(cx), int(cy)))
            continue
        cost, px, py, ang, b, amb, size, thick = best
        rt.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
        rt.SetTextThickness(mm(thick))
        rt.SetTextAngleDegrees(ang)
        rt.SetPosition(pcbnew.VECTOR2I(int(px), int(py)))
        taken.append((b[0] - mm(0.2), b[1] - mm(0.2), b[2] + mm(0.2), b[3] + mm(0.2)))
    return onbody


# greedy: a part boxed in by its neighbours' texts gets first pick on the next pass
first = []
for _ in range(4):
    onbody = place_refs(first)
    if not onbody:
        break
    first = onbody + [r for r in first if r not in onbody]
print('references placed; on the part body (boxed in):', ' '.join(onbody) or 'none')

board.Save(PCB)
print('silkscreen updated')
