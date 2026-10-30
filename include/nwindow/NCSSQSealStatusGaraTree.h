#pragma once
// NCSSQSealStatusGaraTree - the Seven Signs seal status list (UClass "CSSQSealStatusGaraTree", 0x228; new in C3):
// the same shape with two more bars (the dusk's and the dawn's: "ssq_bar1x", "ssq_bar2x"); its DrawItem is its own.
// NCSSQSealStatusListWnd holds one inside an NCSSQSealStatusGaraTreeCtrl.

#include "nwindow/NCGaraTree.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCSSQSealStatusGaraTree : public NCGaraTree
{
    NW_DECLARE_CLASS(NCSSQSealStatusGaraTree, NCGaraTree, 0, 0x102AAFE0)

    UTexture* Panel1Tex;             // +0x1EC "L2UI_CH3.QUESTWND.QUESTWNDPANEL1" (names ours)
    UTexture* Panel2Tex;             // +0x1F0 "...PANEL2"
    UTexture* Panel3Tex;             // +0x1F4 "...PANEL3"
    UTexture* LineTex;               // +0x1F8 "L2UI_CH3.Etc.TextBackline"
    UTexture* SelectTex;             // +0x1FC "L2UI_CH3.ListCtrl.TextSelect"
    UTexture* SelectTex2;            // +0x200 "L2UI_CH3.ListCtrl.TextSelect2"
    UTexture* Bar21Tex;              // +0x204 "L2UI_CH3.SSQWnd.ssq_bar21"
    UTexture* Bar22Tex;              // +0x208 "...ssq_bar22"
    UTexture* Bar2BackTex;           // +0x20C "...ssq_bar2back"
    UTexture* Bar11Tex;              // +0x210 "...ssq_bar11"
    UTexture* Bar12Tex;              // +0x214 "...ssq_bar12"
    UTexture* Bar1BackTex;           // +0x218 "...ssq_bar1back"
    UTexture* BarLineTex;            // +0x21C "...ssq_barline"
    BYTE _Unknown220[0x228 - 0x220]; // +0x220 never touched

    NCSSQSealStatusGaraTree();                                       // 0x100ECFF0
    // ~NCSSQSealStatusGaraTree                                      // 0x100ED040

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10041760 (folded with NCExButton's)
    virtual INT OnCreate();                                          // 37 0x100EB9F0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100EBB80
    virtual INT DrawItem(NCGDevice* Device, NCGaraTVItem* Item, INT X, INT Y);   // 72 0x100EBC70
};
NW_ASSERT_OFFSET(NCSSQSealStatusGaraTree, Panel1Tex, 0x1EC);
NW_ASSERT_OFFSET(NCSSQSealStatusGaraTree, Bar21Tex, 0x204);
NW_ASSERT_OFFSET(NCSSQSealStatusGaraTree, BarLineTex, 0x21C);
NW_ASSERT_SIZE(NCSSQSealStatusGaraTree, 0x228);

#pragma pack(pop)
