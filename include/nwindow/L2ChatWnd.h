#pragma once
// L2ChatWnd - ChatWnd's Lineage II skin (UClass "L2ChatWnd", 0xF0; C3 only, the messenger): a conversation with one friend
// of the Lineage II friend list, in the same 256 x 397 window: the title bar, the talk (L2ChatBox), the edit box, and the
// friend's name at (11, 54). No roster, no invite window, no switchboard: what is typed goes to the game server.
//
// Layout from the stock constructor 0x10125A00, OnCreate 0x10125BD0, OnDestroy 0x101245A0 and OnPaint 0x10124640.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class L2ChatFrameCtrl;
class L2ChatBox;
class NCEditBox;

class L2ChatWnd : public NCWnd
{
    NW_DECLARE_CLASS(L2ChatWnd, NCWnd, 0, 0x102C1318)

    void* Messenger;                 // +0xC8 (name ours) cleared by the constructor; the close button tells it (0x10139490)
    FString FriendName;              // +0xCC (name ours) the friend: shown at (11, 54), L2TaskBox's entries look it up
    INT Unknown_D8;                  // +0xD8 (name ours) 1 from the constructor
    INT bHidden;                     // +0xDC (name ours) 1 while the window is hidden or minimized (ShowWindow clears it)
    L2ChatFrameCtrl* TitleBar;       // +0xE0 (name ours) id 0xA70, 256 x 20
    L2ChatBox* TextBox;              // +0xE4 the talk: 200 lines, 14 shown, 9 / 96 at 238 x 252
    NCEditBox* EditBox;              // +0xE8 id 0xA71, 239 x 16 at (8, 360)
    UTexture* BackTex;               // +0xEC "L2UI_CH3.msn_back3"

    L2ChatWnd();                                                     // 0x10125A00
    // 0x10125B20 (ret 0x10): the window the friend list makes for a friend: (the friend list window, the friend's name by value; the
    // name is destroyed on return)
    L2ChatWnd(void* InMessenger, FString InFriendName);
    // ~L2ChatWnd                                                    // 0x10125AB0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10125970
    virtual INT OnCreate();                                          // 37 0x10125BD0
    virtual INT OnDestroy();                                         // 38 0x101245A0
    virtual void ShowWindow();                                       // 44 0x10124240
    virtual void SetTitle(const TCHAR* Title);                       // 51 0x101242B0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10124640

    // names ours
    INT OnMinimize();                                                // 0x10124260 the minimize button: hidden
    INT OnClose();                                                   // 0x10124280 the close button: hidden, the messenger told
    void ClearAll();                                                 // 0x101242D0 the talk and the edit box emptied
    INT OnSend(WPARAM lParam);                                       // 0x10125320 (ret 4) the edit box's Enter
    void SetFriendName(FString Name);                                // 0x10131650 (ret 0xC; was anonymous) the friend this window talks to
};
NW_ASSERT_OFFSET(L2ChatWnd, FriendName, 0xCC);
NW_ASSERT_OFFSET(L2ChatWnd, bHidden, 0xDC);
NW_ASSERT_OFFSET(L2ChatWnd, TitleBar, 0xE0);
NW_ASSERT_OFFSET(L2ChatWnd, EditBox, 0xE8);
NW_ASSERT_OFFSET(L2ChatWnd, BackTex, 0xEC);
NW_ASSERT_SIZE(L2ChatWnd, 0xF0);

#pragma pack(pop)
