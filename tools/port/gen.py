#!/usr/bin/env python3
"""Generate the C2 headers and sources from the C1 ones (hook mode).

For every file of the C1 tree (include/nwindow/*.h, src/nwindow/*.cpp) writes the C2 version into
this tree. The C1 files are never changed by hand here: they are the input.

Headers (line based; only the lines the maps touch are rewritten):
  * NW_DECLARE_CLASS: the C2 UClass address;
  * fields: `// +0xOFF` moved by db/layout.json, `BYTE _UnknownXXX[n]` fillers for the bytes C2
    added, a trailing filler up to the C2 sizeof; NW_ASSERT_OFFSET / NW_ASSERT_SIZE follow;
  * methods: `// <slot> 0xVA` becomes the C2 slot (db/vslots.json) and address; a virtual that exists
    only in C2 gets an `NewVirtual<slot>()` placeholder at its place in the order.
Sources (units = everything up to an NW_HOOK_* line):
  * a unit whose function is eligible (db/eligible.csv: category A/B, every displacement explained
    by the layout, no field without evidence, no construction of a class whose constructor is not
    ported) is copied with its address, vslot comment and NW_HOOK_* address rewritten;
  * every other matched unit becomes a stub: `NW_NAKED <signature> NW_STOCK(c2 va)` (a jump into the
    stock function) or, for a constructor, a body that breaks: the class still needs its
    constructor defined so the compiled vtable exists;
  * NW_CHECK_VTABLE lines get the C2 vtable address and slot count.
Writes db/gen_report.csv: one row per unit (file, class, name, c1_va, c2_va, action, reason).

usage: python tools/port/gen.py [--only NCButton.h,NCButton.cpp ...]"""
from __future__ import annotations

import argparse
import collections
import csv
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import hdr  # noqa: E402
from common import ROOT, C1_ROOT, read_csv  # noqa: E402
from layout import load_classes  # noqa: E402

# Shared structs (declared in include/ue2) whose size changed between the base client and this one:
# name -> (size in the base header, size in the new one). A class field of such a type (or an array of it)
# is laid out by the compiler with the new size, so the filler after it shrinks by the difference.
SIZE_CHANGED = {"User": (0x1F8, 0x230)}


def _load_struct_sizes():
    """db/struct_sizes.csv: a struct embedded in a window takes its C3 size (the base's NW_ASSERT_SIZE is the old one)"""
    sp = ROOT / "db" / "struct_sizes.csv"
    if not sp.exists():
        return
    old = {}
    for h in (C1_ROOT / "include" / "nwindow").glob("*.h"):
        for m in re.finditer(r"^NW_ASSERT_SIZE\(\s*(\w+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*\)", h.read_text(encoding="utf-8", errors="replace"), re.M):
            old[m.group(1)] = int(m.group(2), 0)
    for r in csv.DictReader(l for l in sp.open(encoding="utf-8") if not l.startswith("#")):
        nm = r["name"]
        if not r["c3_size"]:
            continue
        if nm in old and int(r["c3_size"], 0) != old[nm]:
            SIZE_CHANGED[nm] = (old[nm], int(r["c3_size"], 0))


_load_struct_sizes()

HOOK = re.compile(r"^NW_HOOK_(\w+)\(\s*(0x[0-9A-Fa-f]{8})\s*,\s*([^\n]*)$")
ADDR_COMMENT = re.compile(r"^//\s*0x([0-9a-fA-F]{8})\b")
CHECK_VT = re.compile(r"^(NW_CHECK_VTABLE(?:_FOR)?)\(\s*(\w+)\s*,(\s*\w+\s*,)?\s*(0x[0-9A-Fa-f]{8})\s*,\s*(\d+)\s*\)")
GLOBAL_DEF = re.compile(r"#define\s+(G\w+)\s+NW_GLOBAL\(.*?,\s*(0x[0-9A-Fa-f]{8})\s*\)")
IDENT = re.compile(r"[A-Za-z_]\w*")
BYVALUE = re.compile(r"[(,]\s*(?:const\s+)?(?:FString|NString|FName|NStlWString|NStdString|TArray<[^>]*>)\s+\w+")


class Data:
    def __init__(self):
        self.c1 = load_classes(C1_ROOT)
        self.c2 = load_classes(ROOT)
        self.layout = json.load(open(ROOT / "db" / "layout.json", encoding="utf-8"))
        self.vslots = json.load(open(ROOT / "db" / "vslots.json", encoding="utf-8"))
        self.fmap = {}
        for r in csv.DictReader((ROOT / "db" / "fmap.csv").open(encoding="utf-8")):
            self.fmap[int(r["c1_va"], 16)] = r
        self.elig = {}
        for r in csv.DictReader((ROOT / "db" / "eligible.csv").open(encoding="utf-8")):
            self.elig[int(r["c1_va"], 16)] = r
        self.c2kind = {}
        fp = ROOT / "db" / "functions.csv"
        if fp.exists():
            for r in csv.DictReader(fp.open(encoding="utf-8")):
                self.c2kind[int(r["va"], 16)] = r["kind"]
        self.amap = {}
        self.code_amap = {}
        for r in csv.DictReader((ROOT / "db" / "amap.csv").open(encoding="utf-8")):
            self.amap[int(r["c1"], 16)] = int(r["c2"], 16)
            if r["kind"] == "code" and float(r["share"]) >= 0.9:
                self.code_amap[int(r["c1"], 16)] = int(r["c2"], 16)
        # chain of derived (no evidence) field names per class, and the global names without a map
        self.derived = collections.defaultdict(set)
        self.derived_any = set()
        for n, c in self.layout.items():
            for r in c["fields"]:
                if r["status"].startswith("derived") or r["c2"] is None:
                    self.derived[n].add(r["name"])
                    self.derived_any.add(r["name"])
        self.bad_globals = set()
        for p in sorted((C1_ROOT / "include").rglob("*.h")):
            for m in GLOBAL_DEF.finditer(p.read_text(encoding="utf-8", errors="replace")):
                if int(m.group(2), 16) not in self.amap:
                    self.bad_globals.add(m.group(1))
        # engine-parent slot counts (the engine classes are not in classes.json)
        self.engine_n1, self.engine_n2 = engine_counts(C1_ROOT), engine_counts(ROOT)
        # sizes of the engine classes (tools/ue2_engine_classes.py): the end of the base of NConsoleWnd, NL2Font
        self.engine_size = {}
        ep = ROOT / "analysis" / "engine" / "classes.json"
        if ep.exists():
            self.engine_size = {c["cpp"]: c["size"] for c in json.load(open(ep, encoding="utf-8")) if "size" in c}
        self.ctor_chain_bad = {}
        self.hooked_va2 = {}      # stock address -> the unit that hooks it (one hook per address)
        self.no_virt = set()      # abstract classes of the base headers that declare no virtual method
        self.bases = collections.defaultdict(set)   # class/struct name -> its base class names (base headers)
        base_re = re.compile(r"^(?:class|struct)\s+(\w+)\s*:\s*([^{{]+)")
        for hp in sorted((C1_ROOT / "include" / "nwindow").glob("*.h")):
            for ln in hp.read_text(encoding="utf-8", errors="replace").split("\n"):
                bm = base_re.match(ln)
                if bm:
                    for b in bm.group(2).split(","):
                        b = re.sub(r"\b(public|private|protected|virtual)\b", "", b).strip()
                        if b:
                            self.bases[bm.group(1)].add(b.split()[0])
        self.decl_manual = {}     # (class, method) -> the declaration this client's code needs (db/decl_manual.csv)
        dm_path = ROOT / "db" / "decl_manual.csv"
        if dm_path.exists():
            for r in csv.DictReader(l for l in dm_path.open(encoding="utf-8") if not l.startswith("#")):
                self.decl_manual[(r["class"], r["method"])] = r["declaration"]
        self.sig_changed = {}     # method name -> why: its argument types changed in this client
        sp = ROOT / "db" / "signature_changed.csv"
        if sp.exists():
            for r in csv.DictReader(l for l in sp.open(encoding="utf-8") if not l.startswith("#")):
                self.sig_changed[r["method"]] = r["reason"]
        self.new_virtuals = []      # (class, slot, stock va, header): virtuals only this client has
        self.header_unmapped = {}   # header -> base addresses of its macros with no counterpart here
        self.cand_mode = False
        self.allow_c = False
        self.consts = collections.defaultdict(list)    # base function va -> [(base value, this client's value)]
        cp = ROOT / "analysis" / "port" / "constants.csv"
        if cp.exists():
            for r in csv.DictReader(cp.open(encoding="utf-8")):
                try:
                    self.consts[int(r["c1_va"], 16)].append((int(r["c1_value"], 16) & 0xFFFFFFFF,
                                                           int(r["c2_value"], 16) & 0xFFFFFFFF))
                except ValueError:
                    pass
        self.allow_derived = False
        self.cand_min_sim = 0.6
        self.cand_set = set()
        self.demoted = {}
        dp = ROOT / "db" / "demote.csv"
        if dp.exists():
            for r in csv.DictReader(dp.open(encoding="utf-8")):
                self.demoted[r["c2_va"]] = r["reason"]
        self.bad_helpers = {k[7:] for k in self.demoted if k.startswith("helper:")}

    def map_literal(self, va):
        r = self.fmap.get(va)
        if r:
            return int(r["c2_va"], 16)
        return self.amap.get(va)

    def chain_derived(self, cls):
        out = set()
        while cls in self.c1:
            out |= self.derived.get(cls, set())
            cls = self.c1[cls]["super"]
        return out

    def parent_slots(self, cls):
        """(n1, n2) vtable slot counts of the parent of cls."""
        p = self.c1[cls]["super"]
        # an abstract class that declares no virtual itself (NCCmdTarget): its vtable slots are declared by the
        # class below it, so the slots it "owns" count as the child's
        while p in self.no_virt and p in self.c1:
            p = self.c1[p]["super"]
        if p in self.c1 and p in self.c2:
            return len(self.c1[p].get("vtable_slots", [])), len(self.c2[p].get("vtable_slots", []))
        return self.engine_n1.get(p), self.engine_n2.get(p)


def engine_counts(root: Path):
    out = {}
    p = root / "analysis" / "engine_vtables.txt"
    if not p.exists():
        return out
    for l in p.open(encoding="utf-8", errors="replace"):
        m = re.match(r"== \w+ \?\?_7(\w+)@@6B\w*@+ \((\d+) slots\)", l)
        if m and m.group(1) not in out:
            out[m.group(1)] = int(m.group(2))
    return out


def iv(x):
    """classes.json holds addresses as '0x...' strings (or ints in a few records)."""
    return x if isinstance(x, int) else int(x, 16)


def hexs(v, w=0):
    return "0x%0*X" % (w, v)


# ----------------------------------------------------------------------------------------------
# headers
# ----------------------------------------------------------------------------------------------
def gen_header(D: Data, path: Path, report):
    lines, cds = hdr.parse(path)
    edits = {}      # line index -> replacement text (may contain several lines)
    before = collections.defaultdict(list)   # line index -> lines inserted before it
    after = collections.defaultdict(list)    # line index -> lines inserted after it

    for cd in cds:
        n = cd.name
        if n in D.c1 and n not in D.c2:
            # a class of the base client with no counterpart here: the whole declaration (and its asserts)
            # becomes a comment, so a header that also holds classes that stay still compiles
            last = max([cd.close_line] + [a[0] for a in cd.asserts])
            for ln in range(cd.open_line - 1, last + 1):
                if ln not in edits:
                    edits[ln] = "// C3: class %s gone (no counterpart): %s" % (n, lines[ln]) if lines[ln].strip() else lines[ln]
            continue
        if n not in D.c1 or n not in D.c2:
            continue
        c1, c2 = D.c1[n], D.c2[n]
        lay = D.layout.get(n)
        vs = D.vslots.get(n)
        if cd.decl_line >= 0:
            lines_decl = lines[cd.decl_line]
            edits[cd.decl_line] = re.sub(r"(0x[0-9A-Fa-f]{8})(\s*\))", lambda m: hexs(iv(c2["static_class"]), 8) + m.group(2),
                                         lines_decl, count=1)
        # --- fields
        newoff = {}
        if lay:
            fl = sorted(cd.fields, key=lambda f: f.off)
            recs = {(r["name"], r["c1"]): r for r in lay["fields"]}
            parent = c1["super"]
            prev_end = D.c2[parent]["size"] if parent in D.c2 else D.engine_size.get(parent)
            last_line = None
            items = []
            chk = prev_end
            for f in fl:
                r = recs.get((f.name, f.off))
                if r is not None and r["c2"] is None:
                    # no place in C2 (the class was rewritten, or the field is gone): not declared
                    edits[f.line] = "%s// C2: not placed (no evidence for where it went): %s" % (f.indent, lines[f.line].strip())
                    D.derived[n].add(f.name)
                    D.derived_any.add(f.name)
                    continue
                if r is None:
                    continue
                if chk is not None and r["c2"] < chk and r["status"].startswith("derived"):
                    # no evidence for it and its derived place is taken: the field is gone in C2
                    edits[f.line] = "%s// C2: field gone or unplaced (derived +0x%X is inside the previous field): %s" % (
                        f.indent, r["c2"], lines[f.line].strip())
                    D.derived[n].add(f.name)
                    D.derived_any.add(f.name)
                    continue
                items.append((f, r))
                chk = r["c2"] + min(r["ext"], 4)
            # the compiler lays fields out in declaration order: declare them in the C2 order, in the
            # slots (lines) the C1 declarations had
            slots = [f.line for f, _ in items]
            items.sort(key=lambda it: it[1]["c2"])
            for k, (f, r) in enumerate(items):
                line = slots[k]
                off2 = r["c2"]
                nxt = items[k + 1][1]["c2"] if k + 1 < len(items) else c2["size"]
                ext2 = nxt - off2
                # a field whose type is a shared struct that changed size in this client occupies the new size
                # in the compiled class, whatever extent the base layout recorded
                rext = r["ext"]
                sz = SIZE_CHANGED.get(f.typ.strip())
                if sz and rext % sz[0] == 0:
                    rext = rext // sz[0] * sz[1]
                if f.name.startswith("_Unknown") and not f.tail and " - " in f.arr:
                    # a C1 pad: the same bytes in C2 are whatever lies between the neighbours
                    edits[line] = ("%sBYTE _Unknown%X[0x%X];  // +0x%X (a C1 pad, its C2 extent)" % (f.indent, off2, ext2, off2)
                                   if ext2 > 0 else "%s// C2: pad gone: %s" % (f.indent, lines[f.line].strip()))
                    ext_used = max(ext2, 0)
                elif ext2 < rext and rext >= 8 and ext2 > 0:
                    # an array or struct that is smaller in C2 (TCHAR[128] became an FString): the C1 type no
                    # longer fits, so the bytes are a filler and the name is unusable in copied bodies
                    edits[line] = "%sBYTE _C2Retyped_%s[0x%X];  // +0x%X C2: %s no longer fits (C1: %s)" % (
                        f.indent, f.name, ext2, off2, f.name, lines[f.line].strip().split("//")[0].strip())
                    D.derived[n].add(f.name)
                    D.derived_any.add(f.name)
                    ext_used = ext2
                else:
                    ext_used = rext
                    newoff[f.name] = off2
                    text = lines[f.line]
                    text = re.sub(r"\+0x[0-9A-Fa-f]+", "+" + hexs(off2).replace("0X", "0x").upper().replace("0X", "0x"), text, count=1)
                    if r.get("retype"):
                        text = re.sub(r"\b" + re.escape(f.typ.strip()) + r"\b", r["retype"], text, count=1)
                    if r["status"].startswith("derived"):
                        text = text.rstrip() + "   // C2: offset derived, no evidence"
                    edits[line] = text
                if prev_end is not None and off2 > prev_end:
                    # the name must not collide with a pad of the base header that kept its own (C2-offset) name
                    gap = "_Unknown%X" % prev_end
                    if any(g.name == gap for g in cd.fields):
                        gap = "_UnknownGap%X" % prev_end
                    before[line].append("%s// +0x%X (new in C2)" % (f.indent + "BYTE %s[0x%X - 0x%X]; " % (gap, off2, prev_end), prev_end))
                prev_end = off2 + ext_used
                last_line = line
            if last_line is None and prev_end is not None and c2["size"] > prev_end and cd.decl_line >= 0:
                after[cd.decl_line].append("    BYTE _UnknownTail%X[0x%X - 0x%X];  // +0x%X (new in C2, up to sizeof)" % (
                    prev_end, c2["size"], prev_end, prev_end))
            elif last_line is not None and prev_end is not None and c2["size"] > prev_end:
                after[last_line].append("    BYTE _UnknownTail%X[0x%X - 0x%X];  // +0x%X (new in C2, up to sizeof)" % (
                    prev_end, c2["size"], prev_end, prev_end))
        # --- methods
        nvirt = []
        n1p, n2p = D.parent_slots(n)
        vlines = []
        for m in cd.methods:
            t = lines[m.line]
            if m.kind == "virtual" and vs and m.slot is not None:
                s2 = vs["map"].get(str(m.slot))
                if s2 is None:
                    # no slot in the new vtable: it must not keep one in ours (a `virtual` shifts every later slot and
                    # so every virtual call the class's code makes): it stays a plain member, body and all
                    t2 = re.sub(r"//\s*\d+\s+0x[0-9a-f]{8}", "// C3: slot gone (was %d), no longer virtual" % m.slot, t, count=1)
                    t2 = re.sub(r"\bvirtual\s+", "", t2, count=1)
                    t2 = re.sub(r"\)\s*=\s*0\s*;", ");", t2, count=1)
                    edits[m.line] = t2
                    continue
                slots = c2.get("vtable_slots", [])
                va2 = slots[s2]["va"] if s2 < len(slots) and not slots[s2].get("import") else None
                new = "%d 0x%08x" % (s2, va2) if va2 else "%d" % s2
                edits[m.line] = re.sub(r"//\s*\d+\s+0x[0-9a-f]{8}", "// " + new, t, count=1)
                if n2p is not None and s2 >= n2p:
                    vlines.append((s2, m.line))
            elif m.kind in ("method", "ctor", "dtor"):
                va2 = None
                if m.kind == "ctor" and "(" in t and re.search(r"\(\s*\)", t.split("//")[0]) and c2.get("ctor"):
                    va2 = iv(c2["ctor"])
                else:
                    r = D.fmap.get(m.va)
                    if r:
                        va2 = int(r["c2_va"], 16)
                if va2:
                    edits[m.line] = re.sub(r"0x[0-9a-f]{8}", "0x%08x" % va2, t, count=1)
                else:
                    edits[m.line] = re.sub(r"(0x[0-9a-f]{8})", r"\1 (C1 only: no C2 match)", t, count=1)
        for m in cd.methods:
            ov = D.decl_manual.get((n, m.name))
            if ov is None and m.kind == "virtual":
                ov = D.decl_manual.get(("*", m.name))      # every override of a virtual whose signature changed
            if ov is not None:
                cur = edits.get(m.line, lines[m.line])
                indent = lines[m.line][:len(lines[m.line]) - len(lines[m.line].lstrip())]
                _, sep, cmt = cur.partition("//")
                edits[m.line] = indent + ov + ("   //" + cmt if sep else "")
        # --- the declaration order of the class's own new virtuals is their vtable order: a slot that moved
        # out of order (db/vslots_manual.csv) has to move in the text too. Only single-line declarations
        # are permuted (the lines of the set swap their text; the positions stay)
        if len(vlines) > 1:
            by_line = sorted(vlines, key=lambda x: x[1])
            by_slot = sorted(vlines, key=lambda x: x[0])
            if [x[1] for x in by_line] != [x[1] for x in by_slot]:
                pos = [x[1] for x in by_line]
                oneline = all(re.search(r"[;}]\s*(//.*)?$", lines[ln]) for _, ln in by_line)
                if oneline:
                    text_of = {ln: edits.get(ln, lines[ln]) for _, ln in by_line}
                    for k, (s2_, ln_) in enumerate(by_slot):
                        edits[pos[k]] = text_of[ln_]
                    vlines = [(s2_, pos[k]) for k, (s2_, ln_) in enumerate(by_slot)]
                else:
                    report.append((path.name, n, n, "", "", "note", "virtuals out of slot order and not all on one line"))
        # --- placeholders for virtuals only C2 has, inside this class's own slot range
        if vs and n2p is not None and n not in D.no_virt:
            own_new = sorted(x["slot"] for x in vs["new"] if x["slot"] >= n2p)
            if own_new:
                vl = sorted(vlines)
                for s2 in own_new:
                    va = c2["vtable_slots"][s2]["va"] if s2 < len(c2["vtable_slots"]) else 0
                    # a virtual only this client has: the stock function runs (a naked jump, the stack and the
                    # registers as they came), never a breakpoint: an object whose constructor is ours carries
                    # our vtable
                    if va:
                        txt = ("    virtual void NewVirtual%d();  // %d 0x%08x (only this client has it; defined in "
                               "NNewVirtuals.gen.cpp)" % (s2, s2, va))
                        D.new_virtuals.append((n, s2, va, path.name))
                    else:
                        txt = "    virtual void NewVirtual%d() { NW_UNPORTED(); }  // %d (not named yet)" % (s2, s2)
                    nxt = next((ln for sl, ln in vl if sl > s2), None)
                    if nxt is not None:
                        before[nxt].append(txt)
                    else:
                        # after the last virtual line, else at the end of the class body
                        anchor = max((ln for sl, ln in vl), default=None)
                        if anchor is not None:
                            after[anchor].append(txt)
                        else:
                            before[cd.close_line].append(txt)
        # --- inline member functions that name a field with no place in this client: one that cannot compile goes
        # into comments (a virtual keeps its slot: its body becomes NW_UNPORTED())
        if lay:
            unplaced = {r["name"] for r in lay["fields"] if r["c2"] is None}
            pat = re.compile(r"\b(" + "|".join(re.escape(x) for x in sorted(unplaced)) + r")\b") if unplaced else None
            i = cd.open_line + 1
            while pat and i < cd.close_line:
                cur = edits.get(i, lines[i])
                code = cur.split("//")[0].rstrip()
                one = re.search(r"\)\s*(const\s*)?\{.*\}\s*$", code)
                blk = (re.search(r"\)\s*(const\s*)?$", code) and i + 1 < cd.close_line
                       and lines[i + 1].strip() == "{")
                if one and not cur.lstrip().startswith("//"):
                    if pat.search(code[code.index("{"):]):
                        if re.search(r"\bvirtual\b", code):
                            edits[i] = re.sub(r"\{.*\}", "{ NW_UNPORTED(); }", cur, count=1) if "//" not in cur else \
                                re.sub(r"\{[^/]*\}", "{ NW_UNPORTED(); }", cur, count=1)
                        else:
                            edits[i] = "// C3: uses a field that has no place here: " + cur.strip()
                    i += 1
                    continue
                if blk and not cur.lstrip().startswith("//"):
                    j, depth = i + 1, 0
                    while j <= cd.close_line:
                        depth += lines[j].count("{") - lines[j].count("}")
                        if depth == 0:
                            break
                        j += 1
                    if any(pat.search(b.split("//")[0]) for b in lines[i + 1:j + 1]):
                        if re.search(r"\bvirtual\b", code):
                            edits[i + 1] = "    { NW_UNPORTED(); }"
                            for k in range(i + 2, j + 1):
                                edits[k] = "// C3: " + lines[k]
                        else:
                            for k in range(i, j + 1):
                                edits[k] = "// C3: uses a field that has no place here: " + edits.get(k, lines[k])
                    i = j + 1
                    continue
                i += 1
        # --- asserts
        for (ln, kind, cls, fld, val) in cd.asserts:
            if kind == "SIZE" and cls == n:
                edits[ln] = "NW_ASSERT_SIZE(%s, %s);" % (n, hexs(c2["size"]).replace("0X", "0x"))
            elif kind == "OFFSET" and cls == n:
                if fld in newoff:
                    edits[ln] = "NW_ASSERT_OFFSET(%s, %s, %s);" % (n, fld, hexs(newoff[fld]).replace("0X", "0x"))
                else:
                    edits[ln] = "// " + lines[ln] + "   // C2: field not mapped"
    out = []
    for i, t in enumerate(lines):
        out.extend(before.get(i, []))
        out.append(edits.get(i, t))
        out.extend(after.get(i, []))
    if cds and any(cd.name in D.c2 for cd in cds):
        # the original packs the classes at 4 (a DOUBLE or SQWORD at a 4-aligned offset, which C2 has
        # where C1 happened to have it 8-aligned)
        last_inc = max((i for i, t in enumerate(out[:60]) if t.startswith("#include")), default=None)
        if last_inc is not None and not any(t.startswith("#pragma pack(push, 4)") for t in out):
            out.insert(last_inc + 1, "#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets")
            out.append("#pragma pack(pop)")
    out = [remap_addresses(D, t, path.name) for t in out]
    return "\n".join(out)


# The stock addresses a header macro carries (NW_GLOBAL / ::nw::Addr / StockCall) are the BASE client's: they go
# through the address map. A literal with no counterpart in this client becomes 0xBAD00000 | its low 20 bits (the
# access then faults at once and tools/port/verify_data.py sees a global the stock function does not touch).
ADDR_CTX = re.compile(r"((?:NW_GLOBAL\([^;]*?,\s*)|(?:\bnw::Addr\()|(?:StockCall<.*?>\()|(?:NW_STOCK\())(0x[0-9A-Fa-f]{8})")


def remap_addresses(D, line, fname):
    code, sep, cmt = line.partition("//")
    if not ADDR_CTX.search(code):
        return line
    unmapped = []

    def rep(m):
        va = int(m.group(2), 16)
        new = D.map_literal(va)
        if new is None:
            new = D.code_amap.get(va)
        if new is None:
            unmapped.append(va)
            new = 0xBAD00000 | (va & 0xFFFFF)
        return m.group(1) + "0x%08X" % new

    code = ADDR_CTX.sub(rep, code)
    if unmapped:
        D.header_unmapped.setdefault(fname, []).extend(unmapped)
        cmt = " C3: no address for the base's " + ", ".join("0x%08x" % v for v in unmapped) + (" //" + cmt if cmt.strip() else "")
        sep = "//"
    return code + sep + cmt


# ----------------------------------------------------------------------------------------------
# sources
# ----------------------------------------------------------------------------------------------
def split_units(lines):
    """[(intro lines, function lines (up to and including the hook line), hook match)] + tail."""
    units = []
    start = 0
    for i, l in enumerate(lines):
        if i < start:
            continue
        m = HOOK.match(l)
        if not m:
            continue
        end = i
        depth = l.count("(") - l.count(")")
        while depth > 0 and end + 1 < len(lines):      # NW_HOOK_CTOR_ARGS spans two lines
            end += 1
            depth += lines[end].count("(") - lines[end].count(")")
        chunk = lines[start:end + 1]
        # the function starts at the last `// 0xVA` comment line before the definition
        s = None
        for k in range(len(chunk) - 2, -1, -1):
            if ADDR_COMMENT.match(chunk[k]):
                s = k
                break
        if s is None:
            # fallback: the definition ends with the last column-0 `}`; its signature follows the previous blank line
            close = max((k for k in range(len(chunk) - 1) if chunk[k].startswith("}")), default=None)
            if close is None:
                s = 0
            else:
                k = close
                while k > 0 and not chunk[k].startswith("{"):
                    k -= 1
                while k > 0 and chunk[k - 1].strip():
                    k -= 1
                s = k
        units.append((chunk[:s], chunk[s:], m))
        start = end + 1
    return units, lines[start:]


def signature_of(fn_lines):
    """The definition's head: lines after the comment block up to the opening brace (no initializer list)."""
    head = []
    for l in fn_lines:
        if l.startswith("//") or not l.strip():
            continue
        if l.startswith("{") or l.rstrip().endswith("{"):
            break
        head.append(l)
        if l.strip().endswith((")", ") const")) and not l.strip().endswith(","):
            break
    sig = " ".join(x.strip() for x in head)
    sig = re.sub(r"__declspec\(\w+\)\s*", "", sig)
    # drop a constructor initializer list
    depth = 0
    for i, ch in enumerate(sig):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                rest = sig[i + 1:].strip()
                sig = sig[:i + 1] + (" const" if rest.startswith("const") else "")
                break
    return sig


def gen_source(D: Data, path: Path, hdr_info, report, ctor_bad):
    lines = path.read_text(encoding="utf-8", errors="replace").split("\n")
    units, tail = split_units(lines)
    out = []

    # helpers of this file's intros that call a function whose signature changed (db/signature_changed.csv) or
    # another such helper: a unit that uses one is no more portable than a unit that calls the function itself
    if D.sig_changed:
        helper_re = re.compile(r"^(?:static|inline|FORCEINLINE|__forceinline|template)\b[^;{]*?\b(\w+)\s*\(")
        blocks = [i_ for i_, _, _ in units] + [tail]
        helpers = {}
        for blk in blocks:
            k = 0
            while k < len(blk):
                hm = helper_re.match(blk[k])
                if hm:
                    j = k
                    while j < len(blk) and not blk[j].startswith("}"):
                        j += 1
                    helpers[hm.group(1)] = "\n".join(blk[k:j + 1])
                    k = j + 1
                    continue
                k += 1
        changed = True
        while changed:
            changed = False
            for hname, body in helpers.items():
                if hname in D.bad_helpers:
                    continue
                code_ = re.sub(r"//[^\n]*", "", body)
                if any(re.search(r"\b" + re.escape(m_) + r"\s*\(", code_) for m_ in D.sig_changed) or \
                        any(re.search(r"\b" + re.escape(b_) + r"\b", code_) for b_ in D.bad_helpers if b_ != hname):
                    D.bad_helpers.add(hname)
                    changed = True

    def fix_vt(l):
        m = CHECK_VT.match(l)
        if not m:
            return l
        cls = m.group(2)
        c2 = D.c2.get(cls)
        if not c2 or not c2.get("vtable"):
            return "// C2: no vtable for " + l
        return "%s(%s,%s 0x%08x, %d)%s" % (m.group(1), cls, m.group(3) or "", iv(c2["vtable"]), len(c2.get("vtable_slots", [])),
                                       l[m.end():])

    def hide_helpers(block):
        """A helper of the intro that does not compile against the C2 headers goes inside #if 0."""
        if not D.bad_helpers:
            return block
        res, k = [], 0
        while k < len(block):
            l = block[k]
            fm = re.match(r"^[\w:<>\*&~ ,]+?\b(\w+::~?\w+)\s*\(", l)
            if fm and not l.startswith(("NW_", "//", "#")) and ("fn:" + fm.group(1)) in D.bad_helpers:
                # a definition the vtable needs but that does not compile: an empty body that breaks
                j = k
                while j < len(block) and not block[j].startswith("}"):
                    j += 1
                head = l.split("{")[0].rstrip()
                res.append("// C2: does not compile against the C2 headers (db/demote.csv); stub:")
                res.append(head)
                res.append("{")
                res.append("    NW_UNPORTED_NORET();")
                res.append("}")
                k = j + 1
                continue
            sm = re.match(r"^(?:struct|class)\s+(\w+)\s*$", l)
            if sm and ("struct:" + sm.group(1)) in D.bad_helpers:
                # a local struct whose members do not compile: all of it goes (its users are demoted in turn)
                j = k
                while j < len(block) and not block[j].startswith("};"):
                    j += 1
                res.append("#if 0  // C3: does not compile against the new headers (db/demote.csv)")
                res.extend(block[k:j + 1])
                res.append("#endif")
                k = j + 1
                continue
            hm = re.match(r"^(?:template\s*<[^>]*>\s*)?(?:static|inline|FORCEINLINE|__forceinline)\b[^;{]*?\b(\w+)\s*\(", l)
            if hm and hm.group(1) in D.bad_helpers:
                j = k
                while j < len(block) and not block[j].startswith("}"):
                    j += 1
                res.append("#if 0  // C2: does not compile against the C2 headers (db/demote.csv)")
                res.extend(block[k:j + 1])
                res.append("#endif")
                k = j + 1
                continue
            res.append(l)
            k += 1
        return res

    for intro, fn, hm in units:
        out.extend(remap_addresses(D, fix_vt(l), path.name) for l in hide_helpers(intro))
        kind, va1s, rest = hm.group(1), hm.group(2), hm.group(3)
        va1 = int(va1s, 16)
        args = [a.strip() for a in rest.rstrip(")").split(",")]
        cls = args[0] if kind not in ("FUNC",) else ""
        meth = args[1] if kind in ("METHOD", "METHOD_SIG", "VARARGS", "VARARGS_SLOT") and len(args) > 1 else ""
        name = ("%s::%s" % (cls, meth)) if meth else (cls + "::" + cls if kind.startswith("CTOR") else cls + "::~" + cls if kind == "DTOR" else args[0])
        # the C2 address
        va2 = None
        fm = D.fmap.get(va1)
        if fm:
            va2 = int(fm["c2_va"], 16)
        elif va1 in D.code_amap:
            va2 = D.code_amap[va1]     # no function-map pair, but the callers' operands pair it
        # a virtual: the C2 vtable slot of the class is the authority
        mi = hdr_info.get(cls, {}).get(va1)
        if mi is not None and mi.kind == "virtual" and mi.slot is not None and cls in D.vslots and cls in D.c2:
            s2 = D.vslots[cls]["map"].get(str(mi.slot))
            slots = D.c2[cls].get("vtable_slots", [])
            if s2 is not None and s2 < len(slots) and not slots[s2].get("import"):
                v = slots[s2]["va"]
                if va2 is not None and v != va2:
                    report.append((path.name, cls, name, va1s, "%08x" % va2, "note", "slot says %08x, fmap says %08x" % (v, va2)))
                va2 = v
        if kind == "CTOR" and cls in D.c2 and D.c2[cls].get("ctor"):
            va2 = iv(D.c2[cls]["ctor"])
        if cls in D.c1 and cls not in D.c2:
            # a method of a class this client does not have: no function, nothing to hook (the intro stays:
            # it may hold the file's helpers)
            report.append((path.name, cls, name, va1s, "", "skipped", "class gone in the new client"))
            continue
        ok, why = decide(D, kind, cls, va1, va2, fn, ctor_bad)
        if ok and D.sig_changed:
            body = "\n".join(fn)
            for meth_name, why_sig in D.sig_changed.items():
                if re.search(r"\b" + re.escape(meth_name) + r"\s*\(", body):
                    ok, why = False, "calls %s, whose signature changed (db/signature_changed.csv)" % meth_name
                    break
        if ok and va2 in D.hooked_va2:
            # two base functions land on one stock function here (the original build folded identical bodies): a
            # stock address takes one hook, the first one wins and the other becomes a stub of the same target
            ok, why = False, "duplicate: the stock function is shared with %s (folded identical code)" % D.hooked_va2[va2]
        if ok:
            D.hooked_va2[va2] = name
            new = []
            for l in fn:
                m = HOOK.match(l)
                if m:
                    l = "NW_HOOK_%s(0x%08x, %s" % (m.group(1), va2, m.group(3))
                elif ADDR_COMMENT.match(l) and not new:
                    pass
                new.append(l)
            # first comment line: address and vslot
            for k, l in enumerate(new):
                if ADDR_COMMENT.match(l):
                    l = re.sub(r"0x[0-9a-fA-F]{8}", "0x%08x" % va2, l, count=1)
                    if mi is not None and mi.kind == "virtual" and mi.slot is not None and cls in D.vslots:
                        s2 = D.vslots[cls]["map"].get(str(mi.slot))
                        if s2 is not None:
                            l = re.sub(r"\(vslot \d+\)", "(vslot %d)" % s2, l)
                    new[k] = l
                    break
            for k, l in enumerate(new):
                if HOOK.match(l) or l.lstrip().startswith("//"):
                    continue
                code_part, sep, cmt = l.partition("//")
                code_part = re.sub(r"\b0x(10[0-9A-Fa-f]{6})\b",
                                   lambda m: "0x%08x" % D.map_literal(int(m.group(1), 16)), code_part)
                new[k] = code_part + sep + cmt
            if D.allow_c and va1 in D.consts:
                new = patch_constants(new, D.consts[va1])
            if va2 in D.cand_set:
                new.insert(0, "// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)")
            out.extend(new)
            report.append((path.name, cls, name, va1s, "%08x" % va2, "candidate" if va2 in D.cand_set else "ported", ""))
        else:
            ov_decl = (D.decl_manual.get((cls, meth)) or D.decl_manual.get(("*", meth))) if meth else None
            stub = make_stub(kind, cls, fn, va2, va1s, mi.text if mi is not None else None, meth, cls in D.c2,
                             D.bases.get(cls, ()), decl_to_def(cls, ov_decl) if ov_decl else None)
            if stub:
                out.extend(["// 0x%08x  NOT PORTED (%s)" % (va2 or va1, why)] + stub)
            report.append((path.name, cls, name, va1s, ("%08x" % va2) if va2 else "", "stub" if stub else "skipped", why))
    out.extend(remap_addresses(D, fix_vt(l), path.name) for l in hide_helpers(tail))
    return "\n".join(out)


NUM_RE = re.compile(r"(?<![\w.])(-?\d+|0[xX][0-9A-Fa-f]+)\b")


def patch_constants(lines, pairs):
    """Category C: a value that the instruction pairs change to one other value, and that is a literal of the body
    as many times as instructions differ, takes the other value (db/constants: analysis/port/constants.csv)."""
    rows = collections.defaultdict(list)
    for a, b in pairs:
        rows[a].append(b)
    plan = {}
    for a, bs in rows.items():
        if len(set(bs)) == 1 and a != bs[0]:
            plan[a] = (bs[0], len(bs))
    if not plan:
        return lines
    count = collections.Counter()

    def val(tok):
        return int(tok, 16) if tok.lower().startswith("0x") else int(tok)   # -1 -> 0xFFFFFFFF with the mask below
    for l in lines:
        if HOOK.match(l) or l.lstrip().startswith("//"):
            continue
        for m in NUM_RE.finditer(l.partition("//")[0]):
            v = val(m.group(1)) & 0xFFFFFFFF
            if v in plan:
                count[v] += 1
    do = {v for v, (_, n) in plan.items() if count[v] == n}
    if not do:
        return lines
    out = []
    for l in lines:
        if HOOK.match(l) or l.lstrip().startswith("//"):
            out.append(l)
            continue
        code, sep, cmt = l.partition("//")

        def rep(m):
            v = val(m.group(1)) & 0xFFFFFFFF
            if v not in do:
                return m.group(0)
            y = plan[v][0]
            if y >= 0x80000000 and m.group(1).startswith("-") and y >= 0xFFFFFF00:
                return str(y - 0x100000000)             # a small negative stays a small negative
            return ("0x%X" % y) if m.group(1).lower().startswith("0x") or y >= 0x80000000 else str(y)
        out.append(NUM_RE.sub(rep, code) + sep + cmt)
    return out


def decl_to_def(cls, decl):
    """`virtual INT OnClan();  // ...` of a header -> `INT Cls::OnClan()` (no default arguments)."""
    d = decl.split("//")[0].strip().rstrip(";").strip()
    d = re.sub(r"^(virtual|static|inline)\s+", "", d)
    d = re.sub(r"\s*=\s*[^,)]+(?=[,)])", "", d)                   # default arguments
    m = re.search(r"(~?\w+)\s*\(", d)
    if not m:
        return None
    return d[:m.start()] + cls + "::" + d[m.start():]


def base_initializers(fn, bases):
    """The `Base(args)` entries of a constructor's initializer list that name a base class of the class: a
    constructor stub needs them when the base has no default constructor (members are left out: they may be gone)."""
    if not bases:
        return []
    head = []
    for l in fn:
        if l.startswith("//") or not l.strip() or l.startswith("NW_HOOK_"):
            continue
        if l.startswith("{") or l.rstrip().endswith("{"):
            break
        head.append(l.strip())
    text = " ".join(head)
    # the initializer list starts at the first `:` after the parameter list
    depth, k = 0, None
    for i, ch in enumerate(text):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == ":" and depth == 0 and text[i - 1:i + 1] != "::" and text[i:i + 2] != "::":
            k = i
            break
    if k is None:
        return []
    items, depth, cur = [], 0, ""
    for ch in text[k + 1:]:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            items.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        items.append(cur.strip())
    keep = []
    for it in items:
        m = re.match(r"^(\w+)\s*\(", it)
        if m and m.group(1) in bases:
            keep.append(it)
    return keep


def forward_args(sig):
    """The argument list that forwards a definition's own parameters to the stock function of the same signature:
    a reference becomes its address (the stock function takes the pointer). None when a parameter cannot be
    forwarded (no name, a class by value): the caller keeps the breaking stub."""
    m = re.search(r"\((.*)\)\s*(const)?\s*$", sig)
    if not m:
        return None
    text, items, depth, cur = m.group(1).strip(), [], 0, ""
    if text in ("", "void"):
        return []
    for ch in text:
        if ch in "<(":
            depth += 1
        elif ch in ">)":
            depth -= 1
        if ch == "," and depth == 0:
            items.append(cur.strip())
            cur = ""
        else:
            cur += ch
    items.append(cur.strip())
    out = []
    for it in items:
        it = re.sub(r"\s*=\s*.*$", "", it).strip()
        nm = re.search(r"(\w+)\s*(\[[^\]]*\])?$", it)
        if not nm or nm.group(2) or it == nm.group(1):
            return None
        typ = it[:nm.start()].strip()
        if typ.endswith("&"):
            out.append("&" + nm.group(1))
        elif typ.endswith("*") or re.fullmatch(r"(const\s+)?(unsigned\s+)?(INT|int|DWORD|UBOOL|BOOL|FLOAT|float|BYTE|WORD|"
                                                 r"SWORD|UINT|UBOOL|TCHAR|WCHAR|bool|char|long|ULONG|size_t|E\w+)", typ):
            out.append(nm.group(1))
        else:
            return None          # a class / struct by value
    return out


def make_stub(kind, cls, fn, va2, va1s, decl=None, meth="", is_uclass=True, bases=(), sig_override=None):
    code = [l for l in fn if l.strip() and not l.lstrip().startswith("//") and not l.startswith("NW_HOOK_")]
    if any(re.match(r"^(struct|class)\b", l) for l in fn):
        # the unit is a type definition with the destructor inside (a TArray instantiation): keep the type, drop
        # the hook; the stock destructor runs
        return [l for l in fn if not l.startswith("NW_HOOK_")]
    has_body = any("{" in l for l in fn)
    macro_def = (not has_body and meth and any(re.search(r"\b%s\b" % re.escape(meth), l) for l in code))
    if kind == "DTOR":
        if is_uclass or not has_body:
            return []      # the destructor is inline in NW_DECLARE_CLASS
        sig = signature_of(fn)
        # Not forwarded to the stock destructor: a naked destructor is refused when a member needs unwinding
        # (C3068) and a plain call would destroy the members twice. A hooked function that reaches this body is
        # found by check_breakers (the int3 sits behind the member destruction: the first game run died in the
        # NToolTipInfo constructor that way) and goes back to the stock code.
        return ["%s" % sig, "{", "    NW_UNPORTED();", "}"] if sig else []
    if sig_override:
        sig = sig_override         # the declaration changed in this client (db/decl_manual.csv)
    elif has_body:
        sig = signature_of(fn)
    elif macro_def and decl:
        sig = decl_to_def(cls, decl)         # the C1 body is a macro expansion: the header has the signature
    else:
        return []          # a hook line only: the body is defined by another unit
    if not sig:
        return []
    if kind.startswith("CTOR"):
        inits = base_initializers(fn, bases)
        fwd = forward_args(sig) if va2 is not None else None
        if fwd is not None:
            # our constructor builds the members (empty: the Core constructors only zero them) and the stock one
            # then runs over the object, so what is built is what the stock code builds; a naked constructor is
            # refused by the compiler when a member needs unwinding
            call = "::nw::StockCallAt<void>(0x%08x, this%s);" % (va2, "".join(", " + a for a in fwd))
            return ["%s" % sig] + (["    : " + ", ".join(inits)] if inits else []) + ["{", "    " + call, "}"]
        return ["%s" % sig] + (["    : " + ", ".join(inits)] if inits else []) + ["{", "    NW_UNPORTED();", "}"]
    if BYVALUE.search(sig) or va2 is None:
        # a naked function cannot take a class with a destructor by value, and with no C2 address
        # there is nothing to jump to: a definition that breaks keeps the vtable and the linker happy
        return ["%s" % sig, "{", "    NW_UNPORTED_NORET();", "}"]
    return ["NW_NAKED %s NW_STOCK(0x%08x)" % (sig, va2)]


def indiv(D: Data, va1, va2):
    """Individually eligible: matched, category A/B, every operand explained."""
    el = D.elig.get(va1)
    if va2 is None:
        return False, "no C2 counterpart"
    if D.c2kind.get(va2) in ("thunk", "crt", "eh", "fragment"):
        return False, "the C2 function is a %s (the code moved out of NWindow)" % D.c2kind[va2]
    if "%08x" % va2 in D.demoted:
        return False, "does not compile against the C2 headers: " + D.demoted["%08x" % va2]
    fm = D.fmap.get(va1)
    if D.cand_mode and fm and fm["kind"] == "game" and fm["cat"] in ("C", "D") and float(fm["sim"]) >= D.cand_min_sim             and (el is None or el["ok"] == "1"):
        # a candidate: the C2 function is not the same code, the C1 body is tried anyway and kept only if the
        # compiled result matches the stock function in every set tools/port/verify_data.py compares
        D.cand_set.add(va2)
        return True, ""
    if el is None:
        return False, "not in eligible.csv (category D or not a game function)"
    if el["ok"] != "1":
        return False, "eligible.csv: " + el["reason"]
    if el["cat"] not in ("A", "B"):
        # category C: the same instructions with other constants. The copied body keeps the base's constants:
        # tools/port/verify_data.py compares them with the stock function's, so a constant that matters shows up
        # there and the unit goes back to a stub (db/demote.csv)
        if el["cat"] == "C" and D.allow_c:
            return True, ""
        return False, "category " + el["cat"]
    return True, ""


def decide(D: Data, kind, cls, va1, va2, fn, ctor_bad):
    ok, why = indiv(D, va1, va2)
    if ok and kind.startswith("CTOR") and cls in D.ctor_chain_bad:
        return False, "a base class constructor is not ported (" + D.ctor_chain_bad[cls] + ")"
    if ok:
        ok, why = body_checks(D, cls, fn, ctor_bad)
    return ok, why


def body_checks(D: Data, cls, fn, ctor_bad):
    body = "\n".join(fn)
    code = re.sub(r"//[^\n]*", "", re.sub(r"/\*.*?\*/", "", body, flags=re.S))
    idents = set(IDENT.findall(code))
    badh = idents & D.bad_helpers
    if badh:
        return False, "uses a helper that does not compile in C2: " + ",".join(sorted(badh))
    bad = idents & D.bad_globals
    if bad:
        return False, "global without a C2 address: " + ",".join(sorted(bad))
    chain = D.chain_derived(cls)
    hit = {i for i in chain if re.search(r"(?<![\w>.])" + re.escape(i) + r"\b", code)}
    hit |= {i for i in D.derived_any if re.search(r"(->|\.)" + re.escape(i) + r"\b", code)}
    if hit and not D.allow_derived:
        return False, "field without evidence: " + ",".join(sorted(hit))
    for m in re.finditer(r"\bnew\s+(\w+)", code):
        if m.group(1) in ctor_bad:
            return False, "constructs " + m.group(1) + " (constructor not ported)"
    for m in re.finditer(r"\b0x(10[0-9A-Fa-f]{6})\b", code):
        if D.map_literal(int(m.group(1), 16)) is None:
            return False, "literal address 0x%s without a C2 counterpart" % m.group(1)
    return True, ""


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", default="")
    ap.add_argument("--candidates", action="store_true",
                    help="also copy the C1 body of matched functions that are not the same code (category C/D)")
    ap.add_argument("--min-sim", type=float, default=0.6)
    ap.add_argument("--derived", action="store_true",
                    help="copy bodies that use a field placed by the gap rule (no vote evidence); verify_data's "
                         "displacement sets decide")
    ap.add_argument("--cat-c", action="store_true",
                    help="also copy the body of category C pairs (same shape, constants differ); the judges decide")
    ap.add_argument("--out", default=str(ROOT))
    a = ap.parse_args()
    D = Data()
    D.cand_mode, D.cand_min_sim = a.candidates, a.min_sim
    D.allow_c = a.cat_c
    D.allow_derived = a.derived
    out_root = Path(a.out)
    # files written by hand from some point on (db/hand_owned.txt, one path per line, relative to the repo): the
    # generator does not write them and does not generate their units, but it knows which stock addresses they
    # hook (an address takes one hook) and which constructors they provide (derived classes may run them)
    hand_owned, hand_ctors = set(), set()
    hp = ROOT / "db" / "hand_owned.txt"
    if hp.exists():
        for ln in hp.read_text(encoding="utf-8").splitlines():
            ln = ln.strip()
            if ln and not ln.startswith("#"):
                hand_owned.add(ln.replace(chr(92), "/"))
        for rel in sorted(hand_owned):
            fp = ROOT / rel
            if rel.endswith(".cpp") and fp.exists():
                t = fp.read_text(encoding="utf-8", errors="replace")
                for m in re.finditer(r"^NW_HOOK_(\w+)\(\s*0x([0-9A-Fa-f]{8})\s*,\s*(\w+)", t, re.M):
                    D.hooked_va2.setdefault(int(m.group(2), 16), "hand-owned " + rel)
                    if m.group(1).startswith("CTOR"):
                        hand_ctors.add(m.group(3))
    # names the code of this client has by another name than the base's (db/renames.csv: old,new), whole words
    renames = []
    rp = ROOT / "db" / "renames.csv"
    if rp.exists():
        for r in csv.DictReader(l for l in rp.open(encoding="utf-8") if not l.startswith("#")):
            renames.append((re.compile(r"\b" + re.escape(r["old"]) + r"\b"), r["new"]))

    # exact text this client needs different in a generated file (db/text_patches.csv: file,old,new,note); an `old`
    # that is not there is told, not skipped silently
    patches = collections.defaultdict(list)
    tp = ROOT / "db" / "text_patches.csv"
    if tp.exists():
        for r in csv.DictReader(l for l in tp.open(encoding="utf-8") if not l.startswith("#")):
            patches[r["file"]].append((r["old"].replace(chr(92) + "n", chr(10)), r["new"].replace(chr(92) + "n", chr(10))))

    # structs the client grew (db/struct_sizes.csv: name, C3 size, bytes of padding): the asserted size changes and
    # the struct ends with padding, so no offset the ported functions use moves
    struct_sizes = {}
    sp_ = ROOT / "db" / "struct_sizes.csv"
    if sp_.exists():
        for r in csv.DictReader(l for l in sp_.open(encoding="utf-8") if not l.startswith("#")):
            if r["name"] in struct_sizes:      # a second row of the same struct: one more shift of the offsets
                struct_sizes[r["name"]][2].append((int(r["shift"], 0), int(r["from"], 0)))
            else:
                struct_sizes[r["name"]] = (int(r["c3_size"], 0), int(r["pad"], 0),
                                           [(int(r["shift"], 0), int(r["from"], 0))])

    def apply_struct_sizes(txt):
        for nm, (size, pad, shifts) in struct_sizes.items():
            if any(sh for sh, _ in shifts):
                def move(mo, shifts=shifts):
                    v = int(mo.group(3), 16)
                    tot = sum(sh for sh, frm in shifts if v >= frm)
                    return mo.group(0) if not tot else "%s0x%X);" % (mo.group(1) + mo.group(2), v + tot)
                txt = re.sub(r"^(NW_ASSERT_OFFSET\(\s*%s\s*,)(\s*\w+\s*,\s*)0x([0-9A-Fa-f]+)\s*\);" % re.escape(nm), move, txt, flags=re.M)
            m = re.search(r"^NW_ASSERT_SIZE\(\s*%s\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*\);" % re.escape(nm), txt, re.M)
            if not m:
                continue
            txt = txt[:m.start()] + "NW_ASSERT_SIZE(%s, 0x%X);" % (nm, size) + txt[m.end():]
            if pad:
                close = txt.rfind(chr(10) + "};", 0, m.start())
                if close >= 0:
                    txt = (txt[:close] + chr(10) + "    BYTE _PadC3[%d];   // this client grew the struct (db/struct_sizes.csv)" % pad
                           + txt[close:])
        return txt

    def apply_renames(txt, fname=""):
        for rx, new in renames:
            txt = rx.sub(new, txt)
        if fname.endswith(".h") and struct_sizes:
            txt = apply_struct_sizes(txt)
        for old, new in patches.get(fname, ()):
            if old in txt:
                txt = txt.replace(old, new)
            else:
                print("text_patches.csv: %s has no %r" % (fname, old[:60]))
        return txt
    only = {x for x in a.only.split(",") if x}
    hdr_files = [p for p in sorted((C1_ROOT / "include" / "nwindow").glob("*.h")) if ".gen." not in p.name]
    src_files = [p for p in sorted((C1_ROOT / "src" / "nwindow").glob("*.cpp")) if ".gen." not in p.name]

    # methods of every header by (class, c1 va), for the slot lookup of the sources
    hdr_info = collections.defaultdict(dict)
    for p in hdr_files:
        hl, cds = hdr.parse(p)
        for cd in cds:
            for m in cd.methods:
                if m.va is not None:
                    hdr_info[cd.name].setdefault(m.va, m)
            if (cd.decl_line >= 0 and "NW_DECLARE_ABSTRACT_CLASS" in hl[cd.decl_line]
                    and not any(m.kind == "virtual" for m in cd.methods)):
                D.no_virt.add(cd.name)

    # constructors: a ported constructor runs the base constructors of OUR classes, so every
    # constructor of every ancestor must be ported too; a class with a constructor that cannot
    # be ported cannot be constructed by copied code either (`new X`)
    cls_ctors = collections.defaultdict(list)     # class -> [individually eligible?]
    for p in src_files:
        lines_ = p.read_text(encoding="utf-8", errors="replace").split("\n")
        units_, _ = split_units(lines_)
        for intro, fn, hm in units_:
            if hm.group(1).startswith("CTOR"):
                c = hm.group(3).split(",")[0].strip().rstrip(")")
                va1 = int(hm.group(2), 16)
                fm = D.fmap.get(va1)
                va2 = int(fm["c2_va"], 16) if fm else None
                if hm.group(1) == "CTOR" and c in D.c2 and D.c2[c].get("ctor"):
                    va2 = iv(D.c2[c]["ctor"])
                ok, _ = indiv(D, va1, va2)
                if ok:
                    ok, _ = body_checks(D, c, fn, set())
                cls_ctors[c].append(ok)
    all_ok = {c: all(v) for c, v in cls_ctors.items()}
    for c in hand_ctors:
        all_ok[c] = True        # written by hand (db/hand_owned.txt)

    def chain_ok(c):
        """(True, '') when c and every ancestor of ours has all its constructors ported."""
        while c is not None:
            if c in all_ok and not all_ok[c]:
                return False, c
            c = D.c1[c]["super"] if c in D.c1 else None
        return True, ""

    ctor_bad = set()
    for c in set(cls_ctors) | set(D.c1):
        if not chain_ok(c)[0]:
            ctor_bad.add(c)
        parent = D.c1[c]["super"] if c in D.c1 else None
        okp, badp = chain_ok(parent)
        if not okp:
            D.ctor_chain_bad[c] = badp

    report = []
    (out_root / "include" / "nwindow").mkdir(parents=True, exist_ok=True)
    (out_root / "src" / "nwindow").mkdir(parents=True, exist_ok=True)
    # classes the base client has and this one does not (the C2's MSN window, notice, match option, recipe
    # tree...): their files are not written (and are removed from the output tree), and the includes of
    # their headers go
    dropped = {c for c in D.c1 if c not in D.c2}
    dropped_files = set()
    for p in hdr_files:
        _, cds_ = hdr.parse(p)
        if cds_ and all(cd.name in dropped for cd in cds_):
            dropped_files.add(p.stem)
    inc_re = re.compile(r'^(\s*)#include\s+"(?:nwindow/)?(\w+)\.h"', re.M)

    def drop_includes(txt):
        return inc_re.sub(lambda m: (m.group(1) + '// C3: class gone, no counterpart: #include "' + m.group(2) + '.h"')
                          if m.group(2) in dropped_files else m.group(0), txt)

    if dropped_files:
        print("classes with no counterpart, files not generated:", ", ".join(sorted(dropped_files)))
    for p in hdr_files:
        if only and p.name not in only:
            continue
        if p.stem in dropped_files:
            stale = out_root / "include" / "nwindow" / p.name
            if stale.exists():
                stale.unlink()
            continue
        if "include/nwindow/" + p.name in hand_owned:
            continue
        txt = apply_renames(drop_includes(gen_header(D, p, report)), p.name)
        if p.name == "NWindow.h":
            fwd = "".join("class %s;   // C3: gone, kept as a name for the pointers that still mention it\n" % c
                          for c in sorted(dropped))
            txt = txt.replace("#pragma warning(disable : 4291)", "#pragma warning(disable : 4291)\n\n" + fwd + "\n"
                              "// a function the C2 port has not reimplemented yet: it must never run (hook mode never reaches it)\n"
                              "#define NW_UNPORTED() __debugbreak()\n"
                              "// the same for a function that returns a value (nothing after it is reachable: no return needed);\n"
                              "// constructors use the plain one, or the optimizer drops the vtable store and the vtable with it\n"
                              "#define NW_UNPORTED_NORET() do { __debugbreak(); __assume(0); } while (0)", 1)
        (out_root / "include" / "nwindow" / p.name).write_text(txt, encoding="utf-8", newline="\n")
    for p in src_files:
        if only and p.name not in only:
            continue
        if p.stem in dropped_files:
            stale = out_root / "src" / "nwindow" / p.name
            if stale.exists():
                stale.unlink()
            continue
        if "src/nwindow/" + p.name in hand_owned:
            continue
        txt = apply_renames(drop_includes(gen_source(D, p, hdr_info, report, ctor_bad)), p.name)
        (out_root / "src" / "nwindow" / p.name).write_text(txt, encoding="utf-8", newline="\n")
    # the virtuals only this client has: the function that runs is the stock one (a naked jump, the stack and the
    # registers as they came): an object whose constructor is ours carries OUR vtable, and a slot must never hold
    # a breakpoint
    nv = list(D.new_virtuals)
    for rel in sorted(hand_owned):
        if rel.startswith("include/nwindow/") and rel.endswith(".h") and (ROOT / rel).exists():
            cur = None
            for ln in (ROOT / rel).read_text(encoding="utf-8", errors="replace").splitlines():
                m = re.match(r"^(?:class|struct)\s+(\w+)", ln)
                if m:
                    cur = m.group(1)
                m = re.match(r"^\s*virtual void NewVirtual(\d+)\(\);\s*//\s*\d+ 0x([0-9a-fA-F]{8})", ln)
                if m and cur:
                    nv.append((cur, int(m.group(1)), int(m.group(2), 16), Path(rel).name))
    lines_nv = ["// Generated by tools/port/gen.py - do not edit.",
                "// The virtuals only this client has: the stock function runs.",
                '#include "nwindow/NWindow.h"']
    for h in sorted({x[3] for x in nv}):
        lines_nv.append('#include "nwindow/%s"' % h)
    lines_nv.append("")
    seen_nv = set()
    # a placeholder a hand-written file already defines (a lane gave the slot a body) is not emitted again
    defined_nv = set()
    dre = re.compile(r"^[^\n;]*?\b(\w+)::NewVirtual(\d+)\s*\([^)]*\)\s*(?:const\s*)?(?:\{|NW_STOCK)", re.M)
    for cp in sorted((ROOT / "src" / "nwindow").glob("*.cpp")):
        if cp.name == "NNewVirtuals.gen.cpp":
            continue
        for dm in dre.finditer(cp.read_text(encoding="utf-8", errors="replace")):
            defined_nv.add((dm.group(1), int(dm.group(2))))
    for cls_, slot_, va_, h_ in sorted(nv):
        if (cls_, slot_) in seen_nv or (cls_, slot_) in defined_nv:
            continue
        seen_nv.add((cls_, slot_))
        lines_nv.append("NW_NAKED void %s::NewVirtual%d() NW_STOCK_AT(0x%08x)" % (cls_, slot_, va_))
    (out_root / "src" / "nwindow" / "NNewVirtuals.gen.cpp").write_text(
        chr(10).join(lines_nv) + chr(10), encoding="utf-8", newline=chr(10))
    with (ROOT / "db" / "gen_report.csv").open("w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(["file", "class", "name", "c1_va", "c2_va", "action", "reason"])
        w.writerows(report)
    if a.candidates:
        with (ROOT / "db" / "candidates.csv").open("w", encoding="utf-8", newline="") as f:
            w = csv.writer(f)
            w.writerow(["c2_va"])
            for v in sorted(D.cand_set):
                w.writerow(["%08x" % v])
    cnt = collections.Counter(r[5] for r in report)
    print("units:", dict(cnt))
    why = collections.Counter(r[6].split(":")[0] for r in report if r[5] != "ported")
    print("not ported because:", why.most_common(10))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
