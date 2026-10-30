#pragma once
// NCPartyWnd - the party (UClass "CPartyWnd", 0x104): a column of eight NCPartyStatusWnd, one
// per member, under a vertical title bar on the left, and a button that switches the buff icons
// of the members on and off. The console hands it the server's party packets (AddMember,
// UpdateMember, DeleteMember, DeleteAll); the window grows and shrinks with the members shown.
//
// Desktop (NCConsole) +0xB0 is the party window; the console's vslots 130..133 (add, update,
// delete, delete all) forward the party packets here.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCPartyStatusWnd;
class NCButton;

enum { NUM_PARTY_SLOTS = 8 };

class NCPartyWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPartyWnd, NCWnd, 0, 0x102B0228)

    UTexture* FrameTopTex;           // +0xC8 "L2UI_CH3.FrameCtrl.smallbar1" left edge, shown while there are members
    UTexture* FrameCenterTex;        // +0xCC "L2UI_CH3.FrameCtrl.smallbar2"
    UTexture* FrameBottomTex;        // +0xD0 "L2UI_CH3.FrameCtrl.smallbar3"
    INT NumMembers;                  // +0xD4
    UBOOL bShown;                    // +0xD8 members listed (set by AddMember, cleared with the last member)
    UBOOL bToggle;                   // +0xDC (name ours) flipped by OnToggle; the stock code does not read it
    NCPartyStatusWnd* Members[NUM_PARTY_SLOTS]; // +0xE0 filled from the top, empty = no name
    NCButton* BuffButton;            // +0x100 "party_buffbutton" (NID_PARTY_TOGGLE)

    NCPartyWnd();                                                    // 0x10103550

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100fb1d0
    virtual INT OnCreate();                                          // 37 0x101035f0
    virtual INT OnDestroy();                                         // 38 0x100fd230
    virtual void ShowWindow();                                       // 44 0x100fd600
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100fd310
    virtual INT OnToggle();                                          // 68 0x100fb230 (name ours)

    // names ours. C3: the server's packet adds the leader and the loot rule in front and the class at the end
    void UpdateMember(INT MemberID, const TCHAR* Name, INT HP, INT MaxHP, INT MP,
                      INT MaxMP, INT ClassID);                       // 0x100fd420
    __declspec(noinline) void UpdateLayout();                        // 0x100fd4b0
    void AddMember(INT LeaderID, INT LootRule, INT MemberID, const TCHAR* Name, INT HP, INT MaxHP, INT MP,
                   INT MaxMP, INT ClassID);                          // 0x101019e0
    void DeleteMember(INT MemberID, const TCHAR* Name);              // 0x10101a80 by name, the
                                                                     //   id is not read
    void DeleteAll();                                                // 0x10101bb0
};
NW_ASSERT_OFFSET(NCPartyWnd, FrameTopTex, 0xC8);
NW_ASSERT_OFFSET(NCPartyWnd, NumMembers, 0xD4);
NW_ASSERT_OFFSET(NCPartyWnd, bToggle, 0xDC);
NW_ASSERT_OFFSET(NCPartyWnd, Members, 0xE0);
NW_ASSERT_OFFSET(NCPartyWnd, BuffButton, 0x100);
NW_ASSERT_SIZE(NCPartyWnd, 0x104);

#pragma pack(pop)
