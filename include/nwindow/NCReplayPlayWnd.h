#pragma once
// NCReplayPlayWnd - the playing screen of the replay (UClass "CReplayPlayWnd", 0xD0; C3 only; NCReplayWnd +0xCC): the
// replay's logo in the upper left corner and the small logo in the lower right one, both scaled with the screen
// (the larger of height / 768 and width / 1024). Layout and virtuals from the stock constructor 0x100E7A00, OnCreate
// 0x100E6650, OnDestroy 0x100E6790 and OnPaint 0x100E6DC0.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCReplayPlayWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCReplayPlayWnd, NCWnd, 0, 0x102A8BD8)

    UTexture* LogoTex;                  // +0xC8 localization.ini "<country>_ReplayLogoTexture" (256 x 64)
    UTexture* MiniLogoTex;              // +0xCC localization.ini "<country>_MiniLogoTexture" (232 x 120)

    NCReplayPlayWnd();                                               // 0x100E7A00
    // ~NCReplayPlayWnd                                              // 0x100E7A50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (folded)
    virtual INT OnCreate();                                          // 37 0x100E6650
    virtual INT OnDestroy();                                         // 38 0x100E6790 (NCWnd's OnDestroy and nothing else)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100E6DC0
};
NW_ASSERT_OFFSET(NCReplayPlayWnd, LogoTex, 0xC8);
NW_ASSERT_SIZE(NCReplayPlayWnd, 0xD0);

#pragma pack(pop)
