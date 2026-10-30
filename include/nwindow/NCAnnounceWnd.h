#pragma once
// NCAnnounceWnd - the announce box of the login screen (UClass "CAnnounceWnd", 0xA4; C2 only;
// NCLoginWnd +0xA8): a picture ("Announce_cn") and an OK button (or Enter) that shows the
// announcements (NCNoticeWnd, with the EULA text) when there is one, or else asks for the
// server list.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

class NCAnnounceWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCAnnounceWnd, NCWnd, 0, 0x102923A0)

    UTexture* BackTex;               // +0xC8 "L2UI.etcWnd.Announce_cn" below the title bar   // C2: offset derived, no evidence
    NCButton* OKButton;              // +0xCC
    INT Type;                        // +0xD0 (name ours) set from outside; OK gives it to the notice box's Type   // C2: offset derived, no evidence

    NCAnnounceWnd();                                                 // 0x100b1d90
    // ~NCAnnounceWnd                                                // 0x1009be20 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100b0d60
    virtual INT OnCreate();                                          // 37 0x100b16f0
    virtual INT OnDestroy();                                         // 38 0x100b1490
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100b1540
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 59 0x100b1680
    virtual INT OnOK();                                              // 68 0x100b1640 (name ours)
};
NW_ASSERT_OFFSET(NCAnnounceWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCAnnounceWnd, OKButton, 0xCC);
NW_ASSERT_OFFSET(NCAnnounceWnd, Type, 0xD0);
NW_ASSERT_SIZE(NCAnnounceWnd, 0xD4);

#pragma pack(pop)
