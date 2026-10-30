/*=============================================================================
    NCEquipItemWnd.cpp - the inventory's paperdoll. Stock: 0x10060730 .. 0x10062357 (up to
    the tool tip's jump table), 0x100642d0 .. 0x10065056, 0x100661a0 .. 0x10067684 (the item
    pages' and the inventory's functions sit in between), plus the constructor (0x10068380)
    and the destructor (0x1005fd70).
=============================================================================*/
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NMacroEntry.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NDropDialogInfo.h"
#include "nwindow/NHennaData.h"
#if NW_FEATURES
// phase 6: ItemHoverTip (not in NWindow_C1: the hover tip of the item grids in the recent style)
#include "features/ItemHoverTip.h"
#endif

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
    MSG_DestroyHowMany = 73,    // "How many $s1(s) do you want to destroy?"
    MSG_Destroy = 74,           // "Destroy $s1. Do you want to continue?"
    MSG_CannotDropHere = 179,   // "You cannot discard items here."
    MSG_Crystallize = 336,      // "$s1 is crystallized. Do you want to continue?"
    MSG_Drop = 400,             // "Discard $s1. Do you wish to continue?"
    MSG_PetAdenaHowMuch = 575,  // "How much adena do you wish to transfer to your pet?"
    MSG_PetHowMuch = 576,       // "How much do you wish to transfer?"
};

// the console's test (0x1006b6c0), the shortcut selections the right button forgets (below) and the parent's drop icons
static FORCEINLINE NCDialogBox* MainDialogBox()
{
    return GDesktop->MainDialogBox;
}

// The shortcut bars forget a pending selection (C2: the bars' list sits behind the desktop, +0xF8; 0x10056a20 counts
// them, 0x10060ce0 gets one, 0x1008b4b0 clears its slots and the flag at +0x9D4).
static FORCEINLINE void ClearShortCutSelections()
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

// takes Item off when it is the one of Slot (OnRButtonDown, OnLButtonDblClick): by its body part bits, but an earring
// or a ring by the bit of the ear or the finger that wears it; the right button then clears the shortcut bars'
// selections (C2)
static FORCEINLINE UBOOL UnEquipIfInSlot(NCEquipItemWnd* Wnd, NItemEntry* Item, INT Slot, UBOOL bClearShortCuts)
{
    INT ItemSlot = Wnd->GetEquipSlot(Item->BodyPart, Item->ObjectID);
    if (ItemSlot == 16 || ItemSlot == 15)   // C3: 16 and 15 (C2 15 and 14) are the two-slot items
    {
        if (Slot != (ItemSlot == 16 ? 5 : 6))
            return 0;
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Item->BodyPart);
        GConsole->RequestUnEquipItem(Params);
        if (bClearShortCuts)
            ClearShortCutSelections();
        return 1;
    }
    if (Slot != ItemSlot)
        return 0;
    L2ParamStack Params(10);
    INT BodyPart = Item->BodyPart;
    if (BodyPart & 6)
    {
        User* Me = GConsole->GetMyUser();
        if (Me->Equipped[1] == Item->ObjectID)
            Params.PushBack((void*)2);
        if (Me->Equipped[2] == Item->ObjectID)
            Params.PushBack((void*)4);
    }
    else if (Item->BodyPart & 0x30)
    {
        User* Me = GConsole->GetMyUser();
        if (Me->Equipped[4] == Item->ObjectID)
            Params.PushBack((void*)0x10);
        if (Me->Equipped[5] == Item->ObjectID)
            Params.PushBack((void*)0x20);
    }
    else
        Params.PushBack((void*)(INT_PTR)BodyPart);
    GConsole->RequestUnEquipItem(Params);
    if (bClearShortCuts)
        ClearShortCutSelections();
    return 1;
}


// The icons of the inventory window's trash and recipe boxes show their dragged picture while the carried item is over
// them (repeated inline in the stock OnMouseMove of the paperdoll and of the item pages): the boxes are the parent's
// (relative to its corner), the flags and the buttons too
static FORCEINLINE void UpdateDropIcons(NCWnd* Wnd, NCInventoryWnd* Inventory, INT X, INT Y)
{
    NRectF Box;
    Box.X = Inventory->TrashX + Inventory->ScreenX;
    Box.Y = Inventory->TrashY + Inventory->ScreenY;
    Box.W = Inventory->TrashW;
    Box.H = Inventory->TrashH;
    if (Wnd->PtInRect(X, Y, Box))
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
    if (Wnd->PtInRect(X, Y, Box))
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

// 0x1008d740 - 15 slots in a 5 x 3 grid; the two arrays are empty, nothing is selected
NCEquipItemWnd::NCEquipItemWnd(INT InCellSize)
{
    CellSize = InCellSize;
    SelectTex = NULL;
    ToolTipTex = NULL;
    bDragging = 0;
    bEnchantMode = 0;
    bNoDrag = 0;
    bHasUnknown1C98 = 0;
    Unknown104 = 5;
    Unknown108 = 3;
    NumSlots = 15;
    Selected = -1;
    HoverSlot = -1;
    HoverHenna = -1;
    SelectedIndex = -1;
}
NW_HOOK_CTOR_ARGS(0x1008d740, NCEquipItemWnd, (INT A), (A))

// 0x10083fd0 - the tattoos' and the items' arrays, then the window's
NW_HOOK_DTOR(0x10083fd0, NCEquipItemWnd)


// 0x10088770 (vslot 25)
INT NCEquipItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10088770, NCEquipItemWnd, WindowProc)

// 0x100887d0 (vslot 37) - the textures: the tool tip's, the selection's and the two frames, the two-slot item's cap and
// the tattoo's back
INT NCEquipItemWnd::OnCreate()
{
    NCWnd::OnCreate();
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    SelectTex = FindTexture(TEXT("l2ui.NWindow.item_click"));
    OutlineDownTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine_down"));
    OutlineTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine"));
    DualCapTex = FindTexture(TEXT("L2UI.InventoryWnd.Icon_dualcap"));
    HennaBackTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_Mark"));
    return 1;
}
NW_HOOK_METHOD(0x100887d0, NCEquipItemWnd, OnCreate)

// 0x10088850
void NCEquipItemWnd::SetEnchantMode(UBOOL bOn)
{
    bEnchantMode = bOn;
}
NW_HOOK_METHOD(0x10088850, NCEquipItemWnd, SetEnchantMode)

// 0x10088860 (vslot 68) - the console's answer (the slot an item with this body part sits in)
INT NCEquipItemWnd::GetEquipSlot(INT BodyPart, INT ObjectID)
{
    return GConsole->GetEquipSlot(BodyPart, ObjectID);
}
NW_HOOK_METHOD(0x10088860, NCEquipItemWnd, GetEquipSlot)


// 0x10088a40 (vslot 56) - each item at the place of its slot (the console says which: 0 .. 14 a 5 x 3 grid 39 / 38 pixels
// apart from (7, 7); 15 the full armor over 6 and 11, 16 the two-handed weapon over 5 and 7: two icons with the cap over
// the second), a bow in slot 5 when the item with the flag is worn; the frame of a selected item, or the two of the
// others; then the tattoos' boxes (the hennas the class may have) with their icons, a dark veil over those not usable
INT NCEquipItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCEquipItemWnd::OnPaint);
    Device->SetClip(ScreenX - 1, ScreenY - 1, (INT)(Width + 2.0f), (INT)(Height + 2.0f));
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 0;
    INT Y = 0;
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (!Item)
            continue;
        INT Slot = GetEquipSlot(Item->BodyPart, Item->ObjectID);
        UBOOL bTwoSlot = 0;
        switch (Slot)
        {
        case 0:
            X = 7;
            Y = 7;
            break;
        case 1:
            X = 0x2E;
            Y = 7;
            break;
        case 2:
            X = 0x55;
            Y = 7;
            break;
        case 3:
            X = 0x82;
            Y = 7;
            break;
        case 4:
            X = 0xA9;
            Y = 7;
            break;
        case 5:
            X = 7;
            Y = 0x2D;
            break;
        case 6:
            X = 0x2E;
            Y = 0x2D;
            break;
        case 7:
            X = 0x55;
            Y = 0x2D;
            break;
        case 8:
            X = 0x82;
            Y = 0x2D;
            break;
        case 9:
            X = 0xA9;
            Y = 0x2D;
            break;
        case 10:
            X = 7;
            Y = 0x53;
            break;
        case 11:
            X = 0x2E;
            Y = 0x53;
            break;
        case 12:
            X = 0x55;
            Y = 0x53;
            break;
        case 13:
            X = 0x82;
            Y = 0x53;
            break;
        case 14:
            X = 0xA9;
            Y = 0x53;
            break;
        case 15:
        case 16:
            bTwoSlot = 1;
            break;
        default:
            continue;
        }
        if (Item->UnknownC30 == 6 && bHasUnknown1C98)
        {
            X = 7;
            Y = 0x2D;
            bTwoSlot = 0;
        }
        if (!bTwoSlot)
        {
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon(), 0xFF, 1);
            INT FrameX = X - 1;
            INT FrameY = Y - 1;
            if (Item->IsSelected())
                Device->DrawTexture(FrameX, FrameY, 0x22, 0x22, 0, 0, 0x22, 0x22, SelectTex, 0xFF, 1);
            else
            {
                Device->DrawTexture(FrameX, FrameY, 0x22, 0x22, 0, 0, 0x22, 0x22, OutlineDownTex, 0xFF, 1);
                Device->DrawTexture(FrameX, FrameY, 0x23, 0x23, 0, 0, 0x23, 0x23, OutlineTex, 0xFF, 1);
            }
            continue;
        }
        // a two-slot item: the icons are the item's own, or its second and third when the names are there
        UTexture* First;
        UTexture* Second;
        if (Item->Unknown30C[0] != 0)
        {
            First = Item->GetIcon2();
            Second = Item->GetIcon3();
        }
        else if (Item->Unknown20C[0] != 0)
        {
            First = Item->GetIcon2();
            Second = Item->GetIcon2();
        }
        else
        {
            First = Item->GetIcon();
            Second = Item->GetIcon();
        }
        if (Slot == 16)
        {
            Device->DrawTexture(7, 0x2D, CellSize, CellSize, 0, 0, CellSize, CellSize, First, 0xFF, 1);
            Device->DrawTexture(0x55, 0x2D, CellSize, CellSize, 0, 0, CellSize, CellSize, Second, 0xFF, 1);
            Device->DrawTexture(0x55, 0x2D, CellSize, CellSize, 0, 0, CellSize, CellSize, DualCapTex, 0xFF, 1);
            if (Item->IsSelected())
                Device->DrawTexture(6, 0x2C, 0x22, 0x22, 0, 0, 0x22, 0x22, SelectTex, 0xFF, 1);
            else
            {
                Device->DrawTexture(6, 0x2C, 0x22, 0x22, 0, 0, 0x22, 0x22, OutlineDownTex, 0xFF, 1);
                Device->DrawTexture(6, 0x2C, 0x23, 0x23, 0, 0, 0x23, 0x23, OutlineTex, 0xFF, 1);
            }
        }
        else if (Slot == 15)
        {
            Device->DrawTexture(0x2E, 0x2D, CellSize, CellSize, 0, 0, CellSize, CellSize, First, 0xFF, 1);
            Device->DrawTexture(0x2E, 0x53, CellSize, CellSize, 0, 0, CellSize, CellSize, Second, 0xFF, 1);
            Device->DrawTexture(0x2E, 0x53, CellSize, CellSize, 0, 0, CellSize, CellSize, DualCapTex, 0xFF, 1);
            if (Item->IsSelected())
                Device->DrawTexture(0x2D, 0x2C, 0x22, 0x22, 0, 0, 0x22, 0x22, SelectTex, 0xFF, 1);
            else
            {
                Device->DrawTexture(0x2D, 0x2C, 0x22, 0x22, 0, 0, 0x22, 0x22, OutlineDownTex, 0xFF, 1);
                Device->DrawTexture(0x2D, 0x2C, 0x23, 0x23, 0, 0, 0x23, 0x23, OutlineTex, 0xFF, 1);
            }
        }
    }
    if (HennaBackTex)
    {
        INT Count = GConsole->GetHennaSlotCount();
        for (INT i = 0; i < Count; i++)
        {
            Device->DrawTexture(0xD7, i * 0x1B + 8, 0x1A, 0x1A, 0, 0, 0x1A, 0x1A, HennaBackTex, 0xFF, 1);
            if (i < Hennas.Num() && Hennas(i)->GetIcon())
            {
                Device->DrawStretchedTex(0xD8, i * 0x1B + 9, 0x18, 0x18, Hennas(i)->GetIcon(), 1, 0xFF);
                if (Hennas(i)->UnknownC34 == 0)
                    Device->DrawTexture(0xD9, i * 0x1B + 9, 0x18, 0x18, 0, 0, 0x18, 0x18,
                                        FindTexture(TEXT("NWindow.ChatBack")), 0xFF, 1);
            }
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10088a40, NCEquipItemWnd, OnPaint)


// 0x1008d7f0 (vslot 57) - the tool tip of the slot under the mouse (the item whose slot it is, a two-slot item by either
// of its slots): the title box of an item that is not selected, the full box of a selected one, placed beside the slot; then
// the tattoo under the mouse
INT NCEquipItemWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    INT Row = HoverSlot / 5;
    INT Col = HoverSlot % 5;
    INT Y = (CellSize + 6) * Row + 0xB;
    INT X;
    if (Col == 3)
        X = CellSize * 3 + 0x22;
    else if (Col == 4)
        X = CellSize * 4 + 0x29;
    else
        X = (CellSize + 7) * Col + 7;
    for (INT i = 0; i < Items.Num(); i++)
    {
        INT Slot = GetEquipSlot(Items(i)->BodyPart, Items(i)->ObjectID);
        UBOOL bMatch;
        if (HoverSlot == Slot)
            bMatch = 1;
        else if (Slot == 16)
            bMatch = HoverSlot == 5;
        else if (Slot == 15)
            bMatch = HoverSlot == 6;
        else
            bMatch = 0;
        if (bMatch)
        {
            if (!Items(i)->IsSelected())
            {
                if (ToolTipSkin)
                {
                    NToolTipRect Wnd;
                    Wnd.X = ScreenX;
                    Wnd.Y = ScreenY;
                    Wnd.W = Width;
                    Wnd.H = Height;
                    ToolTipSkin->Draw(Device, Items(i)->ToolTip, X, Y, Wnd);
                }
            }
            else if (ToolTipSkin)
            {
                NToolTipRect Wnd;
                Wnd.X = ScreenX;
                Wnd.Y = ScreenY;
                Wnd.W = Width;
                Wnd.H = Height;
                ToolTipSkin->DrawFull(Device, Items(i)->ToolTip, X, Y, Wnd);
            }
            break;
        }
    }
    if (HoverHenna >= 0 && HoverHenna < Hennas.Num() && Hennas(HoverHenna) && ToolTipSkin)
    {
        NToolTipRect Wnd;
        Wnd.X = ScreenX;
        Wnd.Y = ScreenY;
        Wnd.W = Width;
        Wnd.H = Height;
        ToolTipSkin->DrawFull(Device, Hennas(HoverHenna)->ToolTip, 0xD8, HoverHenna * 0x1A + 8, Wnd);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x1008d7f0, NCEquipItemWnd, OnDrawToolTip)

// 0x100890b0
void NCEquipItemWnd::UnselectAll()
{
    for (INT i = 0; i < Items.Num(); i++)
        Items(i)->Unselect();
}
NW_HOOK_METHOD(0x100890b0, NCEquipItemWnd, UnselectAll)

// 0x100890e0 (ret 8) - the slot whose 32 x 32 box (from (7, 7), the columns 0 / 39 / 78 / 123 / 162 apart and the rows
// 0 / 38 / 76) holds the point, -1 outside
INT NCEquipItemWnd::HitTest(INT X, INT Y)
{
    static const INT ColumnX[5] = { 0, 0x27, 0x4E, 0x7B, 0xA2 };
    static const INT RowY[3] = { 0, 0x26, 0x4C };
    NRectF Box;
    Box.W = 32.0f;
    Box.H = 32.0f;
    for (INT i = 0; i < 15; i++)
    {
        Box.X = ScreenX + 7 + ColumnX[i % 5];
        Box.Y = ScreenY + 7 + RowY[i / 5];
        if (PtInRect(X, Y, Box))
            return i;
    }
    return -1;
}
NW_HOOK_METHOD(0x100890e0, NCEquipItemWnd, HitTest)


// 0x10089290 - the item with Info's object id takes Info's fields and a new tool tip; the entry built for that is
// never freed, as in the stock code
UBOOL NCEquipItemWnd::UpdateItem(NItemInfo* Info)
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
NW_HOOK_METHOD(0x10089290, NCEquipItemWnd, UpdateItem)

// 0x10089390
UBOOL NCEquipItemWnd::HasItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
        if (ObjectID == Items(i)->ObjectID)
            return 1;
    return 0;
}
NW_HOOK_METHOD(0x10089390, NCEquipItemWnd, HasItem)

// 0x100893e0
UBOOL NCEquipItemWnd::FindItem(INT ObjectID, INT* Index)
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
NW_HOOK_METHOD(0x100893e0, NCEquipItemWnd, FindItem)

// 0x1008a620 (vslot 61) - selects the item of the slot under the mouse and picks it up (unless bNoDrag, which lasts
// one click); a two-slot item met on the way unselects all. C2: the questions pending are not forgotten here, the
// desktop's box is left alone.
INT NCEquipItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    guard(NCEquipItemWnd::OnLButtonDown);
    INT Slot = HitTest(LOWORD(lParam), HIWORD(lParam));
    HoverSlot = Slot;
    Selected = Slot;
    UBOOL bFound = 0;
    for (INT i = 0; i < Items.Num(); i++)
    {
        INT ItemSlot = GetEquipSlot(Items(i)->BodyPart, Items(i)->ObjectID);
        if (ItemSlot == 16)
        {
            UnselectAll();
            if (Selected != 5)
                continue;
        }
        else if (ItemSlot == 15)
        {
            UnselectAll();
            if (Selected != 6)
                continue;
        }
        else if (Selected == ItemSlot)
            UnselectAll();
        else
            continue;
        Items(i)->Select();
        SelectedIndex = i;
        bFound = 1;
        break;
    }
    if (!bFound)
        SelectedIndex = -1;
    if (!bNoDrag)
    {
        bDragging = 1;
        GConsole->PlayInterfaceSound(SOUND_PickUp);
    }
    else
        bNoDrag = 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008a620, NCEquipItemWnd, OnLButtonDown)

// 0x10089200 (name ours) - the tattoo under the mouse (its box, relative to the window), -1 outside
INT NCEquipItemWnd::HitTestHenna(INT X, INT Y)
{
    INT RX = X - ScreenX;
    INT RY = Y - ScreenY;
    for (INT i = 0; i < Hennas.Num(); i++)
    {
        if (PtInRect(RX, RY, Hennas(i)->Box))
            return i;
    }
    return -1;
}
NW_HOOK_METHOD(0x10089200, NCEquipItemWnd, HitTestHenna)

// 0x1008a7b0 (vslot 61) - the slot and the tattoo under the mouse; while an item is carried the cursor is its icon, and
// over the inventory's trash or recipe icon that icon shows its dragged picture
INT NCEquipItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    HoverSlot = HitTest(X, Y);
    HoverHenna = HitTestHenna(X, Y);
    if (IsFocused())
    {
        if (bDragging && Selected >= 0 && Selected < NumSlots && SelectedIndex < Items.Num() && SelectedIndex >= 0)
        {
            SetCursor(4, Items(SelectedIndex)->GetIcon());
            LockCursor();
        }
        NCInventoryWnd* Inventory = (NCInventoryWnd*)Entry->Parent->Wnd;
        if (!Inventory->IsA(NClass::NCInventoryWnd()))
            return 1;
        UpdateDropIcons(this, Inventory, X, Y);
    }
    return 1;
}
NW_HOOK_METHOD(0x1008a7b0, NCEquipItemWnd, OnMouseMove)

// The shortcut selections of the macros are forgotten, as the stock paperdoll does after each request (repeated inline)

// The request that takes the item off when it is dropped on the inventory page: the body part, or for a ring / earring
// the part of the finger / ear that holds it (a worn pair is told apart by the object id; the else-if of the fingers is
// the stock's)
static FORCEINLINE void RequestTakeOff(NItemEntry* Item)
{
    L2ParamStack Params(10);
    INT BodyPart = Item->BodyPart;
    if (BodyPart & 6)
    {
        User* Me = GConsole->GetMyUser();
        if (Me->Equipped[1] == Item->ObjectID)
            Params.PushBack((void*)2);
        if (Me->Equipped[2] == Item->ObjectID)
            Params.PushBack((void*)4);
    }
    else if (BodyPart & 0x30)
    {
        User* Me = GConsole->GetMyUser();
        if (Me->Equipped[4] == Item->ObjectID)
            Params.PushBack((void*)0x10);
        else if (Me->Equipped[5] == Item->ObjectID)
            Params.PushBack((void*)0x20);
    }
    else
        Params.PushBack((void*)(INT_PTR)BodyPart);
    GConsole->RequestUnEquipItem(Params);
}

// 0x1008a9d0 (vslot 63) - the release of a carried item (the stock calls it OnLButtonDown in its guard names): with an
// enchant scroll in use the item under the cursor is the one to enchant; dropped on the inventory page the item is taken
// off, on the shortcut bar it gets a shortcut, on the pet's items it is given to the pet (an amount asks first), on the
// trash icon it asks to be destroyed, on the recipe icon to be crystallized (when the player may), on the ground
// (no actor under the cursor) it asks to be discarded
INT NCEquipItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (Selected < 0 || Selected >= NumSlots)
    {
        bDragging = 0;
        return 1;
    }
    // the item of the selected slot
    INT Index = 0;
    if (Items.Num() > 0)
    {
        UBOOL bFound;
        do
        {
            INT Slot = GetEquipSlot(Items(Index)->BodyPart, Items(Index)->ObjectID);
            if (Slot == 16)
                bFound = Selected == 5;
            else if (Slot == 15)
                bFound = Selected == 6;
            else
                bFound = Selected == Slot;
        } while (!bFound && ++Index < Items.Num());
    }
    if (Index == Items.Num())
    {
        bDragging = 0;
        if (bEnchantMode)
        {
            ::nw::VirtualCall<void>(GConsole, 394, (INT)-1);   // RequestEnchantItem
            GDesktop->InventoryWnd->ClearItemSelect();
            UnlockCursor();
        }
        return 1;
    }
    if (bEnchantMode && Index >= 0)
    {
        if (Items(Index))
            ::nw::VirtualCall<void>(GConsole, 394, Items(Index)->ObjectID);
        GDesktop->InventoryWnd->ClearItemSelect();
        UnlockCursor();
    }
    NItemEntry* Item = Items(Index);
    if (bDragging && Item)
    {
        NWndEntry* Found = FindWindowAt(X, Y);
        if (Found && Found->Wnd)
        {
            NCWnd* Target = Found->Wnd;
            if (Target->IsA(NClass::NCInvenItemWnd()))
            {
                // onto the inventory page: taken off (unless it is one of the items that stay on)
                if (Item->Unknown1C98 == 0)
                {
                    RequestTakeOff(Items(Index));
                    UnselectAll();
                    Selected = -1;
                    bDragging = 0;
                    return 1;
                }
            }
            else if (Target->IsA(NClass::NCShortCutWnd()))
            {
                INT ShortCut = ((NCShortCutWnd*)Target)->SlotAtPoint(X, Y);
                if (ShortCut >= 0)
                {
                    L2ParamStack Params(10);
                    Params.PushBack((void*)1);
                    Params.PushBack((void*)(INT_PTR)ShortCut);
                    Params.PushBack((void*)(INT_PTR)Items(Index)->ObjectID);
                    Params.PushBack((void*)1);
                    GConsole->RequestShortCutReg(Params);
                }
                UnselectAll();
                Selected = -1;
                bDragging = 0;
                return 1;
            }
            else if (Target->IsA(NClass::NCPetInvenItemWnd()))
            {
                if (!Item->IsStackable() || Item->Count < 2)
                {
                    L2ParamStack Params(10);
                    Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
                    Params.PushBack((void*)1);
                    GConsole->RequestGiveItemToPet(Params);
                    bDragging = 0;
                    return 1;
                }
                TCHAR Name[64];
                appSprintf(Name, TEXT("%s"), Item->GetName());
                NDialogInfo Info;
                Info.ID = 0x36;
                Info.Type = 3;
                Info.Owner = Entry;
                Info.bMainBox = 1;
                Info.bInput = 1;
                Info.SetSystemMessage(0x23F);
                Info.SetButtons(Name, NULL);
                ::nw::Field<INT>(&Info, 0x18) = Item->ID;
                Info.Unknown74 = Item->Count;
                Info.SetParams(Item->ObjectID, -1);
                GConsole->ShowDialog(&Info);
            }
            else if (Target->IsA(NClass::NCButton()))
            {
                NWndEntry* Parent = Target->Entry->Parent;
                if (Parent && Parent->Wnd && Parent->Wnd->IsA(NClass::NCInventoryWnd()))
                {
                    NCInventoryWnd* Inventory = (NCInventoryWnd*)Parent->Wnd;
                    Inventory->bOverTrash = 0;
                    Inventory->bOverRecipe = 0;
                    Inventory->TrashButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
                    Inventory->TrashButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
                    Inventory->RecipeButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
                    Inventory->RecipeButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
                    if (Inventory->IsInTrash(X, Y))
                    {
                        TCHAR Name[64];
                        if (Item->IsStackable() && Item->Count > 1)
                        {
                            appSprintf(Name, TEXT("%s"), Item->MakeDisplayName());
                            NDialogInfo Info;
                            Info.ID = 5;
                            Info.Type = 3;
                            Info.Owner = Entry;
                            Info.bMainBox = 1;
                            Info.bInput = 1;
                            Info.SetSystemMessage(0x49);
                            Info.SetButtons(Name, NULL);
                            Info.Unknown74 = Item->Count;
                            ::nw::Field<INT>(&Info, 0x18) = Item->ID;
                            Info.SetParams(Item->ObjectID, -1);
                            GConsole->ShowDialog(&Info);
                        }
                        else
                        {
                            appSprintf(Name, TEXT("%s"), Item->MakeDisplayName());
                            NDialogInfo Info;
                            Info.ID = 5;
                            Info.Owner = Entry;
                            Info.bMainBox = 1;
                            Info.bInput = 0;
                            Info.SetSystemMessage(0x4A);
                            Info.SetButtons(Name, NULL);
                            ::nw::Field<INT>(&Info, 0x18) = Item->ID;
                            Info.SetParams(Item->ObjectID, -1);
                            GConsole->ShowDialog(&Info);
                        }
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
                            Info.ID = 6;
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
            }
            else if (Target->IsA(NClass::NCConsole()))
            {
                if (!GConsole->GetTargetedActor(1))
                {
                    TCHAR Name[64];
                    appSprintf(Name, TEXT("%s"), Item->GetName());
                    DropLocation = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
                    if (Item->IsStackable() && Item->Count > 1)
                    {
                        NDialogInfo Info;
                        Info.ID = 4;
                        Info.Type = 3;
                        Info.Owner = Entry;
                        Info.bMainBox = 1;
                        Info.bInput = 1;
                        Info.SetSystemMessage(0x47);
                        Info.SetButtons(Name, NULL);
                        ::nw::Field<INT>(&Info, 0x18) = Item->ID;
                        Info.Unknown74 = Item->Count;
                        Info.SetParams(Item->ObjectID, -1);
                        Info.Location = DropLocation;
                        GConsole->ShowDialog(&Info);
                    }
                    else
                    {
                        NDialogInfo Info;
                        Info.ID = 4;
                        Info.Owner = Entry;
                        Info.bMainBox = 1;
                        Info.bInput = 0;
                        Info.SetSystemMessage(400);
                        Info.SetButtons(Name, NULL);
                        Info.SetParams(Item->ObjectID, -1);
                        Info.Location = DropLocation;
                        GConsole->ShowDialog(&Info);
                    }
                }
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1008a9d0, NCEquipItemWnd, OnLButtonUp)

// 0x1008b480 (vslot 64) - the right button takes off the item of the slot under the mouse (the shortcut selections of
// the macros are forgotten) and drops what was carried
INT NCEquipItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < NumSlots)
    {
        for (INT i = 0; i < Items.Num(); i++)
            if (UnEquipIfInSlot(this, Items(i), Selected, 1))
                break;
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1008b480, NCEquipItemWnd, OnRButtonDown)

// 0x1008b900 (vslot 64) - takes off the item of the slot under the mouse; the release that follows moves nothing
INT NCEquipItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < NumSlots)
    {
        for (INT i = 0; i < Items.Num(); i++)
            if (UnEquipIfInSlot(this, Items(i), Selected, 0))
                break;
    }
    UnselectAll();
    Selected = -1;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1008b900, NCEquipItemWnd, OnLButtonDblClick)

// 0x1008cd60 (vslot 37)
INT NCEquipItemWnd::OnDestroy()
{
    guard(NCEquipItemWnd::OnDestroy);
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (Item)
        {
            Item->ToolTip.~NToolTipInfo();   // 0x100025a0 (inline in the stock code)
            appFree(Item);
        }
    }
    Items.Empty();
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008cd60, NCEquipItemWnd, OnDestroy)

// 0x1008bb60 - the three icons come from their names ("NWindow.BlackTexture" for those missing), written back into
// Info; 0 when the fourteen slots are taken. Every add checks again whether an item with Unknown1C98 is equipped.
UBOOL NCEquipItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= NumSlots)
        return 0;
    if (Info->IconName[0])
        Info->Icon = FindTexture(Info->IconName);
    if (Info->Unknown200[0])
        Info->UnknownC04 = (INT)(INT_PTR)FindTexture(Info->Unknown200);
    if (Info->Unknown300[0])
        Info->UnknownC08 = (INT)(INT_PTR)FindTexture(Info->Unknown300);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    if (!Info->UnknownC04)
        Info->UnknownC04 = (INT)(INT_PTR)FindTexture(TEXT("NWindow.BlackTexture"));
    if (!Info->UnknownC08)
        Info->UnknownC08 = (INT)(INT_PTR)FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = NewItemEntry(Info);
    Item->FillToolTip(2, 0);
    Items.AddItem(Item);
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (Items(i)->Unknown1C98)
        {
            bHasUnknown1C98 = 1;
            break;
        }
        bHasUnknown1C98 = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x1008bb60, NCEquipItemWnd, AddItem)

// 0x1008ce30 (C1 0x100675c0) - the item is deleted; bHasUnknown1C98 stays as it was
UBOOL NCEquipItemWnd::RemoveItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (Items(i)->ObjectID == ObjectID)
        {
            NItemEntry* Item = Items(i);
            Items.Remove(i);
            if (Item)
                delete Item;
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x1008ce30, NCEquipItemWnd, RemoveItem)

// 0x1008cf00 - a tattoo of the game data: its name, text and icon (the black texture when it has none), usable or not,
// at the next of the boxes of the column (x 216, y 27 each from 8, 24 x 24); the tool tip is its name's
void NCEquipItemWnd::AddHenna(INT HennaID, INT bUsable)
{
    FL2HennaData* Data = GL2GameData.GetHennaData(HennaID);
    if (Data)
    {
        NHennaEntry* Henna = (NHennaEntry*)appMalloc(sizeof(NHennaEntry), TEXT("new"));
        if (Henna)
            Henna = new ((void*)Henna) NHennaEntry();
        appStrcpy(Henna->Name, *Data->Name);
        appStrcpy(Henna->Description, *Data->Text);
        appStrcpy(Henna->IconName, *Data->IconName);
        Henna->UnknownC34 = bUsable;
        Henna->Icon = FindTexture(Henna->IconName);
        if (!Henna->Icon)
            Henna->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
        Henna->Box.Y = Hennas.Num() * 0x1B + 8;
        Henna->Box.X = 0xD8;
        Henna->Box.W = 24.0f;
        Henna->Box.H = 24.0f;
        Henna->ToolTip.Fill(Henna, NTT_Name, 0);
        Hennas.AddItem(Henna);
    }
}
NW_HOOK_METHOD(0x1008cf00, NCEquipItemWnd, AddHenna)

// 0x1008e350 (name ours) - every tattoo deleted
void NCEquipItemWnd::ClearHennas()
{
    for (INT i = 0; i < Hennas.Num(); i++)
        if (Hennas(i))
            delete Hennas(i);
    Hennas.Empty();
}
NW_HOOK_METHOD(0x1008e350, NCEquipItemWnd, ClearHennas)

// 0x1008cea0
void NCEquipItemWnd::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i))
            delete Items(i);
    Items.Empty();
}
NW_HOOK_METHOD(0x1008cea0, NCEquipItemWnd, Clear)

// stock body shared with NCEditBox's (0x100a5d70)
void NCEquipItemWnd::ShowWindow()
{
    NCWnd::ShowWindow();
}

// stock body shared with NCChatWnd's (0x1003dab0)
void NCEquipItemWnd::HideWindow()
{
    NCWnd::HideWindow();
}

NW_CHECK_VTABLE(NCEquipItemWnd, 0x10187b88, 70)
