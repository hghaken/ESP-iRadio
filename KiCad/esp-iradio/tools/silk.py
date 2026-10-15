"""Silkscreen of esp-iradio.kicad_pcb (KiCad's Python): small reference texts only where they help
(connectors, ICs, switches, LEDs, transistors, test pads); hidden on the dense 0603 passives (the
F.Fab assembly layer keeps every reference); board markings (title, cell/speaker polarity, LED
meaning, recovery pad names, USB). Saves the board."""
import os
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(os.path.dirname(HERE), 'esp-iradio.kicad_pcb')
mm = pcbnew.FromMM
OX, OY = 100.0, 100.0

board = pcbnew.LoadBoard(PCB)
SHOW = ('J', 'U', 'SW', 'D', 'Q')             # test pads carry their net name instead
for fp in board.GetFootprints():
    ref = fp.GetReference()
    rt = fp.Reference()
    show = ref.startswith(SHOW)
    rt.SetVisible(show)
    rt.SetTextSize(pcbnew.VECTOR2I(mm(0.8), mm(0.8)))
    rt.SetTextThickness(mm(0.12))
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


# cell and speaker polarity (pin 1 = +)
for ref in ('J202', 'J6'):
    x1, y1 = pad_xy(ref, '1')
    x2, y2 = pad_xy(ref, '2')
    text('+', x1, y1 - 2.6, 1.4)
    text('-', x2, y2 - 2.6, 1.4)
x, y = pad_xy('J202', '1')
text('CELL', x + 1.0, y + 2.9, 0.8)
x, y = pad_xy('J6', '1')
text('SPK', x + 1.0, y + 2.9, 0.8)
# charge LEDs, USB, switch
x, y = pad_xy('D202', '1')
text('CHG', x - 2.6, y, 0.8, 0, pcbnew.GR_TEXT_H_ALIGN_RIGHT)
x, y = pad_xy('D203', '1')
text('PWR', x - 2.6, y, 0.8, 0, pcbnew.GR_TEXT_H_ALIGN_RIGHT)
text('5V', 35.5, 49.3, 0.8)
text('ON/OFF ?', 48.5, 53.4, 0.8)                 # ON side: check on a sample (NOTES.md)
# recovery pads
for ref, name in (('TP203', 'EN'), ('TP204', 'IO0'), ('TP205', 'TX'), ('TP206', 'RX'), ('TP207', 'GND')):
    x, y = pad_xy(ref, '1')
    text(name, x, y + 1.9, 0.8)
# title
text('ESP iRadio  Rev B  2026-10', 40.0, 1.6, 0.9)

board.Save(PCB)
print('silkscreen updated')
