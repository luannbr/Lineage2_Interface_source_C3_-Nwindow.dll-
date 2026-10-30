#pragma once
// NCSSQSealStatusListWnd - the Seven Signs seal list tab (UClass "CSSQSealStatusListWnd", 0xD4; new in C3): the
// tree of the seals (NCSSQSealStatusGaraTreeCtrl, 238 x 304) on the "ssq_back2" panel and a button that asks the
// console for the list again. The same shape as the event tab with the control and the panel swapped.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCSSQSealStatusGaraTreeCtrl;

class NCSSQSealStatusListWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSSQSealStatusListWnd, NCWnd, 0, 0x102ABA20)

    NCSSQSealStatusGaraTreeCtrl* TreeCtrl;   // +0xC8 (9, 11, 238 x 304) (name ours)
    UTexture* BackTex;               // +0xCC "L2UI_CH3.SSQWnd.ssq_back2"
    NCButton* RefreshButton;         // +0xD0 id 0xFF00F0A0, text 0x3AA (name ours)

    NCSSQSealStatusListWnd();                                        // 0x100ED200
    // ~NCSSQSealStatusListWnd                                       // 0x100ED250

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100EBAF0
    virtual INT OnCreate();                                          // 37 0x100ED2A0
    virtual INT OnDestroy();                                         // 38 0x100ECDF0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100ECEC0
    virtual INT OnRefresh();                                         // 68 0x100EBB60 (name ours) asks for the list

    void ClearList();                                                // 0x100EBB50 (name ours) the tree emptied
};
NW_ASSERT_OFFSET(NCSSQSealStatusListWnd, TreeCtrl, 0xC8);
NW_ASSERT_OFFSET(NCSSQSealStatusListWnd, RefreshButton, 0xD0);
NW_ASSERT_SIZE(NCSSQSealStatusListWnd, 0xD4);

#pragma pack(pop)
