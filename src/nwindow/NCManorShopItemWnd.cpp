/*=============================================================================
    NCManorShopItemWnd.cpp - the item grids of the manor's shop (a copy of NCShopItemWnd).
    Stock (C3): 0x100a3560 .. 0x100a3980 (AddItem, AddCopy, MoveItem, between the window's functions), 0x100a3f40 ..
    0x100a45ff (OnLButtonUp, OnLButtonDblClick, OnDialogResult, the constructors, the destructor). WindowProc
    (0x100a2850), OnDestroy (0x100d7090), SetTarget (0x10088870), SetMode (0x100de910) and SetSide (0x100a28d0) are
    copies of bodies other classes keep the addresses of (NCShopWnd.cpp hooks them).
=============================================================================*/
#include "nwindow/NCManorShopItemWnd.h"
#include "nwindow/NCShopWnd.h"
#include "nwindow/NCManorShopWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"

enum { DIALOG_SHOP_AMOUNT = 2 };     // NDialogInfo::ID (name ours): an amount for the owner (the shop's)
enum { MSG_HowManyToMove = 72 };     // systemmsg "How many $s1(s) do you want to move?"

// how many of a stack to move; the answer goes to Owner's OnDialogResult. Inline in each stock caller; the name buffer is
// the stock frame's 64 characters. C3 puts the item's id in the info (+0x18, the header names that field bProgress) and
// gives the box the most it may take (Unknown74: the entry's Count). The same as the shop's.
static FORCEINLINE void AskAmount(NCWnd* Owner, NItemEntry* Item, INT Max)
{
    TCHAR Name[64];
    appSprintf(Name, TEXT("%s"), Item->GetName());
    NDialogInfo Info;
    Info.ID = DIALOG_SHOP_AMOUNT;
    Info.Type = 3;
    Info.Owner = Owner->Entry;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.SetSystemMessage(MSG_HowManyToMove);
    Info.SetButtons(Name, NULL);
    ::nw::Field<INT>(&Info, 0x18) = Item->ID;
    Info.Unknown74 = Max;
    GConsole->ShowDialog(&Info);
}

// The tool tip of an entry of this grid, by its side and the grid's mode (inline in each stock caller): the shop's
// ladder with one constant changed (5 for the NPC's goods when buying, where the shop passes 7). Nothing for a side or
// mode out of range.
static FORCEINLINE void ManorFillToolTip(NCManorShopItemWnd* Wnd, NItemEntry* Item)
{
    INT Extra;
    if (Wnd->Side == SHOPSIDE_Shop)
    {
        if (Wnd->Mode == SHOPMODE_Buy)
            Extra = 5;
        else if (Wnd->Mode == SHOPMODE_Sell)
            Extra = 8;
        else
            return;
    }
    else if (Wnd->Side == SHOPSIDE_Player)
    {
        if (Wnd->Mode == SHOPMODE_Buy)
            Extra = 4;
        else if (Wnd->Mode == SHOPMODE_Sell)
            Extra = 8;
        else
            return;
    }
    else
        return;
    Item->FillToolTip(2, Extra);
}

// 0x100a4580
NCManorShopItemWnd::NCManorShopItemWnd()
{
    Target = NULL;
}
NW_HOOK_CTOR(0x100a4580, NCManorShopItemWnd)

// 0x100a4620 (ret 0x18) - the grid of the six arguments (columns, rows, most items, icon size, gaps)
NCManorShopItemWnd::NCManorShopItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX,
                                       INT InGapY)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
    Target = NULL;
}
NW_HOOK_CTOR_ARGS(0x100a4620, NCManorShopItemWnd, (INT A, INT B, INT C, INT D, INT E, INT F), (A, B, C, D, E, F))

// 0x100a45d0
NW_HOOK_DTOR(0x100a45d0, NCManorShopItemWnd)

// stock body shared with NCShopItemWnd::WindowProc (0x100a2850, hooked there): the double click (slot 75) and the
// dialog box's answer (slot 76)
INT NCManorShopItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    if (Msg == NWM_DIALOG_RESULT)
        return OnDialogResult(wParam, lParam) != 0;
    return 0;
}

// stock body shared with NCShopItemWnd::OnDestroy (0x100d7090, hooked there)
INT NCManorShopItemWnd::OnDestroy()
{
    NCItemWnd::OnDestroy();
    return 1;
}

// 0x10088870 (ret 4) - stock body shared with the other grids' setter of +0x148 (NCDeliverItemWnd's Target, NCInvenItemWnd's
// bEnchantMode, NCShopItemWnd's Target)
void NCManorShopItemWnd::SetTarget(NCManorShopItemWnd* InTarget)
{
    Target = InTarget;
}
NW_HOOK_METHOD(0x10088870, NCManorShopItemWnd, SetTarget)

// 0x100de910 (ret 4) - stock body shared with the other grids' setter of +0x14C
void NCManorShopItemWnd::SetMode(INT InMode)
{
    Mode = InMode;
}
NW_HOOK_METHOD(0x100de910, NCManorShopItemWnd, SetMode)

// 0x100a28d0 (ret 4) - stock body shared with the other grids' setter of +0x150
void NCManorShopItemWnd::SetSide(INT InSide)
{
    Side = InSide;
}
NW_HOOK_METHOD(0x100a28d0, NCManorShopItemWnd, SetSide)

// 0x100a3f40 (vslot 63) - the carried item dropped onto another manor grid goes there (that grid becomes the target):
// how many first for a stack (from the NPC's list to the purchases whatever its amount), else one
INT NCManorShopItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (Selected >= 0 && Selected < Items.Num() && bDragging)
    {
        if (!IsInWindow(X, Y))
        {
            NWndEntry* E = FindWindowAt(X, Y);
            if (E && E->Wnd && E->Wnd->IsA(NClass::NCManorShopItemWnd()))
            {
                Target = (NCManorShopItemWnd*)E->Wnd;
                NItemEntry* Item = Items(Selected);
                if (Mode == SHOPMODE_Buy && Target->Side == SHOPSIDE_Player)
                {
                    if (Item->IsStackable())
                    {
                        AskAmount(this, Item, Item->Count);
                        bDragging = 0;
                        return 1;
                    }
                }
                else if (Item->IsStackable() && Item->TradeCount > 1)
                {
                    AskAmount(this, Item, Item->Count);
                    bDragging = 0;
                    return 1;
                }
                MoveItem(Item, 1);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100a3f40, NCManorShopItemWnd, OnLButtonUp)

// 0x100a4210 (vslot 75) - the item goes to the target: how many first for a stack (any stack when buying), else one
INT NCManorShopItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemEntry* Item = Items(Selected);
        if (Mode == SHOPMODE_Buy)
        {
            if (Item->IsStackable())
            {
                AskAmount(this, Item, Item->Count);
                bDragging = 0;
                return 1;
            }
        }
        else if (Item->IsStackable() && Item->TradeCount > 1)
        {
            AskAmount(this, Item, Item->Count);
            bDragging = 0;
            return 1;
        }
        MoveItem(Item, 1);
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100a4210, NCManorShopItemWnd, OnLButtonDblClick)

// 0x100a4470 (vslot 76) - the amount typed (wParam; at most the entry's, in both modes) goes to the target
INT NCManorShopItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    if (Selected < 0 || Selected >= Items.Num())
        return 1;
    NItemEntry* Item = Items(Selected);
    if (!Item)
        return 1;
    INT Count = (INT)wParam;
    if (Count)
    {
        if ((Mode == SHOPMODE_Sell || Mode == SHOPMODE_Buy) && Item->TradeCount <= (SQWORD)Count)
            Count = (INT)Item->TradeCount;
        MoveItem(Item, Count);
    }
    return 1;
}
NW_HOOK_METHOD(0x100a4470, NCManorShopItemWnd, OnDialogResult)

// 0x100a3560 - a new entry, its icon from its name ("NWindow.BlackTexture" when missing, written back into Info), its
// amount the item's count
UBOOL NCManorShopItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
    Info->Icon = FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = NewItemEntry(Info);
    Item->TradeCount = Info->Count;
    ManorFillToolTip(this, Item);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100a3560, NCManorShopItemWnd, AddItem)

// 0x100a36b0 - a copy of another grid's entry holding Count
UBOOL NCManorShopItemWnd::AddCopy(NItemEntry* Src, INT Count)
{
    if (Items.Num() >= MaxItems)
        return 0;
    NItemEntry* Item = (NItemEntry*)appMalloc(sizeof(NItemEntry), TEXT("new"));
    if (Item)
        Item = new ((void*)Item) NItemEntry;
    Item->Copy(Src);
    Item->TradeCount = Count;
    ManorFillToolTip(this, Item);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100a36b0, NCManorShopItemWnd, AddCopy)

// 0x100a37e0 - the whole item goes to the target (the amount joins an entry of the same item there when it is a stack)
// and this entry loses it (an emptied one leaves the grid, not freed, as in the stock code); then the total of the shop
// window shown, else the manor shop's
void NCManorShopItemWnd::MoveItem(NItemEntry* Item, INT Count)
{
    if (!Target)
        return;
    UnselectAll();
    UBOOL bJoined = 0;
    if (Item->IsStackable())
    {
        for (INT i = 0; i < Target->Items.Num(); i++)
        {
            if (Target->Items(i)->ID == Item->ID)
            {
                Target->Items(i)->TradeCount += Count;
                Target->Items(i)->FillToolTip(2, 0);
                bJoined = 1;
                break;
            }
        }
    }
    if (!bJoined)
        Target->AddCopy(Item, Count);
    if (Item->IsStackable() && (SQWORD)Count < Item->TradeCount)
    {
        Item->TradeCount -= Count;
        Item->FillToolTip(2, 0);
    }
    else
        Items.Remove(Selected, 1);
    NCShopWnd* Shop = GDesktop->ShopWnd;
    if (Shop && Shop->IsEnabled() && Shop->IsShowWindow())
        Shop->UpdateTotalPrice();
    else
        ((NCManorShopWnd*)GDesktop->NewWndE4)->UpdateTotalPrice();   // 0x100a3450 (desktop +0x118)
    Selected = -1;
    UpdateScrollRange();
}
NW_HOOK_METHOD(0x100a37e0, NCManorShopItemWnd, MoveItem)

NW_CHECK_VTABLE(NCManorShopItemWnd, 0x1018e6f8, 77)
