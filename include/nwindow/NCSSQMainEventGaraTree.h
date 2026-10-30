#pragma once
// NCSSQMainEventGaraTree - the Seven Signs event list (UClass "CSSQMainEventGaraTree", 0x20C; new in C3): the
// quest tree's shape with the Seven Signs panels; its DrawItem is its own. NCSSQMainEventWnd holds one inside an
// NCSSQMainEventGaraTreeCtrl.

#include "nwindow/NCGaraTree.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCSSQMainEventGaraTree : public NCGaraTree
{
    NW_DECLARE_CLASS(NCSSQMainEventGaraTree, NCGaraTree, 0, 0x102AA590)

    UTexture* Panel1Tex;             // +0x1EC "L2UI_CH3.QUESTWND.QUESTWNDPANEL1" (names ours)
    UTexture* Panel2Tex;             // +0x1F0 "...PANEL2"
    UTexture* Panel3Tex;             // +0x1F4 "...PANEL3"
    UTexture* LineTex;               // +0x1F8 "L2UI_CH3.Etc.TextBackline"
    UTexture* SelectTex;             // +0x1FC "L2UI_CH3.ListCtrl.TextSelect"
    UTexture* SelectTex2;            // +0x200 "L2UI_CH3.ListCtrl.TextSelect2"
    BYTE _Unknown204[0x20C - 0x204]; // +0x204 never touched

    NCSSQMainEventGaraTree();                                        // 0x100EB5A0
    // ~NCSSQMainEventGaraTree                                       // 0x100EB5F0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10041760 (folded with NCExButton's)
    virtual INT OnCreate();                                          // 37 0x100EA260
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100EA370
    virtual INT DrawItem(NCGDevice* Device, NCGaraTVItem* Item, INT X, INT Y);   // 72 0x100EA460
};
NW_ASSERT_OFFSET(NCSSQMainEventGaraTree, Panel1Tex, 0x1EC);
NW_ASSERT_OFFSET(NCSSQMainEventGaraTree, SelectTex2, 0x200);
NW_ASSERT_SIZE(NCSSQMainEventGaraTree, 0x20C);

#pragma pack(pop)
