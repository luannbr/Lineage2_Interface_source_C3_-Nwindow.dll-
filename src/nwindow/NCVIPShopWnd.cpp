/*=============================================================================
    NCVIPShopWnd.cpp - the VIP shop and its pages. C3 stock: 0x10093e90 .. 0x10094030 (list page, item page's
    shared bodies, ShowWindow), 0x10094030 .. 0x100956fd (DrawItemInfo), 0x10095720 .. 0x10096cd0 (the pages' paints, the
    needed items, the shop's), 0x10096d80 (NVIPShopRecord), 0x10096e10 (ShowItem), 0x100970d0 .. 0x10097b80
    (constructors, destructors, OnCreate). NCVIPShopItemListBox's functions are in NCVIPShopItemListBox.cpp.
=============================================================================*/
#include "nwindow/NCVIPShopWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleFns.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NGameData.h"
#include "nwindow/NItemToolTip.h"

// sysstring
enum
{
    STR_Store = 136,
    STR_OK = 140,
    STR_Cancel = 141,
    STR_SkillList = 364,
    STR_List = 562,
    STR_ItemInformation = 564,
    STR_ItemsRequired = 565,
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

/*-----------------------------------------------------------------------------
    NVIPShopRecord
-----------------------------------------------------------------------------*/

// 0x10096d80 - the entry's fields are cleared (NVIPShopEntry's constructor), then the needed items' array (built empty
// and emptied again: the stock constructor ends in FArray::Realloc)
NVIPShopRecord::NVIPShopRecord()
{
    NeededItems.Empty();
}
NW_HOOK_CTOR(0x10096d80, NVIPShopRecord)

/*-----------------------------------------------------------------------------
    NCVIPShopItemListWnd
-----------------------------------------------------------------------------*/

// 0x10097350 - nothing of its own (the list box and the texture are left as allocated)
NCVIPShopItemListWnd::NCVIPShopItemListWnd()
{
}
NW_HOOK_CTOR(0x10097350, NCVIPShopItemListWnd)

// 0x100973a0
NW_HOOK_DTOR(0x100973a0, NCVIPShopItemListWnd)

// 0x10093e90 (vslot 25) - C2: no IsEnabled test; the list box reports the clicked product
INT NCVIPShopItemListWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_VIPSHOP_LIST)
        return OnSelect((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10093e90, NCVIPShopItemListWnd, WindowProc)

// 0x10093ef0 - the list box emptied (a tail jump into its virtual Clear: the stock body is shared with every other
// window that keeps its list box at +0xC8)
void NCVIPShopItemListWnd::Clear()
{
    ListBox->Clear();
}
NW_HOOK_METHOD(0x10093ef0, NCVIPShopItemListWnd, Clear)

// 0x10095a80 (vslot 38) - the base first, then the list box
INT NCVIPShopItemListWnd::OnDestroy()
{
    guard(NCVIPShopItemListWnd::OnDestroy);
    NCWnd::OnDestroy();
    DestroyChild(this, ListBox);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10095a80, NCVIPShopItemListWnd, OnDestroy)

// 0x10095b30 (vslot 56) - C3: nothing is drawn but the clip (the list box paints the rows over the shop's own back)
INT NCVIPShopItemListWnd::OnPaint(NCGDevice* Device)
{
    guard(NCVIPShopItemListWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10095b30, NCVIPShopItemListWnd, OnPaint)

// 0x100965a0
void NCVIPShopItemListWnd::AddEntry(const NVIPShopEntry* Src)
{
    ListBox->AddEntry(Src);
}
NW_HOOK_METHOD(0x100965a0, NCVIPShopItemListWnd, AddEntry)

// 0x100973f0 (vslot 36) - 100 rows kept, 8 seen
INT NCVIPShopItemListWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.VipWnd.VipWndBack1"));
    ListBox = new NCVIPShopItemListBox(100, 8, 16);
    ListBox->ID = NID_VIPSHOP_LIST;
    ListBox->SetPosition(15, 29, 233, 314);
    CreateChild(ListBox, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100973f0, NCVIPShopItemListWnd, OnCreate)

// 0x100974c0 (vslot 68) - a row clicked: the shop (the parent) shows the item page of that product
INT NCVIPShopItemListWnd::OnSelect(INT Index)
{
    NCVIPShopWnd* Shop = (NCVIPShopWnd*)Entry->Parent->Wnd;
    Shop->Selected = Index;
    Shop->ShowItem();
    Shop->SetFocus();
    return 1;
}
NW_HOOK_METHOD(0x100974c0, NCVIPShopItemListWnd, OnSelect)

NW_CHECK_VTABLE(NCVIPShopItemListWnd, 0x1018c360, 69)

/*-----------------------------------------------------------------------------
    NCVIPShopItemInfoWnd
-----------------------------------------------------------------------------*/

// 0x100975c0 - the item entry is built by its member
NCVIPShopItemInfoWnd::NCVIPShopItemInfoWnd()
{
}
NW_HOOK_CTOR(0x100975c0, NCVIPShopItemInfoWnd)

// 0x10097640 - the entry's tool tip goes (the member's destructor), then the scroll window
NW_HOOK_DTOR(0x10097640, NCVIPShopItemInfoWnd)

// stock body shared with NCItemWnd's (0x10093f30, hooked there): the base's answer as a plain 0 / 1
INT NCVIPShopItemInfoWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCScrollWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10093f50 (vslot 37) - C3: the frame texture of the icon
INT NCVIPShopItemInfoWnd::OnCreate()
{
    NCScrollWnd::OnCreate();
    IconBoxTex = FindTexture(TEXT("L2ui_ch3.etc.iconbox"));
    return 1;
}
NW_HOOK_METHOD(0x10093f50, NCVIPShopItemInfoWnd, OnCreate)

// 0x10093f80 (vslot 37)
INT NCVIPShopItemInfoWnd::OnDestroy()
{
    guard(NCVIPShopItemInfoWnd::OnDestroy);
    NCScrollWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10093f80, NCVIPShopItemInfoWnd, OnDestroy)

// 0x10094000 (vslot 44) - shown at the top of the text (scroll position and bar reset)
void NCVIPShopItemInfoWnd::ShowWindow()
{
    ScrollX = 0;
    ScrollY = 0;
    ScrollBar->SetValue(0);
    NCWnd::ShowWindow();
}
NW_HOOK_METHOD(0x10094000, NCVIPShopItemInfoWnd, ShowWindow)

// 0x10095720 - a tail jump to NItemEntry::Reset
void NCVIPShopItemInfoWnd::Clear()
{
    Item.Reset();
}
NW_HOOK_METHOD(0x10095720, NCVIPShopItemInfoWnd, Clear)

// 0x10095e10 (vslot 56) - the item's texts, scrolled, from (1, 4); their height sizes the scroll range
INT NCVIPShopItemInfoWnd::OnPaint(NCGDevice* Device)
{
    guard(NCVIPShopItemInfoWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->PushOrigin(ScrollX, ScrollY);
    SetContentHeight(DrawItemInfo(Device, &Item, 1, 4));
    Device->PopOrigin();
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10095e10, NCVIPShopItemInfoWnd, OnPaint)

// 0x100967a0 - Src copied, then completed from the item table (nothing more when the item is not in it): name,
// additional name, icon, description, and by the item's class the weapon's, the armor's or the etc item's values; the
// type, body part and enchant stay Src's
void NCVIPShopItemInfoWnd::SetItem(NItemEntry* Src)
{
    Item.Copy(Src);
    NViewItemData* Data = GL2GameData.ItemTable->FindRef(Item.ID);
    if (Item.ID < 0 || !Data)
        return;
    INT Class = Data->Class;
    appStrcpy(Item.Name, GL2GameData.GetItemName(Item.ID));
    if (GConsole->GetItemAdditionalName(Item.ID))
        appStrcpy(Item.AdditionalName, GConsole->GetItemAdditionalName(Item.ID));
    appStrcpy(Item.IconName, *Data->IconName);
    appStrcpy(Item.Description, GL2GameData.GetItemDescription(Item.ID));
    Item.Type = Src->Type;
    Item.BodyPart = Src->BodyPart;
    Item.Enchant = Src->Enchant;
    if (Class == ITEMCLASS_Weapon)
    {
        Item.Weight = Data->Weight;
        Item.UnknownC2C = Data->Unknown48;
        Item.UnknownC30 = Data->WeaponType;
        Item.Unknown1C80 = Data->PAtk;
        Item.Unknown1C84 = Data->MAtk;
        Item.Unknown1C90 = Data->ShieldPDef;
        Item.Unknown1C94 = Data->ShieldRate;
        Item.Unknown1C58 = Data->Unknown40;
        Item.CrystalGrade = Data->CrystalGrade;
        Item.Unknown1C60 = Data->Unknown90;
        Item.Unknown1C64 = Data->UnknownA4;
        Item.Unknown1C68 = Data->UnknownA8;
        Item.Unknown1C6C = Data->AttackSpeed;
        Item.Unknown1C70 = Data->UnknownBC;
        Item.Unknown1C78 = Data->Evasion;
        Item.Unknown1CA0 = Data->SoulShots;
        Item.Unknown1CA4 = Data->SpiritShots;
    }
    else if (Class == ITEMCLASS_Armor)
    {
        Item.Weight = Data->Weight;
        Item.UnknownC2C = Data->Unknown48;
        Item.Unknown1C58 = Data->Unknown40;
        Item.CrystalGrade = Data->ArmorCrystalGrade;
        Item.SubType = Data->ArmorType;
        Item.Unknown1C78 = Data->ArmorEvasion;
        Item.Unknown1C88 = Data->PDef;
        Item.Unknown1C8C = Data->MDef;
        Item.Unknown1C9C = Data->MPBonus;
    }
    else if (Class == ITEMCLASS_Etc)
    {
        Item.Weight = Data->Weight;
        Item.UnknownC2C = Data->Unknown48;
        Item.Unknown1C58 = Data->Unknown40;
        Item.bStackable = Data->bStackable;
        if (Data->EtcType == 2)
            Item.Unknown1C98 = 1;
    }
}
NW_HOOK_METHOD(0x100967a0, NCVIPShopItemInfoWnd, SetItem)

// the label of a row of the item's information, in the stock's own steps (inline in each row): "label" grey at (X, Y), " : "
// white right after it; returns the X of the value (past "label : ")
static FORCEINLINE INT DrawStatLabel(NCGDevice* Device, INT X, INT Y, INT LabelID, TCHAR* Buf)
{
    SIZE Size;
    appSprintf(Buf, TEXT("%s"), *GL2GameData.SysString[LabelID]);
    Device->DrawText(X, Y, 0xFFA3A3A3, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    appSprintf(Buf, TEXT(" : "));
    Device->DrawText(Size.cx + X, Y, 0xFFDCDCDC, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Buf, TEXT("%s : "), *GL2GameData.SysString[LabelID]);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    return Size.cx + X;
}

// the value of a row, gold, at the X DrawStatLabel gave
static FORCEINLINE void DrawStatValue(NCGDevice* Device, INT X, INT Y, TCHAR* Buf)
{
    Device->DrawText(X, Y, 0xFFB09B79, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
}

// 0x10094030 - the item's information page from (X, Y): the icon in its frame, the name line (DrawShopItemName), then by the
// kind of the item (Type 0 weapon, 1 armor, 2 etc): the slot (and the weapon's kind) and its rows (P.Atk, M.Atk, speed,
// soulshots, spiritshots / P.Def with the enchant, M.Def, MP bonus / shield and evasion), the weight, and the description
// (wrapped at 200 pixels); returns the Y under the last line. Types 3 and 5 only have the slot, or the weight.
INT NCVIPShopItemInfoWnd::DrawItemInfo(NCGDevice* Device, NItemEntry* Entry, INT X, INT Y)
{
    TCHAR Buf[2046];
    SIZE CharSize;
    SIZE Size;
    if (!Entry)
        return Y;
    Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, IconBoxTex, 0xFF, 1);
    Device->DrawTexture(X, Y, 32, 32, 0, 0, 32, 32, Entry->Icon, 0xFF, 1);
    NCGDevice::GetCharSize(&CharSize, 'A', 0);
    INT NameX = X + 36;
    FString Desc;
    GConsole->ConvertNewLines((TCHAR*)Entry->GetDescription(), Desc);
    FString SlotText;
    GConsole->GetItemSlotText(Entry->Type, Entry->BodyPart, SlotText, Entry->SubType);
    INT MaxW = (INT)(Width - (FLOAT)NameX - 15.0f);
    DrawShopItemName(Device, NameX, Y + 4, MaxW, Entry->ID, Entry->GetName(), Entry->Type, Entry->Enchant,
                     Entry->CrystalGrade, Entry->Count);
    INT LineY = Y + 8 + CharSize.cy;
    INT LabelX = 0x24;
    INT NextY;
    INT VX;
    if (Entry->Type == 0)
    {
        appSprintf(Buf, TEXT("%s / %s"), GConsole->GetWeaponTypeName(Entry->UnknownC30), *SlotText);
        Device->DrawText(0x24, LineY, 0xFFB09B79, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NextY = LineY + 20;
        LabelX = X;
        VX = DrawStatLabel(Device, X, NextY, 94, Buf);
        appSprintf(Buf, TEXT("%d"), ((NConsoleLeftFns*)GConsole)->GetEnchantedPAtk(Entry->UnknownC30, Entry->BodyPart,
                                                           Entry->CrystalGrade, Entry->Enchant, Entry->Unknown1C80));
        DrawStatValue(Device, VX, NextY, Buf);
        LineY = LineY + 24 + CharSize.cy;
        VX = DrawStatLabel(Device, X, LineY, 98, Buf);
        appSprintf(Buf, TEXT("%d"), ((NConsoleLeftFns*)GConsole)->GetEnchantedMAtk(Entry->UnknownC30, Entry->BodyPart,
                                                           Entry->CrystalGrade, Entry->Enchant, Entry->Unknown1C84));
        DrawStatValue(Device, VX, LineY, Buf);
        LineY = LineY + 4 + CharSize.cy;
        VX = DrawStatLabel(Device, X, LineY, 111, Buf);
        appSprintf(Buf, TEXT("%s"), GConsole->GetAttackSpeedText(Entry->Unknown1C6C));
        DrawStatValue(Device, VX, LineY, Buf);
        LineY = LineY + 4 + CharSize.cy;
        VX = DrawStatLabel(Device, X, LineY, 404, Buf);
        appSprintf(Buf, TEXT("X %d"), Entry->Unknown1CA0);
        DrawStatValue(Device, VX, LineY, Buf);
        NextY = LineY + 4 + CharSize.cy;
        VX = DrawStatLabel(Device, X, NextY, 496, Buf);
        appSprintf(Buf, TEXT("X %d"), Entry->Unknown1CA4);
        DrawStatValue(Device, VX, NextY, Buf);
        NextY = NextY + 4 + CharSize.cy;
    }
    else if (Entry->Type == 1)
    {
        appSprintf(Buf, TEXT("%s"), *SlotText);
        Device->DrawText(0x24, LineY, 0xFFB09B79, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NextY = LineY + 20;
        LabelX = X;
        if ((Entry->BodyPart & 0x100) == 0 && (char)Entry->BodyPart >= 0)
        {
            if (!GConsole->IsMagicArmor(Entry->ID))
            {
                VX = DrawStatLabel(Device, X, NextY, 95, Buf);
                appSprintf(Buf, TEXT("%d"),
                           GConsole->GetEnchantedDefense(Entry->CrystalGrade, Entry->Enchant, Entry->Unknown1C88));
                DrawStatValue(Device, VX, NextY, Buf);
                NextY = NextY + 4 + CharSize.cy;
            }
            else
            {
                VX = DrawStatLabel(Device, X, NextY, 388, Buf);
                appSprintf(Buf, TEXT("%d"), Entry->Unknown1C9C);
                DrawStatValue(Device, VX, NextY, Buf);
                LineY = LineY + 24 + CharSize.cy;
                VX = DrawStatLabel(Device, X, LineY, 95, Buf);
                appSprintf(Buf, TEXT("%d"),
                           GConsole->GetEnchantedDefense(Entry->CrystalGrade, Entry->Enchant, Entry->Unknown1C88));
                DrawStatValue(Device, VX, LineY, Buf);
                NextY = LineY + 4 + CharSize.cy;
            }
        }
        else
        {
            VX = DrawStatLabel(Device, X, NextY, 95, Buf);
            appSprintf(Buf, TEXT("%d"),
                       GConsole->GetEnchantedDefense(Entry->CrystalGrade, Entry->Enchant, Entry->Unknown1C90));
            DrawStatValue(Device, VX, NextY, Buf);
            LineY = LineY + 24 + CharSize.cy;
            VX = DrawStatLabel(Device, X, LineY, 317, Buf);
            appSprintf(Buf, TEXT("%d"), Entry->Unknown1C94);
            DrawStatValue(Device, VX, LineY, Buf);
            LineY = LineY + 4 + CharSize.cy;
            VX = DrawStatLabel(Device, X, LineY, 97, Buf);
            appSprintf(Buf, TEXT("%d"), Entry->Unknown1C78);
            DrawStatValue(Device, VX, LineY, Buf);
            NextY = LineY + 4 + CharSize.cy;
        }
    }
    else
    {
        NextY = LineY;
        if (Entry->Type == 2)
        {
            appSprintf(Buf, TEXT("%s"), *SlotText);
            Device->DrawText(0x24, LineY, 0xFFB09B79, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            INT RowY = LineY + 20;
            LabelX = X;
            VX = DrawStatLabel(Device, X, RowY, 99, Buf);
            appSprintf(Buf, TEXT("%d"),
                       GConsole->GetEnchantedDefense(Entry->CrystalGrade, Entry->Enchant, Entry->Unknown1C8C));
            DrawStatValue(Device, VX, RowY, Buf);
            NextY = LineY + 24 + CharSize.cy;
        }
    }
    switch (Entry->Type)
    {
    case 0:
    case 1:
    case 2:
        VX = DrawStatLabel(Device, LabelX, NextY, 52, Buf);
        appSprintf(Buf, TEXT("%d"), Entry->Weight);
        DrawStatValue(Device, VX, NextY, Buf);
        appSprintf(Buf, TEXT("%s"), *Desc);
        if (Buf[0] == 0)
            return NextY;
        NextY = NextY + 4 + CharSize.cy;
        Device->DrawText(LabelX, NextY, 0xFFB2BECF, Buf, 200, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, Buf, 200, 0, 0, 0, 0);
        NextY = NextY + Size.cy;
        return NextY;
    case 3:
        appSprintf(Buf, TEXT("%s"), *SlotText);
        Device->DrawText(LabelX, NextY, 0xFFB09B79, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        appSprintf(Buf, TEXT("%s"), *Desc);
        if (Buf[0] == 0)
            return NextY;
        NextY = NextY + 4 + CharSize.cy;
        Device->DrawText(X, NextY, 0xFFB2BECF, Buf, 200, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, Buf, 200, 0, 0, 0, 0);
        NextY = NextY + Size.cy;
        return NextY;
    case 5:
        VX = DrawStatLabel(Device, LabelX, NextY, 52, Buf);
        appSprintf(Buf, TEXT("%d"), Entry->Weight);
        DrawStatValue(Device, VX, NextY, Buf);
        break;
    }
    appSprintf(Buf, TEXT("%s"), *Desc);
    if (Buf[0] != 0)
    {
        NextY = NextY + 4 + CharSize.cy;
        Device->DrawText(X, NextY, 0xFFB2BECF, Buf, 200, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, Buf, 200, 0, 0, 0, 0);
        NextY = NextY + Size.cy;
    }
    return NextY;
}
NW_HOOK_METHOD(0x10094030, NCVIPShopItemInfoWnd, DrawItemInfo)

NW_CHECK_VTABLE(NCVIPShopItemInfoWnd, 0x1018c5c8, 72)

/*-----------------------------------------------------------------------------
    NCVIPShopNeededItemWnd
-----------------------------------------------------------------------------*/

// 0x10097500 - the items' array is empty
NCVIPShopNeededItemWnd::NCVIPShopNeededItemWnd()
{
}
NW_HOOK_CTOR(0x10097500, NCVIPShopNeededItemWnd)

// 0x10097560 - Items' destructor is TArray<T*>'s (0x1006e700)
NW_HOOK_DTOR(0x10097560, NCVIPShopNeededItemWnd)

// stock body shared with NCItemWnd's (0x10093f30, hooked there): the base's answer as a plain 0 / 1
INT NCVIPShopNeededItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCScrollWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10093f00 (vslot 37) - C3: the frame texture of the icons
INT NCVIPShopNeededItemWnd::OnCreate()
{
    NCScrollWnd::OnCreate();
    IconBoxTex = FindTexture(TEXT("L2ui_ch3.etc.iconbox"));
    return 1;
}
NW_HOOK_METHOD(0x10093f00, NCVIPShopNeededItemWnd, OnCreate)

// 0x10095bf0 (vslot 56) - a row of 36 pixels per item: the icon in its frame, the name line and "x count"
INT NCVIPShopNeededItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCVIPShopneededItemWnd::OnPaint);
    TCHAR Count[2048];
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (Items.Num() > 0)
    {
        Device->PushOrigin(ScrollX, ScrollY);
        INT i;
        for (i = 0; i < Items.Num(); i++)
        {
            INT Y = i * 36;
            Device->DrawTexture(0, Y + 5, 34, 34, 0, 0, 34, 34, IconBoxTex, 0xFF, 1);
            Device->DrawTexture(1, Y + 6, 32, 32, 0, 0, 32, 32, Items(i)->Icon, 0xFF, 1);
            DrawShopItemName(Device, 36, Y + 10, (INT)(Width - 36.0f - 15.0f), Items(i)->ItemID, Items(i)->Name,
                             Items(i)->Type, Items(i)->Enchant, Items(i)->Grade, 0);
            appSprintf(Count, TEXT("x %d"), Items(i)->Count);
            Device->DrawText(36, Y + 25, 0xFFDCDCDC, Count, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
        SetContentHeight(i * 36 + 5);
        Device->PopOrigin();
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10095bf0, NCVIPShopNeededItemWnd, OnPaint)

// 0x100965b0 (vslot 38) - the loop runs while the array is not empty, which it never changes:
// with items left it frees on past the end of the array (Clear, which the shop calls before
// each page, leaves it empty)
INT NCVIPShopNeededItemWnd::OnDestroy()
{
    guard(NCVIPShopNeededItemWnd::OnDestroy);
    for (INT i = 0; Items.Num(); i++)
    {
        if (Items(i))
            appFree(Items(i));
    }
    Items.Empty();
    NCScrollWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100965b0, NCVIPShopNeededItemWnd, OnDestroy)

// 0x10096680 - keeps a copy of Src without its icon name (the icon itself and the C3 values are copied); a failed
// allocation is not checked
void NCVIPShopNeededItemWnd::AddItem(const NVIPNeededItem* Src)
{
    NVIPNeededItem* Item = NULL;
    NVIPNeededItem* Mem = (NVIPNeededItem*)appMalloc(sizeof(NVIPNeededItem), TEXT("new"));
    if (Mem)
        Item = new ((void*)Mem) NVIPNeededItem;
    Item->ItemID = Src->ItemID;
    appStrcpy(Item->Name, Src->Name);
    Item->Icon = Src->Icon;
    Item->Count = Src->Count;
    Item->Enchant = Src->Enchant;
    Item->Grade = Src->Grade;
    Item->Type = Src->Type;
    INT Index = Items.Add();
    Items(Index) = Item;
}
NW_HOOK_METHOD(0x10096680, NCVIPShopNeededItemWnd, AddItem)

// 0x10096740
void NCVIPShopNeededItemWnd::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (Items(i))
            appFree(Items(i));
    }
    Items.Empty();
    UnknownDC = 0;
    UnknownE0 = 0;
}
NW_HOOK_METHOD(0x10096740, NCVIPShopNeededItemWnd, Clear)

NW_CHECK_VTABLE(NCVIPShopNeededItemWnd, 0x1018c4a8, 72)

/*-----------------------------------------------------------------------------
    NCVIPShopWnd
-----------------------------------------------------------------------------*/

// 0x100976a0 - the records' array is empty
NCVIPShopWnd::NCVIPShopWnd()
{
}
NW_HOOK_CTOR(0x100976a0, NCVIPShopWnd)

// 0x10097700 - Records' destructor is TArray<T*>'s (0x1006e700): the records are not freed
NW_HOOK_DTOR(0x10097700, NCVIPShopWnd)

// 0x10095730 (vslot 25) - C2: no IsEnabled test
INT NCVIPShopWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_VIPSHOP_OK)
            return OnOK() != 0;
        if (wParam == NID_VIPSHOP_CANCEL)
            return OnCancel() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10095730, NCVIPShopWnd, WindowProc)

// 0x10095f10 (vslot 38) - the base first, then the pages and the buttons
INT NCVIPShopWnd::OnDestroy()
{
    guard(NCVIPShopWnd::OnDestroy);
    NCWnd::OnDestroy();
    DestroyChild(this, ListWnd);
    DestroyChild(this, InfoWnd);
    DestroyChild(this, NeededWnd);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10095f10, NCVIPShopWnd, OnDestroy)

// 0x10096060 (vslot 56; the guard string has the original's typo) - the item page's background (the window's size
// less the title bar's 20 pixels) and its two headings; the list page has its own background and its heading
INT NCVIPShopWnd::OnPaint(NCGDevice* Device)
{
    guard(NCVIPShopWnd::Onpaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (Page == VIPSHOPPAGE_Item)
    {
        INT H = (INT)(Height - 20.0f);
        INT W = (INT)Width;
        Device->DrawTexture(0, 20, W, H, 0, 0, W, H, BackTex, 0xFF, 1);
        Device->DrawText(11, 32, 0xFFDCDCDC, *SysString(STR_ItemInformation), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->DrawText(11, 203, 0xFFDCDCDC, *SysString(STR_ItemsRequired), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else
    {
        INT H = (INT)(Height - 20.0f);
        INT W = (INT)Width;
        Device->DrawTexture(0, 20, W, H, 0, 0, W, H, ListBackTex, 0xFF, 1);
        Device->DrawText(11, 32, 0xFFDCDCDC, *SysString(STR_List), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10096060, NCVIPShopWnd, OnPaint)

// 0x10096240 (vslot 68) - C3: OK asks first ("Do you wish to buy it?", system message 0x567): the dialog carries the list
// id and the product id; the window hides itself
INT NCVIPShopWnd::OnOK()
{
    NDialogInfo Info;
    Info.bInput = 0;
    Info.Owner = NULL;
    Info.ID = 0x51;
    Info.bMainBox = 1;
    Info.SetParams(ListID, Records(Selected)->ProductID);
    Info.SetSystemMessage(0x567);
    GConsole->ShowDialog(&Info);
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10096240, NCVIPShopWnd, OnOK)

// 0x10096a20 - the list page: a row per product
void NCVIPShopWnd::ShowList()
{
    ListWnd->Clear();
    InfoWnd->Clear();
    NeededWnd->Clear();
    for (INT i = 0; i < Records.Num(); i++)
    {
        NVIPShopEntry Row = *Records(i);
        ListWnd->AddEntry(&Row);
    }
    InfoWnd->HideWindow();
    NeededWnd->HideWindow();
    ListWnd->ShowWindow();
    OKButton->HideWindow();
    CancelButton->HideWindow();
    Page = VIPSHOPPAGE_List;
}
NW_HOOK_METHOD(0x10096a20, NCVIPShopWnd, ShowList)

// 0x10096e10 (name ours) - the item page of the selected product: the pages emptied, the product's needed items copied
// into the needed items' page, its item (completed from the item table) into the information page; the list page
// hides, the others show
void NCVIPShopWnd::ShowItem()
{
    ListWnd->Clear();
    InfoWnd->Clear();
    NeededWnd->Clear();
    if (Selected >= 0)
    {
        for (INT i = 0; i < Records(Selected)->NeededItems.Num(); i++)
        {
            NVIPNeededItem Needed;
            appStrcpy(Needed.Name, Records(Selected)->NeededItems(i)->Name);
            Needed.ItemID = Records(Selected)->NeededItems(i)->ItemID;
            Needed.Count = Records(Selected)->NeededItems(i)->Count;
            Needed.Icon = Records(Selected)->NeededItems(i)->Icon;
            Needed.Enchant = Records(Selected)->NeededItems(i)->Enchant;
            Needed.Grade = Records(Selected)->NeededItems(i)->Grade;
            Needed.Type = Records(Selected)->NeededItems(i)->Type;
            NeededWnd->AddItem(&Needed);
        }
        NItemEntry Entry;
        Entry.ID = Records(Selected)->ItemID;
        Entry.Type = Records(Selected)->Type;
        Entry.BodyPart = Records(Selected)->BodyPart;
        Entry.Icon = Records(Selected)->Icon;
        Entry.Count = Records(Selected)->Count;
        Entry.Enchant = Records(Selected)->Enchant;
        Entry.CrystalGrade = Records(Selected)->Grade;
        InfoWnd->SetItem(&Entry);
        ListWnd->HideWindow();
        InfoWnd->ShowWindow();
        NeededWnd->ShowWindow();
        OKButton->ShowWindow();
        CancelButton->ShowWindow();
        Page = VIPSHOPPAGE_Item;
    }
}
NW_HOOK_METHOD(0x10096e10, NCVIPShopWnd, ShowItem)

// 0x10096af0 - everything emptied; the records are dropped, not freed
void NCVIPShopWnd::Reset()
{
    Selected = -1;
    ListID = 0;
    Records.Empty();
    ListWnd->Clear();
    InfoWnd->Clear();
    NeededWnd->Clear();
}
NW_HOOK_METHOD(0x10096af0, NCVIPShopWnd, Reset)

// 0x10096b40 - a copy of Src to every product with that id; the icon found by its name and, from the item table, the
// item's type and crystal grade (the weapon's, the armor's or the etc item's). An item not in the table ends the
// function (the copy already made is not kept)
void NCVIPShopWnd::AddNeededItem(INT ProductID, const NVIPNeededItem* Src)
{
    for (INT i = 0; i < Records.Num(); i++)
    {
        NVIPShopRecord* Record = Records(i);
        if (Record->ProductID == ProductID)
        {
            NVIPNeededItem* Item = NULL;
            NVIPNeededItem* Mem = (NVIPNeededItem*)appMalloc(sizeof(NVIPNeededItem), TEXT("new"));
            if (Mem)
                Item = new ((void*)Mem) NVIPNeededItem;
            Item->ItemID = Src->ItemID;
            appStrcpy(Item->Name, Src->Name);
            Item->Count = Src->Count;
            Item->Icon = FindTexture(Src->IconName);
            Item->Enchant = Src->Enchant;
            NViewItemData* Data = GL2GameData.ItemTable->FindRef(Src->ItemID);
            if (Src->ItemID < 0)
                return;
            if (!Data)
                return;
            INT Class = Data->Class;
            Item->Type = Class;
            if (Class == ITEMCLASS_Weapon)
                Item->Grade = Data->CrystalGrade;
            else if (Class == ITEMCLASS_Armor)
                Item->Grade = Data->ArmorCrystalGrade;
            else if (Class == ITEMCLASS_Etc)
                Item->Grade = Data->Unknown8C;
            else
                Item->Grade = 0;
            Record->NeededItems.AddItem(Item);
        }
    }
}
NW_HOOK_METHOD(0x10096b40, NCVIPShopWnd, AddNeededItem)

// 0x10096cb0 (vslot 69) - back to the list page, focused
INT NCVIPShopWnd::OnCancel()
{
    ShowList();
    SetFocus();
    return 1;
}
NW_HOOK_METHOD(0x10096cb0, NCVIPShopWnd, OnCancel)

// 0x10096cd0
void NCVIPShopWnd::Open(INT InListID)
{
    Reset();
    ListID = InListID;
}
NW_HOOK_METHOD(0x10096cd0, NCVIPShopWnd, Open)

// 0x10097760 (vslot 37) - C3: the title bar ("Store") as wide as the window, the list page under it and the item page's
// two pages (information, needed items) at their places, the two buttons; the two backgrounds
INT NCVIPShopWnd::OnCreate()
{
    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameID = NID_VIPSHOP_FRAME;
    FrameCtrl->ID = NID_VIPSHOP_FRAME;
    FrameCtrl->SetPosition(0, 0, (INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    FrameCtrl->SetTitleText(*SysString(STR_Store));

    ListWnd = new NCVIPShopItemListWnd;
    ListWnd->SetPosition(0, 20, (INT)Width, (INT)(Height - 20.0f));
    CreateChild(ListWnd, NWS_VISIBLE);

    InfoWnd = new NCVIPShopItemInfoWnd;
    InfoWnd->SetPosition(15, 49, 233, 144);
    CreateChild(InfoWnd, NWS_VISIBLE);

    NeededWnd = new NCVIPShopNeededItemWnd;
    NeededWnd->SetPosition(15, 221, 233, 142);
    CreateChild(NeededWnd, NWS_VISIBLE);

    OKButton = new NCButton(TEXT("L2UI_CH3.Button.btn1_normal"), TEXT("L2UI_CH3.Button.btn1_normalon"), 0);
    OKButton->ID = NID_VIPSHOP_OK;
    OKButton->SetPosition(51, 372, 76, 23);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(STR_OK);

    CancelButton = new NCButton(TEXT("L2UI_CH3.Button.btn1_normal"), TEXT("L2UI_CH3.Button.btn1_normalon"), 0);
    CancelButton->ID = NID_VIPSHOP_CANCEL;
    CancelButton->SetPosition(131, 372, 76, 23);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(STR_Cancel);

    ListBackTex = FindTexture(TEXT("L2UI_CH3.StoreWnd.store2_back"));
    BackTex = FindTexture(TEXT("L2UI_CH3.StoreWnd.store3_back"));
    return 1;
}
NW_HOOK_METHOD(0x10097760, NCVIPShopWnd, OnCreate)

// 0x100970d0 (vslot 45) - hidden, the window is emptied
void NCVIPShopWnd::HideWindow()
{
    NCWnd::HideWindow();
    Reset();
}
NW_HOOK_METHOD(0x100970d0, NCVIPShopWnd, HideWindow)

// 0x100970e0: the body is NCVIPShopWndExt::AddRecord (NConsoleWndMisc.cpp, hooked there); the standalone's is an alias of it
#if NW_HOOK_MODE
NW_NAKED void NCVIPShopWnd::AddRecord(const NVIPShopEntry* Src) NW_STOCK(0x100970e0)
#else
NW_SLOT_IS("?AddRecord@NCVIPShopWnd@@QAEXPBUNVIPShopEntry@@@Z", "?AddRecord@NCVIPShopWndExt@@QAEXPBUNVIPShopEntry@@@Z")   // NCVIPShopWnd::AddRecord = NCVIPShopWndExt::AddRecord (0x100970e0)
#endif

NW_CHECK_VTABLE(NCVIPShopWnd, 0x1018c6e8, 70)
