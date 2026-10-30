/*=============================================================================
    NCShopWnd.cpp - an NPC's shop window and its item grids.
    Stock (C3): 0x100f0150 .. 0x100f2a3b and OnCreate (0x100f2a40 .. 0x100f2f2d); the destructor of the window
    (0x100a4500), WindowProc (0x100a2850) and OnDestroy (0x100d7090) of the grid are copies of bodies the other
    grids keep the addresses of (WindowProc / OnDestroy: NCStoreItemWnd, NCManorShopItemWnd, NCDeliverItemWnd ...).
    The window's WindowProc is NCStoreWnd's (0x100f0070), SetTarget (0x10088870), SetMode (0x100de910), SetSide
    (0x100a28d0) of the grid, ShowWindow (0x100412c0) and SetListID (0x10116b20) of the window are shared too.
=============================================================================*/
#include "nwindow/NCShopWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCManorShopWnd.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"

enum { DIALOG_SHOP_AMOUNT = 2 };     // NDialogInfo::ID (name ours): an amount for the owner
enum { DIALOG_SHOP_LIMIT = 0x1F };   // the purchase asks for more than the NPC has (NDialogInfo::ID, name ours)
enum { MSG_HowManyToMove = 72 };     // systemmsg "How many $s1(s) do you want to move?"
enum { MSG_TooManyBought = 0x53A };  // systemmsg (C3): more of a limited item than the NPC holds
enum { SOUND_Close = 6 };            // NConsoleWnd::PlayInterfaceSound

// sysstring labels
enum
{
    STR_Adena = 134,
    STR_ItemsOnSale = 137,
    STR_Inventory = 138,
    STR_ItemsToBuy = 139,
    STR_OK = 140,
    STR_Cancel = 141,
    STR_BuyPrice = 142,              // "Price"
    STR_SellPrice = 143,             // "Price"
};

// a child window goes (repeated inline in the stock OnDestroys)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// NCFrameCtrl(Decorated, Vertical, Small): 0x10010450 (ret 0xc, NCFrameCtrl.cpp), run on the object `new` allocates
// (StaticAllocateObject of the class in the transient package)
static NCFrameCtrl* NewFrameCtrl(INT A, INT B, INT C)
{
    return new NCFrameCtrl(A, B, C);
}

// how many of a stack to move; the answer goes to Owner's OnDialogResult. Inline in each stock caller; the name buffer is
// the stock frame's 64 characters. C3 puts the item's id in the info (+0x18, the header names that field bProgress) and
// gives the box the most it may take (Unknown74), no flag for the adena any more.
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

// The tool tip of an entry of this grid, by its side and the window's mode (the same ladder is inline in each stock
// caller): the price shown depends on them. Nothing for a side or mode out of range.
static FORCEINLINE void ShopFillToolTip(NCShopItemWnd* Wnd, NItemEntry* Item)
{
    INT Extra;
    if (Wnd->Side == SHOPSIDE_Shop)
    {
        if (Wnd->Mode == SHOPMODE_Buy || Wnd->Mode == SHOPMODE_Preview)
            Extra = 7;
        else if (Wnd->Mode == SHOPMODE_Sell)
            Extra = 8;
        else
            return;
    }
    else if (Wnd->Side == SHOPSIDE_Player)
    {
        if (Wnd->Mode == SHOPMODE_Buy || Wnd->Mode == SHOPMODE_Preview)
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

// a stack joins the target's entry of the same item id when there is one, else a copy holding Count joins the target
// (inline in the stock MoveItem, once for each case)
static FORCEINLINE void AddToTarget(NCShopItemWnd* Target, NItemEntry* Item, INT Count)
{
    if (Item->IsStackable())
    {
        for (INT i = 0; i < Target->Items.Num(); i++)
        {
            if (Target->Items(i)->ID == Item->ID)
            {
                Target->Items(i)->TradeCount += Count;
                ShopFillToolTip(Target, Target->Items(i));
                return;
            }
        }
    }
    Target->AddCopy(Item, Count);
}

/*-----------------------------------------------------------------------------
    NCShopItemWnd.
-----------------------------------------------------------------------------*/

// 0x100f28f0 - Mode and Side are left as allocated
NCShopItemWnd::NCShopItemWnd()
{
    Target = NULL;
}
NW_HOOK_CTOR(0x100f28f0, NCShopItemWnd)

// 0x100f2990 (ret 0x18) - the grid of the six arguments (columns, rows, most items, icon size, gaps)
NCShopItemWnd::NCShopItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
    Target = NULL;
}
NW_HOOK_CTOR_ARGS(0x100f2990, NCShopItemWnd, (INT A, INT B, INT C, INT D, INT E, INT F), (A, B, C, D, E, F))

// 0x100f2940
NW_HOOK_DTOR(0x100f2940, NCShopItemWnd)

// 0x100a2850 (vslot 25) - the body is shared with the other grids' (NCStoreItemWnd, NCManorShopItemWnd,
// NCDeliverItemWnd): the stock keeps one address, which this hooks; the double click is the virtual at slot 75, the
// dialog box's answer the one at slot 76
INT NCShopItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return ::nw::VirtualCall<INT>(this, 75, wParam, lParam) != 0;
    if (Msg == NWM_DIALOG_RESULT)
        return ::nw::VirtualCall<INT>(this, 76, wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100a2850, NCShopItemWnd, WindowProc)

// 0x100d7090 (vslot 38) - the body is shared with the other grids' (NCStoreItemWnd, NCDeliverItemWnd, the private
// stores'): the stock keeps one address, which this hooks
INT NCShopItemWnd::OnDestroy()
{
    NCItemWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100d7090, NCShopItemWnd, OnDestroy)

// stock body shared with NCTradeItemWnd::SetKind (0x10088870)
void NCShopItemWnd::SetTarget(NCShopItemWnd* InTarget)
{
    Target = InTarget;
}

// stock body shared with NCStoreItemWnd's (0x100de910)
void NCShopItemWnd::SetMode(INT InMode)
{
    Mode = InMode;
}

// stock body shared with NCStoreItemWnd's (0x100a28d0)
void NCShopItemWnd::SetSide(INT InSide)
{
    Side = InSide;
}

// 0x100f0200 (vslot 56; the guard's name is the class's own) - the icons from the page's first row; the selection frame
// behind a selected one, the amount (red, at the icon's corner) for the NPC's goods when buying
INT NCShopItemWnd::OnPaint(NCGDevice* Device)
{
    TCHAR Text[16];
    guard(NCShopItemWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    INT X = 1;
    INT Y = 1;
    for (INT Index = TopRow * Columns; Index < Items.Num(); Index++)
    {
        NItemEntry* Item = Items(Index);
        if (!Item)
            continue;
        if (Index % Columns == 0)
        {
            X = 1;
            if (Index != TopRow * Columns)
                Y = Y + (CellGapY + CellSize);
            if ((CellSize + 1) * VisibleRows <= Y)
                break;
        }
        else
            X = X + (CellGapX + CellSize);
        if (Item->IsSelected())
            Device->DrawTexture(X - 1, Y - 1, 0x22, 0x22, 0, 0, 0x22, 0x22, SelectTex, 0xFF, 1);
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon(), 0xFF, 1);
        if (Mode == SHOPMODE_Buy && Side == SHOPSIDE_Shop && Item->Count > 0)
        {
            appSprintf(Text, TEXT("%d"), Item->Count);
            Device->DrawText(X, Y, 0xFFFF0000, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100f0200, NCShopItemWnd, OnPaint)

// 0x100f2370 (vslot 63) - the carried item dropped onto another shop grid goes there (that grid becomes the target): how
// many first for a stack (from the NPC's goods when buying, whatever its amount), else one
INT NCShopItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (bDragging)
    {
        if (!IsInWindow(X, Y))
        {
            NWndEntry* E = FindWindowAt(X, Y);
            if (E && E->Wnd && E->Wnd->IsA(NClass::NCShopItemWnd()))
            {
                Target = (NCShopItemWnd*)E->Wnd;
                NItemEntry* Item = Items(Selected);
                if ((Mode == SHOPMODE_Buy || Mode == SHOPMODE_Preview) && Target->Side == SHOPSIDE_Player)
                {
                    if (Item->IsStackable())
                        AskAmount(this, Item, Item->Count);
                    else
                        MoveItem(Item, 1);
                }
                else if (Item->IsStackable() && Item->TradeCount > 1)
                    AskAmount(this, Item, (INT)Item->TradeCount);
                else
                    MoveItem(Item, 1);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100f2370, NCShopItemWnd, OnLButtonUp)

// 0x100f2620 (vslot 75) - the item goes to the target: how many first for a stack (any stack when buying), else one
INT NCShopItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemEntry* Item = Items(Selected);
        if (Mode == SHOPMODE_Buy || Mode == SHOPMODE_Preview)
        {
            if (Item->IsStackable())
                AskAmount(this, Item, (INT)Item->TradeCount);
            else
                MoveItem(Item, 1);
        }
        else if (Item->IsStackable() && Item->TradeCount > 1)
            AskAmount(this, Item, (INT)Item->TradeCount);
        else
            MoveItem(Item, 1);
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100f2620, NCShopItemWnd, OnLButtonDblClick)

// 0x100f2890 (vslot 76) - the amount typed (wParam; when selling at most the entry's) goes to the target. The stock
// code reads Items(Selected) without a bounds check.
INT NCShopItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    INT Count = (INT)wParam;
    if (Item && Count)
    {
        if (Mode == SHOPMODE_Sell && (SQWORD)Count >= Item->TradeCount)
            Count = (INT)Item->TradeCount;
        MoveItem(Item, Count);
    }
    return 1;
}
NW_HOOK_METHOD(0x100f2890, NCShopItemWnd, OnDialogResult)

// 0x100f1290 - a new entry, its icon from its name ("NWindow.BlackTexture" when missing, written back into Info), its
// amount the item's count
UBOOL NCShopItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
    Info->Icon = FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = NewItemEntry(Info);
    Item->TradeCount = Info->Count;
    ShopFillToolTip(this, Item);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100f1290, NCShopItemWnd, AddItem)

// 0x100f13f0 - a copy of another grid's entry holding Count
UBOOL NCShopItemWnd::AddCopy(NItemEntry* Src, INT Count)
{
    if (Items.Num() >= MaxItems)
        return 0;
    NItemEntry* Item = (NItemEntry*)appMalloc(sizeof(NItemEntry), TEXT("new"));
    if (Item)
        Item = new ((void*)Item) NItemEntry;
    Item->Copy(Src);
    Item->TradeCount = Count;
    ShopFillToolTip(this, Item);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100f13f0, NCShopItemWnd, AddCopy)

// the entry has Count taken off (a stack with more than that), else it leaves the grid (not freed, as in the stock code)
static FORCEINLINE void TakeOff(NCShopItemWnd* Grid, NItemEntry* Item, INT Count)
{
    if (Item->IsStackable() && (SQWORD)Count < Item->TradeCount)
    {
        Item->TradeCount -= Count;
        ShopFillToolTip(Grid, Item);
    }
    else
        Grid->Items.Remove(Grid->Selected, 1);
}

// 0x100f1520 - the item goes to the target. Selling, the target gets the amount (the NPC's list or the inventory) and
// this entry loses it; buying, the NPC's goods stay (the list of purchases gets them) and what goes back is only taken
// off; in the preview's mode (2) the goods leave the NPC's list and come back when they are put back. An emptied entry
// leaves the grid; then the total of the shop window shown, else the preview window's, else the manor shop's.
void NCShopItemWnd::MoveItem(NItemEntry* Item, INT Count)
{
    if (!Target)
        return;
    UnselectAll();
    if (Mode == SHOPMODE_Sell)
    {
        // the stock code has one identical branch for each side
        if (Target->Side == SHOPSIDE_Shop || Target->Side == SHOPSIDE_Player)
        {
            AddToTarget(Target, Item, Count);
            TakeOff(this, Item, Count);
        }
    }
    else if (Mode == SHOPMODE_Buy || Mode == SHOPMODE_Preview)
    {
        if (Target->Side == SHOPSIDE_Shop)
        {
            if (Mode == SHOPMODE_Preview)
                AddToTarget(Target, Item, Count);
            TakeOff(this, Item, Count);
        }
        else if (Target->Side == SHOPSIDE_Player)
        {
            AddToTarget(Target, Item, Count);
            if (Mode == SHOPMODE_Preview)
                TakeOff(this, Item, Count);
        }
    }
    NCShopWnd* Shop = GDesktop->ShopWnd;
    if (Shop && Shop->IsEnabled() && Shop->IsShowWindow())
        Shop->UpdateTotalPrice();
    else
    {
        NCShopWnd* Preview = (NCShopWnd*)GDesktop->ShopPreviewWnd;
        if (Preview && Preview->IsEnabled() && Preview->IsShowWindow())
            Preview->UpdateTotalPrice();
        else
            ((NCManorShopWnd*)GDesktop->NewWndE4)->UpdateTotalPrice();   // 0x100a3450, the manor shop (desktop +0x118)
    }
    Selected = -1;
    UpdateScrollRange();
}
NW_HOOK_METHOD(0x100f1520, NCShopItemWnd, MoveItem)

NW_CHECK_VTABLE(NCShopItemWnd, 0x1019ce38, 77)

/*-----------------------------------------------------------------------------
    NCShopWnd.
-----------------------------------------------------------------------------*/

// 0x100f29d0 - Mode is left as allocated
NCShopWnd::NCShopWnd()
{
    PlayerItems = NULL;
    ShopItems = NULL;
    Unknown9C = 0;
    BackTex = NULL;
    UnknownA4 = 0;
    UpButton = NULL;
    DownButton = NULL;
    OKButton = NULL;
    CancelButton = NULL;
    UnknownB8 = NULL;
    Adena = 0;
    TotalPrice = 0;
    ListID = -1;
}
NW_HOOK_CTOR(0x100f29d0, NCShopWnd)

// 0x100a4500 - the destructor (the base's only; the manor shop's and the preview's call it too)
NW_HOOK_DTOR(0x100a4500, NCShopWnd)

// stock body shared with NCStoreWnd::WindowProc (0x100f0070), which keeps the address: the buttons
INT NCShopWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_STORE_OK)
            return OnOK() != 0;
        if (wParam == NID_STORE_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_STORE_UP)
            return OnUp() != 0;
        if (wParam == NID_STORE_DOWN)
            return OnDown() != 0;
        if (wParam == NID_STORE_CLOSE)
            return OnClose() != 0;
    }
    return 0;
}

// 0x100f2a40 (vslot 37) - title bar (as wide as the window), the grids (6 x 4 icons for the NPC's goods, 6 x 3 below,
// 160 items at most each), the arrows between them, OK and Cancel
INT NCShopWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.StoreWnd.Store1_back"));

    FrameCtrl = NewFrameCtrl(1, 0, 0);
    FrameID = NID_STORE_FRAME;
    FrameCtrl->ID = NID_STORE_FRAME;
    FrameCtrl->SetPosition(0, 0, (DWORD)(INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    ShopItems = new NCShopItemWnd(6, 4, 160, 32, 5, 3);
    ShopItems->SetPosition(9, 48, 239, 139);
    CreateChild(ShopItems, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    ShopItems->SetSide(SHOPSIDE_Shop);
    PlayerItems = new NCShopItemWnd(6, 3, 160, 32, 5, 3);
    PlayerItems->SetPosition(9, 215, 239, 104);
    CreateChild(PlayerItems, NWS_VISIBLE | 0x20);
    PlayerItems->SetSide(SHOPSIDE_Player);
    ShopItems->SetTarget(PlayerItems);
    PlayerItems->SetTarget(ShopItems);

    // the up arrow's pushed texture is its normal one, as in the stock code
    UpButton = new NCButton(TEXT("L2UI_CH3.Button.UpButton"), TEXT("L2UI_CH3.Button.UpButton"), 0);
    UpButton->ID = NID_STORE_UP;
    UpButton->SetPosition(112, 194, 15, 15);
    CreateChild(UpButton, NWS_VISIBLE);
    DownButton = new NCButton(TEXT("L2UI_CH3.Button.DownButton"), TEXT("L2UI_CH3.Button.DownButton"), 0);
    DownButton->ID = NID_STORE_DOWN;
    DownButton->SetPosition(130, 194, 15, 15);
    CreateChild(DownButton, NWS_VISIBLE);

    OKButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    OKButton->ID = NID_STORE_OK;
    OKButton->SetPosition(51, 372, 76, 23);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(STR_OK);
    CancelButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    CancelButton->ID = NID_STORE_CANCEL;
    CancelButton->SetPosition(131, 372, 76, 23);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(STR_Cancel);
    return 1;
}
NW_HOOK_METHOD(0x100f2a40, NCShopWnd, OnCreate)

// 0x100f0920 (vslot 38)
INT NCShopWnd::OnDestroy()
{
    DestroyChild(this, PlayerItems);
    DestroyChild(this, ShopItems);
    DestroyChild(this, UpButton);
    DestroyChild(this, DownButton);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    DestroyChild(this, UnknownB8);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100f0920, NCShopWnd, OnDestroy)

// stock body shared with NCEditBox's (0x100412c0): the layout comes with SetMode
void NCShopWnd::ShowWindow()
{
    NCWnd::ShowWindow();
}

// 0x100f01a0 (vslot 45)
void NCShopWnd::HideWindow()
{
    Clear();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x100f01a0, NCShopWnd, HideWindow)

// 0x100f0420 (vslot 56) - the grids' labels by mode (none for the limited list), the total's and the adena's labels
// right-aligned at 156, their values at 247, thousands separated (the total, 64 bits, through "%I64d")
INT NCShopWnd::OnPaint(NCGDevice* Device)
{
    TCHAR Text[128];
    TCHAR TotalText[128];
    TCHAR AdenaText[128];
    SIZE LabelSize;
    SIZE Size;
    guard(NCShopWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
    {
        INT H = (INT)(Height - 20.0f);
        INT W = (INT)Width;
        Device->DrawTexture(0, 20, W, H, 0, 0, W, H, BackTex, 0xFF, 1);
    }
    if (Mode == SHOPMODE_Buy)
    {
        Device->DrawText(11, 32, 0xFFDCDCDC, *SysString(STR_ItemsOnSale), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        Device->DrawText(11, 198, 0xFFDCDCDC, *SysString(STR_ItemsToBuy), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&LabelSize, *SysString(STR_BuyPrice), 0, 0, 0, 0, 0, L2FONT_Normal);
        Device->DrawText(156 - LabelSize.cx, 331, 0xFFDCDCDC, *SysString(STR_BuyPrice), 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                         0, 0, 0, 0);
        NCGDevice::GetTextExtent(&LabelSize, *SysString(STR_Adena), 0, 0, 0, 0, 0, L2FONT_Normal);
        Device->DrawText(156 - LabelSize.cx, 350, 0xFFDCDCDC, *SysString(STR_Adena), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                         0, 0, 0);
    }
    else if (Mode == SHOPMODE_Sell)
    {
        Device->DrawText(11, 32, 0xFFDCDCDC, *SysString(STR_Inventory), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        Device->DrawText(11, 198, 0xFFDCDCDC, *SysString(STR_ItemsOnSale), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&LabelSize, *SysString(STR_SellPrice), 0, 0, 0, 0, 0, L2FONT_Normal);
        Device->DrawText(156 - LabelSize.cx, 331, 0xFFDCDCDC, *SysString(STR_SellPrice), 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                         0, 0, 0, 0);
        NCGDevice::GetTextExtent(&LabelSize, *SysString(STR_Adena), 0, 0, 0, 0, 0, L2FONT_Normal);
        Device->DrawText(156 - LabelSize.cx, 350, 0xFFDCDCDC, *SysString(STR_Adena), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                         0, 0, 0);
    }
    appSprintf(Text, TEXT("%I64d"), TotalPrice);
    FormatNumber(TotalText, Text);
    appSprintf(Text, TEXT("%s"), TotalText);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(247 - Size.cx, 331, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("%d"), Adena);
    FormatNumber(AdenaText, Text);
    appSprintf(Text, TEXT("%s"), AdenaText);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(247 - Size.cx, 350, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100f0420, NCShopWnd, OnPaint)

// the entries of a grid that hold something
static FORCEINLINE INT CountFilled(NCShopItemWnd* Grid)
{
    INT Num = 0;
    for (INT i = 0; i < Grid->Items.Num(); i++)
        if (Grid->Items(i)->TradeCount > 0)
            Num++;
    return Num;
}

// 0x100f0c00 (vslot 68) - the lower grid's list to the server: list id, count, then (object id, item id, amount) of
// each item sold or (item id, amount) of each item bought; the window closes. C3 first refuses a purchase of more of a
// limited item (the NPC's count above 0) than the NPC holds.
INT NCShopWnd::OnOK()
{
    if (Mode == SHOPMODE_Sell)
    {
        INT Num = CountFilled(ShopItems);
        L2ParamStack Params(Num * 3 + 2);
        Params.PushBack((void*)(INT_PTR)ListID);
        Params.PushBack((void*)(INT_PTR)Num);
        for (INT i = 0; i < ShopItems->Items.Num(); i++)
        {
            if (ShopItems->Items(i)->TradeCount > 0)
            {
                Params.PushBack((void*)(INT_PTR)ShopItems->Items(i)->ObjectID);
                Params.PushBack((void*)(INT_PTR)ShopItems->Items(i)->ID);
                Params.PushBack((void*)(INT_PTR)(INT)ShopItems->Items(i)->TradeCount);
            }
        }
        GConsole->RequestSellItem(Params);
    }
    else if (Mode == SHOPMODE_Buy)
    {
        for (INT i = 0; i < ShopItems->Items.Num(); i++)
        {
            NItemEntry* Goods = ShopItems->Items(i);
            if (Goods->Count > 0)
            {
                INT Wanted = 0;
                for (INT j = 0; j < PlayerItems->Items.Num(); j++)
                {
                    NItemEntry* Bought = PlayerItems->Items(j);
                    if (Bought->ID == Goods->ID)
                    {
                        if (Bought->IsStackable())
                            Wanted += (INT)Bought->TradeCount;
                        else
                            Wanted++;
                    }
                }
                if (Goods->Count < Wanted)
                {
                    NDialogInfo Info;
                    Info.ID = DIALOG_SHOP_LIMIT;
                    Info.bMainBox = 1;
                    Info.bInput = 0;
                    Info.SetSystemMessage(MSG_TooManyBought);
                    GConsole->ShowDialog(&Info);
                    return 1;
                }
            }
        }
        INT Num = CountFilled(PlayerItems);
        L2ParamStack Params(Num * 3 + 2);
        Params.PushBack((void*)(INT_PTR)ListID);
        Params.PushBack((void*)(INT_PTR)Num);
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
        {
            if (PlayerItems->Items(i)->TradeCount > 0)
            {
                Params.PushBack((void*)(INT_PTR)PlayerItems->Items(i)->ID);
                Params.PushBack((void*)(INT_PTR)(INT)PlayerItems->Items(i)->TradeCount);
            }
        }
        GConsole->RequestBuyItem(Params);
    }
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100f0c00, NCShopWnd, OnOK)

// 0x100f01b0 (vslot 69)
INT NCShopWnd::OnCancel()
{
    Clear();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100f01b0, NCShopWnd, OnCancel)

// 0x100f01d0 (vslot 72) - the close button's: OnCancel's work and the window's closing sound
INT NCShopWnd::OnClose()
{
    Clear();
    HideWindow();
    GConsole->PlayInterfaceSound(SOUND_Close);
    return 1;
}
NW_HOOK_METHOD(0x100f01d0, NCShopWnd, OnClose)

// the arrows: the selected item of Grid goes to its target, how many first for a stack of more than one - or for any
// stack with bAnyStack, the down arrow on the NPC's list. Inline in the stock OnUp / OnDown, twice each (the count
// asked is TradeCount; the NPC's list, when buying, asks for its Count)
static FORCEINLINE void MoveSelected(NCShopItemWnd* Grid, UBOOL bAnyStack, UBOOL bAskCount)
{
    INT Sel = Grid->GetSelected();
    if (Sel < 0 || Sel >= Grid->Items.Num())
        return;
    NItemEntry* Item = Grid->Items(Sel);
    if (!Item)
        return;
    if (bAnyStack)
    {
        if (Item->IsStackable())
        {
            AskAmount(Grid, Item, bAskCount ? Item->Count : (INT)Item->TradeCount);
            return;
        }
    }
    else if (Item->IsStackable() && Item->TradeCount > 1)
    {
        AskAmount(Grid, Item, (INT)Item->TradeCount);
        return;
    }
    Grid->MoveItem(Item, 1);
}

// 0x100f1e10 (vslot 70) - the up arrow: from the lower grid
INT NCShopWnd::OnUp()
{
    if (Mode == SHOPMODE_Buy || Mode == SHOPMODE_Preview)
        MoveSelected(PlayerItems, 0, 0);
    else if (Mode == SHOPMODE_Sell)
        MoveSelected(ShopItems, 0, 0);
    return 1;
}
NW_HOOK_METHOD(0x100f1e10, NCShopWnd, OnUp)

// 0x100f20d0 (vslot 71) - the down arrow: from the upper grid (the NPC's list asks for the stack it holds)
INT NCShopWnd::OnDown()
{
    if (Mode == SHOPMODE_Buy || Mode == SHOPMODE_Preview)
        MoveSelected(ShopItems, 1, 1);
    else if (Mode == SHOPMODE_Sell)
        MoveSelected(PlayerItems, 0, 0);
    return 1;
}
NW_HOOK_METHOD(0x100f20d0, NCShopWnd, OnDown)

// 0x100f0150 - one body with the setters of other windows' +0xF8 (the private stores')
void NCShopWnd::SetAdena(INT InAdena)
{
    Adena = InAdena;
}
NW_HOOK_METHOD(0x100f0150, NCShopWnd, SetAdena)

// stock body shared with NCStoreWnd::SetAdena (0x10116b20)
void NCShopWnd::SetListID(INT InListID)
{
    ListID = InListID;
}

// 0x100f0160 - kept out of line like the stock code (HideWindow, OnCancel)
__declspec(noinline) void NCShopWnd::Clear()
{
    ListID = -1;
    Adena = 0;
    TotalPrice = 0;
    PlayerItems->Clear();
    ShopItems->Clear();
}
NW_HOOK_METHOD(0x100f0160, NCShopWnd, Clear)

// 0x100f0a70 (out of line like the stock code) - buying, the NPC's grid on top (4 rows) and the purchases below (3 rows);
// selling, the inventory on top and what is sold below
__declspec(noinline) void NCShopWnd::UpdateLayout()
{
    NRectF Origin;
    if (Mode == SHOPMODE_Buy || Mode == SHOPMODE_Preview)
    {
        ShopItems->UpdateScrollRange();
        PlayerItems->UpdateScrollRange();
        // the stock code fills { LocalX, LocalY, LocalW, LocalH }; SetScreenPos reads x and y
        Origin.X = 0;
        Origin.Y = 0;
        *(INT*)&Origin.W = 0;
        *(INT*)&Origin.H = 0;
        GetLocalRect(&Origin);
        ShopItems->SetScreenPos(9, 48, (const POINT*)&Origin);
        PlayerItems->SetScreenPos(9, 215, (const POINT*)&Origin);
        ::nw::VirtualCall<void>(ShopItems, 68, 239, 139);
        ::nw::VirtualCall<void>(PlayerItems, 68, 239, 104);
        PlayerItems->SetVisibleRows(3);
        ShopItems->SetVisibleRows(4);
    }
    else if (Mode == SHOPMODE_Sell)
    {
        ShopItems->UpdateScrollRange();
        PlayerItems->UpdateScrollRange();
        Origin.X = 0;
        Origin.Y = 0;
        *(INT*)&Origin.W = 0;
        *(INT*)&Origin.H = 0;
        GetLocalRect(&Origin);
        ShopItems->SetScreenPos(9, 215, (const POINT*)&Origin);
        PlayerItems->SetScreenPos(9, 48, (const POINT*)&Origin);
        ::nw::VirtualCall<void>(ShopItems, 68, 239, 104);
        ::nw::VirtualCall<void>(PlayerItems, 68, 239, 139);
        PlayerItems->SetVisibleRows(4);
        ShopItems->SetVisibleRows(3);
    }
}
NW_HOOK_METHOD(0x100f0a70, NCShopWnd, UpdateLayout)

// 0x100f0fa0 - the sum of price x amount over the lower grid, in 64 bits (the manor shop's and the preview's too)
__declspec(noinline) void NCShopWnd::UpdateTotalPrice()
{
    TotalPrice = 0;
    if (Mode == SHOPMODE_Buy || Mode == SHOPMODE_Preview)
    {
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
            TotalPrice += (SQWORD)PlayerItems->Items(i)->GetUnknownC20() * PlayerItems->Items(i)->TradeCount;
    }
    else if (Mode == SHOPMODE_Sell)
    {
        for (INT i = 0; i < ShopItems->Items.Num(); i++)
            TotalPrice += (SQWORD)ShopItems->Items(i)->GetUnknownC20() * ShopItems->Items(i)->TradeCount;
    }
}
NW_HOOK_METHOD(0x100f0fa0, NCShopWnd, UpdateTotalPrice)

// one grid's entries get their name and description again from the item tables, and their tool tips (the stock code
// repeats this for each grid, with the CRT's wcscpy). The tool tip's price follows the side and mode of Ladder: the
// stock code takes them from the NPC's grid for the player's entries too.
static FORCEINLINE void ReloadItemTexts(NCShopItemWnd* Grid, NCShopItemWnd* Ladder)
{
    for (INT i = 0; i < Grid->Items.Num(); i++)
    {
        NItemEntry* Item = Grid->Items(i);
        if (Item)
        {
            wcscpy(Item->Name, GL2GameData.GetItemName(Item->ID));
            wcscpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
            ShopFillToolTip(Ladder, Item);
        }
    }
}

// 0x100f10b0 - after the item texts were reloaded (NConsoleWnd::ReloadGameTexts)
void NCShopWnd::ReloadTexts()
{
    ReloadItemTexts(ShopItems, ShopItems);
    ReloadItemTexts(PlayerItems, ShopItems);
}
NW_HOOK_METHOD(0x100f10b0, NCShopWnd, ReloadTexts)

// 0x100f1db0
void NCShopWnd::SetMode(INT InMode)
{
    Mode = InMode;
    ShopItems->SetMode(Mode);
    PlayerItems->SetMode(Mode);
    UpdateLayout();
}
NW_HOOK_METHOD(0x100f1db0, NCShopWnd, SetMode)

// 0x100f1df0
UBOOL NCShopWnd::AddShopItem(NItemInfo* Info)
{
    return ShopItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100f1df0, NCShopWnd, AddShopItem)

// 0x100f1e00
UBOOL NCShopWnd::AddPlayerItem(NItemInfo* Info)
{
    return PlayerItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100f1e00, NCShopWnd, AddPlayerItem)

// 0x101189a0 (name ours) - folded with NCStoreWnd::GetMode: both windows keep the mode at +0xF0
INT NCShopWnd::GetMode()
{
    return Mode;
}
NW_HOOK_METHOD(0x101189a0, NCShopWnd, GetMode)

NW_CHECK_VTABLE(NCShopWnd, 0x1018e5d0, 73)
