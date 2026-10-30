#pragma once
// NCQuestGaraTreeWnd - the quest tab of the main window (UClass "CQuestGaraTreeWnd", 0x180; new in C3; NCMainWnd
// creates it as tab 4, starting hidden): the accepted quests in an NCQuestGaraTreeCtrl on the "QuestWndBack" panel
// (256 x 335) with the count "(n/15)" and a "?" icon, a "Cancel" button and a check box that keeps the selected
// quest's place marked on the map. Opening it asks the server for the quests again (NConsoleWnd slot 398).
// Slots 69 and 70 (the button's and the check box's commands) are named NCGMQuestWnd::OnDialogResult / OnDestroy in
// the db: they are this class'.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCCheckBox;
class NCQuestGaraTreeCtrl;

class NCQuestGaraTreeWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCQuestGaraTreeWnd, NCWnd, 0, 0x102A1050)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.QUESTWND.QuestWndBack"
    NCQuestGaraTreeCtrl* TreeCtrl;   // +0xCC the quests (7, 27, 242 x 274) (name ours)
    NCButton* CancelButton;          // +0xD0 id 0xFD, text 0x181 (name ours)
    NCCheckBox* MarkCheckBox;        // +0xD4 id 0xFF00FF23, text 0x35C, checked at the start (name ours)
    UTexture* ToolTipBtnTex;         // +0xD8 "L2UI_CH3.QUESTWND.QuestWndToolTipBtn" the "?" icon
    BYTE bDC;                        // +0xDC (0) set with bDD when the selected quest has no map place (name ours)
    BYTE bDD;                        // +0xDD (0)
    INT NumQuests;                   // +0xE0 the count shown in "(n/15)" (the console fills it) (name ours)
    FString ToolTipText;             // +0xE4 (SetToolTipInfo)
    INT ToolTipInfo[4];              // +0xF0 (SetToolTipInfo's four numbers; [3] is the quest id GetTrackedQuestID returns)
    UBOOL bShowToolTip;              // +0x100 the mouse is over the "?" icon (name ours)
    NToolTipInfo ToolTip;            // +0x104 the list of the five info icons and their texts (OnCreate)

    NCQuestGaraTreeWnd();                                            // 0x100DC280
    // ~NCQuestGaraTreeWnd                                           // 0x10084280

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100DA270
    virtual INT OnCreate();                                          // 37 0x100DC310
    virtual INT OnDestroy();                                         // 38 0x100DB400
    virtual void ShowWindow();                                       // 44 0x100DA310
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100DBB50
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x100DC8A0 (the db names it OnPostPaint)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x100DB6D0
    // names ours
    virtual void ClearList();                                        // 68 0x100DA2F0 the tree emptied, the count 0
    virtual INT OnCancelQuest();                                     // 69 0x100DB4A0 (db: NCGMQuestWnd::OnDialogResult)
    virtual INT OnMarkCheckBox();                                    // 70 0x100DC050 (db: NCGMQuestWnd::OnDestroy)

    // 0x100DA3C0 (ret 0x1C, name ours): the tool tip of the selected quest's map place
    void SetToolTipInfo(FString Text, INT A, INT B, INT C, INT D);
    // 0x10116090 (name ours): the quest id of the map place SetToolTipInfo was given last (its fourth number)
    INT GetTrackedQuestID();
    // 0x100DB5D0 (name ours): the map's mark off (the desktop's map window, its map's bShowMark) and the two flags
    // cleared; NCConsole::Reset calls it on the main window's quest tab
    __declspec(noinline) void ClearMark();
};
NW_ASSERT_OFFSET(NCQuestGaraTreeWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCQuestGaraTreeWnd, TreeCtrl, 0xCC);
NW_ASSERT_OFFSET(NCQuestGaraTreeWnd, ToolTipBtnTex, 0xD8);
NW_ASSERT_OFFSET(NCQuestGaraTreeWnd, bDC, 0xDC);
NW_ASSERT_OFFSET(NCQuestGaraTreeWnd, NumQuests, 0xE0);
NW_ASSERT_OFFSET(NCQuestGaraTreeWnd, ToolTipText, 0xE4);
NW_ASSERT_OFFSET(NCQuestGaraTreeWnd, ToolTipInfo, 0xF0);
NW_ASSERT_OFFSET(NCQuestGaraTreeWnd, bShowToolTip, 0x100);
NW_ASSERT_OFFSET(NCQuestGaraTreeWnd, ToolTip, 0x104);
NW_ASSERT_SIZE(NCQuestGaraTreeWnd, 0x180);

#pragma pack(pop)
