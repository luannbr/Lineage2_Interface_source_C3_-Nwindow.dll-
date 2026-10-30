/*=============================================================================
    NCHennaInfoWnd.cpp - the confirmation of a tattoo. Stock: 0x10079170 .. 0x100791ee, 0x100791f0 .. 0x1007923a,
    0x10079240 .. 0x100793f9, 0x10079400 .. 0x10079507, 0x10079c60 .. 0x10079e9b, 0x10079ea0 .. 0x1007ab74,
    0x1007abb0 .. 0x1007ac43, 0x1007b230 .. 0x1007b37f.
=============================================================================*/
#include "nwindow/NCHennaInfoWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NGameData.h"
#include "nwindow/NHennaData.h"

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

// sysstring
enum
{
    STR_OK = 140,
    STR_Cancel = 543,
    STR_StatLabel0 = 104,       // the six stat labels (104 .. 109)
    STR_Price = 637,
    STR_Item = 638,
    STR_Symbol = 639,
    STR_Stats = 640,
    STR_AddSymbol = 651,        // the window's title by the mode
    STR_RemoveSymbol = 652,
};

// 0x100885f0 - the two item entries are constructed by their members; the stats are cleared, the rest is left as allocated
NCHennaInfoWnd::NCHennaInfoWnd()
{
    for (INT i = 0; i < 12; i++)
        Stats[i] = 0;
}
NW_HOOK_CTOR(0x100885f0, NCHennaInfoWnd)

// 0x100886d0
NW_HOOK_DTOR(0x100886d0, NCHennaInfoWnd)

// 0x10086420 (vslot 25) - OK and Cancel
INT NCHennaInfoWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_HENNAINFO_OK)
            return OnOK() != 0;
        if (wParam == NID_HENNAINFO_CANCEL)
            return OnCancel() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10086420, NCHennaInfoWnd, WindowProc)

// 0x10087ff0 (vslot 37) - C3: the title bar is as wide as the window, the two textures and the buttons (Cancel first)
INT NCHennaInfoWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_HENNAINFO_FRAME;
    FrameCtrl->SetPosition(0, 0, (INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI_ch3.HennaWnd.Henna2_back"));

    CancelButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    CancelButton->ID = NID_HENNAINFO_CANCEL;
    CancelButton->SetPosition(51, 372, 76, 23);
    CreateChild(CancelButton, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    CancelButton->SetTextID(STR_Cancel);

    OKButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    OKButton->ID = NID_HENNAINFO_OK;
    OKButton->SetPosition(131, 372, 76, 23);
    CreateChild(OKButton, NWS_VISIBLE | 0x20);
    OKButton->SetTextID(STR_OK);
    return 1;
}
NW_HOOK_METHOD(0x10087ff0, NCHennaInfoWnd, OnCreate)

// 0x10087cb0 (vslot 38) - the base first, then Cancel and OK
INT NCHennaInfoWnd::OnDestroy()
{
    guard(NCHennaInfoWnd::OnDestroy);
    NCWnd::OnDestroy();
    DestroyChild(this, CancelButton);
    DestroyChild(this, OKButton);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10087cb0, NCHennaInfoWnd, OnDestroy)

// 0x10086760 (vslot 44) - the title by the mode
void NCHennaInfoWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Mode == 0)
        SetTitle(*SysString(STR_AddSymbol));
    else if (Mode == 1)
        SetTitle(*SysString(STR_RemoveSymbol));
}
NW_HOOK_METHOD(0x10086760, NCHennaInfoWnd, ShowWindow)

// 0x100866b0 (vslot 68) - the tattoo is drawn (Mode 0) or taken off (1) and the window closes
INT NCHennaInfoWnd::OnOK()
{
    if (Mode == 0)
    {
        GConsole->RequestHennaEquip(Henna.ObjectID);
        HideWindow();
        return 1;
    }
    if (Mode == 1)
        GConsole->RequestHennaUnequip(Henna.ObjectID);
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100866b0, NCHennaInfoWnd, OnOK)

// 0x10086710 (vslot 69) - the window closes and the server is asked for the list again
INT NCHennaInfoWnd::OnCancel()
{
    HideWindow();
    if (Mode == 0)
    {
        GConsole->RequestHennaItemList();
        return 1;
    }
    if (Mode == 1)
        GConsole->RequestHennaUnequipList();
    return 1;
}
NW_HOOK_METHOD(0x10086710, NCHennaInfoWnd, OnCancel)

// 0x100864a0 (name ours) - the twelve stats zeroed
void NCHennaInfoWnd::ClearParams()
{
    for (INT i = 0; i < 12; i++)
        Stats[i] = 0;
}
NW_HOOK_METHOD(0x100864a0, NCHennaInfoWnd, ClearParams)

// 0x100864f0 (name ours) - the dye and the tattoo entries from the game data (nothing when either is not in it): the dye
// keeps the item name, its additional name as the description (as the stock code does), its icon, the count and the price
// of Param; the tattoo its name, description and icon, and its id in ObjectID; then the stats
void NCHennaInfoWnd::SetHenna(const NHennaParam* Param)
{
    NViewItemData* ItemData = (NViewItemData*)GL2GameData.GetItemData(Param->ItemID);
    FL2HennaData* HennaData = GL2GameData.GetHennaData(Param->HennaID);
    if (ItemData && HennaData)
    {
        appStrcpy(Item.Name, *ItemData->Name);
        appStrcpy(Item.Description, *ItemData->AdditionalName);
        appStrcpy(Item.IconName, *ItemData->IconName);
        Item.UnknownC20 = Param->Price;
        Item.Count = Param->Count;
        if (Item.IconName)
            Item.Icon = FindTexture(Item.IconName);
        if (!Item.Icon)
            Item.Icon = FindTexture(TEXT("NWindow.BlackTexture"));
        appStrcpy(Henna.Name, *HennaData->Name);
        appStrcpy(Henna.Description, *HennaData->Description);
        appStrcpy(Henna.IconName, *HennaData->IconName);
        Henna.ObjectID = Param->HennaID;
        if (Henna.IconName)
            Henna.Icon = FindTexture(Henna.IconName);
        if (!Henna.Icon)
            Henna.Icon = FindTexture(TEXT("NWindow.BlackTexture"));
        for (INT i = 0; i < 12; i++)
            Stats[i] = Param->Stats[i];
    }
}
NW_HOOK_METHOD(0x100864f0, NCHennaInfoWnd, SetHenna)

// "now > new" of a stat: the numbers centered on X1 and X2, the ">" at XSep
static FORCEINLINE void DrawStatPair(NCGDevice* Device, INT X1, INT XSep, INT X2, INT Y, INT Now, INT New)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, appItoa(Now), 0, 0, 0, 0, 0);
    Device->DrawText(X1 - Size.cx / 2, Y, 0xFFB09B79, appItoa(Now), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(XSep, Y, 0xFFB09B79, TEXT(">"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, appItoa(New), 0, 0, 0, 0, 0);
    Device->DrawText(X2 - Size.cx / 2, Y, 0xFFB09B79, appItoa(New), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
}

// 0x10086ec0 (vslot 56) - C3: the background under the title bar, the two items (the dye first when drawing, the
// tattoo first when taking off) with their descriptions (the dye's name wraps at the window's width less 60), the six
// stats "now > new" in two columns of three rows, and the adena. The texts are light grey (DCDCDC).
INT NCHennaInfoWnd::OnPaint(NCGDevice* Device)
{
    guard(NCHennaInfoWnd::OnPaint);
    TCHAR Text[260];
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 20, (INT)Width, (INT)(Height - 20.0f), 0, 0, (INT)Width, (INT)(Height - 20.0f), BackTex,
                            0xFF, 1);
    if (Mode == 0)
    {
        FString Desc;
        GConsole->ConvertNewLines(Item.Description, Desc);
        Device->DrawText(18, 43, 0xFFDCDCDC, *SysString(STR_Item), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        if (Item.Icon)
            Device->DrawTexture(16, 71, 32, 32, 0, 0, 32, 32, Item.Icon, 0xFF, 1);
        Device->DrawText(58, 66, 0xFFDCDCDC, Item.Name, (DWORD)(INT)(Width - 60.0f), 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                         0, 0, 0);
        Device->DrawText(58, 80, 0xFFDCDCDC, *Desc, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        appSprintf(Text, TEXT("%s %d"), *SysString(STR_Price), Item.UnknownC20);
        Device->DrawText(58, 94, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->DrawText(18, 132, 0xFFDCDCDC, *SysString(STR_Symbol), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        if (Henna.Icon)
            Device->DrawTexture(16, 161, 32, 32, 0, 0, 32, 32, Henna.Icon, 0xFF, 1);
        Device->DrawText(58, 156, 0xFFDCDCDC, Henna.Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        GConsole->ConvertNewLines(Henna.Description, Desc);
        Device->DrawText(58, 170, 0xFFDCDCDC, *Desc, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else if (Mode == 1)
    {
        FString Desc;
        Device->DrawText(18, 43, 0xFFDCDCDC, *SysString(STR_Symbol), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        if (Henna.Icon)
            Device->DrawTexture(16, 71, 32, 32, 0, 0, 32, 32, Henna.Icon, 0xFF, 1);
        appSprintf(Text, TEXT("%s %s"), *SysString(STR_RemoveSymbol), Henna.Name);
        Device->DrawText(58, 66, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        GConsole->ConvertNewLines(Henna.Description, Desc);
        appStrcpy(Text, *Desc);
        Device->DrawText(58, 80, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->DrawText(18, 132, 0xFFDCDCDC, *SysString(STR_Item), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        if (Item.Icon)
            Device->DrawTexture(16, 161, 32, 32, 0, 0, 32, 32, Item.Icon, 0xFF, 1);
        Device->DrawText(58, 156, 0xFFDCDCDC, Item.Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        GConsole->ConvertNewLines(Item.Description, Desc);
        Device->DrawText(58, 170, 0xFFDCDCDC, *Desc, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        appSprintf(Text, TEXT("%s %d"), *SysString(STR_Price), Item.UnknownC20);
        Device->DrawText(58, 184, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->DrawText(18, 221, 0xFFDCDCDC, *SysString(STR_Stats), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(17, 247, 0xFFA3A3A3, *SysString(STR_StatLabel0), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(17, 263, 0xFFA3A3A3, *SysString(STR_StatLabel0 + 1), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(17, 279, 0xFFA3A3A3, *SysString(STR_StatLabel0 + 2), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(135, 247, 0xFFA3A3A3, *SysString(STR_StatLabel0 + 3), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(135, 263, 0xFFA3A3A3, *SysString(STR_StatLabel0 + 4), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(135, 279, 0xFFA3A3A3, *SysString(STR_StatLabel0 + 5), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    // the left column: cells 1, 4 and 2; the right: 0, 5 and 3
    DrawStatPair(Device, 64, 78, 97, 247, Stats[2], Stats[3]);
    DrawStatPair(Device, 64, 78, 97, 263, Stats[8], Stats[9]);
    DrawStatPair(Device, 64, 78, 97, 279, Stats[4], Stats[5]);
    DrawStatPair(Device, 182, 196, 215, 247, Stats[0], Stats[1]);
    DrawStatPair(Device, 182, 196, 215, 263, Stats[10], Stats[11]);
    DrawStatPair(Device, 182, 196, 215, 279, Stats[6], Stats[7]);
    if (AdenaTex)
        Device->DrawTexture(52, 313, 16, 12, 0, 0, 16, 12, AdenaTex, 0xFF, 1);
    appSprintf(Text, TEXT("%d"), Adena);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Device->DrawText(229 - Size.cx, 329, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10086ec0, NCHennaInfoWnd, OnPaint)

NW_CHECK_VTABLE(NCHennaInfoWnd, 0x10189ad8, 70)
