#pragma once
// NCEulaWnd - the license agreement box of the login screen (UClass "CEulaWnd", 0xAC; C2 only;
// NCLoginWnd +0xA4): the text in an NCNoticeInfoWnd and two buttons, agree (or Enter), which
// goes on to the announcements (NCNoticeWnd, with the EULA text) when there is one or else
// asks for the server list, and the other one, which drops the connection and goes back to
// the login box.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCNoticeInfoWnd;

class NCEulaWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCEulaWnd, NCWnd, 0, 0x10291E80)

    UTexture* BackTex;               // +0xC8 "L2UI.etcWnd.Announce_back" below the title bar
    NCNoticeInfoWnd* InfoWnd;        // +0xCC the text
    NCButton* AgreeButton;           // +0xD0 text 599 (name ours)
    NCButton* DisagreeButton;        // +0xD4 text 600 (name ours)
    INT Type;                        // +0xD8 (name ours) set from outside; agreeing gives it to the notice box's Type   // C2: offset derived, no evidence

    NCEulaWnd();                                                     // 0x100b1ac0
    // ~NCEulaWnd                                                    // 0x1009b6a0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100b0c00
    virtual INT OnCreate();                                          // 37 0x100b1b60
    virtual INT OnDestroy();                                         // 38 0x100b1160
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100b1260
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 59 0x100b0d20
    virtual INT OnAgree();                                           // 68 0x100b1360 (name ours)
    virtual INT OnDisagree();                                        // 69 0x100b0cb0 (name ours)

    // names ours (the text box's, like NCWarningWnd's)
    void SetText(const TCHAR* Text);                                 // 0x100b0c80 empty: ignored
    void ClearText();                                                // 0x100b0ca0
};
NW_ASSERT_OFFSET(NCEulaWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCEulaWnd, InfoWnd, 0xCC);
NW_ASSERT_OFFSET(NCEulaWnd, DisagreeButton, 0xD4);
NW_ASSERT_OFFSET(NCEulaWnd, Type, 0xD8);
NW_ASSERT_SIZE(NCEulaWnd, 0xDC);

#pragma pack(pop)
