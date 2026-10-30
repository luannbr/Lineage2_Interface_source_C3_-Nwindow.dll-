#pragma once
// NCPartyRoomWnd - the window of the party room the player is in (UClass "CPartyRoomWnd", 0x308; new in C3): the
// room's number, title, place, loot rule, level range and members (NCMatchPartyListCtrl: name, class, level range,
// place, role), a chat line (the room's own chat, type 14) and the buttons Manage and Ban (the owner's), Invite and
// Close. The console fills it (SetRoomInfo, SetMembers, UpdateMember) and routes the room's chat to it (AddChatLine).
//
// Desktop (NCConsole) +0x148 is this window.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCPetitionListBox;
class NCEditBox;
class NCMatchPartyListCtrl;
class NCPartyRoomFrameCtrl;
class NCButton;

// NWM_COMMAND ids of the controls (names ours)
enum
{
    NID_PARTYROOM_FRAME = 0xF7,
    NID_PARTYROOM_CHAT = 0xFFA0F020,         // the chat list
    NID_PARTYROOM_EDIT = 0xFFA0F021,         // the chat line: Enter sends it
    NID_PARTYROOM_MANAGE = 0xFFA0F023,       // vslot 68
    NID_PARTYROOM_BAN = 0xFFA0F024,          // vslot 69
    NID_PARTYROOM_INVITE = 0xFFA0F025,       // vslot 70
    NID_PARTYROOM_CLOSE = 0xFFA0F026,        // vslot 71
};

// who the player is in the room (Mode, set by SetMembers / UpdateMember; names ours)
enum { PARTYROOM_Member = 0, PARTYROOM_Owner = 1, PARTYROOM_Other = 2 };

class NCPartyRoomWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPartyRoomWnd, NCWnd, 0, 0x1028EF78)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.PartyMatchWnd.PartyMatch2_Back"
    NCPetitionListBox* ChatList;     // +0xCC (names ours) 40 lines, 4 visible
    NCEditBox* ChatEdit;             // +0xD0
    NCMatchPartyListCtrl* MemberList; // +0xD4 five columns
    NCButton* ManageButton;          // +0xD8 enabled for the owner
    NCButton* BanButton;             // +0xDC enabled for the owner
    NCButton* InviteButton;          // +0xE0 enabled for the owner
    NCButton* CloseButton;           // +0xE4
    INT RoomID;                      // +0xE8
    TCHAR Title[256];                // +0xEC
    INT Location;                    // +0x2EC the place (1 .. 13)
    INT LootRule;                    // +0x2F0 0 .. 4
    INT MinLevel;                    // +0x2F4
    INT MaxLevel;                    // +0x2F8
    INT MaxMembers;                  // +0x2FC
    INT Mode;                        // +0x300 PARTYROOM_*: what the player is in the room (the buttons follow it)
    NCPartyRoomFrameCtrl* RoomFrame; // +0x304 id 0xF7, the title bar

    NCPartyRoomWnd();                                                // 0x100a8bd0
    // ~NCPartyRoomWnd                                               // 0x100a8c20

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a66d0
    virtual INT OnCreate();                                          // 37 0x100a8c70
    virtual INT OnDestroy();                                         // 38 0x100a67b0
    virtual void ShowWindow();                                       // 44 0x100aa860 (folded)
    virtual void Minimize();                                         // 49 0x100a4d80 (NCMatchWnd's: folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100a6960
    // the buttons (names ours)
    virtual INT OnManageBtnClick();                                  // 68 0x100a5100
    virtual INT OnBanBtnClick();                                     // 69 0x100a51f0
    virtual INT OnInviteBtnClick();                                  // 70 0x100a52b0
    virtual INT OnCloseBtnClick();                                   // 71 0x100a6e20

    // the console's calls (names ours)
    void SetRoomInfo(L2ParamStack& Params);                          // 0x100a5450 (the console's slot 376)
    void SetMembers(L2ParamStack& Params);                           // 0x100a6ea0 (slot 377)
    void UpdateMember(L2ParamStack& Params);                         // 0x100a75b0 (slot 379)
    void AddChatLine(const TCHAR* Text, DWORD Color);                // 0x100a55c0 (ret 8: NCConsole::Say's)

    // inlined here and there (names ours)
    void RequestExit();                                              // 0x100a5380 asks to dismiss (owner) or leave
    UTexture* GetClassIcon(INT ClassID);                             // 0x100a54e0 the class group's icon
    INT OnChatEnter(LPARAM lParam);                                  // 0x100a55f0 (ret 4)
};
NW_ASSERT_OFFSET(NCPartyRoomWnd, ChatList, 0xCC);
NW_ASSERT_OFFSET(NCPartyRoomWnd, MemberList, 0xD4);
NW_ASSERT_OFFSET(NCPartyRoomWnd, CloseButton, 0xE4);
NW_ASSERT_OFFSET(NCPartyRoomWnd, RoomID, 0xE8);
NW_ASSERT_OFFSET(NCPartyRoomWnd, Title, 0xEC);
NW_ASSERT_OFFSET(NCPartyRoomWnd, Location, 0x2EC);
NW_ASSERT_OFFSET(NCPartyRoomWnd, MaxMembers, 0x2FC);
NW_ASSERT_OFFSET(NCPartyRoomWnd, Mode, 0x300);
NW_ASSERT_OFFSET(NCPartyRoomWnd, RoomFrame, 0x304);
NW_ASSERT_SIZE(NCPartyRoomWnd, 0x308);

#pragma pack(pop)
