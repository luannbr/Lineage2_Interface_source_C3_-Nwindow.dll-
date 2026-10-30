#pragma once
// NCStatusWnd - base of the HP / MP windows (UClass "CStatusWnd", 0x2FC): the player's
// (NCPlayerStatusWnd: bars, level, exp), the target's (NCTargetStatusWnd: name colored by the
// level difference, HP / MP of monsters and party members, clan and alliance) and one per
// party member (NCPartyStatusWnd, inside NCPartyWnd), the pet's (NCPetStatusWnd) and the
// summon's (NCSummonedStatusWnd). The base only keeps the textures and numbers the subclasses
// draw.
//
// Desktop (NCConsole) +0xC0 is the target window, +0xC4 the player window, +0xB4 the pet
// window, +0xB8 the summon window.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCResizeFrame;
class NCProgressCtrl;

class NCStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCStatusWnd, NCWnd, 0, 0x102AF7E8)

    UTexture* MPTex;                 // +0xC8
    UTexture* HPTex;                 // +0xCC
    UTexture* EXPTex;                // +0xD0
    UTexture* BackTex;               // +0xD4 "NWindow.WhiteTexture" (OnCreate), the subclasses
                                     //   load their own
    UTexture* BackTopTex;            // +0xD8 target window: background (top, center, bottom)   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* BackCenterTex;         // +0xDC   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* BackBottomTex;         // +0xE0   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* FrameTopTex;           // +0xE4 target window: left edge (top, center, bottom)   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* FrameCenterTex;        // +0xE8   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* FrameBottomTex;        // +0xEC   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* UpBtnTex;              // +0xF0 target window: loaded, never drawn (the toggle   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* UpBtnClickTex;         // +0xF4   button loads its own)   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* DownBtnTex;            // +0xF8   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* DownBtnClickTex;       // +0xFC   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* LeftBackTex;           // +0x100 player window: edges
    UTexture* RightBackTex;          // +0x104   // C2: offset derived, no evidence
    UTexture* MPBackTex;             // +0x108 empty bars   // C2: offset derived, no evidence
    UTexture* HPBackTex;             // +0x10C   // C2: offset derived, no evidence
    INT HP;                          // +0x110 party member (SetInfo)
    INT MaxHP;                       // +0x114
    INT MP;                          // +0x118
    INT MaxMP;                       // +0x11C
    INT ObjectID;                    // +0x120 target window: whom a click targets / attacks
    INT ClassID;                     // +0x124 (new in C3, name ours) party member: its class (SetInfo, UpdateInfo)
    INT BarWidth;                    // +0x128 constructor argument (the target window sets 132)   // C3: the constructor stores it here
    INT MaxExtraWidth;               // +0x12C player window: how far it can be widened (202)
    INT ExtraWidth;                  // +0x130 player window: widened by this (bars grow with it)   // C2: offset derived, no evidence
    TCHAR Name[256];                 // +0x134 party member, empty = free slot

    __declspec(noinline) NCStatusWnd(INT InBarWidth = 100);                // 0x10102c70 (out of line: the derived constructors call it)
    // ~NCStatusWnd                                                  // 0x100d3740 (C1 only: no C2 match)

    virtual __declspec(noinline) INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam); // 25 0x10119f60 (folded; the
                                                                     //   derived WindowProcs call it, so out of line)
    virtual __declspec(noinline) INT OnCreate();                     // 37 0x100fa720 (called by the derived OnCreates: out of line)
    virtual INT OnDestroy();                                         // 38 0x100fa750 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100fc4a0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam) { return 1; } // 62 0x1000bd60 (folded)

    // names ours
    UBOOL IsName(const TCHAR* InName);                               // 0x100fa7d0
    virtual __declspec(noinline) void Clear();                       // 68 0x100fa800 (C3: virtual; the party member's adds its buffs)
    __declspec(noinline) UBOOL IsEmpty();                            // 0x100faf90
};
NW_ASSERT_OFFSET(NCStatusWnd, MPTex, 0xC8);
NW_ASSERT_OFFSET(NCStatusWnd, LeftBackTex, 0x100);
NW_ASSERT_OFFSET(NCStatusWnd, HP, 0x110);
NW_ASSERT_OFFSET(NCStatusWnd, ObjectID, 0x120);
NW_ASSERT_OFFSET(NCStatusWnd, ExtraWidth, 0x130);
NW_ASSERT_OFFSET(NCStatusWnd, Name, 0x134);
NW_ASSERT_SIZE(NCStatusWnd, 0x334);

// NWM_COMMAND ids of the status windows' controls
enum
{
    NID_PLAYERSTATUS_RESIZE = 0xF20F04,  // right edge of the player window
    NID_TARGETSTATUS_CLOSE = 0xFA,
    NID_TARGETSTATUS_TOGGLE = 0xFB,      // shows / hides the clan and alliance lines
};

// NCPlayerStatusWnd - the player's name, level, HP / MP / exp bars (UClass "CPlayerStatusWnd",
// 0x308). The right edge widens it (the bars grow with it); a click targets the player.
class NCPlayerStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCPlayerStatusWnd, NCStatusWnd, 0, 0x102B11A0)

    NCWnd* HPBar;                    // +0x334 (C3) NCStatusBarCtrl "ps_hpbar" (ID 0xFFA103F0), grows with the window
    UTexture* PlayerHPBackTex;       // +0x338 "L2UI_CH3.PlayerStatusWnd.ps_hpbar_back"
    UTexture* PlayerMPBackTex;       // +0x33C "ps_mpbar_back"
    UTexture* PlayerEXPBackTex;      // +0x340 "ps_expbar_back"
    UTexture* CPBackTex;             // +0x344 "smallbar_cpback"
    UTexture* CPTex;                 // +0x348 "smallbar_cp"
    UTexture* LevelBackTex;          // +0x34C "ps_levelback"
    UTexture* SizeControl2Tex;       // +0x350 "ps_sizecontrol2"
    User* Player;                    // +0x354 set by each paint
    INT Unknown358;                  // +0x358 (the constructor clears it)
    NCResizeFrame* ResizeFrame;      // +0x35C

    NCPlayerStatusWnd(INT InBarWidth = 100);                               // 0x100d4990 (C1 only: no C2 match)
    // ~NCPlayerStatusWnd                                            // 0x100d49f0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100faa60
    virtual INT OnCreate();                                          // 37 0x10102e60
    virtual INT OnDestroy();                                         // 38 0x100fc630
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10100360
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100faad0
    // names ours
    virtual INT OnResize(INT Delta);                                 // 69 0x100fc6e0

    INT Resize(INT Delta);                                           // 0x100faac0 (name ours)
};
NW_ASSERT_OFFSET(NCPlayerStatusWnd, HPBar, 0x334);
NW_ASSERT_OFFSET(NCPlayerStatusWnd, Player, 0x354);
NW_ASSERT_OFFSET(NCPlayerStatusWnd, ResizeFrame, 0x35C);
NW_ASSERT_SIZE(NCPlayerStatusWnd, 0x360);

// NCTargetStatusWnd - the target (UClass "CTargetStatusWnd", 0x310): its name, HP / MP bars
// for monsters, siege NPCs, the player, its pet and party members; a door or other static
// object shows its name only. For players the window can be expanded to show the clan and
// the alliance. A click targets again (Ctrl: attacks); the close button cancels the target.
class NCTargetStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCTargetStatusWnd, NCStatusWnd, 0, 0x102AFD08)

    NCButton* CloseButton;           // +0x334 "FrameCloseBtn"
    INT Unknown338;                  // +0x338
    INT LevelDiff;                   // +0x33C the player's level minus the target's (name color)
    INT HoverBuff;                   // +0x340 (new in C3, name ours) 1 + the index of the buff icon under the mouse, 0 = none
    UBOOL bExpanded;                 // +0x344 the buff icons / the clan and alliance lines are shown
    NCButton* ToggleButton;          // +0x348 "Target_InfoButton"
    UTexture* BackTex3[6];           // +0x34C (C3) "SmallWindow_Back1".."3" (the short window), "SmallWindow2_Back1".."3" (the tall one)
    NToolTipInfo BuffToolTip;        // +0x364 (new in C3) the tool tip of the buff icon under the mouse

    NCTargetStatusWnd(INT InBarWidth = 100);                               // 0x10102cb0
    // ~NCTargetStatusWnd                                            // 0x10102d50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100fa830
    virtual INT OnCreate();                                          // 37 0x100ff660
    virtual INT OnDestroy();                                         // 38 0x100fc560
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100ff9a0
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x10102100 (the db calls it OnPostPaint)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x10100270
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100fa8b0
    // names ours
    virtual INT OnClose();                                           // 69 0x100fa9f0
    virtual INT OnToggle();                                          // 70 0x100fa950

    void SetLevelDiff(INT Diff);                                     // 0x100fa940 (name ours)
};
NW_ASSERT_OFFSET(NCTargetStatusWnd, CloseButton, 0x334);
NW_ASSERT_OFFSET(NCTargetStatusWnd, LevelDiff, 0x33C);
NW_ASSERT_OFFSET(NCTargetStatusWnd, bExpanded, 0x344);
NW_ASSERT_OFFSET(NCTargetStatusWnd, ToggleButton, 0x348);
NW_ASSERT_OFFSET(NCTargetStatusWnd, BackTex3, 0x34C);
NW_ASSERT_OFFSET(NCTargetStatusWnd, BuffToolTip, 0x364);
NW_ASSERT_SIZE(NCTargetStatusWnd, 0x3E0);

// NCPartyStatusWnd - one party member (UClass "CPartyStatusWnd", 0x300): name, HP / MP bars.
// A click targets the member (Ctrl: attacks), a right click targets what the member targets.
class NCPartyStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCPartyStatusWnd, NCStatusWnd, 0, 0x102B0C78)

    INT MemberID;                    // +0x334 object id of the member (-1)
    TArray<INT> BuffSkillIDs;        // +0x338 (new in C3, name ours) the member's buffs: skill ids (AddBuff)
    TArray<UTexture*> BuffIcons;     // +0x344 (new in C3) their icon textures, one per skill id
    UTexture* DebuffFrameTex;        // +0x350 (new in C3) "L2UI_CH3.PlayerStatusWnd.debuff" drawn over the special ones
    INT LeaderID;                    // +0x354 (new in C3) object id of the party leader (SetInfo)
    INT LootRule;                    // +0x358 (new in C3) the party's item distribution, 0..4 (SetInfo)
    INT Unknown35C;                  // +0x35C (new in C3) the constructor clears it
    NCButton* StyleIcon[9];          // +0x360 (new in C3) one per class group, only the member's is shown
    NCButton* LeaderIcon;            // +0x384 (new in C3) shown while MemberID == LeaderID

    NCPartyStatusWnd(INT InBarWidth = 100);                                // 0x10103410

    virtual INT OnCreate();                                          // 37 0x101010c0
    virtual INT OnDestroy();                                         // 38 0x100fd0c0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x101014c0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100fae70
    virtual INT OnRButtonUp(WPARAM wParam, LPARAM lParam);           // 65 0x100faf00
    virtual void Clear();                                            // 68 0x101019d0 (the base's, then the buffs)

    // names ours. C3: the party packet carries the leader and the loot rule in front and the class at the end
    void SetInfo(INT InLeaderID, INT InLootRule, INT InMemberID, const TCHAR* InName, INT InHP, INT InMaxHP,
                 INT InMP, INT InMaxMP, INT InClassID);              // 0x100fd140 (a new member)
    void UpdateInfo(INT InMemberID, const TCHAR* InName, INT InHP, INT InMaxHP, INT InMP, INT InMaxMP,
                    INT InClassID);                                  // 0x100fd1c0
    __declspec(noinline) void UpdateStyle();                         // 0x100fafa0 shows the class icon and the leader icon, sets their tool tips
    // 0x100fad60 (ret 4, name ours; `this` is not read): the group's name by its group number (0 .. 8: system strings
    // 874 .. 880, 175, 176; NULL for another number)
    __declspec(noinline) const TCHAR* GetStyleName(INT Group);
    __declspec(noinline) void ClearBuffs();                          // 0x10100ca0 (the same code as the end of Clear)
    void AddBuff(INT SkillID);                                       // 0x10100ce0
};
NW_ASSERT_OFFSET(NCPartyStatusWnd, MemberID, 0x334);
NW_ASSERT_OFFSET(NCPartyStatusWnd, BuffSkillIDs, 0x338);
NW_ASSERT_OFFSET(NCPartyStatusWnd, BuffIcons, 0x344);
NW_ASSERT_OFFSET(NCPartyStatusWnd, DebuffFrameTex, 0x350);
NW_ASSERT_OFFSET(NCPartyStatusWnd, LeaderID, 0x354);
NW_ASSERT_OFFSET(NCPartyStatusWnd, StyleIcon, 0x360);
NW_ASSERT_OFFSET(NCPartyStatusWnd, LeaderIcon, 0x384);
NW_ASSERT_SIZE(NCPartyStatusWnd, 0x388);

// NCPetStatusWnd - the pet (UClass "CPetStatusWnd", 0x3D0): name, HP / MP bars and the food
// bar, from what the network handler keeps about the pet (read again by each paint). Shown
// only while the player has a pet (not a summon); a click right of the title bar acts on it.
class NCPetStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCPetStatusWnd, NCStatusWnd, 0, 0x102B0758)

    NCButton* ToggleButton;          // +0x334 (C3) "pet_button" (ID 0xF0F00A1): buff icons on / off
    TArray<INT> BuffSkillIDs;        // +0x338 (C3, name ours) the pet's buffs: skill ids
    TArray<UTexture*> BuffIcons;     // +0x344 (C3) their icon textures
    UTexture* FedTex;                // +0x350 "L2UI_CH3.SmallWnd.FATIGUEBAR"
    UTexture* FedBackTex;            // +0x354 "L2UI_CH3.SmallWnd.FATIGUEBAR_BACK"
    UTexture* PetFrameTopTex;        // +0x358 "L2UI_CH3.FrameCtrl.smallbar1"
    UTexture* PetFrameCenterTex;     // +0x35C "smallbar2"
    UTexture* PetFrameBottomTex;     // +0x360 "smallbar3"
    PetInfo Pet;                     // +0x364 (0xAC) copy of the handler's record, read again by each paint
    INT Unknown410;                  // +0x410 (0) the constructor clears it
    INT Unknown414[2];               // +0x414 (no stock code touches them)
    INT Unknown41C;                  // +0x41C SetInfo
    INT Unknown420;                  // +0x420
    INT Unknown424;                  // +0x424
    UBOOL bBuffs;                    // +0x428 (name ours) buff icons shown (OnToggle)

    NCPetStatusWnd(INT InBarWidth = 100);                                  // 0x100d54d0 (C1 only: no C2 match)
    // ~NCPetStatusWnd                                               // 0x100d5540 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100faaf0 (folded)
    virtual INT OnCreate();                                          // 37 0x10100a60
    virtual void ShowWindow();                                       // 44 0x100fac30
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100fc840
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100fabd0

    void SetInfo(L2ParamStack& Params);                              // 0x100fab50 (name ours)
    virtual INT OnToggle();                                          // 69 0x100fac70 (C3, name ours) the button's command
    virtual INT OnDestroy();                                         // 38 0x10102280 (guard name: NCSummonedStatusWnd's)
    __declspec(noinline) void ClearBuffs();                          // the code of NCPartyStatusWnd's (0x10100ca0)
};
NW_ASSERT_OFFSET(NCPetStatusWnd, BuffSkillIDs, 0x338);
NW_ASSERT_OFFSET(NCPetStatusWnd, BuffIcons, 0x344);
NW_ASSERT_OFFSET(NCPetStatusWnd, FedTex, 0x350);
NW_ASSERT_OFFSET(NCPetStatusWnd, Pet, 0x364);
NW_ASSERT_OFFSET(NCPetStatusWnd, Unknown410, 0x410);
NW_ASSERT_OFFSET(NCPetStatusWnd, Unknown41C, 0x41C);
NW_ASSERT_OFFSET(NCPetStatusWnd, bBuffs, 0x428);
NW_ASSERT_SIZE(NCPetStatusWnd, 0x42C);

// NCSummonedStatusWnd - the summon (UClass "CSummonedStatusWnd", 0x3BC): name, HP / MP bars
// and a bar for the time it has left, run by the console (StartTimer, SetRemaining). Shown only
// while the player has a summon (not a pet).
class NCSummonedStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCSummonedStatusWnd, NCStatusWnd, 0, 0x102B1BF8)

    TArray<INT> BuffSkillIDs;        // +0x334 (C3, name ours) the summon's buffs: skill ids
    TArray<UTexture*> BuffIcons;     // +0x340 (C3) their icon textures
    UTexture* SummonFrameTopTex;     // +0x34C "L2UI_CH3.FrameCtrl.smallbar1"
    UTexture* SummonFrameCenterTex;  // +0x350 "smallbar2"
    UTexture* SummonFrameBottomTex;  // +0x354 "smallbar3"
    NCProgressCtrl* TimeBar;         // +0x358 (ID 0xF0A0B0)
    NCButton* ToggleButton;          // +0x35C (C3) "pet_button" (ID 0xF0A0B1): buff icons on / off
    UBOOL bBuffs;                    // +0x360 (name ours) buff icons shown (OnToggle)
    PetInfo Summon;                  // +0x364 (0xAC) copy of the handler's record, read again by each paint
    UBOOL bTimerStarted;             // +0x410 (StartTimer)
    INT Unknown3B4;                  // +0x414 (0) the constructor clears it

    NCSummonedStatusWnd(INT InBarWidth = 100);                             // 0x100d5950 (C1 only: no C2 match)
    // ~NCSummonedStatusWnd                                          // 0x100d59c0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100fac90 (folded)
    virtual INT OnCreate();                                          // 37 0x10100d40
    virtual INT OnDestroy();                                         // 38 0x10102330
    virtual void ShowWindow();                                       // 44 0x100fad20
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100fccc0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100fabd0

    // names ours
    void SetRemaining(DWORD Remaining);                              // 0x100fad10
    void StartTimer(DWORD Max);                                      // 0x10102410
    virtual INT OnToggle();                                          // 69 0x100facf0 (C3, name ours) the button's command
    __declspec(noinline) void ClearBuffs();                          // 0x10101020 (name ours)
    void AddBuff(INT SkillID);                                       // 0x10101060 (name ours)
};
NW_ASSERT_OFFSET(NCSummonedStatusWnd, BuffSkillIDs, 0x334);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, TimeBar, 0x358);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, bBuffs, 0x360);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, Summon, 0x364);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, bTimerStarted, 0x410);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, Unknown3B4, 0x414);
NW_ASSERT_SIZE(NCSummonedStatusWnd, 0x418);

#pragma pack(pop)