"""GND stitching for esp-iradio.kicad_pcb (KiCad's Python):
1. a via next to every GND SMD pad that has no via of its own (the router treats In1.Cu as the GND
   plane but does not always drop a via), and
2. a grid of stitching vias wherever the GND pours on F.Cu and B.Cu both have room,
so pour islands, pads and the In1.Cu plane are tied together. A via is only placed where its copper
(plus clearance) lies inside the filled GND pour on F.Cu and B.Cu and keeps clear of every In2.Cu
track and of other holes. Refills the zones and saves."""
import math
import os
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(os.path.dirname(HERE), 'esp-iradio.kicad_pcb')
mm, tomm = pcbnew.FromMM, pcbnew.ToMM
VIA_D, VIA_DRILL = 0.55, 0.3
CLR = 0.25          # copper clearance (0.2 rule + margin)
HOLE_GAP = 0.3      # hole to hole (edge to edge)
GRID = 2.5          # stitching grid (mm)

board = pcbnew.LoadBoard(PCB)
gnd = board.FindNet('GND')
filler = pcbnew.ZONE_FILLER(board)
filler.Fill(board.Zones())

fills = {}
for z in board.Zones():
    if z.GetIsRuleArea() or z.GetNetname() != 'GND':
        continue
    for lay in (pcbnew.F_Cu, pcbnew.B_Cu):
        if z.IsOnLayer(lay):
            fills.setdefault(lay, []).append(z.GetFilledPolysList(lay))


def inside_fill(lay, x, y, r):
    for ps in fills.get(lay, []):
        pts = [(x, y)] + [(x + r * math.cos(a), y + r * math.sin(a)) for a in [i * math.pi / 6 for i in range(12)]]
        if all(ps.Contains(pcbnew.VECTOR2I(int(px), int(py))) for px, py in pts):
            return True
    return False


def seg_dist(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    L = dx * dx + dy * dy
    t = 0 if L == 0 else max(0, min(1, ((px - ax) * dx + (py - ay) * dy) / L))
    cx, cy = ax + t * dx, ay + t * dy
    return math.hypot(px - cx, py - cy)


in2 = []        # (ax, ay, bx, by, halfwidth) of non-GND copper on In2.Cu
holes = []      # (x, y, radius) of every drilled hole
for t in board.GetTracks():
    if t.GetClass() == 'PCB_VIA':
        p = t.GetPosition()
        holes.append((p.x, p.y, t.GetDrillValue() / 2))
        if t.GetNetname() != 'GND':
            in2.append((p.x, p.y, p.x, p.y, t.GetWidth(pcbnew.In2_Cu) / 2 if hasattr(t, 'GetWidth') else mm(0.3)))
    elif t.GetLayer() == pcbnew.In2_Cu and t.GetNetname() != 'GND':
        a, b = t.GetStart(), t.GetEnd()
        in2.append((a.x, a.y, b.x, b.y, t.GetWidth() / 2))
for fp in board.GetFootprints():
    for pad in fp.Pads():
        if pad.GetDrillSizeX() > 0:
            p = pad.GetPosition()
            holes.append((p.x, p.y, pad.GetDrillSizeX() / 2))
            if pad.GetNetname() != 'GND':
                r = max(pad.GetSize().x, pad.GetSize().y) / 2
                in2.append((p.x, p.y, p.x, p.y, r))


EDGE = board.GetBoardEdgesBoundingBox()


def ok(x, y, via_d=VIA_D, drill=VIA_DRILL, clr=CLR):
    r = mm(via_d / 2)
    if min(x - EDGE.GetLeft(), EDGE.GetRight() - x, y - EDGE.GetTop(), EDGE.GetBottom() - y) < mm(1.0):
        return False
    if not (inside_fill(pcbnew.F_Cu, x, y, r) and inside_fill(pcbnew.B_Cu, x, y, r)):
        return False
    for ax, ay, bx, by, hw in in2:
        if seg_dist(x, y, ax, ay, bx, by) < r + hw + mm(clr):
            return False
    for hx, hy, hr in holes:
        if math.hypot(x - hx, y - hy) < mm(drill / 2) + hr + mm(HOLE_GAP):
            return False
    return True


def add_via(x, y, via_d=VIA_D, drill=VIA_DRILL):
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(pcbnew.VECTOR2I(int(x), int(y)))
    v.SetWidth(mm(via_d))
    v.SetDrill(mm(drill))
    v.SetNet(gnd)
    v.SetViaType(pcbnew.VIATYPE_THROUGH)
    board.Add(v)
    holes.append((x, y, mm(drill / 2)))


# 2. stitching grid
bb = board.GetBoardEdgesBoundingBox()
grid = 0
y = bb.GetTop() + mm(1.5)
while y < bb.GetBottom() - mm(1.0):
    x = bb.GetLeft() + mm(1.5)
    while x < bb.GetRight() - mm(1.0):
        if ok(x, y):
            add_via(x, y)
            grid += 1
        x += mm(GRID)
    y += mm(GRID)

filler.Fill(board.Zones())


# 3. every separate piece of the outer GND pours gets at least one via to the In1.Cu plane
def refresh_fills():
    fills.clear()
    for z in board.Zones():
        if z.GetIsRuleArea() or z.GetNetname() != 'GND':
            continue
        for lay in (pcbnew.F_Cu, pcbnew.B_Cu):
            if z.IsOnLayer(lay) and not z.IsOnLayer(pcbnew.In1_Cu):
                fills.setdefault(lay, []).append(z.GetFilledPolysList(lay))


def seg_seg(a, b, c, d):
    """distance between segments ab and cd (tuples of x, y)"""
    def cross(o, p, q):
        return (p[0] - o[0]) * (q[1] - o[1]) - (p[1] - o[1]) * (q[0] - o[0])
    if (cross(a, b, c) * cross(a, b, d) < 0) and (cross(c, d, a) * cross(c, d, b) < 0):
        return 0.0
    return min(seg_dist(a[0], a[1], c[0], c[1], d[0], d[1]), seg_dist(b[0], b[1], c[0], c[1], d[0], d[1]),
               seg_dist(c[0], c[1], a[0], a[1], b[0], b[1]), seg_dist(d[0], d[1], a[0], a[1], b[0], b[1]))


def track_clear(a, b, hw, clr=0.2):
    """True if an F.Cu track a-b (half width hw) keeps clr from all non-GND copper on F.Cu."""
    seg = pcbnew.SEG(pcbnew.VECTOR2I(int(a[0]), int(a[1])), pcbnew.VECTOR2I(int(b[0]), int(b[1])))
    for fp in board.GetFootprints():
        for pad in fp.Pads():
            if pad.GetNetname() == 'GND' or not pad.IsOnLayer(pcbnew.F_Cu):
                continue
            if pad.GetEffectiveShape(pcbnew.F_Cu).Collide(seg, int(hw + mm(clr))):
                return False
    for t in board.GetTracks():
        if t.GetNetname() == 'GND':
            continue
        if t.GetClass() == 'PCB_VIA':
            q = t.GetPosition()
            if seg_dist(q.x, q.y, a[0], a[1], b[0], b[1]) < t.GetWidth(pcbnew.F_Cu) / 2 + hw + mm(clr):
                return False
        elif t.GetLayer() == pcbnew.F_Cu:
            s0, s1 = t.GetStart(), t.GetEnd()
            if seg_seg(a, b, (s0.x, s0.y), (s1.x, s1.y)) < t.GetWidth() / 2 + hw + mm(clr):
                return False
    return True


def link_island(outline, gnd_pts):
    """Tie a pour island that has no room for a via to the nearest GND via or pad with a short F.Cu track."""
    pads = [p for fp in board.GetFootprints() for p in fp.Pads()
            if p.GetNetname() == 'GND' and p.IsOnLayer(pcbnew.F_Cu) and outline.PointInside(p.GetPosition())]
    targets = [(t.GetPosition().x, t.GetPosition().y) for t in board.GetTracks()
               if t.GetClass() == 'PCB_VIA' and t.GetNetname() == 'GND']
    targets += [(p.GetPosition().x, p.GetPosition().y) for fp in board.GetFootprints() for p in fp.Pads()
                if p.GetNetname() == 'GND' and p.IsOnLayer(pcbnew.F_Cu) and not outline.PointInside(p.GetPosition())]
    hw = mm(0.125)
    best = None
    for pad in pads:
        a = (pad.GetPosition().x, pad.GetPosition().y)
        for b in targets:
            d = abs(b[0] - a[0]) + abs(b[1] - a[1])
            if d > mm(7.0) or (best is not None and d >= best[0]):
                continue
            for path in ([a, b], [a, (a[0], b[1]), b], [a, (b[0], a[1]), b]):
                if all(track_clear(path[i], path[i + 1], hw) for i in range(len(path) - 1)):
                    best = (d, path, pad)
                    break
    if not best:
        return False
    d, path, pad = best
    for i in range(len(path) - 1):
        t = pcbnew.PCB_TRACK(board)
        t.SetStart(pcbnew.VECTOR2I(int(path[i][0]), int(path[i][1])))
        t.SetEnd(pcbnew.VECTOR2I(int(path[i + 1][0]), int(path[i + 1][1])))
        t.SetWidth(2 * hw)
        t.SetLayer(pcbnew.F_Cu)
        t.SetNet(gnd)
        board.Add(t)
    print('pour island at %s pad %s tied by a %.1f mm track' % (pad.GetParentFootprint().GetReference(), pad.GetNumber(), tomm(d)))
    return True


island_vias = 0
for rnd in range(3):
    refresh_fills()
    gnd_pts = [(t.GetPosition().x, t.GetPosition().y) for t in board.GetTracks()
               if t.GetClass() == 'PCB_VIA' and t.GetNetname() == 'GND']
    gnd_pts += [(p.GetPosition().x, p.GetPosition().y) for fp in board.GetFootprints() for p in fp.Pads()
                if p.GetNetname() == 'GND' and p.GetAttribute() == pcbnew.PAD_ATTRIB_PTH]
    added = 0
    for lay in (pcbnew.F_Cu, pcbnew.B_Cu):
        for ps in fills.get(lay, []):
            for i in range(ps.OutlineCount()):
                outline = ps.Outline(i)
                if any(outline.PointInside(pcbnew.VECTOR2I(int(x), int(y))) for x, y in gnd_pts):
                    continue
                bb = outline.BBox()
                found = False
                for vd, dr in ((VIA_D, VIA_DRILL), (0.5, 0.3)):      # smaller via only where needed (0.3 mm drill: JLCPCB standard, no surcharge)
                    step = mm(0.1)
                    y = bb.GetTop()
                    while y <= bb.GetBottom() and not found:
                        x = bb.GetLeft()
                        while x <= bb.GetRight():
                            if outline.PointInside(pcbnew.VECTOR2I(int(x), int(y))) and ok(x, y, vd, dr, 0.2):
                                add_via(x, y, vd, dr)
                                gnd_pts.append((x, y))
                                added += 1
                                found = True
                                break
                            x += step
                        y += step
                    if found:
                        break
                if not found and lay == pcbnew.F_Cu and link_island(outline, gnd_pts):
                    added += 1
                    found = True
                if not found:
                    c = bb.Centre()
                    print('island without room for a via on %s near (%.2f, %.2f)' % (
                        board.GetLayerName(lay), tomm(c.x), tomm(c.y)))
    island_vias += added
    filler.Fill(board.Zones())
    if not added:
        break

board.Save(PCB)
print('GND vias: %d stitching, %d for pour islands' % (grid, island_vias))
