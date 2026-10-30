/*=============================================================================
    NCShortCutWnd.cpp - the shortcut bar. Stock (C3): 0x100f2f50 .. 0x100f3150 (the slot, its copy), 0x100f3150 ..
    0x100f4030 (layout, hit tests, updates), 0x100f4030 .. 0x100f5280 (the painters), 0x100f5280 .. 0x100f5b00 (OnPaint,
    the mouse), 0x100f5b00 .. 0x100f6270 (Tick, the keys, the pages), 0x100f62f0 .. 0x100f6430 (the slot's constructors),
    0x100f6430 .. 0x100f6f30 (the window's constructor, OnCreate, OnDestroy, OnDrawToolTip), 0x100f6f30 (SetShortCut).
=============================================================================*/
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCActionItemWnd.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCPetWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCPetInventoryWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleFns.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCMacroEditList.h"
#include "nwindow/NCMacroEditWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NMacroEntry.h"
#include "nwindow/NItemInfo.h"
#include "nwindow/NClasses.gen.h"

// the dialog boxes of the shortcut items (NDialogInfo::ID; names ours, the console's UseShortCut has them too): an item
// with a warning asks first, an item with a message of its own shows it
enum { DIALOG_CONFIRM_ITEM = 0x21, DIALOG_ITEM_MESSAGE = 0x45 };

// the sound of an empty slot's key, of the cool time's end (names ours)
enum { SOUND_EmptySlot = 2, SOUND_CoolTimeEnd = 9 };

// the command prefix of the macro editor's lines ("/"): the string at 0x101761e4 through the pointer at 0x10236414
#if NW_HOOK_MODE
#define GShortCutCmdPrefix NW_GLOBAL(const TCHAR*, 0x10236414)
#else
extern const TCHAR* GShortCutCmdPrefix;
#endif

// the console's helpers (names ours, in db/names.csv; NConsoleWnd's, IsDwarf NLeftHelpers.cpp's)
static FORCEINLINE UBOOL IsQuickUseItem(INT ItemID)       // 0x10060190 potions and shots: the key repeats every 200 ms
{
    return GConsole->IsQuickUseItem(ItemID);
}
static FORCEINLINE UBOOL IsShotItem(INT ItemID)           // 0x10062370 soul / spirit shots
{
    return GConsole->IsShotItem(ItemID);
}
static FORCEINLINE UBOOL AsksBeforeUse()                  // 0x1006b6c0 the C3 console's test for the items with a warning
{
    return ((NConsoleLeftFns*)GConsole)->IsDwarf();
}
static FORCEINLINE void RequestAutoShot(L2ParamStack& Params)   // 0x10062360 (ret 4) the shot on / off the auto use
{
    GConsole->RequestAutoShot(Params);
}
static FORCEINLINE void GetSkillCoolTime(INT SkillID, DOUBLE* Total, DOUBLE* Remaining)   // 0x10060150 (ret 0xc)
{
    GConsole->GetSkillCoolTime(SkillID, Total, Remaining);
}

/*-----------------------------------------------------------------------------
    NShortCutItem.
-----------------------------------------------------------------------------*/

// 0x100f62f0 - the entry's defaults and the cool time icon; the slot is not used
NShortCutItem::NShortCutItem()
{
    bUsed = 0;
}
NW_HOOK_CTOR(0x100f62f0, NShortCutItem)

// 0x100f6350 - the entry of the NItemInfo; the slot's own fields are cleared (not used), NItemInfo +0x1768 is kept
NShortCutItem::NShortCutItem(const NItemInfo& Info)
    : NItemEntry((NItemInfo*)&Info)
{
    bUsed = 0;
    bAutoShot = 0;
    Unknown1AAC = 0;
    Unknown1AB0 = 0;
    Unknown1A90 = Info.UnknownC34[12];
}
NW_HOOK_CTOR_ARGS(0x100f6350, NShortCutItem, (NItemInfo* A), (*A))

// 0x100f63d0 - the cool time icon's empty destructor, then the entry's (its tool tip)
NW_HOOK_DTOR(0x100f63d0, NShortCutItem)

// 0x100f2f50 (ret 4) - the entry, then the slot's fields; the cool time is stopped
void NShortCutItem::CopyFrom(NShortCutItem* Src)
{
    NItemEntry::Copy(Src);
    if (Src->GetIcon())
        Icon = Src->GetIcon();
    if (Src->GetName())
        appStrcpy(Name, Src->GetName());
    Type = Src->Type;
    ObjectID = Src->ObjectID;
    ID = Src->ID;
    Count = Src->Count;
    UnknownC18 = Src->UnknownC18;
    Enchant = Src->Enchant;
    bStackable = Src->bStackable;
    UnknownC34 = Src->UnknownC34;
    Unknown1A90 = Src->Unknown1A90;
    CrystalGrade = Src->CrystalGrade;
    bUsed = Src->bUsed;
    Owner = Src->Owner;
    CoolTime.Stop();
    Unknown1910 = Src->Unknown1910;
    bAutoShot = Src->bAutoShot;
    Unknown1AAC = 0;
    Unknown1AB0 = 0;
}
NW_HOOK_METHOD(0x100f2f50, NShortCutItem, CopyFrom)

// 0x100f3060 - the entry is reset, the slot is not used; the cool time is stopped
INT NShortCutItem::Clear()
{
    Reset();
    Unknown1A90 = 0;
    bUsed = 0;
    CoolTime.Stop();
    bAutoShot = 0;
    Unknown1AAC = 0;
    Unknown1AB0 = 0;
    return 0;   // the stock leaves whatever the cool time's Stop left in eax: the callers overwrite it
}
NW_HOOK_METHOD(0x100f3060, NShortCutItem, Clear)

/*-----------------------------------------------------------------------------
    NCShortCutWnd.
-----------------------------------------------------------------------------*/

// 0x100f6430 (ret 0xc) - NControl's stores inline in the stock code, then the textures: the slot frames, the twelve
// keys. C3: no key / icon texture of C2's, the slot's key is a texture of its own
NCShortCutWnd::NCShortCutWnd(INT InCols, INT InPages, INT InIconSize)
{
    Cols = InCols;
    IconSize = InIconSize;
    Keys = NULL;
    BackTex = NULL;
    Unknown1E8 = NULL;
    Pages = InPages;
    PressedSlot = -1;
    HoverSlot = -1;
    Page = 0;
    bHorizontal = 0;
    IconBackTex = FindTexture(TEXT("L2UI_CH3.Etc.menu_outline"));
    IconClickTex = FindTexture(TEXT("L2UI_CH3.Etc.menu_outline_down"));
    KeyTex[0] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f01"));
    KeyTex[1] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f02"));
    KeyTex[2] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f03"));
    KeyTex[3] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f04"));
    KeyTex[4] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f05"));
    KeyTex[5] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f06"));
    KeyTex[6] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f07"));
    KeyTex[7] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f08"));
    KeyTex[8] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f09"));
    KeyTex[9] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f10"));
    KeyTex[10] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f11"));
    KeyTex[11] = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_f12"));
    bPressed = 0;
    PadHold = 0;
}
NW_HOOK_CTOR_ARGS(0x100f6430, NCShortCutWnd, (INT A, INT B, INT C), (A, B, C))

// 0x100f6610 - ConditionalDestroy, then NControl's
NW_HOOK_DTOR(0x100f6610, NCShortCutWnd)

// 0x100f30a0 (vslot 25)
INT NCShortCutWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == (WPARAM)NID_SHORTCUT_NEXT)
            return OnNextClicked() != 0;
        if (wParam == (WPARAM)NID_SHORTCUT_PREV)
            return OnPrevClicked() != 0;
        if (wParam == (WPARAM)NID_SHORTCUT_ROTATE)
            return OnRotateClicked() != 0;
        if (wParam == (WPARAM)NID_SHORTCUT_PAD)
            return OnPadClicked() != 0;   // new in C2
    }
    return 0;
}
NW_HOOK_METHOD(0x100f30a0, NCShortCutWnd, WindowProc)

// NCFrameCtrl(Decorated, Vertical, Small): 0x10010450 (ret 0xc, NCFrameCtrl.cpp), run on the object `new` allocates
// (StaticAllocateObject of the class in the transient package)
static NCFrameCtrl* NewFrameCtrl(INT A, INT B, INT C)
{
    return new NCFrameCtrl(A, B, C);
}

// an arrow or the turn button, repeating every 300 ms while held (repeated inline in the stock OnCreate)
static FORCEINLINE NCPushButton* CreateBarButton(NCWnd* Parent, const TCHAR* NormalTex, const TCHAR* PushedTex,
                                                 INT ID, INT X, INT Y, DWORD W, DWORD H)
{
    NCPushButton* Button = new NCPushButton(NormalTex, PushedTex);
    Button->ID = ID;
    Button->SetPosition(X, Y, W, H);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->RepeatDelay = 300;
    return Button;
}

// 0x100f6660 (vslot 37) - the timer, the textures, the small title bar, the slots and their keys, and the four buttons;
// the game pad button starts hidden
INT NCShortCutWnd::OnCreate()
{
    NControl::OnCreate();
    TimerID = SetTimer(0x1F9, 200, NULL);
    BackTex = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_backv"));
    ToggleTex = FindTexture(TEXT("L2_SkillTime.ToggleEffect.ToggleEffect001"));
    PadHoldLTex = FindTexture(TEXT("L2UI_ch3.Joypad.joypad_L_HOLD"));
    PadHoldRTex = FindTexture(TEXT("L2UI_ch3.Joypad.joypad_R_HOLD"));
    PadLTex = FindTexture(TEXT("L2UI_ch3.Joypad.joypad_L"));
    PadRTex = FindTexture(TEXT("L2UI_ch3.Joypad.joypad_R"));

    FrameCtrl = NewFrameCtrl(1, 0, 1);
    FrameID = NID_SHORTCUT_FRAME;
    FrameCtrl->ID = NID_SHORTCUT_FRAME;
    FrameCtrl->SetPosition(0, 0, 0x2E, 0xC);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    Items = new NShortCutItem[Pages * Cols];
    Keys = new NShortCutKey[Pages * Cols];

    NextButton = CreateBarButton(this, TEXT("L2UI_CH3.ShortcutWnd.shortcut_nextv"),
                                 TEXT("L2UI_CH3.ShortcutWnd.shortcut_nextv_down"), NID_SHORTCUT_NEXT, 0x1F, 0xD, 0xE, 0xE);
    PrevButton = CreateBarButton(this, TEXT("L2UI_CH3.ShortcutWnd.shortcut_prevv"),
                                 TEXT("L2UI_CH3.ShortcutWnd.shortcut_prevv_down"), NID_SHORTCUT_PREV, 1, 0xD, 0xE, 0xE);
    RotateButton = CreateBarButton(this, TEXT("L2UI_CH3.ShortcutWnd.shortcut_rotate"),
                                   TEXT("L2UI_CH3.ShortcutWnd.shortcut_rotate_down"), NID_SHORTCUT_ROTATE, 0x10, 0x1E9, 0xF, 0xF);
    PadButton = CreateBarButton(this, TEXT("L2UI_CH3.ShortcutWnd.shortcut_joypad"),
                                TEXT("L2UI_CH3.ShortcutWnd.shortcut_joypad_down"), NID_SHORTCUT_PAD, 1, 0x1E9, 0xF, 0xF);
    PadButton->HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100f6660, NCShortCutWnd, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x100f6b50 (vslot 38) - the slots and their keys, then the four buttons
INT NCShortCutWnd::OnDestroy()
{
    guard(NCShortCutWnd::OnDestroy);
    if (Items)
    {
        delete[] Items;
        Items = NULL;
    }
    if (Keys)
    {
        delete[] Keys;
        Keys = NULL;
    }
    DestroyChild(this, NextButton);
    DestroyChild(this, PrevButton);
    DestroyChild(this, RotateButton);
    DestroyChild(this, PadButton);
    NControl::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100f6b50, NCShortCutWnd, OnDestroy)

// The macros of the console's macro window (NConsoleWnd::GetMacroCount, 0x1005f830, counts them, 0 without the window,
// GetMacro, 0x1006a480, gets one by its index, NULL out of range; the slot of a macro holds the macro's id in ID)
static FORCEINLINE INT MacroCount()
{
    return GConsole->GetMacroCount();
}
static FORCEINLINE NMacroEntry* MacroAt(INT Index)
{
    return GConsole->GetMacro(Index);
}

// The index of the macro whose id is MacroID, MacroCount() when there is none (the stock code's do-while: the first
// macro is looked at only when there is one)
static FORCEINLINE INT FindMacro(INT MacroID)
{
    INT Index = 0;
    if (MacroCount() > 0)
    {
        do
        {
            if (MacroID == MacroAt(Index)->ID)
                break;
            Index++;
        } while (Index < MacroCount());
    }
    return Index;
}

// a macro's short name, two letters a row from (X, Y) (the second letter of each row 13 pixels right of the first)
static FORCEINLINE void PaintMacroName(NCGDevice* Device, NShortCutItem* Item, INT X, INT Y)
{
    INT Index = FindMacro(Item->ID);
    if (Index < MacroCount())
    {
        const TCHAR* ShortName = MacroAt(Index)->GetShortName();
        if (ShortName)
        {
            FString Text(ShortName);
            for (INT i = 0; i < Text.Len(); i++)
            {
                Device->DrawText(X, Y, 0xFFDCDCDC, *Text.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
                if (i & 1)
                {
                    Y += 13;
                    X -= 13;
                }
                else
                    X += 13;
            }
        }
    }
}

// a used slot of the current page, drawn with its icon at (X, Y) (repeated for the three layouts in the stock
// painters): the icon, the bar's frame (the pad draws its own after the slots), the pressed mark, a macro's short name
// (from the icon's corner + 4), the toggle skill mark and the auto use mark, the flash when the cool time ends (with a
// sound) and the cool time itself
static FORCEINLINE void PaintSlot(NCShortCutWnd* Wnd, NCGDevice* Device, INT Slot, INT X, INT Y, UBOOL bFrame)
{
    NShortCutItem* Item = &Wnd->Items[Slot];
    INT Size = Wnd->IconSize;
    Device->DrawTexture(X, Y, Size, Size, 0, 0, Size, Size, Item->GetIcon(), 0xFF, 1);
    if (bFrame)
        Device->DrawTexture(X - 1, Y - 1, 0x22, 0x22, 0, 0, 0x22, 0x22, Wnd->IconBackTex, 0xFF, 1);
    if (Wnd->PressedSlot == Slot && Wnd->bPressed)
        Device->DrawTexture(X - 1, Y - 1, 0x22, 0x22, 0, 0, 0x22, 0x22, Wnd->IconClickTex, 0xFF, 1);
    if (Item->Kind() == SHORTCUT_Macro)
        PaintMacroName(Device, Item, X + 4, Y + 4);
    if (GConsole->IsToggleSkill(Item->ID, Item->UnknownC18) && GConsole->HasAbnormal(Item->ID))
    {
        Wnd->ToggleTex->UnknownA4 = 9.0f;
        Device->DrawTexture(X, Y, 0x20, 0x20, 0, 0, 0x20, 0x20, Wnd->ToggleTex, 0xFF, 1);
    }
    if (Item->bAutoShot)
    {
        Wnd->ToggleTex->UnknownA4 = 9.0f;
        Device->DrawTexture(X, Y, 0x20, 0x20, 0, 0, 0x20, 0x20, Wnd->ToggleTex, 0xFF, 1);
    }
    if (Item->CoolTime.bFlashing)
    {
        Size = Wnd->IconSize;
        Device->DrawBrightenTexture(X, Y, Size, Size, 0, 0, Size, Size, Item->CoolTime.GetFlashTexture(), 0xFF008080);
        if (Item->CoolTime.bSoundDue)
        {
            GConsole->PlayInterfaceSound(SOUND_CoolTimeEnd);
            Item->CoolTime.bSoundDue = 0;
        }
    }
    if (Item->CoolTime.bRunning && !Item->CoolTime.IsIdle() && Item->CoolTime.GetTexture())
    {
        Size = Wnd->IconSize;
        Device->DrawTexture(X, Y, Size, Size, 0, 0, Size, Size, Item->CoolTime.GetTexture(), 0xFF, 1);
    }
}

// the page's number in the large digits, centered at CenterX (the tenth page, two digits, from X10 and X0)
static FORCEINLINE void PaintPageNumber(NCGDevice* Device, INT PageNumber, INT CenterX, INT X10, INT X0, INT Y)
{
    TCHAR Text[6];
    appSprintf(Text, TEXT("%d"), PageNumber);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0, L2FONT_Normal);
    if (PageNumber < 10)
        Device->DrawSpecialDigit(1, CenterX - Size.cx / 2, Y, 0xFFDCDCDC, Text);
    else if (PageNumber == 10)
    {
        appSprintf(Text, TEXT("%d"), 1);
        Device->DrawSpecialDigit(1, X10, Y, 0xFFDCDCDC, Text);
        appSprintf(Text, TEXT("%d"), 0);
        Device->DrawSpecialDigit(1, X0, Y, 0xFFDCDCDC, Text);
    }
}

// 0x100f4030 - the vertical bar (46 x 504: the slots at X 7, from Y 33) or the horizontal one (504 x 46: from X 33, at Y 7);
// the slots go 5 pixels apart, with 5 more after the fourth and the eighth. Each slot ends with its key's texture
void NCShortCutWnd::PaintBar(NCGDevice* Device)
{
    if (bHorizontal)
    {
        if (BackTex)
            Device->DrawTexture(0xC, 0, 0x1EC, 0x2E, 0, 0, 0x1EC, 0x2E, BackTex, 0xFF, 1);
        PaintPageNumber(Device, Page + 1, 0x13, 0xE, 0x12, 0x12);
    }
    else
    {
        if (BackTex)
            Device->DrawTexture(0, 0xC, 0x2E, 0x1EC, 0, 0, 0x2E, 0x1EC, BackTex, 0xFF, 1);
        PaintPageNumber(Device, Page + 1, 0x16, 0x11, 0x15, 0x10);
    }
    INT First = Page * Cols;
    INT Pos = 0x21;
    for (INT i = 0; i < Cols; i++)
    {
        if (Items[First + i].bUsed)
        {
            if (bHorizontal)
                PaintSlot(this, Device, First + i, Pos, 7, 1);
            else
                PaintSlot(this, Device, First + i, 7, Pos, 1);
        }
        if (bHorizontal)
            Device->DrawTexture(Pos - 1, 4, 0x10, 0x10, 0, 0, 0x10, 0x10, KeyTex[i], 0xFF, 1);
        else
            Device->DrawTexture(4, Pos - 1, 0x10, 0x10, 0, 0, 0x10, 0x10, KeyTex[i], 0xFF, 1);
        Pos += IconSize + 5;
        if (i == 3 || i == 7)
            Pos += 5;
    }
}
NW_HOOK_METHOD(0x100f4030, NCShortCutWnd, PaintBar)

// 0x100f4b70 - the game pad layout (172 x 82): the pad's back, the L and R marks (lit for the hold), the page number
// and the four slots of the current hold (a diamond: left, bottom, right, top), each with its frame and key texture
void NCShortCutWnd::PaintPad(NCGDevice* Device)
{
    if (BackTex)
        Device->DrawTexture(0xC, 0, 0xA0, 0x52, 0, 0, 0xA0, 0x52, BackTex, 0xFF, 1);
    if (PadHold == 1)
    {
        Device->DrawTexture(0x26, 6, 0x10, 0xE, 0, 0, 0x10, 0xE, PadHoldLTex, 0xFF, 1);
        Device->DrawTexture(0x82, 6, 0x10, 0xE, 0, 0, 0x10, 0xE, PadRTex, 0xFF, 1);
    }
    else if (PadHold == 2)
    {
        Device->DrawTexture(0x82, 6, 0x10, 0xE, 0, 0, 0x10, 0xE, PadHoldRTex, 0xFF, 1);
        Device->DrawTexture(0x26, 6, 0x10, 0xE, 0, 0, 0x10, 0xE, PadLTex, 0xFF, 1);
    }
    else if (PadHold == 0)
    {
        Device->DrawTexture(0x26, 6, 0x10, 0xE, 0, 0, 0x10, 0xE, PadLTex, 0xFF, 1);
        Device->DrawTexture(0x82, 6, 0x10, 0xE, 0, 0, 0x10, 0xE, PadRTex, 0xFF, 1);
    }
    PaintPageNumber(Device, Page + 1, 0x13, 0xD, 0x11, 0x25);

    INT X = 0x27;
    INT Y = 0x19;
    for (INT k = PadHold * 4; k < PadHold * 4 + 4; k++)
    {
        if (k >= 12)
            return;
        switch (k & 3)
        {
        case 0:
            X = 0x27;
            Y = 0x19;
            break;
        case 1:
            X = 0x4C;
            Y = 0x2B;
            break;
        case 2:
            X = 0x71;
            Y = 0x19;
            break;
        case 3:
            X = 0x4C;
            Y = 7;
            break;
        }
        INT Slot = Cols * Page + k;
        if (Items[Slot].bUsed)
            PaintSlot(this, Device, Slot, X, Y, 0);
        Device->DrawTexture(X - 1, Y - 1, 0x22, 0x22, 0, 0, 0x22, 0x22, IconBackTex, 0xFF, 1);
        Device->DrawTexture(X - 1, Y - 1, 0x10, 0x10, 0, 0, 0x10, 0x10, KeyTex[k], 0xFF, 1);
    }
}
NW_HOOK_METHOD(0x100f4b70, NCShortCutWnd, PaintPad)

// 0x100f5280 (vslot 56) - the bar or the pad inside the window's clip
INT NCShortCutWnd::OnPaint(NCGDevice* Device)
{
    guard(NCShortCutWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (bPadMode == 0)
        PaintBar(Device);
    else if (bPadMode == 1)
        PaintPad(Device);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100f5280, NCShortCutWnd, OnPaint)

// 0x100f6cd0 (vslot 57) - the tool tip of the slot under the mouse: the full box of its tip, beside the slot (X 7, or
// Y 7 on the horizontal bar, the pad's own places)
INT NCShortCutWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (bPadMode == 0)
    {
        if (HoverSlot >= 0 && Items[HoverSlot].bUsed)
        {
            INT Rel = HoverSlot - Page * Cols;
            INT Pos = (IconSize + 5) * Rel + 0x21;
            if (Rel >= 4)
                Pos += 5;
            if (Rel >= 8)
                Pos += 5;
            if (ToolTipSkin)
            {
                NToolTipRect Wnd;
                Wnd.X = ScreenX;
                Wnd.Y = ScreenY;
                Wnd.W = Width;
                Wnd.H = Height;
                if (bHorizontal == 0)
                    ToolTipSkin->DrawFull(Device, Items[HoverSlot].ToolTip, 7, Pos, Wnd);
                else
                    ToolTipSkin->DrawFull(Device, Items[HoverSlot].ToolTip, Pos, 7, Wnd);
            }
        }
    }
    else if (bPadMode == 1 && HoverSlot >= 0 && Items[HoverSlot].bUsed)
    {
        INT X = 0, Y = 0;
        switch (HoverSlot % 4)
        {
        case 0:
            X = 0x23;
            Y = 0x1A;
            break;
        case 1:
            X = 0x49;
            Y = 0x2C;
            break;
        case 2:
            X = 0x6F;
            Y = 0x1A;
            break;
        case 3:
            X = 0x49;
            Y = 7;
            break;
        }
        if (ToolTipSkin)
        {
            NToolTipRect Wnd;
            Wnd.X = ScreenX;
            Wnd.Y = ScreenY;
            Wnd.W = Width;
            Wnd.H = Height;
            ToolTipSkin->DrawFull(Device, Items[HoverSlot].ToolTip, X, Y, Wnd);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x100f6cd0, NCShortCutWnd, OnDrawToolTip)

// 0x100f5360 (vslot 62) - the slot under the mouse is pressed when it holds something
INT NCShortCutWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Slot = SlotAtPoint(LOWORD(lParam), HIWORD(lParam));
    PressedSlot = Slot;
    HoverSlot = Slot;
    if (Slot >= 0 && Items[Slot].bUsed)
    {
        bDragging = 1;
        bPressed = 1;
    }
    return 1;
}
NW_HOOK_METHOD(0x100f5360, NCShortCutWnd, OnLButtonDown)

// 0x100f53c0 (vslot 61) - the hover slot; while the slot is dragged the cursor is its icon, and over a line of the
// macro editor the line is marked (every other mark of the editor goes)
INT NCShortCutWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    HoverSlot = SlotAtPoint(X, Y);
    if (IsFocused() && bDragging)
    {
        if (PressedSlot >= 0 && Items[PressedSlot].GetIcon())
        {
            SetCursor(4, Items[PressedSlot].GetIcon());
            LockCursor();
        }
        NWndEntry* Found = FindWindowAt(X, Y);
        if (Found && Found->Wnd && Found->Wnd->IsA(NClass::NCEditBox()))
        {
            NWndEntry* Parent = Found->Parent;
            if (Parent && Parent->Parent && Parent->Parent->Wnd && Parent->Parent->Wnd->IsA(NClass::NCMacroEditWnd()))
            {
                NCMacroEditWnd* EditWnd = (NCMacroEditWnd*)Parent->Parent->Wnd;
                NCConsole* Desktop = GDesktop;
                if (Desktop)
                {
                    if ((NCMacroEditWnd*)Desktop->NewWndFC == EditWnd)
                    {
                        INT Line = EditWnd->EditList->HitLine(X, Y);
                        if (Line < 0)
                            return 1;
                        EditWnd->EditList->Edits(Line)->Unknown1D8 = 1;
                        return 1;
                    }
                    EditWnd = (NCMacroEditWnd*)Desktop->NewWndFC;
                }
                if (EditWnd)
                    EditWnd->ClearMarks();
                return 1;
            }
        }
        if (GConsole && GDesktop && GDesktop->NewWndFC)
            ((NCMacroEditWnd*)GDesktop->NewWndFC)->ClearMarks();
    }
    return 1;
}
NW_HOOK_METHOD(0x100f53c0, NCShortCutWnd, OnMouseMove)

// An item with a warning asks before it is used: system message 0x31E with the item's name on the button; the answer
// comes back to the bar's entry with the item's object id (repeated inline in the stock OnLButtonUp and
// OnShortCutKeyDown)
static FORCEINLINE void ShowItemWarning(NCShortCutWnd* Wnd, NShortCutItem* Item)
{
    TCHAR Name[64];
    appSprintf(Name, TEXT("%s"), Item->NItemEntry::GetName());
    NDialogInfo Info;
    Info.ID = DIALOG_CONFIRM_ITEM;
    Info.Owner = Wnd->Entry;
    Info.bMainBox = 1;
    Info.bInput = 0;
    Info.SetSystemMessage(0x31E);
    Info.SetButtons(Name, NULL);
    Info.SetParams(Item->ObjectID, -1);
    GConsole->ShowDialog(&Info);
}

// An item with a message of its own shows it (system message number from the item: NItemEntry::GetUnknown19AC)
static FORCEINLINE void ShowItemMessage(NCShortCutWnd* Wnd, NShortCutItem* Item)
{
    NDialogInfo Info;
    Info.ID = DIALOG_ITEM_MESSAGE;
    Info.Owner = Wnd->Entry;
    Info.bMainBox = 1;
    Info.bInput = 0;
    Info.SetSystemMessage(Item->GetUnknown19AC());
    Info.SetParams(Item->ObjectID, -1);
    GConsole->ShowDialog(&Info);
}

// 0x100f5550 (vslot 63) - the release of a dragged slot: on the slot itself the item is used (after its warning or its
// message), on a line of the macro editor the line gets "/<command> <page> <slot>" typed in, and more than 30 pixels
// off the bar the shortcut is deleted
INT NCShortCutWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = (INT)((DWORD)lParam >> 16);
    bPressed = 0;
    INT Pressed = PressedSlot;
    if (Pressed < 0)
    {
        bDragging = 0;
        return 1;
    }
    INT Over = SlotAtPoint(X, Y);
    if (Keys[Pressed].bReady != 0)
    {
        if (Over == Pressed && Items[Pressed].bUsed)
        {
            NShortCutItem* Item = &Items[Pressed];
            if (Item->Kind() == SHORTCUT_Item && Item->Unknown1998 != 0 && AsksBeforeUse())
                ShowItemWarning(this, &Items[PressedSlot]);
            else if (Items[PressedSlot].GetUnknown19AC() > 0)
                ShowItemMessage(this, &Items[PressedSlot]);
            else if (PressedSlot >= 0)
            {
                GConsole->UseShortCut(PressedSlot, 0, 0);
                Keys[PressedSlot].bReady = 0;
                Keys[PressedSlot].OnKeyUp();
            }
        }
        else if (bDragging)
        {
            NWndEntry* Found = FindWindowAt(X, Y);
            if (Found && Found->Wnd && Found->Wnd->IsA(NClass::NCEditBox()))
            {
                // over a line box of the macro editor (its list is the box's parent's parent's child)
                if (Found->Parent && Found->Parent->Parent && Found->Parent->Parent->Wnd
                    && Found->Parent->Parent->Wnd->IsA(NClass::NCMacroEditWnd()))
                {
                    NCMacroEditWnd* EditWnd = (NCMacroEditWnd*)Found->Parent->Parent->Wnd;
                    if (GDesktop && GDesktop->NewWndFC == (NCWnd*)EditWnd)
                    {
                        INT Line = EditWnd->EditList->HitLine(X, Y);
                        if (Line >= 0)
                        {
                            TCHAR Command[0x400];
                            appSprintf(Command, TEXT("%s%s %d %d"), GShortCutCmdPrefix,
                                       *GL2GameData.Unknown54188[28], PressedSlot / 12 + 1, PressedSlot % 12 + 1);
                            if (appStrlen(Command) <= 0x20)
                            {
                                EditWnd->EditList->Edits(Line)->ClearText();
                                EditWnd->EditList->Edits(Line)->InsertText(Command, 1);
                            }
                            EditWnd->ClearMark(Line);
                        }
                    }
                }
            }
            else
            {
                // dropped off the bar: 30 pixels of room around it, then the slot is deleted
                INT LX = X;
                INT LY = Y;
                if (X < ScreenX)
                    LX = X + 0x1E;
                else if ((DOUBLE)ScreenX + (DOUBLE)Width < (DOUBLE)X)
                    LX = X - 0x1E;
                if (Y < ScreenY)
                    LY = Y + 0x1E;
                else if ((DOUBLE)ScreenY + (DOUBLE)Height < (DOUBLE)Y)
                    LY = Y - 0x1E;
                if (!IsInWindow((WORD)LX, (WORD)LY))
                {
                    L2ParamStack Params(10);
                    Params.PushBack((void*)(INT_PTR)PressedSlot);
                    GConsole->RequestShortCutDel(Params);
                    Items[PressedSlot].Clear();
                    PressedSlot = -1;
                }
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100f5550, NCShortCutWnd, OnLButtonUp)

// 0x100f3f00 (vslot 64) - a right click on a shot slot flips its auto use
INT NCShortCutWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Slot = SlotAtPoint(LOWORD(lParam), HIWORD(lParam));
    if (Slot >= 0 && Items[Slot].bUsed && IsShotItem(Items[Slot].ID))
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Items[Slot].ID);
        Params.PushBack((void*)(INT_PTR)(Items[Slot].bAutoShot == 0));
        RequestAutoShot(Params);
    }
    return 1;
}
NW_HOOK_METHOD(0x100f3f00, NCShortCutWnd, OnRButtonDown)

// 0x100f6270 (vslot 72)
INT NCShortCutWnd::OnNextClicked()
{
    bDragging = 0;
    NextPage();
    return 1;
}
NW_HOOK_METHOD(0x100f6270, NCShortCutWnd, OnNextClicked)

// 0x100f6290 (vslot 73)
INT NCShortCutWnd::OnPrevClicked()
{
    bDragging = 0;
    PrevPage();
    return 1;
}
NW_HOOK_METHOD(0x100f6290, NCShortCutWnd, OnPrevClicked)

// 0x100f3af0 (vslot 74) - turn the bar
INT NCShortCutWnd::OnRotateClicked()
{
    bDragging = 0;
    bHorizontal = (bHorizontal == 0);
    UpdateLayout();
    return 1;
}
NW_HOOK_METHOD(0x100f3af0, NCShortCutWnd, OnRotateClicked)

// 0x100f36c0 (ret 4) - the slots of this inventory item follow its enchant (when it has one) or its count
INT NCShortCutWnd::UpdateItem(INT ObjectID)
{
    for (INT i = 0; i < Cols * Pages; i++)
    {
        NShortCutItem* Slot = &Items[i];
        if (Slot->bUsed && Slot->Kind() == SHORTCUT_Item && Slot->ObjectID == ObjectID)
        {
            NItemEntry* Item = GDesktop->InventoryWnd->FindItem(ObjectID);
            if (!Item)
                Slot->Clear();
            else if (Item->Enchant > 0)
            {
                Slot->Enchant = Item->Enchant;
                Slot->ToolTip.Fill(Slot, NTT_ShortCut, 0);
            }
            else if (Item->IsStackable())
            {
                Slot->Count = Item->Count;
                Slot->ToolTip.Fill(Slot, NTT_ShortCut, 0);
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100f36c0, NCShortCutWnd, UpdateItem)

// 0x100f37e0 (ret 4) - the same for the pet's inventory
INT NCShortCutWnd::UpdatePetItem(INT ObjectID)
{
    for (INT i = 0; i < Cols * Pages; i++)
    {
        NShortCutItem* Slot = &Items[i];
        if (Slot->bUsed && Slot->Kind() == SHORTCUT_Item && Slot->ObjectID == ObjectID)
        {
            NItemEntry* Item = GDesktop->PetWnd->InventoryWnd->GetItem(ObjectID);
            if (!Item)
                Slot->Clear();
            else
            {
                if (Item->Enchant > 0)
                    Slot->Enchant = Item->Enchant;
                else if (Item->IsStackable())
                    Slot->Count = Item->Count;
                else
                    continue;
                Slot->ToolTip.Fill(Slot, NTT_ShortCut, 0);
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100f37e0, NCShortCutWnd, UpdatePetItem)

// 0x100f38e0 (ret 8) - the slot of the current page under a point of the bar (-1 none): the slots are IconSize + 5 apart
// from 33 (5 more after the fourth and the eighth) and a few pixels off the bar's edge count
INT NCShortCutWnd::SlotAt(INT X, INT Y)
{
    INT Result = -1;
    if (bHorizontal == 0)
    {
        INT RelX = X - ScreenX;
        INT RelY = Y - ScreenY;
        INT Pos = 0x21;
        if (RelX > 5 && RelX <= IconSize + 6)
        {
            for (INT i = 0; i < Cols; i++)
            {
                if (RelY >= Pos && RelY <= IconSize + Pos)
                    Result = Page * Cols + i;
                Pos += IconSize + 5;
                if (i == 3 || i == 7)
                    Pos += 5;
            }
        }
    }
    else
    {
        INT RelX = X - ScreenX;
        INT RelY = Y - ScreenY;
        INT Pos = 0x21;
        if (RelY > 5 && RelY <= IconSize + 6)
        {
            for (INT i = 0; i < Cols; i++)
            {
                if (RelX >= Pos && RelX <= IconSize + Pos)
                    Result = Page * Cols + i;
                Pos += IconSize + 5;
                if (i == 3 || i == 7)
                    Pos += 5;
            }
        }
    }
    return Result;
}
NW_HOOK_METHOD(0x100f38e0, NCShortCutWnd, SlotAt)

// 0x100f39e0 (ret 8) - the four slots of the pad (a diamond), numbered from the hold's first: -1 none
INT NCShortCutWnd::PadSlotAt(INT X, INT Y)
{
    INT RelX = X - ScreenX;
    INT RelY = Y - ScreenY;
    for (INT i = 0; i < 4; i++)
    {
        INT BoxX = 0;
        INT BoxY = 0;
        switch (i)
        {
        case 0:
            BoxX = 0x27;
            BoxY = 0x19;
            break;
        case 1:
            BoxX = 0x4C;
            BoxY = 0x2B;
            break;
        case 2:
            BoxX = 0x71;
            BoxY = 0x19;
            break;
        case 3:
            BoxX = 0x4C;
            BoxY = 7;
            break;
        }
        if (RelX >= BoxX && RelX < IconSize + BoxX && RelY >= BoxY && RelY <= IconSize + BoxY)
            return i + Page * Cols + PadHold * 4;
    }
    return -1;
}
NW_HOOK_METHOD(0x100f39e0, NCShortCutWnd, PadSlotAt)

// 0x100f3aa0 (ret 8) - by the layout: the bar's slots, the pad's, or none
INT NCShortCutWnd::SlotAtPoint(INT X, INT Y)
{
    if (bPadMode == 0)
        return SlotAt(X, Y);
    if (bPadMode == 1)
        return PadSlotAt(X, Y);
    return -1;
}
NW_HOOK_METHOD(0x100f3aa0, NCShortCutWnd, SlotAtPoint)

// 0x100f3420 (ret 4, name ours) - one of the 120 slots holds this item and its auto use is on
UBOOL NCShortCutWnd::IsAutoShotSlot(INT ItemID)
{
    for (INT i = 0; i < 0x78; i++)
    {
        if (Items[i].ID == ItemID && Items[i].bAutoShot)
            return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x100f3420, NCShortCutWnd, IsAutoShotSlot)

// 0x100f3460 (ret 8) - a macro of the console: Mode 2, its slots take its icon, name and tool tip; Mode 0, the slots
// are deleted (the server is told)
INT NCShortCutWnd::UpdateMacro(INT MacroID, INT Mode)
{
    if (Mode == 2)
    {
        for (INT i = 0; i < Cols * Pages; i++)
        {
            NShortCutItem* Slot = &Items[i];
            if (Slot->bUsed && Slot->Kind() == SHORTCUT_Macro && Slot->ID == MacroID)
            {
                INT Index = FindMacro(MacroID);
                if (Index < MacroCount())
                {
                    INT IconType = GConsole->GetMacroIconType(MacroID);
                    appSprintf(Slot->IconName, TEXT("%s%d"), TEXT("L2UI.MacroWnd.Macro_Icon"), IconType + 1);
                    Slot->Icon = FindTexture(Slot->IconName);
                    TCHAR* Name = GConsole->GetMacroName(MacroID);
                    appSprintf(Slot->Name, TEXT("%s"), Name);
                    Slot->ToolTip.Fill(Slot, NTT_ShortCut, 0);
                }
            }
        }
    }
    else if (Mode == 0)
    {
        for (INT i = 0; i < Cols * Pages; i++)
        {
            NShortCutItem* Slot = &Items[i];
            if (Slot->bUsed && Slot->Kind() == SHORTCUT_Macro && Slot->ID == MacroID)
            {
                L2ParamStack Params(10);
                Params.PushBack((void*)(INT_PTR)i);
                GConsole->RequestShortCutDel(Params);
                Slot->Clear();
                PressedSlot = -1;
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100f3460, NCShortCutWnd, UpdateMacro)

// 0x100f4000 (name ours) - C3: a key that waits for the cool time is ready again on the release too
void NShortCutKey::OnKeyUp()
{
    if ((bOnKeyUp && !bReady) || (bOnCoolTime && !bReady))
        bReady = 1;
}
NW_HOOK_METHOD(0x100f4000, NShortCutKey, OnKeyUp)

// 0x100f3150 (vslot 69) - the bar after a turn (bHorizontal says which): the corner of the old bar keeps its place (the
// window turns around its top left corner's offset by the size difference, not off the screen), the size, the small
// title bar's and the buttons' places and textures. The game pad button's textures are set the same way twice (the stock
// code calls the pushed-texture setter for both)
void NCShortCutWnd::UpdateLayout()
{
    DOUBLE NewX = (DOUBLE)ScreenX + (DOUBLE)Width - (DOUBLE)Height;
    if (bHorizontal == 0)
    {
        INT X = (INT)NewX;
        INT Y = (INT)((DOUBLE)ScreenY + (DOUBLE)Height - (DOUBLE)Width);
        if (Y < 0)
            Y = 0;
        SetPosition(X, Y, 0x2E, 0x1F8);
        SetWindowSize(46.0f, 504.0f);
        FrameCtrl->SetPosition(0, 0, 0x2E, 0xC);
        FrameCtrl->bVertical = 0;
        FrameCtrl->bSmall = 1;
        BackTex = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_backv"));
        NextButton->SetPosition(0x1F, 0xD, 0xE, 0xE);
        PrevButton->SetPosition(1, 0xD, 0xE, 0xE);
        RotateButton->SetPosition(0x10, 0x1E9, 0xF, 0xF);
        PadButton->SetPosition(1, 0x1E9, 0xF, 0xF);
        NextButton->SetNormalTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_next"));
        NextButton->SetPushedTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_next_down"));
        PrevButton->SetNormalTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_prev"));
        PrevButton->SetPushedTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_prev_down"));
        RotateButton->SetNormalTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_rotate"));
        RotateButton->SetPushedTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_rotate_down"));
        PadButton->SetPushedTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_joypad"));
        PadButton->SetPushedTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_joypad_down"));
    }
    else
    {
        INT X = (INT)NewX;
        INT Y = (INT)((DOUBLE)ScreenY + (DOUBLE)Height - (DOUBLE)Width);
        if (X < 0)
            X = 0;
        SetPosition(X, Y, 0x1F8, 0x2E);
        SetWindowSize(504.0f, 46.0f);
        FrameCtrl->SetPosition(0, 0, 0xC, 0x2E);
        FrameCtrl->bVertical = 1;
        FrameCtrl->bSmall = 1;
        BackTex = FindTexture(TEXT("L2UI_CH3.ShortcutWnd.shortcut_back"));
        PrevButton->SetPosition(0xD, 0x1F, 0xE, 0xE);
        NextButton->SetPosition(0xD, 1, 0xE, 0xE);
        RotateButton->SetPosition(0x1E9, 0x10, 0xF, 0xF);
        PadButton->SetPosition(0x1E9, 1, 0xF, 0xF);
        NextButton->SetNormalTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_nextv"));
        NextButton->SetPushedTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_nextv_down"));
        PrevButton->SetNormalTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_prevv"));
        PrevButton->SetPushedTex(TEXT("L2UI_CH3.ShortcutWnd.shortcut_prevv_down"));
    }
    if (GL2UsingJoystick && GL2CanUseJoystick)
        PadButton->ShowWindow();
    else
        PadButton->HideWindow();
}
NW_HOOK_METHOD(0x100f3150, NCShortCutWnd, UpdateLayout)

// 0x100f3d00 (vslot 70)
void NCShortCutWnd::SetPadHoldL(bool bHold)
{
    PadHold = bHold;
}
NW_HOOK_METHOD(0x100f3d00, NCShortCutWnd, SetPadHoldL)

// 0x100f3d20 (vslot 71)
void NCShortCutWnd::SetPadHoldR(bool bHold)
{
    PadHold = bHold ? 2 : 0;
}
NW_HOOK_METHOD(0x100f3d20, NCShortCutWnd, SetPadHoldR)

// 0x100f3d40 (vslot 75) - to the game pad layout (only while a game pad is in use: the pad's back, the arrows beside
// it, the turn button gone, 172 x 82) or back to the bar
INT NCShortCutWnd::OnPadClicked()
{
    if (bPadMode == 0)
    {
        if (GL2UsingJoystick && GL2CanUseJoystick)
        {
            BackTex = FindTexture(TEXT("L2UI_CH3.Joypad.joypad_back"));
            NextButton->SetNormalTex(TEXT("L2UI_ch3.shortcutwnd.shortcut_nextv"));
            NextButton->SetPushedTex(TEXT("L2UI_ch3.shortcutwnd.shortcut_nextv_down"));
            PrevButton->SetNormalTex(TEXT("L2UI_ch3.shortcutwnd.shortcut_prevv"));
            PrevButton->SetPushedTex(TEXT("L2UI_ch3.shortcutwnd.shortcut_prevv_down"));
            PadButton->SetPushedTex(TEXT("L2UI_ch3.shortcutwnd.shortcut_joypad"));
            PadButton->SetPushedTex(TEXT("L2UI_ch3.shortcutwnd.shortcut_joypad_down"));
            FrameCtrl->bVertical = 1;
            FrameCtrl->bSmall = 1;
            PadButton->SetPosition(0x9D, 0x22, 0xF, 0xF);
            PadButton->ShowWindow();
            NextButton->SetPosition(0xD, 0x13, 0xF, 0xF);
            PrevButton->SetPosition(0xD, 0x31, 0xF, 0xF);
            RotateButton->HideWindow();
            FrameCtrl->SetPosition(0, 0, 0xC, 0x52);
            SetWindowSize(172.0f, 82.0f);
            PadHold = 0;
            bPadMode = 1;
            return 1;
        }
    }
    else if (bPadMode == 1)
    {
        bPadMode = 0;
        RotateButton->ShowWindow();
        UpdateLayout();
    }
    return 1;
}
NW_HOOK_METHOD(0x100f3d40, NCShortCutWnd, OnPadClicked)

// 0x100f3ec0 (vslot 44) - the pad button shows with the bar only while the game pad is in use
void NCShortCutWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (GL2UsingJoystick && GL2CanUseJoystick)
        PadButton->ShowWindow();
    else
        PadButton->HideWindow();
}
NW_HOOK_METHOD(0x100f3ec0, NCShortCutWnd, ShowWindow)

// 0x100f3b20 - after a language change the name of every slot is read again from its table and the tip rebuilt
void NCShortCutWnd::ReloadNames()
{
    for (INT i = 0; i < Cols * Pages; i++)
    {
        NShortCutItem* Slot = &Items[i];
        if (!Slot->bUsed)
            continue;
        if (Slot->Kind() == SHORTCUT_Item && GConsole->GetItemName(Slot->ID))
            appStrcpy(Slot->Name, GConsole->GetItemName(Slot->ID));
        else if (Slot->Kind() == SHORTCUT_Skill && GConsole->GetSkillName(Slot->ID, Slot->UnknownC18))
            appStrcpy(Slot->Name, GConsole->GetSkillName(Slot->ID, Slot->UnknownC18));
        else if (Slot->Kind() == SHORTCUT_Action && GConsole->GetActionName(Slot->ID))
            appStrcpy(Slot->Name, GConsole->GetActionName(Slot->ID));
        else if (Slot->Kind() == SHORTCUT_Macro && GConsole->GetMacroName(Slot->ID))
            appStrcpy(Slot->Name, GConsole->GetMacroName(Slot->ID));
        else
            continue;
        Slot->ToolTip.Fill(Slot, NTT_ShortCut, 0);
    }
}
NW_HOOK_METHOD(0x100f3b20, NCShortCutWnd, ReloadNames)

// 0x100f3ac0 - the slot of the current page's Index
INT NCShortCutWnd::PageSlot(INT Index)
{
    if (!Page)
        return Index;
    return Cols * Page + Index;
}
NW_HOOK_METHOD(0x100f3ac0, NCShortCutWnd, PageSlot)

// 0x100f5a30 - the slot's item cleared, its key ready again
__declspec(noinline) INT NCShortCutWnd::DeleteShortCut(INT Slot)
{
    if (Pages * Cols <= Slot)
        return 0;
    ItemAt(Slot)->Clear();
    Keys[Slot] = NShortCutKey();
    return 1;
}
NW_HOOK_METHOD(0x100f5a30, NCShortCutWnd, DeleteShortCut)

// 0x100f5a90 - leaving a page makes its keys ready again
void NCShortCutWnd::SetPage(INT NewPage)
{
    if (NewPage < 0 || NewPage >= Pages)
        return;
    if (NewPage != Page)
    {
        for (INT i = 0; i < Cols; i++)
        {
            Keys[Page * Cols + i].bReady = 1;
            Keys[Page * Cols + i].Elapsed = 0;
        }
    }
    Page = NewPage;
}
NW_HOOK_METHOD(0x100f5a90, NCShortCutWnd, SetPage)

// 0x100f5b00 - the keys' clocks: a slot with a skill's cool time waits for it to end, the others a fixed time
void NCShortCutWnd::Tick(FLOAT DeltaTime)
{
    INT Delta = (INT)(DeltaTime * 1000.0f);
    for (INT i = 0; i < Pages * Cols; i++)
    {
        NShortCutItem* Slot = &Items[i];
        NShortCutKey* Key = &Keys[i];
        if (Slot->ID > 0)
        {
            if (Key->bOnCoolTime)
            {
                if (Slot->CoolTime.IsIdle() && Key->bOnCoolTime && !Key->bReady)
                    Key->bReady = 1;
            }
            else if (!Key->bOnKeyUp && !Key->bOnCoolTime && !Key->bReady)
            {
                Key->Elapsed += Delta;
                if (Key->Elapsed > Key->Interval)
                {
                    Key->Elapsed = 0;
                    Key->bReady = 1;
                }
            }
        }
    }
}
NW_HOOK_METHOD(0x100f5b00, NCShortCutWnd, Tick)

// the slot a key uses on the current page: F1 .. F12, or 1 .. 9, 0, -, = while the chat line needs Enter (and no edit
// box has the focus); -1 for other keys (repeated inline in both key handlers)
static FORCEINLINE INT KeySlot(NCShortCutWnd* Wnd, UINT Key, UBOOL bDown)
{
    INT Slot = -1;
    if (Key >= VK_F1 && Key <= VK_F12)
        Slot = Wnd->Page * Wnd->Cols + Key - VK_F1;
    if (GIsEnterChatting && (!GFocusEntry || !GFocusEntry->Wnd->vfn032()))
    {
        switch (Key)
        {
        case '0':
            Slot = Wnd->Page * Wnd->Cols + 9;
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            Slot = Wnd->Page * Wnd->Cols + Key - '1';
            break;
        case 0xBB:   // VK_OEM_PLUS ("=")
            Slot = Wnd->Page * Wnd->Cols + 0xB;
            break;
        case 0xBD:   // VK_OEM_MINUS ("-")
            Slot = Wnd->Page * Wnd->Cols + 10;
            break;
        }
    }
    return Slot;
}

// 0x100f5be0 (ret 8) - a key released: its slot's key is ready again when it waited for that
INT NCShortCutWnd::OnShortCutKeyUp(UINT Key, LPARAM lParam)
{
    INT Slot = KeySlot(this, Key, 0);
    if (Slot >= 0)
    {
        NShortCutKey* K = &Keys[Slot];
        if ((K->bOnKeyUp && !K->bReady) || (K->bOnCoolTime && !K->bReady))
            K->bReady = 1;
        IsShotItem(Items[Slot].ID);
    }
    return 0;
}
NW_HOOK_METHOD(0x100f5be0, NCShortCutWnd, OnShortCutKeyUp)

// 0x100f5d90 (ret 8; guard name UseShortCutItem) - a key pressed: an empty slot beeps, an item with a warning asks, an item
// with a message of its own shows it, anything else is used and its key waits. 1 when the key was not ready
INT NCShortCutWnd::OnShortCutKeyDown(UINT Key, LPARAM lParam)
{
    guard(NCShortCutWnd::UseShortCutItem);
    INT Slot = KeySlot(this, Key, 1);
    if (Slot >= 0)
    {
        if (Keys[Slot].bReady == 0)
            return 1;
        if (Slot < Pages * Cols)
        {
            NShortCutItem* Item = &Items[Slot];
            if (!Item->bUsed)
            {
                if (Slot < Pages * Cols && !Items[Slot].bUsed)
                    GConsole->PlayInterfaceSound(SOUND_EmptySlot);
            }
            else
            {
                if (Item->Kind() == SHORTCUT_Item && Item->Unknown1998 != 0 && AsksBeforeUse())
                    ShowItemWarning(this, &Items[Slot]);
                else
                {
                    if (Items[Slot].Kind() != SHORTCUT_Item || Items[Slot].GetUnknown19AC() < 1)
                    {
                        GConsole->UseShortCut(Slot, 0, 0);
                        Keys[Slot].bReady = 0;
                        return 0;
                    }
                    ShowItemMessage(this, &Items[Slot]);
                }
            }
        }
    }
    return 0;
    unguard;
}
NW_HOOK_METHOD(0x100f5d90, NCShortCutWnd, OnShortCutKeyDown)

// the slots of the page just shown stop flashing (repeated inline in NextPage and PrevPage)
static FORCEINLINE void StopPageFlashes(NCShortCutWnd* Wnd)
{
    INT First = Wnd->Page * Wnd->Cols;
    for (INT i = 0; i < Wnd->Cols; i++)
        Wnd->ItemCoolTime(First + i)->bFlashing = 0;
}

// 0x100f6190 - after the last page back to the first (page 9 fixed)
void NCShortCutWnd::NextPage()
{
    if (Page < Pages - 1)
        Page++;
    else if (Page == 9)
        Page = 0;
    StopPageFlashes(this);
}
NW_HOOK_METHOD(0x100f6190, NCShortCutWnd, NextPage)

// 0x100f6200 - before the first page to page 9
void NCShortCutWnd::PrevPage()
{
    if (Page > 0)
        Page--;
    else if (Page == 0)
        Page = 9;
    StopPageFlashes(this);
}
NW_HOOK_METHOD(0x100f6200, NCShortCutWnd, PrevPage)

// 0x100f62b0
void NCShortCutWnd::ClearShortCuts()
{
    for (INT i = 0; i < Cols * Pages; i++)
        DeleteShortCut(i);
}
NW_HOOK_METHOD(0x100f62b0, NCShortCutWnd, ClearShortCuts)

// 0x100f6f30 (ret 8) - a shortcut the server registered (NItemInfo +0x179C is its slot, +0x1710 its kind, +0x400 the
// icon's name): the icon is looked up (the macro's or the black texture when there is none), a slot entry is built and
// copied over the slot, and the slot's key is set by the kind: potions and shots every 200 ms (a shot on auto use is
// marked), skills when their cool time ends (a toggle skill every second), the attack / pick up actions every 200 /
// 500 ms, everything else when released. A skill resumes its cool time
INT NCShortCutWnd::SetShortCut(NItemInfo* Info, INT InOwner)
{
    INT Slot = NItemInfoInt(Info, 0x179C);
    if (Slot >= Pages * Cols)
        return 0;
    if (appStrlen(Info->IconName) > 0)
        Info->Icon = FindTexture(Info->IconName);
    else
        Info->Icon = NULL;
    if (Info->Icon == NULL)
    {
        if (Info->Unknown1710 == SHORTCUT_Macro)
            Info->Icon = FindTexture(TEXT("L2UI.MacroWnd.Macro_Icon1"));
        else
            Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    }

    NShortCutItem* Item = (NShortCutItem*)appMalloc(sizeof(NShortCutItem), TEXT("new"));
    if (Item)
        Item = new ((void*)Item) NShortCutItem(*Info);
    Item->Owner = InOwner;
    Item->CoolTime.Stop();
    switch (Info->Unknown1710)
    {
    case SHORTCUT_Item:
        Keys[Slot].bOnKeyUp = 0;
        Keys[Slot].bOnCoolTime = 0;
        if (IsQuickUseItem(Item->ID))
        {
            Keys[Slot].Interval = 200;
            if (IsShotItem(Item->ID) && IsAutoShotSlot(Item->ID))
                Item->bAutoShot = 1;
        }
        else
        {
            Keys[Slot].bOnKeyUp = 1;
            Keys[Slot].OnKeyUp();
        }
        break;
    case SHORTCUT_Skill:
        Keys[Slot].bOnKeyUp = 0;
        Keys[Slot].bOnCoolTime = 0;
        if (GConsole->IsToggleSkill(Item->ID, Item->UnknownC18))
            Keys[Slot].Interval = 1000;
        else
        {
            Keys[Slot].bOnCoolTime = 1;
            Keys[Slot].OnKeyUp();
        }
        break;
    case SHORTCUT_Action:
        Keys[Slot].bOnKeyUp = 0;
        Keys[Slot].bOnCoolTime = 0;
        if (Item->ID == ACTION_Attack)
            Keys[Slot].Interval = 200;
        else if (Item->ID == ACTION_PickUp)
            Keys[Slot].Interval = 500;
        else
        {
            Keys[Slot].bOnKeyUp = 1;
            Keys[Slot].OnKeyUp();
        }
        break;
    case SHORTCUT_Macro:
        Keys[Slot].bOnCoolTime = 0;
        Keys[Slot].bOnKeyUp = 1;
        Keys[Slot].OnKeyUp();
        break;
    }
    Items[Slot].CopyFrom(Item);
    Items[Slot].bUsed = 1;
    Items[Slot].ToolTip.Fill(&Items[Slot], NTT_ShortCut, 0);
    if (Info->Unknown1710 == SHORTCUT_Skill)
    {
        DOUBLE Total, Remaining;
        GetSkillCoolTime(Info->ID, &Total, &Remaining);
        if (Total > 0.0)
            Items[Slot].CoolTime.Start(Total, Remaining);
    }
    Item->~NShortCutItem();
    appFree(Item);
    return 1;
}
NW_HOOK_METHOD(0x100f6f30, NCShortCutWnd, SetShortCut)

NW_CHECK_VTABLE(NCShortCutWnd, 0x1019d9e8, 76)
