# C3 stage 5: the standalone NWindow.dll - working guide

Written in English on purpose (agents read it better). Read `docs/notas/C3_Agentes.md` (sections 3, 5, 6 and 6b..6f) first: the judges,
the git rules and the classes of bug that killed the game are the same. The model is the C4 project, which forked from this tree (its tag
`c3-base`) and did this stage to the end (its standalone logs in and enters the world): `<C4 project>`,
`docs/notas/C4_Standalone.md` (read-only for you; its addresses are C4's, never copy one). The C2 project's story and decisions are in
`docs/notas/C2_Fase5.md` (sections "Decisoes", "nProtect e NKeyCrypt no standalone" and "Resultado").

## 0. What the goal is
Abandon the original `NWindow.dll` completely: a `NWindow.dll` built only from our source (`NW_HOOK_MODE=0`, no hook runtime, nothing of
the stock image in memory), loaded by the game in place of the stock one. Decisions that stay (owner, 2026): **GameGuard (`NPG*`, the
nProtect static library, 84 functions at `0x10144B70..0x1014C0BF`) is out**: in the standalone it is the documented no-op of
`include/nwindow/NGameGuard.h` / `src/standalone/NwGameGuard.cpp`, except the answer to the engine's GameGuard query (section 2c);
NKeyCrypt is real code with `LoadLibrary`/`GetProcAddress` by ordinal (`src/nwindow/NKeyCrypt.cpp`, already in this tree).

Pipeline, in order: (1) every file compiles with `NW_HOOK_MODE=0` (**done at s5-prep, all 296 objects**), (2) the link closes with 0
unresolved symbols (the lanes' work), (3) `tools/verify_standalone.py` (PE, imports, resources, exports, classes, vtables vs the stock) and
`tools/smoke_standalone.py` (loads outside the game, the same output as the stock) pass, (4) differential tests against the stock (the C3
emulator harness has no standalone mode yet: C4's `tools/port/emu/sa.py` is the model), (5) the owner tests it in the game (only the
coordinator installs: `<C3 client>\system_nwtest`, never a `system` folder).

## 1. The state you start from (s5-prep, branch `agent/sa-prep`)
- The hook build is unchanged in behaviour: 4,970 hooks, `hooks: 4970 installed, 4970 registered, 0 errors`, `verify 0 unexplained`,
  regcheck 0 problems, `CHECKS: ALL OK`. **Your branch must keep it that way.** `verify_data` shows `2 differ` since `c3-final`
  (`NMsnSock::ReadLine` 0x10141210, `MsnDecipherProduct` 0x10144650: constants of the messenger; not from stage 5 - the msn lanes may note them).
- **The standalone objects compile, all of them** (`cmake --build build --config Release --target nwd_standalone_objs`), and the
  standalone link (`NWindowStandalone`) fails only on sites: **383 unresolved symbols** - `NW_STOCK` 191, `nw::StockCallAt` 177 (one symbol
  per signature; 900+ call sites), `NW_GLOBAL` 14 (13 stock globals + `GShortCutChatKey`), `nw::Addr` 1 (85 call sites).
- The `/FORCE:UNRESOLVED` probe (`python tools/standalone_report.py --probe`, never loaded) already proves: PE 12 ok, resources 7 ok,
  exports 315 ok (the 311 names + checks), **classes 309/309 registered exactly like the stock**, check_internal_ctors 309 registrations
  identical, 306 of 307 compiled InternalConstructors push the stock constants. What it does not pass yet: vtables (517 slots in 303 vtables
  still point to an int3 placeholder, mostly inherited ones), 32 imports (unreachable code), and the differences of section 2d.
- What prep did (the C4 `s5-prep` equivalent, retargeted to C3; see the commit log of `agent/sa-prep`):
  * `src/standalone/NwStandalone.h`: `StockCallAt` / `TraceCall` / `StockFread` for the standalone, `NW_SLOT_IS`; **UClass stays 0x520 bytes
    in C3** (Core's native constructor 0x10134550 touches nothing past +0x51C; C4 is 0x530), `UField::SuperField` +0x28 (C4: +0x34).
  * the console vtable (section 2c), the GameGuard answer (section 2c), `NwCrashLog.cpp` (the fixed one: no re-entry, the caller of an eip
    outside every module), the 12 default constructors the InternalConstructor needs (`check_internal_ctors.py`: 309 OK),
    `Classes.gen.cpp` / `NWindow.def` regenerated for the 309 classes / 311 exports of C3 (they were the C2 ones), `NMsnCommand.h`'s struct
    renamed `NMsnSessionCommand` (two different `NMsnCommand` structs could not meet in `Classes.gen.cpp`).
  * tools: `tools/port/sa_sites.py` (the work list), `tools/port/vt_audit.py` (our vtable layout against the header slot numbers),
    `verify_standalone.py` (int3-stub slots apart, probe name, tail-jump slots, comment lines), `standalone_report.py` (StockCallAt, probe
    verification), `smoke_standalone.py` / `nwd_smoke_standalone.cpp` (NULL-vptr diagnostic, `SuperOff`), `gen_globals.py` (problem list,
    `floats` / `fstrings` kinds, reach report), `hdr.py` (`__declspec` names, dead `#if` branches, upper-case addresses: what vt_audit
    reads), `gen_engine_decls.py` (unnamed pure slots are `NewVirtual<N>`). `sa_sites.py` also counts a `#define X NW_GLOBAL(...)` /
    `nw::Addr(...)` the standalone view still sees (C4's skipped every `#define`: 16 sites were invisible).

### The work list (`python tools/port/sa_sites.py`)
A site is one place the standalone view of the source still reaches the stock image: an unresolved symbol or an `int3` it would run.

| construct (standalone view) | sites | what it is | what it must become |
|---|---:|---|---|
| `nw::StockCallAt<R>(va, this, ...)` | 757 | thiscall into the stock image | a typed call of OUR function that holds that address |
| `NW_NAKED ... NW_STOCK(_AT)(va)` | 238 (112 of them `NewVirtual<N>` placeholders of `NNewVirtuals.gen.cpp`) | a body that jumps into the stock image | a real body, or `NW_SLOT_IS` |
| `nw::Addr(va)` | 102 | a stock function pointer or a stock table | our function pointer / our table |
| `NW_GLOBAL(T, va)` | 16 | a stock global variable | our global (`NwGlobals.cpp`, `gen_globals.py`) |
| `NW_UNPORTED` / `_NORET` | 9 | a `__debugbreak()` where something was never ported | the real code, or proof that nothing reaches it |
| **total** | **1,122 in 180 files** | | |

The targets (distinct addresses): 724 - `done` 662, `data/none` 25, `dynamic` 21, `lib` 16. **Every `StockCallAt` and `NW_STOCK` target is `done`** (a function we already have:
the work is mechanical, but check each convention); the rest are `lib` (the stock image's import thunks: winsock, CRT, `NPK*`),
`data/none` (stock tables, globals) and `dynamic` (an address in a macro or a variable). There is no `todo` target.

## 2. The method for each construct
1. **Find our function** that holds the address: `python tools/port/sa_sites.py --files <file> --list` (the `done` name), then
   `grep -rn "0x1005D350" src include` (the `NW_HOOK_METHOD(va, Class, Name)` / `NW_HOOK_FUNC(va, Name)` / `NW_HOOK_CTOR` line) or
   `db/functions.csv` (`src` column). It is a method, a free function, a constructor/destructor, or a forwarder that itself sits on a
   placeholder (then follow it to the real body). Addresses in this tree are written both ways (`0x1005d350` / `0x1005D350`): grep both.
2. **Check the convention before you convert** (this is where the game died before): the stock `ret N` (`analysis/ghidra/asm/<va>.asm`)
   against the parameters of our function; `this` in ecx; stdcall vs cdecl for free functions; a by-value non-trivial parameter
   (`FString`, a struct) is **destroyed by the callee**, so a typed call that copies it twice destroys it twice. Once a site is a typed call
   the compiler checks the count, you check the meaning.
3. **Convert in both modes** unless the target really stays in the stock image (only the GameGuard entry points, already behind
   `NGameGuard.h`). A call of our own function from our code is the same call in the hook build (the stock entry jumps to our function
   anyway), and the hook judges then check it. If a site cannot be shared, split it with `#if NW_HOOK_MODE ... #else ... #endif`; the hook
   branch keeps the old, verified code. A hooked function whose code changes shows up as an "explained" difference: add a specific line to
   `db/verify_notes.csv` (and `db/verify_data_notes.csv` when verify_data moves).
4. **Template out-of-line copies**: a `StockCallAt` target that is the stock's non-inlined copy of a template member (`TArray::Remove`,
   `Empty`, a map's `Rehash`): call the member; if the stock copy differs from the template (growth rule, hash size), port the copy.
5. **`NW_STOCK` placeholders** (a method that exists in the header only so the vtable has a slot): keep the hook line, give the standalone
   a real body or, when the stock slot holds a function we have under another name, `NW_SLOT_IS` (section 2b). A 1-byte `ret` or a `ret N`
   that pops arguments is NOT the same as `{}` for the callers.
6. **Never** leave an `int3`/`__debugbreak` body that something can reach; never invent behaviour: what you cannot prove is reported.

## 2b. Recurring patterns (from C4; the C3 sites are the same kinds)
- **`NewStock<T>` / `StockCallAt<T*>(Ctor, Mem, ...)`** (a stock constructor run on memory we allocated): construct with our C++
  constructor (`new (Mem) T(args)` after the same allocation). Check the class's constructor is a real body (`NW_HOOK_CTOR(va, T)` with
  code), not a forwarder to the stock constructor; if it is, porting it is the site's real work.
- **`*(void**)this = nw::Addr(0x101....)`** (a stock vtable written by hand in a hooked constructor; `NConsoleWnd.cpp` 0x10185F80 /
  0x10185F7C): hook build only (`#if NW_HOOK_MODE`); in the standalone the compiler writes our vtable, and `vt_audit` /
  `verify_standalone --checks vtables` prove it equals the stock one slot by slot.
- **`(UClass*)nw::Addr(0x102....)`** (a stock `PrivateStaticClass`): `T::StaticClass()` of the class at that address
  (`analysis/classes.json`: `static_class` -> `cpp`).
- **`lib` targets** (`send`, `recv`, `connect`, `WSAAsyncSelect`, `htonl`, `closesocket`, `InternetOpenW`..., the CRT): call the real import
  (`ws2_32`, `wininet`, `shlwapi` are on the standalone link line). The 32 imports `verify_standalone --checks imports` lists today are the
  ones whose callers are still stubs: they come back as the sites close.
- **`dynamic` targets** (an address in a macro or a variable, e.g. `MsnWnd.h`'s helpers, `NotificationProcess.cpp`'s handler tables):
  resolve to the method it names and call that.
- **`Addr` of a table** (`0x101AB164`, `0x101A98CC`, `0x1019FE2C`, ...): a stock data table: our copy (`gen_globals.py` SPEC entry, or a
  `static const` in the file when it is small and read-only).
- **A vtable slot that must hold the stock function itself** (`verify_standalone --checks vtables` compares, slot by slot, the function name
  and `ret N`): in the standalone branch write `NW_SLOT_IS("<decorated placeholder>", "<decorated target>")` (`src/standalone/NwStandalone.h`;
  the decorated names are in `build\standalone_probe\Release\NWindow_probe.map`); the linker makes the placeholder an alias. A forwarder
  `void NewVirtualN() { Member(); }` is a different function with its own frame and loses the member's result in eax.
- **A stand-in struct (a function holder) is declared ONCE**, in a header every caller can include. Two definitions of one name break
  `Classes.gen.cpp` and `tools/port/vt_audit.py` (its translation unit includes every header). Today one pair is left: `NMsnGroup`
  (`NMsnGroup.h` and `NMsnProtocol.h`); vt_audit skips `NMsnProtocol.h` and its includers until sa-msn-proto merges them.

## 2c. C3-specific: the console vtable and the GameGuard query (done in prep)
- **The console's vtable was not the stock one.** `include/ue2/gen/UL2ConsoleWnd.vt.inc` named the engine's unnamed pure slots
  `PureSlot<N>`, while `NConsoleWnd` declared `NewVirtual<N>`: a different name is a NEW virtual, appended at the end, so the compiled
  `NConsoleWnd` had 455 slots for the stock's 405 and its own slots 390..404 (RequestServerList .. RequestPledgePower) sat at 440..454.
  The hook build never noticed (the hooked constructor writes the stock vtable by hand, and nothing of ours calls those slots), the
  standalone would have. Fixed as C4 fixed it: `tools/gen_engine_decls.py` names them `NewVirtual<N>`, 28 placeholders
  (`NewVirtual166` .. `NewVirtual389`, `NNewVirtuals.gen.cpp`) for the slots NConsoleWnd implements with a non-virtual `C3Virtual<N>` (and
  `IsAttackKeyDown`, slot 297), the duplicate `NewVirtual204` removed (`HideTrainWnd` is slot 204; also `NCTree`'s three dead `NewVirtual69..71` stubs, which nothing declared virtual), and the C3 signatures of slots 29
  (`GameGuardQuery`, five values, ret 0x14), 209 (`AddVIPItemList`, six, ret 0x18) and 210 (`AddVIPNeedItemList`, four, ret 0x10) in
  `tools/ue2_data/l2consolewnd_slots_c3.csv`. `python tools/port/vt_audit.py`: 308 classes checked, 0 differ (the messenger protocol headers skipped, section 2b). **The lane that owns the
  console slots turns each `NewVirtual<N>` placeholder whose function exists into `NW_SLOT_IS(...)` - and checks the convention first**:
  most `C3Virtual<N>` end in a tail jump (166 -> 0x10102BA0, 168 -> 0x10102B30, 206 -> vslot +0xB0, 270 -> 0x10053830, 291 -> 0x10139670,
  293 -> 0x10131530, 375 -> 0x100A4DE0, 385/386 -> a vslot), so the arguments the slot really takes are the tail target's: several are
  declared `()` here although the engine passes values (read the engine caller or the target's `ret N` before writing the alias).
- **The GameGuard query.** The engine asks the console (slot 29, `NConsoleWnd::GameGuardQuery`, 0x10061D00, five values, ret 0x14) from four
  places of the C3 `Engine.dll`, by `FL2NetNotify` slot 371 / `UGameEngine::OnGameGuardQuery` (`mov ecx,[ecx+0x34]; jmp [eax+0x74]`):
  kind 3 from the login server's Init (`AC_PROTOCOL`, 0x103F54EB, **only when the Init's protocol is above 0x9FD4**), kind 4 from its
  GGAuth (`AC_GAMEGUARD`, 0x103F56EF), kind 1 from CharacterSelected (0x103F84C6), kind 0 from the game server's GameGuardQueryPacket
  (0x104030D2). The stock sends the four values to GameMon (`NPGameLibApi::SendAuthQuery` 0x10144DC0) and keeps the kind at 0x1027BA70;
  GameMon's answer comes back through the message handler 0x100623C0 (message 0x3F9, case 0x10062442; only the library calls it), which by
  the kind replies to the game server (kind 0, NetHandler slot 267 `ReplyGameGuardQuery`), keeps the four dwords at `Engine +0x358..+0x364`
  (1..4), sends AuthGameGuard (kind 3, NetHandler slot 72 `ResponseAuthGameGuard`, which writes packet 7 with the session id and those
  dwords, 0x103ECEA0) or asks for the login (kind 4, slot 71 `RequestLogin`). In the stock image GameMon is started by the library's PreInit
  from a static initializer (0x1016AFB5 -> 0x10062990 -> 0x10144BF0, `CreateProcessA`), so the hook build has it; the standalone has no
  library. C4's login stalled exactly there. **C3 now answers in the standalone** (`NConsoleWnd::GameGuardQueryX`, `NConsoleWndC2.cpp`:
  the same cases, an answer of zeros; slot 29 is `NW_SLOT_IS` to `GameGuardQueryX`); the hook build is unchanged. With the Lucera C3 server it
  is dormant: that server sends login protocol 0x785A (its `L2LoginClient` accepts `RequestAuthLogin` in state CONNECTED for C3), so the
  engine never asks kind 3 there.

## 2d. What the probe already shows (lanes: fix or note in `tests/standalone/notes.csv`)
- vtables: `NCBroadcastObserverBuffWnd` slot 25 (stock `NControl::WindowProc`, ours the NCAbnormalStatusWnd one), `NCJHtmlEdit` slot 68
  (the stock's shared 8-byte tail-jump body), `NCMatchWnd` slot 45 (stock `NCWnd::HideWindow`), `NCMinimapWnd` slot 30 (stock
  `NCWnd::KillFocus`), `NConsoleWnd` slot 278 (stock `EmptyTwoArgs` 0x10003400, ours `PrivateStoreMsg {}`): C4 resolved the same kinds
  with `NW_SLOT_IS` or a note with evidence (C4 `tests/standalone/notes.csv`).
- source: `NCPrivateBuyFrameCtrl`, `NCPrivateShopFrameCtrl`, `NCRecipeShopFrameCtrl`, `NCTradeFrameCtrl` have no `NW_CHECK_VTABLE`.
- `check_internal_ctors`: `NCPrivateBuyFrameCtrl`'s compiled InternalConstructor inlines its constructor (an EH frame) where the stock one
  calls 0x100D66D0 and pushes nothing.
- imports: 32 names the stock's game code uses are not imported yet (FBlowFish, `FString(INT,INT)`, `FString(const char*)`, wininet,
  ws2_32 ...): they follow the sites; C4 explained `FString(INT,INT)`, `FString(const char*)` and `InStr(const FString&)` with notes.
- `src/standalone/NwGlobals.cpp` is still **the C2 file** (C2 addresses and initial values: wrong in silence for C3). `python
  tools/gen_globals.py` lists 16 problems today (13 globals without a standalone extern, `GShortCutChatKey` not in SPEC, two SPEC entries the
  source no longer has) and writes nothing until they are solved; then it regenerates the file from the C3 image and lists what the
  standalone still reaches by address. `gen_globals.py --check DLL MAP` reads them back from a linked DLL.

## 3. Setup and proof
Your worktree is `<worktrees>\<lane>` (branch `agent/<lane>`, made by the coordinator with
`bash tools/port/new_worktree.sh <lane>`). Work only there; each worktree has its own `build\`. Two builds:
- the hook cycle (the judge you must keep green; run it in the background, read the end of the log; never two at once in one worktree):
      cmd //c "tools\port\cycle.bat <worktrees>\<lane>" > cycle.log 2>&1
  `hooks: 4970 installed, 4970 registered, 0 errors`, `N ok, M explained, 0 unexplained`, regcheck 0 problems, `CHECKS: ALL OK`.
- the standalone objects (compile only), from a `vcvars32` shell (the cycle configures `build\` with `NWD_GAME_SOURCES=ON`):
      cmake --build build --config Release --target nwd_standalone_objs -- /m
  The first build compiles the whole set (296 objects, several minutes); then MSBuild is incremental. The compiler messages of this machine
  are in Portuguese whatever `VSLANG` says: grep the error code (`error C`), not the English text. `python tools/standalone_report.py
  [--probe]` builds `NWindowStandalone` (and the probe), groups the unresolved symbols and runs `verify_standalone` / `check_internal_ctors`
  on the probe (`build/standalone_build.log`). The link itself is the coordinator's after the merge.
Your finish line, per lane (all three, in this order):
1. `python tools/port/sa_sites.py --files "<your files, comma separated globs>" --total` prints 0 (or every remaining site is in your report
   with address, size and the reason);
2. your files compile in the standalone with no new error, and `python tools/port/vt_audit.py --class <your classes>` reports no difference;
3. the hook cycle ends as above.
Use the Edit/Write tools for files with backslashes or regexes. Never run `tools/build_db.py` alone, `tools/port/gen.py` or
`judge_loop.sh` (the cycle does what is needed).

## 4. The lanes (`db/lanes_s5.json`)
| lane | sites | files (the full list: `db/lanes_s5.json`) | what |
|---|---:|---|---|
| `sa-console-a` | 192 | `NCConsole.cpp`, `NConsoleWndMisc.cpp` | the console's stock calls, part 1 (the desktop and its ~90 windows; the console's misc functions) |
| `sa-console-b` | 188 | the other `NConsoleWnd*.cpp` (10), `NConsoleWnd.h`, the 75 `NConsoleWnd::NewVirtual` lines of `NNewVirtuals.gen.cpp` | the console's stock calls, part 2, and the console slots (section 2c; slot 278) |
| `sa-msn-proto` | 209 | `Msn*.cpp` of the protocol, `MSNSocket`, `NotificationProcess`, `SwitchboardSession`, `DispatchProcess`, `NMsn*`, `NChat*` (22 .cpp + headers) | the messenger protocol layer (63 `nw::Addr`: imports, handler tables, data); one `NMsnGroup`, one command struct |
| `sa-msn-ui` | 207 | `MsnWnd`, `ChatWnd`, `L2*`, `Friend*`, `Invite*`, `Alarm*`, the dialogs, the messenger's Loading/Login/Option/Status/Version/Task windows, `NCChatWnd`, `NCGaraDialogBox`, `NCKeyCryptWarningDialogBox` (35 .cpp + headers) | the messenger windows and dialogs |
| `sa-win-a` | 135 | game windows and helpers `NCAbnormalStatusWnd` .. `NCPartyRoomWnd` (57 .cpp + headers) | first half by name; probe vtable differences of 4 classes (section 2d) |
| `sa-win-b` | 138 | game windows and helpers `NCPartyWnd` .. `NToolTip` (53 .cpp + headers) | second half by name; the 4 FrameCtrl classes without `NW_CHECK_VTABLE`, `NCPrivateBuyFrameCtrl`'s InternalConstructor |
| `sa-globals-vslots` | 53 | `src/standalone/*`, `tools/gen_globals.py`, the other 37 `NewVirtual` lines of `NNewVirtuals.gen.cpp`, the 16 `NW_GLOBAL` lines, `tests/standalone/notes.csv`, `NGameGuard.h`, `NKeyCrypt.h` | `NwGlobals.cpp` from the C3 image (still the C2 file), the shared base-class placeholders (most of the 517 int3 vtable slots), the verification runs once the link closes |

Sizes are sites (sum 1,122). The four big lanes sit in few, long files; the window lanes are many short files. Every `StockCallAt` /
`NW_STOCK` target is a `done` function, so the size is mostly mechanical; the risk is in the conventions (section 2) and, for the messenger
lanes, in the `nw::Addr` of imports and tables.

Shared files: `include/nwindow/NConsoleWnd.h` (sa-console-b owns it; sa-console-a adds the smallest hunks it needs),
`src/nwindow/NNewVirtuals.gen.cpp` (two owners: sa-console-b the `NConsoleWnd::NewVirtual` lines, sa-globals-vslots every other line; edit
your lines in place, do not reorder, reformat or regenerate the file), the `#if NW_HOOK_MODE / #else extern` lines of the 14 `NW_GLOBAL`
sites (sa-globals-vslots, smallest hunks, in the files of other lanes: `NCMinimapWnd.cpp`, `NCChatWnd.cpp`, `ChatWnd.cpp`,
`MessengerWnd.cpp`, `MsnWnd.cpp`, `StatusWnd.cpp`, `NConsoleWndInput.cpp`). A header of another lane, `ue2` headers, `NCWnd.h`,
`NControl.h`, `NWindow.h`: the smallest hunk, listed in your report.

## 5. Rules (same as every wave)
- Never touch a game `system` folder; never install anything. The coordinator merges; you never merge or push. Never remove a worktree by
  hand (`bash tools/port/remove_worktree.sh <name>`). No Haiku-model subagents.
- Edit only the files of your lane (and the header/cpp that share their class name). Commit small and often on `agent/<lane>`, English
  messages, with the trailer the harness gives you. Commit only your files and `db/verify_notes.csv`, `db/verify_data_notes.csv`,
  `db/verify_args_notes.csv`, `db/names.csv`, `tests/standalone/notes.csv`. The cycle regenerates `db/functions.csv`, `db/verify_data.csv`,
  `db/stub_check.csv`, `db/verify_args.csv`, `docs/PROGRESSO.md`: `git checkout -- <file>` them before committing.
- The session scratchpad is shared by all agents: name every temporary file with your lane.
- A site that needs more than about half an hour of investigation: leave it, list it in the report (address, what you tried), go on.

## 6. Report (under 60 lines)
Sites converted (count by kind), sites left and why (file:line, target address, what blocked), headers changed outside your lane (file,
hunk, why), judge lines added, the last cycle summary (`hooks: ...`, `N ok, M explained, K unexplained`, regcheck, `CHECKS:`), the result of
`sa_sites.py --total`, of the standalone compile of your files and of `vt_audit.py` for your classes, and what the coordinator must know to merge.

## 7. Result (29-30/10/2026)
- The lanes merged (`agent/sa-integ`): `python tools/port/sa_sites.py --total` prints 0 and `NWindowStandalone` links with **0 unresolved
  symbols** (`build/standalone/Release/NWindow.dll`, 2,198,016 bytes).
- `tools/verify_standalone.py --dll build/standalone/Release/NWindow.dll`: pe 12 ok, imports 14 ok + 6 explained, resources 7, exports 315,
  classes 310, vtables 309 ok + 3 explained, source 3 ok + 5 explained: OK. `tools/port/check_internal_ctors.py`: 307 compiled
  InternalConstructors push the stock constants, 309 registrations identical. `tools/gen_globals.py --check`: 94 globals, 0 different.
  `tools/smoke_standalone.py`: 9/9 stages, 309 classes match, the same 438 lines as the stock `NWindow.dll`.
- The hook build on the merged tree is unchanged: hooks 4970/4970, 0 errors, 4738 ok + 232 explained, 0 unexplained, regcheck 0 problems,
  `verify_data` 2 differ (the two known messenger constants), `CHECKS: ALL OK`.
- The import notes of the probe were closed with evidence: `FString(INT,INT)` (0x101279c0, 0x1013e1c0), `FString(const char*)` (nine
  functions, the literals read at each call), `FString::InStr(const FString&)` (0x10143ed0), `GIsL2NetLogin` (read by 0x10077f20, then
  jumped over unconditionally).
- 30/10/2026: the standalone `NWindow.dll` loads in the game in place of the stock one (the owner's test in the test `System` folder).
