#pragma once
// NCSSQMainEventWnd - the Seven Signs event list tab (UClass "CSSQMainEventWnd", 0xD4; new in C3): the tree of the
// events (NCSSQMainEventGaraTreeCtrl, 240 x 304) on the "ssq_back2" panel and a button that asks the console for
// the list again.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCSSQMainEventGaraTreeCtrl;

class NCSSQMainEventWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSSQMainEventWnd, NCWnd, 0, 0x102AA068)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.SSQWnd.ssq_back2"
    NCSSQMainEventGaraTreeCtrl* TreeCtrl;   // +0xCC (8, 11, 240 x 304) (name ours)
    NCButton* RefreshButton;         // +0xD0 id 0xFF00F0A1, text 0x3AA (name ours)

    NCSSQMainEventWnd();                                             // 0x100EB7B0
    // ~NCSSQMainEventWnd                                            // 0x100EB800

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100EA2E0
    virtual INT OnCreate();                                          // 37 0x100EB850
    virtual INT OnDestroy();                                         // 38 0x100EB490
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100EB4D0
    virtual INT OnRefresh();                                         // 68 0x100EA350 (name ours) asks for the list

    void ClearList();                                                // 0x100EA340 (name ours) the tree emptied
};
NW_ASSERT_OFFSET(NCSSQMainEventWnd, TreeCtrl, 0xCC);
NW_ASSERT_OFFSET(NCSSQMainEventWnd, RefreshButton, 0xD0);
NW_ASSERT_SIZE(NCSSQMainEventWnd, 0xD4);

#pragma pack(pop)
