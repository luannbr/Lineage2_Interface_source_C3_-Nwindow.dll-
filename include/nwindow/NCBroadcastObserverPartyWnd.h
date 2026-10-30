#pragma once
// NCBroadcastObserverPartyWnd - one player's row in a team's party list of the broadcast observer screen (UClass
// "CBroadcastObserverPartyWnd", 0x114; C3 only; NCBroadcastObserverPartyListWnd +0xC8..): a 280 x 70 banner (three
// pieces, a darker one for a dead player), the team's emblem, the class or name of the player and his CP / HP / MP
// bars. A click passes to the parent list and selects the player (the owner follows him).
// Layout and virtuals from the stock constructor 0x10115F20, OnCreate 0x10115360, OnDestroy 0x101155A0, the mouse
// handlers 0x10115620 .. 0x10115780, GetUser 0x10115820, IsSelected 0x101158C0 and OnPaint 0x10115960.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCBroadcastObserverPartyListWnd;
struct NBroadcastUser;

class NCBroadcastObserverPartyWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCBroadcastObserverPartyWnd, NCWnd, 0, 0x102B9788)

    UTexture* BackTex[3];            // +0xC8 "L2UI_CH3.BroadcastObs.br_party2_back1" .. "3": the banner (names ours)
    UTexture* DeadBackTex[3];        // +0xD4 "br_dead2_back1" .. "3": the banner of a dead player
    UTexture* HpBarTex;              // +0xE0 "br_bar2_hp"
    UTexture* MpBarTex;              // +0xE4 "br_bar2_mp"
    UTexture* CpBarTex;              // +0xE8 "br_bar2_cp"
    UTexture* HpBackTex;             // +0xEC "br_bar2back_hp"
    UTexture* MpBackTex;             // +0xF0 "br_bar2back_mp"
    UTexture* CpBackTex;             // +0xF4 "br_bar2back_cp"
    UTexture* TeamTex;               // +0xF8 "br_party1_<slot+1>" (first team) / "br_party2_<slot+1>" (second): the emblem
    UTexture* DeadTeamTex;           // +0xFC "br_party_X": the emblem of a dead player
    UTexture* SelectTex[3];          // +0x100 "br_partyon_back1" .. "3": the overlay of the selected row
    INT Slot;                        // +0x10C the roster slot (0..8)
    NCBroadcastObserverPartyListWnd* ParentList; // +0x110 the list (and, through it, the owner)

    NCBroadcastObserverPartyWnd(NCBroadcastObserverPartyListWnd* InParent = NULL, INT InSlot = 0); // 0x10115F20 (ret 8); defaults = what the stock InternalConstructor pushes
    // ~NCBroadcastObserverPartyWnd                                  // 0x10116020

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (NControl's body, folded)
    virtual INT OnCreate();                                          // 37 0x10115360 (guard: NCBroadcastObserverWnd::OnCreate)
    virtual INT OnDestroy();                                         // 38 0x101155A0 (guard: NCBroadcastObserverWnd::OnDestroy)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10115960 (guard: NCBroadcastObserverWnd::OnPaint)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x10115780
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x10115620
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x101156E0

    NBroadcastUser* GetUser();                                       // 0x10115820 the row's roster line (the list's)
    INT IsSelected();                                                // 0x101158C0
};
NW_ASSERT_OFFSET(NCBroadcastObserverPartyWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCBroadcastObserverPartyWnd, DeadBackTex, 0xD4);
NW_ASSERT_OFFSET(NCBroadcastObserverPartyWnd, HpBarTex, 0xE0);
NW_ASSERT_OFFSET(NCBroadcastObserverPartyWnd, TeamTex, 0xF8);
NW_ASSERT_OFFSET(NCBroadcastObserverPartyWnd, SelectTex, 0x100);
NW_ASSERT_OFFSET(NCBroadcastObserverPartyWnd, Slot, 0x10C);
NW_ASSERT_OFFSET(NCBroadcastObserverPartyWnd, ParentList, 0x110);
NW_ASSERT_SIZE(NCBroadcastObserverPartyWnd, 0x114);

#pragma pack(pop)
