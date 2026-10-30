#pragma once
// MessageDialog - the messenger's plain message box (UClass "MessageDialog", 0x1E8; C3 only, made by MsnWnd::ShowMessage and
// the like): a text and an OK button that closes it.

#include "nwindow/NMsnDlgCommon.h"
#pragma pack(push, 4)

enum
{
    NID_MESSAGE_OK = 0xA2F,        // name ours
};

class MessageDialog : public NCDialogBox
{
    NW_DECLARE_CLASS(MessageDialog, NCDialogBox, 0, 0x102BF438)

    FString Message;                 // +0x1D4 the text, drawn at (11, 16) once it has a character
    NCButton* OKButton;              // +0x1E0 id 0xA2F, 76 x 23 at (90, 75)
    UTexture* BackTex;               // +0x1E4 "L2UI_CH3.dialog.system_back"

    MessageDialog();                                                 // 0x10124000
    // ~MessageDialog                                                // 0x101240A0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x101219A0
    virtual INT OnCreate();                                          // 37 0x10123480
    virtual INT OnDestroy();                                         // 38 0x10121A00
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10121A40

    // name ours
    INT OnOK();                                                      // 0x1007BEE0 (folded with NCOptionWnd's) the box closed
};
NW_ASSERT_OFFSET(MessageDialog, Message, 0x1D4);
NW_ASSERT_OFFSET(MessageDialog, OKButton, 0x1E0);
NW_ASSERT_OFFSET(MessageDialog, BackTex, 0x1E4);
NW_ASSERT_SIZE(MessageDialog, 0x1E8);

#pragma pack(pop)
