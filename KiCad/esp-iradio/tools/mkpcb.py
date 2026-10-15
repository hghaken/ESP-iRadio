"""Build esp-iradio.kicad_pcb from the schematic netlist: 4 layers, outline, footprints with fields,
schematic links (path, sheet), nets and the placement in placement.py. Run with KiCad's Python:
    "C:/Program Files/KiCad/9.0/bin/python.exe" mkpcb.py <netlist.net> [--check]
--check only reports courtyard overlaps and parts outside the board, it writes nothing.
Overwrites the board: only for the first layout, before any hand routing."""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import pcbnew
import fplib
import netparse
import placement

PRJ = os.path.dirname(HERE)
PCB = os.path.join(PRJ, 'esp-iradio.kicad_pcb')
OX, OY = 100.0, 100.0            # board origin in KiCad coordinates (mm)
mm = pcbnew.FromMM


def vec(x, y):
    return pcbnew.VECTOR2I(mm(OX + x), mm(OY + y))


def courtyard_abs(fp):
    bb = None
    for lay in (pcbnew.F_CrtYd,):
        try:
            sh = fp.GetCourtyard(lay)
            if sh.OutlineCount():
                bb = sh.BBox()
        except Exception:
            pass
    if bb is None or bb.GetWidth() == 0:
        bb = fp.GetBoundingBox(False)
    for pad in fp.Pads():                # EasyEDA courtyards do not always cover the pads
        pb = pad.GetBoundingBox()
        pb.Inflate(mm(0.1))
        bb.Merge(pb)
    t = pcbnew.ToMM
    return (t(bb.GetLeft()) - OX, t(bb.GetTop()) - OY, t(bb.GetRight()) - OX, t(bb.GetBottom()) - OY)


def build(netfile, check_only=False):
    comps, nets = netparse.read(netfile)
    # always start from an empty board file (the project file next to it keeps rules and net classes)
    target = PCB if not check_only else os.path.join(PRJ, '_placement_check.kicad_pcb')
    open(target, 'w', encoding='utf-8').write('(kicad_pcb (version 20241229) (generator "pcbnew") (generator_version "9.0"))')
    board = pcbnew.LoadBoard(target)
    if check_only:
        os.remove(target)
    board.SetCopperLayerCount(4)
    board.SetLayerType(pcbnew.In1_Cu, pcbnew.LT_POWER)     # GND plane only: the router keeps signals off it

    # nets
    netinfo = {}
    for name in sorted(nets):
        ni = pcbnew.NETINFO_ITEM(board, name)
        board.Add(ni)
        netinfo[name] = ni
    pad_net = {}
    for name, nodes in nets.items():
        for ref, pin in nodes:
            pad_net[(ref, pin)] = name

    missing = [r for r in comps if r not in placement.P]
    if missing:
        print('NOT PLACED:', ' '.join(sorted(missing)))
    boxes = {}
    for ref, c in sorted(comps.items()):
        fp = fplib.load(c['footprint'])
        fp.SetReference(ref)
        fp.SetValue(c['value'])
        for k, v in c['fields'].items():
            if k in ('Footprint', 'Datasheet', 'Description') or not v:
                continue
            fp.SetField(k, v)
        path = (c['sheet_tstamps'] or '/') + c['uuid']
        fp.SetPath(pcbnew.KIID_PATH(path))
        fp.SetSheetname(c.get('sheetname') or '')
        fp.SetSheetfile(c.get('sheetfile') or '')
        x, y, rot = placement.P.get(ref, (90.0, 10.0 + 3 * len(boxes) % 50, 0))
        fp.SetPosition(vec(x, y))
        fp.SetOrientationDegrees(rot)
        for pad in fp.Pads():
            n = pad_net.get((ref, pad.GetNumber()))
            if n:
                pad.SetNet(netinfo[n])
        board.Add(fp)
        boxes[ref] = courtyard_abs(fp)

    # checks: overlap of courtyards, inside the outline
    bad = 0
    refs = sorted(boxes)
    for i, a in enumerate(refs):
        ax0, ay0, ax1, ay1 = boxes[a]
        if a[0] != 'H' and (ax0 < -0.6 or ay0 < -0.6 or ax1 > placement.W + 0.6 or ay1 > placement.H + 0.6):
            print('OUTSIDE %-6s %s' % (a, ['%.1f' % v for v in boxes[a]]))
            bad += 1
        for b in refs[i + 1:]:
            bx0, by0, bx1, by1 = boxes[b]
            ox = min(ax1, bx1) - max(ax0, bx0)
            oy = min(ay1, by1) - max(ay0, by0)
            # the display module's M2.5 holes sit 3.9 mm from its header ends: standoffs <= 4.5 mm round
            if {a[0], b[0]} == {'H', 'J'} and {a, b} & {'J3', 'J7'}:
                continue
            if ox > 0.05 and oy > 0.05:
                print('OVERLAP %-6s %-6s %.2f x %.2f' % (a, b, ox, oy))
                bad += 1
    for r in os.environ.get('SHOWBOX', '').split():
        print('BOX %-6s %s' % (r, ' '.join('%.2f' % v for v in boxes[r])))
    print('%d parts, %d nets, %d placement problems' % (len(comps), len(nets), bad))
    if check_only:
        return board

    # outline, display panel outline, title text
    def seg(layer, x0, y0, x1, y1, w=0.1):
        s = pcbnew.PCB_SHAPE(board)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(vec(x0, y0))
        s.SetEnd(vec(x1, y1))
        s.SetLayer(layer)
        s.SetWidth(mm(w))
        board.Add(s)

    def rect(layer, x0, y0, x1, y1, w=0.1):
        seg(layer, x0, y0, x1, y0, w); seg(layer, x1, y0, x1, y1, w)
        seg(layer, x1, y1, x0, y1, w); seg(layer, x0, y1, x0, y0, w)

    rect(pcbnew.Edge_Cuts, 0, 0, placement.W, placement.H, 0.1)
    for k, (label, box) in enumerate(placement.OUTLINES):
        rect(pcbnew.Dwgs_User, *box, w=0.15)
        txt = pcbnew.PCB_TEXT(board)
        txt.SetText(label)
        txt.SetPosition(vec(box[0] + 1, box[1] + 1.5 + 1.6 * k))
        txt.SetLayer(pcbnew.Dwgs_User)
        txt.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_LEFT)
        board.Add(txt)

    # GND plane on In1.Cu (the router connects GND pads to it with vias)
    z = pcbnew.ZONE(board)
    z.SetLayer(pcbnew.In1_Cu)
    z.SetNet(netinfo['GND'])
    z.SetIsRuleArea(False)
    ol = z.Outline()
    ol.NewOutline()
    for x, y in ((0.3, 0.3), (placement.W - 0.3, 0.3), (placement.W - 0.3, placement.H - 0.3), (0.3, placement.H - 0.3)):
        ol.Append(mm(OX + x), mm(OY + y))
    z.SetLocalClearance(mm(0.2))
    z.SetMinThickness(mm(0.2))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL)
    z.SetAssignedPriority(0)
    board.Add(z)

    # keep tracks and vias 0.6 mm from the board edge (JLCPCB: copper >= 0.3 mm from a routed edge)
    W_, H_ = placement.W, placement.H
    for x0, y0, x1, y1 in ((0, 0, W_, 0.6), (0, H_ - 0.6, W_, H_), (0, 0, 0.6, H_), (W_ - 0.6, 0, W_, H_)):
        k = pcbnew.ZONE(board)
        k.SetIsRuleArea(True)
        k.SetDoNotAllowTracks(True)
        k.SetDoNotAllowVias(True)
        k.SetDoNotAllowPads(False)
        k.SetDoNotAllowCopperPour(False)
        k.SetDoNotAllowFootprints(False)
        ls = pcbnew.LSET()
        for lay in (pcbnew.F_Cu, pcbnew.In1_Cu, pcbnew.In2_Cu, pcbnew.B_Cu):
            ls.AddLayer(lay)
        k.SetLayerSet(ls)
        o = k.Outline()
        o.NewOutline()
        for x, y in ((x0, y0), (x1, y0), (x1, y1), (x0, y1)):
            o.Append(mm(OX + x), mm(OY + y))
        board.Add(k)

    board.Save(PCB)
    print('written', PCB)
    return board


if __name__ == '__main__':
    build(sys.argv[1], '--check' in sys.argv)
