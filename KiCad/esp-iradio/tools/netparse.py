"""Read a KiCad 'kicadsexpr' netlist: components (ref, value, footprint, fields, sheet path, uuid) and nets."""
import re


def _block(t, i):
    depth, j, ins = 0, i, False
    while True:
        c = t[j]
        if ins:
            if c == '\\':
                j += 2
                continue
            if c == '"':
                ins = False
        elif c == '"':
            ins = True
        elif c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0:
                return t[i:j + 1]
        j += 1


def read(path):
    t = open(path, encoding='utf-8').read()
    comps = {}
    i = t.index('(components')
    blk = _block(t, i)
    for m in re.finditer(r'\(comp \(ref "([^"]+)"\)', blk):
        c = _block(blk, m.start())
        ref = m.group(1)
        g = lambda pat: (re.search(pat, c, re.S) or [None, None])[1]
        fields = dict(re.findall(r'\(field \(name "([^"]+)"\) "([^"]*)"\)', c))
        comps[ref] = dict(
            ref=ref, value=g(r'\(value "([^"]*)"\)'), footprint=g(r'\(footprint "([^"]*)"\)'),
            fields=fields, sheet_names=g(r'\(sheetpath \(names "([^"]*)"\)'),
            sheet_tstamps=g(r'\(tstamps "(/[^"]*)"\)'), uuid=g(r'\)\s*\(tstamps "([0-9a-f-]{36})"\)'),
            sheetfile=g(r'\(property \(name "Sheetfile"\) \(value "([^"]*)"\)'),
            sheetname=g(r'\(property \(name "Sheetname"\) \(value "([^"]*)"\)'))
    nets = {}
    i = t.index('(nets')
    blk = _block(t, i)
    for m in re.finditer(r'\(net \(code "?\d+"?\) \(name "([^"]*)"\)', blk):
        n = _block(blk, m.start())
        nets[m.group(1)] = re.findall(r'\(node \(ref "([^"]+)"\) \(pin "([^"]+)"\)', n)
    return comps, nets
