/*=============================================================================
    NCTrainWnd.cpp - the skill trainer window and its two pages. Stock: 0x100baae0 ..
    0x100bb4bf, 0x100bb4f0 .. 0x100bb53f, 0x100bb580 .. 0x100bc50a, 0x100bc540 .. 0x100bc54a,
    0x100bc7a0 .. 0x100bca96, 0x100bcc50 .. 0x100bcd19, 0x10089310 .. 0x1008937e and
    NTrainSkillInfo's 0x10047260 .. 0x100472de, 0x1004ddf0 .. 0x1004de67.
=============================================================================*/
#include "nwindow/NCTrainWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCTrainListBox.h"
#include "nwindow/NConsoleWnd.h"

#include <wchar.h>

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

// the player's SP at the bottom right of both pages (repeated in the stock OnPaints)
#if 0  // C2: does not compile against the C2 headers (db/demote.csv)
static FORCEINLINE void DrawPlayerSP(NCGDevice* Device)
{
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        TCHAR Text[1024];
        SIZE Size;
        appSprintf(Text, TEXT("%d"), Me->SP);
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
        Device->DrawText(220 - Size.cx, 290, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                         0, 0);
    }
}
#endif

/*-----------------------------------------------------------------------------
    NTrainSkillInfo
-----------------------------------------------------------------------------*/

// 0x100627a0 - the texts empty, no icon and no item rows; the skill, its level and the unknown +0x4A8 and +0x4BC -1,
// the numbers 0
NTrainSkillInfo::NTrainSkillInfo()
{
    Name[0] = 0;
    IconName[0] = 0;
    Unknown20C[0] = 0;
    Icon = NULL;
    Items[0] = NULL;
    Items[1] = NULL;
    Items[2] = NULL;
    Items[3] = NULL;
    SkillID = 0;
    SP = 0;
    MpCost = 0;
    Range = 0;
    Unknown2B8 = 0;
    SkillID = -1;
    Level = -1;
    Unknown2A8 = -1;
    Unknown2BC = -1;
}
NW_HOOK_CTOR(0x100627a0, NTrainSkillInfo)

// 0x100778e0 - the item rows go (each one's tool tip object, then the block), then the description
NTrainSkillInfo::~NTrainSkillInfo()
{
    for (INT i = 0; i < 4; i++)
    {
        NItemEntry* Item = Items[i];
        if (Item)
        {
            Item->ToolTip.~NToolTipInfo();   // 0x100025a0 (out of line: the stock calls it)
            appFree(Item);
            Items[i] = NULL;
        }
    }
}
NW_HOOK_DTOR(0x100778e0, NTrainSkillInfo)

/*-----------------------------------------------------------------------------
    NCTrainListWnd
-----------------------------------------------------------------------------*/

// 0x10109300
NCTrainListWnd::NCTrainListWnd()
{
    BackTex = NULL;
    ListBox = NULL;
}
NW_HOOK_CTOR(0x10109300, NCTrainListWnd)

// 0x10109350
NW_HOOK_DTOR(0x10109350, NCTrainListWnd)

// 0x10107490 (vslot 25) - C2 has no IsEnabled check of its own
INT NCTrainListWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_TRAIN_LISTBOX)
        return OnListCommand(lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10107490, NCTrainListWnd, WindowProc)

// 0x101093a0 (vslot 35) - 7 rows seen, 100 at most
INT NCTrainListWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.SkillTrainWnd.SkillTrain1"));
    ListBox = new NCTrainListBox(100, 7, 16);
    ListBox->ID = NID_TRAIN_LISTBOX;
    ListBox->SetPosition(20, 27, 230, 274);
    CreateChild(ListBox, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x101093a0, NCTrainListWnd, OnCreate)

// 0x100eb490 (vslot 38): NCSSQMainEventWnd's body, shared (hooked there; the standalone's slot is an alias of it)
#if NW_HOOK_MODE
NW_NAKED INT NCTrainListWnd::OnDestroy() NW_STOCK(0x100eb490)
#else
NW_SLOT_IS("?OnDestroy@NCTrainListWnd@@UAEHXZ", "?OnDestroy@NCSSQMainEventWnd@@UAEHXZ")   // NCTrainListWnd::OnDestroy = NCSSQMainEventWnd::OnDestroy (0x100eb490)
#endif

// the player's SP at the bottom of both pages (repeated in the stock OnPaints): the number right-aligned at 224 and
// "SP" (sysstring 92) at 130
static FORCEINLINE void DrawPlayerSP(NCGDevice* Device)
{
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        TCHAR Text[128];
        SIZE Size;
        appSprintf(Text, TEXT("%d"), Me->SP);
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
        Device->DrawText(0xE0 - Size.cx, 0x146, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->DrawText(0x82, 0x146, 0xFFDCDCDC, *GL2GameData.SysString[92], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
}

// 0x10107ef0 (vslot 56) - C3: the background (256 x 381), "Skill list" (sysstring 364) and the SP
INT NCTrainListWnd::OnPaint(NCGDevice* Device)
{
    guard(NCTrainListWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 0, 256, 381, 0, 0, 256, 381, BackTex, 0xFF, 1);
    if (*GL2GameData.SysString[364])
        Device->DrawText(11, 12, 0xFFDCDCDC, *GL2GameData.SysString[364], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    DrawPlayerSP(Device);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10107ef0, NCTrainListWnd, OnPaint)

// 0x101080d0 (vslot 62) - the selected row's skill and level to the server; lParam (the row)
// is not used
INT NCTrainListWnd::OnListCommand(LPARAM lParam)
{
    NTrainEntry Entry;
    if (ListBox->GetSelectedEntry(&Entry))
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Entry.SkillID);
        Params.PushBack((void*)(INT_PTR)Entry.Level);
        ::nw::VirtualCall<void>(GConsole, 400, &Params);   // RequestAcquireSkillInfo (NConsoleWnd slot 400)
    }
    return 1;
}
NW_HOOK_METHOD(0x101080d0, NCTrainListWnd, OnListCommand)

// 0x100b2b90 - the list box's Clear (vslot 70) as a tail jump; the stock body is shared with
// NCPetInventoryWnd::ClearItems (hooked there). Out of line: NCTrainWnd::Reset calls it
__declspec(noinline) void NCTrainListWnd::Clear()
{
    ListBox->Clear();
}

// 0x10108f10
void NCTrainListWnd::AddEntry(const NTrainEntry* Entry)
{
    ListBox->AddEntry(Entry);
}
NW_HOOK_METHOD(0x10108f10, NCTrainListWnd, AddEntry)

NW_CHECK_VTABLE(NCTrainListWnd, 0x101a0e40, 69)

/*-----------------------------------------------------------------------------
    NCTrainInfoWnd
-----------------------------------------------------------------------------*/

// 0x10109470 - NCWnd's, the skill record's constructor (the member), no background, no buttons
NCTrainInfoWnd::NCTrainInfoWnd()
{
    BackTex = NULL;
    LearnButton = NULL;
    ListButton = NULL;
}
NW_HOOK_CTOR(0x10109470, NCTrainInfoWnd)

// 0x10109500
NW_HOOK_DTOR(0x10109500, NCTrainInfoWnd)

// 0x10107500 (vslot 25) - C2 has no IsEnabled check of its own
INT NCTrainInfoWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_TRAIN_LEARN)
            return OnLearn() != 0;
        if (wParam == NID_TRAIN_LIST)
            return OnList() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10107500, NCTrainInfoWnd, WindowProc)

// a button of the page (repeated inline in the stock OnCreate)
#if 0  // C2: does not compile against the C2 headers (db/demote.csv)
static FORCEINLINE NCButton* CreatePageButton(NCWnd* Parent, INT ID, INT X, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI.control.DefaultButton"),
                                    TEXT("L2UI.control.DefaultButton_click"), 0);
    Button->ID = ID;
    Button->SetPosition(X, 313, 67, 19);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetTextID(TextID);
    return Button;
}
#endif

// 0x10108f20 (vslot 37) - C3: the background, the four rows of required items (item entries made empty), the Learn
// (id 2) and List (id 3) buttons under them and the two outlines of the skill's icon
INT NCTrainInfoWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.SkillTrainWnd.SkillTrain2"));
    for (INT i = 0; i < 4; i++)
        Info.Items[i] = new NItemEntry();
    LearnButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    LearnButton->ID = NID_TRAIN_LEARN;
    LearnButton->SetPosition(0x33, 0x160, 0x4C, 0x17);
    CreateChild(LearnButton, NWS_VISIBLE);
    LearnButton->SetTextID(0x170);
    ListButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    ListButton->ID = NID_TRAIN_LIST;
    ListButton->SetPosition(0x83, 0x160, 0x4C, 0x17);
    CreateChild(ListButton, NWS_VISIBLE);
    ListButton->SetTextID(0x171);
    OutLineDownTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine_down"));
    OutLineTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine"));
    return 1;
}
NW_HOOK_METHOD(0x10108f20, NCTrainInfoWnd, OnCreate)

// 0x101081d0 (vslot 38) - NCWnd's comes first here; the item entries stay (Info's destructor)
INT NCTrainInfoWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    DestroyChild(this, LearnButton);
    DestroyChild(this, ListButton);
    return 1;
}
NW_HOOK_METHOD(0x101081d0, NCTrainInfoWnd, OnDestroy)

// "label : " (the label a sysstring, empty when missing) in grey, the colon in white; returns
// the X of the value, past "label : " (repeated inline for each such line of the stock code)
#if 0  // C2: does not compile against the C2 headers (db/demote.csv)
static FORCEINLINE INT DrawLabel(NCGDevice* Device, INT X, INT Y, TCHAR* Text, INT LabelID)
{
    SIZE Size;
    if (*NCObject::SysString(LabelID))
        appSprintf(Text, TEXT("%s"), *NCObject::SysString(LabelID));
    else
        Text[0] = 0;
    Device->DrawText(X, Y, 0xFFA3A3A3, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT(" : "));
    Device->DrawText(X + Size.cx, Y, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (*NCObject::SysString(LabelID))
        appSprintf(Text, TEXT("%s : "), *NCObject::SysString(LabelID));
    else
        Text[0] = 0;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    return X + Size.cx;
}
#endif

// "label" in grey, then " : " in light grey, the value after "label : " (the label is a sysstring, empty when it
// is missing; a line of the stock paint: the same spelled out for the MP, the range and the SP); returns the X where
// the value goes
static FORCEINLINE INT DrawLabeledLine(NCGDevice* Device, INT Y, INT LabelID)
{
    TCHAR Text[32];
    SIZE Size;
    if (*GL2GameData.SysString[LabelID])
        appSprintf(Text, TEXT("%s"), *GL2GameData.SysString[LabelID]);
    else
        Text[0] = 0;
    Device->DrawText(0xD, Y, 0xFFA3A3A3, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT(" : "));
    Device->DrawText(Size.cx + 0xD, Y, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (*GL2GameData.SysString[LabelID])
        appSprintf(Text, TEXT("%s : "), *GL2GameData.SysString[LabelID]);
    else
        Text[0] = 0;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    return Size.cx + 0xD;
}

// 0x10108240 (vslot 56) - C3: the background (256 x 381), "Skill explanation" (366) and "Learning condition" (367),
// the skill's icon in its two outlines, its name, "Lv" and the level (and the text after it), then MP cost and range
// (when it has one), the description (wrapped at 230), "SP" and the required SP, the required items (an icon and
// "name X count" in 140 pixels, 34 apart) and the player's SP
INT NCTrainInfoWnd::OnPaint(NCGDevice* Device)
{
    guard(NCTrainInfoWnd::OnPaint);
    TCHAR Text[32];
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 0, 256, 381, 0, 0, 256, 381, BackTex, 0xFF, 1);
    if (*GL2GameData.SysString[366])
        Device->DrawText(11, 12, 0xFFDCDCDC, *GL2GameData.SysString[366], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (*GL2GameData.SysString[367])
        Device->DrawText(11, 200, 0xFFDCDCDC, *GL2GameData.SysString[367], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (Info.Icon)
    {
        Device->DrawTexture(13, 29, 34, 34, 0, 0, 34, 34, OutLineDownTex, 0xFF, 1);
        Device->DrawTexture(13, 29, 35, 35, 0, 0, 35, 35, OutLineTex, 0xFF, 1);
        Device->DrawTexture(14, 30, 32, 32, 0, 0, 32, 32, Info.Icon, 0xFF, 1);
    }
    const TCHAR* Name = Info.Name;
    if (Name && *Name)
        Device->DrawText(0x33, 0x20, 0xFFDCDCDC, Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    // "Lv" (sysstring 88), the level and the text after it
    if (*GL2GameData.SysString[88])
        appSprintf(Text, TEXT("%s"), *GL2GameData.SysString[88]);
    else
        Text[0] = 0;
    Device->DrawText(0x33, 0x30, 0xFFA3A3A3, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    INT LabelW = Size.cx;
    appSprintf(Text, TEXT("%d"), Info.Level);
    Device->DrawText(LabelW + 0x35, 0x30, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    const TCHAR* After = Info.Unknown20C;
    if (After && *After)
        Device->DrawText(LabelW + 0x3F + Size.cx, 0x30, 0xFFB09B79, After, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    // MP cost
    INT ValueX = DrawLabeledLine(Device, 0x44, 320);
    appSprintf(Text, TEXT("%d"), Info.MpCost);
    Device->DrawText(ValueX, 0x44, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    // range (a negative one is not shown)
    if (Info.Range >= 0)
    {
        ValueX = DrawLabeledLine(Device, 0x50, 321);
        appSprintf(Text, TEXT("%d"), Info.Range);
        Device->DrawText(ValueX, 0x50, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    // the description, wrapped at 230 pixels
    if (*Info.Desc)
        Device->DrawText(0xD, 0x66, 0xFFB2BECF, *Info.Desc, 0xE6, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    // SP needed
    ValueX = DrawLabeledLine(Device, 0xDC, 365);
    appSprintf(Text, TEXT("%d"), Info.SP);
    Device->DrawText(ValueX, 0xDC, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    // the items needed: a row each that is shown
    for (INT i = 0; i < 4; i++)
    {
        NItemEntry* Item = Info.Items[i];
        if (Item->GetUnknownC34() > 0)
        {
            if (Item->GetIcon())
                Device->DrawTexture(0xD, i * 0x22 + 0xF2, 0x20, 0x20, 0, 0, 0x20, 0x20, Item->GetIcon(), 0xFF, 1);
            if (Item->GetName())
                appSprintf(Text, TEXT("%s X %d"), Item->GetName(), Item->Count);
            else
                Text[0] = 0;
            Device->DrawText(0x31, i * 0x22 + 0xF2, 0xFFDCDCDC, Text, 0x8C, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
    }
    DrawPlayerSP(Device);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10108240, NCTrainInfoWnd, OnPaint)

// 0x10107700 (vslot 62) - "Learn"
INT NCTrainInfoWnd::OnLearn()
{
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)Info.SkillID);
    Params.PushBack((void*)(INT_PTR)Info.Level);
    ::nw::VirtualCall<void>(GConsole, 401, &Params);   // RequestAcquireSkill (NConsoleWnd slot 401)
    return 1;
}
NW_HOOK_METHOD(0x10107700, NCTrainInfoWnd, OnLearn)

// 0x10107790 (vslot 69) - "List": back to the list page (the buttons are hidden after the
// page)
INT NCTrainInfoWnd::OnList()
{
    HideWindow();
    GDesktop->TrainWnd->ListWnd->ShowWindow();
    LearnButton->HideWindow();
    ListButton->HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10107790, NCTrainInfoWnd, OnList)

// 0x10107570 - empty strings and a NULL icon are not copied, the description always is; the
// icon is looked up again by name (NWindow.BlackTexture when missing). Info.Items stays.
void NCTrainInfoWnd::SetInfo(NTrainSkillInfo* Src)
{
    if (Src->Name && Src->Name[0])
        wcscpy(Info.Name, Src->Name);
    const TCHAR* IconName = Src->IconName;
    if (IconName && IconName[0])
        wcscpy(Info.IconName, IconName);
    Info.Desc = Src->Desc;
    if (Src->Unknown20C && Src->Unknown20C[0])
        wcscpy(Info.Unknown20C, Src->Unknown20C);
    if (Src->Icon)
        Info.Icon = Src->Icon;
    Info.SkillID = Src->SkillID;
    Info.Level = Src->Level;
    Info.Unknown2A8 = Src->Unknown2A8;
    Info.SP = Src->SP;
    Info.MpCost = Src->MpCost;
    Info.Range = Src->Range;
    Info.Unknown2B8 = Src->Unknown2B8;
    Info.Unknown2BC = Src->Unknown2BC;
    if (IconName && IconName[0])
        Info.Icon = FindTexture(IconName);
    if (!Info.Icon)
        Info.Icon = FindTexture(TEXT("NWindow.BlackTexture"));
}
NW_HOOK_METHOD(0x10107570, NCTrainInfoWnd, SetInfo)

// 0x10107690 - before the server's answer; the description, Unknown2A8, MP cost and range are
// left, the item rows hidden (NItemEntry::UnknownC34 -1)
void NCTrainInfoWnd::Reset()
{
    Info.Name[0] = 0;
    Info.IconName[0] = 0;
    Info.Unknown20C[0] = 0;
    Info.Icon = NULL;
    Info.SP = 0;
    Info.SkillID = -1;
    Info.Level = -1;
    Info.Unknown2B8 = 0;
    Info.Unknown2BC = -1;
    for (INT i = 0; i < 4; i++)
        Info.Items[i]->SetUnknownC34(-1);
}
NW_HOOK_METHOD(0x10107690, NCTrainInfoWnd, Reset)

// 0x10108ba0 - into the first hidden row (none free: dropped). The item goes through a
// temporary NItemEntry that is never deleted (a leak per required item).
void NCTrainInfoWnd::AddRequiredItem(NItemInfo* Item)
{
    for (INT i = 0; i < 4; i++)
    {
        if (Info.Items[i]->GetUnknownC34() < 0)
        {
            const TCHAR* IconName = Item->IconName;
            if (IconName)
                Item->Icon = FindTexture(IconName);
            if (!Item->Icon)
                Item->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
            Info.Items[i]->Copy(NewItemEntry(Item));
            Info.Items[i]->SetUnknownC34(1);
            return;
        }
    }
}
NW_HOOK_METHOD(0x10108ba0, NCTrainInfoWnd, AddRequiredItem)

NW_CHECK_VTABLE(NCTrainInfoWnd, 0x101a0fa0, 70)

/*-----------------------------------------------------------------------------
    NCTrainWnd
-----------------------------------------------------------------------------*/

// 0x10109560
NCTrainWnd::NCTrainWnd()
{
    ListWnd = NULL;
    InfoWnd = NULL;
}
NW_HOOK_CTOR(0x10109560, NCTrainWnd)

// 0x101095b0
NW_HOOK_DTOR(0x101095b0, NCTrainWnd)

// stock body shared with NControl's (0x10004e90)
INT NCTrainWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10109600 (vslot 37) - C3: the title bar (id 4: "Skill Trainer", sysstring 477) over the two pages, in the same
// place (256 x 381 under the bar), both shown (the list's own OnCreate and the console decide which one is up)
INT NCTrainWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameID = NID_TRAIN_FRAME;
    FrameCtrl->ID = NID_TRAIN_FRAME;
    FrameCtrl->SetPosition(0, 0, 0x100, 0x14);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    FrameCtrl->SetTitleText(*GL2GameData.SysString[477]);
    ListWnd = new NCTrainListWnd();
    ListWnd->SetPosition(0, 0x14, 0x100, 0x17D);
    CreateChild(ListWnd, NWS_VISIBLE);
    InfoWnd = new NCTrainInfoWnd();
    InfoWnd->SetPosition(0, 0x14, 0x100, 0x17D);
    CreateChild(InfoWnd, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10109600, NCTrainWnd, OnCreate)

// 0x100c4e30 (vslot 38) - also NCMSView_CharacterSelectWnd's and
// NCPawnView_CharacterSelectWnd's (their children at +0x94 and +0x98)
INT NCTrainWnd::OnDestroy()
{
    DestroyChild(this, ListWnd);
    DestroyChild(this, InfoWnd);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100c4e30, NCTrainWnd, OnDestroy)

// stock body shared with NCEditBox's (0x100a5d70)
void NCTrainWnd::ShowWindow()
{
    NCWnd::ShowWindow();
}

// stock body shared with NCChatWnd's (0x1003dab0)
void NCTrainWnd::HideWindow()
{
    NCWnd::HideWindow();
}

// 0x10108c90 (vslot 56) - nothing is drawn (both pages are children that paint themselves)
INT NCTrainWnd::OnPaint(NCGDevice* Device)
{
    guard(NCTrainWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10108c90, NCTrainWnd, OnPaint)

// 0x10107830 - a new list is coming
void NCTrainWnd::Reset()
{
    ListWnd->Clear();
    InfoWnd->Reset();
}
NW_HOOK_METHOD(0x10107830, NCTrainWnd, Reset)

// 0x101077e0
void NCTrainWnd::ShowList()
{
    InfoWnd->HideWindow();
    ListWnd->ShowWindow();
}
NW_HOOK_METHOD(0x101077e0, NCTrainWnd, ShowList)

// 0x10107800
void NCTrainWnd::ShowInfo()
{
    ListWnd->HideWindow();
    InfoWnd->ShowWindow();
}
NW_HOOK_METHOD(0x10107800, NCTrainWnd, ShowInfo)

// 0x10107820
void NCTrainWnd::SetSkillInfo(NTrainSkillInfo* Info)
{
    InfoWnd->SetInfo(Info);
}
NW_HOOK_METHOD(0x10107820, NCTrainWnd, SetSkillInfo)

// 0x10108d50
void NCTrainWnd::AddRequiredItem(NItemInfo* Item)
{
    InfoWnd->AddRequiredItem(Item);
}
NW_HOOK_METHOD(0x10108d50, NCTrainWnd, AddRequiredItem)

// 0x10109150 (no C1 match) - on to the list page
void NCTrainWnd::AddEntry(const NTrainEntry* Entry)
{
    ListWnd->AddEntry(Entry);
}
NW_HOOK_METHOD(0x10109150, NCTrainWnd, AddEntry)

NW_CHECK_VTABLE(NCTrainWnd, 0x101a10b8, 68)
