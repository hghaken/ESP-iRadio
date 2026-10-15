"""Design rules and net classes of the integrated board (JLCPCB 4-layer standard process), written into
esp-iradio.kicad_pro. Run with any Python 3 before the board scripts."""
import json
import os

PRO = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'esp-iradio.kicad_pro')
PS = '/Power, charger and USB/'


def netclass(name, track, clearance, via_d, via_drill, prio):
    return {'bus_width': 12, 'clearance': clearance, 'diff_pair_gap': 0.2, 'diff_pair_via_gap': 0.25,
            'diff_pair_width': 0.2, 'line_style': 0, 'microvia_diameter': 0.3, 'microvia_drill': 0.1,
            'name': name, 'pcb_color': 'rgba(0, 0, 0, 0.000)', 'priority': prio,
            'schematic_color': 'rgba(0, 0, 0, 0.000)', 'track_width': track, 'via_diameter': via_d,
            'via_drill': via_drill, 'wire_width': 6}


# nets that carry current: VBUS/VIN up to 1.4 A, cell and VSYS up to ~2 A peaks, speaker ~1 A
POWER = ['VSYS', 'GND', '+3V3', PS + 'VBUS', PS + 'VIN', PS + 'VBAT', PS + 'BATN', 'Net-(J202-Pin_1)']
# 0.3 mm: DAC supply, speaker outputs (they leave a 0.5 mm pitch QFN)
LIGHT = ['+3V3A', 'Net-(J6-Pin_1)', 'Net-(J6-Pin_2)']

d = json.load(open(PRO, encoding='utf-8'))
ns = d.setdefault('net_settings', {})
ns['classes'] = [netclass('Default', 0.2, 0.15, 0.55, 0.3, 2147483647),
                 netclass('Power', 0.5, 0.15, 0.7, 0.35, 0),
                 netclass('Supply', 0.3, 0.15, 0.55, 0.3, 1)]
ns['netclass_patterns'] = ([{'netclass': 'Power', 'pattern': n} for n in POWER] +
                           [{'netclass': 'Supply', 'pattern': n} for n in LIGHT])
ns['netclass_assignments'] = None
rules = d.setdefault('board', {}).setdefault('design_settings', {}).setdefault('rules', {})
rules.update({'min_clearance': 0.15, 'min_track_width': 0.15, 'min_connection': 0.15,
              'min_via_diameter': 0.4, 'min_via_annular_width': 0.1, 'min_through_hole_diameter': 0.2,  # 0.2: QFN thermal vias
              'min_copper_edge_clearance': 0.3, 'min_hole_clearance': 0.25, 'min_hole_to_hole': 0.25,
              'min_silk_clearance': 0.0, 'min_resolved_spokes': 1, 'min_text_height': 0.8, 'min_text_thickness': 0.12})
json.dump(d, open(PRO, 'w', encoding='utf-8', newline='\n'), indent=2)
print('rules and net classes written to', PRO)

# custom rule: the display module's M2.5 standoffs (H1, H5-H7) sit next to its sockets (J3, J7) by design;
# their courtyards touch, the parts do not (round standoffs ~4 mm across; a 5 mm hex would hit the socket). Every other overlap still counts.
DRU = PRO[:-len('.kicad_pro')] + '.kicad_dru'
HOLES = "(A.Reference == 'H1' || A.Reference == 'H5' || A.Reference == 'H6' || A.Reference == 'H7')"
SOCKS = "(B.Reference == 'J3' || B.Reference == 'J7')"
open(DRU, 'w', encoding='utf-8', newline='\n').write(
    '(version 1)\n'
    '(rule "display module standoffs next to its sockets"\n'
    '  (condition "%s && %s")\n'
    '  (constraint courtyard_clearance)\n'
    '  (severity ignore))\n' % (HOLES, SOCKS))
print('custom rules written to', DRU)
