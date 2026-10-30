#pragma once
// NCReplayMenuWnd - the replay's menu screen (UClass "CReplayMenuWnd", 0xD8; C3 only; NCReplayWnd +0xC8): a
// full-screen backdrop with the function window (the four buttons) in the lower right corner.
// Layout and virtuals from the stock constructor 0x100E7880, OnCreate 0x100E7930, OnDestroy 0x100E6C10 and OnPaint
// 0x100E6CC0.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCReplayFunctionWnd;

class NCReplayMenuWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCReplayMenuWnd, NCWnd, 0, 0x102A7C68)

    NCReplayFunctionWnd* FunctionWnd;   // +0xC8 (name ours) 120 x 138, 154 x 236 from the lower right corner
    INT UnknownCC;                      // +0xCC (name ours) cleared by the constructor, never used
    INT UnknownD0;                      // +0xD0 (name ours) the same
    UTexture* BackTex;                  // +0xD4 (name ours) "L2UI_CH3.BenchMarkWnd.BenchMark_Title" (1024 x 768 piece)

    NCReplayMenuWnd();                                               // 0x100E7880
    // ~NCReplayMenuWnd                                              // 0x100E78E0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (folded)
    virtual INT OnCreate();                                          // 37 0x100E7930
    virtual INT OnDestroy();                                         // 38 0x100E6C10
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100E6CC0
};
NW_ASSERT_OFFSET(NCReplayMenuWnd, FunctionWnd, 0xC8);
NW_ASSERT_OFFSET(NCReplayMenuWnd, BackTex, 0xD4);
NW_ASSERT_SIZE(NCReplayMenuWnd, 0xD8);

#pragma pack(pop)
