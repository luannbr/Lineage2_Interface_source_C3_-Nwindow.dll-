#pragma once
// NCBroadcastObserverPlayerWnd - the followed player of a team in the broadcast observer screen (UClass
// "CBroadcastObserverPlayerWnd", 0x108; C3 only; NCBroadcastObserverWnd +0xCC / +0xD0): a 520 x 224 banner with his
// level, class and name, the CP / HP / MP bars (500 pixels full) and, in the corner, the window of the effects on him
// (NCBroadcastObserverBuffWnd). Which player of the team it shows is the slot SetUserIndex gives it.
// Layout and virtuals from the stock constructor 0x10116920, OnCreate 0x101166E0, OnDestroy 0x101161B0, OnPaint
// 0x101162C0 and the small methods 0x101160A0 .. 0x10116230.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCBroadcastObserverWnd;
class NCBroadcastObserverBuffWnd;
struct NBroadcastUser;

class NCBroadcastObserverPlayerWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCBroadcastObserverPlayerWnd, NCWnd, 0, 0x102B9CA8)

    UTexture* BackTex[3];            // +0xC8 "L2UI_CH3.BroadcastObs.br_party1_back1" .. "3": the banner (names ours)
    UTexture* DeadBackTex[3];        // +0xD4 "br_dead1_back1" .. "3": the banner of a dead player
    UTexture* HpBarTex;              // +0xE0 "br_bar1_hp"
    UTexture* MpBarTex;              // +0xE4 "br_bar1_mp"
    UTexture* CpBarTex;              // +0xE8 "br_bar1_cp"
    UTexture* HpBackTex;             // +0xEC "br_bar1back_hp"
    UTexture* MpBackTex;             // +0xF0 "br_bar1back_mp"
    UTexture* CpBackTex;             // +0xF4 "br_bar1back_cp"
    NCBroadcastObserverBuffWnd* BuffWnd; // +0xF8 the effects on the player (400 x 80 in the corner)
    INT UserSlot;                    // +0xFC the roster slot shown (-1: none)
    INT Team;                        // +0x100 0 or 1
    NCBroadcastObserverWnd* Owner;   // +0x104 the observer window (the roster is its)

    NCBroadcastObserverPlayerWnd(NCBroadcastObserverWnd* InOwner = NULL, INT InTeam = 0); // 0x10116920 (ret 8); defaults = what the stock InternalConstructor pushes
    // ~NCBroadcastObserverPlayerWnd                                 // 0x101169F0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (NControl's body, folded)
    virtual INT OnCreate();                                          // 37 0x101166E0 (guard: NCBroadcastObserverWnd::OnCreate)
    virtual INT OnDestroy();                                         // 38 0x101161B0 (guard: NCBroadcastObserverWnd::OnDestroy)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x101162C0 (guard: NCBroadcastObserverWnd::OnPaint)

    void RefreshBuffWnd();                                           // 0x101160A0 the effects window reads the player again
    NBroadcastUser* GetUser();                                       // 0x10116120 the roster line of the shown player
    void SetUserIndex(INT Slot);                                     // 0x10116230 (ret 4) show another player of the team
};
NW_ASSERT_OFFSET(NCBroadcastObserverPlayerWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCBroadcastObserverPlayerWnd, DeadBackTex, 0xD4);
NW_ASSERT_OFFSET(NCBroadcastObserverPlayerWnd, HpBarTex, 0xE0);
NW_ASSERT_OFFSET(NCBroadcastObserverPlayerWnd, BuffWnd, 0xF8);
NW_ASSERT_OFFSET(NCBroadcastObserverPlayerWnd, UserSlot, 0xFC);
NW_ASSERT_OFFSET(NCBroadcastObserverPlayerWnd, Team, 0x100);
NW_ASSERT_OFFSET(NCBroadcastObserverPlayerWnd, Owner, 0x104);
NW_ASSERT_SIZE(NCBroadcastObserverPlayerWnd, 0x108);

#pragma pack(pop)
