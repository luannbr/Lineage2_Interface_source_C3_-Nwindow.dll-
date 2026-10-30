/*=============================================================================
    NCManorShopWnd.cpp - the manor's seed and crop shop (a copy of NCShopWnd).
    Stock (C3): 0x100a28e0 .. 0x100a3a00 (WindowProc, the setters, Clear, HideWindow, OnCancel, OnClose, OnPaint, OnOK,
    OnDestroy, UpdateLayout, UpdateTotalPrice, SetMode, AddShopItem, AddPlayerItem, OnUp, OnDown), 0x100a4660 ..
    0x100a4bef (constructor, destructor, OnCreate); the grids' functions among them are in NCManorShopItemWnd.cpp.
=============================================================================*/
#include "nwindow/NCManorShopWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"

enum { DIALOG_SHOP_AMOUNT = 2 };     // NDialogInfo::ID (name ours): an amount for the owner (the shop's)
enum { MSG_HowManyToMove = 72 };     // systemmsg "How many $s1(s) do you want to move?"
enum { SOUND_Close = 6 };            // NConsoleWnd::PlayInterfaceSound

// sysstring labels (the shop's)
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

// how many of a stack to move; the answer goes to Owner's OnDialogResult. Inline in each stock caller; the name buffer is
// the stock frame's 64 characters. C3 puts the item's id in the info (+0x18, the header names that field bProgress) and
// gives the box the most it may take (Unknown74: Max). The same as the shop's.
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

// 0x100a4660 - the shop's constructor runs, and none of the copies of the fields is set (the grids' pointers come with
// OnCreate)
NCManorShopWnd::NCManorShopWnd()
{
}
NW_HOOK_CTOR(0x100a4660, NCManorShopWnd)

// 0x100a46b0
NW_HOOK_DTOR(0x100a46b0, NCManorShopWnd)

// 0x100a28e0 (vslot 25) - the shop's procedure (0x100f0070), then the OK button's test again (never reached: the shop's
// has answered it)
INT NCManorShopWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCShopWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_STORE_OK)
        return OnOK() != 0;
    return 0;
}
NW_HOOK_METHOD(0x100a28e0, NCManorShopWnd, WindowProc)

// 0x100a2940 (name ours) - the console sets the list id the request sends back
void NCManorShopWnd::SetListID(INT InListID)
{
    ListID = InListID;
}
NW_HOOK_METHOD(0x100a2940, NCManorShopWnd, SetListID)

// 0x100a2950 (name ours)
void NCManorShopWnd::SetAdena(INT InAdena)
{
    Adena = InAdena;
}
NW_HOOK_METHOD(0x100a2950, NCManorShopWnd, SetAdena)

// 0x100a2960 (name ours) - kept out of line like the stock code (HideWindow, OnCancel); the grids' Clear is virtual 70
__declspec(noinline) void NCManorShopWnd::Clear()
{
    ListID = -1;
    Adena = 0;
    TotalPrice = 0;
    PlayerItems->Clear();
    ShopItems->Clear();
}
NW_HOOK_METHOD(0x100a2960, NCManorShopWnd, Clear)

// 0x100a29a0 (vslot 45)
void NCManorShopWnd::HideWindow()
{
    Clear();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x100a29a0, NCManorShopWnd, HideWindow)

// 0x100a29b0 (vslot 69)
INT NCManorShopWnd::OnCancel()
{
    Clear();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100a29b0, NCManorShopWnd, OnCancel)

// 0x100a29d0 (vslot 72) - OnCancel's work and the window's closing sound
INT NCManorShopWnd::OnClose()
{
    Clear();
    HideWindow();
    GConsole->PlayInterfaceSound(SOUND_Close);
    return 1;
}
NW_HOOK_METHOD(0x100a29d0, NCManorShopWnd, OnClose)

// slots 70..72 of this shop's vtable: its own OnUp / OnDown / OnClose (hooked above). The placeholders it inherits from
// NCShopWnd would reach the plain shop's functions (0x100f1e10 ...), not the manor shop's
#if NW_HOOK_MODE
NW_NAKED void NCManorShopWnd::NewVirtual70() NW_STOCK_AT(0x100a39e0)
#else
NW_SLOT_IS("?NewVirtual70@NCManorShopWnd@@UAEXXZ", "?OnUp@NCManorShopWnd@@QAEHXZ")   // NCManorShopWnd::NewVirtual70 = NCManorShopWnd::OnUp (0x100a39e0)
#endif
#if NW_HOOK_MODE
NW_NAKED void NCManorShopWnd::NewVirtual71() NW_STOCK_AT(0x100a3ca0)
#else
NW_SLOT_IS("?NewVirtual71@NCManorShopWnd@@UAEXXZ", "?OnDown@NCManorShopWnd@@QAEHXZ")   // NCManorShopWnd::NewVirtual71 = NCManorShopWnd::OnDown (0x100a3ca0)
#endif
#if NW_HOOK_MODE
NW_NAKED void NCManorShopWnd::NewVirtual72() NW_STOCK_AT(0x100a29d0)
#else
NW_SLOT_IS("?NewVirtual72@NCManorShopWnd@@UAEXXZ", "?OnClose@NCManorShopWnd@@QAEHXZ")   // NCManorShopWnd::NewVirtual72 = NCManorShopWnd::OnClose (0x100a29d0)
#endif

// 0x100a2a00 (vslot 56) - the shop's painting on the copies of the fields: the grids' labels by mode, the total's and
// the adena's labels right-aligned at 156, their values at 247, thousands separated (the total, 64 bits, drawn with "%d": its low half)
INT NCManorShopWnd::OnPaint(NCGDevice* Device)
{
    TCHAR Text[128];
    TCHAR TotalText[128];
    TCHAR AdenaText[128];
    SIZE LabelSize;
    SIZE Size;
    guard(NCManorShopWnd::OnPaint);
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
    appSprintf(Text, TEXT("%d"), TotalPrice);   // the low half only (the shop's draws both with "%I64d")
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
NW_HOOK_METHOD(0x100a2a00, NCManorShopWnd, OnPaint)

// the entries of a grid that hold something
static FORCEINLINE INT CountFilled(NCManorShopItemWnd* Grid)
{
    INT Num = 0;
    for (INT i = 0; i < Grid->Items.Num(); i++)
        if (Grid->Items(i)->TradeCount > 0)
            Num++;
    return Num;
}

// 0x100a2f00 (vslot 68) - the lower grid's list to the server: the list id, the count, then (object id, item id, amount)
// of each crop sold (RequestProcureCrop) or (item id, amount) of each seed bought (RequestBuySeed); the window closes
INT NCManorShopWnd::OnOK()
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
        GConsole->RequestProcureCrop(Params);
    }
    else if (Mode == SHOPMODE_Buy)
    {
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
        GConsole->RequestBuySeed(Params);
    }
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100a2f00, NCManorShopWnd, OnOK)

// 0x100a3180 (vslot 38) - the children go, in field order, then the base
INT NCManorShopWnd::OnDestroy()
{
    DestroyChild(this, PlayerItems);
    DestroyChild(this, ShopItems);
    DestroyChild(this, UpButton);
    DestroyChild(this, DownButton);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    DestroyChild(this, UnknownF8);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100a3180, NCManorShopWnd, OnDestroy)

// 0x100a32d0 - buying, the NPC's grid on top (4 rows) and the purchases below (3 rows); selling, the inventory on top
// and what is sold below
__declspec(noinline) void NCManorShopWnd::UpdateLayout()
{
    NRectF Origin;
    if (Mode == SHOPMODE_Buy)
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
        return;
    }
    if (Mode == SHOPMODE_Sell)
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
NW_HOOK_METHOD(0x100a32d0, NCManorShopWnd, UpdateLayout)

// 0x100a3450 - the sum of price x amount over the lower grid, in 64 bits
__declspec(noinline) void NCManorShopWnd::UpdateTotalPrice()
{
    TotalPrice = 0;
    if (Mode == SHOPMODE_Buy)
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
NW_HOOK_METHOD(0x100a3450, NCManorShopWnd, UpdateTotalPrice)

// 0x100a3980 (name ours) - the grids' mode too (the NPC's first), then the layout
void NCManorShopWnd::SetMode(INT InMode)
{
    Mode = InMode;
    ShopItems->SetMode(Mode);
    PlayerItems->SetMode(Mode);
    UpdateLayout();
}
NW_HOOK_METHOD(0x100a3980, NCManorShopWnd, SetMode)

// 0x100a39c0 (name ours; a tail jump into the grid's AddItem)
UBOOL NCManorShopWnd::AddShopItem(NItemInfo* Info)
{
    return ShopItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100a39c0, NCManorShopWnd, AddShopItem)

// 0x100a39d0 (name ours; a tail jump into the grid's AddItem)
UBOOL NCManorShopWnd::AddPlayerItem(NItemInfo* Info)
{
    return PlayerItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100a39d0, NCManorShopWnd, AddPlayerItem)

// the arrows: the selected item of Grid goes to its target, how many first for a stack of more than one - or for any
// stack with bAnyStack, the down arrow on the NPC's list (inline in the stock OnUp / OnDown, twice each; the amount asked
// is the entry's Count)
static FORCEINLINE void MoveSelected(NCManorShopItemWnd* Grid, UBOOL bAnyStack)
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
            AskAmount(Grid, Item, Item->Count);
            return;
        }
    }
    else if (Item->IsStackable() && Item->TradeCount > 1)
    {
        AskAmount(Grid, Item, Item->Count);
        return;
    }
    Grid->MoveItem(Item, 1);
}

// 0x100a39e0 (slot 70) - the up arrow: from the lower grid
INT NCManorShopWnd::OnUp()
{
    if (Mode == SHOPMODE_Buy)
        MoveSelected(PlayerItems, 0);
    else if (Mode == SHOPMODE_Sell)
        MoveSelected(ShopItems, 0);
    return 1;
}
NW_HOOK_METHOD(0x100a39e0, NCManorShopWnd, OnUp)

// 0x100a3ca0 (slot 71) - the down arrow: from the upper grid (the NPC's list asks for any stack)
INT NCManorShopWnd::OnDown()
{
    if (Mode == SHOPMODE_Buy)
        MoveSelected(ShopItems, 1);
    else if (Mode == SHOPMODE_Sell)
        MoveSelected(PlayerItems, 0);
    return 1;
}
NW_HOOK_METHOD(0x100a3ca0, NCManorShopWnd, OnDown)

// 0x100a4700 (vslot 37) - the shop's window: the title bar, the grids (6 x 4 icons for the NPC's, 6 x 3 below, 160
// items at most each), the arrows between them, OK and Cancel
INT NCManorShopWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.StoreWnd.Store1_back"));

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameID = NID_STORE_FRAME;
    FrameCtrl->ID = NID_STORE_FRAME;
    FrameCtrl->SetPosition(0, 0, (DWORD)(INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    ShopItems = new NCManorShopItemWnd(6, 4, 160, 32, 5, 3);
    ShopItems->SetPosition(9, 48, 239, 139);
    CreateChild(ShopItems, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    ShopItems->SetSide(SHOPSIDE_Shop);
    PlayerItems = new NCManorShopItemWnd(6, 3, 160, 32, 5, 3);
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
NW_HOOK_METHOD(0x100a4700, NCManorShopWnd, OnCreate)

NW_CHECK_VTABLE(NCManorShopWnd, 0x1018e830, 73)
