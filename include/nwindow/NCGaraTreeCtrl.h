#pragma once
// NCGaraTreeCtrl - a scrolling window with one NCGaraTree inside (UClass "CGaraTreeCtrl", 0x114; new in C3): the
// tree is as wide as the window less the bar's 16 pixels and grows with its entries; the window's own scroll moves the
// tree up (NCGaraTree::ScrollOffset follows). NCQuestGaraTreeCtrl and the Seven Signs ones repeat the code with
// their own tree at +0x114 / +0x118.

#include "nwindow/NCGaraTree.h"
#include "nwindow/NCScrollWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCGaraTreeCtrl : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCGaraTreeCtrl, NCScrollWnd, 0, 0x10269B58)

    NCGaraTree* Tree;                // +0x100 (id 0xA00001) (name ours)
    BYTE _Unknown104[0x114 - 0x104]; // +0x104 never touched

    NCGaraTreeCtrl();                                                // 0x10013B20
    // ~NCGaraTreeCtrl                                               // 0x10013B70

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10093F30 (folded with NCItemWnd's)
    virtual INT OnCreate();                                          // 37 0x10013BC0
    virtual INT OnDestroy();                                         // 38 0x10012F30
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10012FE0
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x100133B0
    virtual INT ScrollUp();                                          // 69 0x100131A0
    virtual INT ScrollDown();                                        // 70 0x10013260
    virtual INT OnScrollBar(INT Value);                              // 71 0x10013330
    // names ours
    virtual void ResetScroll();                                      // 72 0x100134E0 the tree back at the top
    virtual void ArrangeItem(NCGaraTVItem* Item);                    // 73 0x100130D0 shows the entries that are inside
                                                                     //   the window, hides the others
};
NW_ASSERT_OFFSET(NCGaraTreeCtrl, Tree, 0x100);
NW_ASSERT_SIZE(NCGaraTreeCtrl, 0x114);

#pragma pack(pop)
