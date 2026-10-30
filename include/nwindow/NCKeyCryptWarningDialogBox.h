#pragma once
// NCKeyCryptWarningDialogBox - the keyboard protection warning of the login screen (UClass
// "CKeyCryptWarningDialogBox", 0x1AC; C2 only; NCLoginWnd +0xB0): nProtect's keyboard
// protection (npkcrypt) is not running; a box with system message 1239 and an OK button that
// closes it.

#include "nwindow/NCDialogBox.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

class NCKeyCryptWarningDialogBox : public NCDialogBox
{
    NW_DECLARE_CLASS(NCKeyCryptWarningDialogBox, NCDialogBox, 0, 0x102869B0)

    UTexture* BackTex;               // +0x1D4 "L2UI.EtcWnd.Popup"
    NCButton* OKButton;              // +0x1D8 id 0x1004, centered, text SysString 140

    NCKeyCryptWarningDialogBox();                                    // 0x100930e0
    // ~NCKeyCryptWarningDialogBox                                   // 0x10082d30 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10091f80
    virtual INT OnCreate();                                          // 37 0x10092c30
    virtual INT OnDestroy();                                         // 38 0x10092400
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100924b0
    virtual INT OnOK();                                              // 69 0x10091fe0 (name ours)
};
NW_ASSERT_OFFSET(NCKeyCryptWarningDialogBox, BackTex, 0x1D4);
NW_ASSERT_OFFSET(NCKeyCryptWarningDialogBox, OKButton, 0x1D8);
NW_ASSERT_SIZE(NCKeyCryptWarningDialogBox, 0x1DC);

#pragma pack(pop)
