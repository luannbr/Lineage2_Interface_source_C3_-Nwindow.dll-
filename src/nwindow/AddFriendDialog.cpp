/*=============================================================================
    AddFriendDialog.cpp - the box that asks for the account of a new contact (C3 only). Stock: 0x10120640 (ShowWindow),
    0x101205E0, 0x101208F0 .. 0x10120A5F (WindowProc, OnPaint), OnDestroy 0x10121390, OnCreate 0x10121D60, the
    constructor 0x101239E0 and the destructor 0x10123A50.
=============================================================================*/
#include "nwindow/AddFriendDialog.h"
#include "nwindow/MsnWnd.h"
#include "nwindow/NCGDevice.h"

enum { MSN_TEXT_ADDFRIEND = 1314 };   // systemmsg line (GL2GameData + 0x4EC9C)

// 0x101239e0 - the base constructor, the protocol object kept, the buttons and the texture none (the edit line is not
// cleared)
AddFriendDialog::AddFriendDialog(NMsnLink* InMsn)
    : Msn(InMsn), OKButton(NULL), CancelButton(NULL), BackTex(NULL)
{
}
NW_HOOK_CTOR_ARGS(0x101239e0, AddFriendDialog, (NMsnLink* A), (A))

// the messenger window makes the box through this (MsnWnd.cpp cannot include the dialogs' headers: see MsnWnd.h)
NCWnd* MsnNewAddFriendDialog(void* Link) { return new AddFriendDialog((NMsnLink*)Link); }

// 0x10123a50
NW_HOOK_DTOR(0x10123a50, AddFriendDialog)

// 0x101208f0 (vslot 25) - OK sends the account, Cancel closes the box
INT AddFriendDialog::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCDialogBox::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_ADDFRIEND_OK)
            return OnOK() != 0;
        if ((INT)wParam == NID_ADDFRIEND_CANCEL)
            return OnCancel() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x101208f0, AddFriendDialog, WindowProc)

// 0x101205e0 (name ours) - a text in the line goes to the protocol object as a new friend (allowed), then the box closes
INT AddFriendDialog::OnOK()
{
    if (EditBox->CommitText())
        Msn->RequestAddFriend(FString(EditBox->CommitText()), 1);
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x101205e0, AddFriendDialog, OnOK)

// 0x1007bee0 (name ours) - the body the linker folded with NCOptionWnd::OnCancel (HideWindow, return 1): called there
#if NW_HOOK_MODE
NW_NAKED INT AddFriendDialog::OnCancel() NW_STOCK(0x1007BEE0)
#else
INT AddFriendDialog::OnCancel()   // the standalone: the folded body itself (NCOptionWnd::OnCancel 0x1007BEE0: HideWindow, 1)
{
    HideWindow();
    return 1;
}
#endif

// 0x10120640 (vslot 44) - shown, with the line emptied (the body is one with AddGroupDialog's)
void AddFriendDialog::ShowWindow()
{
    NCWnd::ShowWindow();
    EditBox->ClearText();
}
NW_HOOK_METHOD(0x10120640, AddFriendDialog, ShowWindow)

// 0x10121d60 (vslot 37) - the back texture, the edit line (156 x 17 at (50, 48), 150 px past the box), OK (51, 75) and
// Cancel (131, 75)
INT AddFriendDialog::OnCreate()
{
    NCDialogBox::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.dialog.system_back"));
    EditBox = new NCEditBox(0, NULL, 1, 150, 0);
    EditBox->ID = NID_ADDFRIEND_EDIT;
    EditBox->SetPosition(50, 48, 156, 17);
    CreateChild(EditBox, NWS_VISIBLE);
    OKButton = NewDlgButton(this, NID_ADDFRIEND_OK, 51, 75, 140);
    CancelButton = NewDlgButton(this, NID_ADDFRIEND_CANCEL, 131, 75, 141);
    return 1;
}
NW_HOOK_METHOD(0x10121d60, AddFriendDialog, OnCreate)

// 0x10121390 (vslot 38) - the edit line, OK and Cancel go, then NCDialogBox (one body with AddGroupDialog's)
INT AddFriendDialog::OnDestroy()
{
    DestroyDlgChild(this, EditBox);
    DestroyDlgChild(this, OKButton);
    DestroyDlgChild(this, CancelButton);
    NCDialogBox::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10121390, AddFriendDialog, OnDestroy)

// 0x10120960 (vslot 56) - the back texture over the box and the text at (11, 16), wrapped to the width less 22
INT AddFriendDialog::OnPaint(NCGDevice* Device)
{
    TCHAR Text[1024];
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, (INT)Width, (INT)Height, BackTex, 0xFF, 1);
    appStrcpy(Text, MsnDlgText(MSN_TEXT_ADDFRIEND));
    Device->DrawText(11, 16, 0xFFDCDCDC, Text, (INT)(Width - 22.0f), 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10120960, AddFriendDialog, OnPaint)

NW_CHECK_VTABLE(AddFriendDialog, 0x101A7ED0, 69)
