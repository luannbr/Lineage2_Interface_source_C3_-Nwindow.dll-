#pragma once
// NCQuestGaraTree - the quest list of the main window's quest tab (UClass "CQuestGaraTree", 0x228; new in C3): an
// NCGaraTree whose entries are the quests (the groups) and their steps, drawn on the three panels of the quest
// window with the five info icons, the selected one on the "TextSelect" bar. DrawItem is its own (the stock one is
// 2851 bytes).

#include "nwindow/NCGaraTree.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCQuestGaraTree : public NCGaraTree
{
    NW_DECLARE_CLASS(NCQuestGaraTree, NCGaraTree, 0, 0x102A1A98)

    UTexture* Panel1Tex;             // +0x1EC "L2UI_CH3.QUESTWND.QUESTWNDPANEL1" (name ours)
    UTexture* Panel2Tex;             // +0x1F0 "...PANEL2"
    UTexture* Panel3Tex;             // +0x1F4 "...PANEL3"
    UTexture* OutlineTex;            // +0x1F8 "L2UI_CH3.Etc.menu_outline"
    UTexture* SelectTex;             // +0x1FC "L2UI_CH3.ListCtrl.TextSelect"
    UTexture* SelectTex2;            // +0x200 "L2UI_CH3.ListCtrl.TextSelect2"
    NCGaraTVItem* SelectedItem;      // +0x204 the entry that was clicked last (NCGaraTVItem::OnLButtonDown sets it)
    UTexture* InfoIconTex[5];        // +0x208 "L2UI_CH3.QUESTWND.QuestWndInfoIcon_1" .. "_5"
    BYTE Unknown21C;                 // +0x21C (0)
    BYTE _Unknown21D[0x228 - 0x21D]; // +0x21D never touched

    NCQuestGaraTree();                                               // 0x100DC070
    // ~NCQuestGaraTree                                              // 0x100DC0C0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10041760 (folded with NCExButton's)
    virtual INT OnCreate();                                          // 37 0x100DA190
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100DA440
    virtual INT DrawItem(NCGDevice* Device, NCGaraTVItem* Item, INT X, INT Y);   // 72 0x100DA530
};
NW_ASSERT_OFFSET(NCQuestGaraTree, Panel1Tex, 0x1EC);
NW_ASSERT_OFFSET(NCQuestGaraTree, SelectedItem, 0x204);
NW_ASSERT_OFFSET(NCQuestGaraTree, InfoIconTex, 0x208);
NW_ASSERT_OFFSET(NCQuestGaraTree, Unknown21C, 0x21C);
NW_ASSERT_SIZE(NCQuestGaraTree, 0x228);

#pragma pack(pop)
