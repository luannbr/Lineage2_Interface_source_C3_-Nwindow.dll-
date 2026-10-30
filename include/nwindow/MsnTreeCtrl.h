#pragma once
// MsnTreeCtrl - the messenger's scrolling person tree (UClass "CMsnTreeCtrl", 0x128; C3 only, no C2 source): an NCTreeCtrl
// that keeps its own tree (MsnTree, at +0x124; the base's +0x100 stays empty), so every virtual the base has for the tree is
// repeated here on the other field. The messenger window makes it (0x10139A40) and the protocol object's notices reach it
// through the window (AddGroup, AddFriend, RemoveFriend).
//
// Layout and virtuals from the stock constructor 0x101438A0, OnCreate 0x10142810, OnDestroy 0x10141BC0, OnPaint 0x10141C00,
// Clear 0x10141CE0, ScrollUp / ScrollDown / OnScrollBar / OnMouseWheel 0x10141D70 .. 0x10141F80.

#include "nwindow/NCTreeCtrl.h"
#include "nwindow/MsnWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class MsnTree;

class MsnTreeCtrl : public NCTreeCtrl
{
    NW_DECLARE_CLASS(MsnTreeCtrl, NCTreeCtrl, 0, 0x102C6588)

    INT Unknown114;                  // +0x114 the constructor clears these four; nothing in the functions below uses them
    INT Unknown118;                  // +0x118
    void* FriendBook;                // +0x11C an object whose array at +4 holds the person records (RefreshGroup copies it); nothing in
                                     //   the functions of this file sets it (name ours)
    INT Unknown120;                  // +0x120
    MsnTree* MsnTreeWnd;             // +0x124 id 0xA00, as wide as the control less the scroll bar (name ours)

    MsnTreeCtrl();                                                   // 0x101438A0
    // ~MsnTreeCtrl                                                  // 0x10143910

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10141B60 (shared with L2FriendTreeCtrl)
    virtual INT OnCreate();                                          // 37 0x10142810
    virtual INT OnDestroy();                                         // 38 0x10141BC0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10141C00
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x10141F80
    virtual INT ScrollUp();                                          // 69 0x10141D70
    virtual INT ScrollDown();                                        // 70 0x10141E30
    virtual INT OnScrollBar(INT Value);                              // 71 0x10141F00
    virtual void Clear();                                            // 72 0x10141CE0

    // the protocol object's notices (names ours; the records are passed by value and freed by the callee)
    // (the two bodies are hooked as functions of their own, not through NW_HOOK_METHOD's forwarder, which would copy the
    // record once more into a frame of its own)
    static void __fastcall AddGroupBody(MsnTreeCtrl* Self, void* Unused, NMsnGroupArg Group);       // 0x101428D0 (ret 0x1C)
    static void __fastcall AddFriendBody(MsnTreeCtrl* Self, void* Unused, NMsnPersonRec Person); // 0x10142A40 (ret 0x44)
    void AddGroup(NMsnGroupArg Group) { AddGroupBody(this, NULL, Group); }
    void AddFriend(NMsnPersonRec Person) { AddFriendBody(this, NULL, Person); }
    __declspec(noinline) void RemoveFriend(NMsnStringArg Account);   // 0x10143520 (ret 0xC)
    __declspec(noinline) NCTVItem* FindFriendItem(const TCHAR* Account, NCTVItem* Item);   // 0x101420F0 (ret 8) the entry of this account, from Item down
    __declspec(noinline) void RefreshGroup(NCTVItem** Group);        // 0x10143320 (ret 4) the entries under the group again from the person list
};
NW_ASSERT_OFFSET(MsnTreeCtrl, MsnTreeWnd, 0x124);
NW_ASSERT_SIZE(MsnTreeCtrl, 0x128);

#pragma pack(pop)
