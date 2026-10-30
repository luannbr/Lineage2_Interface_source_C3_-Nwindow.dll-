#pragma once
// MoveGroupDialog - the box that asks to which contact group a contact moves (UClass "MoveGroupDialog", 0x22C; C3 only,
// made by MsnWnd::OnCreate with the protocol object as its owner): a combo box listing the groups (filled when the box
// is shown) and OK / Cancel. The contact is the record kept in the box (set from outside before it is shown). OK asks the
// protocol object to move the contact to the chosen group and closes the box.

#include "nwindow/NMsnDlgCommon.h"
#include "nwindow/NMsnBuddy.h"
#pragma pack(push, 4)

enum
{
    NID_MOVEGROUP_OK = 0xA23,      // names ours (the combo box has no id)
    NID_MOVEGROUP_CANCEL = 0xA24,
};

class MoveGroupDialog : public NCDialogBox
{
    NW_DECLARE_CLASS(MoveGroupDialog, NCDialogBox, 0, 0x102BEF10)

    NMsnLink* Msn;                   // +0x1D4 the protocol object (constructor argument)
    NMsnBuddy Buddy;                 // +0x1D8 the contact to move (its second number, +0x1DC, is the group it is in)
    NCComboBox* GroupCombo;          // +0x21C the groups, 156 x 17 at (50, 48) (child style NWS_DLG_COMBO)
    NCButton* OKButton;              // +0x220 id 0xA23, 76 x 23 at (51, 75)
    NCButton* CancelButton;          // +0x224 id 0xA24, at (131, 75)
    UTexture* BackTex;               // +0x228 "L2UI_CH3.dialog.system_back"

    MoveGroupDialog(NMsnLink* InMsn = NULL);                         // 0x10123CA0
    // ~MoveGroupDialog                                              // 0x10123D50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10122980
    virtual INT OnCreate();                                          // 37 0x101229F0
    virtual INT OnDestroy();                                         // 38 0x10121120
    virtual void ShowWindow();                                       // 44 0x101236D0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x101211C0

    // names ours
    __declspec(noinline) INT OnOK();                                                      // 0x101212C0 the contact moved to the chosen group, the box closed
    __declspec(noinline) INT OnCancel();                                                  // 0x1007BEE0 (folded with NCOptionWnd's) the box closed
};
NW_ASSERT_OFFSET(MoveGroupDialog, Msn, 0x1D4);
NW_ASSERT_OFFSET(MoveGroupDialog, Buddy, 0x1D8);
NW_ASSERT_OFFSET(MoveGroupDialog, GroupCombo, 0x21C);
NW_ASSERT_OFFSET(MoveGroupDialog, OKButton, 0x220);
NW_ASSERT_OFFSET(MoveGroupDialog, CancelButton, 0x224);
NW_ASSERT_OFFSET(MoveGroupDialog, BackTex, 0x228);
NW_ASSERT_SIZE(MoveGroupDialog, 0x22C);

#pragma pack(pop)
