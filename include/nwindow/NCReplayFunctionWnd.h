#pragma once
// NCReplayFunctionWnd - four buttons one under the other over the lobby menu's backdrop (UClass "CReplayFunctionWnd",
// 0xDC; C3 only; NCReplayMenuWnd +0xC8), the same shape as NCLoginFunctionWnd. Their handlers are the virtual slots
// 68 .. 71 (names ours). Layout and virtuals from the stock constructor 0x100E77C0, OnCreate 0x100E7270, OnDestroy
// 0x100E69F0, OnPaint 0x100E6B20 and the handlers 0x100E64F0 .. 0x100E6630.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCButton;

class NCReplayFunctionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCReplayFunctionWnd, NCWnd, 0, 0x102A8198)

    UTexture* BackTex;                  // +0xC8 "L2UI_CH3.LobbyWnd.lobby_menuback" (120 x 138)
    NCButton* ReplayButton;             // +0xCC id 0xAFF22011, SysString 1023 (names ours)
    NCButton* BenchmarkButton;          // +0xD0 id 0xAFF22012, SysString 1022
    NCButton* OptionsButton;            // +0xD4 id 0xAFF22013, SysString 156
    NCButton* ExitButton;               // +0xD8 id 0xAFF22014, SysString 153

    NCReplayFunctionWnd();                                           // 0x100E77C0
    // ~NCReplayFunctionWnd                                          // 0x100E7830

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100E64F0 the ids go to the four handlers
    virtual INT OnCreate();                                          // 37 0x100E7270
    virtual INT OnDestroy();                                         // 38 0x100E69F0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100E6B20
    virtual INT OnReplayBtnClick();                                  // 68 0x100E65A0 (name ours) the replay starts (state 0)
    virtual INT OnBenchmarkBtnClick();                               // 69 0x100E65D0 (name ours) the benchmark starts
    virtual INT OnOptionsBtnClick();                                 // 70 0x100E6630 (name ours) the options window
    virtual INT OnExitBtnClick();                                    // 71 0x10092210 (name ours; folded with NCAuthWnd::OnCancel) the client ends
};
NW_ASSERT_OFFSET(NCReplayFunctionWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCReplayFunctionWnd, ExitButton, 0xD8);
NW_ASSERT_SIZE(NCReplayFunctionWnd, 0xDC);

#pragma pack(pop)
