#pragma once
// L2ChatFrameCtrl - ChatFrameCtrl's Lineage II skin (UClass "L2ChatFrameCtrl", 0x238; C3 only, the messenger): the title
// bar of L2ChatWnd. Its buttons tell the parent like ChatFrameCtrl's; the bar itself is painted by NCFrameCtrl.

#include "nwindow/NCFrameCtrl.h"
#pragma pack(push, 4)

class L2ChatFrameCtrl : public NCFrameCtrl
{
    NW_DECLARE_CLASS(L2ChatFrameCtrl, NCFrameCtrl, 0, 0x102C1D68)

    L2ChatFrameCtrl();                                               // 0x101258A0
    L2ChatFrameCtrl(INT InDecorated, INT InVertical, INT InSmall);   // 0x10125940 (anonymous in the db) (ret 0xC)
    // ~L2ChatFrameCtrl                                              // 0x101258F0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119AA0 (ChatFrameCtrl's, shared)
    virtual INT OnClose();                                           // 70 0x10124210 (ChatFrameCtrl's, shared)
    virtual INT OnMinimize();                                        // 71 0x101241E0 (ChatFrameCtrl's, shared)
};
NW_ASSERT_SIZE(L2ChatFrameCtrl, 0x238);

#pragma pack(pop)
