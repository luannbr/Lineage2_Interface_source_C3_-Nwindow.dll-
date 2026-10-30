/*=============================================================================
    NCRecipeShopItemWnd.cpp - the item grids of the recipe shop's manage window (new in C2).
    Stock (C3): 0x100E0680 .. 0x100E0900 (AddItem, AddCopy, MoveItem), 0x100E1330 .. 0x100E1640 (the mouse handlers),
    0x100E16E0 / 0x100E1730 / 0x100E1780 (the constructors, the destructor).
    WindowProc (0x100A2850) and OnDestroy (0x100D7090) are bodies shared with NCShopItemWnd; SetTarget (0x10088870) and
    SetMode (0x100DE910) with NCStoreItemWnd.
=============================================================================*/
#include "nwindow/NCRecipeShopItemWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NClasses.gen.h"

enum { DIALOG_RECIPE_PRICE = 0x23 };  // NDialogInfo::ID (name ours): a price for the owner (the answer is OnDialogResult)
enum { MSG_PriceToBuy = 0x142 };      // systemmsg line the price box shows (OnDialogResult's twin in NCRecipeShopWnd)
enum { MSG_PriceToBuyDrop = 0x3c3 };  // the same box when the entry was dropped on the grid

// the price question (inline in the three stock callers): the entry's name goes through sprintf into a 64-character
// buffer that nothing reads afterwards, the box asks for a number and answers Owner. C3 puts the recipe's id in the
// info (+0x18, the header names that field bProgress)
static FORCEINLINE void AskPrice(NCRecipeShopItemWnd* Owner, NItemEntry* Item, INT MsgID)
{
    TCHAR Name[64];
    appSprintf(Name, TEXT("%s"), Item->GetName());
    NDialogInfo Info;
    Info.ID = DIALOG_RECIPE_PRICE;
    Info.Type = 3;
    Info.Owner = Owner->Entry;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.SetSystemMessage(MsgID);
    ::nw::Field<INT>(&Info, 0x18) = Item->ID;
    GConsole->ShowDialog(&Info);
}

// 0x100e16e0 - the other grid is none
NCRecipeShopItemWnd::NCRecipeShopItemWnd()
{
    Target = NULL;
}
NW_HOOK_CTOR(0x100e16e0, NCRecipeShopItemWnd)

// 0x100e1780 (ret 0x18) - the grid of the six arguments (columns, rows, most items, icon size, gaps)
NCRecipeShopItemWnd::NCRecipeShopItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
    Target = NULL;
}
NW_HOOK_CTOR_ARGS(0x100e1780, NCRecipeShopItemWnd, (INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY), (InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY))

// 0x100e1730
NW_HOOK_DTOR(0x100e1730, NCRecipeShopItemWnd)

// 0x100a2850 (vslot 25, ret 0xC): the stock body is NCShopItemWnd's (hooked there): the double click and the dialog
// box's answer
#if NW_HOOK_MODE
NW_NAKED INT NCRecipeShopItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x100a2850)
#else
NW_SLOT_IS("?WindowProc@NCRecipeShopItemWnd@@UAEHIIJ@Z", "?WindowProc@NCShopItemWnd@@UAEHIIJ@Z")   // NCRecipeShopItemWnd::WindowProc = NCShopItemWnd::WindowProc (0x100a2850)
#endif

// 0x100d7090 (vslot 38): the stock body is NCShopItemWnd's (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCRecipeShopItemWnd::OnDestroy() NW_STOCK(0x100d7090)
#else
NW_SLOT_IS("?OnDestroy@NCRecipeShopItemWnd@@UAEHXZ", "?OnDestroy@NCShopItemWnd@@UAEHXZ")   // NCRecipeShopItemWnd::OnDestroy = NCShopItemWnd::OnDestroy (0x100d7090)
#endif

// 0x10088870 (ret 4): the body NCStoreItemWnd::SetTarget hooks (Target at +0x148)
#if NW_HOOK_MODE
NW_NAKED void NCRecipeShopItemWnd::SetTarget(NCRecipeShopItemWnd* InTarget) NW_STOCK(0x10088870)
#else
NW_SLOT_IS("?SetTarget@NCRecipeShopItemWnd@@QAEXPAV1@@Z", "?SetTarget@NCManorShopItemWnd@@QAEXPAV1@@Z")   // NCRecipeShopItemWnd::SetTarget = NCManorShopItemWnd::SetTarget (0x10088870)
#endif

// 0x100de910 (ret 4): the body NCStoreItemWnd::SetMode hooks (Mode at +0x14C)
#if NW_HOOK_MODE
NW_NAKED void NCRecipeShopItemWnd::SetMode(INT InMode) NW_STOCK(0x100de910)
#else
NW_SLOT_IS("?SetMode@NCRecipeShopItemWnd@@QAEXH@Z", "?SetMode@NCManorShopItemWnd@@QAEXH@Z")   // NCRecipeShopItemWnd::SetMode = NCManorShopItemWnd::SetMode (0x100de910)
#endif

// 0x100e0680 - a new entry from the item record (icon from its name, "NWindow.BlackTexture" when missing, written back
// into Info), its TradeCount the record's count; the tool tip is the name line (NTT_ItemName), with the count line
// (NTTO_Count) on the Mode 0 grid. 0 when the grid is full.
UBOOL NCRecipeShopItemWnd::AddItem(NItemInfo* Info)
{
    if (MaxItems <= Items.Num())
        return 0;
    Info->Icon = FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = NewItemEntry(Info);
    Item->TradeCount = Info->Count;
    if (Mode == 0)
        Item->FillToolTip(1, 0x20);
    else if (Mode == 1)
        Item->FillToolTip(1, 0);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100e0680, NCRecipeShopItemWnd, AddItem)

// 0x100e07a0 - a copy of another grid's entry (the move of a recipe into the Mode 0 grid), same tool tip as AddItem
UBOOL NCRecipeShopItemWnd::AddCopy(NItemEntry* Src)
{
    if (MaxItems <= Items.Num())
        return 0;
    NItemEntry* Item = (NItemEntry*)appMalloc(sizeof(NItemEntry), TEXT("new"));
    if (Item)
        Item = new ((void*)Item) NItemEntry;
    Item->Copy(Src);
    if (Mode == 0)
        Item->FillToolTip(1, 0x20);
    else if (Mode == 1)
        Item->FillToolTip(1, 0);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100e07a0, NCRecipeShopItemWnd, AddCopy)

// 0x100e0880 - the entry goes to the other grid: a copy into the Mode 0 grid unless it holds the recipe already (the ids
// are compared), out of this grid (not freed, as in the stock code) when the target is the Mode 1 grid
void NCRecipeShopItemWnd::MoveItem(NItemEntry* Item)
{
    if (Target)
    {
        UnselectAll();
        if (Target->Mode == 0)
        {
            for (INT i = 0; i < Target->Items.Num(); i++)
                if (Target->Items(i)->ID == Item->ID)
                    goto Done;
            Target->AddCopy(Item);
        }
        else if (Target->Mode == 1)
            Items.Remove(Selected, 1);
Done:
        Selected = -1;
        UpdateScrollRange();
    }
}
NW_HOOK_METHOD(0x100e0880, NCRecipeShopItemWnd, MoveItem)

// 0x100e1330 (vslot 63) - the carried entry dropped onto the other grid: the Mode 0 grid asks the price first, the Mode 1
// grid takes it back at once (C3: the cursor is left alone)
INT NCRecipeShopItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (Selected >= 0 && Selected < Items.Num() && bDragging)
    {
        if (!IsInWindow(X, Y))
        {
            NWndEntry* E = FindWindowAt(X, Y);
            if (E && E->Wnd && E->Wnd->IsA(NClass::NCRecipeShopItemWnd()))
            {
                Target = (NCRecipeShopItemWnd*)E->Wnd;
                NItemEntry* Item = Items(Selected);
                if (Target->Mode == 0)
                    AskPrice(this, Item, MSG_PriceToBuyDrop);
                else if (Target->Mode == 1)
                    MoveItem(Item);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100e1330, NCRecipeShopItemWnd, OnLButtonUp)

// 0x100e14c0 (vslot 75) - a double click moves the entry the same way (the target is the other grid already)
INT NCRecipeShopItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemEntry* Item = Items(Selected);
        if (Target->Mode == 0)
            AskPrice(this, Item, MSG_PriceToBuy);
        else if (Target->Mode == 1)
            MoveItem(Item);
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100e14c0, NCRecipeShopItemWnd, OnLButtonDblClick)

// 0x100e1610 (vslot 76) - the price typed (wParam) goes into the selected entry, which then moves to the other grid; the
// stock code reads Items(Selected) without a bounds check
INT NCRecipeShopItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    if (Item)
    {
        Item->UnknownC20 = (INT)wParam;
        MoveItem(Item);
    }
    return 1;
}
NW_HOOK_METHOD(0x100e1610, NCRecipeShopItemWnd, OnDialogResult)

NW_CHECK_VTABLE(NCRecipeShopItemWnd, 0x10199610, 77)
