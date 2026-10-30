#pragma once
// L2FriendTreeCtrl - the scrolling tree of the Lineage II friend list (UClass "CL2FriendTreeCtrl", 0x11C; C3 only, no C2
// source): like MsnTreeCtrl, an NCTreeCtrl that keeps its own tree (L2FriendTree, at +0x118; the base's +0x100 stays empty)
// and repeats the base's virtuals on that field. L2FriendWnd makes it.
//
// Layout and virtuals from the stock constructor 0x10143A00, OnCreate 0x10143AA0, OnDestroy 0x10142210, OnPaint 0x10142250,
// Clear 0x10142330, ScrollUp / ScrollDown / OnScrollBar / OnMouseWheel 0x101423C0 .. 0x101425D0, WindowProc 0x10141B60.

#include "nwindow/NCTreeCtrl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class L2FriendTree;

class L2FriendTreeCtrl : public NCTreeCtrl
{
    NW_DECLARE_CLASS(L2FriendTreeCtrl, NCTreeCtrl, 0, 0x102C6060)

    INT Unknown114;                  // +0x114 the constructor clears it; nothing uses it
    L2FriendTree* FriendTree;        // +0x118 id 0xA01, as wide as the control less the scroll bar (name ours)

    L2FriendTreeCtrl();                                              // 0x10143A00
    // ~L2FriendTreeCtrl                                             // 0x10143A50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10141B60 (also MsnTreeCtrl's)
    virtual INT OnCreate();                                          // 37 0x10143AA0
    virtual INT OnDestroy();                                         // 38 0x10142210
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10142250
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x101425D0
    virtual INT ScrollUp();                                          // 69 0x101423C0
    virtual INT ScrollDown();                                        // 70 0x10142480
    virtual INT OnScrollBar(INT Value);                              // 71 0x10142550
    virtual void Clear();                                            // 72 0x10142330

    // name ours (anonymous in the db; ret 8): a double click is passed up to the parent's parent (both tree controls)
    __declspec(noinline) INT OnDoubleClick(WPARAM wParam, LPARAM lParam);   // 0x10141B10
};
NW_ASSERT_OFFSET(L2FriendTreeCtrl, FriendTree, 0x118);
NW_ASSERT_SIZE(L2FriendTreeCtrl, 0x11C);

#pragma pack(pop)
