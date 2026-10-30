#pragma once
// NCTreeCtrl - a scrolling tree view (UClass "CTreeCtrl", 0xE4 in C2, 0x114 in C3): an NCScrollWnd with an
// NCTree (Tree) inside it, 16 pixels narrower than the window (the scroll bar's place). Scrolling
// moves the tree window and keeps its ScrollOffset (the tree's paint clips with it). The messenger's
// MsnTreeCtrl is built on it.

#include "nwindow/NCScrollWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCTree;
class NCTVItem;

class NCTreeCtrl : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCTreeCtrl, NCScrollWnd, 0, 0x10270B58)

    NCTree* Tree;                    // +0x100 id 0xA00001 (name ours)
    BYTE _UnknownD4[0xE4 - 0xD4];    // +0x104 never touched   // C2: offset derived, no evidence

    NCTreeCtrl();                                                    // 0x1002f8a0
    // ~NCTreeCtrl                                                   // 0x1002a9f0 (C1 only: no C2 match)

    virtual INT OnCreate();                                          // 37 0x1002f940
    virtual INT OnDestroy();                                         // 38 0x1002e980
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1002ea30
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x1002ed20
    virtual INT ScrollUp();                                          // 69 0x1002eb10
    virtual INT ScrollDown();                                        // 70 0x1002ebd0
    virtual INT OnScrollBar(INT Value);                              // 71 0x1002eca0
    virtual void Clear();                                            // 72 0x1002ee50 (name ours) the tree emptied, scrolled back
    // C3: ShowVisible is gone (the tree's own paint shows and hides its entries); NCRecipeTreeCtrl's header keeps its own
};
NW_ASSERT_OFFSET(NCTreeCtrl, Tree, 0x100);
NW_ASSERT_SIZE(NCTreeCtrl, 0x114);

#pragma pack(pop)
