#pragma once
// NCCmdTarget / NCWnd - base of every window and control (UClass "CCmdTarget", "CWnd").
//
// A window lives in a tree: NCVirtualWndMain::RegisterNWindow gives it an NWndEntry, its
// parent keeps the entries of its children in a linked list (NWndListNode), messages arrive
// through WindowProc (vslot 24) as Win32 message numbers. Coordinates: Local* are relative
// to the parent, Screen* absolute; SetPosition keeps both.

#include "nwindow/NCObject.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCWnd;
class NCVirtualWndMain;
class NCFrameCtrl;
class NCIcon;
class NCGDevice;

// Registration record of a window (NCVirtualWndMain::RegisterNWindow, 16 bytes) (name ours).
struct NWndEntry
{
    INT bValid;              // +0x00 1 while registered
    NCWnd* Wnd;              // +0x04
    NCVirtualWndMain* Main;  // +0x08 the manager that registered it
    NWndEntry* Parent;       // +0x0C entry of the parent window, NULL for top-level windows

    NWndEntry() : bValid(0), Wnd(NULL), Main(NULL), Parent(NULL) {}
};
NW_ASSERT_SIZE(NWndEntry, 0x10);

// Node of NCWnd's child list, allocated with new (12 bytes) (name ours).
struct NWndListNode
{
    NWndEntry* Entry;        // +0x00
    NWndListNode* Prev;      // +0x04
    NWndListNode* Next;      // +0x08

    NWndListNode() : Entry(NULL), Prev(NULL), Next(NULL) {}
};
NW_ASSERT_SIZE(NWndListNode, 0xC);

// NCWnd::Style bits (names ours)
enum
{
    NWS_VISIBLE = 0x0002,
    NWS_UNKNOWN20 = 0x0020,        // C2 (name ours): NCWnd::OnCreate sets it, with NWS_TOPMOST | NWS_VISIBLE
                                   //   (0x1022), on the icon of a minimizable window; its effect is not known
    NWS_NOHITTEST = 0x0008,        // skipped by NCVirtualWndMain hit tests
    NWS_NOTINPARENTLIST = 0x0040,  // RemoveChildWindow leaves the parent's list alone
    NWS_TITLEBAR = 0x0080,         // OnCreate adds an NCFrameCtrl title bar
    NWS_TOPMOST = 0x1000,          // NCVirtualWndMain keeps these in a separate array
    NWS_MINIMIZABLE = 0x2000,      // OnCreate adds an NCIcon shown while minimized; a
                                   //   decorated NCFrameCtrl adds a minimize button
    NWS_NOCLOSE = 0x4000,          // a decorated NCFrameCtrl leaves out its close button
    NWS_MODAL = 0x0100,            // C2 (name ours): SetModal / ClearModal (vslots 53, 54) set and clear it
    NWS_NOFOCUSNEXT = 0x80000,     // C3 (name ours): FocusNextWindow passes over it
    NWS_CLIPTOPARENT = 0x100000,   // C3 (name ours): SetPosition keeps the window inside its parent (ClipToParent)
    NWS_NOFOCUSSEARCH = 0x200000,  // C3 (name ours): SetFocus does not look below the window for a taker
    NWS_DRAGABLE = 0x400000,       // C3 (name ours): the mouse drags the window (OnLButtonDown / OnMouseMove)
    NWS_ICON = 0x81022,            // C3 (name ours): the style OnCreate gives the minimized icon
    NWS_ITEMSELECT = 0x40000,      // C2 (name ours): the inventory is picking an item (vslots 51, 52 set and
                                   //   clear it: NConsoleWnd::SelectItemToEnchant, NCInventoryWnd::OnMouseMove)
};

// Sent by NCVirtualWndMain after each paint to the window under the cursor, wParam = the
// NCGDevice, handled by vslot 51 (tool tips) (name ours).
enum { NWM_COMMAND = 0x501, NWM_DRAWTOOLTIP = 0x505 };

// NCWnd statics (names ours)
#if NW_HOOK_MODE
// C2 addresses (db/amap.csv: C1 0x10123d54 / 0x10123d50 / 0x1010a998; the generator left the C1 ones here)
#define GFocusEntry  NW_GLOBAL(NWndEntry*, 0x1027107C)  // entry of the window with the keyboard focus
#define GShiftDown   NW_GLOBAL(INT, 0x10271078)         // Shift held (set by OnKeyDown/OnKeyUp)
#define GNextFrameID NW_GLOBAL(INT, 0x101F3134)         // next id for a title bar (NCWnd::OnCreate)
#else
extern NWndEntry* GFocusEntry;
extern INT GShiftDown;
extern INT GNextFrameID;
#endif

// The tool tips of the C2 windows (names ours). NCWnd::OnCreate makes an NToolTipSkin for every window (nine
// textures "L2UI.Tooltip.Tool01".."Tool09" and an NToolTipInfo). What a tool tip shows is an NToolTipInfo that
// its owner keeps (NCButton +0x1D0, NItemEntry +0x19B0, NListRow +0x810, a macro's first member): filled by SetText
// or one of the Fill* builders, which end by measuring it (Layout). The window's OnDrawToolTip hands the skin a
// copy of the info by value, the window's rectangle and the position of the thing the tip is about; the skin
// draws its box with the nine textures and the text on it: a small box with the title line (Draw, for the item
// under the mouse) or the full box with the lines and the description (DrawFull, for the selected item).
// Layouts from the stock constructor 0x10002230, copy constructor 0x10002560, copy assignment 0x1001aca0, the
// skin's destructor 0x10029f20, the builders and the draw routines (0x10027500 .. 0x1002a230).

class NConsoleWnd;
struct NItemEntry;
struct NListRow;

// NToolTipInfo::Style (+0x50): what the tool tip is of; the builders and the draw routines branch on it. The
// values are the ones the windows pass to NItemEntry::FillToolTip (names ours)
enum
{
    NTT_ItemName = 0x001,    // an item's name, additional name and grade, then its weight (a short tip)
    NTT_Item = 0x002,        // an item: the title, then a line per property of its type, the description
    NTT_Skill = 0x004,       // a skill: the title with its level, then type, MP cost and range
    NTT_Action = 0x008,      // an action (name only; Fill takes it with NTT_Name)
    NTT_SkillName = 0x010,   // a skill's name and level only (the abnormal status icons)
    NTT_ShortCut = 0x020,    // a shortcut slot (an item, a skill, an action or a macro: the title depends on Type)
    NTT_Recipe = 0x040,      // a recipe: name, additional name, grade, the two lines of its numbers
    NTT_Name = 0x080,        // the name only
    NTT_Text = 0x100,        // a name and a description (the macros: FillMacro)
    NTT_Desc = 0x200,        // a description under a few lines only (the party matching rows: FillMatchRow)
    NTT_Plain = 0x400,       // SetText: a text, the first line of the title
    NTT_Wanted = 0x800,      // with NTT_Item: the private store's buy list also shows the amount wanted
    NTT_Unknown1000 = 0x1000,  // C3 (name ours): Layout sizes only the title box, 174 wide, with the lines under it
    NTT_Unknown2000 = 0x2000,  // C3 (name ours): Layout treats it as NTT_Text / NTT_Plain (name, description)
    NTT_Unknown8000 = 0x8000,  // C3 (name ours): Layout treats it as NTT_Desc (lines and description, 174 wide)
};

// NToolTipInfo::Options (+0x54): the second argument of Fill (names ours)
enum
{
    NTTO_NoEnchant = 0x01,   // the "+N" before the name is not shown
    NTTO_NoCount = 0x02,     // the " (count)" after the name is not shown
    NTTO_Price = 0x04,       // Fill adds a first line "Price : N Adena" (the shops'); drawn under the name
    NTTO_PriceEach = 0x08,   // the same with the "For Each" before the number (a stack's unit price)
    NTTO_Count = 0x20,       // Fill adds a first line with the count alone (the shortcut slot's amount)
};

// One line of the tool tip (0x414 bytes; the lines array's element): "label : value". The lines are built on the
// stack and copied into the array with the stock `rep movsd` of 0x105 dwords (name ours)
struct NToolTipLine
{
    TCHAR Label[256];       // +0x000
    TCHAR Value[256];       // +0x200
    DWORD LabelColor;       // +0x400 grey (0xFFA3A3A3); the draw routines use the constant, not this
    DWORD ValueColor;       // +0x404 0xFFB09B79; an amount of adena takes the color of its size (NumberColor)
    INT Unused408;          // +0x408 never written by the builders (the stack's garbage is copied)
    INT bLabel;             // +0x40C 1: the label is drawn before the value; 0: the value alone
    INT bColon;             // +0x410 1: the label is followed by " : ", 0: by a space
    UTexture* Icon;         // +0x414 new in C3: the texture the NTT_Unknown1000 tips draw before the label (the lines
                            //   are copied with 0x106 dwords there)

    // as the builders initialize their local line (the Label/Value start empty)
    NToolTipLine() : LabelColor(0xFFA3A3A3), ValueColor(0xFFB09B79), Unused408(0), bLabel(1), bColon(1), Icon(NULL)
    {
        Label[0] = 0;
        Value[0] = 0;
    }
};
NW_ASSERT_SIZE(NToolTipLine, 0x418);
typedef TArray<NToolTipLine> NToolTipLineArray;

// a window's screen rectangle as the skin's draw routines take it by value: x, y (INT), width, height (FLOAT, as
// NCWnd's own rectangle: NRectF) (name ours); also the two boxes of NToolTipInfo, whose X and Y nothing uses
struct NToolTipRect
{
    INT X, Y;
    FLOAT W, H;
};

struct NToolTipInfo
{
    FString Name;           // +0x00 the item's / skill's name
    FString Name2;          // +0x0C its additional name (shown after the name, in yellow)
    INT Enchant;            // +0x18 the "+N" before the name (shown when > 0)
    INT Grade;              // +0x1C crystal grade (NConsoleWnd::GetGradeText: "`D`")
    INT Count;              // +0x20 the amount (the stack, or the trade's offer)
    INT Extra24;            // +0x24 a shortcut skill's second number: " (<sysstring 91>:N)" after its level (> 0)
    INT bShowCount;         // +0x28 the amount is shown (" (N)")
    INT Type;               // +0x2C the item's Type (NItemEntry +0x190C: 0 weapon, 1 armor, 2 jewel, 3.., 5 etc);
                            //   -1 none (C2: also the kind of a shortcut, now ShortCutType)
    INT Level;              // +0x30 a skill's level
    INT ItemID;             // +0x34 the item's id (NItemEntry +0x1918), only stored
    INT ShortCutType;       // +0x38 new in C3 (name ours): what a NTT_ShortCut tip is of: 1 item, 2 skill, 3 action,
                            //   4 macro (C2 kept this in Type; the constructor clears it, Clear leaves it alone)
    NToolTipLineArray Lines;     // +0x3C the property lines (elements of 0x418 bytes) (C2 +0x38)
    FString Description;    // +0x48 the item's / skill's description (the newlines escaped) (C2 +0x44)
    DWORD Style;            // +0x54 NTT_* (C2 +0x50)
    DWORD Options;          // +0x58 NTTO_* (C2 +0x54)
    NToolTipRect FullBox;   // +0x5C (X, Y unused) the full box's text size: W (+0x64) and H (+0x68), by Layout (C2 +0x58)
    NToolTipRect TitleBox;  // +0x6C (X, Y unused) the title line's size: W (+0x74) and H (+0x78), by Layout (C2 +0x68)

    // out of line like the stock ones (the callers call 0x10002230 / 0x10002560 / 0x100022d0 / 0x1001aca0)
    __declspec(noinline) NToolTipInfo();                        // 0x100024f0
    __declspec(noinline) NToolTipInfo(const NToolTipInfo& Src); // 0x10002610 (the copy a by-value argument is made of)
    __declspec(noinline) ~NToolTipInfo();                       // 0x100025a0 (the members' destructors)
    __declspec(noinline) NToolTipInfo& operator=(const NToolTipInfo& Src);  // 0x1001dd10

    // 0x1002ad60: the sizes of the boxes for the Style, from the texts and the font (the builders end with it)
    __declspec(noinline) void Layout();
    // 0x1002c4f0: back to an empty tip (Name, Name2, Description, the lines and the numbers; Style / Options kept
    // by the builders' own stores)
    __declspec(noinline) void Clear();
    // 0x1002c4d0: the text and its Style (NCButton::SetToolTip passes NTT_Plain), then Layout
    __declspec(noinline) void SetText(const TCHAR* InText, INT InStyle);
    // 0x1002d7f0 (ret 0xC, name ours): a tip of a name and a description (either may be NULL, both: nothing happens):
    // emptied, the Style set, the texts stored, then Layout
    __declspec(noinline) void SetNameDesc(const TCHAR* InName, const TCHAR* InDesc, INT InStyle);
    // 0x1002ad40 (name ours): bShowCount is 1, 2 or 3: the amount follows the name
    __declspec(noinline) UBOOL ShowsCount();
    // 0x1002c650 (ret 0xC): the tip of an item entry (or the skill / action / shortcut / recipe the entry
    // stands for) for a Style and Options
    __declspec(noinline) void Fill(NItemEntry* Item, DWORD InStyle, DWORD InOptions);
    // 0x1002d7a0 (ret 8): a macro object's tip: it starts with its NToolTipInfo (`this` is the macro) and keeps
    // its name at +0x9EC and its description at +0xA06
    __declspec(noinline) void FillMacro(NToolTipInfo* Macro, DWORD InStyle);
    // 0x1002d840 (ret 8): the party matching row's tip: the level, the class and the party's purpose, and the
    // row's text as the description (`this` is the row's NToolTipInfo)
    __declspec(noinline) void FillMatchRow(NListRow* Row, DWORD InStyle);

    // 0x1002ac00 (cdecl; name ours): Value with a comma between thousands, in a static buffer
    static const TCHAR* FormatNumber(INT Value);
    // 0x1002acb0 (cdecl; name ours): the color of a formatted amount: light grey up to four digits, then pink, yellow,
    // green and cyan in turn as it grows
    static DWORD NumberColor(const TCHAR* Text);
};
NW_ASSERT_OFFSET(NToolTipInfo, Enchant, 0x18);
NW_ASSERT_OFFSET(NToolTipInfo, Type, 0x2C);
NW_ASSERT_OFFSET(NToolTipInfo, ItemID, 0x34);
NW_ASSERT_OFFSET(NToolTipInfo, Lines, 0x3C);
NW_ASSERT_OFFSET(NToolTipInfo, Description, 0x48);
NW_ASSERT_OFFSET(NToolTipInfo, Style, 0x54);
NW_ASSERT_OFFSET(NToolTipInfo, FullBox, 0x5C);
NW_ASSERT_OFFSET(NToolTipInfo, TitleBox, 0x6C);
NW_ASSERT_SIZE(NToolTipInfo, 0x7C);

struct NToolTipSkin
{
    UTexture* Tex[9];       // +0x00 "L2UI.Tooltip.Tool01" .. "Tool09" (corners, edges and fill, row by row), set by NCWnd::OnCreate
    NToolTipInfo Info;      // +0x24 the copy of the tip being drawn (Draw / DrawFull store it first)

    __declspec(noinline) NToolTipSkin();     // 0x1002dba0
    __declspec(noinline) ~NToolTipSkin();    // 0x1002dbc0: the arrays and strings are emptied by hand, then the
                                             //   members go (0x100025a0)

    // 0x1002df40 (ret 0x98): the box of the title line (TitleBox + 10) with the title text (DrawTitleText) at the
    // position X, Y the owner gives (relative to the device's origin, the window's corner), which the routine moves to
    // keep the box on the screen (C3: by value; C2 took pointers). Info is taken BY VALUE: the routine destroys its copy.
    // Nothing is drawn for a tip with no name and no lines.
    void Draw(NCGDevice* Device, NToolTipInfo Info, INT X, INT Y, NToolTipRect Wnd);
    // 0x1002dc40 (ret 0x98): the same for the full box (FullBox + 10) with DrawFullText
    void DrawFull(NCGDevice* Device, NToolTipInfo Info, INT X, INT Y, NToolTipRect Wnd);
    // 0x1002c0f0 (ret 0xC, name ours): the title line at X, Y: "+N" (orange), the name, the additional name
    // (yellow), the grade, " (count)", the 0x1000 style's list and the first property line when Options has
    // NTTO_Price / NTTO_PriceEach
    void DrawTitleText(NCGDevice* Device, INT X, INT Y);
    // 0x1002b800 (ret 0xC, name ours): the full box's text: the title by Style / ShortCutType, the lines, the description
    void DrawFullText(NCGDevice* Device, INT X, INT Y);
};
NW_ASSERT_SIZE(NToolTipSkin, 0xA0);   // C3: the NToolTipInfo inside grew by 4

class NCCmdTarget : public NCObject
{
    NW_DECLARE_ABSTRACT_CLASS(NCCmdTarget, NCObject, 0, 0x10263338)
    // ctor: NCObject's (0x10020ca0, identical code folded); ~NCCmdTarget 0x10001690
};
NW_ASSERT_SIZE(NCCmdTarget, 0x28);

// A position and a FLOAT size in screen pixels (name ours): what NCWnd hands PtInRect by value, and what
// GetPositioningRect / ClipToParent fill.
struct NRectF
{
    INT X;
    INT Y;
    FLOAT W;
    FLOAT H;
};
NW_ASSERT_SIZE(NRectF, 0x10);

class NCWnd : public NCCmdTarget
{
    NW_DECLARE_CLASS(NCWnd, NCCmdTarget, 0, 0x10271088)

    // The sides of the sticky windows of Src\NWnd.cpp (names ours; the assert strings say SD_MAX). The order is the
    // one of GetOppStickDirection's pairs: 0 <-> 3, 1 <-> 2.
    enum { SD_TOP = 0, SD_LEFT = 1, SD_RIGHT = 2, SD_BOTTOM = 3, SD_MAX = 4 };
    // Bits of PosFlags (names ours): RearrangePosition sets POS_REARRANGED while it moves a window along, SetWindowSize
    // clears POS_REARRANGED | POS_SIZED on the children before it rearranges them.
    enum { POS_REARRANGED = 1, POS_SIZED = 2 };

    // C3 layout, 0xC8 bytes (C2: 0x98). The local rectangle (LocalX/Y/W/H) is gone, the size is FLOAT, the icon
    // moved up, the sticking was reworked (four arrays of entries) and there is an anchor.
    NWndListNode* ChildHead;   // +0x28
    NWndListNode* ChildTail;   // +0x2C
    NCIcon* Icon;              // +0x30 shown instead of the window while minimized (NWS_MINIMIZABLE)
    UBOOL bFocus;              // +0x34
    union
    {
        struct
        {
            INT ScreenX;       // +0x38 absolute
            INT ScreenY;       // +0x3C
            FLOAT Width;       // +0x40
            FLOAT Height;      // +0x44
        };
        NRectF Rect;           // the same four: IsInWindow hands them to PtInRect by value
    };
    DWORD Style;               // +0x48 NWS_*
    UBOOL bMouseDown;          // +0x4C (name ours) OnLButtonDown sets it, OnLButtonUp clears it; OnMouseMove drags
                               //   while it is set
    DWORD PosFlags;            // +0x50 (name ours) POS_*
    NWndEntry* Entry;          // +0x54 own registration
    UBOOL bEnabled;            // +0x58
    NCFrameCtrl* FrameCtrl;    // +0x5C title bar (NWS_TITLEBAR)
    INT FrameID;               // +0x60 id given to FrameCtrl (GNextFrameID++)
    INT DragMouseX;            // +0x64 last mouse position of a drag
    INT DragMouseY;            // +0x68
    NToolTipSkin* ToolTipSkin; // +0x6C made by OnCreate for every window, freed by OnDestroy (name ours)
    NWndEntry* NextTab;        // +0x70 TAB goes there
    NWndEntry* PrevTab;        // +0x74 Shift+TAB goes there
    INT Unknown78;             // +0x78 no function of NCWnd touches it (C2: the window number of the windows INI)
    INT ClipOffsetX;           // +0x7C (name ours) ClipToParent: how far the left edge was moved into the parent
    INT ClipOffsetY;           // +0x80 the same for the top edge
    TArray<NWndEntry*> StuckWnds[SD_MAX];   // +0x84 the windows stuck to each side (InitStuckState empties them)
    INT AnchorType;            // +0xB4 SetAnchor: 0 none, 1..9 where in the parent the window is held (names ours)
    INT AnchorOffsetX;         // +0xB8
    INT AnchorOffsetY;         // +0xBC
    FLOAT ScaleX;              // +0xC0 RearrangePosition: the share of a size change of the group the window follows
    FLOAT ScaleY;              // +0xC4

    NCWnd();                                                     // 0x10031d80

    virtual __declspec(noinline) INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam); // 25 0x1002fa20
    virtual UBOOL IsEnabled() { return bEnabled; }               // 26 0x10002770
    virtual UBOOL IsInWindow(INT X, INT Y);                      // 27 0x1002fdd0 (name ours)
    virtual UBOOL IsFocused() { return bFocus; }                 // 28 0x10002780 (name ours)
    virtual void SetFocus();                                     // 29 0x10030c60
    virtual void KillFocus();                                    // 30 0x1002fd50
    virtual void SetStyle(DWORD NewStyle) { Style = NewStyle; }  // 31 0x1002fd70
    virtual DWORD GetStyle() { return Style; }                   // 32 0x10002790
    virtual INT vfn032() { return 0; }                           // 33 0x10001af0 (folded) 1 = takes keyboard input
    virtual INT vfn033() { return 0; }                           // 34 0x10001af0 (folded)
    virtual void NewVirtual35();  // 35 0x10001af0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual INT OnTimer(INT TimerID) { return 0; }               // 36 0x10001b10 (folded) WM_TIMER
    virtual INT OnCreate();                                      // 37 0x10030fc0
    virtual INT OnDestroy();                                     // 38 0x10031310
    virtual UBOOL IsShowWindow();                                // 39 0x1002fc70 (name ours)
    virtual UBOOL PtInRect(INT X, INT Y, NRectF Rect);           // 40 0x1002fd80 (edges included)
    virtual void SetTabOrder(NWndEntry* Next, NWndEntry* Prev);  // 41 0x1002fbe0 (name ours)
    virtual void EnableWindow(UBOOL bEnable);                    // 42 0x1002fc00 (name ours)
    virtual void SetWindowSize(FLOAT W, FLOAT H);                // 43 0x10031e40
    virtual void ShowWindow();                                   // 44 0x1002fc30
    virtual void HideWindow();                                   // 45 0x10030c00
    virtual void MoveWindow(INT DX, INT DY);                     // 46 0x10032aa0 (name ours) snaps
    virtual void OffsetWindow(INT DX, INT DY);                   // 47 0x1004d460 (name ours)
    virtual void Restore();                                      // 48 0x100311d0 (name ours)
    virtual void Minimize();                                     // 49 0x1002fe30 (the db says MinimizeWindow)
    virtual void CenterInParent();                               // 50 0x10030f40 (name ours)
    virtual void SetTitle(const TCHAR* Title);                   // 51 0x1002ff60 (name ours)
    virtual void SetItemSelect();                                // 52 0x10030000 (name ours)
    virtual void ClearItemSelect();                              // 53 0x10030020 (name ours)
    virtual void SetModal(NWndEntry* E);                         // 54 0x1002ffc0 (name ours)
    virtual void ClearModal(NWndEntry* E);                       // 55 0x1002ffe0 (name ours)
    virtual INT OnPaint(NCGDevice* Device) { return 0; }         // 56 0x10001b10 (folded) WM_PAINT
    virtual INT OnDrawToolTip(NCGDevice* Device) { return 0; }   // 57 0x10001b10 (folded) (name ours)
    virtual INT OnChar(WPARAM wParam, LPARAM lParam) { return 0; }          // 58 0x1007d6b0 (folded)
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);                    // 59 0x1002feb0
    virtual INT OnKeyUp(WPARAM wParam, LPARAM lParam);                      // 60 0x1002ff30
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);                  // 61 0x100313f0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);                // 62 0x100314f0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);                  // 63 0x100315c0 (guard: OnLButtonDown)
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam) { return 0; }   // 64 0x1007d6b0 (folded)
    virtual INT OnRButtonUp(WPARAM wParam, LPARAM lParam) { return 0; }     // 65 0x1007d6b0 (folded)
    virtual INT OnMove(WPARAM wParam, LPARAM lParam);                       // 66 0x1002fe80 WM_MOVE
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam) { return 0; }    // 67 0x1007d6b0 (folded)

    // non-virtual
    // This is NCCmdTarget::WindowProc (vslot 25 of NCCmdTarget's vtable): WM_CREATE, WM_DESTROY and WM_TIMER go to
    // OnCreate / OnDestroy / OnTimer. NCWnd::WindowProc calls it first.
    __declspec(noinline) INT DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);   // 0x10004c70 (name ours)
    void BringChildToTop(NWndEntry* Child);                      // 0x1002fc80 (name ours)
    NWndEntry* FindWindowAt(INT X, INT Y);                       // 0x1002fe20 (name ours)
    void FormatNumber(TCHAR* Out, const TCHAR* Digits);          // 0x10030040 (name ours)
    UBOOL AddChildNode(NWndEntry* Child);                        // 0x100303c0 (name ours)
    void RemoveChildNode(NWndEntry* Child);                      // 0x10030420 (name ours)
    NWndEntry* AddChildWindow(NCWnd* Child, DWORD ChildStyle);   // 0x10030990 (name ours)
    void RemoveChildWindow(NWndEntry* Child);                    // 0x100309d0 (name ours)
    void SetPosition(INT X, INT Y, DWORD W, DWORD H);            // 0x10030db0 X, Y relative to the parent; W and H 0
                                                                 //   keep the size
    void SetScreenPos(INT X, INT Y, const POINT* Origin);        // 0x10030ef0 (name ours)
    void Snap(POINT* Delta);                                     // 0x100321d0 (name ours) sticky windows

    // The second half of CreateChild on the child (name ours): an out-of-line copy of the
    // inline code for the call sites where the stock compiler stopped inlining it.
    void AttachEntry(NWndEntry* E);                              // 0x10005530

    // Registers a newly created child the way every OnCreate does (inlined in the stock
    // code, name ours): entry under this window, screen position made absolute, WM_CREATE.
    // Forced inline: inside a guard (/EHa) our compiler kept it out of line.
    FORCEINLINE NWndEntry* CreateChild(NCWnd* Child, DWORD ChildStyle)
    {
        NWndEntry* E = AddChildWindow(Child, ChildStyle);
        Child->Entry = E;
        if (E->Parent)
        {
            NCWnd* Parent = E->Parent->Wnd;
            Child->ScreenX += Parent->ScreenX;
            Child->ScreenY += Parent->ScreenY;
        }
        Child->WindowProc(WM_CREATE, 0, 0);
        return E;
    }

    // Sticky windows and anchors (Src\NWnd.cpp, new in C3; the names of the ones with a guard() are the stock ones)
    void InitStuckState();                                       // 0x10030d10 empties the four lists
    __declspec(noinline) UBOOL IsAnchored();                     // 0x10030230 (name ours)
    __declspec(noinline) UBOOL IsDragable();                     // 0x10030240 (name ours)
    __declspec(noinline) void AddPosFlag(DWORD Flag);            // 0x1002ff90 (name ours)
    __declspec(noinline) void ClearPosFlag(DWORD Flag);          // 0x1002ffa0 (name ours)
    __declspec(noinline) DWORD GetPosFlags();                    // 0x1012b140 (folded with another class's getter, name ours)
    NCWnd* GetDragableParentWnd();                               // 0x10030ab0 the nearest ancestor with NWS_DRAGABLE
    __declspec(noinline) NRectF* GetLocalRect(NRectF* Out);          // 0x10030900 position relative to the parent (name ours)
    __declspec(noinline) INT* GetLocalPos(INT* Out);             // 0x10030950 the same, only the position (name ours)
    INT GetOppStickDirection(INT Direction);                     // 0x10030170
    void SetAnchor(INT Type, INT OffsetX, INT OffsetY);          // 0x10030a10
    void MoveChildWindow(INT DX, INT DY);                        // 0x10030490 moves the window and its subtree
    void ClipToParent(NRectF* Out);                              // 0x100306d0 (name ours)
    void GetStuckWndList(TArray<NCWnd*>& a_StuckWndList);        // 0x10031680 this and everything stuck to it
    UBOOL IsStuck(INT Direction, NWndEntry* Wnd);                // 0x10031770 -1 any direction, -1 / -2 as Wnd
    void StickWnd(INT Direction, NWndEntry* Wnd);                // 0x10032090
    void UnstickWnd(INT Direction, NWndEntry* Wnd);              // 0x10031ca0
    void GetPositioningRect(NRectF* a_pRect, NCWnd** ppLeft, NCWnd** ppRight, NCWnd** ppTop, NCWnd** ppBottom);   // 0x10031920
    void RearrangePosition(FLOAT OldW, FLOAT OldH, FLOAT NewW, FLOAT NewH);   // 0x10031ac0
    UBOOL FocusFirstInput();                                     // 0x1002fcf0 (name ours)
    UBOOL HasSliderBarChild();                                   // 0x10030310 (name ours)
    UBOOL FocusSliderBarParent();                                // 0x10030350 (name ours)
    void FocusNextWindow(NWndEntry* Hidden);                     // 0x10030250 (name ours)
};
NW_ASSERT_OFFSET(NCWnd, ChildHead, 0x28);
NW_ASSERT_OFFSET(NCWnd, Icon, 0x30);
NW_ASSERT_OFFSET(NCWnd, bFocus, 0x34);
NW_ASSERT_OFFSET(NCWnd, ScreenX, 0x38);
NW_ASSERT_OFFSET(NCWnd, Width, 0x40);
NW_ASSERT_OFFSET(NCWnd, Style, 0x48);
NW_ASSERT_OFFSET(NCWnd, bMouseDown, 0x4C);
NW_ASSERT_OFFSET(NCWnd, PosFlags, 0x50);
NW_ASSERT_OFFSET(NCWnd, Entry, 0x54);
NW_ASSERT_OFFSET(NCWnd, bEnabled, 0x58);
NW_ASSERT_OFFSET(NCWnd, FrameCtrl, 0x5C);
NW_ASSERT_OFFSET(NCWnd, ToolTipSkin, 0x6C);
NW_ASSERT_OFFSET(NCWnd, NextTab, 0x70);
NW_ASSERT_OFFSET(NCWnd, ClipOffsetX, 0x7C);
NW_ASSERT_OFFSET(NCWnd, StuckWnds, 0x84);
NW_ASSERT_OFFSET(NCWnd, AnchorType, 0xB4);
NW_ASSERT_OFFSET(NCWnd, ScaleX, 0xC0);
NW_ASSERT_SIZE(NCWnd, 0xC8);

#pragma pack(pop)