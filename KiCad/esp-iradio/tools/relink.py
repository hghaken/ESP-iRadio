"""Point every footprint of esp-iradio.kicad_pcb at its schematic symbol again (path = sheet path +
symbol UUID from the netlist), without touching the layout. Run with KiCad's Python:
    "C:/Program Files/KiCad/9.0/bin/python.exe" relink.py <netlist.net>"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import pcbnew
import netparse

PCB = os.path.join(os.path.dirname(HERE), 'esp-iradio.kicad_pcb')
comps, _ = netparse.read(sys.argv[1])
board = pcbnew.LoadBoard(PCB)
n = 0
for fp in board.GetFootprints():
    c = comps.get(fp.GetReference())
    if not c:
        print('no symbol for', fp.GetReference())
        continue
    path = (c['sheet_tstamps'] or '/') + c['uuid']
    if fp.GetPath().AsString() != path:
        fp.SetPath(pcbnew.KIID_PATH(path))
        n += 1
board.Save(PCB)
print('%d footprint links updated' % n)
