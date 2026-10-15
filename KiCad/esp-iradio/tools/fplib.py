"""Footprint loading for the PCB scripts (run with KiCad's own Python: pcbnew)."""
import os
import pcbnew

KICAD_FP = 'C:/Program Files/KiCad/9.0/share/kicad/footprints'
HERE = os.path.dirname(os.path.abspath(__file__))
PRJ = os.path.dirname(HERE)
PROJECT_LIBS = {'esp-iradio': os.path.join(PRJ, 'lib', 'esp-iradio.pretty'),
                'lcsc': os.path.join(PRJ, 'lib', 'lcsc.pretty')}


def lib_path(nick):
    return PROJECT_LIBS.get(nick) or os.path.join(KICAD_FP, nick + '.pretty')


def load(fpid):
    nick, name = fpid.split(':', 1)
    fp = pcbnew.FootprintLoad(lib_path(nick), name)
    if fp is None:
        raise KeyError(fpid)
    fp.SetFPID(pcbnew.LIB_ID(nick, name))
    return fp


def courtyard(fp):
    """Courtyard box (mm) of an unplaced footprint: (xmin, ymin, xmax, ymax) relative to its origin."""
    bb = fp.GetCourtyard(pcbnew.F_CrtYd).BBox() if hasattr(fp, 'GetCourtyard') else None
    if bb is None or bb.GetWidth() == 0:
        bb = fp.GetBoundingBox(False)
    s = pcbnew.ToMM
    return (s(bb.GetLeft()), s(bb.GetTop()), s(bb.GetRight()), s(bb.GetBottom()))
