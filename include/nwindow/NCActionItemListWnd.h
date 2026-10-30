#pragma once
// NCActionItemListWnd - the Basic page of the action window (UClass "CActionItemListWnd",
// 0xA4): three icon grids, filled from actionname-*.dat by category (1 basic, 2 party,
// 3 social), under the labels "Party" and "Social".

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCActionItemWnd;

class NCActionItemListWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCActionItemListWnd, NCWnd, 0, 0x10279038)

    NCActionItemWnd* BasicGrid;            // +0xC8 6 x 2   // C2: offset derived, no evidence
    NCActionItemWnd* PartyGrid;            // +0xCC 6 x 1   // C2: offset derived, no evidence
    NCActionItemWnd* SocialGrid;           // +0xD0 6 x 3   // C2: offset derived, no evidence
    UTexture* BackTex;                     // +0xD4 "L2UI.ActionWnd.ActionBack"   // C2: offset derived, no evidence

    NCActionItemListWnd();                                           // 0x1004ab60
    // ~NCActionItemListWnd                                          // 0x10040d20 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x1004ac00
    virtual INT OnDestroy();                                         // 38 0x10047ac0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100479c0

    void LoadActions();                                              // 0x1004a660 (name ours; stock)
    void ClearGrids();                                               // 0x1004ade0 (name ours)
};
NW_ASSERT_OFFSET(NCActionItemListWnd, BasicGrid, 0xC8);
NW_ASSERT_OFFSET(NCActionItemListWnd, BackTex, 0xD4);
NW_ASSERT_SIZE(NCActionItemListWnd, 0xD8);

#pragma pack(pop)