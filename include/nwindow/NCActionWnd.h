#pragma once
// NCActionWnd - the action window (UClass "CActionWnd", 0xA8; a page of the character window,
// desktop +0x94 +0x98): tabs Basic (the action icons, NCActionItemListWnd) and Clan
// (NCPledgeWnd). The Clan tab is disabled without a clan; shown with one, the window asks the
// server for the member list.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCActionItemListWnd;
class NCPledgeWnd;
class NCTabControl;

enum { NID_ACTION_TABS = 0xF00A };

class NCActionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCActionWnd, NCWnd, 0, 0x102780C8)

    UTexture* BackTex;                     // +0xC8 "sek.cbui304" (loaded, never drawn)   // C2: offset derived, no evidence
    NCActionItemListWnd* ActionList;       // +0xCC
    NCPledgeWnd* PledgeWnd;                // +0xD0
    // C2: not placed (no evidence for where it went): NCTabControl* TabCtrl;                 // +0xA4
    // C2: not placed (no evidence for where it went): NCWnd* UnusedA4;                       // +0xA8 destroyed, never created   // C2: offset derived, no evidence

    NCActionWnd();                                                   // 0x1004aed0
    // ~NCActionWnd                                                  // 0x10042b60 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x1004af70
    virtual INT OnDestroy();                                         // 38 0x10049170 (folded)
    virtual void ShowWindow();                                       // 44 0x1004b1b0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100490d0

    // names ours
    void ResetPledgeWnd();                                           // 0x10036870 (C1 only: no C2 match) (C1 only: no C2 match) (the console)
    void ReloadActions();                                            // 0x1003e180 (C1 only: no C2 match) texts reloaded
};
NW_ASSERT_OFFSET(NCActionWnd, BackTex, 0xC8);
// NW_ASSERT_OFFSET(NCActionWnd, TabCtrl, 0xA4);   // C2: field not mapped
NW_ASSERT_SIZE(NCActionWnd, 0xD4);

#pragma pack(pop)