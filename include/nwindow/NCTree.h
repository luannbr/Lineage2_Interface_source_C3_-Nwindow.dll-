#pragma once
// NCTree - a tree view (UClass "CTree", 0x1FC): the root NCTVItem and, under it, the others. They are no windows any
// more (C3): plain objects the paint puts in place (DrawItem lays them out one under the other, the children of an
// expanded entry indented, and returns the height used) and that a click on the entry's picture or text opens and
// closes. NCTreeCtrl holds one inside a scrolling window; the messenger's MsnTree (and its FriendTree) derive from it
// with their own DrawItem. The slots 69 .. 71 are the C2 NCTree's AddItem / Clear / DrawItem (there 68 .. 71, with
// DeleteItem between them: it is no slot in C3).

#include "nwindow/NCTVItem.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCGDevice;

class NCTree : public NControl
{
    NW_DECLARE_CLASS(NCTree, NControl, 0, 0x10270638)

    NCTVItem* Root;                  // +0x1D4 NULL until the first AddItem without a parent (name ours)
    INT Unknown1D8;                  // +0x1D8 never touched
    NCTVItem* Unknown1DC;            // +0x1DC (0) cleared when the entry it holds is deleted
    NCTVItem* Unknown1E0;            // +0x1E0 (0) the same
    NCTVItem* Selected;              // +0x1E4 (0) the entry the last click chose (name ours)
    NCTVItem* Hover;                 // +0x1E8 (0) the entry under the mouse, or NULL (name ours)
    INT ContentHeight;               // +0x1EC what DrawItem returned in the last paint (name ours)
    INT ScrollOffset;                // +0x1F0 how far NCTreeCtrl moved it up (the paint's clip follows) (name ours)
    UTexture* SelectTex;             // +0x1F4 "L2UI_CH3.ListCtrl.TextSelect": the selection bar's left part
    UTexture* SelectTex2;            // +0x1F8 "L2UI_CH3.ListCtrl.TextSelect2": the right end

    NCTree();                                                        // 0x1002F770
    // ~NCTree                                                       // 0x1002F850

    virtual INT OnDestroy();                                         // 38 0x1002E250 (the entries go)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1002E630
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x1002E8B0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x1002E830
    // names ours (the C2 NCTree's)
    virtual NCTVItem* AddItem(NTVItemData* Data, NCTVItem* ParentItem);   // 69 0x1002F5F0 the root when ParentItem is
                                                                     //   NULL (only the first), else under ParentItem;
                                                                     //   NULL: not added
    virtual void Clear();                                            // 70 0x1002F740 everything
    virtual INT DrawItem(NCGDevice* Device, NCTVItem* Item, INT X, INT Y);   // 71 0x1002EF30 the height used

    void DeleteItem(NCTVItem** Item);                                // 0x1002F6B0 (ret 4, name ours) the entry and what
                                                                     //   is under it; its place in the tree is cleared
    NCTVItem* FindItemAt(INT X, INT Y, NCTVItem* Item);              // 0x1002E740 (ret 0xC, name ours) the entry whose
                                                                     //   picture or text holds the point (the first of
                                                                     //   those under Item that is visible), or NULL

};
NW_ASSERT_OFFSET(NCTree, Root, 0x1D4);
NW_ASSERT_OFFSET(NCTree, Selected, 0x1E4);
NW_ASSERT_OFFSET(NCTree, Hover, 0x1E8);
NW_ASSERT_OFFSET(NCTree, ContentHeight, 0x1EC);
NW_ASSERT_OFFSET(NCTree, ScrollOffset, 0x1F0);
NW_ASSERT_OFFSET(NCTree, SelectTex, 0x1F4);
NW_ASSERT_SIZE(NCTree, 0x1FC);

#pragma pack(pop)
