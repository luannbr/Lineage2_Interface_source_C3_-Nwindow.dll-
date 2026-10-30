/*=============================================================================
    NCPetInventoryWnd.cpp - the pet window's Items page and its item grid.
    Stock: 0x1008eea0 .. 0x1008ef50, 0x1008ef60 .. 0x10090b4c, 0x10091830 .. 0x10091d06,
    0x10091d10 .. 0x10091f6f, 0x10091f70 .. 0x10091fa0, 0x10093a10 .. 0x10093e45,
    0x10093e50 .. 0x100940a6, 0x100941c0 .. 0x1009436d (ClearItems: a copy of the body
    NCTrainListWnd keeps the address of, 0x100bab50).
=============================================================================*/
#include "nwindow/NCPetInventoryWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_ItemPick = 3 };

enum { DIALOG_PET_ITEM_TO_OWNER = 0x37 };  // NDialogInfo::ID (name ours; C1: 0x33)
enum { MSG_PetItemToOwnerHowMany = 536 };  // systemmsg 0x218: how many to give back (C3 asks the same for adena)
enum { ITEM_Adena = 57 };

// a system line in the desktop's chat window (NCConsole::AddSystemText): Text when not empty
static FORCEINLINE void DesktopChatMessage(const TCHAR* Text, INT P2, INT P3)
{
    GDesktop->AddSystemText(Text, P2, P3);
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

/*-----------------------------------------------------------------------------
    NCPetInvenItemWnd.
-----------------------------------------------------------------------------*/

// 0x100cf3b0 - the item grid's defaults, nothing pending
NCPetInvenItemWnd::NCPetInvenItemWnd()
{
    bDropPending = 0;
    bToOwnerPending = 0;
    bDblClicked = 0;
}
NW_HOOK_CTOR(0x100cf3b0, NCPetInvenItemWnd)

// 0x100cf460 (ret 0x18) - the grid of the six arguments (columns, rows, most items, icon size, gaps)
NCPetInvenItemWnd::NCPetInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX,
                                     INT InGapY)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
    bDropPending = 0;
    bToOwnerPending = 0;
    bDblClicked = 0;
}
NW_HOOK_CTOR_ARGS(0x100cf460, NCPetInvenItemWnd, (INT A, INT B, INT C, INT D, INT E, INT F), (A, B, C, D, E, F))

// 0x100cf410
NW_HOOK_DTOR(0x100cf410, NCPetInvenItemWnd)


// stock body shared with NCInvenItemWnd::WindowProc (0x100c4610), which keeps the address
INT NCPetInvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}

// 0x100cbaf0 (vslot 37) - the frame of the equipped items and the two frames the unselected ones are drawn in (only the
// second is used)
INT NCPetInvenItemWnd::OnCreate()
{
    NCItemWnd::OnCreate();
    EquippedTex = FindTexture(TEXT("l2ui_ch3.PetWnd.petitem_click"));
    OutlineDownTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine_down"));
    OutlineTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine"));
    return 1;
}
NW_HOOK_METHOD(0x100cbaf0, NCPetInvenItemWnd, OnCreate)

// 0x100cc7a0 (vslot 56) - the icons from the page's first row, 5 pixels apart, a row of CellSize + 3; behind each one
// the selection frame (when selected) and the outline, over it the equipped mark (32 pixels)
INT NCPetInvenItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPetInvenItemWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
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
                Y = Y + CellSize + 3;
            if ((CellSize + 1) * VisibleRows <= Y)
                break;
        }
        else
            X = X + CellSize + 5;
        if (Item->IsSelected())
            Device->DrawTexture(X - 1, Y - 1, 0x22, 0x22, 0, 0, 0x22, 0x22, SelectTex, 0xFF, 1);
        if (OutlineTex)
            Device->DrawTexture(X - 1, Y - 1, 0x22, 0x22, 0, 0, 0x22, 0x22, OutlineTex, 0xFF, 1);
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon(), 0xFF, 1);
        if (Item->bEquipped)
            Device->DrawTexture(X, Y, 0x20, 0x20, 0, 0, 0x20, 0x20, EquippedTex, 0xFF, 1);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100cc7a0, NCPetInvenItemWnd, OnPaint)

// 0x100cc9c0 (vslot 62) - selects the item under the mouse and picks it up (unless bNoDrag, which lasts one click)
INT NCPetInvenItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
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
            bDragging = 1;
            GConsole->PlayInterfaceSound(SOUND_ItemPick);
            return 1;
        }
        bNoDrag = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x100cc9c0, NCPetInvenItemWnd, OnLButtonDown)

// 0x100cca50 (vslot 75) - a double click makes the pet use the item under the mouse; the release that follows moves
// nothing
INT NCPetInvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    bDragging = 0;
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    Selected = Index;
    if (Index >= 0 && Index < Items.Num())
        GConsole->RequestPetUseItem(Items(Index)->ObjectID);
    bDblClicked = 1;
    UnselectAll();
    Selected = -1;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100cca50, NCPetInvenItemWnd, OnLButtonDblClick)

// 0x100ccae0 (vslot 64) - the right button makes the pet use the item under the mouse
INT NCPetInvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    Selected = Index;
    if (Index >= 0 && Index < Items.Num())
        GConsole->RequestPetUseItem(Items(Index)->ObjectID);
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100ccae0, NCPetInvenItemWnd, OnRButtonDown)

// 0x100ceb00 (vslot 63) - the release of the item carried (unless a double click has just used it): inside the grid it
// moves to the place under the mouse (the last one when that is past the end); dropped on the player's inventory or
// paperdoll it goes back to the owner (a stack asks how many first), on the shortcut bar it gets a shortcut
INT NCPetInvenItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (Selected < 0 || Selected >= Items.Num())
    {
        bDragging = 0;
        return 1;
    }
    if (bDragging && !bDblClicked)
    {
        NItemEntry* Item = Items(Selected);
        if (!Item)
        {
            bDragging = 0;
            return 1;
        }
        if (IsInWindow(X, Y))
        {
            INT Target = HitTest(X, Y, 0, 0);
            INT Current = Selected;
            if (Target != Current && Target >= 0 && Current >= 0 && Current < Items.Num())
            {
                if (Target >= Items.Num())
                {
                    Items.Remove(Current, 1);
                    Items.AddItem(Item);
                    Selected = Items.Num() - 1;
                    bDblClicked = 0;
                    bDragging = 0;
                    return 1;
                }
                Items.Remove(Current, 1);
                Items.Insert(Target, 1);
                Items(Target) = Item;
                bDblClicked = 0;
                bDragging = 0;
                return 1;
            }
        }
        else
        {
            NWndEntry* Found = FindWindowAt(X, Y);
            if (Found && Found->Wnd)
            {
                NCWnd* Target = Found->Wnd;
                if (Target->IsA(NClass::NCInvenItemWnd()) || Target->IsA(NClass::NCEquipItemWnd()))
                {
                    bToOwnerPending = 1;
                    if (Item->IsStackable() && Item->Count > 1)
                    {
                        TCHAR Name[64];
                        appSprintf(Name, TEXT("%s"), Item->GetName());
                        NDialogInfo Info;
                        Info.ID = DIALOG_PET_ITEM_TO_OWNER;
                        Info.Type = 3;
                        Info.Owner = Entry;
                        Info.bMainBox = 1;
                        Info.bInput = 1;
                        Info.SetSystemMessage(MSG_PetItemToOwnerHowMany);
                        Info.SetButtons(Name, NULL);
                        ::nw::Field<INT>(&Info, 0x18) = Item->ID;
                        Info.Unknown74 = Item->Count;
                        Info.SetParams(Item->ObjectID, -1);
                        GConsole->ShowDialog(&Info);
                    }
                    else
                    {
                        L2ParamStack Params(10);
                        Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
                        Params.PushBack((void*)1);
                        Params.PushBack((void*)0);
                        GConsole->RequestGetItemFromPet(Params);
                    }
                }
                else if (Target->IsA(NClass::NCShortCutWnd()))
                {
                    INT Slot = ((NCShortCutWnd*)Target)->SlotAtPoint(X, Y);
                    if (Slot >= 0)
                    {
                        L2ParamStack Params(10);
                        Params.PushBack((void*)1);
                        Params.PushBack((void*)(INT_PTR)Slot);
                        Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
                        Params.PushBack((void*)2);
                        GConsole->RequestShortCutReg(Params);
                    }
                    UnselectAll();
                    Selected = -1;
                }
            }
        }
    }
    bDblClicked = 0;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100ceb00, NCPetInvenItemWnd, OnLButtonUp)

// 0x100cf150 (vslot 70) - the entries are deleted
void NCPetInvenItemWnd::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i))
            delete Items(i);
    Items.Empty();
}
NW_HOOK_METHOD(0x100cf150, NCPetInvenItemWnd, Clear)

// 0x100ceef0 (C1 0x10093e50; no C2 match, found by the callers of NCPetWnd's console entry) - the icon comes from
// its name ("NWindow.BlackTexture" when missing) and is written back into Info; the scroll range is not updated
// here. No guard (the stock frame only unwinds the new entry); the stock code tests the address of Info's name for
// NULL first: a compiler artifact, Info is never NULL there
__declspec(noinline) UBOOL NCPetInvenItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
    Info->Icon = FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    // the allocation is inline in the stock code: GMalloc->Malloc(0x1A28, "new")
    NItemEntry* Item = (NItemEntry*)appMalloc(sizeof(NItemEntry), TEXT("new"));
    if (Item)
        Item = new (Item) NItemEntry(Info);
    Item->FillToolTip(2, 0);
    Items.AddItem(Item);
    return 1;
}
NW_HOOK_METHOD(0x100ceef0, NCPetInvenItemWnd, AddItem)

// 0x100ccb40 - the entry with the same object id takes Info's values (through a new entry that is never freed, as in
// the stock code)
UBOOL NCPetInvenItemWnd::UpdateItem(NItemInfo* Info)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Old = Items(i);
        if (Info->ObjectID == Old->ObjectID)
        {
            Info->Icon = FindTexture(Info->IconName);
            if (!Info->Icon)
                Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
            NItemEntry* New = NewItemEntry(Info);
            New->FillToolTip(2, 0);
            Old->Copy(New);
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x100ccb40, NCPetInvenItemWnd, UpdateItem)

// 0x100cf0e0 - one body with NCTradeItemWnd's (which keeps an unhooked copy)
UBOOL NCPetInvenItemWnd::RemoveItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (Items(i)->ObjectID == ObjectID)
        {
            NItemEntry* Item = Items(i);
            Items.Remove(i, 1);
            if (Item)
                delete Item;
            return 1;
        }
    }
    return 0;
}

NW_HOOK_METHOD(0x100cf0e0, NCPetInvenItemWnd, RemoveItem)

// 0x100ccc40 (NCInvenItemWnd has an unhooked copy)
UBOOL NCPetInvenItemWnd::HasItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
        if (ObjectID == Items(i)->ObjectID)
            return 1;
    return 0;
}
NW_HOOK_METHOD(0x100ccc40, NCPetInvenItemWnd, HasItem)

// 0x100ccc90
UBOOL NCPetInvenItemWnd::FindItem(INT ObjectID, INT* Index)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (ObjectID == Items(i)->ObjectID)
        {
            *Index = i;
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x100ccc90, NCPetInvenItemWnd, FindItem)

NW_CHECK_VTABLE(NCPetInvenItemWnd, 0x101965b0, 76)

/*-----------------------------------------------------------------------------
    NCPetInventoryWnd.
-----------------------------------------------------------------------------*/

// 0x100cf4b0 - nothing of its own (the textures and the grid come with OnCreate)
NCPetInventoryWnd::NCPetInventoryWnd()
{
}
NW_HOOK_CTOR(0x100cf4b0, NCPetInventoryWnd)

// 0x100cf500
NW_HOOK_DTOR(0x100cf500, NCPetInventoryWnd)


// stock body shared with NControl's (0x10004e90)
INT NCPetInventoryWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100cf550 (vslot 37) - the grid, 6 x 2 icons of 32 pixels (5 and 3 pixels apart), 160 at most
INT NCPetInventoryWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.PetWnd.Petinterface_back33"));
    Items = new NCPetInvenItemWnd(6, 2, 160, 32, 5, 3);
    Items->SetPosition(0x12, 9, 0xEB, 0x45);
    CreateChild(Items, NWS_VISIBLE | 0x20);
    return 1;
}
NW_HOOK_METHOD(0x100cf550, NCPetInventoryWnd, OnCreate)

// 0x100ccce0 (vslot 38) - the grid goes, then the base
INT NCPetInventoryWnd::OnDestroy()
{
    guard(NCPetInventoryWnd::OnDestroy);
    DestroyChild(this, Items);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100ccce0, NCPetInventoryWnd, OnDestroy)

// stock body shared with NCPetActionWnd's (0x10091680)
INT NCPetInventoryWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}

// 0x100cefe0 (C1 0x10094010; no C2 match) - the grid takes it
UBOOL NCPetInventoryWnd::AddItem(NItemInfo* Info)
{
    Items->AddItem(Info);
    return 1;
}
NW_HOOK_METHOD(0x100cefe0, NCPetInventoryWnd, AddItem)

// 0x100cf1b0 (C1 0x10094030; no C2 match) - the server's change of one item: 1 added, 2 changed (when here), 3
// removed (the stock code calls 0x100dbb90 for that, the body NCTradeItemWnd keeps: ours is the grid's own copy)
UBOOL NCPetInventoryWnd::UpdateItem(INT Op, NItemInfo* Info)
{
    if (Op == 1)
    {
        Items->AddItem(Info);
        return 1;
    }
    if (Op == 2)
    {
        if (Items->HasItem(Info->ObjectID))
            Items->UpdateItem(Info);
        return 1;
    }
    if (Op == 3)
        Items->RemoveItem(Info->ObjectID);
    return 1;
}
NW_HOOK_METHOD(0x100cf1b0, NCPetInventoryWnd, UpdateItem)

// 0x100ccd90 - the pet's item with that object id (the shortcut bar and the console)
NItemEntry* NCPetInventoryWnd::GetItem(INT ObjectID)
{
    INT Index;
    if (Items->FindItem(ObjectID, &Index))
        return Items->Items(Index);
    return NULL;
}
NW_HOOK_METHOD(0x100ccd90, NCPetInventoryWnd, GetItem)

// 0x100b2b90 is NCTrainListWnd::Clear's body, shared (a tail jump through the child pointer at +0xCC, a virtual call of
// the grid's Clear); not hooked here
void NCPetInventoryWnd::ClearItems()
{
    Items->Clear();
}

NW_CHECK_VTABLE(NCPetInventoryWnd, 0x101966e0, 68)
