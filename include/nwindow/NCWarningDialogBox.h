#pragma once
// NCWarningDialogBox - the small box with a text and an OK button (UClass "CWarningDialogBox",
// 0x1BC; C2 only; NConsoleWnd +0x3554, made by Initialize): the console shows it modal with a
// text (SetText keeps the first one it gets until the box is closed). OK closes it, or, when
// the box was raised for a fatal reason (bExit), ends the client.

#include "nwindow/NCDialogBox.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

class NCWarningDialogBox : public NCDialogBox
{
    NW_DECLARE_CLASS(NCWarningDialogBox, NCDialogBox, 0, 0x10264D00)

    UTexture* BackTex;               // +0x1D4 "L2UI_ch3.dialog.system_back" (C3; C2 "L2UI.EtcWnd.Popup")
    UTexture* IconTex;               // +0x1D8 "L2UI_ch3.dialog.warningicon", drawn at (11, 15) (OnPaint 0x100096d0)
    NCButton* OKButton;              // +0x1DC id 0x1004, centered, text SysString 140
    FString Text;                    // +0x1E0 the message (name ours)
    INT bExit;                       // +0x1EC OK ends the client (name ours; set from outside)

    NCWarningDialogBox();                                            // 0x1000a170
    // ~NCWarningDialogBox                                           // 0x10009170 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10091f80 (folded with
                                                                     //    NCKeyCryptWarningDialogBox's)
    virtual INT OnCreate();                                          // 37 0x10009dd0
    virtual INT OnDestroy();                                         // 38 0x10009830
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100096d0
    virtual INT OnOK();                                              // 69 0x100098e0 (name ours)

    // name ours (the stock has a guard of its own with this name): the message the box shows
    void SetWarningMessage(const TCHAR* InText);                     // 0x10007520 (C1 SetText: 0x10006e00)
};
NW_ASSERT_OFFSET(NCWarningDialogBox, BackTex, 0x1D4);
NW_ASSERT_OFFSET(NCWarningDialogBox, OKButton, 0x1DC);
NW_ASSERT_OFFSET(NCWarningDialogBox, Text, 0x1E0);
NW_ASSERT_OFFSET(NCWarningDialogBox, bExit, 0x1EC);
NW_ASSERT_SIZE(NCWarningDialogBox, 0x1F0);

#pragma pack(pop)
