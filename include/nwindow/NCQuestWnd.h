#pragma once
// NCQuestWnd - the quest window (UClass "CQuestWnd", 0xCC, 240 x 300; NCMainWnd creates it,
// NCGMQuestWnd derives from it): the accepted quests in a list and, below, two tab pages: the
// selected quest's description (NCQuestInfoWnd) and the quest items (NCQuestItemWnd). Opening
// it asks the server for the quests again; "Cancel" gives up the selected quest after asking.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCListBox;
class NCQuestInfoWnd;
class NCQuestItemWnd;
class NCTabControl;
struct NItemInfo;

class NCQuestWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCQuestWnd, NCWnd, 0, 0x102A1FC0)

    UTexture* TopTex;                // +0xC8 "L2UI.QuestWnd.questBack1" the top 115 pixels
    UTexture* InfoBackTex;           // +0xCC "L2UI.QuestWnd.questBack2" the rest, page 0
    UTexture* ItemBackTex;           // +0xD0 "L2UI.QuestWnd.questBack3" the rest, page 1
    UTexture* ItemPageIconTex;       // +0xD4 "sek.cbui303" 14 x 14 at (220, 127) on page 1
    NCListBox* QuestList;            // +0xD8 the quest names, the quest ids as parameters
    NCQuestInfoWnd* InfoWnd;         // +0xDC page 0 "Description"
    NCQuestItemWnd* ItemWnd;         // +0xE0 page 1 "Item"
    NCTabControl* TabCtrl;           // +0xE4
    NCButton* CancelButton;          // +0xE8 page 0 only
    NCWnd* NewChild[2];              // +0xEC, +0xC0 new in C2 (NCPushButtons): [0] the arrow back (command 0xF9), [1] the arrow
                                     //   forward (0xF8) of the description page; shown with it, hidden on the item page (OnTab)
    NRectF DropRect;                 // +0xF4 (160, 420, 27.0, 27.0; C3: the size is FLOAT) the rectangle where a quest item
                                     //   dropped asks to be destroyed (IsInDropZone): below the window, so a drop
                                     //   inside never hits it
    INT CurTab;                      // +0x104 page shown

    NCQuestWnd();                                                    // 0x100dde00
    // ~NCQuestWnd                                                   // 0x10073c50 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100dca00
    virtual INT OnCreate();                                          // 37 0x100ddef0
    virtual INT OnDestroy();                                         // 38 0x100dd5a0
    virtual void ShowWindow();                                       // 44 0x100de4a0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100dd750
    // names ours
    virtual UBOOL IsInDropZone(INT X, INT Y);                        // 68 0x100dcba0
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 69 0x100dd960
    virtual INT OnCancel();                                          // 70 0x100dd990
    virtual INT OnSelectQuest(INT Index);                            // 71 0x100dda90
    virtual INT OnTab(INT Page);                                     // 72 0x100dcb00
    virtual INT OnNewButton1();                                      // 73 0x100ddab0 (C2 only; name ours) the command 0xF8: next step
    virtual INT OnNewButton2();                                      // 74 0x100ddad0 (C2 only; name ours) the command 0xF9: previous step

    // names ours: the console fills the window with these
    __declspec(noinline) void Clear();                               // 0x100de470 (called by ShowWindow: out of line)
};
NW_ASSERT_OFFSET(NCQuestWnd, TopTex, 0xC8);
NW_ASSERT_OFFSET(NCQuestWnd, QuestList, 0xD8);
NW_ASSERT_OFFSET(NCQuestWnd, DropRect, 0xF4);
NW_ASSERT_OFFSET(NCQuestWnd, CurTab, 0x104);
NW_ASSERT_SIZE(NCQuestWnd, 0x108);

#pragma pack(pop)