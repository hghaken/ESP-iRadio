"""Connect a pad that the autorouter left unconnected, the way it would be done by hand (KiCad's Python):
    "C:/Program Files/KiCad/9.0/bin/python.exe" bridge.py REF PAD [REF PAD ...]
For each pad: a straight or L-shaped F.Cu track to copper of the same net, or a short F.Cu stub, a new
via, and a straight or L-shaped track on B.Cu or In2.Cu to an existing via of the net. Tracks are 0.3 mm
(they have to leave fine-pitch pads), every candidate is checked against the copper of other nets on its
layer, other holes and the edge keep-out; the shortest that fits is used. Refills zones, saves."""
import math
import os
import sys
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(os.path.dirname(HERE), 'esp-iradio.kicad_pcb')
mm, tomm = pcbnew.FromMM, pcbnew.ToMM
OX, OY = 100.0, 100.0
CLR, HOLE_GAP, EDGE = 0.2, 0.25, 0.7
HW = mm(0.15)                       # half width of the bridge tracks (0.3 mm)
VIA_D, VIA_DRILL = 0.55, 0.3

board = pcbnew.LoadBoard(PCB)
EB = board.GetBoardEdgesBoundingBox()


def seg_dist(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    L = dx * dx + dy * dy
    t = 0 if L == 0 else max(0, min(1, ((px - ax) * dx + (py - ay) * dy) / L))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def seg_seg(a, b, c, d):
    def cross(o, p, q):
        return (p[0] - o[0]) * (q[1] - o[1]) - (p[1] - o[1]) * (q[0] - o[0])
    if cross(a, b, c) * cross(a, b, d) < 0 and cross(c, d, a) * cross(c, d, b) < 0:
        return 0.0
    return min(seg_dist(a[0], a[1], c[0], c[1], d[0], d[1]), seg_dist(b[0], b[1], c[0], c[1], d[0], d[1]),
               seg_dist(c[0], c[1], a[0], a[1], b[0], b[1]), seg_dist(d[0], d[1], a[0], a[1], b[0], b[1]))


def inside(x, y, margin):
    return min(x - EB.GetLeft(), EB.GetRight() - x, y - EB.GetTop(), EB.GetBottom() - y) >= mm(margin)


def seg_ok(a, b, layer, net):
    if not (inside(*a, EDGE) and inside(*b, EDGE)):
        return False
    seg = pcbnew.SEG(pcbnew.VECTOR2I(int(a[0]), int(a[1])), pcbnew.VECTOR2I(int(b[0]), int(b[1])))
    for fp in board.GetFootprints():
        for p in fp.Pads():
            if p.GetNetname() == net or not p.IsOnLayer(layer):
                continue
            if p.GetEffectiveShape(layer).Collide(seg, int(HW + mm(CLR))):
                return False
    for t in board.GetTracks():
        if t.GetNetname() == net:
            continue
        if t.GetClass() == 'PCB_VIA':
            q = t.GetPosition()
            if seg_dist(q.x, q.y, a[0], a[1], b[0], b[1]) < t.GetWidth(layer) / 2 + HW + mm(CLR):
                return False
        elif t.GetLayer() == layer:
            s0, s1 = t.GetStart(), t.GetEnd()
            if seg_seg(a, b, (s0.x, s0.y), (s1.x, s1.y)) < t.GetWidth() / 2 + HW + mm(CLR):
                return False
    return True


def via_ok(x, y, net):
    if not inside(x, y, 1.0):
        return False
    r = mm(VIA_D / 2)
    c = pcbnew.VECTOR2I(int(x), int(y))
    seg = pcbnew.SEG(c, c)
    for t in board.GetTracks():
        if t.GetClass() == 'PCB_VIA':
            q = t.GetPosition()
            if math.hypot(q.x - x, q.y - y) - mm(VIA_DRILL / 2) - t.GetDrillValue() / 2 < mm(HOLE_GAP):
                return False
            if t.GetNetname() != net and math.hypot(q.x - x, q.y - y) < r + t.GetWidth(pcbnew.F_Cu) / 2 + mm(CLR):
                return False
        elif t.GetNetname() != net:
            a, b = t.GetStart(), t.GetEnd()
            if seg_dist(x, y, a.x, a.y, b.x, b.y) < r + t.GetWidth() / 2 + mm(CLR):
                return False
    for fp in board.GetFootprints():
        for p in fp.Pads():
            if p.GetDrillSizeX() > 0:
                q = p.GetPosition()
                if math.hypot(q.x - x, q.y - y) - mm(VIA_DRILL / 2) - p.GetDrillSizeX() / 2 < mm(HOLE_GAP):
                    return False
            if p.GetNetname() == net:
                continue
            for lay in (pcbnew.F_Cu, pcbnew.B_Cu):
                if p.IsOnLayer(lay) and p.GetEffectiveShape(lay).Collide(seg, int(r + mm(CLR))):
                    return False
    return True


def paths(a, b):
    return ([a, b], [a, (a[0], b[1]), b], [a, (b[0], a[1]), b])


def add_track(path, layer, net):
    for i in range(len(path) - 1):
        t = pcbnew.PCB_TRACK(board)
        t.SetStart(pcbnew.VECTOR2I(int(path[i][0]), int(path[i][1])))
        t.SetEnd(pcbnew.VECTOR2I(int(path[i + 1][0]), int(path[i + 1][1])))
        t.SetWidth(2 * HW)
        t.SetLayer(layer)
        t.SetNet(net)
        board.Add(t)


def bridge(ref, num):
    fp = board.FindFootprintByReference(ref)
    pad = [p for p in fp.Pads() if p.GetNumber() == num][0]
    netname, net = pad.GetNetname(), pad.GetNet()
    a = (pad.GetPosition().x, pad.GetPosition().y)
    same_pads = {(id(p)) for p in fp.Pads() if p.GetNetname() == netname}
    targets = []                       # (x, y, is_via)
    for t in board.GetTracks():
        if t.GetNetname() != netname:
            continue
        if t.GetClass() == 'PCB_VIA':
            targets.append((t.GetPosition().x, t.GetPosition().y, True))
        else:
            for v in (t.GetStart(), t.GetEnd()):
                if math.hypot(v.x - a[0], v.y - a[1]) > mm(1.5):     # not this pad's own stub
                    targets.append((v.x, v.y, False))
    best = None
    # 1. F.Cu only
    for x, y, is_via in targets:
        d = abs(x - a[0]) + abs(y - a[1])
        if d > mm(12) or (best and d >= best[0]):
            continue
        for pth in paths(a, (x, y)):
            if all(seg_ok(pth[i], pth[i + 1], pcbnew.F_Cu, netname) for i in range(len(pth) - 1)):
                best = (d, [(pcbnew.F_Cu, pth)], None)
                break
    # 2. stub + new via + inner/bottom layer to an existing via of the net
    if best is None:
        for sd in (1.0, 1.5, 2.0, 2.5):
            for k in range(16):
                ang = k * math.pi / 8
                v = (a[0] + mm(sd) * math.cos(ang), a[1] + mm(sd) * math.sin(ang))
                if not (seg_ok(a, v, pcbnew.F_Cu, netname) and via_ok(v[0], v[1], netname)):
                    continue
                for x, y, is_via in targets:
                    if not is_via:
                        continue
                    d = mm(sd) + abs(x - v[0]) + abs(y - v[1])
                    if d > mm(25) or (best and d >= best[0]):
                        continue
                    for lay in (pcbnew.B_Cu, pcbnew.In2_Cu):
                        ok = False
                        for pth in paths(v, (x, y)):
                            if all(seg_ok(pth[i], pth[i + 1], lay, netname) for i in range(len(pth) - 1)):
                                best = (d, [(pcbnew.F_Cu, [a, v]), (lay, pth)], v)
                                ok = True
                                break
                        if ok:
                            break
    if best is None:
        print('%s pad %s (%s): no path found' % (ref, num, netname))
        return False
    d, parts, v = best
    for lay, pth in parts:
        add_track(pth, lay, net)
    if v:
        via = pcbnew.PCB_VIA(board)
        via.SetPosition(pcbnew.VECTOR2I(int(v[0]), int(v[1])))
        via.SetWidth(mm(VIA_D))
        via.SetDrill(mm(VIA_DRILL))
        via.SetNet(net)
        via.SetViaType(pcbnew.VIATYPE_THROUGH)
        board.Add(via)
    print('%s pad %s (%s): bridged, %.1f mm, %s' % (ref, num, netname, tomm(d),
          ' + '.join(board.GetLayerName(l) for l, _ in parts) + (' + via' if v else '')))
    return True


args = sys.argv[1:]
for i in range(0, len(args), 2):
    bridge(args[i], args[i + 1])
pcbnew.ZONE_FILLER(board).Fill(board.Zones())
board.Save(PCB)
print('saved')
