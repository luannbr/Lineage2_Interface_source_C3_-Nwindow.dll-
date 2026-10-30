/*=============================================================================
    NCInventoryWnd.cpp - the inventory. Stock: 0x100641f0 .. 0x10064289, 0x10065920 ..
    0x10066182, 0x100681a0 .. 0x10068373 and OnCreate (0x100684d0); the item pages' and the
    paperdoll's functions sit in between.
=============================================================================*/
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NConsoleFns.h"

#include <stdio.h>
#include <wchar.h>

// ids (names ours)
enum
{
    NID_INVENTORY_FRAME = 0xF2,
    NID_INVENTORY_TAB = 0xFE001A,
    NID_INVENTORY_MANUFACTURE = 0xFE001C,
};

// sysstring
enum
{
    STR_Item = 2,
    STR_Quest = 118,
};

// the console's test (0x1006b6c0) that shows the manufacture button
static FORCEINLINE UBOOL CanManufacture()
{
    return ((NConsoleLeftFns*)GConsole)->IsDwarf();
}

// 0x1008db00 - the icon boxes: the trash at (208, 351), the recipe icon at (14, 351), 34 x 34 (floats)
NCInventoryWnd::NCInventoryWnd()
{
    BackTex = NULL;
    TabCtrl = NULL;
    ItemWnd[0] = NULL;
    ItemWnd[1] = NULL;
    bOverRecipe = 0;
    bOverTrash = 0;
    bManufacturePressed = 0;
    ManufactureButton = NULL;
    TrashButton = NULL;
    RecipeButton = NULL;
    TrashW = 34.0f;
    TrashH = 34.0f;
    TrashX = 0xD0;
    TrashY = 0x15F;
    RecipeW = 34.0f;
    RecipeH = 34.0f;
    RecipeX = 0xE;
    RecipeY = 0x15F;
}
NW_HOOK_CTOR(0x1008db00, NCInventoryWnd)

// 0x1008dc10 - the order list, then the window's
NW_HOOK_DTOR(0x1008dc10, NCInventoryWnd)

// 0x10088880 (vslot 25) - C2: no IsEnabled test; the manufacture button
INT NCInventoryWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_INVENTORY_MANUFACTURE)
        return OnManufacture() != 0;
    return 0;
}
NW_HOOK_METHOD(0x10088880, NCInventoryWnd, WindowProc)

// 0x10088a20 (vslot 67; name ours) - the manufacture button: the recipe book is asked for
INT NCInventoryWnd::OnManufacture()
{
    GConsole->RequestRecipeBookOpen();
    return 1;
}
NW_HOOK_METHOD(0x10088a20, NCInventoryWnd, OnManufacture)

// 0x1008a320 (ret 8, name ours) - the point is inside the manufacture button (its box relative to this window, edges
// included)
UBOOL NCInventoryWnd::IsInManufactureButton(INT X, INT Y)
{
    NRectF Rect;
    Rect.X = 0;
    Rect.Y = 0;
    Rect.W = 0.0f;
    Rect.H = 0.0f;
    ManufactureButton->GetLocalRect(&Rect);
    INT RelX = X - (ScreenX + Rect.X);
    INT RelY = Y - (ScreenY + Rect.Y);
    if (RelX >= 0 && RelX <= (INT)Rect.W && RelY >= 0 && RelY <= (INT)Rect.H)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x1008a320, NCInventoryWnd, IsInManufactureButton)

// 0x10088960 (ret 8) - the point is inside the trash icon's box (from this window's corner; the box is a position
// and a size in floats), edges included
UBOOL NCInventoryWnd::IsInTrash(INT X, INT Y)
{
    INT RelX = X - (TrashX + ScreenX);
    INT RelY = Y - (TrashY + ScreenY);
    if (RelX >= 0 && RelX <= (INT)TrashW && RelY >= 0 && RelY <= (INT)TrashH)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10088960, NCInventoryWnd, IsInTrash)

// 0x100889c0 (ret 8) - the same for the recipe icon's box
UBOOL NCInventoryWnd::IsInRecipe(INT X, INT Y)
{
    INT RelX = X - (RecipeX + ScreenX);
    INT RelY = Y - (RecipeY + ScreenY);
    if (RelX >= 0 && RelX <= (INT)RecipeW && RelY >= 0 && RelY <= (INT)RecipeH)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x100889c0, NCInventoryWnd, IsInRecipe)

// 0x1008c970 (vslot 61) - while an enchant scroll is in use the mouse leaving the window cancels it; the outline over the
// manufacture button is the pressed one while the mouse is down on it
INT NCInventoryWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = (SWORD)LOWORD(lParam);
    INT Y = (SWORD)HIWORD(lParam);
    if (EquipWnd->bEnchantMode)
    {
        NRectF Rect;
        Rect.X = 0;
        Rect.Y = 0;
        Rect.W = 0.0f;
        Rect.H = 0.0f;
        GetLocalRect(&Rect);
        if (!PtInRect(X, Y, Rect))
        {
            ::nw::VirtualCall<void>(GConsole, 394, (INT)-1);   // RequestEnchantItem (the header's vtable order is not the stock's here)
            ClearItemSelect();
            UnlockCursor();
        }
    }
    if (IsInManufactureButton(X, Y) && bManufacturePressed)
        CurOutlineTex = OutlineDownTex;
    else
        CurOutlineTex = OutlineTex;
    return 1;
}
NW_HOOK_METHOD(0x1008c970, NCInventoryWnd, OnMouseMove)

// 0x1008c930 (vslot 62) - the mouse goes down on the manufacture button
INT NCInventoryWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsInManufactureButton((SWORD)LOWORD(lParam), (SWORD)HIWORD(lParam)))
    {
        bManufacturePressed = 1;
        CurOutlineTex = OutlineDownTex;
    }
    return 1;
}
NW_HOOK_METHOD(0x1008c930, NCInventoryWnd, OnLButtonDown)

// 0x10088940 (vslot 63)
INT NCInventoryWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    CurOutlineTex = OutlineTex;
    bManufacturePressed = 0;
    return 1;
}
NW_HOOK_METHOD(0x10088940, NCInventoryWnd, OnLButtonUp)

// 0x100888e0 (vslot 49) - minimizing: the icon shows the inventory's picture (and is brought in when the window was
// left of the screen), the window hides and the icon takes the focus
void NCInventoryWnd::Minimize()
{
    if (Icon)
        Icon->SetIcon(TEXT("L2UI_CH3.MenuIcon.Menuicon2"), 0x8A, 1);
    if (ScreenX < 0 && Icon)
        Icon->OffsetWindow(-ScreenX, 0);
    HideWindow();
    if (Icon)
    {
        Icon->ShowWindow();
        Icon->SetFocus();
    }
}
NW_HOOK_METHOD(0x100888e0, NCInventoryWnd, Minimize)

// 0x1008cab0 - the ids of the "Item" page in its order, written to the file of the character (..\iwo)
void NCInventoryWnd::SaveItemOrder()
{
    ItemOrder.Empty();
    INT Count = ItemWnd[0]->Items.Num();
    for (INT i = 0; i < Count; i++)
        ItemOrder.AddItem(ItemWnd[0]->Items(i)->ID);
    if (::nw::VirtualCall<UBOOL>(GFileManager, 10, TEXT("..\\iwo"), 0))   // FFileManager::MakeDirectory
    {
        FArchive* Ar = GFileManager->CreateFileWriter(ItemOrderFile, FILEWRITE_EvenIfReadOnly, GNull);
        if (Ar)
        {
            for (INT i = 0; i < Count; i++)
                Ar->ByteOrderSerialize(&ItemOrder(i), 4);
            Ar->Close();
            delete Ar;
        }
    }
}
NW_HOOK_METHOD(0x1008cab0, NCInventoryWnd, SaveItemOrder)

// 0x1008cb80 (name ours) - the file of the character (..\iwo\<name>_<server>.dat) read into the order list: the whole
// file in a buffer, then its integers
void NCInventoryWnd::ReadItemOrder()
{
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        ItemOrder.Empty();
        appSprintf(ItemOrderFile, TEXT("..\\iwo\\%s_%d.dat"), Me->GetName(), GConsole->Unknown39D0);
        FArchive* File = GFileManager->CreateFileReader(ItemOrderFile, 0, GNull);
        if (File)
        {
            INT Size = File->TotalSize();
            BYTE* Buffer = (BYTE*)appMalloc(Size + 2, TEXT("new"));
            memset(Buffer, 0, Size + 2);
            File->Serialize(Buffer, Size);
            FArchive* Mem = GFileManager->CreateMemReader(Buffer, Size);
            if (Mem)
            {
                for (INT N = Size >> 2; N > 0; N--)
                {
                    INT Value = 0;
                    Mem->ByteOrderSerialize(&Value, 4);
                    ItemOrder.AddItem(Value);
                }
                Mem->Close();
            }
            File->Close();
            appFree(Buffer);
            if (Mem)
                delete Mem;
        }
    }
}
NW_HOOK_METHOD(0x1008cb80, NCInventoryWnd, ReadItemOrder)

// 0x1008ccd0 (name ours) - the page's last item moves to the front (a new item shows first)
void NCInventoryWnd::MoveLastToFront()
{
    NCInvenItemWnd* Page = ItemWnd[0];
    INT Count = Page->Items.Num();
    if (Count > -1)
    {
        NItemEntry* Last = Page->Items(Count - 1);
        Page->Items.Remove(Count - 1, 1);
        Page->Items.Insert(0, 1);
        Page->Items(0) = Last;
    }
}
NW_HOOK_METHOD(0x1008ccd0, NCInventoryWnd, MoveLastToFront)

// 0x1008cd30 (vslot 45) - the page's order is remembered when it has items
void NCInventoryWnd::HideWindow()
{
    NCWnd::HideWindow();
    if (ItemWnd[0]->Items.Num() > 0)
        SaveItemOrder();
}
NW_HOOK_METHOD(0x1008cd30, NCInventoryWnd, HideWindow)

// 0x1008d540 - the "Item" page back in the saved order: the items the file does not know (new ones) stay first, then the
// saved ids one by one: each is looked for from its own place on and swapped (by content) to the next free place
void NCInventoryWnd::RestoreItemOrder()
{
    NCInvenItemWnd* Page = ItemWnd[0];
    INT Count = Page->Items.Num();
    INT First = 0;
    for (INT i = 0; i < Count; i++)
    {
        INT Known = 0;
        for (INT j = 0; j < ItemOrder.Num(); j++)
        {
            if (Page->Items(i)->ID == ItemOrder(j))
            {
                Known = 1;
                break;
            }
        }
        if (Known)
            break;
        First++;
    }
    for (INT k = 0; k < ItemOrder.Num(); k++)
    {
        if (k >= Page->Items.Num())
            continue;
        INT Found = k;
        INT bFound = 0;
        for (; Found < Page->Items.Num(); Found++)
        {
            if (ItemOrder(k) == Page->Items(Found)->ID && First < Page->Items.Num())
            {
                bFound = 1;
                break;
            }
        }
        if (bFound)
        {
            NItemEntry Temp;
            Temp.Copy(Page->Items(First));
            Page->Items(First)->Copy(Page->Items(Found));
            Page->Items(Found)->Copy(&Temp);
            First++;
        }
    }
}
NW_HOOK_METHOD(0x1008d540, NCInventoryWnd, RestoreItemOrder)

// 0x1008d720 (name ours) - the console calls it: only when the page has items
void NCInventoryWnd::ApplyItemOrder()
{
    if (ItemWnd[0]->Items.Num() > 0)
        RestoreItemOrder();
}
NW_HOOK_METHOD(0x1008d720, NCInventoryWnd, ApplyItemOrder)

// 0x1008d530 (name ours) - the paperdoll's AddHenna (a tail jump in the stock code; the console's slot 137 jumps here)
void NCInventoryWnd::AddHenna(INT HennaID, INT bUsable)
{
    EquipWnd->AddHenna(HennaID, bUsable);
}
NW_HOOK_METHOD(0x1008d530, NCInventoryWnd, AddHenna)

// 0x1008e3b0 (name ours) - the paperdoll's ClearHennas (a tail jump; the console's slot 136 jumps here)
void NCInventoryWnd::ClearHennas()
{
    EquipWnd->ClearHennas();
}
NW_HOOK_METHOD(0x1008e3b0, NCInventoryWnd, ClearHennas)

// 0x10089ea0 (vslot 56) - the back, the adena (the sum of the "Item" page's adena items), the weight bar (85 pixels at
// most: the share of the player's load in the bar's color for its size; the text beside it in 2 decimals), the slots used
// and the most the character may carry, the manufacture button's outline
INT NCInventoryWnd::OnPaint(NCGDevice* Device)
{
    guard(NCInventoryWnd::OnPaint);
    TCHAR Text[128];
    TCHAR Formatted[128];
    TCHAR PercentText[128];
    TCHAR CountText[68];
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 0x14, 0x100, 0x17D, 0, 0, 0x100, 0x17D, BackTex, 0xFF, 1);

    appSprintf(Text, TEXT("%d"), ItemWnd[0]->GetAdena());
    FormatNumber(Formatted, Text);
    appSprintf(Text, TEXT("%s"), Formatted);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(0xC7 - Size.cx, 0x164, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    if (AdenaTex)
        Device->DrawTexture(0x62, 0x163, 0x10, 0xC, 0, 0, 0x10, 0xC, AdenaTex, 0xFF, 1);
    if (WeightIconTex)
        Device->DrawTexture(0x62, 0x174, 0x10, 0xC, 0, 0, 0x10, 0xC, WeightIconTex, 0xFF, 1);

    User* Me = GConsole->GetMyUser();
    CountText[0] = 0;
    if (Me)
    {
        INT MaxLoad = Me->MaxLoad;
        FLOAT Percent = 0.0f;
        if (MaxLoad != 0)
        {
            INT Load = Me->Load;
            INT BarLen = Load * 0x55 / MaxLoad;
            if (BarLen > 0)
            {
                Percent = (FLOAT)((DOUBLE)Load / (DOUBLE)MaxLoad * 100.0);
                UTexture* Tex;
                if (Percent <= 50.0f)
                    Tex = WeightTex[0];
                else if (Percent <= 66.6)
                    Tex = WeightTex[1];
                else if (Percent <= 80.0f)
                    Tex = WeightTex[2];
                else if (Percent <= 100.0f)
                    Tex = WeightTex[3];
                else
                    Tex = WeightTex[4];
                Device->DrawTexture(0x75, 0x174, BarLen, 0xC, 0, 0, BarLen, 0xC, Tex, 0xFF, 1);
            }
        }
        _swprintf(PercentText, TEXT("%.2f%s"), (DOUBLE)Percent, TEXT("%"));
        NCGDevice::GetTextExtent(&Size, PercentText, 0, 0, 0, 0, 0, L2FONT_Normal);
        Device->DrawSpecialDigit(1, 0x9D - Size.cx / 2, 0x176, 0xFFDCDCDC, PercentText);
        if (Me->bCanCrystallize)
        {
            if (!TrashButton->IsShowWindow())
                TrashButton->ShowWindow();
        }
        appSprintf(CountText, TEXT("(%d/%d)"), EquipWnd->Items.Num() + ItemWnd[1]->Items.Num() + ItemWnd[0]->Items.Num(),
                   (INT)::nw::Field<SWORD>(Me, 0x220));
    }
    NCGDevice::GetTextExtent(&Size, CountText, 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText((INT)((DOUBLE)Width - (DOUBLE)Size.cx - 5.0), 0xA2, 0xFFB09B79, CountText, 0, 0, 0, L2FONT_Normal, 0, 0,
                     0, 0, 0, 0, 0);
    if (CurOutlineTex && ManufactureButton->IsShowWindow())
        Device->DrawTexture(0x35, 0x15F, 0x22, 0x22, 0, 0, 0x22, 0x22, CurOutlineTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10089ea0, NCInventoryWnd, OnPaint)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        if (Child)
            delete Child;
        Child = NULL;
    }
}

// 0x1008d100 (vslot 38) - the page order is saved, then the two pages, the paperdoll, the tabs and the three buttons go
INT NCInventoryWnd::OnDestroy()
{
    guard(NCInventoryWnd::OnDestroy);
    SaveItemOrder();
    DestroyChild(this, ItemWnd[0]);
    DestroyChild(this, ItemWnd[1]);
    DestroyChild(this, EquipWnd);
    DestroyChild(this, TabCtrl);
    DestroyChild(this, ManufactureButton);
    DestroyChild(this, TrashButton);
    DestroyChild(this, RecipeButton);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008d100, NCInventoryWnd, OnDestroy)

// 0x1008d2e0 (vslot 44) - the current tab's page shows; the minimized icon (if any) goes back under the window and hides;
// the recipe icon shows with the crystallize flag, the manufacture button with the console's test; the saved order is read
void NCInventoryWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
    if (Icon)
    {
        INT X = Icon->ScreenX;
        INT Y = Icon->ScreenY;
        SetPosition(X, Y, (INT)Width, (INT)Height);
        Icon->SetPosition(0, 0, 0, 0);
        Icon->HideWindow();
    }
    if (GConsole->GetMyUser() && GConsole->GetMyUser()->bCanCrystallize)
        RecipeButton->ShowWindow();
    else
        RecipeButton->HideWindow();
    if (CanManufacture())
        ManufactureButton->ShowWindow();
    else
        ManufactureButton->HideWindow();
    ReadItemOrder();
}
NW_HOOK_METHOD(0x1008d2e0, NCInventoryWnd, ShowWindow)

// 0x1008a3b0 - an enchant scroll in use (the console): the item windows keep the cursor, which shows the icon of the
// item picked up (C3 no longer puts the arrow back)
void NCInventoryWnd::SetEnchantMode(UBOOL bOn)
{
    EquipWnd->SetEnchantMode(bOn);
    ItemWnd[0]->SetEnchantMode(bOn);
    if (bOn)
    {
        NCInvenItemWnd* Page = ItemWnd[0];
        if (Page && Page->Selected >= 0 && Page->Selected < Page->Items.Num())
        {
            SetCursor(4, Page->Items(Page->Selected)->GetIcon());
            LockCursor();
        }
    }
}
NW_HOOK_METHOD(0x1008a3b0, NCInventoryWnd, SetEnchantMode)

// 0x1008a420 - names and descriptions again from the item tables (the language changed), then the tool tips
void NCInventoryWnd::ReloadTexts()
{
    for (INT i = 0; i < EquipWnd->Items.Num(); i++)
    {
        NItemEntry* Item = EquipWnd->Items(i);
        if (!Item)
            continue;
        appStrcpy(Item->Name, GL2GameData.GetItemName(Item->ID));
        appStrcpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
        Item->FillToolTip(2, 0);
    }
    for (INT i = 0; i < ItemWnd[0]->Items.Num(); i++)
    {
        NItemEntry* Item = ItemWnd[0]->Items(i);
        if (!Item)
            continue;
        appStrcpy(Item->Name, GL2GameData.GetItemName(Item->ID));
        appStrcpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
        Item->FillToolTip(2, 0);
    }
    for (INT i = 0; i < ItemWnd[1]->Items.Num(); i++)
    {
        NItemEntry* Item = ItemWnd[1]->Items(i);
        if (!Item)
            continue;
        appStrcpy(Item->Name, GL2GameData.GetItemName(Item->ID));
        appStrcpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
        Item->FillToolTip(2, 0);
    }
}
NW_HOOK_METHOD(0x1008a420, NCInventoryWnd, ReloadTexts)

// 0x1008a5b0 - the quest page is not searched
NItemEntry* NCInventoryWnd::FindItem(INT ObjectID)
{
    INT Index;
    if (EquipWnd->FindItem(ObjectID, &Index))
        return EquipWnd->Items(Index);
    if (ItemWnd[0]->FindItem(ObjectID, &Index))
        return ItemWnd[0]->Items(Index);
    return NULL;
}
NW_HOOK_METHOD(0x1008a5b0, NCInventoryWnd, FindItem)

// 0x1008d2b0
void NCInventoryWnd::ClearItems()
{
    EquipWnd->Clear();
    ItemWnd[0]->Clear();
    ItemWnd[1]->Clear();
}
NW_HOOK_METHOD(0x1008d2b0, NCInventoryWnd, ClearItems)

// 0x1008ca60 - equipped items (NItemInfo::UnknownC8C) to the paperdoll, quest items (type 3) to the "Quest" page,
// the rest to the "Item" page
UBOOL NCInventoryWnd::AddItem(NItemInfo* Info)
{
    INT Page = Info->Param == 3;
    if (NItemInfoInt(Info, 0x1798))
    {
        EquipWnd->AddItem(Info);
        return 1;
    }
    if (ItemWnd[Page])
        ItemWnd[Page]->AddItem(Info);
    return 1;
}
NW_HOOK_METHOD(0x1008ca60, NCInventoryWnd, AddItem)

// 0x1008d3c0 - an item added (1), changed (2) or removed (3): an equipped item (NItemInfo::UnknownC8C) goes to the
// paperdoll, a quest item (type 3) to the "Quest" page, the rest to the "Item" page, where a new item shows first
// (MoveLastToFront); a changed item moves between the paperdoll and its page when it was put on or taken off. C2: the
// console calls it (C1 did it inline).
UBOOL NCInventoryWnd::UpdateItem(NItemInfo* Info, INT Change)
{
    INT Page = Info->Param == 3;
    if (Change == 1)
    {
        if (NItemInfoInt(Info, 0x1798))
        {
            EquipWnd->AddItem(Info);
            return 1;
        }
    }
    else if (Change == 2)
    {
        if (NItemInfoInt(Info, 0x1798))
        {
            INT ObjectID = Info->ObjectID;
            if (EquipWnd->HasItem(ObjectID))
            {
                EquipWnd->UpdateItem(Info);
                return 1;
            }
            ItemWnd[0]->RemoveItem(ObjectID);
            EquipWnd->AddItem(Info);
            return 1;
        }
        INT ObjectID = Info->ObjectID;
        NCInvenItemWnd* Wnd = ItemWnd[Page];
        if (Wnd->HasItem(ObjectID))
        {
            Wnd->UpdateItem(Info);
            return 1;
        }
        EquipWnd->RemoveItem(ObjectID);
    }
    else
    {
        if (Change == 3)
        {
            if (NItemInfoInt(Info, 0x1798))
            {
                if (EquipWnd)
                    EquipWnd->RemoveItem(Info->ObjectID);
            }
            else
                ItemWnd[Page]->RemoveItem(Info->ObjectID);
        }
        return 1;
    }
    ItemWnd[Page]->AddItem(Info);
    if (Page == 0)
        MoveLastToFront();
    return 1;
}
NW_HOOK_METHOD(0x1008d3c0, NCInventoryWnd, UpdateItem)

// 0x1008dc70 (vslot 37) - the textures, the small title bar (id 0xF2), the paperdoll (8, 31; 240 x 122), the two item
// pages (9, 189; 236 x 139: 6 x 4 icons of 32 pixels, 250 at most) and the two tabs, the manufacture, recipe and trash
// buttons
INT NCInventoryWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_Back"));
    WeightTex[0] = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.PS_weightbar1"));
    WeightTex[1] = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.PS_weightbar2"));
    WeightTex[2] = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.PS_weightbar3"));
    WeightTex[3] = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.PS_weightbar4"));
    WeightTex[4] = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.PS_weightbar5"));
    AdenaTex = FindTexture(TEXT("L2UI.WindowIcon.WindowIconAdena"));
    WeightIconTex = FindTexture(TEXT("L2UI.WindowIcon.Weight"));
    OutlineTex = FindTexture(TEXT("L2UI_CH3.Etc.menu_outline"));
    OutlineDownTex = FindTexture(TEXT("L2UI_CH3.Etc.menu_outline_down"));
    CurOutlineTex = OutlineTex;

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_INVENTORY_FRAME;
    FrameCtrl->SetPosition(0, 0, 0x100, 0x14);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    EquipWnd = new NCEquipItemWnd(0x20);
    EquipWnd->SetPosition(8, 0x1F, 0xF0, 0x7A);
    CreateChild(EquipWnd, NWS_VISIBLE | 0x20);

    ItemWnd[0] = new NCInvenItemWnd(6, 4, 0xFA, 0x20, 5, 3);
    ItemWnd[0]->SetPosition(9, 0xBD, 0xEC, 0x8B);
    CreateChild(ItemWnd[0], NWS_VISIBLE | 0x20);

    ItemWnd[1] = new NCInvenItemWnd(6, 4, 0xFA, 0x20, 5, 3);
    ItemWnd[1]->SetPosition(9, 0xBD, 0xEC, 0x8B);
    CreateChild(ItemWnd[1], NWS_VISIBLE | 0x20);

    TabCtrl = new NCTabControl();
    TabCtrl->ID = NID_INVENTORY_TAB;
    TabCtrl->SetPosition(0xC, 0x9F, 0xBD, 0x17);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(STR_Item, ItemWnd[0], 0x5E, 0x17, TEXT("L2UI_CH3.InventoryWnd.Inventory_tab2"),
                    TEXT("L2UI_CH3.InventoryWnd.Inventory_tab1"), 1, -1);
    TabCtrl->AddTab(STR_Quest, ItemWnd[1], 0x5E, 0x17, TEXT("L2UI_CH3.InventoryWnd.Inventory_tab2"),
                    TEXT("L2UI_CH3.InventoryWnd.Inventory_tab1"), 1, -1);

    ManufactureButton = new NCButton(TEXT("L2UI_CH3.InventoryWnd.Inventory_book"),
                                     TEXT("L2UI_CH3.InventoryWnd.Inventory_book_down"), 0);
    ManufactureButton->ID = NID_INVENTORY_MANUFACTURE;
    ManufactureButton->SetPosition(0x36, 0x160, 0x20, 0x20);
    CreateChild(ManufactureButton, NWS_VISIBLE | 0x20);
    ManufactureButton->SetToolTip(0x289, 1);

    RecipeTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
    RecipeDragTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe_drag"));
    TrashTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
    TrashDragTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash_drag"));

    RecipeButton = new NCButton(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"),
                                TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"), 0);
    RecipeButton->SetPosition(RecipeX, RecipeY, (DWORD)(INT)RecipeW, (DWORD)(INT)RecipeH);
    CreateChild(RecipeButton, NWS_VISIBLE | 0x20);
    RecipeButton->SetToolTip(0x37B, 1);

    TrashButton = new NCButton(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"),
                               TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"), 0);
    TrashButton->SetPosition(TrashX, TrashY, (DWORD)(INT)TrashW, (DWORD)(INT)TrashH);
    CreateChild(TrashButton, NWS_VISIBLE | 0x20);
    TrashButton->HideWindow();
    TrashButton->SetToolTip(0x37A, 1);
    return 1;
}
NW_HOOK_METHOD(0x1008dc70, NCInventoryWnd, OnCreate)

NW_CHECK_VTABLE(NCInventoryWnd, 0x1018a000, 69)
