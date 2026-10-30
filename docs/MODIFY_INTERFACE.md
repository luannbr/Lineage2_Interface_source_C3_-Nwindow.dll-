# How to modify the interface

[Português](MODIFICAR_INTERFACE.md) · **English** · [Русский](MODIFY_INTERFACE.ru.md)

A practical guide to changing the C3 interface in this source: how the windows are built, how to add a feature
layer (C3 **has none**) and step-by-step recipes (a new shortcut bar, a button, a window, a chat colour, a tool-tip
line). The two targets that exist, `NWindowDecomp` (hook) and `NWindowStandalone`, are faithful to the original and
are where the judges compare with it: every interface change goes into a **third** target that you create (section
2) and sits behind `#if NW_FEATURES`.

## 1. How the windows are built

### The hierarchy

| Class | File | Role |
|---|---|---|
| `NCObject : UObject` (0x28) | `include/nwindow/NCObject.h` | base of everything; `FindTexture(TEXT("Package.Group.Name"))` loads a texture |
| `NCWnd` (0xC8 bytes) | `include/nwindow/NCWnd.h` | every window: position (`LocalX/Y` relative to the parent, `ScreenX/Y`, `Width`, `Height` absolute), style, children, focus, title bar (`FrameCtrl`), minimised icon |
| `NControl : NCWnd` (0x1D4) | `include/nwindow/NControl.h` | every control; has an `ID` and tells its parent with `NWM_COMMAND` (wParam = `ID`) |
| `NCButton`, `NCPushButton`, ... | `include/nwindow/NCButton.h` | buttons with normal/pushed textures (`SetNormalTex`, `SetPushedTex`, `SetText`, `SetTextID`) |
| `NCEditBox`, `NCListBox`, `NCScrollWnd`, `NCComboBox`, `NCTabControl`, `NCSliderBar`, `NCListCtrl`, `NCMultiEdit`, `NCHtmlViewer`... | `include/nwindow/` | the other controls |
| `NC*Wnd` | `include/nwindow/` | the game windows (inventory, chat, shortcut bar, status...) |
| `NCVirtualWndMain` | `include/nwindow/NCVirtualWndMain.h` | owner of every window: registration, z-order, hit tests, focus, message dispatch, cursors |
| `NConsoleWnd : UL2ConsoleWnd` (0x39E8) | `include/nwindow/NConsoleWnd.h` | the engine's console (405 slots: network and UI events); `GConsole` |
| `NCConsole` (0x228) | `include/nwindow/NCConsole.h` | the in-game **desktop** (`GConsole->GetDesktop()`, `GDesktop`): parent of every in-game window, with a pointer to each (`ShortCutWnd` +0x138, `ChatWnd`...) |

### Creating a window or a control

The pattern of the original code (e.g. `NCSystemMenuWnd::OnCreate` in `src/nwindow/NCSystemMenuWnd.cpp`):

```cpp
NCButton* Button = new NCButton(TEXT("L2UI_CH3.SystemMenuWnd.systemicon1"),
                                TEXT("L2UI_CH3.SystemMenuWnd.systemicon1_down"), 0);
Button->ID = NID_SYSMENU_BUTTON_C;      // what arrives in the parent's NWM_COMMAND
Button->SetPosition(13, 29, 36, 36);    // relative to the parent
CreateChild(Button, NWS_VISIBLE);       // registers it, makes the position absolute, sends WM_CREATE -> OnCreate
```

- `new` of a package class is UE2's `operator new` (the object is a `UObject`, allocated with the size of the
  registered `UClass`).
- `CreateChild(child, style)` (`NCWnd`, inline) = `AddChildWindow` + `Entry` + screen position + `WM_CREATE`. The
  desktop registers the game windows with `AddChildWindow(window, style)`, `HideWindow()` and the inline attach
  (`AttachInline` in `src/nwindow/NCConsole.cpp`).
- Styles (`NWS_*` in `NCWnd.h`): `NWS_VISIBLE`, `NWS_TOPMOST` (separate list, on top), `NWS_TITLEBAR` (`OnCreate`
  makes an `NCFrameCtrl` title bar), `NWS_MINIMIZABLE`, `NWS_NOCLOSE`, and the C3 ones: `NWS_DRAGABLE`,
  `NWS_CLIPTOPARENT`, `NWS_NOFOCUSNEXT`...
- Destroying: in the owner's `OnDestroy`, `RemoveChildWindow(Child->Entry); delete Child;` (see
  `NCSystemMenuWnd::OnDestroy`). `NCWnd::OnDestroy` only destroys the title bar and the icon.

### Messages, painting, tool tips

- Everything arrives through `WindowProc(Msg, wParam, lParam)` (slot 25) with Win32 message numbers; `NCWnd`
  dispatches to the virtuals (**C3** slot numbers): `OnCreate` (37), `OnDestroy` (38), `ShowWindow` (44),
  `HideWindow` (45), `OnPaint(NCGDevice*)` (56), `OnDrawToolTip` (57), `OnKeyDown` (59), `OnMouseMove` (61),
  `OnLButtonDown/Up` (62/63), `OnRButtonDown` (64), `OnMove` (66, `WM_MOVE`).
- A control tells its parent with `NWM_COMMAND` (`0x501`), `wParam` = the control's `ID`; the parent handles it in
  its `WindowProc` (example: `NCSystemMenuWnd::WindowProc`).
- Painting (`OnPaint`) uses the `NCGDevice` (`include/nwindow/NCGDevice.h`): `SetClip`, `BeginPaint(ScreenX,
  ScreenY)`, `DrawTexture`, `DrawStretchedTex`, `DrawRect`, `DrawLine`, `DrawText(...)`, `EndPaint`, `ReleaseClip`.
  Textures are loaded once in `OnCreate` with `FindTexture` (the client's `L2UI_CH3.*` and `L2UI.*` packages).
- Tool tip: `OnDrawToolTip(NCGDevice*)` of the window under the mouse, called after painting.

### Console and desktop

`NConsoleWnd` (the object the engine creates from `L2.ini`) receives the server and game events (its 405 slots)
and owns the `NCConsole` desktop. `NCConsole::OnCreate` (`src/nwindow/NCConsole.cpp`) creates every game window
(positions from the screen size) and `NCConsole::OnDestroy` destroys them (`DESTROY_AT`). `NConsoleWnd::Tick`
(`src/nwindow/NConsoleWndInput.cpp`) runs every frame.

## 2. Adding a feature layer (C3 has none)

In C1 the new features live in a third target, `NWindowL2`, with `NW_FEATURES=1` and `src/features/` (see the C1
project). **None of that exists in this tree.** The `#if NW_FEATURES` blocks found in `src/nwindow`
(`NCAbnormalStatusWnd.cpp`, `NCActionItemWnd.cpp`, `NCChatWnd.cpp`, `NCEquipItemWnd.cpp`, `NCItemWnd.cpp`,
`NCommandParser.cpp`, `NConsoleWndEvents.cpp`, `NConsoleWndInput.cpp`, `NConsoleWndLogin.cpp`,
`NConsoleWndWindows.cpp`, `NCVideoOptionWnd.cpp`, `NCVirtualWndMain.cpp`, `NCWnd.cpp`; and `include/nwindow/NCVideoOptionWnd.h`) are call sites inherited
from C1 and inert: they include `features/*.h` headers that are not here. The way to build the layer on top of the
existing classes:

1. **CMake target.** In `CMakeLists.txt`, a second object library with the same sources as `nwd_standalone_objs`
   plus `src/features/*.cpp`, the same flags and `NW_FEATURES=1`, and the DLL through the function that already
   exists: `nwd_standalone_dll_of(nwd_l2_objs NWindowL2 NWindow "${CMAKE_BINARY_DIR}/l2")`. The two faithful
   targets stay without `NW_FEATURES`.
2. **The inherited sites.** Before switching `NW_FEATURES` on, decide every inherited block: delete it, or bring the
   C1 feature over and adapt it to C3 (C1 addresses, layouts and slot numbers do **not** apply here). Otherwise the
   new target does not compile.
3. **Feature code** in `src/features/<Feature>.cpp` + `include/features/<Feature>.h`, inside `namespace nwf` and
   `#if NW_FEATURES`. In decompiled code only the call site, in `#if NW_FEATURES ... #endif` with
   `// phase 6: <Feature> ...`; the faithful code must read the same without it. Mind the `__LINE__` of `check()`:
   never change the line count above a `check()` (keep blank lines, as `NConsoleWndLogin.cpp` does).
4. **Wiring points** you create yourself: `DllMain` (`src/standalone/NWindowPackage.cpp`) to install and undo what
   touches other modules; the start of `NConsoleWnd::Tick` for per-frame work (under an exception handler, as in
   C1); `NCConsole::OnCreate`/`OnDestroy` to create/destroy windows; the `WindowProc`/`OnPaint`/`OnDrawToolTip` of
   the windows the feature changes.
5. **Settings**: C3 has no feature settings reader. C1 uses the registry (`HKCU\Software\Lineage2C1\NWindow\<Section>`)
   and no file next to the DLL; do the same with a key of your own.
6. **New classes** (a window with fields of its own) need a `UClass` registered only in the new target, with the
   `autoclass<Class>` export after the stock ordinals; `tools/verify_standalone.py --l2` accepts exactly the extra
   classes of `tests/standalone/l2_extras.csv` (empty today). C1's `NW_DECLARE_L2_CLASS` / `NW_IMPLEMENT_L2_CLASS`
   macros are not in this tree; the model is `NW_IMPLEMENT_CLASS` in `include/nwindow/NWindow.h`.
7. **Proof**: the full cycle green (it proves hook mode and the standalone did not change),
   `verify_standalone.py --dll build/l2/Release/NWindow.dll --l2`, then an in-game test in a copy of the `system`
   folder.

## 3. Recipe: an extra shortcut bar (as in C5 / Interlude)

Nothing of this recipe is implemented in C3; it is the method the C1 project implemented and **tested in the game**
(in an experimental branch), adapted to the C3 facts of this source.

**The idea (C5's):** the extra bar **owns no slots**. It is one more `NCShortCutWnd` instance showing another page
of the same shortcut table. In C3 the table holds **12 × 10 = 120** shortcuts (12 per page, F1..F12, 10 pages;
`new NCShortCutWnd(12, 10, 32)` in `NCConsole::OnCreate`). A shortcut's slot is `page × 12 + position`, a flat
number the server stores and sends back; a bar that only shows another page changes nothing in the protocol, the
server or the textures. The class keeps its size (0x270) and no new `UClass` is needed.

**Steps** (in a target with `NW_FEATURES`, section 2):

1. **Feature files:** `include/features/ShortCutBars.h` and `src/features/ShortCutBars.cpp`, with functions such as
   `ShortCutBars_Create(NCConsole* Desktop, NCShortCutWnd* Main)`, `ShortCutBars_Destroy`, `ShortCutBars_OnTick`,
   `ShortCutBars_OnCommand`, `ShortCutBars_OnMove`, `ShortCutBars_OnDropOnOtherBar`, `ShortCutBars_OnPaint`. The
   state (the bars, the button) lives in globals of the `.cpp`, never in new fields of `NCShortCutWnd`.
2. **Create** in `NCConsole::OnCreate` (`src/nwindow/NCConsole.cpp`), right after the main bar (`W = new
   NCShortCutWnd(12, 10, 32)` ... `AddChildWindow(..., 0x85022)`, `HideWindow()`, the inline attach): for each extra
   bar, `new NCShortCutWnd(Main->Cols, Main->Pages, Main->IconSize)`, `SetPosition` and the same registration. The
   stock `OnCreate` made tables of its own: free them (`delete[] Bar->Items`, `delete[] Bar->Keys`) and point to the
   main bar's (`Bar->Items = Main->Items`, `Bar->Keys = Main->Keys`). `Bar->Page` = the first page shown. Hide the
   extra bars' `RotateButton` and `PadButton` (the main bar turns them all; the game-pad mode, `bPadMode`, stays on
   the main bar).
3. **Destroy** in `NCConsole::OnDestroy`, **before** `DESTROY_AT(0x138)` (the main bar): on the extra bars set
   `Items` and `Keys` to `NULL` (they belong to the main bar; otherwise the stock `OnDestroy` frees them twice), then
   `RemoveChildWindow(Bar->Entry)` and `delete Bar`.
4. **Follow the main bar** at the end of `NCShortCutWnd::Tick` (`src/nwindow/NCShortCutWnd.cpp`; only the main bar
   ticks): for each extra bar, the same orientation (`bHorizontal`, and `UpdateLayout()` when it changes), the
   position (above a horizontal bar, left of a vertical one, the other side when the screen has no room; 46 × 504
   vertical, 504 × 46 horizontal) through `SetPosition`, and shown only while the main bar is.
5. **Expand button** (optional): an `NCPushButton` child of the main bar (`CreateChild`), with a new `ID` after the
   stock ones (the bar's go up to `0xFF000009`, `NID_SHORTCUT_PAD`), handled in the `NWM_COMMAND` branch of
   `NCShortCutWnd::WindowProc`. The page arrows (`L2UI_CH3.ShortcutWnd.*`) can serve as art.
6. **Dragging between bars:** in `NCShortCutWnd::OnLButtonUp`, the stock branch that **deletes** a shortcut dropped
   off the bar (`GConsole->RequestShortCutDel`) first checks whether the point is over another bar; if so it moves
   the shortcut to the slot there (register + delete through the same protocol; a swap when the slot is taken), and
   a drop on a bar but not on a slot leaves it alone.
7. **Dragging an extra bar's handle moves the group:** at the start of `NCShortCutWnd::WindowProc`, a `WM_MOVE` of an
   extra bar is passed on to the main bar, with the grab point (`FrameCtrl`, `GrabX/GrabY`) corrected by the extra
   bar's offset (from the computed layout, not the current positions, which lag a frame); without it `NCWnd::Snap`
   pulls the group and the mouse "jumps".
8. **Art:** check the backgrounds `L2UI_CH3.ShortcutWnd.shortcut_backv` / `shortcut_back` (`UpdateLayout`); if the
   turn button's icon is painted into the texture, cover that cell on the extra bars at the end of
   `NCShortCutWnd::OnPaint` (`DrawTexture` between `BeginPaint`/`EndPaint`).

**Good to know:** tool tip, click to use, C3's soulshot auto-use (`bAutoShot`, per slot, in the shared table) and
drag from the inventory/skills work unchanged, because it is the same class and everything is computed from
`Page`. The keys (F1..F12, pages, game pad) stay on the main bar. The server's packet filter may drop shortcut
registrations sent too fast. Only the game shows position and art problems.

## 4. Recipe: a new button in an existing window

Example: one more button in the system menu (`NCSystemMenuWnd`; in C3 it has 7 icons of 36 × 36 at x = 13, one every
37 px from y = 29, made by `CreateMenuButton`).

1. `include/features/MyButton.h` / `src/features/MyButton.cpp` (`namespace nwf`, `#if NW_FEATURES`):
   `MyButton_Create(NCWnd* Owner)`, `MyButton_OnCommand(NCWnd* Owner, WPARAM ID)`, `MyButton_Destroy(NCWnd* Owner)`;
   the button pointer lives in a global of the `.cpp`.
2. Create it at the end of `NCSystemMenuWnd::OnCreate`: `#if NW_FEATURES` / `// phase 6: MyButton ...` /
   `nwf::MyButton_Create(this);` / `#endif`. Inside: `new NCButton(normal, pushed, 0)`, an `ID` no control of that
   window uses (see its `NID_SYSMENU_*` enum, `0xF0F0xx`), `SetPosition` inside the window's area,
   `Owner->CreateChild(Button, NWS_VISIBLE)`.
3. Handle the click in the `NWM_COMMAND` branch of `NCSystemMenuWnd::WindowProc`:
   `if (nwf::MyButton_OnCommand(this, wParam)) return 1;` (in `#if NW_FEATURES`).
4. Destroy it in `NCSystemMenuWnd::OnDestroy`, before `NCWnd::OnDestroy()`: `RemoveChildWindow(Button->Entry);
   delete Button;`.
5. If the window has to grow, change its `SetPosition` in `NCConsole::OnCreate` and the background painting
   (`L2UI_CH3.SystemMenuWnd.systemmenu_back`) in its `OnPaint` — always in `#if NW_FEATURES`, leaving the stock
   branch intact.

## 5. Recipe: a new window

1. A window with fields or virtuals of its own is a new class, registered only in the feature target (section 2,
   item 6): `class NCMyWnd : public NCWnd { ... };` with its `UClass`, and its line in
   `tests/standalone/l2_extras.csv` (`class,super,class_flags,feature`).
2. Override the virtuals you need: `OnCreate` (call `NCWnd::OnCreate()` first, load textures with `FindTexture`,
   create the title bar — `new NCFrameCtrl(1, 0, 0)` like the system menu — and the controls), `OnDestroy`
   (destroy the children, then `NCWnd::OnDestroy()`), `WindowProc` (`NWM_COMMAND`), `OnPaint`.
3. Create it on the desktop when needed (`SetPosition`, `GDesktop` as the parent in `CreateChild(this,
   NWS_VISIBLE)`, `HideWindow()`) and show it with `ShowWindow()` + `GDesktop->BringChildToTop(Entry)`.
4. Models in this source: `NCSystemMenuWnd` (small, with buttons), `NCAbnormalStatusWnd` (icon grid with a tool
   tip), the private store windows (`NCPrivateShopWnd`, scrolling lists).

## 6. Recipe: a new chat colour

The server sends no colour: it sends the chat **type** (`Say2`), and the client picks the colour in
`NConsoleWnd::Say2` (0x10073630, `src/nwindow/NConsoleWndEvents.cpp`): an `if/else` chain by type (`SAY2_Shout`
orange `0xFFFF7200`, `SAY2_Party` green, `SAY2_Pledge`, `SAY2_Tell`, `SAY2_Market`, `SAY2_Ally`, petition,
announcement; types 11 and 14 are new in C3) that ends in a light-grey `else` (`0xFFDCDCDC`). `NConsoleWnd::NpcSay`,
right below, has its own chain.

1. A type C3 does not know: add, in `#if NW_FEATURES`, a branch before the last `else` (or call a
   `nwf::ChatColors_Find(Type, &Color)` function there), with the colour as ARGB (e.g. `0xFF408CFF`).
2. The line still goes to the chat as a normal one (`GetDesktop()->Say(..., Color, Type)`). Do not reuse types with
   side effects (`SAY2_Tell` adds the speaker to the tell list).
3. Changing the colour of a type C3 **already** colours means changing a faithful branch: do it under
   `#if NW_FEATURES` inside `Say2`, keeping the original colour in the `#else` branch.

## 7. Recipe: a new line in the item tool tip

The full item tool tip is drawn by the routines of `include/nwindow/NItemToolTip.h`, which the item grids use:
`SetTexts`/`SetStoreTexts` fill `ToolTipName` and `ToolTipText` (the text that **sizes** the box) and `DrawLines`
draws the lines, 13 pixels apart (`DrawTypeLines`, then `DrawTailLines`, which ends with the description).

1. Add the line in both places, in `#if NW_FEATURES`: in the sizing text (so the box grows) and in the drawing (for
   example in `DrawTailLines`, before the description, with `DrawLabel`/`DrawValue` and `Y += 13`).
2. The routines are templates instantiated by each grid with different parameters (inventory, paperdoll, trade,
   warehouse, private stores, pet): check every copy you change.
3. For other windows' tool tips the model is `NCAbnormalStatusWnd::OnDrawToolTip`
   (`src/nwindow/NCAbnormalStatusWnd.cpp`): compute the extra height before the box is drawn and draw the line
   after it.

## 8. The build and test loop

```bat
cmake --build build --config Release --target NWindowL2
python tools\cycle.py --system "<C3 client>\system_nwtest" --game-source --standalone --no-install
python tools\verify_standalone.py --dll build\l2\Release\NWindow.dll --l2
```

1. Build the feature target (the name is the one you gave it in section 2) until there is no error and no new
   warning.
2. Run the full cycle: it proves hook mode and the faithful standalone did not change (the judges compare them with
   the original). Then `verify_standalone --l2` on the feature DLL.
3. Test in the game: close the game, keep the `NWindow.dll` of a **copy** of the `system` folder aside and copy
   your DLL over it. The cycle does not draw the interface: position, art and dragging only show in the game.

## 9. Safety rules

- Never add fields or new virtual slots to an original class: its size is registered with the engine and the
  original code indexes into it and its vtable. Overriding a slot the class already inherits is fine (inside
  `#if NW_FEATURES`). Feature state goes into globals of the `.cpp` or into a new class.
- In decompiled code, only the call site, in `#if NW_FEATURES` with `// phase 6: ...`; the stock branch stays as it
  is, and the line count above a `check()` does not change.
- Any run-time patch of another module checks the bytes first and switches the feature off when they do not match.
- Settings in the registry, never in files next to the DLL.
- Same protocol as the original client: the server does not change; respect its packet filter limits.
- Test on a copy of the client, never in the folder you play from; no game binary goes into the repository.
- Small commits, one subject each, with the cycle green.
