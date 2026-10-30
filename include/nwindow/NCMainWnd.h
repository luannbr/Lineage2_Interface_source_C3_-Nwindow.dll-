#pragma once
// NCMainWnd - the character window (UClass "CMainWnd", 0xAC; Alt+T, Alt+K, Alt+C, Alt+U): four
// pages under a row of tab buttons - status (NCDetailStatusWnd), skills (NCMagicSkillWnd),
// actions (NCActionWnd) and quests (NCQuestWnd) - and the title of the page shown. Minimized,
// its icon is the page's.
//
// Desktop (NCConsole) +0x94 is this window.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCDetailStatusWnd;
class NCActionWnd;
class NCTabControl;

// NWM_COMMAND ids (name ours)
enum
{
    NID_MAIN_FRAME = 0xF2A6,
    NID_MAIN_TABS = 0xF2A7,           // lParam: the tab clicked
};

// the pages, in tab order (names ours)
enum
{
    MAINTAB_Status = 0,
    MAINTAB_Skills = 1,
    MAINTAB_Actions = 2,
    MAINTAB_Pledge = 3,             // new in C3 (NCPledgeWnd, SysString 439)
    MAINTAB_Quests = 4,
};

class NCMainWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMainWnd, NCWnd, 0, 0x1027AAF8)

    NCDetailStatusWnd* DetailWnd;    // +0xC8 tab 0
    NCActionWnd* ActionWnd;          // +0xCC tab 2
    NCWnd* PledgeWnd;                // +0xD0 tab 3 NCPledgeWnd (new in C3)
    NCWnd* SkillWnd;                 // +0xD4 tab 1 NCMagicSkillWnd
    NCWnd* QuestWnd;                 // +0xD8 tab 4 NCQuestGaraTreeWnd (C3; starts hidden)
    NCTabControl* TabCtrl;           // +0xDC
    UTexture* BackTex;               // +0xE0 "L2UI.NWindow.mainwndback" (behind the tabs)

    NCMainWnd();                                                     // 0x10056880
    // ~NCMainWnd                                                    // 0x1004c830 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10053eb0
    virtual INT OnCreate();                                          // 37 0x10055140
    virtual INT OnDestroy();                                         // 38 0x10054900
    virtual void ShowWindow();                                       // 44 0x1005a3a0
    virtual void Minimize();                                         // 49 0x10054cb0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10054c10
    virtual INT OnTabChanged(INT Tab);                               // 68 0x10053f10 (name ours)

    // names ours
    void ShowTab(INT Tab);                                           // 0x10054a80 (shortcut keys)
    // 0x100565A0 (anonymous in the db): the clan page's tab button enabled again and the current page shown again (the
    // console calls it when the clan information arrives or goes); the window hides itself when it was not shown
    void UpdatePledgeTab();
};
NW_ASSERT_OFFSET(NCMainWnd, DetailWnd, 0xC8);
NW_ASSERT_OFFSET(NCMainWnd, TabCtrl, 0xDC);
NW_ASSERT_SIZE(NCMainWnd, 0xE4);

#pragma pack(pop)