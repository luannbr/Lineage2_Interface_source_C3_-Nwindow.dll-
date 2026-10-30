#pragma once
// L2FriendWnd - the Lineage II friend list page of the messenger (UClass "CL2FriendWnd", 0x100; C3 only, no C2 source): the
// picture and a scrolling tree of the friends (L2FriendTreeCtrl). A double click on a friend that is on line opens a chat
// window with him (L2ChatWnd, the chat lane's), which is also entered in two tables here and in the task window (L2TaskWnd,
// a small box on the desktop that lists the open chats). MessengerWnd makes it, as the second tab.
//
// Layout and virtuals from the stock constructor 0x1013B6D0, destructor 0x1013B7E0, OnCreate 0x10136000, OnDestroy 0x10133E30,
// OnPaint 0x10133D90, ShowWindow 0x10137370 and WindowProc 0x1013AC10.

#include "nwindow/MsnWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class L2FriendTreeCtrl;

class L2FriendWnd : public NCWnd
{
    NW_DECLARE_CLASS(L2FriendWnd, NCWnd, 0, 0x102C4178)

    void* Book;                      // +0xC8 a 0x20-byte object the constructor makes (0x1012DA10: an array and a string table); nothing
                                     //   here reads it (name ours)
    NMsnWndStrMap ChatWnds;             // +0xCC the chat windows by the friend's name (name ours)
    NMsnWndIntMap FreeWnds;             // +0xE0 the chat windows by a number: ShowWindow hides them, the "free" one is reused (name ours)
    L2FriendTreeCtrl* TreeCtrl;      // +0xF4 the friends, 240 x 270 at (8, 45)
    NCWnd* TaskWnd;                  // +0xF8 L2TaskWnd, made when the first chat opens (a child of the desktop)
    UTexture* BackTex;               // +0xFC "L2UI_CH3.Msn.msn_back0"

    L2FriendWnd();                                                   // 0x1013B6D0
    // ~L2FriendWnd                                                  // 0x1013B7E0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1013AC10
    virtual INT OnCreate();                                          // 37 0x10136000
    virtual INT OnDestroy();                                         // 38 0x10133E30
    virtual void ShowWindow();                                       // 44 0x10137370
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10133D90

    // names ours (anonymous in the db)
    __declspec(noinline) void ShowTaskWnd();                                              // 0x101360C0 the task window made (once) and shown
    __declspec(noinline) NCWnd* FindFreeChatWnd();                                        // 0x101373F0 the first chat window of FreeWnds that is not in use
    __declspec(noinline) INT OnDoubleClick(INT X, INT Y);                                 // 0x10139510 (ret 8) a friend of the tree opens a chat
    // 0x10138CB0 (ret 0xC): the chat window of this friend made or shown (the name is given by value and freed here)
    __declspec(noinline) void OpenChat(NMsnStringArg Name);
    // what the chat windows of the friends ask of this page (the L2ChatWnd's, the chat lane's); the strings are given by value
    __declspec(noinline) void* FindFriend(NMsnStringArg Name);       // 0x10131450 (ret 0xC) the record of the friend of that name in the book, NULL for none
    __declspec(noinline) void SendWhisper(NMsnStringArg Name, NMsnStringArg Text);   // 0x101313B0 (ret 0x18) the console sends the text to the friend
    __declspec(noinline) void OnChatClosed(NCWnd* Chat);             // 0x10139490 (ret 4) a chat window closed: it leaves the name table, the task window and the number table
};
NW_ASSERT_OFFSET(L2FriendWnd, Book, 0xC8);
NW_ASSERT_OFFSET(L2FriendWnd, ChatWnds, 0xCC);
NW_ASSERT_OFFSET(L2FriendWnd, FreeWnds, 0xE0);
NW_ASSERT_OFFSET(L2FriendWnd, TreeCtrl, 0xF4);
NW_ASSERT_OFFSET(L2FriendWnd, TaskWnd, 0xF8);
NW_ASSERT_OFFSET(L2FriendWnd, BackTex, 0xFC);
NW_ASSERT_SIZE(L2FriendWnd, 0x100);

#pragma pack(pop)
