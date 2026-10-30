#pragma once
// NCSkillWnd - the skill list (UClass "CSkillWnd", 0x1E0; the page of NCMagicSkillWnd): tabs
// Active and Passive (sysstring 120 / 121), each an NCSkillItemWnd grid of 6 x 8 icons (80 at
// most; C3: 5 pixels apart across, 3 down). The console fills them from the server's skill list
// (AddSkill).

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCSkillItemWnd;
class NCGMSkillItemWnd;
class NCTabControl;
struct NItemInfo;

// NWM_COMMAND ids of the tab controls
enum { NID_SKILL_TABS = 0x0AF11000, NID_GMSKILL_TABS = 0x00F0F012 };

class NCSkillWnd : public NControl
{
    NW_DECLARE_CLASS(NCSkillWnd, NControl, 0, 0x1028C128)

    NCSkillItemWnd* ActiveList;            // +0x1D4
    NCSkillItemWnd* PassiveList;           // +0x1D8
    NCTabControl* TabCtrl;                 // +0x1DC

    NCSkillWnd();                                                    // 0x100a23b0
    // ~NCSkillWnd                                                   // 0x100a2420

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100127a0 (NCButton's, folded)
    virtual INT OnCreate();                                          // 37 0x100a2470
    virtual INT OnDestroy();                                         // 38 0x100a1aa0
    virtual void ShowWindow();                                       // 44 0x100a1130
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10081e00 (NCGMActionWnd's, folded)

    // names ours
    INT AddSkill(NItemInfo* Info);                                   // 0x1007e2a0 the console
    void ClearLists();                                               // 0x100a1150 both lists empty
};
NW_ASSERT_OFFSET(NCSkillWnd, ActiveList, 0x1D4);
NW_ASSERT_OFFSET(NCSkillWnd, TabCtrl, 0x1DC);
NW_ASSERT_SIZE(NCSkillWnd, 0x1E0);

// NCGMSkillWnd - the GM's view of a player's skills (UClass "CGMSkillWnd", 0x1AC; the page of
// NCGMMagicSkillWnd): NCSkillWnd again, a class of its own under NControl with NCGMSkillItemWnd
// grids. Its other methods are copies of NCSkillWnd's that the linker folded into one body
// each (the console's AddSkill calls land on NCSkillWnd's hooks).
class NCGMSkillWnd : public NControl
{
    NW_DECLARE_CLASS(NCGMSkillWnd, NControl, 0, 0x1027FD80)

    NCGMSkillItemWnd* ActiveList;          // +0x1D4
    NCGMSkillItemWnd* PassiveList;         // +0x1D8
    NCTabControl* TabCtrl;                 // +0x1DC

    NCGMSkillWnd();                                                  // 0x100849d0
    // ~NCGMSkillWnd                                                 // 0x10084a40

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100127a0 (folded)
    virtual INT OnCreate();                                          // 37 0x10084a90
    virtual INT OnDestroy();                                         // 38 0x1007f780
    virtual void ShowWindow();                                       // 44 0x100a1130 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10081e00 (folded)

    // names ours
    INT AddSkill(NItemInfo* Info);                                   // 0x1007e2a0 (folded)
    void ClearLists();                                               // 0x100a1150 (folded)
};
NW_ASSERT_OFFSET(NCGMSkillWnd, ActiveList, 0x1D4);
NW_ASSERT_OFFSET(NCGMSkillWnd, TabCtrl, 0x1DC);
NW_ASSERT_SIZE(NCGMSkillWnd, 0x1E0);

#pragma pack(pop)