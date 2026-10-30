#pragma once
// NCQuestItemWnd - the quest window's "Item" page (UClass "CQuestItemWnd", 0x118): the quest
// items, 6 x 4 icons of 32 pixels (at most 80). The selected item's tool tip lists its name
// (enchant, count), slot and description. An item dropped on the quest window's drop zone
// asks whether to destroy it (how many, when stacked).

#include "nwindow/NCItemWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCQuestItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCQuestItemWnd, NCItemWnd, 0, 0x102A2A08)

    NCQuestItemWnd();                                                // 0x100ddd30
    NCQuestItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x100dddd0
    // ~NCQuestItemWnd                                               // 0x100ddd80

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100dc990
    virtual INT OnCreate();                                          // 37 0x100dc9f0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100dd0e0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100dd2a0
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 74 0x1001cce0
    // names ours
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 75 0x100dd4e0

    // names ours
};
NW_ASSERT_SIZE(NCQuestItemWnd, 0x148);

#pragma pack(pop)