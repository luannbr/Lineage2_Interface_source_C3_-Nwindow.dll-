#pragma once
// NCQuestGaraTreeCtrl - the scrolling window of the quest tree (UClass "CQuestGaraTreeCtrl", 0x120; new in C3):
// NCGaraTreeCtrl's code repeated with the quest tree at +0x114 (the base's +0x100 tree is never made).

#include "nwindow/NCGaraTreeCtrl.h"
#include "nwindow/NCQuestGaraTree.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCQuestGaraTreeCtrl : public NCGaraTreeCtrl
{
    NW_DECLARE_CLASS(NCQuestGaraTreeCtrl, NCGaraTreeCtrl, 0, 0x102A1578)

    NCQuestGaraTree* QuestTree;      // +0x114 (id 0xA00) (name ours)
    NCGaraTVItem* RootItem;          // +0x118 the root entry NConsoleWnd makes at the start of a quest list (0x100DB380)
    INT SelectedID;                  // +0x11C the quest the console marked as selected (an entry's +0x1A08)

    NCQuestGaraTreeCtrl();                                           // 0x100DC110
    // ~NCQuestGaraTreeCtrl                                          // 0x100DC170

    virtual INT OnCreate();                                          // 37 0x100DC1C0
    virtual INT OnDestroy();                                         // 38 0x100EAF80
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100DB070
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x100E3ED0
    virtual INT ScrollUp();                                          // 69 0x100ECAD0
    virtual INT ScrollDown();                                        // 70 0x100DB2B0
    virtual INT OnScrollBar(INT Value);                              // 71 0x100ECB90
    virtual void ResetScroll();                                      // 72 0x100DB200 (the base's with the quest tree)
    virtual void ArrangeItem(NCGaraTVItem* Item);                    // 73 0x100ECA00

    // 0x100DB1A0 (name ours, from NCGMWnd.cpp's view struct): the id of the quest the tree has marked (the first entry
    // under the root whose bClicked is set), -1 when there is none
    __declspec(noinline) INT FindMarkedQuest();
};
NW_ASSERT_OFFSET(NCQuestGaraTreeCtrl, QuestTree, 0x114);
NW_ASSERT_OFFSET(NCQuestGaraTreeCtrl, RootItem, 0x118);
NW_ASSERT_OFFSET(NCQuestGaraTreeCtrl, SelectedID, 0x11C);
NW_ASSERT_SIZE(NCQuestGaraTreeCtrl, 0x120);

#pragma pack(pop)
