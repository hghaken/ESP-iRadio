"""Hand edits on the routed esp-iradio.kicad_pcb (agreed in the layout review, claude-/grok-bevindingen.md
round 5); no regeneration, no new route. Run with KiCad's Python after stitch.py and silk.py:
1. U201 (BQ24074) exposed pad: a fifth thermal via in the middle of the four (07: >= 5);
2. U3 (WROVER-IE) ground land, 3 x 3 squares: a via at each inner corner of the gaps, as in
   Espressif's land pattern (datasheet v2.4, figure 15), small and tented;
3. charge-LED labels: "C" (CHG, red D202) and "P" (PWR, green D203) left of the LEDs, off the
   USB-C body (the full words do not fit between the connector and the LEDs).
0. (2026-10-16) no hole below 0.3 mm: JLCPCB's standard process starts there, 0.2 mm holes cost
   ~$37 extra per order. The thermal-via pads of the QFN footprints (0.5 mm pad, 0.2 mm drill)
   get a 0.3 mm drill, the vias below are 0.5 / 0.3 mm (an older smaller via is resized).
Every new via is checked against the copper of other nets on all layers and against other holes;
it is skipped (with a message) when it does not fit. Refills the zones and saves.
Idempotent: a via already at the spot is not added twice."""
import math
import os
import pcbnew
import placement

HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(os.path.dirname(HERE), 'esp-iradio.kicad_pcb')
mm, tomm = pcbnew.FromMM, pcbnew.ToMM
OX, OY = placement.OX, placement.OY
CLR, HOLE_GAP = 0.2, 0.25
MIN_DRILL = 0.3

board = pcbnew.LoadBoard(PCB)
gnd = board.FindNet('GND')


def seg_dist(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    L = dx * dx + dy * dy
    t = 0 if L == 0 else max(0, min(1, ((px - ax) * dx + (py - ay) * dy) / L))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def via_fits(x, y, d, drill, skip=None):
    r = mm(d / 2)
    c = pcbnew.VECTOR2I(int(x), int(y))
    seg = pcbnew.SEG(c, c)
    for t in board.GetTracks():
        if skip is not None and t.m_Uuid.AsString() == skip.m_Uuid.AsString():
            continue
        if t.GetClass() == 'PCB_VIA':
            q = t.GetPosition()
            gap = math.hypot(q.x - x, q.y - y) - mm(drill / 2) - t.GetDrillValue() / 2
            if gap < mm(HOLE_GAP):
                return 'hole %.2f mm from via at %s' % (tomm(gap), (round(tomm(q.x) - OX, 2), round(tomm(q.y) - OY, 2)))
            if t.GetNetname() != 'GND' and math.hypot(q.x - x, q.y - y) < r + t.GetWidth(pcbnew.F_Cu) / 2 + mm(CLR):
                return 'too close to a %s via' % t.GetNetname()
        elif t.GetNetname() != 'GND':
            a, b = t.GetStart(), t.GetEnd()
            if seg_dist(x, y, a.x, a.y, b.x, b.y) < r + t.GetWidth() / 2 + mm(CLR):
                return 'too close to a %s track on %s' % (t.GetNetname(), t.GetLayerName())
    for fp in board.GetFootprints():
        for p in fp.Pads():
            if p.GetDrillSizeX() > 0:
                q = p.GetPosition()
                gap = math.hypot(q.x - x, q.y - y) - mm(drill / 2) - p.GetDrillSizeX() / 2
                if gap < mm(HOLE_GAP):
                    return 'hole %.2f mm from a hole of %s' % (tomm(gap), fp.GetReference())
            if p.GetNetname() == 'GND':
                continue
            for lay in (pcbnew.F_Cu, pcbnew.B_Cu):
                if p.IsOnLayer(lay) and p.GetEffectiveShape(lay).Collide(seg, int(r + mm(CLR))):
                    return 'too close to pad %s of %s' % (p.GetNumber(), fp.GetReference())
    return None


def add_via(x, y, d, drill, why):
    for t in board.GetTracks():
        if t.GetClass() == 'PCB_VIA' and math.hypot(t.GetPosition().x - x, t.GetPosition().y - y) < mm(0.05):
            if t.GetDrillValue() < mm(drill) - 1 or t.GetWidth(pcbnew.F_Cu) < mm(d) - 1:
                problem = via_fits(x, y, d, drill, skip=t)
                if problem:
                    print('NOT RESIZED %s: %s' % (why, problem))
                    return True
                t.SetWidth(mm(d))
                t.SetDrill(mm(drill))
                print('via resized to %.2f / %.2f: %s' % (d, drill, why))
            else:
                print('via already there:', why)
            return True
    problem = via_fits(x, y, d, drill)
    if problem:
        print('SKIPPED %s: %s' % (why, problem))
        return False
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(pcbnew.VECTOR2I(int(x), int(y)))
    v.SetWidth(mm(d))
    v.SetDrill(mm(drill))
    v.SetNet(gnd)
    v.SetViaType(pcbnew.VIATYPE_THROUGH)
    board.Add(v)
    print('via added:', why)
    return True


# 0. thermal-via pads of the footprints (QFN ThermalVias): 0.3 mm drill, pad at least 0.5 mm
for fp in board.GetFootprints():
    for p in fp.Pads():
        if 0 < p.GetDrillSizeX() < mm(MIN_DRILL) - 1:
            p.SetDrillSize(pcbnew.VECTOR2I(mm(MIN_DRILL), mm(MIN_DRILL)))
            if p.GetSize().x < mm(MIN_DRILL + 0.2):
                p.SetSize(pcbnew.VECTOR2I(mm(MIN_DRILL + 0.2), mm(MIN_DRILL + 0.2)))
            print('pad %s.%s: drill %.2f mm' % (fp.GetReference(), p.GetNumber(), MIN_DRILL))

# 1. fifth thermal via under the BQ24074: the centre of its exposed pad
fp = board.FindFootprintByReference('U201')
ep = [p for p in fp.Pads() if p.GetNumber() == '17' and p.GetDrillSizeX() == 0][0]
c = ep.GetPosition()
# the centre first, then the middles between the footprint's four vias (0.59 mm off centre)
off = mm(0.59)
for dx, dy in ((0, 0), (0, -off), (0, off), (-off, 0), (off, 0)):
    if add_via(c.x + dx, c.y + dy, 0.5, 0.3, 'U201 exposed pad, 5th thermal via (%+.2f, %+.2f)' % (tomm(dx), tomm(dy))):
        break

# 2. WROVER ground land: vias at the four inner corners of the 3 x 3 squares
fp = board.FindFootprintByReference('U3')
sq = [p.GetPosition() for p in fp.Pads() if p.GetNumber() == '39']
xs = sorted({q.x for q in sq})
ys = sorted({q.y for q in sq})
for i in range(2):
    for j in range(2):
        x = (xs[i] + xs[i + 1]) / 2
        y = (ys[j] + ys[j + 1]) / 2
        add_via(x, y, 0.5, 0.3, 'U3 ground land (%.2f, %.2f)' % (tomm(x) - OX, tomm(y) - OY))

# 3. charge-LED labels
for ref, old, new in (('D202', 'CHG', 'C'), ('D203', 'PWR', 'P')):
    led = board.FindFootprintByReference(ref)
    p1 = [p for p in led.Pads() if p.GetNumber() == '1'][0]
    x = p1.GetPosition().x - p1.GetSize().x / 2 - mm(0.2)
    y = p1.GetPosition().y
    for d in board.GetDrawings():
        if isinstance(d, pcbnew.PCB_TEXT) and d.GetLayer() == pcbnew.F_SilkS and d.GetText() in (old, new):
            d.SetText(new)
            d.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_RIGHT)
            d.SetPosition(pcbnew.VECTOR2I(int(x), int(y)))
            print('label %s -> "%s" at (%.2f, %.2f)' % (old, new, tomm(x) - OX, tomm(y) - OY))

pcbnew.ZONE_FILLER(board).Fill(board.Zones())
board.Save(PCB)
print('saved')
