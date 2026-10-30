#pragma once
// NCBroadcastGMWnd - the GM's control window of the broadcast ("event match") tools (UClass "CBroadcastGMWnd", 0x738; new in C3: the
// desktop NCConsole makes it at +0x1AC, NConsoleWnd shows it and feeds it the server's replies). A GM sets two parties (their
// names, leaders and locks) up, creates a game, starts / pauses / ends it, sets the score, sends announcements and
// fireworks, opens the fence window (NCBroadcastGMFenceWnd) and a page of command links read from an option file; every
// button sends one "eventmatch ..." command through the console's virtual 192 (0x1005EEB0: a command line as an FString).
// The server answers with the Notify* calls (NConsoleWnd::OnEventMatch*), which enable / disable the buttons and fill the
// two team lists.
//
// Layout and virtuals from the stock constructor 0x10112820, destructor 0x101129C0, OnCreate 0x10110080, OnDestroy 0x1010E6C0,
// OnPaint 0x1010EAB0, OnTimer 0x1010EED0 and the handlers 0x1010CB70 .. 0x101120C0 (names ours unless a guard string says it).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCButton;
class NCEditBox;
class NCListCtrl;
class L2ParamStack;

// A pair of numbers read from the option file (names ours). A buff is (BuffID, BuffLv). A skill exception is stored as
// (the last BuffID read, ExpSkillID): the stock read leaves the first word stale and the readers (the "A%d" / "D%d" lists
// of OnStartBroadcastGameBtnClick) use the second one
struct NBroadcastGMPair
{
    INT First;                       // +0x00
    INT Second;                      // +0x04
};
NW_ASSERT_SIZE(NBroadcastGMPair, 0x8);

// A player of a team as the server lists it (0x50 bytes; the constructor 0x1010FF10 and the destructor 0x1010FFC0 are the
// stock out-of-line copies, hooked in NCBroadcastGMWnd.cpp). Filled by NotifyTeamInfo, shown by NotifyTeamInfoEnd.
struct NBroadcastGMMember
{
    INT ID;                          // +0x00 object id the server gave (-1: the slot is free)
    FString Name;                    // +0x04
    INT Value10;                     // +0x10 the first number of the packet (also copied to +0x34)
    INT Value14;                     // +0x14 .. +0x30: the next eight numbers of the packet
    INT Value18;                     // +0x18
    INT Value1C;                     // +0x1C
    INT Value20;                     // +0x20
    INT Value24;                     // +0x24
    INT Level;                       // +0x28 shown in the list's second cell (name ours)
    INT ClassID;                     // +0x2C shown as the class name (NConsoleWnd::GetClassNameString) (name ours)
    INT Value30;                     // +0x30
    INT Value34;                     // +0x34 a copy of Value10
    TArray<INT> ListA;               // +0x38 emptied by the constructor, nothing in this window's code fills it
    TArray<INT> ListB;               // +0x44

    NBroadcastGMMember();            // 0x1010FF10
    ~NBroadcastGMMember();           // 0x1010FFC0
};
NW_ASSERT_OFFSET(NBroadcastGMMember, Name, 0x4);
NW_ASSERT_OFFSET(NBroadcastGMMember, Level, 0x28);
NW_ASSERT_OFFSET(NBroadcastGMMember, ListA, 0x38);
NW_ASSERT_OFFSET(NBroadcastGMMember, ListB, 0x44);
NW_ASSERT_SIZE(NBroadcastGMMember, 0x50);

// A team (0x2E0 bytes; constructor 0x10110020 and destructor 0x10111DC0): up to nine players
struct NBroadcastGMTeam
{
    INT Unknown00;                   // +0x00 never touched
    FString Name;                    // +0x04
    NBroadcastGMMember Members[9];   // +0x10

    NBroadcastGMTeam();              // 0x10110020
    ~NBroadcastGMTeam();             // 0x10111DC0
};
NW_ASSERT_OFFSET(NBroadcastGMTeam, Members, 0x10);
NW_ASSERT_SIZE(NBroadcastGMTeam, 0x2E0);

// What the option file lists (ReadBroadcastOptionFile): the teleport delay, which skills / items the GM may use ("DefaultAllow"
// with the exceptions), and the buffs to cast when a game starts. 0x30 bytes (its inline destructor is 0x10111D60); the three
// numbers are left alone by the constructor, the read sets them.
struct NBroadcastGMOption
{
    INT TeleportDelay;               // +0x00 "Etc" / "TeleportDelay"
    INT SkillDefaultAllow;           // +0x04 "Skill" / "DefaultAllow"
    INT ItemDefaultAllow;            // +0x08 "Item" / "DefaultAllow"
    TArray<NBroadcastGMPair> ExpSkills;   // +0x0C the exceptions to the skill default
    TArray<INT> ExpItems;            // +0x18 the exceptions to the item default
    TArray<NBroadcastGMPair> Buffs;  // +0x24 "Buff" / "BuffID%d", "BuffLv%d"
};
NW_ASSERT_OFFSET(NBroadcastGMOption, ExpSkills, 0xC);
NW_ASSERT_OFFSET(NBroadcastGMOption, ExpItems, 0x18);
NW_ASSERT_OFFSET(NBroadcastGMOption, Buffs, 0x24);
NW_ASSERT_SIZE(NBroadcastGMOption, 0x30);

class NCBroadcastGMWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCBroadcastGMWnd, NCWnd, 0, 0x102B7DC0)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.Minimap.MapBack" (OnCreate)
    NCEditBox* PartyAEdit;           // +0xCC the party A name (id 0xFFFFE001) (names ours for every control)
    NCEditBox* PartyBEdit;           // +0xD0 the party B name (0xFFFFE002)
    NCEditBox* ScoreAEdit;           // +0xD4 the score of A, one digit (0xFFFFE009)
    NCEditBox* ScoreBEdit;           // +0xD8 the score of B (0xFFFFE00A)
    NCEditBox* OptionFileEdit;       // +0xDC the option file to read when the game starts (0xFFFFE003)
    NCEditBox* CommandFileEdit;      // +0xE0 the command file the command page lists (0xFFFFE004)
    NCEditBox* LeaderAEdit;          // +0xE4 the party A leader's name (0xFFFFE005)
    NCEditBox* LeaderBEdit;          // +0xE8 the party B leader's name (0xFFFFE006)
    NCEditBox* AnnounceEdit;         // +0xEC the announcement text, 25 characters (0xFFFFE008)
    NCButton* CommandButton;         // +0xF0 id 0xFFFFD003 shows the command page
    NCButton* SetLeaderAButton;      // +0xF4 0xFFFFD004
    NCButton* LockAButton;           // +0xF8 0xFFFFD005
    NCButton* SetLeaderBButton;      // +0xFC 0xFFFFD006
    NCButton* LockBButton;           // +0x100 0xFFFFD007
    NCButton* AnnounceButton;        // +0x104 0xFFFFD009
    NCButton* StartButton;           // +0x108 0xFFFFD00A
    NCButton* CreateButton;          // +0x10C 0xFFFFD00B creates / removes the game
    NCButton* PauseButton;           // +0x110 0xFFFFD00C pauses / resumes
    NCButton* CounterButton;         // +0x114 0xFFFFD00D sets the score
    NCButton* EndMsgButton;          // +0x118 0xFFFFD00E sends the "game ended" announcement
    NCButton* FenceButton;           // +0x11C 0xFFFFD00F opens the fence window / removes the fence
    NCButton* FireworkAButton;       // +0x120 0xFFFFD010 fireworks for the party A
    NCButton* FireworkBButton;       // +0x124 0xFFFFD011
    NCListCtrl* TeamAList;           // +0x128 id 0xFFFFC001 (the constructor leaves both pointers alone)
    NCListCtrl* TeamBList;           // +0x12C 0xFFFFC002
    FString GameID;                  // +0x130 the id of the created game as text ("" without one)
    NBroadcastGMOption Option;       // +0x13C the option file's lists
    NBroadcastGMTeam Teams[2];       // +0x16C the two teams (A, B)
    INT TeamFillIndex;               // +0x72C the team NotifyTeamInfo fills (0 A, 1 B, -1 none)
    INT CountDownStep;               // +0x730 the countdown's step (4 .. 0; -1 none)
    INT CountDownTimer;              // +0x734 the timer id of the countdown (0: none)

    NCBroadcastGMWnd();                                              // 0x10112820
    // ~NCBroadcastGMWnd                                             // 0x101129C0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10112660
    virtual INT OnTimer(INT TimerID);                                // 36 0x1010EED0 the countdown (0xFFFFA001)
    virtual INT OnCreate();                                          // 37 0x10110080
    virtual INT OnDestroy();                                         // 38 0x1010E6C0
    virtual void Minimize();                                         // 49 0x1010E540 (the db says MinimizeWindow)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1010EAB0

    // the buttons (WindowProc: NWM_COMMAND with the id)
    INT OnShowBroadcastCommandWndBtnClick();                         // 0x10111E20
    INT OnSetBroadcastPartyALeaderBtnClick();                        // 0x1010CB70
    INT OnLockBroadcastPartyABtnClick();                             // 0x1010F250
    INT OnSetBroadcastPartyBLeaderBtnClick();                        // 0x1010CC10
    INT OnLockBroadcastPartyBBtnClick();                             // 0x1010F490
    INT OnSendBroadcastAnnounceBtnClick();                           // 0x1010F6D0
    INT OnStartBroadcastGameBtnClick();                              // 0x101120C0
    INT OnCreateBroadcastGameBtnClick();                             // 0x1010F7B0
    INT OnPauseBroadcastGameBtnClick();                              // 0x1010F9C0
    INT OnSetGameCounterBtnClick();                                  // 0x1010FB60
    INT OnSendGameEndMsgBtnClick();                                  // 0x1010FC50
    INT OnControlFenceBtnClick();                                    // 0x1010FD10
    INT OnTeamAFirecrackerBtnClick();                                // 0x1010CCB0
    INT OnTeamBFirecrackerBtnClick();                                // 0x1010CD90

    // the server's replies (NConsoleWnd::OnEventMatch*; the console calls them by address)
    FString GetBroadcastGameID();                                    // 0x1010CE70
    void NotifyCreateBroadcastGame(FString InGameID);                // 0x1010CF00
    void NotifyRemoveBroadcastGame();                                // 0x1010D090
    void NotifyLockPartyA();                                         // 0x1010D290
    void NotifyLockPartyB();                                         // 0x1010D380
    void NotifyUnlockPartyA();                                       // 0x1010D470
    void NotifyUnlockPartyB();                                       // 0x1010D540 (the stock guard string says NotifyLockPartyB)
    void NotifyStartBroadcastGame();                                 // 0x1010D610
    void NotifyEndBroadcastGame();                                   // 0x1010D750
    void NotifyPauseBroadcastGame();                                 // 0x1010D840
    void NotifyResumeBroadcastGame();                                // 0x1010D8E0
    void NotifyTeamInfoStart(INT InGameID, INT Team, INT Unused);    // 0x1010D980
    void NotifyTeamInfo(INT Index, INT ID, FString Name, INT P1, INT P2, INT P3, INT P4, INT P5, INT P6, INT P7,
                        INT P8, INT P9, INT P10);                    // 0x1010DA90
    void NotifyTeamInfoEnd();                                        // 0x1010DB90
    void NotifyFenceInfo(FString X, FString Y, FString Z, FString Width, FString Height);   // 0x1010E3C0
    void StartCountDown();                                           // 0x1010E5E0 the timer and the first announcement
    // the console's entries for a game found when the window is opened (NConsoleWnd 382 / 383; names ours): they take the
    // next values of the packet with L2ParamStack::Top (ret 4 each)
    void NotifyGameInfo(L2ParamStack* Params);                       // 0x1010DD50 the game's state, parties, locks and fence
    void NotifyTeamMember(L2ParamStack* Params);                     // 0x1010E110 a player of a party (his row and his leader box)
    UBOOL ReadBroadcastOptionFile(FString FileName);                 // 0x10111660 the option file (GConfig), 0 on a bad one
};
NW_ASSERT_OFFSET(NCBroadcastGMWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCBroadcastGMWnd, PartyAEdit, 0xCC);
NW_ASSERT_OFFSET(NCBroadcastGMWnd, AnnounceEdit, 0xEC);
NW_ASSERT_OFFSET(NCBroadcastGMWnd, CommandButton, 0xF0);
NW_ASSERT_OFFSET(NCBroadcastGMWnd, FireworkBButton, 0x124);
NW_ASSERT_OFFSET(NCBroadcastGMWnd, TeamAList, 0x128);
NW_ASSERT_OFFSET(NCBroadcastGMWnd, GameID, 0x130);
NW_ASSERT_OFFSET(NCBroadcastGMWnd, Option, 0x13C);
NW_ASSERT_OFFSET(NCBroadcastGMWnd, Teams, 0x16C);
NW_ASSERT_OFFSET(NCBroadcastGMWnd, TeamFillIndex, 0x72C);
NW_ASSERT_OFFSET(NCBroadcastGMWnd, CountDownTimer, 0x734);
NW_ASSERT_SIZE(NCBroadcastGMWnd, 0x738);

#pragma pack(pop)
