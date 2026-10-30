/*=============================================================================
    NCDeliverWnd.cpp - the package windows: the choice of the character and the package
    window with its item grids.
    C3 stock: 0x10079fe0 .. 0x1007bdb0 (the grids' 0x1007b330 .. 0x1007b760, MoveItem 0x1007ac30; AddItem 0x1007aa40,
    AddCopy 0x1007ab50 and AddInventoryItem 0x1007ad80 are called by address). WindowProc (0x100a2850), OnDestroy
    (0x100d7090) and SetTarget (0x10088870) of the grid, WindowProc (0x100f0070) of the package window and OnCancel
    (0x1007bee0) of the choice are copies of bodies other classes keep the addresses of.
=============================================================================*/
#include "nwindow/NCDeliverWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NClasses.gen.h"

enum { DIALOG_DELIVER_AMOUNT = 3 };  // NDialogInfo::ID (name ours): an amount for the owner
enum { MSG_HowManyToMove = 72 };     // systemmsg "How many $s1(s) do you want to move?"
enum { MSG_ChooseReceiver = 664 };   // systemmsg "Please choose a person to receive."
enum { ITEM_Adena = 57 };
enum { SOUND_WindowClose = 6 };      // interface sound (name ours)
enum { DELIVER_FEE_PER_SLOT = 1000 };  // adena (name ours)
enum { MAX_DELIVER_TARGETS = 8 };

// sysstring labels
enum
{
    STR_Adena = 134,
    STR_Inventory = 138,
    STR_OK = 140,
    STR_Cancel = 141,
    STR_SendingList = 560,
    STR_SendingFee = 561,
};

// the package window
static NCDeliverWnd* DeliverWnd()
{
    return GDesktop->DeliverWnd;
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

// how many of a stack to move: the box (the whole stack at most, Unknown74) answers to Owner's OnDialogResult; the item's
// id goes in the field at +0x18 (C3: not the adena flag of C2). Inline in each stock caller; the name buffer is the stock
// frame's 64 characters
static FORCEINLINE void AskAmount(NCWnd* Owner, NItemEntry* Item)
{
    TCHAR Name[64];
    appSprintf(Name, TEXT("%s"), Item->GetName());
    NDialogInfo Info;
    Info.ID = DIALOG_DELIVER_AMOUNT;
    Info.Type = 3;
    Info.Owner = Owner->Entry;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.SetSystemMessage(MSG_HowManyToMove);
    Info.SetButtons(Name, NULL);
    Info.Unknown74 = Item->Count;
    ::nw::Field<INT>(&Info, 0x18) = Item->ID;
    GConsole->ShowDialog(&Info);
}

/*-----------------------------------------------------------------------------
    NCDeliverItemWnd.
-----------------------------------------------------------------------------*/

// 0x1007b6c0 - the grid's own partner is none
NCDeliverItemWnd::NCDeliverItemWnd()
{
    Target = NULL;
}
NW_HOOK_CTOR(0x1007b6c0, NCDeliverItemWnd)

// 0x1007b760 - the grids of the package window: columns, rows, most items, cell size and the gaps
NCDeliverItemWnd::NCDeliverItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX,
                                   INT InGapY)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
    Target = NULL;
}
NW_HOOK_CTOR_ARGS(0x1007b760, NCDeliverItemWnd, (INT A, INT B, INT C, INT D, INT E, INT F), (A, B, C, D, E, F))

// 0x1007b710 - the base's (ConditionalDestroy, then NCItemWnd's)
NW_HOOK_DTOR(0x1007b710, NCDeliverItemWnd)


// stock body shared with NCShopItemWnd::WindowProc (0x100c0e40), which keeps the address: the double click and the
// dialog box's answer
INT NCDeliverItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    if (Msg == NWM_DIALOG_RESULT)
        return OnDialogResult(wParam, lParam) != 0;
    return 0;
}

// stock body shared with NCShopItemWnd::OnDestroy (0x100c46f0), which keeps the address
INT NCDeliverItemWnd::OnDestroy()
{
    NCItemWnd::OnDestroy();
    return 1;
}

// stock body shared with NCStoreItemWnd::SetTarget (0x100d8aa0), which keeps the address
void NCDeliverItemWnd::SetTarget(NCDeliverItemWnd* InTarget)
{
    Target = InTarget;
}


// 0x1007b330 (vslot 63) - an item let go over the other grid (the one under the mouse becomes the target) is moved: one
// at a time, or asked how many for a stack of 2 or more
INT NCDeliverItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (Selected > -1 && Selected < Items.Num() && bDragging)
    {
        INT X = LOWORD(lParam);
        INT Y = HIWORD(lParam);
        if (!IsInWindow(X, Y))
        {
            NWndEntry* Found = FindWindowAt(X, Y);
            if (Found && Found->Wnd && Found->Wnd->IsA(NCDeliverItemWnd::StaticClass()))
            {
                Target = (NCDeliverItemWnd*)Found->Wnd;
                NItemEntry* Item = Items(Selected);
                if (!Item->IsStackable() || Item->TradeCount < 2)
                    MoveItem(Item, 1);
                else
                    AskAmount(this, Item);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1007b330, NCDeliverItemWnd, OnLButtonUp)

// 0x1007b500 (vslot 75) - a double click moves the item under the mouse the same way (to the target it had)
INT NCDeliverItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    if (Selected > -1 && Selected < Items.Num())
    {
        NItemEntry* Item = Items(Selected);
        if (!Item->IsStackable() || Item->TradeCount < 2)
            MoveItem(Item, 1);
        else
            AskAmount(this, Item);
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1007b500, NCDeliverItemWnd, OnLButtonDblClick)

// 0x1007b670 (vslot 76) - the amount typed (the stack at most) of the selected item is moved
INT NCDeliverItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    if (Item && wParam != 0)
    {
        if (Item->TradeCount <= (SQWORD)(INT)wParam)
            wParam = (WPARAM)(DWORD)Item->TradeCount;
        MoveItem(Item, (INT)wParam);
    }
    return 1;
}
NW_HOOK_METHOD(0x1007b670, NCDeliverItemWnd, OnDialogResult)

// 0x1007aa40  NOT PORTED (the console lane's: called by address; ret 4)
#if NW_HOOK_MODE
NW_NAKED UBOOL NCDeliverItemWnd::AddItem(NItemInfo* Info) NW_STOCK(0x1007aa40)
#else
NW_SLOT_IS("?AddItem@NCDeliverItemWnd@@QAEHPAUNItemInfo@@@Z", "?AddItem@NCDeliverItemWndExt@@QAEHPAUNItemInfo@@@Z")   // NCDeliverItemWnd::AddItem = NCDeliverItemWndExt::AddItem (0x1007aa40)
#endif

// 0x1007ab50 (ret 8; called by address) - a copy of Src with Count as its amount (0 when the grid is full): the entry
// made by the stock constructor, its tool tip filled for NTT_Item, the scroll range updated
UBOOL NCDeliverItemWnd::AddCopy(NItemEntry* Src, INT Count)
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
NW_HOOK_METHOD(0x1007ab50, NCDeliverItemWnd, AddCopy)

// 0x1007ac30 - Count of Item goes to the target grid (to the entry of the same item when it is a stack, else a copy), and
// leaves this one (the entry when it was the last, else its amount less Count); the fee follows
void NCDeliverItemWnd::MoveItem(NItemEntry* Item, INT Count)
{
    if (!Target)
        return;
    UnselectAll();
    UBOOL bStacked = 0;
    if (Item->IsStackable())
    {
        for (INT i = 0; i < Target->Items.Num(); i++)
        {
            if (Target->Items(i)->ID == Item->ID)
            {
                Target->Items(i)->TradeCount += Count;
                Target->Items(i)->FillToolTip(2, 0);
                bStacked = 1;
                break;
            }
        }
    }
    if (!bStacked)
        Target->AddCopy(Item, Count);
    if (Item->IsStackable() && Item->TradeCount > (SQWORD)Count)
    {
        Item->TradeCount -= Count;
        Item->FillToolTip(2, 0);
    }
    else
        Items.Remove(Selected, 1);
    DeliverWnd()->UpdateFee();
    Selected = -1;
    UpdateScrollRange();
}
NW_HOOK_METHOD(0x1007ac30, NCDeliverItemWnd, MoveItem)

NW_CHECK_VTABLE(NCDeliverItemWnd, 0x10186b20, 77)

/*-----------------------------------------------------------------------------
    NCDeliverWnd.
-----------------------------------------------------------------------------*/

// 0x1007b7a0
NCDeliverWnd::NCDeliverWnd()
{
    InventoryItems = NULL;
    SendItems = NULL;
    BackTex = NULL;
    UpButton = NULL;
    DownButton = NULL;
    OKButton = NULL;
    CancelButton = NULL;
    UnknownB0 = NULL;
    Adena = 0;
    Fee = 0;
    TargetID = -1;
}
NW_HOOK_CTOR(0x1007b7a0, NCDeliverWnd)

// 0x1007b830
NW_HOOK_DTOR(0x1007b830, NCDeliverWnd)

// stock body shared with NCStoreWnd::WindowProc (0x1006ed90), which keeps the address: the buttons (nothing sends
// NID_STORE_CLOSE)
INT NCDeliverWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
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

// 0x1007b880 (vslot 37) - the base's, the back, the title bar as wide as the window, the two grids (6 columns of 32 pixel
// cells, 3 rows below and 4 above, 5 / 3 apart; each the other's target), the arrows, OK and Cancel
INT NCDeliverWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.StoreWnd.Store1_back"));

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameID = NID_STORE_FRAME;
    FrameCtrl->ID = NID_STORE_FRAME;
    FrameCtrl->SetPosition(0, 0, (INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    SendItems = new NCDeliverItemWnd(6, 3, 160, 32, 5, 3);
    SendItems->SetPosition(9, 215, 239, 104);
    CreateChild(SendItems, NWS_VISIBLE | NWS_UNKNOWN20);

    InventoryItems = new NCDeliverItemWnd(6, 4, 160, 32, 5, 3);
    InventoryItems->SetPosition(9, 48, 239, 139);
    CreateChild(InventoryItems, NWS_VISIBLE | NWS_UNKNOWN20);

    SendItems->SetTarget(InventoryItems);
    InventoryItems->SetTarget(SendItems);

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
NW_HOOK_METHOD(0x1007b880, NCDeliverWnd, OnCreate)

// 0x1007a630 (vslot 38) - the grids, the arrows and the buttons go (and the window of +0xE4, never created), then the base
INT NCDeliverWnd::OnDestroy()
{
    DestroyChild(this, InventoryItems);
    DestroyChild(this, SendItems);
    DestroyChild(this, UpButton);
    DestroyChild(this, DownButton);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    DestroyChild(this, UnknownB0);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1007a630, NCDeliverWnd, OnDestroy)

// 0x10079fe0 (vslot 44) - opens emptied (the console fills it after)
void NCDeliverWnd::ShowWindow()
{
    Clear();
    NCWnd::ShowWindow();
}
NW_HOOK_METHOD(0x10079fe0, NCDeliverWnd, ShowWindow)

// 0x10079ff0 (vslot 45)
void NCDeliverWnd::HideWindow()
{
    Clear();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x10079ff0, NCDeliverWnd, HideWindow)

// 0x1007a2a0 (vslot 56) - the back below the title bar, the headings, and the fee and the adena right-aligned at x = 156
// (the labels) and x = 247 (the numbers, with their separators)
INT NCDeliverWnd::OnPaint(NCGDevice* Device)
{
    guard(NCDeliverWnd::OnPaint);
    TCHAR Text[132];
    TCHAR FeeText[128];
    TCHAR AdenaText[128];
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
    {
        INT H = (INT)(Height - 20.0f);
        INT W = (INT)Width;
        Device->DrawTexture(0, 20, W, H, 0, 0, W, H, BackTex, 0xFF, 1);
    }
    Device->DrawText(11, 32, 0xFFDCDCDC, *SysString(STR_Inventory), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(11, 198, 0xFFDCDCDC, *SysString(STR_SendingList), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *SysString(STR_SendingFee), 0, 0, 0, 0, 0);
    Device->DrawText(156 - Size.cx, 331, 0xFFDCDCDC, *SysString(STR_SendingFee), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *SysString(STR_Adena), 0, 0, 0, 0, 0);
    Device->DrawText(156 - Size.cx, 350, 0xFFDCDCDC, *SysString(STR_Adena), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("%d"), Fee);
    FormatNumber(FeeText, Text);
    appSprintf(Text, TEXT("%s"), FeeText);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Device->DrawText(247 - Size.cx, 331, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("%d"), Adena);
    FormatNumber(AdenaText, Text);
    appSprintf(Text, TEXT("%s"), AdenaText);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Device->DrawText(247 - Size.cx, 350, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007a2a0, NCDeliverWnd, OnPaint)

// 0x1007a780 (vslot 68) - the list sent to the server: the character, how many entries have an amount, and for each its
// first field and its amount; then the window closes
INT NCDeliverWnd::OnOK()
{
    INT Count = 0;
    for (INT i = 0; i < SendItems->Items.Num(); i++)
    {
        NItemEntry* Item = SendItems->Items(i);
        if (Item->TradeCount > 0)
            Count++;
    }
    L2ParamStack Params(Count * 2 + 2);
    Params.PushBack((void*)(INT_PTR)TargetID);
    Params.PushBack((void*)(INT_PTR)Count);
    for (INT i = 0; i < SendItems->Items.Num(); i++)
    {
        NItemEntry* Item = SendItems->Items(i);
        if (Item->TradeCount > 0)
        {
            Params.PushBack((void*)(INT_PTR)Item->UnknownC34);
            Params.PushBack((void*)(INT_PTR)(INT)SendItems->Items(i)->TradeCount);
        }
    }
    GConsole->RequestPackageSend(Params);
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1007a780, NCDeliverWnd, OnOK)

// 0x1007a000 (vslot 69)
INT NCDeliverWnd::OnCancel()
{
    Clear();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1007a000, NCDeliverWnd, OnCancel)

// 0x1007a050 (vslot 72) - the close of the window: emptied, hidden, with the window close sound
INT NCDeliverWnd::OnClose()
{
    Clear();
    HideWindow();
    GConsole->PlayInterfaceSound(SOUND_WindowClose);
    return 1;
}
NW_HOOK_METHOD(0x1007a050, NCDeliverWnd, OnClose)

// 0x1007ad90 (vslot 70) - the up arrow: the selected item of the lower grid goes up, one at a time or asked for a stack
INT NCDeliverWnd::OnUp()
{
    INT Sel = SendItems->GetSelected();
    if (Sel > -1 && Sel < SendItems->Items.Num())
    {
        NItemEntry* Item = SendItems->Items(Sel);
        if (Item)
        {
            if (!Item->IsStackable() || Item->TradeCount < 2)
                SendItems->MoveItem(Item, 1);
            else
                AskAmount(SendItems, Item);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x1007ad90, NCDeliverWnd, OnUp)

// 0x1007af00 (vslot 71) - the down arrow: the same for the upper grid
INT NCDeliverWnd::OnDown()
{
    INT Sel = InventoryItems->GetSelected();
    if (Sel > -1 && Sel < InventoryItems->Items.Num())
    {
        NItemEntry* Item = InventoryItems->Items(Sel);
        if (Item)
        {
            if (!Item->IsStackable() || Item->TradeCount < 2)
                InventoryItems->MoveItem(Item, 1);
            else
                AskAmount(InventoryItems, Item);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x1007af00, NCDeliverWnd, OnDown)

// 0x100deca0 - the player's adena (the console sets it before the list); the stock body is one with
// NCRecipeBuyListWnd::SetAdena's (hooked there, which keeps the address)
void NCDeliverWnd::SetAdena(INT InAdena)
{
    Adena = InAdena;
}

// 0x10079f90 - the target of the delivery (+0xF0)
void NCDeliverWnd::SetTargetID(INT InTargetID)
{
    TargetID = InTargetID;
}
NW_HOOK_METHOD(0x10079f90, NCDeliverWnd, SetTargetID)


// 0x10079fa0 - kept out of line like the stock code (ShowWindow, HideWindow, OnCancel)
__declspec(noinline) void NCDeliverWnd::Clear()
{
    TargetID = -1;
    Adena = 0;
    Fee = 0;
    InventoryItems->Clear();
    SendItems->Clear();
}
NW_HOOK_METHOD(0x10079fa0, NCDeliverWnd, Clear)

// 0x1007a020 - 1000 adena for each slot sent
void NCDeliverWnd::UpdateFee()
{
    Fee = 0;
    Fee = SendItems->Items.Num() * DELIVER_FEE_PER_SLOT;
}
NW_HOOK_METHOD(0x1007a020, NCDeliverWnd, UpdateFee)

// 0x1007ad80  NOT PORTED (the console lane's: called by address; ret 4)
#if NW_HOOK_MODE
NW_NAKED UBOOL NCDeliverWnd::AddInventoryItem(NItemInfo* Info) NW_STOCK(0x1007ad80)
#else
NW_SLOT_IS("?AddInventoryItem@NCDeliverWnd@@QAEHPAUNItemInfo@@@Z", "?AddInventoryItem@NCDeliverWndExt@@QAEHPAUNItemInfo@@@Z")   // NCDeliverWnd::AddInventoryItem = NCDeliverWndExt::AddInventoryItem (0x1007ad80)
#endif

NW_CHECK_VTABLE(NCDeliverWnd, 0x10186c58, 73)

/*-----------------------------------------------------------------------------
    NCSelectDeliverWnd.
-----------------------------------------------------------------------------*/

// 0x1007bd50 - the targets are left as allocated
NCSelectDeliverWnd::NCSelectDeliverWnd()
{
    NumTargets = 0;
    Selected = -1;
}
NW_HOOK_CTOR(0x1007bd50, NCSelectDeliverWnd)

// 0x1007bdb0
NW_HOOK_DTOR(0x1007bdb0, NCSelectDeliverWnd)

// 0x1007a080 (vslot 25) - the buttons and the combo box
INT NCSelectDeliverWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_SELDELIVER_OK)
            return OnOK() != 0;
        if (wParam == NID_SELDELIVER_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_SELDELIVER_TARGETS)
            return OnSelect(lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1007a080, NCSelectDeliverWnd, WindowProc)

// 0x1007b070 (vslot 37) - the base's, the title bar as wide as the window, the combo box of the characters, OK and
// Cancel, the back
INT NCSelectDeliverWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_SELDELIVER_FRAME;
    FrameCtrl->SetPosition(0, 0, (INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    TargetCombo = new NCComboBox;
    TargetCombo->ID = NID_SELDELIVER_TARGETS;
    TargetCombo->SetPosition(39, 61, 178, 17);
    CreateChild(TargetCombo, NWS_VISIBLE);

    OKButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    OKButton->ID = NID_SELDELIVER_OK;
    OKButton->SetPosition(51, 172, 76, 23);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(STR_OK);

    CancelButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    CancelButton->ID = NID_SELDELIVER_CANCEL;
    CancelButton->SetPosition(131, 172, 76, 23);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(STR_Cancel);

    BackTex = FindTexture(TEXT("L2UI_ch3.DeliverWnd.Warehouse_back"));
    return 1;
}
NW_HOOK_METHOD(0x1007b070, NCSelectDeliverWnd, OnCreate)

// 0x1007a9a0 (vslot 38) - the combo box and the buttons go, then the base
INT NCSelectDeliverWnd::OnDestroy()
{
    DestroyChild(this, TargetCombo);
    DestroyChild(this, CancelButton);
    DestroyChild(this, OKButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1007a9a0, NCSelectDeliverWnd, OnDestroy)

// 0x1007a1c0 (vslot 44) - opens emptied (the console adds the characters after)
void NCSelectDeliverWnd::ShowWindow()
{
    Clear();
    NCWnd::ShowWindow();
}
NW_HOOK_METHOD(0x1007a1c0, NCSelectDeliverWnd, ShowWindow)

// 0x1007a1d0 (vslot 45)
void NCSelectDeliverWnd::HideWindow()
{
    Clear();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x1007a1d0, NCSelectDeliverWnd, HideWindow)

// 0x1007a8d0 (vslot 56) - the back stretched below the title bar and the heading ("Please choose a person to receive."); no
// guard frame
INT NCSelectDeliverWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
    {
        INT H = (INT)(Height - 20.0f);
        INT W = (INT)Width;
        Device->DrawStretchedTex(0, 20, W, H, 0, 0, W, H, BackTex, 0xFF);
    }
    Device->DrawText(11, 32, 0xFFDCDCDC, *GL2GameData.SystemMsg[MSG_ChooseReceiver].Message, 0, 0, 0, L2FONT_Normal, 0, 0,
                     0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x1007a8d0, NCSelectDeliverWnd, OnPaint)

// 0x1007a280 (vslot 68) - the combo box's choice
INT NCSelectDeliverWnd::OnSelect(LPARAM Index)
{
    Selected = (INT)Index;
    return 1;
}
NW_HOOK_METHOD(0x1007a280, NCSelectDeliverWnd, OnSelect)

// 0x1007a1e0 (vslot 69) - with a character chosen, the server is asked what can be sent to it and the window closes
INT NCSelectDeliverWnd::OnOK()
{
    if (Selected >= 0)
    {
        L2ParamStack Params(1);
        Params.PushBack((void*)(INT_PTR)Targets[Selected].ID);
        GConsole->RequestPackageSendableItemList(Params);
        HideWindow();
    }
    return 1;
}
NW_HOOK_METHOD(0x1007a1e0, NCSelectDeliverWnd, OnOK)

// stock body shared with NCOptionWnd's (0x10059d00)
INT NCSelectDeliverWnd::OnCancel()
{
    HideWindow();
    return 1;
}

// 0x1007a120 - 8 at most; the rest are dropped (the name buffer holds 24 characters: a longer one overruns it)
void NCSelectDeliverWnd::AddTarget(INT ID, const TCHAR* Name)
{
    if (NumTargets >= MAX_DELIVER_TARGETS)
        return;
    Targets[NumTargets].ID = ID;
    appStrcpy(Targets[NumTargets].Name, Name);
    TargetCombo->AddItem(Targets[NumTargets].Name);
    NumTargets++;
}
NW_HOOK_METHOD(0x1007a120, NCSelectDeliverWnd, AddTarget)

// 0x1007a190 - kept out of line like the stock code (ShowWindow, HideWindow)
__declspec(noinline) void NCSelectDeliverWnd::Clear()
{
    TargetCombo->ClearItems();
    Selected = -1;
    NumTargets = 0;
}
NW_HOOK_METHOD(0x1007a190, NCSelectDeliverWnd, Clear)

NW_CHECK_VTABLE(NCSelectDeliverWnd, 0x10186e98, 71)
