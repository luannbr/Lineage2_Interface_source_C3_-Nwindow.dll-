#pragma once
// RemoveFriendDialog - the box that asks before a contact is removed (UClass "RemoveFriendDialog", 0x230; C3 only, made by
// MsnWnd::OnCreate with the window and the protocol object): a question, a check box ("also block this contact") and
// OK / Cancel. OK removes the contact selected in the window's tree (its account is read from the tree item) and closes
// the box, or, with nothing selected, the window shows a message instead; showing the box clears the check box.

#include "nwindow/NMsnDlgCommon.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NMsnBuddy.h"
#include "nwindow/NMsnLink.h"
#pragma pack(push, 4)

enum
{
    NID_REMOVEFRIEND_OK = 0xA33,   // names ours
    NID_REMOVEFRIEND_CANCEL = 0xA34,
};

class RemoveFriendDialog : public NCDialogBox
{
    NW_DECLARE_CLASS(RemoveFriendDialog, NCDialogBox, 0, 0x102BDF98)

    MsnWnd* Wnd;                     // +0x1D4 the messenger window (constructor argument 1)
    NMsnLink* Msn;                   // +0x1D8 the protocol object (constructor argument 2)
    NMsnBuddy Buddy;                 // +0x1DC a contact record, made and destroyed with the box (nothing in this class uses it)
    NCCheckBox* BlockCheck;          // +0x220 12 high at (13, 63): the contact is blocked too (SysString-less text: systemmsg 1316)
    NCButton* OKButton;              // +0x224 id 0xA33, 76 x 23 at (51, 112)
    NCButton* CancelButton;          // +0x228 id 0xA34, at (131, 112)
    UTexture* BackTex;               // +0x22C "L2UI_CH3.Msn.msn_optionback4"

    RemoveFriendDialog(MsnWnd* InWnd = NULL, NMsnLink* InMsn = NULL); // 0x10123AA0
    // ~RemoveFriendDialog                                           // 0x10123B50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10121FB0
    virtual INT OnCreate();                                          // 37 0x10122020
    virtual INT OnDestroy();                                         // 38 0x10120A60
    virtual void ShowWindow();                                       // 44 0x10120C50
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10120B00

    // names ours
    __declspec(noinline) INT OnOK();                                                      // 0x10120C70 the selected contact removed (and blocked), the box closed
    __declspec(noinline) INT OnCancel();                                                  // 0x1007BEE0 (folded with NCOptionWnd's) the box closed
};
NW_ASSERT_OFFSET(RemoveFriendDialog, Wnd, 0x1D4);
NW_ASSERT_OFFSET(RemoveFriendDialog, Msn, 0x1D8);
NW_ASSERT_OFFSET(RemoveFriendDialog, Buddy, 0x1DC);
NW_ASSERT_OFFSET(RemoveFriendDialog, BlockCheck, 0x220);
NW_ASSERT_OFFSET(RemoveFriendDialog, OKButton, 0x224);
NW_ASSERT_OFFSET(RemoveFriendDialog, CancelButton, 0x228);
NW_ASSERT_OFFSET(RemoveFriendDialog, BackTex, 0x22C);
NW_ASSERT_SIZE(RemoveFriendDialog, 0x230);

#pragma pack(pop)
