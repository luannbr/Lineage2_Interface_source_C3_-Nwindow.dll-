#pragma once
// NCWarningWnd - the login screen's "Warning" box (UClass "CWarningWnd", 0xA8; NCLoginWnd
// creates it): a text in an NCNoticeInfoWnd and an OK button (or Enter) that asks for the
// server list. A copy of NCNoticeWnd's code: even its OnPaint guard is NCNoticeWnd's.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCNoticeInfoWnd;

class NCWarningWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCWarningWnd, NCWnd, 0, 0x102928C8)

    INT TimerResult;                 // +0xC8 SetTimer's (ShowWindow)
    INT Unknown98;                   // +0xCC (0) cleared by ShowWindow / HideWindow
    UTexture* BackTex;               // +0xD0 "L2UI.etcWnd.Announce_back" below the title bar
    NCNoticeInfoWnd* InfoWnd;        // +0xD4 the text
    NCButton* OKButton;              // +0xD8

    NCWarningWnd();                                                  // 0x100b1890
    // ~NCWarningWnd                                                 // 0x1009b240 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100b0ac0
    virtual INT OnTimer(INT TimerID) { return 1; }                   // 36 0x10003360 (folded)
    virtual INT OnCreate();                                          // 37 0x100b1930
    virtual INT OnDestroy();                                         // 38 0x100b0f90
    virtual void ShowWindow();                                       // 44 0x100b0bb0
    virtual void HideWindow();                                       // 45 0x100b0be0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100b1060
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 59 0x100b0b70
    // names ours
    virtual INT OnOK();                                              // 68 0x100b0b50 server list

    void SetText(const TCHAR* Text);                                 // 0x1009a820 (C1 only: no C2 match) empty: ignored
    void ClearText();                                                // 0x1009a840 (C1 only: no C2 match)
};
NW_ASSERT_OFFSET(NCWarningWnd, TimerResult, 0xC8);
NW_ASSERT_OFFSET(NCWarningWnd, BackTex, 0xD0);
NW_ASSERT_OFFSET(NCWarningWnd, OKButton, 0xD8);
NW_ASSERT_SIZE(NCWarningWnd, 0xDC);

#pragma pack(pop)