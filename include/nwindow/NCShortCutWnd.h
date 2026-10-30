#pragma once
// NCShortCutWnd - the shortcut bar (UClass "CShortCutWnd", 0x270; desktop +0x138): Pages pages
// of Cols slots (12 x 10 in the C3 client: F1 .. F12), each an item, a skill or an action the server registered
// (NConsoleWnd hands them over: SetShortCut). F1 .. F12 (or 1 .. 0, -, = while the chat line needs
// Enter) or a click uses the slot of the current page; the arrows turn the pages, the corner
// button turns the bar between vertical (46 x 504) and horizontal (504 x 46), the game pad button
// shows the pad layout (160 x 82: four slots at a time, L / R hold). A slot dragged off the bar
// is deleted on the server. Each slot's key waits before it works again: until released,
// until its skill's cool time is over, or a fixed time (shots, attack, pick up).

#include "nwindow/NControl.h"
#include "nwindow/NCCoolTimeIcon.h"
#include "nwindow/NCItemWnd.h"        // NItemEntry: the slots are built on it
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

struct NItemInfo;
class NCPushButton;

// NWM_COMMAND ids of the bar's controls
enum
{
    NID_SHORTCUT_FRAME = (INT)0xFF000003,
    NID_SHORTCUT_NEXT = (INT)0xFF000004,
    NID_SHORTCUT_PREV = (INT)0xFF000005,
    NID_SHORTCUT_ROTATE = (INT)0xFF000008,
    NID_SHORTCUT_PAD = (INT)0xFF000009,      // new in C2 (the id the WindowProc tests; the button's own id is set in OnCreate)
};

// NShortCutItem::Type (names ours)
enum { SHORTCUT_Item = 1, SHORTCUT_Skill = 2, SHORTCUT_Action = 3, SHORTCUT_Macro = 4 /* new in C2 */ };

// A slot of the bar (name ours; 0x1AB4 bytes in C3, 0x1A7C in C2; an array of Cols * Pages): an item entry (the item,
// skill, action or macro the slot holds) with the cool time icon and the slot's own fields. C3: the slot's kind is the
// entry's +0x1910 (Unknown1910; SetShortCut stores NItemInfo +0x1710 there; Kind() below), ObjectID +0x1914, ID
// +0x1918 (the item, skill, action or macro id), UnknownC18 +0x191C the skill's level, Count +0x1920, Enchant +0x193C,
// bStackable +0x1944, Unknown1998 +0x199C the "asks first" flag, the tool tip +0x19B8. Layout from the constructors
// (0x100f62f0, 0x100f6350), CopyFrom (0x100f2f50) and Clear (0x100f3060).
struct NShortCutItem : public NItemEntry
{
    NCCoolTimeIcon CoolTime;       // +0x1A34
    INT Owner;                     // +0x1A8C items: 1 the player's inventory, 2 the pet's (UseShortCut); copied by CopyFrom, left by Clear
    INT Unknown1A90;               // +0x1A90 NItemInfo +0x1768 (the constructor), copied by CopyFrom, cleared by Clear
    UBOOL bUsed;                   // +0x1A94 the slot holds something (cleared by the constructors and by Clear)
    UBOOL bAutoShot;               // +0x1A98 (name ours) new in C3: the shot is on auto use (SetShortCut sets it for a shot the
                                   //   auto list holds; the painters draw the toggle effect over it; OnRButtonDown flips it)
    BYTE _Unknown1A9C[0x1AAC - 0x1A9C];   // +0x1A9C (not touched by the code seen)
    INT Unknown1AAC;               // +0x1AAC (0 in the constructor, CopyFrom and Clear)
    INT Unknown1AB0;               // +0x1AB0 (0 in the constructor, CopyFrom and Clear)

    NShortCutItem();                                                 // 0x100f62f0
    NShortCutItem(const NItemInfo& Info);                            // 0x100f6350
    // ~NShortCutItem                                                // 0x100f63d0 (the cool time's empty destructor 0x10001b00,
                                                                     //   then the entry's: its tool tip)
    __declspec(noinline) void CopyFrom(NShortCutItem* Src);          // 0x100f2f50 the entry's Copy, then the slot's fields; the
                                                                     //   cool time stops
    __declspec(noinline) INT Clear();                                // 0x100f3060 the entry's Reset, the fields; the cool time stops

    // the slot's kind, SHORTCUT_* (the entry's Unknown1910 in this client)
    INT& Kind() { return Unknown1910; }
};
NW_ASSERT_OFFSET(NShortCutItem, CoolTime, 0x1A34);
NW_ASSERT_OFFSET(NShortCutItem, Owner, 0x1A8C);
NW_ASSERT_OFFSET(NShortCutItem, bUsed, 0x1A94);
NW_ASSERT_OFFSET(NShortCutItem, bAutoShot, 0x1A98);
NW_ASSERT_OFFSET(NShortCutItem, Unknown1AAC, 0x1AAC);
NW_ASSERT_SIZE(NShortCutItem, 0x1AB4);

// When the key of a slot works again (name ours; 0x14 bytes, an array of Cols * Pages).
struct NShortCutKey
{
    DWORD Elapsed;                 // +0x00 ms since the last use
    DWORD Interval;                // +0x04 ms it waits (shots, attack: 200, pick up: 500)
    UBOOL bReady;                  // +0x08 the key uses the slot
    UBOOL bOnKeyUp;                // +0x0C ready again when the key is released
    UBOOL bOnCoolTime;             // +0x10 ready again when the cool time is over

    NShortCutKey() : Elapsed(0), Interval(0), bReady(1), bOnKeyUp(0), bOnCoolTime(0) {}

    // 0x100f4000 (name ours): the key that waits for its release is ready again (the mouse's release uses it like the
    // key's)
    __declspec(noinline) void OnKeyUp();
};
NW_ASSERT_SIZE(NShortCutKey, 0x14);

// C3 layout from the constructor (0x100f6430), OnCreate (0x100f6660), UpdateLayout (0x100f3150) and the painters.
class NCShortCutWnd : public NControl
{
    NW_DECLARE_CLASS(NCShortCutWnd, NControl, 0, 0x102ADE28)

    INT TimerID;                           // +0x1D4 (name ours) SetTimer(0x1F9, 200 ms) in OnCreate
    INT bPadMode;                          // +0x1D8 (name ours) 0 the bar, 1 the game pad layout (OnPadClicked; the options
                                           //   window compares it with 1)
    NShortCutKey* Keys;                    // +0x1DC new NShortCutKey[Cols * Pages]
    NShortCutItem* Items;                  // +0x1E0 new NShortCutItem[Cols * Pages]
    UTexture* BackTex;                     // +0x1E4 "L2UI_CH3.ShortcutWnd.shortcut_backv" / "_back" (UpdateLayout), the pad's "joypad_back"
    UTexture* Unknown1E8;                  // +0x1E8 (0: the constructor clears it, nothing sets it)
    UTexture* IconBackTex;                 // +0x1EC "L2UI_CH3.Etc.menu_outline" the frame of a slot
    UTexture* IconClickTex;                // +0x1F0 "L2UI_CH3.Etc.menu_outline_down" over the pressed one
    UTexture* ToggleTex;                   // +0x1F4 "L2_SkillTime.ToggleEffect.ToggleEffect001"
    UTexture* KeyTex[12];                  // +0x1F8 "L2UI_CH3.ShortcutWnd.shortcut_f01" .. "_f12": the key of each slot
    NCPushButton* NextButton;              // +0x228 right / down arrow
    NCPushButton* PrevButton;              // +0x22C left / up arrow
    NCPushButton* RotateButton;            // +0x230 vertical / horizontal
    NCPushButton* PadButton;               // +0x234 "L2UI_CH3.ShortcutWnd.shortcut_joypad", hidden until the game pad is in use
    UBOOL bPressed;                        // +0x238 mouse button down on PressedSlot
    UBOOL bDragging;                       // +0x23C
    UBOOL bHorizontal;                     // +0x240
    INT Cols;                              // +0x244 slots per page (12)
    INT Pages;                             // +0x248 (10)
    INT Page;                              // +0x24C
    INT IconSize;                          // +0x250 (32)
    INT PressedSlot;                       // +0x254 -1 = none
    INT HoverSlot;                         // +0x258 slot under the mouse, -1 = none
    INT PadHold;                           // +0x25C (name ours) 0, 1 = left hold, 2 = right hold (SetPadHoldL / R)
    UTexture* PadHoldLTex;                 // +0x260 "L2UI_ch3.Joypad.joypad_L_HOLD"
    UTexture* PadHoldRTex;                 // +0x264 "L2UI_ch3.Joypad.joypad_R_HOLD"
    UTexture* PadLTex;                     // +0x268 "L2UI_ch3.Joypad.joypad_L"
    UTexture* PadRTex;                     // +0x26C "L2UI_ch3.Joypad.joypad_R"

    // the slot and its cool time (the stock Tick works on the cool time at +0x1A34)
    NShortCutItem* ItemAt(INT Index) { return &Items[Index]; }
    NCCoolTimeIcon* ItemCoolTime(INT Index) { return &Items[Index].CoolTime; }

    NCShortCutWnd(INT InCols = 12, INT InPages = 10, INT InIconSize = 32);          // 0x100f6430 (ret 0xC)
    // ~NCShortCutWnd                                                // 0x100f6610

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100f30a0
    virtual INT OnCreate();                                          // 37 0x100f6660
    virtual INT OnDestroy();                                         // 38 0x100f6b50
    virtual void ShowWindow();                                       // 44 0x100f3ec0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100f5280
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x100f6cd0
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x100f53c0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100f5360
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100f5550
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 64 0x100f3f00 (the shot on / off the auto use)
    // names ours
    virtual void UpdateLayout();                                     // 69 0x100f3150 after a turn: the bar's size, the textures and
                                                                     //   the buttons of the vertical / horizontal layout
    virtual void SetPadHoldL(bool bHold);                            // 70 0x100f3d00 PadHold = bHold
    virtual void SetPadHoldR(bool bHold);                            // 71 0x100f3d20 PadHold = bHold ? 2 : 0
    virtual INT OnNextClicked();                                     // 72 0x100f6270
    virtual INT OnPrevClicked();                                     // 73 0x100f6290
    virtual INT OnRotateClicked();                                   // 74 0x100f3af0
    virtual INT OnPadClicked();                                      // 75 0x100f3d40 the game pad layout on / off

    // names ours; the console calls most of them (UpdateShortCut, ..., Tick, HookShortCutKey, ReloadGameTexts)
    INT UpdateItem(INT ObjectID);                                    // 0x100f36c0 inventory
    INT UpdatePetItem(INT ObjectID);                                 // 0x100f37e0 pet inventory
    __declspec(noinline) INT SlotAt(INT X, INT Y);                                        // 0x100f38e0 -1 = none
    void ReloadNames();                                              // 0x100f3b20 new language: the name and the tool tip of every slot again
    INT PageSlot(INT Index);                                         // 0x100f3ac0
    INT DeleteShortCut(INT Slot);                                    // 0x100f5a30
    void SetPage(INT NewPage);                                       // 0x100f5a90
    void Tick(FLOAT DeltaTime);                                      // 0x100f5b00
    INT OnShortCutKeyUp(UINT Key, LPARAM lParam);                    // 0x100f5be0
    INT OnShortCutKeyDown(UINT Key, LPARAM lParam);                  // 0x100f5d90 (the console's HookShortCutKey calls it)
    void NextPage();                                                 // 0x100f6190
    void PrevPage();                                                 // 0x100f6200
    void ClearShortCuts();                                           // 0x100f62b0
    INT SetShortCut(NItemInfo* Info, INT Owner);                     // 0x100f6f30 (the console's AddShortCutItem calls it)
    __declspec(noinline) INT SlotAtPoint(INT X, INT Y);                                   // 0x100f3aa0 the slot under a screen point by the layout (bPadMode): SlotAt, PadSlotAt or -1
    __declspec(noinline) INT PadSlotAt(INT X, INT Y);                                     // 0x100f39e0 the game pad layout's four slots (PadHold * 4 + 0 .. 3), -1 none
    INT UpdateMacro(INT MacroID, INT Mode);                          // 0x100f3460 the console's UpdateMacroShortCut: Mode 2, the slots of the macro take its icon, name and tool tip; 0, they are deleted
    void PaintBar(NCGDevice* Device);                                // 0x100f4030 the vertical / horizontal bar (OnPaint's)
    void PaintPad(NCGDevice* Device);                                // 0x100f4b70 the game pad layout (OnPaint's)
    UBOOL IsAutoShotSlot(INT ItemID);                                // 0x100f3420 (name ours) a used slot of this item that is on auto use
};
NW_ASSERT_OFFSET(NCShortCutWnd, TimerID, 0x1D4);
NW_ASSERT_OFFSET(NCShortCutWnd, Keys, 0x1DC);
NW_ASSERT_OFFSET(NCShortCutWnd, BackTex, 0x1E4);
NW_ASSERT_OFFSET(NCShortCutWnd, IconBackTex, 0x1EC);
NW_ASSERT_OFFSET(NCShortCutWnd, ToggleTex, 0x1F4);
NW_ASSERT_OFFSET(NCShortCutWnd, KeyTex, 0x1F8);
NW_ASSERT_OFFSET(NCShortCutWnd, NextButton, 0x228);
NW_ASSERT_OFFSET(NCShortCutWnd, PadButton, 0x234);
NW_ASSERT_OFFSET(NCShortCutWnd, Cols, 0x244);
NW_ASSERT_OFFSET(NCShortCutWnd, HoverSlot, 0x258);
NW_ASSERT_OFFSET(NCShortCutWnd, PadHoldRTex, 0x264);
NW_ASSERT_OFFSET(NCShortCutWnd, PadRTex, 0x26C);
NW_ASSERT_SIZE(NCShortCutWnd, 0x270);

#pragma pack(pop)
