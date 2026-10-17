"""Autoroute esp-iradio.kicad_pcb with Freerouting (CLI), then fill the zones. Run with KiCad's Python:
    "C:/Program Files/KiCad/9.0/bin/python.exe" route.py [passes]
Exports a Specctra DSN next to the board, runs freerouting-cli, imports the session, fills zones, saves."""
import os
import subprocess
import sys
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
PRJ = os.path.dirname(HERE)
PCB = os.path.join(PRJ, 'esp-iradio.kicad_pcb')
JAVA = os.environ.get('JAVA', 'C:/Users/Admin/tools/jdk-21.0.12.1+1-jre/bin/java.exe')
FR = os.environ.get('FREEROUTING', 'C:/Users/Admin/tools/freerouting-1.9.0.jar')   # 2.x jars need Java 25
args = [a for a in sys.argv[1:] if not a.startswith('--')]
passes = args[0] if args else '100'
KEEP = '--keep' in sys.argv          # keep the existing tracks, route only what is missing
work = os.path.join(PRJ, 'route')
os.makedirs(work, exist_ok=True)
dsn, ses = os.path.join(work, 'board.dsn'), os.path.join(work, 'board.ses')

board = pcbnew.LoadBoard(PCB)
if not KEEP:
    for t in list(board.GetTracks()):
        board.Remove(t)
for z in list(board.Zones()):
    if not z.GetIsRuleArea() and not z.IsOnLayer(pcbnew.In1_Cu) and (z.IsOnLayer(pcbnew.F_Cu) or z.IsOnLayer(pcbnew.B_Cu)):
        board.Remove(z)          # outer GND pours are added again after routing
assert pcbnew.ExportSpecctraDSN(board, dsn), 'DSN export failed'
print('DSN written', dsn)
# -dct 0: no "Autorouter is about to start" confirmation dialog with its countdown (owner, 2026-10-17)
r = subprocess.run([JAVA, '-jar', FR, '-de', dsn, '-do', ses, '-mp', passes, '-dct', '0'],
                   capture_output=True, text=True, cwd=work)
print(r.stdout[-3000:])
print(r.stderr[-2000:])
assert os.path.exists(ses), 'no session file'
assert pcbnew.ImportSpecctraSES(board, ses), 'SES import failed'
# GND pours on the outer layers (connect the QFN ground pins to their exposed pads, shield the board)
gnd = board.FindNet('GND')
bb = board.GetBoardEdgesBoundingBox()
for lay, prio in ((pcbnew.F_Cu, 0), (pcbnew.B_Cu, 0)):
    z = pcbnew.ZONE(board)
    z.SetLayer(lay)
    z.SetNet(gnd)
    z.SetAssignedPriority(prio)
    z.SetLocalClearance(pcbnew.FromMM(0.2))
    z.SetMinThickness(pcbnew.FromMM(0.2))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL)          # SMD pads solid (reflow); THT pads below
    z.SetThermalReliefGap(pcbnew.FromMM(0.25))
    z.SetThermalReliefSpokeWidth(pcbnew.FromMM(0.3))
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    o = z.Outline()
    o.NewOutline()
    m = pcbnew.FromMM(0.3)
    for x, y in ((bb.GetLeft() + m, bb.GetTop() + m), (bb.GetRight() - m, bb.GetTop() + m),
                 (bb.GetRight() - m, bb.GetBottom() - m), (bb.GetLeft() + m, bb.GetBottom() - m)):
        o.Append(x, y)
    board.Add(z)
for fp in board.GetFootprints():            # through-hole pads keep thermal reliefs (soldering)
    for pad in fp.Pads():
        if pad.GetAttribute() == pcbnew.PAD_ATTRIB_PTH and pad.GetNetname() == 'GND' and pad.GetDrillSizeX() > pcbnew.FromMM(0.5):
            pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_THERMAL)
filler = pcbnew.ZONE_FILLER(board)
filler.Fill(board.Zones())
board.Save(PCB)
print('routed and saved', PCB)
