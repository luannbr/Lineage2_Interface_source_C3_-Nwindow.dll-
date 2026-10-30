#!/usr/bin/env python3
"""The checks no judge of the cycle does, in one run (what the game runs found, one by one):

  check_breakers   no hooked function reaches an NW_UNPORTED() int3 (also behind a prologue, also in vtables)
  check_retval     a hook that returns nothing / a byte / not a float while a stock caller reads eax / st0
  check_stubs      a compiled call of a stub that declares other arguments than the stock `ret N` pops
  check_frames     a local struct smaller here than in the stock frame (the /GS cookie dies first)
  size_audit       the stock constructor writes past the size the header asserts
  verify_args      a constant argument of a call that the stock passes (also through a register: `push edi`, edi = 0)
                   and no call of ours matches (NewFrameCtrl(1, 0, 0) where the stock builds (0, 0, 0): a second X)
  check_crt        our code calls a stateful entry point of OUR static C runtime (fread, fopen, ...) on what the
                   stock image's runtime owns (a FILE* from the stock NL2Font::Load faulted the first wave-1 run)

Prints one line per check and `CHECKS: ALL OK` (exit 0) or the failures (exit 1). Run after tools/cycle.py: it reads
build/bin/Release/NWindowDecomp.{dll,map} and build/smoke_system/NWindowDecomp.log of this tree.

Known and accepted (the compilers differ in the small, text buffers that only get short strings): the four frames
in KNOWN_FRAMES. A new one is a lead to look at, not noise.

usage: python tools/port/checks.py"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
KNOWN_FRAMES = {"100095d0", "10014870", "10015040", "10109b10"}


def run(script: str, *args: str) -> tuple[int, str]:
    p = subprocess.run([sys.executable, str(ROOT / "tools" / "port" / script), *args], cwd=ROOT,
                       capture_output=True, text=True, errors="replace")
    return p.returncode, p.stdout + p.stderr


def main() -> int:
    fails = []

    rc, out = run("check_breakers.py")
    line = out.strip().splitlines()[-1] if out.strip() else "(no output)"
    print("check_breakers:", line)
    if rc:
        fails.append(("check_breakers", out))

    rc, out = run("check_retval.py")
    print("check_retval:  ", out.strip().splitlines()[-1] if out.strip() else "(no output)")
    if rc:
        fails.append(("check_retval", out))

    rc, out = run("check_stubs.py")
    m = re.search(r"^(\d+) compiled call site", out, re.M)
    calls = int(m.group(1)) if m else -1
    print("check_stubs:   ", [l for l in out.splitlines() if "stubs," in l][:1], "compiled calls of a mismatching stub:", calls)
    if calls != 0:
        fails.append(("check_stubs", out))

    rc, out = run("check_frames.py")
    rows = re.findall(r"^\s+([0-9a-f]{8}) (\S+)", out, re.M)
    new = [r for r in rows if r[0] not in KNOWN_FRAMES]
    print("check_frames:  ", "%d flagged, %d new" % (len(rows), len(new)))
    if new:
        fails.append(("check_frames", "\n".join(l for l in out.splitlines() if any(l.strip().startswith(n[0]) for n in new))))

    rc, out = run("size_audit.py")
    print("size_audit:    ", out.strip().splitlines()[-1] if out.strip() else "(no output)")
    if rc:
        fails.append(("size_audit", out))

    rc, out = run("check_crt.py")
    print("check_crt:     ", out.strip().splitlines()[-1] if out.strip() else "(no output)")
    if rc:
        fails.append(("check_crt", out))

    rc, out = run("verify_args.py", "--smoke", "--quiet")
    print("verify_args:   ", out.strip().splitlines()[-1] if out.strip() else "(no output)")
    if rc:
        diff_lines = [l for l in out.splitlines() if l.startswith("DIFF")]
        fails.append(("verify_args", chr(10).join(diff_lines) or out))

    if fails:
        print("CHECKS: %d FAILED" % len(fails))
        for name, text in fails:
            print("--- %s" % name)
            print("\n".join(text.strip().splitlines()[:30]))
        return 1
    print("CHECKS: ALL OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
