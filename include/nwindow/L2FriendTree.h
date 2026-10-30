#pragma once
// L2FriendTree - the tree of the Lineage II friend list (UClass "CL2FriendTree", 0x1FC; C3 only, no C2 source): an NCTree with
// no fields of its own, drawn by its own DrawItem. L2FriendTreeCtrl makes it (id 0xA01); the messenger's person tree
// (MsnTree) shares its window procedure and its double click.
//
// Virtuals from the stock constructor 0x10143960, WindowProc 0x101421B0 (shared with MsnTree), the double click 0x1012FA40 and
// DrawItem 0x10142BC0.

#include "nwindow/NCTree.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class L2FriendTree : public NCTree
{
    NW_DECLARE_CLASS(L2FriendTree, NCTree, 0, 0x102C6AA8)

    L2FriendTree();                                                  // 0x10143960
    // ~L2FriendTree                                                 // 0x101439B0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x101421B0 (also MsnTree's)
    virtual INT OnCreate();                                          // 37 0x10141B50 (NControl's, then 1; shared)
    virtual INT DrawItem(NCGDevice* Device, NCTVItem* Item, INT X, INT Y);   // 71 0x10142BC0

    // name ours (anonymous in the db; ret 8): a double click is told to the parent, at the position in this window's coordinates
    __declspec(noinline) INT OnDoubleClick(WPARAM wParam, LPARAM lParam);   // 0x1012FA40 (also MsnTree's)
};
NW_ASSERT_SIZE(L2FriendTree, 0x1FC);

#pragma pack(pop)
