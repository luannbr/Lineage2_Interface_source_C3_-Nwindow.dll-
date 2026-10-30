#pragma once
// NCMatchWnd - the party matching window (UClass "CMatchWnd", 0xF8): the rooms players have opened to look for a
// party (NCMatchListCtrl: number, title, type, level range, members), a page at a time. Two selection boxes
// (the place and the kind of room), buttons for the previous and the next page, a new search, a new room (the
// NCPartyRoomMakingWnd) and a quick join. The console fills it (SetList, ClearList); the buttons ask the server
// and stay disabled until a 5 second timer re-enables them.
//
// C3 layout from OnCreate (0x100a7ca0) and its users. The C2 window (invite / options / whisper / targeting) is
// gone with its slots.
//
// Desktop (NCConsole) +0x140 is this window.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCMatchListCtrl;
class NCMatchFrameCtrl;
class NCComboBox;
class NCButton;

// NWM_COMMAND ids of the controls (names ours)
enum
{
    NID_MATCH_PREV = 0xF2,       // vslot 68
    NID_MATCH_NEXT = 0xF3,       // vslot 69
    NID_MATCH_SEARCH = 0xF4,     // vslot 70
    NID_MATCH_CREATE = 0xF5,     // vslot 71
    NID_MATCH_JOIN = 0xF6,       // vslot 72
    NID_MATCH_FRAME = 0xF7,
    NID_MATCH_AREA = 0xF8,       // the place selection box
    NID_MATCH_TYPE = 0xF9,       // the room kind selection box
};

// the window's timer: the buttons wake up again (SetTimer 5000)
enum { TIMER_MatchButtons = 0x1F8 };

class NCMatchWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMatchWnd, NCWnd, 0, 0x1028D098)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.PartyMatchWnd.PartyMatch1_Back"
    NCComboBox* AreaCombo;           // +0xCC (names ours) the place: -2, -1, then 1 .. 13 (the item values)
    NCComboBox* TypeCombo;           // +0xD0 the kind of room (two items)
    NCMatchListCtrl* MatchList;      // +0xD4 five columns
    NCButton* PrevButton;            // +0xD8 id 0xF2
    NCButton* NextButton;            // +0xDC id 0xF3
    NCButton* SearchButton;          // +0xE0 id 0xF4
    NCButton* CreateButton;          // +0xE4 id 0xF5
    NCButton* JoinButton;            // +0xE8 id 0xF6
    INT TimerID;                     // +0xEC SetTimer(0x1F8, 5000)
    INT Page;                        // +0xF0 the page the list shows (the console sets it with the list; 0 = none)
    NCMatchFrameCtrl* MatchFrame;    // +0xF4 id 0xF7, the title bar (the window's own FrameCtrl is not used)

    NCMatchWnd();                                                    // 0x100a7c00
    // ~NCMatchWnd                                                   // 0x100a7c50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a4c40
    virtual INT OnTimer(INT InTimerID);                              // 36 0x100a4d20
    virtual INT OnCreate();                                          // 37 0x100a7ca0
    virtual INT OnDestroy();                                         // 38 0x100a57e0
    virtual void ShowWindow();                                       // 44 0x100aa860 (folded)
    virtual void HideWindow();                                       // 45 0x10030c00 (folded)
    virtual void Minimize();                                         // 49 0x100a4d80
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100a5990
    // the buttons (names ours): each asks the console for a page of the list and disables four of the buttons
    virtual INT OnPrevPage();                                        // 68 0x100a5c80 (db: OnInvite)
    virtual INT OnNextPage();                                        // 69 0x100a5d80
    virtual INT OnSearch();                                          // 70 0x100a5e70 page 1
    virtual INT OnCreateRoom();                                      // 71 0x100a5f60 opens the room making window
    virtual INT OnJoin();                                            // 72 0x100a4e00 asks to join any room (id 0)

    // the console's calls (names ours)
    void SetList(L2ParamStack& Params);                              // 0x100a5b10 (the console's slot 374)
    void ClearList();                                                // 0x100a4de0 (the console's slot 375)
};
NW_ASSERT_OFFSET(NCMatchWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCMatchWnd, AreaCombo, 0xCC);
NW_ASSERT_OFFSET(NCMatchWnd, MatchList, 0xD4);
NW_ASSERT_OFFSET(NCMatchWnd, JoinButton, 0xE8);
NW_ASSERT_OFFSET(NCMatchWnd, TimerID, 0xEC);
NW_ASSERT_OFFSET(NCMatchWnd, Page, 0xF0);
NW_ASSERT_OFFSET(NCMatchWnd, MatchFrame, 0xF4);
NW_ASSERT_SIZE(NCMatchWnd, 0xF8);

#pragma pack(pop)
