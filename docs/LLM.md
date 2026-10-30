# Guide for AI coding agents (LLMs)

Read this before you touch the code. It condenses `README.en.md`, `docs/PLANO.md`, `docs/PLANO_C3.md`,
`docs/CONVENCOES.md`, `docs/notas/C3_Agentes.md` and `docs/notas/C3_Standalone.md` (the first four are in
Portuguese; the two `C3_*` notes and the code comments are in English). When this file and the code disagree, the
code and the judges win.

## 1. What the code base is

A C++ rebuild of the Lineage II C3 (*Rise of Darkness*) `NWindow.dll` (the UI DLL; stock build TimeDateStamp
`0x4329071F`, VC++ 7.0 13.00.9466, md5 `f0362880cdec6eedc6b8f860a0bc9945`, ImageBase `0x10000000`, 309
classes, 311 exports). It was ported from the C2 project's source (itself ported from C1); the port tools live in
`tools/port/`. One source, two CMake targets:

| Target | Define | Role |
|---|---|---|
| `NWindowDecomp` | `NW_HOOK_MODE=1` | hook DLL loaded by the stock `NWindow.dll`; each `NW_HOOK_*` puts a `jmp ours` at a stock entry |
| `NWindowStandalone` | `NW_HOOK_MODE=0` | the standalone `NWindow.dll` (links, passes the judges, loads in the game) |

Always configure with `-DNWD_GAME_SOURCES=ON` (the default `OFF` compiles only `src/re`, with no game code).
`NWindowStandaloneProbe` (`/FORCE:UNRESOLVED`) is a diagnostic: never load it. There is **no** phase-6 feature
target in C3; `#if NW_FEATURES` blocks in `src/nwindow` are inert call sites inherited from C1 whose headers are
not in this tree (see `docs/MODIFY_INTERFACE.md`, section 2, before enabling anything like it).

Key places:

| Path | Content |
|---|---|
| `include/nwindow/`, `src/nwindow/` | decompiled game code, **one class per file**, file = class name (288 `.cpp`) |
| `include/ue2/` | Core/Engine declarations (`UE2Core.h`, `UE2Engine.h`, generated vtables in `gen/*.vt.inc`, from `tools/gen_engine_decls.py`) |
| `src/re/` | hook runtime: `NwHook.h` (macros), installer, log, ini, `NwCrtCompat.cpp`, `Prologues.gen.cpp` |
| `src/standalone/` | `NwStandalone.h` (empty hook macros, `NW_SLOT_IS`, `StockCallAt` stubs), `Classes.gen.cpp` (309 `NW_IMPLEMENT_CLASS`), `NwGlobals.cpp` (`tools/gen_globals.py`), `NwGameGuard.cpp`, `NwCrashLog.cpp`, `NWindow.def`, `NWindowPackage.cpp` (DllMain) |
| `db/` | `functions.csv` (generated, never edit) + hand-written `names.csv`, `vslots.csv`, `verify_notes.csv`, `verify_data_notes.csv`, `verify_args_notes.csv`, `morto.csv`, `signature_changed.csv`, `not_functions.csv`; port maps (`fmap.csv`, `amap.csv`, `layout.json`, `vslots.json`) |
| `analysis/` | facts from the stock binary (`classes.json`/`classes.md`: size, parent, vtable of the 309 classes; `eh.json`; `engine_vtables.txt`; `engine/*.json` layouts from Engine.u/Core.u); `analysis/c2/` the C2 equivalents; `analysis/ghidra/` (not in git) = Ghidra export, `decomp/<va>.c` and `asm/<va>.asm` per function |
| `tests/standalone/notes.csv` | accepted standalone differences, each with evidence |
| `orig/` | the user's stock binaries (not in git) |

Object model: everything is a UE2 `UObject`. `NCObject : UObject` (0x28) → `NCCmdTarget` → `NCWnd` (0xC8 bytes,
the window tree) → `NControl` (0x1D4, controls) and the C3 `NC*Wnd` windows. `NConsoleWnd : UL2ConsoleWnd`
(0x39E8 bytes, 405 vtable slots: the engine's network/UI callbacks) owns the window manager (`NCVirtualWndMain`)
and the in-game desktop `NCConsole` (0x228, `GetDesktop()`), whose `OnCreate` builds every in-game window. The
messenger (`Msn*`, `NotificationProcess`, `SwitchboardSession`, `MSNSocket`, `ChatWnd`, `MsnWnd`, ...) is a large,
fully ported, server-less subsystem. `NCWnd` virtual slots in C3: `WindowProc` 25, `OnCreate` 37, `OnDestroy` 38,
`ShowWindow` 44, `HideWindow` 45, `OnPaint` 56, `OnDrawToolTip` 57, `OnKeyDown` 59, `OnMouseMove` 61,
`OnLButtonDown/Up` 62/63, `OnRButtonDown` 64, `OnMove` 66 (C1's numbers differ: never copy a slot number across
chronicles).

## 2. How a function is (re)implemented

Almost everything is done; new work is fixes, judge notes, or the GameGuard library (out of scope by the owner's
decision). The loop, when you need it:

1. Find the function in `db/functions.csv` (`va`, `kind`, `status`, `src`, `notes`).
2. Read `analysis/ghidra/decomp/<va>.c`, then **check the assembly** (`analysis/ghidra/asm/<va>.asm`,
   `python tools/disasm.py <va|Class::Method>`): Ghidra gets parameter counts wrong (check `ret N`), mixes
   parameters with the return address and does not know vtable slots. Helpers: `tools/xrefs.py` (who references
   an address), `tools/vtable.py <Class>` (slots vs parent), `tools/engine_vtables.py <Class>` (engine virtual
   order), `tools/port/dis_ours.py` (our compiled function).
3. Write it in `src/nwindow/<Class>.cpp`, UE2 style, address in a comment, hook macro right after:

   ```cpp
   // 0x10073630 (vslot 174) - a chat line ...
   UBOOL NConsoleWnd::Say2(L2ParamStack& Params)
   {
       ...
   }
   NW_HOOK_METHOD(0x10073630, NConsoleWnd, Say2)
   ```

   Declarations carry `// 0x...` too; invented names carry `(name ours)`. Layouts: `NW_ASSERT_OFFSET(Class, Field,
   0x..)` for every known field and `NW_ASSERT_SIZE(Class, 0x..)` = the `IMPLEMENT_CLASS` size from
   `analysis/classes.md`; unknown fields are `UnknownXX` with the offset in the name. Each class has
   `NW_DECLARE_CLASS(Class, Super, Flags, StaticClassVA)` and `NW_CHECK_VTABLE(Class, StockVtableVA, Slots)`.
4. Run the cycle until green (section 4).

Hook macros (`src/re/NwHook.h`; empty in the standalone, `src/standalone/NwStandalone.h`):

| Macro | Use |
|---|---|
| `NW_HOOK_METHOD(va, Class, Method)` | virtual or non-virtual method (installs a non-virtual forwarder, so a virtual never loops through the stock vtable) |
| `NW_HOOK_METHOD_SIG(va, Class, Method, Sig)` | overloaded method, `Sig = R (Class::*)(A...)` |
| `NW_HOOK_VARARGS(va, Class, Method, Sig)` | `...` method (cdecl, `this` on the stack); must not be virtual |
| `NW_HOOK_FUNC(va, Func)` | free/static function, same convention as stock |
| `NW_HOOK_CTOR(va, Class)`, `NW_HOOK_CTOR_ARGS(va, Class, (Params), (Args))`, `NW_HOOK_DTOR(va, Class)` | constructors / destructor |
| `NW_NAKED ... NW_STOCK(va)` | a body that jumps into stock code (hook mode); in the standalone an `int3` plus a link-time marker |
| `nw::StockCall<R>` / `nw::StockCallAt<R>(va, obj, args...)`, `nw::Field<T>(obj, off)`, `nw::VirtualCall<R>(obj, slot, ...)` | scaffolding; `StockCallAt` must not survive in the standalone view (`tools/port/sa_sites.py` counts them: 0 today) |
| `NW_GLOBAL(Type, va)` | a stock global in hook mode; the standalone defines it in `src/standalone/NwGlobals.cpp` |
| `NW_SLOT_IS("<placeholder>", "<target>")` | standalone only: a vtable placeholder is a linker alias of the function the stock slot holds |

Globals are declared in two branches: `#if NW_HOOK_MODE #define GX NW_GLOBAL(T, va) #else extern T GX; #endif`.
Anything that reaches the stock image from the standalone view is a "site": `python tools/port/sa_sites.py --total`
must stay 0, and `python tools/standalone_report.py` turns link errors into a work list.

## 3. Traps (each one has bitten this project)

- **Never hook a scalar deleting destructor**: the stock one calls the (already hooked) destructor.
- **Virtual overloads are laid out in reverse declaration order** by MSVC: declare the higher slot first.
  `NW_CHECK_VTABLE` + `verify_standalone --checks source` + `tools/port/vt_audit.py` catch it.
- **A new virtual name is a new slot.** Engine placeholders are `NewVirtual<N>` (`tools/gen_engine_decls.py`); a
  different name appends a slot and shifts the vtable (the C3 console once had 455 slots instead of 405).
- **Calling conventions**: thiscall is called through `__fastcall` with a dummy `edx`; varargs members are cdecl;
  `ret N` in the stock assembly is the truth for the stack bytes. A stub declared with the wrong argument count
  corrupts the caller's stack (`db/signature_changed.csv` lists the C2 → C3 signature changes). A by-value
  non-trivial argument (`FString`, a struct) is destroyed by the callee.
- **Packet handlers** (`NConsoleWnd::Receive*`, `Add*List`) pop `L2ParamStack` values in order: pop exactly what the
  stock pops, in the same order and with the same types.
- **Register assumptions (`/GL`)**: a stock caller may keep using `ecx`/`edx`/`eax` across a direct call it "knows"
  does not touch them. `tools/regcheck.py` lists such callers: hook them together with the callee.
- **ICF folding**: identical functions share one address in the stock image. One hook serves all; the `notes`
  column shows the cases.
- **Two C runtimes in hook mode**: the stock image's static CRT and ours. Heap is bridged (GMalloc), but a
  `FILE*` the stock code opened must be read with the stock runtime (`nw::StockFread`); `tools/port/check_crt.py`
  watches it. Keep `legacy_stdio_float_rounding.obj` and `src/re/NwCrtCompat.cpp`; never use UCRT
  `_wtol/_wtoi/_ttoi/atol/atoi` (use `NWtol`/`NAtol`, `include/nwindow/NCrt.h`).
- **`__LINE__` matters**: `check()` passes its source line to `appFailAssert`, and `tools/port/verify_data.py` compares
  those constants with the stock ones; do not add or remove lines above a `check()` (keep a blank line instead).
- **Compiler flags are part of the ABI**: `/Zc:wchar_t- /GR- /EHa /arch:IA32 /Oy-`, static CRT; standalone objects
  use `/Ob1`. x87 maths: write `integer × float constant / integer` in `DOUBLE`.
- **Plain C++ in game code**: no STL, lambdas, `auto` or new templates in `src/nwindow`; UE2 types (`INT`, `DWORD`,
  `TCHAR`, `UBOOL`, `TEXT()`, `FString`, `TArray`, `appMalloc`), `guard/unguard` with the original string.
- **Faithful before pretty**: reproduce odd stock behaviour exactly; fixes are separate, annotated changes.
- **Layout is frozen**: never add a field or a new virtual slot to a stock class. Its size is registered with
  Core and the engine and stock code index into it.
- **GameGuard stays out**: `NPG*` (84 functions, `0x10144B70..0x1014C0BF`) is not ported; the standalone has the
  no-op of `NGameGuard.h` / `NwGameGuard.cpp` and the query answer of `NConsoleWnd::GameGuardQueryX`. Do not
  "complete" it.

## 4. The judges and what "green" means

`python tools/cycle.py --system <test system folder> --game-source [--standalone] [--no-install]` runs, in order,
and stops at the first failure: configure + build → smoke (`nwd_smoke.exe`, every hook `ok` in
`NWindowDecomp.log`) → `verify_hooks.py` (ret bytes, vtable slots, imports per function vs stock; accepted
differences only with a reason in `db/verify_notes.csv`) → `regcheck.py` → with `--standalone`:
`verify_standalone.py --checks source` → `NWindowStandalone` build → `verify_standalone.py --dll` →
`smoke_standalone.py` → install (unless `--no-install`) → `build_db.py` (regenerates `db/functions.csv`,
`docs/PROGRESSO.md`). `tools\port\cycle.bat` then adds `tools/port/verify_data.py`, `check_breakers.py` and
`checks.py` (prints `CHECKS: ALL OK`). Expected today: `hooks: 4970 installed, 4970 registered, 0 errors`,
`0 unexplained`, regcheck 0 problems, `verify_data` 2 differ (two known messenger constants), `CHECKS: ALL OK`;
standalone `verify_standalone` OK and `smoke_standalone` 9/9 stages.

**Green = exit code 0**, with no new compiler warning. A difference is never "fixed" by editing a notes file
without understanding it. `difftest.py` still holds the C1 cases and refuses the C3 image: do not rely on it. The
unicorn tests (`tools/port/emu/t_*.py`) are targeted differential tests; run the ones near your change.
Prerequisites: `orig/NWindow.dll`, `orig/Core.dll`, `orig/Engine.dll`; the Ghidra export (`bash
tools/ghidra/run_full.sh`, `NWD_GHIDRA_HOME`); a copy of a C3 `system` folder for `--system` (or
`NWD_CLIENT_SYSTEM`). The cycle does **not** draw the UI: visual changes need an in-game test by a human.

## 5. Adding new behaviour

There is no feature layer. If you are asked to add one, follow `docs/MODIFY_INTERFACE.md` (section 2): a new CMake
target that reuses `nwd_standalone_dll_of` with a second object library compiled with `NW_FEATURES=1`, code in
`src/features/` / `include/features/` inside `namespace nwf`, and only call sites (`#if NW_FEATURES`, `// phase 6:
...`) in decompiled code. Before switching `NW_FEATURES` on, deal with the inherited C1 call sites (they include
headers that do not exist here). Hook mode and `NWindowStandalone` must stay byte-for-byte what the judges check.

## 6. Working rules

- **Stay faithful to the original first.** Reproduce, then improve in a separate, annotated change.
- **Prove every change with the judges**; never weaken a judge or a notes file to make a change pass.
- **Small commits**, one subject each, message saying what was checked (the cycle summary line).
- Do not install into the folder the user plays from; in-game tests only with the user's consent.
- Never commit game binaries (`orig/`, `*.dll`, `*.exe`; see `.gitignore`).
- Keep the address comment and the `NW_HOOK_*` together; keep `(name ours)` until something confirms a name.
- No machine paths in tools: a folder outside the repository comes from an option or an `NWD_*` variable
  (README, "Machine paths").
- When unsure about a stock behaviour, read the assembly again rather than guessing; write down what was verified
  and what was inferred.
