#pragma once
// NCBroadcastObserverSpecialMsgWnd - the big banners of the broadcast observer screen (UClass
// "CBroadcastObserverSpecialMsgWnd", 0xFC; C3 only): the match's "start", "finish" and "game over" pictures and the
// 5..1 countdown, one at a time (Mode), over a three-piece band. The console sets Mode and shows the window; a timer
// hides it again (1 second for the start picture, 5 seconds for the others).
// Layout and virtuals from the stock constructor 0x10117000, OnCreate 0x10116B30, OnDestroy 0x10116C70, ShowWindow
// 0x10116A60, OnTimer 0x10116CF0 and OnPaint 0x10116D90.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

// NCBroadcastObserverSpecialMsgWnd::Mode (names ours)
enum
{
    SPECMSG_Finish = 1,       // "br_msg1_finish" 512 x 256
    SPECMSG_Start = 2,        // "br_msg1_start" 512 x 512 (the constructor's value)
    SPECMSG_GameOver = 3,     // "br_msg1_gameover" 512 x 256
    SPECMSG_FirstCount = 4,   // "br_msg1_count1" .. "count5": 256 x 256 (Mode 4 .. 8)
};

class NCBroadcastObserverSpecialMsgWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCBroadcastObserverSpecialMsgWnd, NCWnd, 0, 0x102BA1D8)

    UTexture* BandTex[3];            // +0xC8 "L2UI_CH3.BroadcastObs.br_msg1_back1" .. "3": left end, middle, right end (names ours)
    UTexture* StartTex;              // +0xD4 "L2UI_CH3.BroadcastObs.br_msg1_start"
    UTexture* FinishTex;             // +0xD8 "br_msg1_finish"
    UTexture* GameOverTex;           // +0xDC "br_msg1_gameover"
    UTexture* CountTex[5];           // +0xE0 "br_msg1_count1" .. "5"
    INT Mode;                        // +0xF4 what OnPaint draws (SPECMSG_*; 2 from the constructor)
    INT TimerID;                     // +0xF8 the hide timer (0xC0000)

    NCBroadcastObserverSpecialMsgWnd();                              // 0x10117000
    // ~NCBroadcastObserverSpecialMsgWnd                             // 0x101170C0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (NControl's body, folded)
    virtual INT OnTimer(INT TimerID);                                // 36 0x10116CF0
    virtual INT OnCreate();                                          // 37 0x10116B30 (guard: NCBroadcastObserverWnd::OnCreate)
    virtual INT OnDestroy();                                         // 38 0x10116C70 (guard: NCBroadcastObserverWnd::OnDestroy)
    virtual void ShowWindow();                                       // 44 0x10116A60
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10116D90
};
NW_ASSERT_OFFSET(NCBroadcastObserverSpecialMsgWnd, BandTex, 0xC8);
NW_ASSERT_OFFSET(NCBroadcastObserverSpecialMsgWnd, StartTex, 0xD4);
NW_ASSERT_OFFSET(NCBroadcastObserverSpecialMsgWnd, CountTex, 0xE0);
NW_ASSERT_OFFSET(NCBroadcastObserverSpecialMsgWnd, Mode, 0xF4);
NW_ASSERT_OFFSET(NCBroadcastObserverSpecialMsgWnd, TimerID, 0xF8);
NW_ASSERT_SIZE(NCBroadcastObserverSpecialMsgWnd, 0xFC);

#pragma pack(pop)
