/*=============================================================================
    NCItemWnd.cpp - grid of item icons, base of the item windows, and NItemEntry, the record
    of each item. Stock: 0x10015b80 .. 0x100178e0 (NCCoolTimeIcon's code sits in between:
    0x10016620 .. 0x10016850, 0x100170e0 .. 0x100172a0, 0x100173c0 .. 0x10017550) plus
    WindowProc (0x1006bb90).
=============================================================================*/
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCCoolTimeIcon.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCMacroEditList.h"
#include "nwindow/NCMacroEditWnd.h"
#include "nwindow/NCSkillItemWnd.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NConsoleWnd.h"
#if NW_FEATURES
// phase 6: ItemHoverTip (not in NWindow_C1: the hover tip of the item grids in the recent style)
#include "features/ItemHoverTip.h"
#endif

enum { SOUND_PickUp = 3 };  // an item picked up (name ours)
enum { SOUND_CoolTimeEnd = 9 };  // a cool time ended (name ours)

// the desktop's message box (NDialogInfo::bMainBox) and the second one
static FORCEINLINE NCDialogBox* MainDialogBox()
{
    return GDesktop->MainDialogBox;
}
static FORCEINLINE NCDialogBox* SecondDialogBox()
{
    return GDesktop->SecondDialogBox;
}

/*-----------------------------------------------------------------------------
    NItemEntry
-----------------------------------------------------------------------------*/

// 0x1001d8b0 - the defaults: every number 0 but the -1s; the tool tip object is built first (0x100024f0)
NItemEntry::NItemEntry()
{
    new ((void*)&ToolTip) NToolTipInfo();
    Name[0] = 0;
    AdditionalName[0] = 0;
    IconName[0] = 0;
    Unknown20C[0] = 0;
    Unknown30C[0] = 0;
    Description[0] = 0;
    Icon = NULL;
    Unknown004 = 0;
    Unknown008 = 0;
    TradeCount = 0;
    bSelected = 0;
    ObjectID = 0;
    ID = 0;
    Count = 0;
    UnknownC20 = 0;
    BodyPart = 0;
    Weight = 0;
    Unknown1C80 = 0;
    Unknown1C84 = 0;
    Unknown1C88 = 0;
    Unknown1C8C = 0;
    Unknown1C90 = 0;
    Unknown1C94 = 0;
    Unknown1C58 = 0;
    CrystalGrade = 0;
    Unknown1C60 = 0;
    Unknown1C64 = 0;
    Unknown1C68 = 0;
    Unknown1C6C = 0;
    Unknown1C70 = 0;
    Unknown1C78 = 0;
    Unknown1C7C = 0;
    Enchant = 0;
    UnknownC3C = 0;
    bStackable = 0;
    bEquipped = 0;
    Unknown1C98 = 0;
    Unknown1998 = 0;
    Unknown1C9C = 0;
    Unknown1CA0 = 0;
    Unknown1CA4 = 0;
    Unknown1CA8 = 0;
    Unknown19AC = 0;
    DisplayName[0] = 0;
    Type = -1;
    Unknown1910 = -1;
    UnknownC18 = -1;
    UnknownC2C = -1;
    UnknownC30 = -1;
    SubType = -1;
    UnknownC34 = -1;
    Unknown19B4 = -1;
}
NW_HOOK_CTOR(0x1001d8b0, NItemEntry)

// 0x1001da00 - the fields do not keep NItemInfo's order; Unknown1C60 / Unknown1C64 (NItemInfo +0x1758 / +0x175C) stay
// uninitialized, and so does the display name past its first character. C3's NItemInfo has a dword at +0x178C that
// C2 did not (its later fields sit 4 higher than NItemInfo.h says: read by offset)
NItemEntry::NItemEntry(NItemInfo* Src)
{
    new ((void*)&ToolTip) NToolTipInfo();
    Icon = NULL;
    Unknown004 = 0;
    Unknown008 = 0;
    Name[0] = 0;
    AdditionalName[0] = 0;
    IconName[0] = 0;
    Unknown20C[0] = 0;
    Unknown30C[0] = 0;
    Description[0] = 0;
    DisplayName[0] = 0;
    TradeCount = 0;
    bSelected = 0;

    Icon = Src->Icon;
    Unknown004 = Src->UnknownC04;
    Unknown008 = Src->UnknownC08;
    appStrcpy(Name, Src->Name);
    appStrcpy(AdditionalName, Src->AdditionalName);
    appStrcpy(IconName, Src->IconName);
    appStrcpy(Unknown20C, Src->Unknown200);
    appStrcpy(Unknown30C, Src->Unknown300);
    appStrcpy(Description, Src->Description);
    Type = Src->Param;
    Unknown1910 = Src->Unknown1710;
    ObjectID = Src->ObjectID;
    ID = Src->ID;
    Count = Src->Count;
    UnknownC20 = Src->UnknownC1C;
    UnknownC18 = Src->UnknownC20;
    BodyPart = Src->BodyPart;
    Weight = Src->UnknownC28;
    UnknownC2C = Src->UnknownC2C;
    UnknownC30 = Src->UnknownC30;
    Unknown1C80 = Src->UnknownC34[0];
    Unknown1C84 = Src->UnknownC34[1];
    Unknown1C88 = Src->UnknownC34[2];
    Unknown1C8C = Src->UnknownC34[3];
    Unknown1C90 = Src->UnknownC34[4];
    Unknown1C94 = Src->UnknownC34[5];
    Unknown1C58 = Src->UnknownC34[6];
    CrystalGrade = Src->UnknownC34[7];
    Unknown1C68 = Src->UnknownC34[10];
    Unknown1C6C = Src->UnknownC34[11];
    Unknown1C70 = Src->UnknownC34[12];
    SubType = Src->SubType;
    Unknown1C78 = Src->UnknownC6C;
    Unknown1C7C = Src->UnknownC70;
    Enchant = Src->Enchant;
    UnknownC3C = *(SWORD*)((BYTE*)Src + 0x1790);
    bStackable = NItemInfoInt(Src, 0x1794);
    bEquipped = NItemInfoInt(Src, 0x1798);
    UnknownC34 = NItemInfoInt(Src, 0x179C);
    Unknown1C98 = NItemInfoInt(Src, 0x17A0);
    Unknown1998 = NItemInfoInt(Src, 0x17A4);
    Unknown1C9C = Src->UnknownC78;
    Unknown1CA0 = Src->UnknownC7C;
    Unknown1CA4 = Src->UnknownC80;
    Unknown1CA8 = NItemInfoInt(Src, 0x17A8);
    Unknown19AC = Src->Unknown1788;
    Unknown19B4 = NItemInfoInt(Src, 0x178C);
}
NW_HOOK_CTOR_ARGS(0x1001da00, NItemEntry, (NItemInfo* A), (A))

// 0x10101d80 - the tool tip's destructor on the entry's tool tip (a tail jump into NToolTipInfo's, 0x100025a0, in the
// stock body; the union keeps the compiler from building and destroying it by itself)
NItemEntry::~NItemEntry()
{
    ToolTip.~NToolTipInfo();
}
NW_HOOK_DTOR(0x10101d80, NItemEntry)

// 0x1001c5c0
const TCHAR* NItemEntry::GetName()
{
    const TCHAR* S = Name;
    return S && *S ? S : NULL;
}
NW_HOOK_METHOD(0x1001c5c0, NItemEntry, GetName)

// 0x1001c5d0 - the name the tool tip and the lists show: "+N" (the enchant, when above 0 and the type in Unknown1910 is
// not 7, 0xD or 0xE), the name, "-" and the additional name
const TCHAR* NItemEntry::MakeDisplayName()
{
    TCHAR* Out = DisplayName;
    Out[0] = 0;
    if (Enchant > 0 && Unknown1910 != 7 && Unknown1910 != 0xD && Unknown1910 != 0xE)
        appSprintf(Out, TEXT("+%d"), Enchant);
    const TCHAR* S = Name;
    if (S && *S)
        appStrcat(Out, S);
    const TCHAR* Add = AdditionalName;
    if (Add && *Add)
    {
        appStrcat(Out, TEXT("-"));
        appStrcat(Out, Add);
    }
    Out[appStrlen(Out) + 1] = 0;
    return Out;
}
NW_HOOK_METHOD(0x1001c5d0, NItemEntry, MakeDisplayName)

// 0x1001c670
const TCHAR* NItemEntry::GetAdditionalName()
{
    const TCHAR* S = AdditionalName;
    return S && *S ? S : NULL;
}
NW_HOOK_METHOD(0x1001c670, NItemEntry, GetAdditionalName)

// 0x1001c690
const TCHAR* NItemEntry::GetDescription()
{
    const TCHAR* S = Description;
    return S && *S ? S : NULL;
}
NW_HOOK_METHOD(0x1001c690, NItemEntry, GetDescription)

// 0x1012c680 / 0x1012c690 / 0x1012c6a0 (names ours; the header's inline getters) - `mov eax, [ecx]` / `[ecx+4]` / `[ecx+8]`: the
// first three dwords (the icon and the two icons of a two-slot item). The linker folded each body with every class whose
// first dwords are read the same way (a list's pointer, the string's Length at +8 ...), so the hooks serve them all
NW_HOOK_METHOD(0x1012c680, NItemEntry, GetIcon)
NW_HOOK_METHOD(0x1012c690, NItemEntry, GetUnknown004)
NW_HOOK_METHOD(0x1012c6a0, NItemEntry, GetUnknown008)

// 0x1001c6b0
INT NItemEntry::GetUnknownC20()
{
    return UnknownC20;
}
NW_HOOK_METHOD(0x1001c6b0, NItemEntry, GetUnknownC20)

// 0x1001c6c0
INT NItemEntry::GetUnknownC34()
{
    return UnknownC34;
}
NW_HOOK_METHOD(0x1001c6c0, NItemEntry, GetUnknownC34)

// 0x1001c6d0
void NItemEntry::SetUnknownC34(INT Value)
{
    UnknownC34 = Value;
}
NW_HOOK_METHOD(0x1001c6d0, NItemEntry, SetUnknownC34)

// 0x1001de40 - the texts and the first three fields only when set in Src (the three icon textures all look at
// IconName's first character: the stock code does, whatever the intent was); the display name is Src's own (built),
// the tool tip object is copied whole (0x1001dd10)
void NItemEntry::Copy(NItemEntry* Src)
{
    if (Src->GetIcon())
        Icon = Src->GetIcon();
    if (Src->GetUnknown004())
        Unknown004 = Src->GetUnknown004();
    if (Src->GetUnknown008())
        Unknown008 = Src->GetUnknown008();
    if (Src->GetName())
        appStrcpy(Name, Src->GetName());
    else
        Name[0] = 0;
    const TCHAR* SrcIconName = Src->IconName;
    if (SrcIconName && *SrcIconName)
        appStrcpy(IconName, SrcIconName);
    if (Src->Unknown20C && *SrcIconName)
        appStrcpy(Unknown20C, Src->Unknown20C);
    if (Src->Unknown30C && *SrcIconName)
        appStrcpy(Unknown30C, Src->Unknown30C);
    if (Src->GetAdditionalName())
        appStrcpy(AdditionalName, Src->GetAdditionalName());
    else
        AdditionalName[0] = 0;
    if (Src->GetDescription())
        appStrcpy(Description, Src->GetDescription());
    else
        Description[0] = 0;
    if (Src->MakeDisplayName())
        appStrcpy(DisplayName, Src->MakeDisplayName());
    else
        DisplayName[0] = 0;
    Type = Src->Type;
    Unknown1910 = Src->Unknown1910;
    ObjectID = Src->ObjectID;
    ID = Src->ID;
    Count = Src->Count;
    UnknownC20 = Src->UnknownC20;
    UnknownC18 = Src->UnknownC18;
    BodyPart = Src->BodyPart;
    Weight = Src->Weight;
    UnknownC2C = Src->UnknownC2C;
    UnknownC30 = Src->UnknownC30;
    Enchant = Src->Enchant;
    UnknownC3C = Src->UnknownC3C;
    bStackable = Src->bStackable;
    bEquipped = Src->bEquipped;
    UnknownC34 = Src->UnknownC34;
    bSelected = Src->bSelected;
    Unknown1C58 = Src->Unknown1C58;
    CrystalGrade = Src->CrystalGrade;
    Unknown1C60 = Src->Unknown1C60;
    Unknown1C64 = Src->Unknown1C64;
    Unknown1C68 = Src->Unknown1C68;
    Unknown1C6C = Src->Unknown1C6C;
    Unknown1C70 = Src->Unknown1C70;
    SubType = Src->SubType;
    Unknown1C80 = Src->Unknown1C80;
    Unknown1C84 = Src->Unknown1C84;
    Unknown1C88 = Src->Unknown1C88;
    Unknown1C8C = Src->Unknown1C8C;
    Unknown1C90 = Src->Unknown1C90;
    Unknown1C94 = Src->Unknown1C94;
    Unknown1C78 = Src->Unknown1C78;
    Unknown1C98 = Src->Unknown1C98;
    Unknown1998 = Src->Unknown1998;
    Unknown1C9C = Src->Unknown1C9C;
    Unknown1CA0 = Src->Unknown1CA0;
    Unknown1CA4 = Src->Unknown1CA4;
    Unknown1CA8 = Src->Unknown1CA8;
    Unknown1C7C = Src->Unknown1C7C;
    Unknown19AC = Src->Unknown19AC;
    Unknown19B4 = Src->Unknown19B4;
    ToolTip = Src->ToolTip;   // NToolTipInfo's copy assignment (0x1001dd10)
}
NW_HOOK_METHOD(0x1001de40, NItemEntry, Copy)

// NItemEntry::FillToolTip is inline in NCItemWnd.h (NToolTipInfo::Fill, 0x1002c650)

// 0x1001c6e0
void NItemEntry::Select()
{
    bSelected = 1;
}
NW_HOOK_METHOD(0x1001c6e0, NItemEntry, Select)

// 0x1001c6f0
UBOOL NItemEntry::IsSelected()
{
    return bSelected;
}
NW_HOOK_METHOD(0x1001c6f0, NItemEntry, IsSelected)

// 0x1001c700 - naked: the stock loop of UnselectAll (0x1001d270) counts in eax across the call, so eax must come
// back untouched; bSelected is at +0x1954 (NW_ASSERT_OFFSET)
NW_NAKED INT NItemEntry::Unselect()
{
    __asm
    {
        mov dword ptr [ecx + 0x1954], 0
        ret
    }
}
NW_HOOK_METHOD(0x1001c700, NItemEntry, Unselect)

// 0x1001c710 - C3: the field is a kind (1, 2 or 3: the count shows), not a flag
UBOOL NItemEntry::IsStackable()
{
    return bStackable == 1 || bStackable == 2 || bStackable == 3;
}
NW_HOOK_METHOD(0x1001c710, NItemEntry, IsStackable)

// 0x1001c730 (name ours)
INT NItemEntry::GetUnknown19AC()
{
    return Unknown19AC;
}
NW_HOOK_METHOD(0x1001c730, NItemEntry, GetUnknown19AC)

// 0x1001c740 - the constructor's defaults, but the tool tip object, TradeCount, bSelected, Unknown1CA8, Unknown19AC
// and Unknown19B4 are kept
void NItemEntry::Reset()
{
    Name[0] = 0;
    AdditionalName[0] = 0;
    IconName[0] = 0;
    Unknown20C[0] = 0;
    Unknown30C[0] = 0;
    Description[0] = 0;
    Icon = NULL;
    Unknown004 = 0;
    Unknown008 = 0;
    Type = -1;
    Unknown1910 = -1;
    ObjectID = 0;
    ID = 0;
    Count = 0;
    UnknownC20 = 0;
    UnknownC18 = -1;
    BodyPart = 0;
    Weight = 0;
    UnknownC2C = -1;
    UnknownC30 = -1;
    Unknown1C80 = 0;
    Unknown1C84 = 0;
    Unknown1C88 = 0;
    Unknown1C8C = 0;
    Unknown1C90 = 0;
    Unknown1C94 = 0;
    Unknown1C58 = 0;
    CrystalGrade = 0;
    Unknown1C60 = 0;
    Unknown1C64 = 0;
    Unknown1C68 = 0;
    Unknown1C6C = 0;
    Unknown1C70 = 0;
    SubType = -1;
    Unknown1C78 = 0;
    Unknown1C7C = 0;
    Enchant = 0;
    UnknownC3C = 0;
    bStackable = 0;
    bEquipped = 0;
    UnknownC34 = -1;
    Unknown1C98 = 0;
    Unknown1998 = 0;
    Unknown1C9C = 0;
    Unknown1CA0 = 0;
    Unknown1CA4 = 0;
    DisplayName[0] = 0;
}
NW_HOOK_METHOD(0x1001c740, NItemEntry, Reset)

// 0x100075a0 (the stock keeps it among NDialogInfo's: a small record's constructor; some callers build the request
// inline) - Unknown02 is left as it is
NItemRequest::NItemRequest()
{
    *(WORD*)&Type = 0xFFFF;
    ObjectID = 0;
    ItemID = 0;
    Count = 0;
    Unknown10 = 0;
    Unknown14 = 0;
    BodyPart = 0;
    Enchant = 0;
    Unknown1C = 0;
    bEquipped = 0;
    Unknown24 = 0;
    Unknown28 = 0;
    Unknown2C = -1;
}
NW_HOOK_CTOR(0x100075a0, NItemRequest)

/*-----------------------------------------------------------------------------
    NCItemWnd
-----------------------------------------------------------------------------*/

// 0x1001e190 - 5 x 5 icons of 32 pixels (the cell gaps are left as allocated)
NCItemWnd::NCItemWnd()
{
    BackTex = NULL;
    SelectTex = NULL;
    ToolTipTex = NULL;
    Columns = 5;
    VisibleRows = 5;
    TopRow = 0;
    NumRows = 0;
    bDragging = 0;
    bNoDrag = 0;
    MaxItems = 25;
    CellSize = 32;
    Unknown0C8 = 16;
    Selected = -1;
    HoverIndex = -1;
}
NW_HOOK_CTOR(0x1001e190, NCItemWnd)

// 0x1001e250 - Items' destructor is TArray<NItemEntry*>'s (0x1001d7a0)
NW_HOOK_DTOR(0x1001e250, NCItemWnd)

// 0x1001e2b0 - NumRows and bNoDrag are left as allocated
NCItemWnd::NCItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY)
{
    BackTex = NULL;
    SelectTex = NULL;
    ToolTipTex = NULL;
    Columns = InColumns;
    MaxItems = InMaxItems;
    TopRow = 0;
    bDragging = 0;
    VisibleRows = InVisibleRows;
    Selected = -1;
    HoverIndex = -1;
    CellGapX = InGapX;
    CellSize = InCellSize;
    Unknown0C8 = 16;
    CellGapY = InGapY;
}
NW_HOOK_CTOR_ARGS(0x1001e2b0, NCItemWnd, (INT A, INT B, INT C, INT D, INT E, INT F), (A, B, C, D, E, F))


// 0x10093f30 (vslot 25) - the base's answer as a plain 0 / 1
INT NCItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCScrollWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x10093f30, NCItemWnd, WindowProc)

// 0x1001cee0 (vslot 37)
INT NCItemWnd::OnCreate()
{
    NCScrollWnd::OnCreate();
    BackTex = FindTexture(TEXT("NWindow.BlackTexture"));
    SelectTex = FindTexture(TEXT("l2ui.nwindow.item_click"));
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    InitScrollBar();
    return 1;
}
NW_HOOK_METHOD(0x1001cee0, NCItemWnd, OnCreate)

// 0x1001cb70 - the range starts as TopRow rows, as in the stock code
void NCItemWnd::InitScrollBar()
{
    SetRepeatDelay(200);
    ScrollBar->SetRange(TopRow, VisibleRows);
    ScrollBar->SetValue(TopRow);
}
NW_HOOK_METHOD(0x1001cb70, NCItemWnd, InitScrollBar)

// 0x1001c860 (vslot 38)
INT NCItemWnd::OnDestroy()
{
    guard(NCItemWnd::OnDestroy);
    Clear();
    NCScrollWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1001c860, NCItemWnd, OnDestroy)

// 0x1001cc20 (vslot 44) - shown, the scroll range follows what was added while it was hidden
void NCItemWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    UpdateScrollRange();
}
NW_HOOK_METHOD(0x1001cc20, NCItemWnd, ShowWindow)

// 0x1001ca60 (vslot 63) - the base's release (the mouse button state), answered 1
INT NCItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    NCWnd::OnLButtonUp(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x1001ca60, NCItemWnd, OnLButtonUp)

// 0x1001cad0 (vslot 45) - closes the desktop's message boxes this window opened (C3 tests the desktop and the box)
void NCItemWnd::HideWindow()
{
    if (GDesktop && GDesktop->SecondDialogBox)
    {
        if (GDesktop->SecondDialogBox->GetOwner() == Entry)
            GDesktop->SecondDialogBox->HideWindow();
    }
    if (GDesktop && GDesktop->MainDialogBox)
    {
        if (GDesktop->MainDialogBox->GetOwner() == Entry)
            GDesktop->MainDialogBox->HideWindow();
    }
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x1001cad0, NCItemWnd, HideWindow)

// 0x1001c8e0 - the cells count CellSize + the gaps; the right and bottom edges belong to the last column / row. The
// offsets move the window's origin (C3 callers pass 0, 0)
INT NCItemWnd::HitTest(INT X, INT Y, INT OffsetX, INT OffsetY)
{
    X -= ScreenX + OffsetX;
    Y -= ScreenY + OffsetY;
    if (X < 0)
        return -1;
    INT CellW = CellGapX + CellSize;
    if (X > Columns * CellW)
        return -1;
    if (Y < 0)
        return -1;
    INT CellH = CellGapY + CellSize;
    if (Y > VisibleRows * CellH)
        return -1;
    INT Col = X / CellW;
    if (Col >= Columns)
        Col--;
    INT Row = Y / CellH;
    if (Row >= VisibleRows)
        Row--;
    return (TopRow + Row) * Columns + Col;
}
NW_HOOK_METHOD(0x1001c8e0, NCItemWnd, HitTest)

// 0x1001c980
INT NCItemWnd::GetSelected()
{
    if (Selected >= 0 && Selected < Items.Num())
        return Selected;
    return -1;
}
NW_HOOK_METHOD(0x1001c980, NCItemWnd, GetSelected)

// 0x1001c9a0
INT NCItemWnd::GetItemCount()
{
    return Items.Num();
}
NW_HOOK_METHOD(0x1001c9a0, NCItemWnd, GetItemCount)

// 0x1001c9b0 (vslot 67) - a row per notch
INT NCItemWnd::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    SWORD Notches = (SWORD)((SWORD)HIWORD(wParam) / 120);
    if (Notches > 0)
    {
        if (TopRow > Notches)
            TopRow -= Notches;
        else
            TopRow = 0;
    }
    else if (NumRows - TopRow + Notches > VisibleRows)
        TopRow -= Notches;
    else if (NumRows >= VisibleRows)
        TopRow = NumRows - VisibleRows;
    else
        TopRow = 0;
    ScrollBar->SetValue(TopRow);
    return 1;
}
NW_HOOK_METHOD(0x1001c9b0, NCItemWnd, OnMouseWheel)

// 0x1001cb60 - the stock body is shared with other classes' one-field setters
void NCItemWnd::SetVisibleRows(INT Rows)
{
    VisibleRows = Rows;
}
NW_HOOK_METHOD(0x1001cb60, NCItemWnd, SetVisibleRows)

// 0x1001cbb0 - rows for the items; TopRow comes back when the last row would leave the view
void NCItemWnd::UpdateScrollRange()
{
    NumRows = Items.Num() / Columns;
    if (Items.Num() % Columns)
        NumRows++;
    if (NumRows >= VisibleRows && NumRows - TopRow < VisibleRows)
        TopRow = NumRows - VisibleRows;
    ScrollBar->SetRange(NumRows, VisibleRows);
    ScrollBar->SetValue(TopRow);
}
NW_HOOK_METHOD(0x1001cbb0, NCItemWnd, UpdateScrollRange)

// 0x1001cc30 (vslot 71) - NCScrollWnd's OnScrollBar overridden (TopRow = Value)
INT NCItemWnd::OnScrollBar(INT Value)
{
    TopRow = Value;
    return 1;
}
NW_HOOK_METHOD(0x1001cc30, NCItemWnd, OnScrollBar)

// 0x1001cc50 (vslot 69)
INT NCItemWnd::ScrollUp()
{
    if (NumRows > VisibleRows)
    {
        if (--TopRow < 0)
            TopRow = 0;
        ScrollBar->SetValue(TopRow);
    }
    return 1;
}
NW_HOOK_METHOD(0x1001cc50, NCItemWnd, ScrollUp)

// 0x1001cc90 (vslot 70)
INT NCItemWnd::ScrollDown()
{
    if (NumRows > VisibleRows)
    {
        TopRow++;
        if (TopRow > NumRows - VisibleRows)
            TopRow = NumRows - VisibleRows;
        ScrollBar->SetValue(TopRow);
    }
    return 1;
}
NW_HOOK_METHOD(0x1001cc90, NCItemWnd, ScrollDown)

// 0x1001cce0 (vslot 74) - `ret 0x10` shared with other empty four-argument methods
void NCItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
}
NW_HOOK_METHOD(0x1001cce0, NCItemWnd, DrawItemToolTip)

// 0x1001cf30 (vslot 56) - the clip is the window's rectangle; the first cell sits at (1, 1) and the cells are
// CellSize + the gaps apart; the rows stop at VisibleRows * (CellSize + 1) pixels
INT NCItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCItemWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 1;
    INT Y = 1;
    for (INT i = TopRow * Columns; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (!Item)
            continue;
        if (i % Columns == 0)
        {
            X = 1;
            if (i != TopRow * Columns)
                Y += CellGapY + CellSize;
            if (VisibleRows * (CellSize + 1) <= Y)
                break;
        }
        else
            X += CellGapX + CellSize;
        if (Item->IsSelected())
            Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, SelectTex, 0xFF, 1);
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon(), 0xFF, 1);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1001cf30, NCItemWnd, OnPaint)

// 0x1001e350 (vslot 57) - the tool tip is drawn with the window's skin: over the item under the mouse the skin gets a
// copy of its tool tip object by value, the cell's position in the window (X and Y, which the routine moves to keep
// the box on the screen) and the window's rectangle (the FLOAT size, copied bit for bit). The selected item's box is the
// full one (0x1002dc40), any other item's the title line (0x1002df40)
INT NCItemWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (HoverIndex >= 0 && HoverIndex < Items.Num())
    {
        INT Index = HoverIndex - TopRow * Columns;
        INT Y = (CellGapY + CellSize) * (Index / Columns);
        INT X = (CellGapX + CellSize) * (Index % Columns);
        NItemEntry* Item = Items(HoverIndex);
        if (Item->IsSelected())
        {
            if (ToolTipSkin)
            {
                NToolTipRect Wnd;
                Wnd.X = ScreenX;
                Wnd.Y = ScreenY;
                Wnd.W = Width;
                Wnd.H = Height;
                ToolTipSkin->DrawFull(Device, Item->ToolTip, X, Y, Wnd);
            }
        }
        else if (ToolTipSkin)
        {
            NToolTipRect Wnd;
            Wnd.X = ScreenX;
            Wnd.Y = ScreenY;
            Wnd.W = Width;
            Wnd.H = Height;
            ToolTipSkin->Draw(Device, Item->ToolTip, X, Y, Wnd);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x1001e350, NCItemWnd, OnDrawToolTip)

// 0x1001d0f0 (vslot 62) - selects the item under the mouse and picks it up (unless bNoDrag, which lasts one
// click); the desktop's message box closes, and keeps the focus if still up.
INT NCItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    Selected = Index;
    HoverIndex = Index;
    if (Index >= 0 && Index < Items.Num())
    {
        UnselectAll();
        Items(Selected)->Select();
        if (!bNoDrag)
        {
            if (MainDialogBox()->IsShowWindow())
            {
                MainDialogBox()->Reset();
                MainDialogBox()->HideWindow();
            }
            bDragging = 1;
            GConsole->PlayInterfaceSound(SOUND_PickUp);
        }
        else
            bNoDrag = 0;
        if (MainDialogBox()->IsShowWindow())
            MainDialogBox()->SetFocus();
    }
    return 1;
}
NW_HOOK_METHOD(0x1001d0f0, NCItemWnd, OnLButtonDown)

// 0x1001d200 (vslot 61) - while an item is carried the cursor is its icon (type 4: a color cursor built from the
// texture); C3 no longer puts the arrow back
INT NCItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    HoverIndex = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    if (IsFocused() && bDragging && Selected >= 0 && Selected < Items.Num())
    {
        SetCursor(4, Items(Selected)->GetIcon());
        LockCursor();
    }
    return 1;
}
NW_HOOK_METHOD(0x1001d200, NCItemWnd, OnMouseMove)

// 0x1001d270 (vslot 73)
void NCItemWnd::UnselectAll()
{
    for (INT i = 0; i < Items.Num(); i++)
        Items(i)->Unselect();
}
NW_HOOK_METHOD(0x1001d270, NCItemWnd, UnselectAll)

// 0x1001ddc0 (vslot 72) - the entries are deleted (~NItemEntry, then the memory goes back to GMalloc)
void NCItemWnd::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i))
            delete Items(i);
    Items.Empty();
    TopRow = 0;
    NumRows = 0;
    Selected = -1;
    HoverIndex = -1;
}
NW_HOOK_METHOD(0x1001ddc0, NCItemWnd, Clear)

NW_CHECK_VTABLE(NCItemWnd, 0x1017c950, 75)

/*-----------------------------------------------------------------------------
    The skill grid's functions the stock code names after NCItemWnd (guard strings): they work on
    NCSkillItemWnd's fields (the icon frames, the toggle animation, the cool time icons), a class
    of another lane's file, so they are methods of this class (names ours) that read their object
    as the derived class, and the cell offset of a point is the item window's 0x100a10a0
-----------------------------------------------------------------------------*/

// 0x100a10a0 (name ours) - the item under a point of the skill grid: like HitTest but the cells are CellSize + 5 wide and
// CellSize + 3 high whatever the gaps say (the skill grid never sets them); -1 outside
INT NCItemWnd::HitTestSkill(INT X, INT Y)
{
    X -= ScreenX;
    Y -= ScreenY;
    if (X < 0)
        return -1;
    INT CellW = CellSize + 5;
    if (X > CellW * Columns)
        return -1;
    if (Y < 0)
        return -1;
    INT CellH = CellSize + 3;
    if (Y > CellH * VisibleRows)
        return -1;
    X /= CellW;
    if (X >= Columns)
        X--;
    Y /= CellH;
    if (Y >= VisibleRows)
        Y--;
    return (TopRow + Y) * Columns + X;
}
NW_HOOK_METHOD(0x100a10a0, NCItemWnd, HitTestSkill)


