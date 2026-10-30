/*=============================================================================
    NCStoreWnd.cpp - the warehouse window, its item grids and the GM's view of a warehouse.
    Stock (C3): 0x101040c0 .. 0x10105400 (the window and its grids), 0x10103bf0 .. 0x10103cd5, the GM's view at
    0x1007e330, 0x1007e350, 0x10081970 and 0x10085060. WindowProc (0x100a2850) and OnDestroy (0x100d7090) of the grid
    and OnDestroy (0x100f0920) of the window are copies of bodies other classes keep the addresses of (NCShopWnd.cpp
    hooks them); SetTarget (0x10088870), SetMode (0x100de910) and SetSide (0x100a28d0) of the grid are one body each
    with the shop's and the private stores' grids, SetAdena (0x10116b20) and GetMode (0x101189a0) of the window one
    with the shop's.
=============================================================================*/
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NClasses.gen.h"

enum { DIALOG_STORE_AMOUNT = 3 };    // NDialogInfo::ID (name ours): an amount for the owner
enum { MSG_HowManyToMove = 72 };     // systemmsg "How many $s1(s) do you want to move?"
enum { STORE_FEE_PER_SLOT = 30 };    // adena (name ours)
enum { SOUND_Close = 6 };            // NConsoleWnd::PlayInterfaceSound

// sysstring labels
enum
{
    STR_StoredItems = 132,
    STR_ItemsToRetrieve = 133,
    STR_Adena = 134,
    STR_Fee = 135,
    STR_Inventory = 138,
    STR_OK = 140,
    STR_Cancel = 141,
};

// the warehouse window
static NCStoreWnd* StoreWnd()
{
    return GDesktop->StoreWnd;
}

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
// gives the box the most it may take (Unknown74: the entry's TradeCount).
static FORCEINLINE void AskAmount(NCWnd* Owner, NItemEntry* Item, INT Max)
{
    TCHAR Name[64];
    appSprintf(Name, TEXT("%s"), Item->GetName());
    NDialogInfo Info;
    Info.ID = DIALOG_STORE_AMOUNT;
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

/*-----------------------------------------------------------------------------
    NCStoreItemWnd.
-----------------------------------------------------------------------------*/

// 0x10105220 - Mode and Side are left as allocated
NCStoreItemWnd::NCStoreItemWnd()
{
    Target = NULL;
}
NW_HOOK_CTOR(0x10105220, NCStoreItemWnd)

// 0x101052c0 (ret 0x18) - the grid of the six arguments (columns, rows, most items, icon size, gaps)
NCStoreItemWnd::NCStoreItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
    Target = NULL;
}
NW_HOOK_CTOR_ARGS(0x101052c0, NCStoreItemWnd, (INT A, INT B, INT C, INT D, INT E, INT F), (A, B, C, D, E, F))

// 0x10105270
NW_HOOK_DTOR(0x10105270, NCStoreItemWnd)

// stock body shared with NCShopItemWnd::WindowProc (0x100a2850, hooked there): the double click (slot 75) and the
// dialog box's answer (slot 76)
INT NCStoreItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
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
INT NCStoreItemWnd::OnDestroy()
{
    NCItemWnd::OnDestroy();
    return 1;
}

// the partner grid: one body with NCTradeItemWnd::SetKind's (0x10088870, hooked there)
void NCStoreItemWnd::SetTarget(NCStoreItemWnd* InTarget)
{
    Target = InTarget;
}

// the grids' mode (+14C): one body with the shop's (0x100de910)
void NCStoreItemWnd::SetMode(INT InMode)
{
    Mode = InMode;
}

// the grids' side (+150): one body with the shop's (0x100a28d0)
void NCStoreItemWnd::SetSide(INT InSide)
{
    Side = InSide;
}

// 0x10104e80 (vslot 63) - the carried item dropped onto another warehouse grid goes there (that grid becomes the
// target): how many first for a stack of more than one, else one
INT NCStoreItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (Selected >= 0 && Selected < Items.Num() && bDragging)
    {
        if (!IsInWindow(X, Y))
        {
            NWndEntry* E = FindWindowAt(X, Y);
            if (E && E->Wnd && E->Wnd->IsA(NClass::NCStoreItemWnd()))
            {
                Target = (NCStoreItemWnd*)E->Wnd;
                NItemEntry* Item = Items(Selected);
                if (Item->IsStackable() && Item->TradeCount > 1)
                    AskAmount(this, Item, (INT)Item->TradeCount);
                else
                    MoveItem(Item, 1);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10104e80, NCStoreItemWnd, OnLButtonUp)

// 0x10105050 (vslot 75) - the item goes to the target: how many first for a stack of more than one, else one
INT NCStoreItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemEntry* Item = Items(Selected);
        if (Item->IsStackable() && Item->TradeCount > 1)
            AskAmount(this, Item, (INT)Item->TradeCount);
        else
            MoveItem(Item, 1);
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10105050, NCStoreItemWnd, OnLButtonDblClick)

// 0x101051c0 (vslot 76) - the amount typed (at most the entry's) goes to the target
INT NCStoreItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    if (Selected >= 0)
    {
        NItemEntry* Item = Items(Selected);
        INT Count = (INT)wParam;
        if (Item && Count)
        {
            if ((SQWORD)Count >= Item->TradeCount)
                Count = (INT)Item->TradeCount;
            MoveItem(Item, Count);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x101051c0, NCStoreItemWnd, OnDialogResult)

// 0x101045c0 - a new entry, its icon from its name ("NWindow.BlackTexture" when missing, written back into Info), its
// amount the item's count
UBOOL NCStoreItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
    Info->Icon = FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = NewItemEntry(Info);
    Item->TradeCount = Info->Count;
    Item->FillToolTip(2, 0);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x101045c0, NCStoreItemWnd, AddItem)

// 0x101046d0 - a copy of another grid's entry holding Count
UBOOL NCStoreItemWnd::AddCopy(NItemEntry* Src, INT Count)
{
    if (Items.Num() >= MaxItems)
        return 0;
    NItemEntry* Item = (NItemEntry*)appMalloc(sizeof(NItemEntry), TEXT("new"));
    if (Item)
        Item = new ((void*)Item) NItemEntry;
    Item->Copy(Src);
    Item->TradeCount = Count;
    Item->FillToolTip(2, 0);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x101046d0, NCStoreItemWnd, AddCopy)

// a stack joins the target's entry of the same item id when there is one (inline in the
// stock MoveItem)
static FORCEINLINE UBOOL AddToStack(NCStoreItemWnd* Target, NItemEntry* Item, INT Count)
{
    if (!Item->IsStackable())
        return 0;
    for (INT i = 0; i < Target->Items.Num(); i++)
    {
        if (Target->Items(i)->ID == Item->ID)
        {
            Target->Items(i)->TradeCount += Count;
            Target->Items(i)->FillToolTip(2, 0);
            return 1;
        }
    }
    return 0;
}

// 0x101047b0 - the item goes to the target (the amount joins an entry of the same item there when it is a stack) and
// this entry loses it (an emptied one leaves the grid, not freed, as in the stock code); then the fee is updated
void NCStoreItemWnd::MoveItem(NItemEntry* Item, INT Count)
{
    if (!Target)
        return;
    UnselectAll();
    if (!AddToStack(Target, Item, Count))
        Target->AddCopy(Item, Count);
    if (Item->IsStackable() && (SQWORD)Count < Item->TradeCount)
    {
        Item->TradeCount -= Count;
        Item->FillToolTip(2, 0);
    }
    else
        Items.Remove(Selected, 1);
    StoreWnd()->UpdateFee();
    Selected = -1;
    UpdateScrollRange();
}
NW_HOOK_METHOD(0x101047b0, NCStoreItemWnd, MoveItem)

NW_CHECK_VTABLE(NCStoreItemWnd, 0x101a05d8, 77)

/*-----------------------------------------------------------------------------
    NCStoreWnd.
-----------------------------------------------------------------------------*/

// 0x10105300 - Mode is left as allocated
NCStoreWnd::NCStoreWnd()
{
    InventoryItems = NULL;
    StoredItems = NULL;
    Unknown9C = 0;
    BackTex = NULL;
    UnknownA4 = 0;
    UpButton = NULL;
    DownButton = NULL;
    OKButton = NULL;
    CancelButton = NULL;
    UnknownB8 = NULL;
    Adena = 0;
    Fee = 0;
}
NW_HOOK_CTOR(0x10105300, NCStoreWnd)

// 0x10084200
NW_HOOK_DTOR(0x10084200, NCStoreWnd)


// 0x100f0070 (vslot 25) - the buttons; one body with NCShopWnd's and NCDeliverWnd's (and the base of NCGMStoreWnd's)
INT NCStoreWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
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
NW_HOOK_METHOD(0x100f0070, NCStoreWnd, WindowProc)

// 0x10105360 (vslot 37) - title bar (as wide as the window), the grids (6 x 4 icons for the warehouse's, 6 x 3 for the
// inventory's, 200 items at most each), the arrows between them, OK and Cancel
INT NCStoreWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.StoreWnd.Store1_back"));

    FrameCtrl = NewFrameCtrl(1, 0, 0);
    FrameID = NID_STORE_FRAME;
    FrameCtrl->ID = NID_STORE_FRAME;
    FrameCtrl->SetPosition(0, 0, (DWORD)(INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    StoredItems = new NCStoreItemWnd(6, 4, 200, 32, 5, 3);
    StoredItems->SetPosition(9, 48, 239, 139);
    CreateChild(StoredItems, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    StoredItems->SetSide(0);
    InventoryItems = new NCStoreItemWnd(6, 3, 200, 32, 5, 3);
    InventoryItems->SetPosition(9, 215, 239, 104);
    CreateChild(InventoryItems, NWS_VISIBLE | 0x20);
    InventoryItems->SetSide(1);
    StoredItems->SetTarget(InventoryItems);
    InventoryItems->SetTarget(StoredItems);

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
NW_HOOK_METHOD(0x10105360, NCStoreWnd, OnCreate)

// stock body shared with NCShopWnd's (0x100f0920, hooked there)
INT NCStoreWnd::OnDestroy()
{
    DestroyChild(this, InventoryItems);
    DestroyChild(this, StoredItems);
    DestroyChild(this, UpButton);
    DestroyChild(this, DownButton);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    DestroyChild(this, UnknownB8);
    NCWnd::OnDestroy();
    return 1;
}

// 0x10104250 (vslot 43)
void NCStoreWnd::ShowWindow()
{
    UpdateLayout();
    NCWnd::ShowWindow();
}
NW_HOOK_METHOD(0x10104250, NCStoreWnd, ShowWindow)

// 0x10103c50 (vslot 45)
void NCStoreWnd::HideWindow()
{
    Clear();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x10103c50, NCStoreWnd, HideWindow)

// 0x10103ce0 (vslot 56) - the grids' labels by mode, the fee's and the adena's labels right-aligned at 156, their
// values at 247, thousands separated
INT NCStoreWnd::OnPaint(NCGDevice* Device)
{
    TCHAR Text[128];
    TCHAR FeeText[128];
    TCHAR AdenaText[128];
    SIZE Size;
    guard(NCStoreWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
    {
        INT H = (INT)(Height - 20.0f);
        INT W = (INT)Width;
        Device->DrawTexture(0, 20, W, H, 0, 0, W, H, BackTex, 0xFF, 1);
    }
    if (Mode == STOREMODE_Deposit)
    {
        Device->DrawText(11, 32, 0xFFDCDCDC, *SysString(STR_Inventory), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        Device->DrawText(11, 198, 0xFFDCDCDC, *SysString(STR_StoredItems), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }
    else if (Mode == STOREMODE_Withdraw)
    {
        Device->DrawText(11, 32, 0xFFDCDCDC, *SysString(STR_StoredItems), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        Device->DrawText(11, 198, 0xFFDCDCDC, *SysString(STR_ItemsToRetrieve), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0,
                         0);
    }
    NCGDevice::GetTextExtent(&Size, *SysString(STR_Fee), 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(156 - Size.cx, 331, 0xFFDCDCDC, *SysString(STR_Fee), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *SysString(STR_Adena), 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(156 - Size.cx, 350, 0xFFDCDCDC, *SysString(STR_Adena), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("%d"), Fee);
    FormatNumber(FeeText, Text);
    appSprintf(Text, TEXT("%s"), FeeText);
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
NW_HOOK_METHOD(0x10103ce0, NCStoreWnd, OnPaint)

// the ids and amounts of a grid's entries that hold something: count, then (id, amount) each
// (inline twice in the stock OnOK)
static FORCEINLINE void PushItemList(L2ParamStack& Params, NCStoreItemWnd* Grid, INT Num)
{
    Params.PushBack((void*)(INT_PTR)Num);
    for (INT i = 0; i < Grid->Items.Num(); i++)
    {
        if (Grid->Items(i)->TradeCount > 0)
        {
            Params.PushBack((void*)(INT_PTR)Grid->Items(i)->UnknownC34);
            Params.PushBack((void*)(INT_PTR)Grid->Items(i)->TradeCount);
        }
    }
}

// the entries of a grid that hold something
static FORCEINLINE INT CountFilled(NCStoreItemWnd* Grid)
{
    INT Num = 0;
    for (INT i = 0; i < Grid->Items.Num(); i++)
        if (Grid->Items(i)->TradeCount > 0)
            Num++;
    return Num;
}

// 0x10104260 (vslot 68) - the lower grid's list to the server: depositing, what is in the warehouse's grid (count, then
// the item id and amount of each); withdrawing (the mode is set to 1 on the way), the inventory's grid; the window closes
INT NCStoreWnd::OnOK()
{
    if (Mode == STOREMODE_Deposit)
    {
        INT Num = CountFilled(StoredItems);
        L2ParamStack Params(Num * 3 + 2);
        PushItemList(Params, StoredItems, Num);
        GConsole->RequestWarehouseDeposit(Params);
    }
    else
    {
        Mode = STOREMODE_Withdraw;
        INT Num = CountFilled(InventoryItems);
        L2ParamStack Params(Num * 3 + 2);
        PushItemList(Params, InventoryItems, Num);
        GConsole->RequestWarehouseWithdraw(Params);
    }
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10104260, NCStoreWnd, OnOK)

// 0x10103c60 (vslot 69)
INT NCStoreWnd::OnCancel()
{
    Clear();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10103c60, NCStoreWnd, OnCancel)

// 0x10103cb0 (vslot 72) - OnCancel's work and the window's closing sound
INT NCStoreWnd::OnClose()
{
    Clear();
    HideWindow();
    GConsole->PlayInterfaceSound(SOUND_Close);
    return 1;
}
NW_HOOK_METHOD(0x10103cb0, NCStoreWnd, OnClose)

// the arrows: the selected item of Grid goes to its target, how many first for a stack of more than one
// (inline in the stock OnUp / OnDown, twice each)
static FORCEINLINE void MoveSelected(NCStoreItemWnd* Grid)
{
    INT Sel = Grid->GetSelected();
    if (Sel < 0 || Sel >= Grid->Items.Num())
        return;
    NItemEntry* Item = Grid->Items(Sel);
    if (!Item)
        return;
    if (Item->IsStackable() && Item->TradeCount > 1)
        AskAmount(Grid, Item, (INT)Item->TradeCount);
    else
        Grid->MoveItem(Item, 1);
}

// 0x10104920 (vslot 70) - the up arrow: from the lower grid (withdrawing, the inventory's; depositing, the warehouse's)
INT NCStoreWnd::OnUp()
{
    if (Mode == STOREMODE_Withdraw)
        MoveSelected(InventoryItems);
    else if (Mode == STOREMODE_Deposit)
        MoveSelected(StoredItems);
    return 1;
}
NW_HOOK_METHOD(0x10104920, NCStoreWnd, OnUp)

// 0x10104bd0 (vslot 71) - the down arrow: from the upper grid
INT NCStoreWnd::OnDown()
{
    if (Mode == STOREMODE_Withdraw)
        MoveSelected(StoredItems);
    else if (Mode == STOREMODE_Deposit)
        MoveSelected(InventoryItems);
    return 1;
}
NW_HOOK_METHOD(0x10104bd0, NCStoreWnd, OnDown)

// 0x10103bf0
void NCStoreWnd::SetMode(INT InMode)
{
    Mode = InMode;
    StoredItems->SetMode(Mode);
    InventoryItems->SetMode(Mode);
}
NW_HOOK_METHOD(0x10103bf0, NCStoreWnd, SetMode)

// 0x10116b20 - one body with NCShopWnd::SetListID's (the shop keeps its list id at +0xF4 too)
void NCStoreWnd::SetAdena(INT InAdena)
{
    Adena = InAdena;
}
NW_HOOK_METHOD(0x10116b20, NCStoreWnd, SetAdena)

// 0x10103c20 - kept out of line like the stock code (HideWindow, OnCancel)
__declspec(noinline) void NCStoreWnd::Clear()
{
    Adena = 0;
    Fee = 0;
    InventoryItems->Clear();
    StoredItems->Clear();
}
NW_HOOK_METHOD(0x10103c20, NCStoreWnd, Clear)

// 0x10103c80 - 30 adena for each slot deposited; withdrawing is free
void NCStoreWnd::UpdateFee()
{
    Fee = 0;
    if (Mode == STOREMODE_Withdraw)
        Fee = 0;
    else if (Mode == STOREMODE_Deposit)
        Fee = StoredItems->Items.Num() * STORE_FEE_PER_SLOT;
}
NW_HOOK_METHOD(0x10103c80, NCStoreWnd, UpdateFee)

// stock body shared with NCShopWnd::GetMode (0x101189a0, hooked there)
INT NCStoreWnd::GetMode()
{
    return Mode;
}

// 0x101040c0 - withdrawing, the warehouse's grid on top (4 rows) and the inventory's below (3 rows); depositing, the
// inventory's on top and the warehouse's below
void NCStoreWnd::UpdateLayout()
{
    NRectF Origin;
    if (Mode == STOREMODE_Withdraw)
    {
        StoredItems->UpdateScrollRange();
        InventoryItems->UpdateScrollRange();
        // the stock code fills { LocalX, LocalY, LocalW, LocalH }; SetScreenPos reads x and y
        Origin.X = 0;
        Origin.Y = 0;
        *(INT*)&Origin.W = 0;
        *(INT*)&Origin.H = 0;
        GetLocalRect(&Origin);
        StoredItems->SetScreenPos(9, 48, (const POINT*)&Origin);
        InventoryItems->SetScreenPos(9, 215, (const POINT*)&Origin);
        ::nw::VirtualCall<void>(StoredItems, 68, 239, 139);
        ::nw::VirtualCall<void>(InventoryItems, 68, 239, 104);
        InventoryItems->SetVisibleRows(3);
        StoredItems->SetVisibleRows(4);
    }
    else if (Mode == STOREMODE_Deposit)
    {
        StoredItems->UpdateScrollRange();
        InventoryItems->UpdateScrollRange();
        Origin.X = 0;
        Origin.Y = 0;
        *(INT*)&Origin.W = 0;
        *(INT*)&Origin.H = 0;
        GetLocalRect(&Origin);
        StoredItems->SetScreenPos(9, 215, (const POINT*)&Origin);
        InventoryItems->SetScreenPos(9, 48, (const POINT*)&Origin);
        ::nw::VirtualCall<void>(StoredItems, 68, 239, 104);
        ::nw::VirtualCall<void>(InventoryItems, 68, 239, 139);
        InventoryItems->SetVisibleRows(4);
        StoredItems->SetVisibleRows(3);
    }
}
NW_HOOK_METHOD(0x101040c0, NCStoreWnd, UpdateLayout)

// one grid's entries get their name and description again from the item tables, and their
// tool tips (the stock code repeats this for each grid)
static FORCEINLINE void ReloadItemTexts(NCStoreItemWnd* Grid)
{
    for (INT i = 0; i < Grid->Items.Num(); i++)
    {
        NItemEntry* Item = Grid->Items(i);
        if (Item)
        {
            appStrcpy(Item->Name, GL2GameData.GetItemName(Item->ID));
            appStrcpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
            Item->FillToolTip(2, 0);
        }
    }
}

// 0x101044b0 - after the item texts were reloaded (NConsoleWnd::ReloadGameTexts)
void NCStoreWnd::ReloadTexts()
{
    ReloadItemTexts(StoredItems);
    ReloadItemTexts(InventoryItems);
}
NW_HOOK_METHOD(0x101044b0, NCStoreWnd, ReloadTexts)

// 0x10104900
UBOOL NCStoreWnd::AddInventoryItem(NItemInfo* Info)
{
    return InventoryItems->AddItem(Info);
}
NW_HOOK_METHOD(0x10104900, NCStoreWnd, AddInventoryItem)

// 0x10104910
UBOOL NCStoreWnd::AddStoredItem(NItemInfo* Info)
{
    return StoredItems->AddItem(Info);
}
NW_HOOK_METHOD(0x10104910, NCStoreWnd, AddStoredItem)

NW_CHECK_VTABLE(NCStoreWnd, 0x10188010, 73)

/*-----------------------------------------------------------------------------
    NCGMStoreWnd.
-----------------------------------------------------------------------------*/

// 0x10085060
NCGMStoreWnd::NCGMStoreWnd()
{
}
NW_HOOK_CTOR(0x10085060, NCGMStoreWnd)

// 0x100850b0
NW_HOOK_DTOR(0x100850b0, NCGMStoreWnd)


// 0x1007e330 (vslot 25) - NCStoreWnd's, as a plain 0 / 1
INT NCGMStoreWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCStoreWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1007e330, NCGMStoreWnd, WindowProc)

// 0x1007e350 (vslot 37) - the warehouse window with its buttons disabled: a GM only looks
INT NCGMStoreWnd::OnCreate()
{
    NCStoreWnd::OnCreate();
    UpButton->EnableWindow(0);
    DownButton->EnableWindow(0);
    OKButton->EnableWindow(0);
    CancelButton->EnableWindow(0);
    return 1;
}
NW_HOOK_METHOD(0x1007e350, NCGMStoreWnd, OnCreate)

// 0x10081970 (vslot 56) - the clip and the origin, then the warehouse window's own painting
INT NCGMStoreWnd::OnPaint(NCGDevice* Device)
{
    guard(NCGMStoreWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    NCStoreWnd::OnPaint(Device);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10081970, NCGMStoreWnd, OnPaint)

NW_CHECK_VTABLE(NCGMStoreWnd, 0x10188d50, 73)
