"""Differential test of the NConsoleWnd constructor (0x10077AB0) and destructor (0x10077E80): the stock function and ours (the hook DLL's
ConstructConsole / DestroyConsole) run side by side under unicorn with the engine imports stubbed, on the same object, and the memory they
leave and the imports they call (name, `this` relative to the object, arguments) are compared. Our vtable pointers are the stock addresses
moved to g_nwBase (NWBASE), turned back before the comparison.
usage: python tools/port/emu/t_console.py   (needs build/bin/Release/NWindowDecomp.dll and build/smoke_system/NWindowDecomp.log)"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emu import *

SIZE = 0x39E8
LOG = ROOT + r'\build\smoke_system\NWindowDecomp.log'

# callee-popped bytes of the imports the two functions call
POP = {
    '??0UL2ConsoleWnd@@QAE@XZ': 0, '??1UL2ConsoleWnd@@UAE@XZ': 0, '?ConditionalDestroy@UObject@@QAEHXZ': 0,
    '?Realloc@FArray@@IAEXH@Z': 4, '?Remove@FArray@@QAEXHHH@Z': 12, '??1FArray@@QAE@XZ': 0, '?appFailAssert@@YAXPBD0H@Z': 0,
}


def our_entries():
    out = {}
    for line in open(LOG, encoding='latin-1'):
        m = re.search(r'\s([0-9a-f]{8}) (\S+)\s+ours=([0-9a-f]+)', line)
        if m:
            out[int(m.group(1), 16)] = int(m.group(3), 16)
    return out


class ConsoleEmu(Emu):
    def __init__(self, kind):
        super().__init__(kind)
        self.last_pop = 0
        self.calls = []
        self.obj = self.alloc(0x4000)

    def dispatch(self, name):
        kind, nm = name.split(':', 1)
        if kind != 'imp' or nm not in POP:
            raise RuntimeError('unstubbed ' + name)
        self.last_pop = POP[nm]
        t = self.reg(UC_X86_REG_ECX)
        args = tuple(self.stack_arg(i) for i in range(POP[nm] // 4))
        self.calls.append((nm.split('@')[0].lstrip('?'), t - self.obj, args))
        if nm == '??0UL2ConsoleWnd@@QAE@XZ':
            # the engine's constructor stores its own vtables (the stock constructor overwrites them)
            self.write32(t, 0xB0B0B0B0)
            self.write32(t + 0x28, 0xB1B1B1B1)
            return t
        if nm == '?ConditionalDestroy@UObject@@QAEHXZ':
            return 1
        if nm == '?appFailAssert@@YAXPBD0H@Z':
            raise RuntimeError('appFailAssert')
        return None

    def image(self):
        raw = bytearray(self.uc.mem_read(self.obj, SIZE))
        if self.kind == 'ours':
            for off in range(0, SIZE - 3, 4):
                v = struct.unpack_from('<I', raw, off)[0]
                if NWBASE <= v < NWBASE + 0x400000:
                    struct.pack_into('<I', raw, off, v - NWBASE + 0x10000000)
        return bytes(raw)


def run(kind, fill, fn, setup=None):
    e = ConsoleEmu(kind)
    e.uc.mem_write(e.obj, bytes([fill]) * SIZE)
    if setup:
        setup(e)
    if kind == 'stock':
        va = fn
    else:
        va = e.base + our_entries()[fn]
    ret = e.call(va, this=e.obj)
    return e, ret


def diff(a, b):
    return ['+0x%X: stock %02X ours %02X' % (i, x, y) for i, (x, y) in enumerate(zip(a, b)) if x != y][:20]


def compare(title, fn, fill, setup=None):
    s, sret = run('stock', fill, fn, setup)
    o, oret = run('ours', fill, fn, setup)
    ok = True
    if s.image() != o.image():
        ok = False
        print('  memory differs:', diff(s.image(), o.image()))
    if s.calls != o.calls:
        ok = False
        print('  imports differ:\n    stock', s.calls, '\n    ours ', o.calls)
    if (sret - s.obj if sret else 0) != (oret - o.obj if oret else 0) and sret in (s.obj, 0) and oret in (o.obj, 0):
        ok = False
        print('  return differs', hex(sret), hex(oret))
    print('%s fill %02X: %s (%d import calls, returns %s)' % (title, fill, 'SAME' if ok else 'DIFFERENT', len(s.calls),
                                                          'this' if sret == s.obj else hex(sret)))
    return ok


def filled_arrays(e):
    """The arrays the destructor walks, with elements: two mode lists, the system message parameters, the timers."""
    for off, num in ((0x3934, 3), (0x3940, 2), (0x3980, 4), (0x3990, 5)):
        e.write32(e.obj + off, e.alloc(256))
        e.write32(e.obj + off + 4, num)
        e.write32(e.obj + off + 8, num + 2)
    e.write32(e.obj + 0x392C, 1)


if __name__ == '__main__':
    good = True
    for fill in (0x00, 0xAA):
        good &= compare('constructor', 0x10077AB0, fill)
    good &= compare('destructor (empty arrays)', 0x10077E80, 0x00)
    good &= compare('destructor (arrays with elements)', 0x10077E80, 0x00, filled_arrays)
    print('ALL SAME' if good else 'DIFFERENCES')
    sys.exit(0 if good else 1)
