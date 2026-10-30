#pragma once
// NCBroadcastObserverTopWnd - the score bar at the top of the broadcast observer screen (UClass
// "CBroadcastObserverTopWnd", 0x11C; C3 only; NCBroadcastObserverWnd +0xC8): a wide band of six textures with the
// two teams' names and scores (one big digit each, from the "br_score0".."9" textures) around a colon. It slides in
// from above when the score changes, stays five seconds and slides out again (three timers).
// Layout and virtuals from the stock constructor 0x10117AB0, OnCreate 0x10117120, OnDestroy 0x10117260, ShowWindow
// 0x10117450, OnTimer 0x10117950, OnPaint 0x10117520 and the two getters 0x101172E0 / 0x101173A0.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCBroadcastObserverWnd;

class NCBroadcastObserverTopWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCBroadcastObserverTopWnd, NCWnd, 0, 0x102BA700)

    UTexture* BackTex[6];            // +0xC8 "L2UI_CH3.BroadcastObs.br_top_back1" .. "6" (names ours): the left end, the
                                     //   stretched middle, the right end of the first team's part, a divider, ...
    UTexture* ScoreTex[10];          // +0xE0 "br_score0" .. "br_score9": the score digits (128 x 128)
    UTexture* ColonTex;              // +0x108 "br_scorecolon"
    NCBroadcastObserverWnd* Owner;   // +0x10C the observer window (the rosters and scores are its)
    INT EndTimerID;                  // +0x110 (name ours) cleared by the slide-out's last tick, never set
    INT HoldTimerID;                 // +0x114 the five second wait (0xFE0002)
    INT SlideTimerID;                // +0x118 the slide (0xFE0000 down, 0xFE0001 up), one millisecond per step

    NCBroadcastObserverTopWnd(NCBroadcastObserverWnd* InOwner = NULL); // 0x10117AB0 (ret 4); defaults = what the stock InternalConstructor pushes
    // ~NCBroadcastObserverTopWnd                                    // 0x10117B80

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (NControl's body, folded)
    virtual INT OnTimer(INT TimerID);                                // 36 0x10117950
    virtual INT OnCreate();                                          // 37 0x10117120 (guard: NCBroadcastObserverWnd::OnCreate)
    virtual INT OnDestroy();                                         // 38 0x10117260 (guard: NCBroadcastObserverWnd::OnDestroy)
    virtual void ShowWindow();                                       // 44 0x10117450 the slide starts
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10117520 (guard: NCBroadcastObserverWnd::OnPaint)

    UTexture* GetScore(INT Team);                                    // 0x101172E0 (ret 4) the team's score digit (0 when it has two)
    FString GetTeamName(INT Team);                                   // 0x101173A0 (ret 8) the owner's name of the team ("" without one)
};
NW_ASSERT_OFFSET(NCBroadcastObserverTopWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCBroadcastObserverTopWnd, ScoreTex, 0xE0);
NW_ASSERT_OFFSET(NCBroadcastObserverTopWnd, ColonTex, 0x108);
NW_ASSERT_OFFSET(NCBroadcastObserverTopWnd, Owner, 0x10C);
NW_ASSERT_OFFSET(NCBroadcastObserverTopWnd, SlideTimerID, 0x118);
NW_ASSERT_SIZE(NCBroadcastObserverTopWnd, 0x11C);

#pragma pack(pop)
