/*=============================================================================
    NCInvenItemWnd.cpp - a page of the inventory's items. Stock: 0x10062360 .. 0x100641e7,
    0x10065060 .. 0x1006590f, 0x10067690 .. 0x1006819f and 0x10068440 .. 0x100684c9 (the
    paperdoll and the inventory sit in between), plus the destructor (0x1005fe00) and
    WindowProc (0x10057b10).
=============================================================================*/
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NMacroEntry.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NDropDialogInfo.h"
#include "nwindow/NConsoleFns.h"

enum { ITEM_ADENA = 57 };
enum { SOUND_PickUp = 3 };

// NDialogInfo::ID of the questions (names ours)
enum
{
    DIALOG_DROP_ITEM = 4,
    DIALOG_DESTROY_ITEM = 5,
    DIALOG_CRYSTALLIZE_ITEM = 6,
    DIALOG_GIVE_TO_PET = 54,    // 0x36 (C1: 50)
};

// systemmsg
enum
{
    MSG_DropHowMany = 71,       // "How many $s1(s) do you want to discard?"
    MSG_MoveHowMany = 72,       // "How many $s1(s) do you want to move?"
    MSG_DestroyHowMany = 73,    // "How many $s1(s) do you want to destroy?"
    MSG_Destroy = 74,           // "Destroy $s1. Do you want to continue?"
    MSG_CannotDropHere = 179,   // "You cannot discard items here."
    MSG_Crystallize = 336,      // "$s1 is crystallized. Do you want to continue?"
    MSG_AreYouSure = 365,       // "Are you sure?"
    MSG_Drop = 400,             // "Discard $s1. Do you wish to continue?"
    MSG_Place = 651,            // "Place $s1 in the current location and direction..."
};

static FORCEINLINE NCDialogBox* MainDialogBox()
{
    return GDesktop->MainDialogBox;
}

// 0x1008da50 - the item grid's defaults; no scroll mode, no double click pending
NCInvenItemWnd::NCInvenItemWnd()
{
    bEnchantMode = 0;
    bDblClicked = 0;
}
NW_HOOK_CTOR(0x1008da50, NCInvenItemWnd)

// 0x1008da70 (ret 0x18) - the grid of the six arguments (columns, rows, most items, icon size, gaps) and the two frames
// the unselected icons are drawn in
NCInvenItemWnd::NCInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
    bDblClicked = 0;
    OutlineDownTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine_down"));
    OutlineTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine"));
}
NW_HOOK_CTOR_ARGS(0x1008da70, NCInvenItemWnd, (INT A, INT B, INT C, INT D, INT E, INT F), (A, B, C, D, E, F))

// 0x10084070
NW_HOOK_DTOR(0x10084070, NCInvenItemWnd)


// 0x100cba90 (vslot 25) - the stock body is shared with NCPetInvenItemWnd's (the double click is the virtual at
// slot 73 in both); C2 has no dialog result message any more
INT NCInvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100cba90, NCInvenItemWnd, WindowProc)

// 0x100d8aa0 - the stock body is shared with NCStoreItemWnd::SetMode and the other grids' setters of the dword at
// +0x110, which keeps the address
void NCInvenItemWnd::SetEnchantMode(UBOOL bOn)
{
    bEnchantMode = bOn;
}

// The shortcut bars forget a pending selection (C2: the bars' list sits behind the desktop, +0xF8; 0x10056a20
// counts them, 0x10060ce0 gets one, 0x1008b4b0 clears its slots and the flag at +0x9D4).
static void ClearShortCutSelections()
{
    if (GConsole)
    {
        for (INT i = 0; i < GConsole->GetMacroCount(); i++)
        {
            if (GConsole->GetMacro(i) && GConsole->GetMacro(i)->bRunning)
                GConsole->GetMacro(i)->Reset();
        }
    }
}

// the console's test (0x1006b6c0) that an item with the warning flag asks first
static FORCEINLINE UBOOL AsksBeforeUse()
{
    return ((NConsoleLeftFns*)GConsole)->IsDwarf();
}

// Uses the item the way a double click and the right button do: an item with the flag at +0x199C asks first when the
// console says so (dialog 0x21, system message 0x31E with the item's name on the button), one with a message id
// (GetUnknown19AC) shows that message (dialog 0x45), any other is used at once
static FORCEINLINE void UseItem(NCInvenItemWnd* Wnd, NItemEntry* Item)
{
    if (Item && Item->Unknown1998 && AsksBeforeUse())
    {
        TCHAR Name[64];
        appSprintf(Name, TEXT("%s"), Item->GetName());
        NDialogInfo Dialog;
        Dialog.Owner = Wnd->Entry;
        Dialog.ID = 0x21;
        Dialog.bMainBox = 1;
        Dialog.bInput = 0;
        Dialog.SetSystemMessage(0x31E);
        Dialog.SetButtons(Name, NULL);
        Dialog.SetParams(Item->ObjectID, -1);
        GConsole->ShowDialog(&Dialog);
    }
    else if (Item->GetUnknown19AC() > 0)
    {
        NDialogInfo Dialog;
        Dialog.Owner = Wnd->Entry;
        Dialog.ID = 0x45;
        Dialog.bMainBox = 1;
        Dialog.bInput = 0;
        Dialog.SetSystemMessage(Item->GetUnknown19AC());
        Dialog.SetParams(Item->ObjectID, -1);
        GConsole->ShowDialog(&Dialog);
    }
    else
        GConsole->RequestUseItem(Item->ObjectID);
}

// 0x10089430 (vslot 64) - the right button uses the item under the mouse (the shortcut selections of the macros are
// forgotten) and drops what was carried
INT NCInvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam), 1, 1);
    Selected = Index;
    if (Index >= 0 && Index < Items.Num())
    {
        UseItem(this, Items(Index));
        ClearShortCutSelections();
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10089430, NCInvenItemWnd, OnRButtonDown)

// 0x100896a0 (vslot 62) - selects the item under the mouse and picks it up (unless bNoDrag, which lasts one click)
INT NCInvenItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam), 1, 1);
    HoverIndex = Index;
    Selected = Index;
    if (Index >= 0 && Index < Items.Num())
    {
        UnselectAll();
        Items(Selected)->Select();
        if (!bNoDrag)
        {
            bDragging = 1;
            GConsole->PlayInterfaceSound(SOUND_PickUp);
            return 1;
        }
        bNoDrag = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x100896a0, NCInvenItemWnd, OnLButtonDown)

// 0x10089730 (vslot 75) - a double click uses the item under the mouse; the release that follows moves nothing
INT NCInvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    bDragging = 0;
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam), 1, 1);
    Selected = Index;
    if (Index >= 0 && Index < Items.Num())
        UseItem(this, Items(Index));
    bDblClicked = 1;
    UnselectAll();
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10089730, NCInvenItemWnd, OnLButtonDblClick)

// 0x10089950 - the item with Info's object id takes Info's fields and a new tool tip; the entry built for that is
// never freed, as in the stock code
UBOOL NCInvenItemWnd::UpdateItem(NItemInfo* Info)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Found = Items(i);
        if (Info->ObjectID == Found->ObjectID)
        {
            Info->Icon = FindTexture(Info->IconName);
            if (!Info->Icon)
                Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
            NItemEntry* Updated = NewItemEntry(Info);
            Updated->FillToolTip(2, 0);
            Found->Copy(Updated);
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x10089950, NCInvenItemWnd, UpdateItem)

// 0x10089a50
INT NCInvenItemWnd::GetAdena()
{
    INT Adena = 0;
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (Item && Item->ID == ITEM_ADENA)
            Adena += Item->Count;
    }
    return Adena;
}
NW_HOOK_METHOD(0x10089a50, NCInvenItemWnd, GetAdena)

// 0x10089a80 (vslot 56; the guard's name is NCItemWnd's) - the icons from the page's first row, 5 pixels apart, a row
// of CellSize + 3; a selected one in the selection frame, the others in two frames of their own (34 and 35 pixels)
INT NCInvenItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCItemWnd::OnPaint);
    Device->SetClip(ScreenX - 1, ScreenY - 1, (INT)(Width + 2.0f), (INT)(Height + 3.0f));
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 1;
    INT Y = 1;
    INT First = TopRow * Columns;
    for (INT Index = First; Index < Items.Num(); Index++)
    {
        NItemEntry* Item = Items(Index);
        if (!Item)
            continue;
        if (Index % Columns == 0)
        {
            X = 1;
            if (Index != TopRow * Columns)
                Y = Y + CellSize + 3;
            if ((CellSize + 3) * VisibleRows + 1 <= Y)
                break;
        }
        else
            X = X + CellSize + 5;
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon(), 0xFF, 1);
        if (Item->IsSelected())
            Device->DrawTexture(X - 1, Y - 1, 0x22, 0x22, 0, 0, 0x22, 0x22, SelectTex, 0xFF, 1);
        else
        {
            Device->DrawTexture(X - 1, Y - 1, 0x22, 0x22, 0, 0, 0x22, 0x22, OutlineDownTex, 0xFF, 1);
            Device->DrawTexture(X - 1, Y - 1, 0x23, 0x23, 0, 0, 0x23, 0x23, OutlineTex, 0xFF, 1);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10089a80, NCInvenItemWnd, OnPaint)

// 0x10089ca0 (vslot 61) - the item under the mouse; while an item is carried the cursor is its icon, and over the
// inventory's trash or recipe icon that icon shows its dragged picture (the window keeps the flags)
INT NCInvenItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    HoverIndex = HitTest(X, Y, 1, 1);
    if (IsFocused())
    {
        if (bDragging && Selected >= 0 && Selected < Items.Num())
        {
            SetCursor(4, Items(Selected)->GetIcon());
            LockCursor();
        }
        NCInventoryWnd* Inventory = (NCInventoryWnd*)Entry->Parent->Wnd;
        if (!Inventory->IsA(NClass::NCInventoryWnd()))
            return 1;
        NRectF Box;
        Box.X = Inventory->TrashX + Inventory->ScreenX;
        Box.Y = Inventory->TrashY + Inventory->ScreenY;
        Box.W = Inventory->TrashW;
        Box.H = Inventory->TrashH;
        if (PtInRect(X, Y, Box))
        {
            Inventory->bOverTrash = 1;
            Inventory->TrashButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash_drag"));
            Inventory->TrashButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash_drag"));
        }
        else
        {
            Inventory->bOverTrash = 0;
            Inventory->TrashButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
            Inventory->TrashButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
        }
        Box.X = Inventory->RecipeX + Inventory->ScreenX;
        Box.Y = Inventory->RecipeY + Inventory->ScreenY;
        Box.W = Inventory->RecipeW;
        Box.H = Inventory->RecipeH;
        if (PtInRect(X, Y, Box))
        {
            Inventory->bOverRecipe = 1;
            Inventory->RecipeButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe_drag"));
            Inventory->RecipeButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe_drag"));
        }
        else
        {
            Inventory->bOverRecipe = 0;
            Inventory->RecipeButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
            Inventory->RecipeButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10089ca0, NCInvenItemWnd, OnMouseMove)

// 0x1008bd10 (vslot 63) - the release of the item carried (unless a double click has just used it): with an enchant
// scroll in use the item is the one to enchant; inside the page it moves to the place under the mouse (the last one when
// that is past the end); dropped on the paperdoll it is put on, on the shortcut bar it gets a shortcut, on the pet's
// window it is given to the pet (an amount asks first), on the inventory's trash icon it asks to be destroyed, on the
// recipe icon to be crystallized (when the player may), on the ground (no actor under the cursor) it asks to be
// discarded or, for the etc items of kind 8, placed; the recipe book's item window ignores it
INT NCInvenItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (Selected < 0 || Selected >= Items.Num())
    {
        bDragging = 0;
        return 1;
    }
    if (bEnchantMode && !bDblClicked && Items(Selected))
    {
        ::nw::VirtualCall<void>(GConsole, 394, Items(Selected)->ObjectID);   // RequestEnchantItem
        GDesktop->InventoryWnd->ClearItemSelect();
        UnlockCursor();
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
            INT Target = HitTest(X, Y, 1, 1);
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
                if (Target->IsA(NClass::NCEquipItemWnd()))
                {
                    ((NCEquipItemWnd*)Target)->HitTest(X, Y);
                    GConsole->RequestUseItem(Items(Selected)->ObjectID);
                    UnselectAll();
                    Selected = -1;
                    bDblClicked = 0;
                    bDragging = 0;
                    return 1;
                }
                else if (Target->IsA(NClass::NCShortCutWnd()))
                {
                    INT ShortCut = ((NCShortCutWnd*)Target)->SlotAtPoint(X, Y);
                    if (ShortCut >= 0)
                    {
                        L2ParamStack Params(10);
                        Params.PushBack((void*)1);
                        Params.PushBack((void*)(INT_PTR)ShortCut);
                        Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
                        Params.PushBack((void*)1);
                        GConsole->RequestShortCutReg(Params);
                    }
                    UnselectAll();
                    Selected = -1;
                    bDblClicked = 0;
                    bDragging = 0;
                    return 1;
                }
                else if (Target->IsA(NClass::NCPetWnd()))
                {
                    if (!Item->IsStackable() || Item->Count < 2)
                    {
                        L2ParamStack Params(10);
                        Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
                        Params.PushBack((void*)1);
                        GConsole->RequestGiveItemToPet(Params);
                        bDblClicked = 0;
                        bDragging = 0;
                        return 1;
                    }
                    TCHAR Name[64];
                    appSprintf(Name, TEXT("%s"), Item->GetName());
                    NDialogInfo Info;
                    Info.ID = DIALOG_GIVE_TO_PET;
                    Info.Type = 3;
                    Info.Owner = Entry;
                    Info.bMainBox = 1;
                    Info.bInput = 1;
                    Info.SetSystemMessage(0x48);
                    Info.SetButtons(Name, NULL);
                    ::nw::Field<INT>(&Info, 0x18) = Item->ID;
                    Info.Unknown74 = Item->Count;
                    Info.SetParams(Item->ObjectID, -1);
                    GConsole->ShowDialog(&Info);
                }
                else if (Target->IsA(NClass::NCButton())
                         && Target->Entry->Parent && Target->Entry->Parent->Wnd
                         && Target->Entry->Parent->Wnd->IsA(NClass::NCInventoryWnd()))
                {
                    NCInventoryWnd* Inventory = (NCInventoryWnd*)Target->Entry->Parent->Wnd;
                    Inventory->bOverTrash = 0;
                    Inventory->bOverRecipe = 0;
                    Inventory->TrashButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
                    Inventory->TrashButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
                    Inventory->RecipeButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
                    Inventory->RecipeButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
                    if (Inventory->IsInTrash(X, Y))
                    {
                        TCHAR Name[64];
                        appSprintf(Name, TEXT("%s"), Item->MakeDisplayName());
                        NDialogInfo Info;
                        if (!Item->IsStackable() || Item->Count < 2)
                        {
                            Info.ID = DIALOG_DESTROY_ITEM;
                            Info.Owner = Entry;
                            Info.bMainBox = 1;
                            Info.bInput = 0;
                            Info.SetSystemMessage(MSG_Destroy);
                            Info.SetButtons(Name, NULL);
                            ::nw::Field<INT>(&Info, 0x18) = Item->ID;
                            Info.SetParams(Item->ObjectID, -1);
                        }
                        else
                        {
                            Info.ID = DIALOG_DESTROY_ITEM;
                            Info.Type = 3;
                            Info.Owner = Entry;
                            Info.bMainBox = 1;
                            Info.bInput = 1;
                            Info.SetSystemMessage(MSG_DestroyHowMany);
                            Info.SetButtons(Name, NULL);
                            ::nw::Field<INT>(&Info, 0x18) = Item->ID;
                            Info.Unknown74 = Item->Count;
                            Info.SetParams(Item->ObjectID, -1);
                        }
                        GConsole->ShowDialog(&Info);
                    }
                    else if (Inventory->IsInRecipe(X, Y))
                    {
                        NViewItemData* Data = NItemTable::Find(&Item->ID);
                        User* Me = GConsole->GetMyUser();
                        if (Me->bCanCrystallize && Data && Data->CrystalCount != 0)
                        {
                            TCHAR Name[64];
                            appSprintf(Name, TEXT("%s"), Item->MakeDisplayName());
                            NDialogInfo Info;
                            Info.ID = DIALOG_CRYSTALLIZE_ITEM;
                            Info.Owner = Entry;
                            Info.bMainBox = 1;
                            Info.bInput = 0;
                            Info.SetSystemMessage(0x150);
                            Info.SetButtons(Name, NULL);
                            Info.SetParams(Item->ObjectID, -1);
                            GConsole->ShowDialog(&Info);
                        }
                    }
                }
                else if (!Target->IsA(NClass::NCRecipeBookItemWnd()) && Target->IsA(NClass::NCConsole()))
                {
                    if (!GConsole->GetTargetedActor(1))
                    {
                        TCHAR Name[64];
                        appSprintf(Name, TEXT("%s"), Item->GetName());
                        DropLocation = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
                        UBOOL bPlace = 0;
                        NViewItemData* Data = NItemTable::Find(&Item->ID);
                        if (Data && Data->Class == ITEMCLASS_Etc && Data->EtcType == 8)
                            bPlace = 1;
                        NDialogInfo Info;
                        if (!Item->IsStackable() || Item->Count < 2)
                        {
                            Info.ID = DIALOG_DROP_ITEM;
                            Info.Owner = Entry;
                            Info.bMainBox = 1;
                            Info.bInput = 0;
                            if (bPlace)
                                Info.SetSystemMessage(MSG_Place);
                            else
                                Info.SetSystemMessage(MSG_Drop);
                            Info.SetButtons(Name, NULL);
                            Info.SetParams(Item->ObjectID, -1);
                            Info.Location = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
                        }
                        else if (bPlace)
                        {
                            Info.ID = DIALOG_DROP_ITEM;
                            Info.Owner = Entry;
                            Info.bMainBox = 1;
                            Info.bInput = 0;
                            Info.SetSystemMessage(MSG_Place);
                            Info.SetButtons(Name, NULL);
                            Info.SetParams(Item->ObjectID, -1);
                            Info.Location = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
                        }
                        else
                        {
                            Info.ID = DIALOG_DROP_ITEM;
                            Info.Type = 3;
                            Info.Owner = Entry;
                            Info.bMainBox = 1;
                            Info.bInput = 1;
                            Info.SetSystemMessage(MSG_DropHowMany);
                            Info.SetButtons(Name, NULL);
                            ::nw::Field<INT>(&Info, 0x18) = Item->ID;
                            Info.Unknown74 = Item->Count;
                            Info.SetParams(Item->ObjectID, -1);
                            Info.Location = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
                        }
                        GConsole->ShowDialog(&Info);
                    }
                }
            }
        }
    }
    bDblClicked = 0;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1008bd10, NCInvenItemWnd, OnLButtonUp)

// 0x1008c830 - 0 when the page is full
UBOOL NCInvenItemWnd::AddItem(NItemInfo* Info)
{
    if (MaxItems <= Items.Num())
        return 0;
    Info->Icon = FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = NewItemEntry(Info);
    Item->FillToolTip(2, 0);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x1008c830, NCInvenItemWnd, AddItem)

// 0x1008d070 (C1 0x10068120) - the entry is deleted and the scroll range updated
UBOOL NCInvenItemWnd::RemoveItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (Items(i)->ObjectID == ObjectID)
        {
            NItemEntry* Item = Items(i);
            Items.Remove(i);
            if (Item)
                delete Item;
            UpdateScrollRange();
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x1008d070, NCInvenItemWnd, RemoveItem)

// stock body shared with NCPetInvenItemWnd (0x10091c70), which keeps the address
UBOOL NCInvenItemWnd::HasItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
        if (ObjectID == Items(i)->ObjectID)
            return 1;
    return 0;
}

// stock body shared with NCPetInvenItemWnd (0x10091cc0), which keeps the address
UBOOL NCInvenItemWnd::FindItem(INT ObjectID, INT* Index)
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

NW_CHECK_VTABLE(NCInvenItemWnd, 0x10187ca0, 76)
