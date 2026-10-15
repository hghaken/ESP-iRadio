"""Tiny KiCad 9 schematic writer: embeds library symbols, places symbols, draws wires.

Coordinates in mm, schematic space (y down). Pin positions are computed from the library
(y up) with the symbol's rotation (CCW degrees, as in the file)."""
import re, uuid, os

WIRE_HEAD = '\t(wire'
WIRE_FMT = ('\t(wire\n\t\t(pts\n\t\t\t(xy %g %g) (xy %g %g)\n\t\t)\n\t\t(stroke\n\t\t\t(width 0)\n'
            '\t\t\t(type default)\n\t\t)\n\t\t(uuid "%s")\n\t)\n')
KICAD_SYM = 'C:/Program Files/KiCad/9.0/share/kicad/symbols/'


def _u():
    return str(uuid.uuid4())


def _block(text, start):
    depth, i, ins = 0, start, False
    while True:
        ch = text[i]
        if ins:
            if ch == '\\':
                i += 2
                continue
            if ch == '"':
                ins = False
        elif ch == '"':
            ins = True
        elif ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
        i += 1


class Libs:
    def __init__(self, extra):
        self.paths = dict(extra)  # libname -> file
        self.cache = {}

    def text(self, lib):
        if lib not in self.cache:
            p = self.paths.get(lib, KICAD_SYM + lib + '.kicad_sym')
            self.cache[lib] = open(p, encoding='utf-8').read()
        return self.cache[lib]

    def raw(self, lib, name):
        t = self.text(lib)
        m = re.search(r'\n\s*\(symbol "%s"[\s\n]' % re.escape(name), t)
        if not m:
            raise KeyError(lib + ':' + name)
        blk = _block(t, t.index('(symbol', m.start()))
        ext = re.search(r'\(extends "([^"]+)"\)', blk)
        if ext:
            # flatten: parent graphics and pins, the child's own properties
            parent = self.raw(lib, ext.group(1))
            pname = ext.group(1)
            child_props = re.findall(r'\(property "[^"]*" "[^"]*".*?\n\t\t\)', blk, re.S)
            body = re.sub(r'\(property "[^"]*" "[^"]*".*?\n\t\t\)\n', '', parent, flags=re.S)
            body = body.replace('(symbol "%s"' % pname, '(symbol "%s"' % name, 1)
            body = body.replace('(symbol "%s_' % pname, '(symbol "%s_' % name)
            first_sub = body.index('(symbol "%s_' % name)
            body = body[:first_sub] + '\n\t\t'.join(child_props) + '\n\t\t' + body[first_sub:]
            return body
        return blk

    def pins(self, lib, name, unit=1, style=1):
        blk = self.raw(lib, name)
        out = {}
        for sm in re.finditer(r'\(symbol "%s_(\d+)_(\d+)"' % re.escape(name), blk):
            u, s = int(sm.group(1)), int(sm.group(2))
            if u not in (0, unit) or s not in (0, style):
                continue
            sub = _block(blk, sm.start())
            for pm in re.finditer(r'\(pin (\w+) \w+\s*\(at ([-\d.]+) ([-\d.]+) ([-\d.]+)\)(.*?)\(name "([^"]*)".*?\(number "([^"]*)"', sub, re.S):
                num, nm = pm.group(7), pm.group(6)
                out[num] = (float(pm.group(2)), float(pm.group(3)), nm)
        return out

    def embedded(self, lib, name):
        blk = self.raw(lib, name)
        return blk.replace('(symbol "%s"' % name, '(symbol "%s:%s"' % (lib, name), 1)


def rot(px, py, r):
    r %= 360
    if r == 0:
        return px, py
    if r == 90:
        return -py, px
    if r == 180:
        return -px, -py
    if r == 270:
        return py, -px
    raise ValueError(r)


def q(v):
    return round(v * 100) / 100


class Sheet:
    def __init__(self, libs, root_uuid, project, paper='A3', sheet_uuid=None, uuid=None, pwr_start=0):
        # root_uuid: uuid of the root sheet. A sub-sheet passes its own sheet_uuid (the uuid of the
        # sheet symbol in the root) and its file uuid; its symbols then live at /root/sheet.
        self.libs, self.root, self.project, self.paper = libs, root_uuid, project, paper
        self.inst_path = '/' + root_uuid + ('/' + sheet_uuid if sheet_uuid else '')
        self.file_uuid = uuid or root_uuid
        self.used = {}
        self.items = []
        self.symbols = {}
        self.labels = []
        self.sheets = []
        self.pwr_n = pwr_start

    def add(self, libid, ref, x, y, r=0, value=None, footprint='', unit=1, style=1, fields=None,
            hide_value=False, hide_ref=False, dnp=False, extra=None, mirror=None):
        lib, name = libid.split(':')
        self.used[libid] = (lib, name)
        pins = self.libs.pins(lib, name, unit, style)
        absp = {}
        for num, (px, py, nm) in pins.items():
            if mirror == 'y':
                px = -px
            elif mirror == 'x':
                py = -py
            rx, ry = rot(px, py, r)
            absp[num] = (q(x + rx), q(y - ry))
            absp.setdefault(nm, absp[num])
        self.symbols[ref] = dict(libid=libid, x=x, y=y, r=r, pins=absp, value=value or name,
                                 footprint=footprint, unit=unit, style=style, fields=fields or {},
                                 hide_value=hide_value, hide_ref=hide_ref, dnp=dnp, extra=extra or {},
                                 mirror=mirror, nums=list(pins))
        return absp

    def P(self, ref, pin):
        return self.symbols[ref]['pins'][str(pin)]

    def add_power(self, libid, net, at, r, style, valfield, show):
        self.pwr_n += 1
        ref = '#PWR%03d' % self.pwr_n
        self.add(libid, ref, at[0], at[1], r, value=net, style=style, hide_ref=True,
                 hide_value=not show, fields={'val': valfield, 'ref': (at[0], at[1], 0, None)})
        return ref

    def power(self, kind, net, at, r=0, style=1, show=True):
        self.pwr_n += 1
        ref = '#PWR%03d' % self.pwr_n
        self.add(kind, ref, at[0], at[1], r, value=net, style=style, hide_ref=True, hide_value=not show)
        return ref

    def finalize(self):
        """Add junctions where 3+ wire ends / pins meet (at least one wire), and report every pin or
        wire end that lies inside another wire segment (would connect unintentionally or silently)."""
        segs = getattr(self, 'segs', [])
        pts = set()
        for a, b in segs:
            pts.add(a); pts.add(b)
        for ref, s in self.symbols.items():
            for k in s['nums']:
                pts.add(s['pins'][k])
        for at in self.labels:
            pts.add(at)
        split = []
        for a, b in segs:
            lo_x, hi_x = sorted((a[0], b[0])); lo_y, hi_y = sorted((a[1], b[1]))
            inner = sorted([p for p in pts if p not in (a, b) and lo_x <= p[0] <= hi_x and lo_y <= p[1] <= hi_y],
                           key=lambda p: abs(p[0] - a[0]) + abs(p[1] - a[1]))
            chain = [a] + inner + [b]
            split += list(zip(chain, chain[1:]))
            if inner:
                self.splits = getattr(self, 'splits', []) + inner
        segs = split
        self.items = [i for i in self.items if not i.startswith(WIRE_HEAD)]
        for a, b in segs:
            self.items.append(WIRE_FMT % (a[0], a[1], b[0], b[1], _u()))
        self.segs = segs
        cnt = {}
        for a, b in segs:
            for p in (a, b):
                cnt[p] = cnt.get(p, [0, 0])
                cnt[p][0] += 1
        pinpts = []
        for ref, s in self.symbols.items():
            for k in s['nums']:
                pinpts.append((ref, k, s['pins'][k]))
        seen = set()
        for ref, k, p in pinpts:
            if (ref, p) in seen:
                continue
            seen.add((ref, p))
            cnt[p] = cnt.get(p, [0, 0])
            cnt[p][1] += 1
        for p, (w, n) in cnt.items():
            if w >= 1 and w + n >= 3:
                self.junction(p)
        for p in getattr(self, 'splits', []):
            owners = [r for r, k, pp in pinpts if pp == p]
            print('T at %s %s' % (p, owners))
        warn = 0
        for a, b in []:
            lo_x, hi_x = sorted((a[0], b[0]))
            lo_y, hi_y = sorted((a[1], b[1]))
            for p in cnt:
                if p in (a, b):
                    continue
                if lo_x <= p[0] <= hi_x and lo_y <= p[1] <= hi_y:
                    owners = [r for r, k, pp in pinpts if pp == p]
                    print('WARN point %s %s lies inside wire %s-%s' % (p, owners, a, b))
                    warn += 1
        return warn

    def glabel(self, name, at, angle=0):
        """Global label whose connection point is `at`; angle 0 = text to the right of the point,
        180 = to the left, 90 = up, 270 = down."""
        at = (q(at[0]), q(at[1]))
        self.labels.append(at)
        just = 'left' if angle in (0, 90) else 'right'
        self.items.append(('\t(global_label "%s"\n\t\t(shape bidirectional)\n\t\t(at %g %g %d)\n'
                           '\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n\t\t\t(justify %s)\n\t\t)\n'
                           '\t\t(uuid "%s")\n\t\t(property "Intersheetrefs" "${INTERSHEET_REFS}"\n\t\t\t(at %g %g 0)\n'
                           '\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n\t\t\t\t(hide yes)\n\t\t\t)\n\t\t)\n\t)\n')
                          % (name, at[0], at[1], angle, just, _u(), at[0], at[1]))

    def label(self, name, at, angle=0):
        """Local net label whose connection point is `at`; angle 0 = text to the right, 180 = left,
        90 = up, 270 = down."""
        at = (q(at[0]), q(at[1]))
        self.labels.append(at)
        just = 'left bottom' if angle in (0, 90) else 'right bottom'
        self.items.append(('\t(label "%s"\n\t\t(at %g %g %d)\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n'
                           '\t\t\t)\n\t\t\t(justify %s)\n\t\t)\n\t\t(uuid "%s")\n\t)\n') % (name, at[0], at[1], angle, just, _u()))

    def sheet_symbol(self, name, filename, at, size, sheet_uuid, page):
        x, y = at
        w, h = size
        self.items.append(('\t(sheet\n\t\t(at %g %g)\n\t\t(size %g %g)\n\t\t(exclude_from_sim no)\n\t\t(in_bom yes)\n'
                           '\t\t(on_board yes)\n\t\t(dnp no)\n\t\t(fields_autoplaced yes)\n\t\t(stroke\n\t\t\t(width 0.1524)\n'
                           '\t\t\t(type solid)\n\t\t)\n\t\t(fill\n\t\t\t(color 0 0 0 0.0000)\n\t\t)\n\t\t(uuid "%s")\n'
                           '\t\t(property "Sheetname" "%s"\n\t\t\t(at %g %g 0)\n\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n'
                           '\t\t\t\t)\n\t\t\t\t(justify left bottom)\n\t\t\t)\n\t\t)\n'
                           '\t\t(property "Sheetfile" "%s"\n\t\t\t(at %g %g 0)\n\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n'
                           '\t\t\t\t)\n\t\t\t\t(justify left top)\n\t\t\t)\n\t\t)\n'
                           '\t\t(instances\n\t\t\t(project "%s"\n\t\t\t\t(path "/%s"\n\t\t\t\t\t(page "%s")\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)\n')
                          % (x, y, w, h, sheet_uuid, name, x, y - 1.27, filename, x, y + h + 1.27, self.project, self.root, page))

    def wire(self, *pts):
        pts = [(q(a), q(b)) for a, b in pts]
        if not hasattr(self, 'segs'):
            self.segs = []
        for a, b in zip(pts, pts[1:]):
            if a != b:
                assert a[0] == b[0] or a[1] == b[1], ('diagonal wire', a, b)
                self.segs.append((a, b))
                self.items.append('\t(wire\n\t\t(pts\n\t\t\t(xy %g %g) (xy %g %g)\n\t\t)\n\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n\t\t(uuid "%s")\n\t)\n' % (a[0], a[1], b[0], b[1], _u()))

    def junction(self, at):
        self.items.append('\t(junction\n\t\t(at %g %g)\n\t\t(diameter 0)\n\t\t(color 0 0 0 0)\n\t\t(uuid "%s")\n\t)\n' % (q(at[0]), q(at[1]), _u()))

    def nc(self, at):
        self.items.append('\t(no_connect\n\t\t(at %g %g)\n\t\t(uuid "%s")\n\t)\n' % (q(at[0]), q(at[1]), _u()))

    def text(self, s, at, size=2.0, bold=False, justify='left'):
        b = '\n\t\t\t\t(bold yes)' if bold else ''
        self.items.append('\t(text "%s"\n\t\t(exclude_from_sim no)\n\t\t(at %g %g 0)\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size %g %g)%s\n\t\t\t)\n\t\t\t(justify %s)\n\t\t)\n\t\t(uuid "%s")\n\t)\n' % (s, q(at[0]), q(at[1]), size, size, b, justify, _u()))

    def _prop(self, name, val, at, angle=0, hide=False, justify=None):
        j = '\n\t\t\t\t(justify %s)' % justify if justify else ''
        h = '\n\t\t\t\t(hide yes)' if hide else ''
        return '\t\t(property "%s" "%s"\n\t\t\t(at %g %g %g)\n\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)%s%s\n\t\t\t)\n\t\t)\n' % (name, val, q(at[0]), q(at[1]), angle, j, h)

    def _symbol_text(self, ref, s):
        lib, name = s['libid'].split(':')
        x, y, r = s['x'], s['y'], s['r']
        f = s['fields']
        ref_at = f.get('ref', (x + 3.81, y - 1.27, 0, 'left'))
        val_at = f.get('val', (x + 3.81, y + 1.27, 0, 'left'))
        out = '\t(symbol\n\t\t(lib_id "%s")\n\t\t(at %g %g %d)\n' % (s['libid'], q(x), q(y), r)
        if s['mirror']:
            out += '\t\t(mirror %s)\n' % s['mirror']
        out += '\t\t(unit %d)\n' % s['unit']
        if s['style'] != 1:
            out += '\t\t(convert %d)\n' % s['style']
        out += '\t\t(exclude_from_sim no)\n\t\t(in_bom yes)\n\t\t(on_board yes)\n\t\t(dnp %s)\n\t\t(uuid "%s")\n' % ('yes' if s['dnp'] else 'no', _u())
        out += self._prop('Reference', ref, ref_at[:2], ref_at[2], s['hide_ref'], ref_at[3])
        out += self._prop('Value', s['value'], val_at[:2], val_at[2], s['hide_value'], val_at[3])
        out += self._prop('Footprint', s['footprint'], (x, y), 0, True)
        out += self._prop('Datasheet', '', (x, y), 0, True)
        for k, v in s['extra'].items():
            out += self._prop(k, v, (x, y), 0, True)
        seen = set()
        for num in self.libs.pins(lib, name, s['unit'], s['style']):
            if num in seen:
                continue
            seen.add(num)
            out += '\t\t(pin "%s"\n\t\t\t(uuid "%s")\n\t\t)\n' % (num, _u())
        out += '\t\t(instances\n\t\t\t(project "%s"\n\t\t\t\t(path "%s"\n\t\t\t\t\t(reference "%s")\n\t\t\t\t\t(unit %d)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)\n' % (self.project, self.inst_path, ref, s['unit'])
        return out

    def write(self, path, title=('', '', '', '')):
        libs = ''.join('\n' + self.libs.embedded(l, n) for l, n in sorted(self.used.values()))
        head = ('(kicad_sch\n\t(version 20250114)\n\t(generator "eeschema")\n\t(generator_version "9.0")\n'
                '\t(uuid "%s")\n\t(paper "%s")\n' % (self.file_uuid, self.paper))
        tb = '\t(title_block\n\t\t(title "%s")\n\t\t(date "%s")\n\t\t(rev "%s")\n\t\t(company "%s")\n\t)\n' % title
        body = head + tb + '\t(lib_symbols' + libs + '\n\t)\n'
        body += ''.join(self.items)
        body += ''.join(self._symbol_text(r, s) for r, s in self.symbols.items())
        if self.file_uuid == self.root:
            body += '\t(sheet_instances\n\t\t(path "/"\n\t\t\t(page "1")\n\t\t)\n\t)\n'
        body += '\t(embedded_fonts no)\n)\n'
        open(path, 'w', encoding='utf-8', newline='\n').write(body)
