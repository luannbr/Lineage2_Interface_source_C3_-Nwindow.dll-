/*=============================================================================
    OptionWnd.cpp - the messenger's options window (C3 only; C3 addresses). Stock: 0x101325E0 (OnDestroy), 0x10132810
    (OnPaint), 0x10132940 (the settings in), 0x10132A40 (OK), 0x10135510 (OnCreate), 0x10135E20 (ShowWindow), 0x10135E30 /
    0x10135E50 / 0x10135E70 (the group buttons), 0x10136670 (WindowProc), 0x10137960 / 0x10137A10 (constructor, destructor).
=============================================================================*/
#include "nwindow/OptionWnd.h"
#include "nwindow/MsnWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NGameData.h"

enum
{
    OPTION_OKID = 0xA17,
    OPTION_CancelID = 0xA18,
    OPTION_NameEditID = 0xA1A,
    OPTION_AddGroupID = 0xA1B,
    OPTION_RenameGroupID = 0xA1C,
    OPTION_RemoveGroupID = 0xA1D,
    OPTION_FrameID = 0xA53,
};

// A child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// A check box of the window (repeated inline in the stock OnCreate): the label (a system string), 250 wide, at (20, Y)
static FORCEINLINE NCCheckBox* CreateOptionBox(NCWnd* Parent, INT StringID, INT Y, INT H, INT TextParam)
{
    NCCheckBox* Box = new NCCheckBox(*MSN_SYSSTR(StringID), 0, 0, TextParam);
    Box->SetPosition(0x14, Y, 0xFA, H);
    Parent->CreateChild(Box, NWS_VISIBLE);
    return Box;
}

// 0x10137960 - the owner and the protocol object are the arguments, nothing made yet
OptionWnd::OptionWnd(MsnWnd* InOwner, Msn* InMsn)
{
    Owner = InOwner;
    NameEdit = NULL;
    AddGroupButton = NULL;
    RenameGroupButton = NULL;
    RemoveGroupButton = NULL;
    LoginAlarmBox = NULL;
    MessageAlarmBox = NULL;
    NewMailAlarmBox = NULL;
    SoundAlarmBox = NULL;
    DontDisturbBox = NULL;
    HideAllBox = NULL;
    OKButton = NULL;
    CancelButton = NULL;
    BackTex = NULL;
    MsnObj = InMsn;
}
NW_HOOK_CTOR_ARGS(0x10137960, OptionWnd, (MsnWnd* A, Msn* B), (A, B))

// 0x10137a10
NW_HOOK_DTOR(0x10137A10, OptionWnd)

// 0x10136670 (vslot 25) - the three group buttons, OK, and cancel (which hides the window, NCOptionWnd's folded OnCancel)
INT OptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == OPTION_AddGroupID)
            return OnAddGroupButton() != 0;
        if (wParam == OPTION_RenameGroupID)
            return OnRenameGroupButton() != 0;
        if (wParam == OPTION_RemoveGroupID)
            return OnRemoveGroupButton() != 0;
        if (wParam == OPTION_OKID)
            return OnOKButton() != 0;
        if (wParam == OPTION_CancelID)
            return ((NCOptionWnd*)(NCWnd*)this)->NCOptionWnd::OnCancel() != 0;   // 0x1007BEE0, NCOptionWnd::OnCancel (shared): hides the window, 1
    }
    return 0;
}
NW_HOOK_METHOD(0x10136670, OptionWnd, WindowProc)

// 0x10135510 (vslot 37) - NCWnd's, the picture, a title bar of its own (the decorated NCFrameCtrl, 256 x 20, id 0xA53), the
// name box, the three group buttons, the six check boxes and OK / cancel
INT OptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.Msn.msn_optionback2"));

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameID = OPTION_FrameID;
    FrameCtrl->ID = OPTION_FrameID;
    FrameCtrl->SetPosition(0, 0, 0x100, 0x14);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    NameEdit = new NCEditBox(0, NULL, 1, 0x108, 0);
    NameEdit->ID = OPTION_NameEditID;
    NameEdit->SetPosition(0x12, 0x34, 0xDC, 0x10);
    CreateChild(NameEdit, NWS_VISIBLE);

    AddGroupButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    AddGroupButton->ID = OPTION_AddGroupID;
    AddGroupButton->SetPosition(0x10, 0x67, 0x4C, 0x17);
    CreateChild(AddGroupButton, NWS_VISIBLE);
    AddGroupButton->SetText(*MSN_SYSSTR(984));

    RenameGroupButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    RenameGroupButton->ID = OPTION_RenameGroupID;
    RenameGroupButton->SetPosition(0x5B, 0x67, 0x4C, 0x17);
    CreateChild(RenameGroupButton, NWS_VISIBLE);
    RenameGroupButton->SetText(*MSN_SYSSTR(985));

    RemoveGroupButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    RemoveGroupButton->ID = OPTION_RemoveGroupID;
    RemoveGroupButton->SetPosition(0xA6, 0x67, 0x4C, 0x17);
    CreateChild(RemoveGroupButton, NWS_VISIBLE);
    RemoveGroupButton->SetText(*MSN_SYSSTR(986));

    LoginAlarmBox = CreateOptionBox(this, 987, 0x9C, 0x14, 0);
    MessageAlarmBox = CreateOptionBox(this, 988, 0xAF, 0x14, 0);
    NewMailAlarmBox = CreateOptionBox(this, 989, 0xC2, 0x14, 0);
    SoundAlarmBox = CreateOptionBox(this, 990, 0xD5, 0x14, 0);
    DontDisturbBox = CreateOptionBox(this, 991, 0xE8, 0x14, 0);
    HideAllBox = CreateOptionBox(this, 992, 0xFB, 0x1E, 200);

    OKButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    OKButton->ID = OPTION_OKID;
    OKButton->SetPosition(0x32, 0x121, 0x4C, 0x17);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetText(*MSN_SYSSTR(140));

    CancelButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    CancelButton->ID = OPTION_CancelID;
    CancelButton->SetPosition(0x82, 0x121, 0x4C, 0x17);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetText(*MSN_SYSSTR(141));
    return 1;
}
NW_HOOK_METHOD(0x10135510, OptionWnd, OnCreate)

// 0x101325e0 (vslot 38) - every control goes, in the order they were made, then NCWnd's
INT OptionWnd::OnDestroy()
{
    DestroyChild(this, NameEdit);
    DestroyChild(this, AddGroupButton);
    DestroyChild(this, RenameGroupButton);
    DestroyChild(this, RemoveGroupButton);
    DestroyChild(this, LoginAlarmBox);
    DestroyChild(this, MessageAlarmBox);
    DestroyChild(this, NewMailAlarmBox);
    DestroyChild(this, SoundAlarmBox);
    DestroyChild(this, DontDisturbBox);
    DestroyChild(this, HideAllBox);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x101325E0, OptionWnd, OnDestroy)

// 0x10132810 (vslot 56) - the picture under the title bar (from y 20) and the three headings (system strings 1009 .. 1011)
INT OptionWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
    {
        INT H = (INT)Height;
        INT W = (INT)Width;
        Device->DrawTexture(0, 0x14, W, H, 0, 0, W, H, BackTex, 0xFF, 1);
    }
    Device->DrawText(0x16, 0x1F, 0xFFDCDCDC, *MSN_SYSSTR(1009), 0, 0, 0, (L2FontType)0, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(0x16, 0x52, 0xFFDCDCDC, *MSN_SYSSTR(1010), 0, 0, 0, (L2FontType)0, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(0x16, 0x87, 0xFFDCDCDC, *MSN_SYSSTR(1011), 0, 0, 0, (L2FontType)0, 0, 0, 0, 0, 0, 0, 0);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10132810, OptionWnd, OnPaint)

// 0x10135e20 (vslot 44) - NCWnd's, then the settings go into the boxes
void OptionWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    LoadSettings();
}
NW_HOOK_METHOD(0x10135E20, OptionWnd, ShowWindow)

// 0x10132940 (name ours; anonymous in the db) - the name box is emptied and takes the signed-in person's name (the record at
// +0x54 of the protocol object), the six boxes take the stock's settings
void OptionWnd::LoadSettings()
{
    NameEdit->ClearText();
    NMsnPersonRec* Person = MsnGetSelf(MsnObj);
    if (Person)
    {
        BYTE AccountRaw[sizeof(FString)];
        FString& Account = *Person->GetDisplayName((FString*)AccountRaw);
        NameEdit->InsertText(*Account, 1);
        Account.~FString();
    }
    LoginAlarmBox->bChecked = GMsnLoginAlarm;
    MessageAlarmBox->bChecked = GMsnMessageAlarm;
    NewMailAlarmBox->bChecked = GMsnNewEMailAlarm;
    SoundAlarmBox->bChecked = GMsnSoundAlarm;
    DontDisturbBox->bChecked = GMsnDontDisturb;
    HideAllBox->bChecked = GMsnHideAll;
}
NW_HOOK_METHOD(0x10132940, OptionWnd, LoadSettings)

// 0x10132a40 (name ours; anonymous in the db) - the new name goes to the protocol object, the six boxes to the stock's
// settings and to Option.ini ("Msn" section), and the window goes
INT OptionWnd::OnOKButton()
{
    if (NameEdit->CommitText())
        MsnSetDisplayName(MsnObj, NameEdit->CommitText());                 // the protocol object's SetDisplayName
    GMsnLoginAlarm = LoginAlarmBox->bChecked;
    GMsnMessageAlarm = MessageAlarmBox->bChecked;
    GMsnNewEMailAlarm = NewMailAlarmBox->bChecked;
    GMsnSoundAlarm = SoundAlarmBox->bChecked;
    GMsnDontDisturb = DontDisturbBox->bChecked;
    GMsnHideAll = HideAllBox->bChecked;
    GConfig->SetBool(TEXT("Msn"), TEXT("LoginAlarm"), GMsnLoginAlarm, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Msn"), TEXT("MessageAlarm"), GMsnMessageAlarm, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Msn"), TEXT("NewEMailAlarm"), GMsnNewEMailAlarm, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Msn"), TEXT("SoundAlarm"), GMsnSoundAlarm, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Msn"), TEXT("DontDisturb"), GMsnDontDisturb, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Msn"), TEXT("HideAll"), GMsnHideAll, TEXT("Option.ini"));
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10132A40, OptionWnd, OnOKButton)

// 0x10135e30 (name ours; anonymous in the db) - the owner shows its add-group dialog
INT OptionWnd::OnAddGroupButton()
{
    Owner->ShowAddGroupDialog();
    return 1;
}
NW_HOOK_METHOD(0x10135E30, OptionWnd, OnAddGroupButton)

// 0x10135e50 (name ours; anonymous in the db)
INT OptionWnd::OnRenameGroupButton()
{
    Owner->ShowRenameGroupDialog();
    return 1;
}
NW_HOOK_METHOD(0x10135E50, OptionWnd, OnRenameGroupButton)

// 0x10135e70 (name ours; anonymous in the db)
INT OptionWnd::OnRemoveGroupButton()
{
    Owner->ShowRemoveGroupDialog();
    return 1;
}
NW_HOOK_METHOD(0x10135E70, OptionWnd, OnRemoveGroupButton)

NW_CHECK_VTABLE(OptionWnd, 0x101AA808, 68)
