#pragma once
// AddFriendDialog - the box that asks for the account of a new contact (UClass "AddFriendDialog", 0x1E8; C3 only, made by
// MsnWnd::OnCreate with the protocol object as its owner): an edit line and OK / Cancel. OK sends the text to the
// protocol object as a new friend (on the allow list) and closes the box; showing it empties the line.

#include "nwindow/NMsnDlgCommon.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NMsnLink.h"
#pragma pack(push, 4)

enum
{
    NID_ADDFRIEND_EDIT = 0xA20,    // names ours
    NID_ADDFRIEND_OK = 0xA21,
    NID_ADDFRIEND_CANCEL = 0xA22,
};

class AddFriendDialog : public NCDialogBox
{
    NW_DECLARE_CLASS(AddFriendDialog, NCDialogBox, 0, 0x102BFE80)

    NMsnLink* Msn;                   // +0x1D4 the protocol object (constructor argument)
    NCEditBox* EditBox;              // +0x1D8 the account, 156 x 17 at (50, 48) (the stock constructor does not clear it)
    NCButton* OKButton;              // +0x1DC
    NCButton* CancelButton;          // +0x1E0
    UTexture* BackTex;               // +0x1E4 "L2UI_CH3.dialog.system_back"

    AddFriendDialog(NMsnLink* InMsn = NULL);                         // 0x101239E0
    // ~AddFriendDialog                                              // 0x10123A50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x101208F0
    virtual INT OnCreate();                                          // 37 0x10121D60
    virtual INT OnDestroy();                                         // 38 0x10121390 (folded with AddGroupDialog's)
    virtual void ShowWindow();                                       // 44 0x10120640 (folded with AddGroupDialog's)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10120960

    // names ours
    __declspec(noinline) INT OnOK();                                                      // 0x101205E0 the account sent, the box closed
    __declspec(noinline) INT OnCancel();                                                  // 0x1007BEE0 (folded with NCOptionWnd's) the box closed
};
NW_ASSERT_OFFSET(AddFriendDialog, Msn, 0x1D4);
NW_ASSERT_OFFSET(AddFriendDialog, EditBox, 0x1D8);
NW_ASSERT_OFFSET(AddFriendDialog, OKButton, 0x1DC);
NW_ASSERT_OFFSET(AddFriendDialog, CancelButton, 0x1E0);
NW_ASSERT_OFFSET(AddFriendDialog, BackTex, 0x1E4);
NW_ASSERT_SIZE(AddFriendDialog, 0x1E8);

#pragma pack(pop)
