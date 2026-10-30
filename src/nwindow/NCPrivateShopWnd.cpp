/*=============================================================================
    NCPrivateShopWnd.cpp - the private store (sell) window and its grids. Stock (C3): 0x100d6fb0 ..
    0x100da180 (the window, its grids and its title bar); ShowWindow (0x100d71d0), HideWindow (0x100d7250),
    UpdateTotal (0x100d7f70) and CheckTotal (0x100d7ea0) are one body with the buy window's (NCPrivateBuyWnd.cpp).
=============================================================================*/
#include "nwindow/NCPrivateShopWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NItemToolTip.h"

// NWM_COMMAND ids (names ours)
enum
{
    NID_STORE_ABORT = 0xF0,
    NID_STORE_FRAME = 0xF3,
    NID_STORE_START = 0xF4,
    NID_STORE_UP = 0xF6,
    NID_STORE_DOWN = 0xF7,
    NID_STORE_CLOSE = 0xF8,
    NID_STORE_MESSAGE = 0xF9,
};

// NDialogInfo::ID of the questions (names ours)
enum
{
    DIALOG_STORE_PRICE = 0x25,           // C1: 0x21 (the five ids moved up by four)
    DIALOG_STORE_AMOUNT = 0x26,
    DIALOG_STORE_MESSAGE = 0x27,
    DIALOG_STORE_PRICE_CAUTION = 0x28,
    DIALOG_STORE_BUY_CAUTION = 0x29,
};

// systemmsg
enum
{
    MSG_MoveHowMany = 72,       // "How many $s1(s) do you want to move?"
    MSG_SetPrice = 322,         // "Please decide on the sales price."
    MSG_StoreMessage = 334,     // "Please enter store message."
    MSG_PriceCaution = 569,     // "Caution--the item price greatly differs from the shop's
                                //   standard price. Do you wish to continue?"
    MSG_TooMany = 1036,         // more than the entry holds
};

// sysstring
enum
{
    STR_Adena = 134,
    STR_ItemsOnSale = 137,
    STR_Inventory = 138,
    STR_ItemsToBuy = 139,
    STR_OK = 140,
    STR_Price = 142,
    STR_PriceSell = 143,
    STR_Message = 384,
    STR_Abort = 385,
    STR_Start = 428,
};

// a child window goes (repeated inline in the stock OnDestroys)
template <class T> static FORCEINLINE void DestroyStoreChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// a price 5 times away from the item's reference price, either way (the reference 0: none)
static FORCEINLINE UBOOL IsUnusualPrice(NItemEntry* Item)
{
    return Item->Unknown1CA8 > 0
        && (Item->UnknownC20 <= Item->Unknown1CA8 / 5 || Item->UnknownC20 >= Item->Unknown1CA8 * 5);
}

// the desktop's box asks how many of the item to move (NWM_STORE_AMOUNT answers the grid); the most it takes is the
// entry's amount in the trade. C3: the item's id goes with it (+0x18 of the info; the header names that field bProgress)
static FORCEINLINE void AskAmountIn(NDialogInfo& Info, NCWnd* Grid, NItemEntry* Item)
{
    TCHAR Name[64];
    appSprintf(Name, TEXT("%s"), Item->GetName());
    Info.ID = DIALOG_STORE_AMOUNT;
    Info.Type = 3;
    Info.Owner = Grid->Entry;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.SetSystemMessage(MSG_MoveHowMany);
    Info.SetButtons(Name, NULL);
    ::nw::Field<INT>(&Info, 0x18) = Item->ID;
    Info.Unknown74 = (INT)Item->TradeCount;
    GConsole->ShowDialog(&Info);
}

static FORCEINLINE void AskAmount(NCWnd* Grid, NItemEntry* Item)
{
    NDialogInfo Info;
    AskAmountIn(Info, Grid, Item);
}

// ... the price, up to 100,000,000 (NWM_STORE_PRICE answers)
static FORCEINLINE void AskPrice(NCWnd* Grid, NItemEntry* Item)
{
    NDialogInfo Info;
    Info.ID = DIALOG_STORE_PRICE;
    Info.Type = 3;
    Info.Owner = Grid->Entry;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.bLargeAmount = 1;
    Info.SetSystemMessage(MSG_SetPrice);
    ::nw::Field<INT>(&Info, 0x18) = Item->ID;
    GConsole->ShowDialog(&Info);
}

// the tool tip of an entry by the grid's side: the player's side (1) and the store's (0) read different columns
static FORCEINLINE void PrivateShopToolTip(NItemEntry* Item, INT Side)
{
    if (Side == 1)
        Item->FillToolTip(2, 0);
    else if (Side == 0)
        Item->FillToolTip(2, 4);
}

// the entry of the partner's list a stack joins: the same item id (inline in the stock MoveItem)
static FORCEINLINE NItemEntry* FindStackIn(NCPrivateShopItemWnd* Grid, NItemEntry* Item)
{
    if (Item->IsStackable())
    {
        for (INT i = 0; i < Grid->Items.Num(); i++)
            if (Grid->Items(i)->ID == Item->ID)
                return Grid->Items(i);
    }
    return NULL;
}

/*-----------------------------------------------------------------------------
    NCPrivateShopFrameCtrl.
-----------------------------------------------------------------------------*/

// 0x100d72d0 (vslot 70) - closing from the bar: the window's own close (a store of one's own is told to cancel) when
// the bar sits in the desktop's sell window, then the frame's
INT NCPrivateShopFrameCtrl::OnClose()
{
    NWndEntry* Parent = Entry->Parent;
    if (Parent)
    {
        NCWnd* W = Parent->Wnd;
        if (W == (NCWnd*)GDesktop->PrivateShopWnd)
        {
            if (((NCPrivateShopWnd*)W)->Mode == 1)
                GConsole->RequestPrivateStoreQuitSell();
            ((NCPrivateShopWnd*)Entry->Parent->Wnd)->Reset();
        }
    }
    return NCFrameCtrl::OnClose();
}
NW_HOOK_METHOD(0x100d72d0, NCPrivateShopFrameCtrl, OnClose)

// 0x100d9930 - NCFrameCtrl's (a decorated bar) and this class's vtable
NCPrivateShopFrameCtrl::NCPrivateShopFrameCtrl()
{
}
NW_HOOK_CTOR(0x100d9930, NCPrivateShopFrameCtrl)

NW_CHECK_VTABLE(NCPrivateShopFrameCtrl, 0x10197ed8, 72)

// 0x100d9980
NW_HOOK_DTOR(0x100d9980, NCPrivateShopFrameCtrl)

/*-----------------------------------------------------------------------------
    NCPrivateShopItemWnd.
-----------------------------------------------------------------------------*/

// 0x100d99d0
NCPrivateShopItemWnd::NCPrivateShopItemWnd()
{
    Partner = NULL;
    Unknown124 = 0;
}
NW_HOOK_CTOR(0x100d99d0, NCPrivateShopItemWnd)

// 0x100d9a70
NCPrivateShopItemWnd::NCPrivateShopItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
    Partner = NULL;
    Unknown124 = 0;
}
NW_HOOK_CTOR_ARGS(0x100d9a70, NCPrivateShopItemWnd, (INT A, INT B, INT C, INT D, INT E, INT F), (A, B, C, D, E, F))

// 0x100d9a20
NW_HOOK_DTOR(0x100d9a20, NCPrivateShopItemWnd)

// 0x100d6fb0 (vslot 25) - the double click and the dialog box's answers (the price, the amount, the unusual price
// confirmed or not)
INT NCPrivateShopItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    if (Msg == NWM_STORE_PRICE_CANCEL)
        return OnPriceCancel(wParam, lParam) != 0;
    // slots 77, 78, 79 (the header keeps them as placeholders: the bodies are this class's OnPrice, OnAmount, OnPriceOK)
    if (Msg == NWM_STORE_PRICE)
        return ::nw::VirtualCall<INT>(this, 77, wParam, lParam) != 0;
    if (Msg == NWM_STORE_AMOUNT)
        return ::nw::VirtualCall<INT>(this, 78, wParam, lParam) != 0;
    if (Msg == NWM_STORE_PRICE_OK)
        return ::nw::VirtualCall<INT>(this, 79, wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100d6fb0, NCPrivateShopItemWnd, WindowProc)

// stock body shared with NCDeliverItemWnd's (0x100d7090)
INT NCPrivateShopItemWnd::OnDestroy()
{
    NCItemWnd::OnDestroy();
    return 1;
}

// 0x100d8ed0 (vslot 63) - the drop of the carried item: at someone's store (Mode 0) a stack asks how many; setting
// one's own up, going on sale asks the price, a stack going back how many (with more than one)
INT NCPrivateShopItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (Selected >= 0 && Selected < Items.Num() && bDragging)
    {
        if (!IsInWindow(X, Y))
        {
            NWndEntry* E = FindWindowAt(X, Y);
            if (E && E->Wnd && E->Wnd->IsA(NCPrivateShopItemWnd::StaticClass()))
            {
                Partner = (NCPrivateShopItemWnd*)E->Wnd;
                NItemEntry* Item = Items(Selected);
                if (Mode == 0)
                {
                    if (Item->IsStackable())
                        AskAmount(this, Item);
                    else
                        MoveItem(Item, 1);
                }
                else if (Partner->Side == 0)
                {
                    TCHAR Name[64];
                    appSprintf(Name, TEXT("%s"), Item->GetName());
                    AskPrice(this, Item);
                }
                else if (Item->IsStackable() && Item->Count > 1)
                    AskAmount(this, Item);
                else
                    MoveItem(Item, 1);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100d8ed0, NCPrivateShopItemWnd, OnLButtonUp)

// 0x100d9230 (vslot 75) - OnLButtonUp's moves, onto Partner (a stack going back asks how many whatever its count)
INT NCPrivateShopItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemEntry* Item = Items(Selected);
        if (Mode == 0)
        {
            if (Item->IsStackable())
                AskAmount(this, Item);
            else
                MoveItem(Item, 1);
        }
        else if (Partner->Side == 0)
        {
            TCHAR Name[64];
            appSprintf(Name, TEXT("%s"), Item->GetName());
            AskPrice(this, Item);
        }
        else if (Item->IsStackable())
            AskAmount(this, Item);
        else
            MoveItem(Item, 1);
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100d9230, NCPrivateShopItemWnd, OnLButtonDblClick)

// 0x100d9660 (vslot 77) - the price typed for the selected item (setting one's own store up): it takes it; a price 5
// times away from the reference asks again (0: the box stays), then a stack how many; the move's answer is the result
INT NCPrivateShopItemWnd::OnPrice(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    INT Price = (INT)wParam;
    if (!Item)
        return 1;
    if (!Price)
        return 1;
    if (Mode == 1 && Price > 0)
    {
        // the stock frame keeps both records (the amount's and the caution's) apart: so do these two
        NDialogInfo AmountInfo;
        NDialogInfo Info;
        INT Reference = Item->Unknown1CA8;
        Item->UnknownC20 = Price;
        if (Reference > 0 && (Price <= Reference / 5 || Price >= Reference * 5))
        {
            Info.ID = DIALOG_STORE_PRICE_CAUTION;
            Info.Owner = Entry;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.SetSystemMessage(MSG_PriceCaution);
            GConsole->ShowDialog(&Info);
            return 0;
        }
        if (!Item->IsStackable() || Item->TradeCount <= 1)
            return MoveItem(Item, 1);
        AskAmountIn(AmountInfo, this, Item);
        return 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x100d9660, NCPrivateShopItemWnd, OnPrice)

// 0x100d9880 (vslot 78) - the amount typed: that many of the selected item move; a system message refuses more than
// there is; the move's answer is the result
INT NCPrivateShopItemWnd::OnAmount(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    INT Amount = (INT)wParam;
    if (!Item)
        return 1;
    if (!Amount)
        return 1;
    if (Mode == 1 && Amount > 0)
        Unknown124 = 0;
    else
    {
        if (Mode != 0)
            return 1;
        if (Amount < 1)
            return 1;
    }
    if (Item->TradeCount < Amount)
    {
        GConsole->ShowSystemMessage(MSG_TooMany, 0xFFDCDCDC);
        return 1;
    }
    return MoveItem(Item, Amount);
}
NW_HOOK_METHOD(0x100d9880, NCPrivateShopItemWnd, OnAmount)

// 0x100d9520 (vslot 79) - the unusual price confirmed: a stack asks how many, anything else moves (a missing item too,
// as in the stock code)
INT NCPrivateShopItemWnd::OnPriceOK(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    if (Item && Item->IsStackable() && Item->TradeCount > 1)
    {
        AskAmount(this, Item);
        return 0;
    }
    return MoveItem(Item, 1);
}
NW_HOOK_METHOD(0x100d9520, NCPrivateShopItemWnd, OnPriceOK)

// the partner (the stock body is 0x10088870, one with the other grids' setters of the field at +0x148)
void NCPrivateShopItemWnd::SetPartner(NCPrivateShopItemWnd* InPartner)
{
    Partner = InPartner;
}

// the stock body is 0x100de910, one with the other grids' setters of the field at +0x14C
void NCPrivateShopItemWnd::SetMode(INT InMode)
{
    Mode = InMode;
}

// the stock body is 0x100a28d0, one with the other grids' setters of the field at +0x150
void NCPrivateShopItemWnd::SetSide(INT InSide)
{
    Side = InSide;
}

// 0x100d82e0 - the icon comes from its name ("NWindow.BlackTexture" when missing), written back into Info; the whole
// count is there (C3: 64 bits)
UBOOL NCPrivateShopItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
    Info->Icon = FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = NewItemEntry(Info);
    Item->TradeCount = Info->Count;
    PrivateShopToolTip(Item, Side);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100d82e0, NCPrivateShopItemWnd, AddItem)

// 0x100d8400 - a copy of Src with Count there
UBOOL NCPrivateShopItemWnd::AddEntry(NItemEntry* Src, INT Count)
{
    if (Items.Num() >= MaxItems)
        return 0;
    NItemEntry* Item = (NItemEntry*)appMalloc(sizeof(NItemEntry), TEXT("new"));
    if (Item)
        Item = new ((void*)Item) NItemEntry;
    Item->Copy(Src);
    Item->TradeCount = Count;
    PrivateShopToolTip(Item, Side);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100d8400, NCPrivateShopItemWnd, AddEntry)

// the same body as the buy window's CheckTotal (0x100d7ea0, hooked there): the grids call the one of the desktop's window
UBOOL NCPrivateShopWnd::CheckTotal(INT ItemID, INT Count, INT Price)
{
    INT Sum = 0;
    INT Total = 0;
    for (INT i = 0; i < Count; i++)
    {
        Sum += Price;
        if (Sum > 2000000000)
            return 0;
        if (Sum < 0)
            return 0;
    }
    if (StoreItems)
    {
        for (INT i = 0; i < StoreItems->Items.Num(); i++)
        {
            NItemEntry* Entry = StoreItems->Items(i);
            if (Entry->ID == ItemID)
                Total += (INT)Entry->TradeCount * Price;
            else
                Total += Entry->GetUnknownC20() * (INT)Entry->TradeCount;
        }
    }
    return Sum + Total <= 2000000000 && Sum + Total >= 0;
}

// 0x100d84f0 - Count of the selected Item to Partner: a stack joins the same item there (going on sale it takes this
// price, and it first checks that the list's total stays within two billion), anything else a copy; what is left stays,
// or the entry leaves this grid (not freed, as in the stock code). The window adds up its price again.
INT NCPrivateShopItemWnd::MoveItem(NItemEntry* Item, INT Count)
{
    if (!Partner)
        return 1;
    UnselectAll();
    if (Mode == 1 || Mode == 0)
    {
        if (Partner->Side == 0 || Partner->Side == 1)
        {
            UBOOL bPrice = Mode == 1 && Partner->Side == 0;
            if (bPrice && !GDesktop->PrivateShopWnd->CheckTotal(Item->ID, Count, Item->GetUnknownC20()))
                return 0;
            NItemEntry* Same = FindStackIn(Partner, Item);
            if (Same)
            {
                Same->TradeCount += Count;
                if (bPrice)
                    Same->UnknownC20 = Item->UnknownC20;
                PrivateShopToolTip(Same, Partner->Side);
            }
            else
                Partner->AddEntry(Item, Count);
            if (Item->IsStackable() && Count < Item->TradeCount)
            {
                Item->TradeCount -= Count;
                PrivateShopToolTip(Item, Side);
            }
            else
                Items.Remove(Selected, 1);
        }
    }
    GDesktop->PrivateShopWnd->UpdateTotal();
    Selected = -1;
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100d84f0, NCPrivateShopItemWnd, MoveItem)

NW_CHECK_VTABLE(NCPrivateShopItemWnd, 0x10197ff8, 80)

/*-----------------------------------------------------------------------------
    NCPrivateShopWnd.
-----------------------------------------------------------------------------*/

// 0x100d9ac0 - Mode is left as allocated
NCPrivateShopWnd::NCPrivateShopWnd()
{
    PlayerItems = NULL;
    StoreItems = NULL;
    Unknown9C = 0;
    BackTex = NULL;
    UnknownA4 = 0;
    UpButton = NULL;
    DownButton = NULL;
    StartButton = NULL;
    MessageButton = NULL;
    AbortButton = NULL;
    UnknownBC = NULL;
    Adena = 0;
    TotalPrice = 0;
    OwnerID = -1;
}
NW_HOOK_CTOR(0x100d9ac0, NCPrivateShopWnd)

// 0x100d9b70
NW_HOOK_DTOR(0x100d9b70, NCPrivateShopWnd)

// 0x100d70a0 (vslot 25) - the buttons and the unusual prices confirmed
INT NCPrivateShopWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_STORE_START)
            return OnStart() != 0;
        if (wParam == NID_STORE_UP)
            return OnUp() != 0;
        if (wParam == NID_STORE_DOWN)
            return OnDown() != 0;
        if (wParam == NID_STORE_CLOSE)
            return OnClose() != 0;
        if (wParam == NID_STORE_MESSAGE)
            return OnMessage() != 0;
        if (wParam == NID_STORE_ABORT)
            return OnAbort() != 0;
    }
    else if (Msg == NWM_STORE_BUY_OK)
        return ::nw::VirtualCall<INT>(this, 74, wParam, lParam) != 0;   // slot 74 (a placeholder in the header: OnBuyConfirmed)
    return 0;
}
NW_HOOK_METHOD(0x100d70a0, NCPrivateShopWnd, WindowProc)

// 0x100d71c0 - the player's adena in the store window (the console sets it); the stock body is shared with the buy
// window's SetAdena
void NCPrivateShopWnd::SetAdena(INT InAdena)
{
    Adena = InAdena;
}
NW_HOOK_METHOD(0x100d71c0, NCPrivateShopWnd, SetAdena)

// stock body shared with NCShopWnd::SetAdena (0x100efa00); the console sets it
void NCPrivateShopWnd::SetOwnerID(INT InOwnerID)
{
    OwnerID = InOwnerID;
}

// 0x100d3e40 - the stock keeps one body with the buy window's (its fields sit at the same places)
void NCPrivateShopWnd::Reset()
{
    OwnerID = -1;
    Adena = 0;
    TotalPrice = 0;
    PlayerItems->Clear();
    StoreItems->Clear();
}
NW_HOOK_METHOD(0x100d3e40, NCPrivateShopWnd, Reset)

// 0x100d7260 (vslot 71) - closing one's own store also tells the server (NetHandler slot 197); the close sound
INT NCPrivateShopWnd::OnClose()
{
    if (Mode == 1)
        GConsole->RequestPrivateStoreManageCancel();
    Reset();
    HideWindow();
    GConsole->PlayInterfaceSound(6);
    return 1;
}
NW_HOOK_METHOD(0x100d7260, NCPrivateShopWnd, OnClose)

// 0x100d72a0 (vslot 73) - Abort: the store closes (NetHandler slot 198)
INT NCPrivateShopWnd::OnAbort()
{
    GConsole->RequestPrivateStoreQuitSell();
    Reset();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100d72a0, NCPrivateShopWnd, OnAbort)

// 0x100d7320 (vslot 56) - the texts of the two grids, the amount of the list and the player's adena; the clip is the
// window's, the texture (0, 20)
INT NCPrivateShopWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 20, (INT)Width, (INT)(Height - 20.0f), 0, 0, (INT)Width, (INT)(Height - 20.0f), BackTex, 0xFF, 1);
    SIZE Size;
    if (Mode == 0)
    {
        Device->DrawText(11, 32, 0xFFDCDCDC, *SysString(STR_ItemsOnSale), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->DrawText(11, 198, 0xFFDCDCDC, *SysString(STR_ItemsToBuy), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *SysString(STR_Price), 0, 0, 0, 0, 0);
        Device->DrawText(156 - Size.cx, 331, 0xFFDCDCDC, *SysString(STR_Price), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *SysString(STR_Adena), 0, 0, 0, 0, 0);
        Device->DrawText(156 - Size.cx, 350, 0xFFDCDCDC, *SysString(STR_Adena), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else if (Mode == 1)
    {
        Device->DrawText(11, 32, 0xFFDCDCDC, *SysString(STR_Inventory), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->DrawText(11, 198, 0xFFDCDCDC, *SysString(STR_ItemsOnSale), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *SysString(STR_PriceSell), 0, 0, 0, 0, 0);
        Device->DrawText(156 - Size.cx, 331, 0xFFDCDCDC, *SysString(STR_PriceSell), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *SysString(STR_Adena), 0, 0, 0, 0, 0);
        Device->DrawText(156 - Size.cx, 350, 0xFFDCDCDC, *SysString(STR_Adena), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }

    // "%d" takes the total's low half and its high one is the next argument, as in the stock code
    TCHAR Buf[132];
    TCHAR Total[128];
    TCHAR AdenaText[128];
    appSprintf(Buf, TEXT("%d"), TotalPrice);
    FormatNumber(Total, Buf);
    appSprintf(Buf, TEXT("%s"), Total);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawText(247 - Size.cx, 331, 0xFFDCDCDC, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Buf, TEXT("%d"), Adena);
    FormatNumber(AdenaText, Buf);
    appSprintf(Buf, TEXT("%s"), AdenaText);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawText(247 - Size.cx, 350, 0xFFDCDCDC, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100d7320, NCPrivateShopWnd, OnPaint)

// 0x100d79c0 - also the buy window's (the one body): the store's list on top at someone's store, below when setting
// one's own up; the two grids are 139 and 104 pixels tall with 4 and 3 rows
void NCPrivateShopWnd::UpdateLayout()
{
    // the stock code fills { LocalX, LocalY, LocalW, LocalH }; SetScreenPos reads x and y
    NRectF Origin;
    if (Mode == 0)
    {
        StoreItems->UpdateScrollRange();
        PlayerItems->UpdateScrollRange();
        Origin.X = 0;
        Origin.Y = 0;
        *(INT*)&Origin.W = 0;
        *(INT*)&Origin.H = 0;
        GetLocalRect(&Origin);
        StoreItems->SetScreenPos(9, 48, (POINT*)&Origin);
        PlayerItems->SetScreenPos(9, 215, (POINT*)&Origin);
        ::nw::VirtualCall<void>(StoreItems, 68, 239, 139);
        ::nw::VirtualCall<void>(PlayerItems, 68, 239, 104);
        PlayerItems->SetVisibleRows(3);
        StoreItems->SetVisibleRows(4);
        return;
    }
    if (Mode == 1)
    {
        StoreItems->UpdateScrollRange();
        PlayerItems->UpdateScrollRange();
        Origin.X = 0;
        Origin.Y = 0;
        *(INT*)&Origin.W = 0;
        *(INT*)&Origin.H = 0;
        GetLocalRect(&Origin);
        StoreItems->SetScreenPos(9, 215, (POINT*)&Origin);
        PlayerItems->SetScreenPos(9, 48, (POINT*)&Origin);
        ::nw::VirtualCall<void>(StoreItems, 68, 239, 104);
        ::nw::VirtualCall<void>(PlayerItems, 68, 239, 139);
        StoreItems->SetVisibleRows(3);
        PlayerItems->SetVisibleRows(4);
    }
}
NW_HOOK_METHOD(0x100d79c0, NCPrivateShopWnd, UpdateLayout)

// 0x100d7b40 (vslot 68) - Start: the store opens with the list on sale (its count, then the object id, count and price
// of each; NetHandler slot 196). OK at someone's store: the items chosen are bought (the owner's object id first, then
// the object id, count and price of each; slot 200), unusual prices asking first.
INT NCPrivateShopWnd::OnStart()
{
    if (Mode == 1)
    {
        INT Count = 0;
        for (INT i = 0; i < StoreItems->Items.Num(); i++)
            if (StoreItems->Items(i)->TradeCount > 0)
                Count++;
        L2ParamStack Params(Count * 3 + 1);
        Params.PushBack((void*)(INT_PTR)Count);
        for (INT i = 0; i < StoreItems->Items.Num(); i++)
        {
            NItemEntry* Item = StoreItems->Items(i);
            if (Item->TradeCount > 0)
            {
                Params.PushBack((void*)(INT_PTR)Item->ObjectID);
                Params.PushBack((void*)(INT_PTR)Item->TradeCount);
                Params.PushBack((void*)(INT_PTR)Item->UnknownC20);
            }
        }
        GConsole->RequestSetPrivateStoreListSell(Params);
        Reset();
        HideWindow();
    }
    else if (Mode == 0)
    {
        INT Count = 0;
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
            if (PlayerItems->Items(i)->TradeCount > 0)
                Count++;
        L2ParamStack Params(Count * 3 + 2);
        Params.PushBack((void*)(INT_PTR)OwnerID);
        Params.PushBack((void*)(INT_PTR)Count);
        UBOOL bUsual = 1;
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
        {
            NItemEntry* Item = PlayerItems->Items(i);
            if (Item->TradeCount > 0)
            {
                if (IsUnusualPrice(Item))
                    bUsual = 0;
                Params.PushBack((void*)(INT_PTR)Item->ObjectID);
                Params.PushBack((void*)(INT_PTR)Item->TradeCount);
                Params.PushBack((void*)(INT_PTR)Item->UnknownC20);
            }
        }
        if (bUsual)
        {
            GConsole->RequestPrivateStoreBuy(Params);
            Reset();
            HideWindow();
        }
        else
        {
            NDialogInfo Info;
            Info.ID = DIALOG_STORE_BUY_CAUTION;
            Info.Owner = Entry;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.SetSystemMessage(MSG_PriceCaution);
            GConsole->ShowDialog(&Info);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100d7b40, NCPrivateShopWnd, OnStart)

// 0x100d8080 (vslot 72) - Message: the store's message, typed in the desktop's box (dialog 0x27 hands it to the console);
// the box starts with the message the player's pawn has now (the FString at +0x161C of the pawn)
INT NCPrivateShopWnd::OnMessage()
{
    NDialogInfo Info;
    Info.ID = DIALOG_STORE_MESSAGE;
    Info.Owner = PlayerItems->Entry;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.SetSystemMessage(MSG_StoreMessage);
    Info.bLargeAmount = 0;
    Info.Type = 0;
    const TCHAR* Text = NULL;
    User* Me = GConsole->GetMyUser();
    if (Me && Me->Pawn)
        Text = *nw::Field<FString>(Me->Pawn, 0x161C);
    Info.SetText60(Text);
    GConsole->ShowDialog(&Info);
    return 1;
}
NW_HOOK_METHOD(0x100d8080, NCPrivateShopWnd, OnMessage)

// 0x100d8160 (vslot 74) - buying at the unusual prices confirmed
INT NCPrivateShopWnd::OnBuyConfirmed(WPARAM wParam, LPARAM lParam)
{
    if (Mode == 0)
    {
        INT Count = 0;
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
            if (PlayerItems->Items(i)->TradeCount > 0)
                Count++;
        L2ParamStack Params(Count * 3 + 2);
        Params.PushBack((void*)(INT_PTR)OwnerID);
        Params.PushBack((void*)(INT_PTR)Count);
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
        {
            NItemEntry* Item = PlayerItems->Items(i);
            if (Item->TradeCount > 0)
            {
                Params.PushBack((void*)(INT_PTR)Item->ObjectID);
                Params.PushBack((void*)(INT_PTR)Item->TradeCount);
                Params.PushBack((void*)(INT_PTR)Item->UnknownC20);
            }
        }
        GConsole->RequestPrivateStoreBuy(Params);
    }
    Reset();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100d8160, NCPrivateShopWnd, OnBuyConfirmed)

// 0x100d8970 - both grids take the mode; they move to its places
void NCPrivateShopWnd::SetMode(INT InMode)
{
    Mode = InMode;
    StoreItems->SetMode(Mode);
    PlayerItems->SetMode(Mode);
    UpdateLayout();
}
NW_HOOK_METHOD(0x100d8970, NCPrivateShopWnd, SetMode)

// 0x100d89b0 - one's own list adds up again
void NCPrivateShopWnd::AddStoreItem(NItemInfo* Info)
{
    StoreItems->AddItem(Info);
    if (Mode == 1)
        UpdateTotal();
}
NW_HOOK_METHOD(0x100d89b0, NCPrivateShopWnd, AddStoreItem)

// 0x100d89e0
UBOOL NCPrivateShopWnd::AddPlayerItem(NItemInfo* Info)
{
    return PlayerItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100d89e0, NCPrivateShopWnd, AddPlayerItem)

// 0x100d89f0 (vslot 69) - the up arrow: the selected item of the lower grid goes up (a stack of more than one asks how
// many)
INT NCPrivateShopWnd::OnUp()
{
    if (Mode == 0)
    {
        INT Sel = PlayerItems->GetSelected();
        if (Sel < 0)
            return 1;
        if (Sel >= PlayerItems->Items.Num())
            return 1;
        NItemEntry* Item = PlayerItems->Items(Sel);
        if (!Item)
            return 1;
        if (Item->IsStackable() && Item->TradeCount > 1)
        {
            AskAmount(PlayerItems, Item);
            return 1;
        }
        PlayerItems->MoveItem(Item, 1);
    }
    else
    {
        if (Mode != 1)
            return 1;
        INT Sel = StoreItems->GetSelected();
        if (Sel < 0)
            return 1;
        if (Sel >= StoreItems->Items.Num())
            return 1;
        NItemEntry* Item = StoreItems->Items(Sel);
        if (!Item)
            return 1;
        if (Item->IsStackable() && Item->TradeCount > 1)
        {
            AskAmount(StoreItems, Item);
            return 1;
        }
        StoreItems->MoveItem(Item, 1);
    }
    return 1;
}
NW_HOOK_METHOD(0x100d89f0, NCPrivateShopWnd, OnUp)

// 0x100d8cb0 (vslot 70) - the down arrow: at someone's store the selected item comes down (a stack asks how many);
// setting one's own up, the price first
INT NCPrivateShopWnd::OnDown()
{
    if (Mode == 0)
    {
        INT Sel = StoreItems->GetSelected();
        if (Sel < 0)
            return 1;
        if (Sel >= StoreItems->Items.Num())
            return 1;
        NItemEntry* Item = StoreItems->Items(Sel);
        if (!Item)
            return 1;
        if (!Item->IsStackable())
        {
            StoreItems->MoveItem(Item, 1);
            return 1;
        }
        AskAmount(StoreItems, Item);
    }
    else
    {
        if (Mode != 1)
            return 1;
        INT Sel = PlayerItems->GetSelected();
        if (Sel < 0)
            return 1;
        if (Sel >= PlayerItems->Items.Num())
            return 1;
        NItemEntry* Item = PlayerItems->Items(Sel);
        if (!Item)
            return 1;
        AskPrice(PlayerItems, Item);
    }
    return 1;
}
NW_HOOK_METHOD(0x100d8cb0, NCPrivateShopWnd, OnDown)

// 0x100d9bc0 (vslot 37) - the buy window's OnCreate with the sell grids (the sizes swapped)
INT NCPrivateShopWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.StoreWnd.Store1_back"));

    ShopFrame = new NCPrivateShopFrameCtrl();
    FrameID = NID_STORE_FRAME;
    ShopFrame->ID = NID_STORE_FRAME;
    ShopFrame->SetPosition(0, 0, (INT)Width, 20);
    CreateChild(ShopFrame, NWS_VISIBLE);

    StoreItems = new NCPrivateShopItemWnd(6, 3, 160, 32, 5, 3);
    StoreItems->SetPosition(9, 215, 239, 104);
    CreateChild(StoreItems, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    StoreItems->SetSide(0);

    PlayerItems = new NCPrivateShopItemWnd(6, 4, 160, 32, 5, 3);
    PlayerItems->SetPosition(9, 48, 239, 139);
    CreateChild(PlayerItems, NWS_VISIBLE | 0x20);
    PlayerItems->SetSide(1);

    StoreItems->SetPartner(PlayerItems);
    PlayerItems->SetPartner(StoreItems);

    UpButton = new NCButton(TEXT("L2UI_CH3.Button.UpButton"), TEXT("L2UI_CH3.Button.UpButton"), 0);
    UpButton->ID = NID_STORE_UP;
    UpButton->SetPosition(112, 194, 15, 15);
    CreateChild(UpButton, NWS_VISIBLE);

    DownButton = new NCButton(TEXT("L2UI_CH3.Button.DownButton"), TEXT("L2UI_CH3.Button.DownButton"), 0);
    DownButton->ID = NID_STORE_DOWN;
    DownButton->SetPosition(130, 194, 15, 15);
    CreateChild(DownButton, NWS_VISIBLE);

    StartButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    StartButton->ID = NID_STORE_START;
    StartButton->SetPosition(164, 372, 76, 23);
    CreateChild(StartButton, NWS_VISIBLE);
    StartButton->SetTextID(STR_Start);

    MessageButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    MessageButton->ID = NID_STORE_MESSAGE;
    MessageButton->SetPosition(87, 372, 76, 23);
    CreateChild(MessageButton, NWS_VISIBLE);
    MessageButton->SetTextID(STR_Message);

    AbortButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    AbortButton->ID = NID_STORE_ABORT;
    AbortButton->SetPosition(10, 372, 76, 23);
    CreateChild(AbortButton, NWS_VISIBLE);
    AbortButton->SetTextID(STR_Abort);
    return 1;
}
NW_HOOK_METHOD(0x100d9bc0, NCPrivateShopWnd, OnCreate)

// 0x100d7820 (vslot 38) - the children go (each one out of the window's list, then deleted), the title bar first
INT NCPrivateShopWnd::OnDestroy()
{
    DestroyStoreChild(this, ShopFrame);
    DestroyStoreChild(this, PlayerItems);
    DestroyStoreChild(this, StoreItems);
    DestroyStoreChild(this, UpButton);
    DestroyStoreChild(this, DownButton);
    DestroyStoreChild(this, StartButton);
    DestroyStoreChild(this, UnknownBC);
    DestroyStoreChild(this, MessageButton);
    DestroyStoreChild(this, AbortButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100d7820, NCPrivateShopWnd, OnDestroy)

// stock body shared with NCPrivateBuyWnd's (0x100d71d0, hooked there)
void NCPrivateShopWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Mode == 1)
    {
        AbortButton->ShowWindow();
        MessageButton->ShowWindow();
        StartButton->SetTextID(STR_Start);
    }
    else if (Mode == 0)
    {
        AbortButton->HideWindow();
        MessageButton->HideWindow();
        StartButton->SetTextID(STR_OK);
    }
}

// stock body shared with NCPrivateBuyWnd's (0x100d7250, hooked there)
void NCPrivateShopWnd::HideWindow()
{
    Reset();
    NCWnd::HideWindow();
}

// stock body shared with NCPrivateBuyWnd's (0x100d7f70, hooked there)
void NCPrivateShopWnd::UpdateTotal()
{
    TotalPrice = 0;
    if (Mode == 0)
    {
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
            TotalPrice += (SQWORD)PlayerItems->Items(i)->GetUnknownC20() * PlayerItems->Items(i)->TradeCount;
    }
    else if (Mode == 1)
    {
        for (INT i = 0; i < StoreItems->Items.Num(); i++)
            TotalPrice += (SQWORD)StoreItems->Items(i)->GetUnknownC20() * StoreItems->Items(i)->TradeCount;
    }
}

NW_CHECK_VTABLE(NCPrivateShopWnd, 0x10198138, 75)
