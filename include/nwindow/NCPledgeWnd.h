#pragma once
// NCPledgeWnd - the clan page of the action window (UClass "CPledgeWnd", 0x3B0): crest, name,
// leader, level, castle / hideout, the member list (name, level, on / out; NCListCtrl) and
// nine buttons: invite, title, crest, war, truce, surrender, expel, yield, leave. Only the
// clan leader gets the first seven; only the others get Leave. The console fills it
// (NConsoleWnd::ReceivePledgeInfo, AddPledgeMember...). NCActionWnd +0x9C is this window.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCListCtrl;
class NCButton;

// What the console hands over about the clan (name ours; 0x62C bytes in C2, 0xEC in C1: the three
// texts are 256 characters now; built on the stack by NConsoleWnd::ReceivePledgeInfo). The window
// keeps a copy (Info) but SetPledgeInfo copies only part of it.
// C2 layout: from the stock SetPledgeInfo / UpdatePledgeInfo (the copies field by field) and the
// window's constructor and ClearInfo (the stores into the copy); C1 offsets in brackets.
struct NPledgeInfo
{
    TCHAR Name[256];           // +0x000 (C1: 32)
    INT PledgeID;              // +0x200 the crest texture is looked up by it (C1 +0x40)
    INT CrestID;               // +0x204 > 0: the clan has a crest (C1 +0x44)
    TCHAR LeaderName[256];     // +0x208 (C1 +0x48, 32 characters)
    INT Level;                 // +0x408 (C1 +0x88)
    UBOOL bHasCastle;          // +0x40C (C1 +0x8C)
    UBOOL bHasHideout;         // +0x410 (C1 +0x90)
    INT Unknown94;             // +0x414 (C1 +0x94)
    INT Unknown98;             // +0x418 (C1 +0x98)
    TCHAR StatusText[256];     // +0x41C "In War", "In Siege", "Dispersing"... (not copied) (C1 +0x9C, 32)
    INT Status;                // +0x61C index the console picked StatusText by (C1 +0xDC)
    INT UnknownE0;             // +0x620 (not copied)
    INT UnknownE4;             // +0x624 (not copied)
    INT UnknownE8;             // +0x628 (C1 +0xE8)

    // inlined in the console's slots 75, 76 and 203 (UnknownE8 is left)
    NPledgeInfo()
    {
        Name[0] = 0;
        LeaderName[0] = 0;
        StatusText[0] = 0;
        PledgeID = 0;
        CrestID = 0;
        UnknownE4 = 0;
        Level = 0;
        bHasCastle = 0;
        bHasHideout = 0;
        Unknown94 = 0;
        Unknown98 = 0;
        Status = 0;
        UnknownE0 = 0;
    }
};
NW_ASSERT_OFFSET(NPledgeInfo, PledgeID, 0x200);
NW_ASSERT_OFFSET(NPledgeInfo, LeaderName, 0x208);
NW_ASSERT_OFFSET(NPledgeInfo, Level, 0x408);
NW_ASSERT_OFFSET(NPledgeInfo, StatusText, 0x41C);
NW_ASSERT_OFFSET(NPledgeInfo, Status, 0x61C);
NW_ASSERT_SIZE(NPledgeInfo, 0x62C);

// A member as the console hands it over (name ours; 0x214 bytes, built on the stack by
// NConsoleWnd::AddPledgeMember)
struct NPledgeMember
{
    TCHAR Name[256];           // +0x000
    INT Level;                 // +0x200
    INT ClassID;               // +0x204 (-1)
    INT Unknown208;            // +0x208 (-1)
    INT Unknown20C;            // +0x20C (-1)
    UBOOL bOnline;             // +0x210 C2: the member's id while it is online, 0 when not (the row keeps it as its first
                               //   param: NCPledgeWnd::OnNewButton2 sends it as the member of the privileges request)

    // inlined in the console's slots 78, 79 and 205
    NPledgeMember()
    {
        Name[0] = 0;
        Level = 0;
        ClassID = -1;
        Unknown208 = -1;
        Unknown20C = -1;
        bOnline = 0;
    }
    BYTE _PadC3[52];   // this client grew the struct (db/struct_sizes.csv)
};
NW_ASSERT_OFFSET(NPledgeMember, Level, 0x200);
NW_ASSERT_OFFSET(NPledgeMember, bOnline, 0x210);
NW_ASSERT_SIZE(NPledgeMember, 0x248);

// NWM_COMMAND ids of the buttons (names ours)
enum
{
    NID_PLEDGE_CREST = 0xF00C,
    NID_PLEDGE_WAR = 0xF00D,
    NID_PLEDGE_TRUCE = 0xF00E,
    NID_PLEDGE_SURRENDER = 0xF010,
    NID_PLEDGE_INVITE = 0xF011,
    NID_PLEDGE_YIELD = 0xF012,
    NID_PLEDGE_TITLE = 0xF013,
    NID_PLEDGE_EXPEL = 0xF014,
    NID_PLEDGE_LEAVE = 0xF015,
    NID_PLEDGE_NEW1 = 0xF019,         // new in C2 (text 666)
    NID_PLEDGE_NEW2 = 0xF018,         // new in C2 (text 667)
    NID_PLEDGE_NEW3 = 0xF01A,         // new in C3 (text 842, name ours)
    NID_PLEDGE_NEW4 = 0xF01B,         // new in C3 (text 843, name ours)
};

class NCPledgeWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPledgeWnd, NCWnd, 0, 0x102785E8)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.BloodHoodWnd.BloodHood_Back"
    UTexture* CrestTex;              // +0xCC (NULL) NCGMPledgeWnd: the crest found by each paint   // C2: offset derived, no evidence
    NPledgeInfo Info;                // +0xD0 (0x62C bytes in C2; C1 +0x9C, 0xEC bytes)
    INT Unknown6FC;                  // +0x6FC C3: SetPledgeInfo / UpdatePledgeInfo copy the console record's +0x62C here (name ours):
                                     //   the castle's id (OnPaint: GetCastleName(it) when bHasCastle)
    INT Unknown700;                  // +0x700 the record's +0x630 (name ours): the hideout's id (the same, when bHasHideout)
    INT Unknown704;                  // +0x704 the record's +0x634 (name ours): 0 picks sysstring 894 as the status text
    NCListCtrl* MemberList;          // +0x708 four columns: name, level, class (an icon), online (an icon)
    // the buttons, C3 order (OnCreate 0x10049610 creates them in this order of ids: 0xF011, 0xF013, 0xF01A, 0xF014,
    // 0xF00C, 0xF01B, 0xF019, 0xF018, 0xF015, 0xF00D, 0xF00E, 0xF010, 0xF012); thirteen, two more than C2
    NCButton* InviteButton;          // +0x70C id 0xF011 text 330
    NCButton* TitleButton;           // +0x710 id 0xF013 text 331
    NCButton* Button714;             // +0x714 id 0xF01A text 842 (new in C3; name ours) -> vslot 80 (OnRemoveTitle)
    NCButton* WarButton;             // +0x718 id 0xF00D text 333
    NCButton* TruceButton;           // +0x71C id 0xF00E text 334
    NCButton* CrestButton;           // +0x720 id 0xF00C text 332
    NCButton* ExpelButton;           // +0x724 id 0xF014 text 338
    NCButton* YieldButton;           // +0x728 id 0xF012 text 336 always enabled in C2; ShowWindow disables it for the leader
    NCButton* Button72C;             // +0x72C id 0xF01B text 843 (new in C3; name ours) -> vslot 81 (OnRemoveCrest)
    NCButton* NewButton6F4;          // +0x730 id 0xF019 text 666 (OnNewButton1)
    NCButton* NewButton6F8;          // +0x734 id 0xF018 text 667 (OnNewButton2)
    NCButton* SurrenderButton;       // +0x738 id 0xF010 text 335
    NCButton* LeaveButton;           // +0x73C id 0xF015 text 337
    UTexture* ClassIcons[9];         // +0x740 "L2UI_CH3.PartyWnd.party_styleicon1" .. 7, "_1_1", "_1_2": the icon of each class group
    BYTE _Unknown764[0x964 - 0x764]; // +0x764 not used by the stock code of this class

    NCPledgeWnd();                                                   // 0x1004ae10
    // ~NCPledgeWnd                                                  // 0x10041020 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10046cb0
    virtual INT OnCreate();                                          // 37 0x10049610
    virtual INT OnDestroy();                                         // 38 0x10048020
    virtual void ShowWindow();                                       // 44 0x10046eb0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10047bc0
    // the buttons (names ours): dialogs NCDialogBox answers by ID, or messages
    virtual INT OnInvite();                                          // 68 0x10047350
    virtual INT OnSetCrest();                                        // 69 0x100489b0
    virtual INT OnDeclareWar();                                      // 70 0x10048a50
    virtual INT OnEndWar();                                          // 71 0x10048af0
    virtual INT OnSurrender();                                       // 72 0x10048b90
    virtual INT OnExpel();                                           // 73 0x10048c30
    virtual INT OnSurrenderPersonally();                             // 74 0x10048d20
    virtual INT OnWithdraw();                                        // 75 0x10048dc0
    virtual INT OnGiveTitle();                                       // 76 0x10048790
    virtual INT OnNewButton1();                                      // 77 0x10048ea0 (C2 only; name ours) the button 0xF019 (NewButton6F4): the player's own
                                                                     //   privileges (the empower window, view mode)
    virtual INT OnNewButton2();                                      // 78 0x10048f60 (C2 only; name ours) the button 0xF018 (NewButton6F8): the selected
                                                                     //   member's privileges (the empower window, edit mode; the leader's)
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam) { return 1; } // 79 0x1000bd60 (folded)
    virtual INT OnRemoveTitle();                                     // 80 0x10048860 (new in C3; name ours) take the targeted
                                                                     //   player's title away (confirmation dialog 0x48)
    virtual INT OnRemoveCrest();                                     // 81 0x10048920 (new in C3; name ours) delete the clan's
                                                                     //   crest (confirmation dialog 0x49)

    // the console's calls (names ours)
    void SetPledgeInfo(NPledgeInfo* InInfo);                         // 0x10047170
    void UpdatePledgeInfo(NPledgeInfo* InInfo);                      // 0x10047250
    __declspec(noinline) void ClearMembers();                        // 0x100472e0
    __declspec(noinline) void ClearInfo();                           // 0x100472f0
    void Reset();                                                    // 0x10048300
    void AddMember(NPledgeMember* Member);                           // 0x10048310
    void UpdateMember(NPledgeMember* Member);                        // 0x100485e0
    void DeleteMember(const TCHAR* Name);                            // 0x1004a3e0
};
NW_ASSERT_OFFSET(NCPledgeWnd, Info, 0xD0);
NW_ASSERT_OFFSET(NCPledgeWnd, Unknown6FC, 0x6FC);
NW_ASSERT_OFFSET(NCPledgeWnd, MemberList, 0x708);
NW_ASSERT_OFFSET(NCPledgeWnd, CrestButton, 0x720);
NW_ASSERT_OFFSET(NCPledgeWnd, NewButton6F8, 0x734);
NW_ASSERT_OFFSET(NCPledgeWnd, LeaveButton, 0x73C);
NW_ASSERT_OFFSET(NCPledgeWnd, ClassIcons, 0x740);
NW_ASSERT_SIZE(NCPledgeWnd, 0x964);

#pragma pack(pop)