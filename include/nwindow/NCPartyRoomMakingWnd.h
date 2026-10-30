#pragma once
// NCPartyRoomMakingWnd - the window that opens or edits a party room (UClass "CPartyRoomMakingWnd", 0xE8; new in
// C3): the title, how many members (2 to 9), the level range, OK and Cancel. OK asks the server to open the room
// (or, with the number of a room the player owns, to change it) and closes the window.
//
// Opened by NCMatchWnd (a new room) and by NCPartyRoomWnd's Manage button (the room's own data).
//
// Desktop (NCConsole) +0x144 is this window.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCEditBox;
class NCComboBox;
class NCButton;

// NWM_COMMAND ids of the controls (names ours)
enum
{
    NID_ROOMMAKING_TITLE = 0xFF0FF021,
    NID_ROOMMAKING_MEMBERS = 0xFF0FF022,
    NID_ROOMMAKING_MINLEVEL = 0xFF0FF023,
    NID_ROOMMAKING_MAXLEVEL = 0xFF0FF024,
    NID_ROOMMAKING_OK = 0xFF0FF026,          // vslot 68
    NID_ROOMMAKING_CANCEL = 0xFF0FF027,      // vslot 69
};

class NCPartyRoomMakingWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPartyRoomMakingWnd, NCWnd, 0, 0x1028E010)

    NCEditBox* TitleEdit;            // +0xC8 id 0xFF0FF021 (names ours)
    NCComboBox* MembersCombo;        // +0xCC id 0xFF0FF022, "2" .. "9": the selected index + 2 members
    NCEditBox* MinLevelEdit;         // +0xD0 id 0xFF0FF023 (a number, Type 3)
    NCEditBox* MaxLevelEdit;         // +0xD4 id 0xFF0FF024
    NCButton* OKButton;              // +0xD8 id 0xFF0FF026
    NCButton* CancelButton;          // +0xDC id 0xFF0FF027
    UTexture* BackTex;               // +0xE0 "L2UI_CH3.PartyMatchWnd.PartyMatch3_Back"
    INT RoomID;                      // +0xE4 0: a new room, else the room being changed (SetData)

    NCPartyRoomMakingWnd();                                          // 0x100a8a60
    // ~NCPartyRoomMakingWnd                                         // 0x100a8ab0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a4ec0
    virtual INT OnCreate();                                          // 37 0x100a7130
    virtual INT OnDestroy();                                         // 38 0x100a6000
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100a6180
    // the buttons (names ours)
    virtual INT OnOkBtnClick();                                      // 68 0x100a6440
    virtual INT OnCancelBtnClick();                                  // 69 0x100a5040

    // the window's contents for a room (the sixth argument is not read): the room's number, its title, the
    // selected index of the members box (members - 2), the level range as numbers (name ours)
    void SetData(INT InRoomID, const TCHAR* Title, INT MembersIndex, INT MinLevel, INT MaxLevel, INT Unused); // 0x100a4f40 (ret 0x18)
};
NW_ASSERT_OFFSET(NCPartyRoomMakingWnd, TitleEdit, 0xC8);
NW_ASSERT_OFFSET(NCPartyRoomMakingWnd, CancelButton, 0xDC);
NW_ASSERT_OFFSET(NCPartyRoomMakingWnd, BackTex, 0xE0);
NW_ASSERT_OFFSET(NCPartyRoomMakingWnd, RoomID, 0xE4);
NW_ASSERT_SIZE(NCPartyRoomMakingWnd, 0xE8);

#pragma pack(pop)
