#pragma once
// NCSkillItemWnd - a skill grid of the skill window (UClass "CSkillItemWnd", 0x134; the Active
// and Passive lists of NCSkillWnd, 6 x 7 icons of 32 pixels). Each skill has a cool time icon
// drawn over it (CoolTimes, one per item, same index); the active ones get the icon frame and,
// while pressed, the click frame, and a toggle skill whose effect is on the player the toggle
// animation. Released over the same icon a skill is used (Ctrl forces, Shift stays in place);
// dropped on a shortcut bar slot it is registered there. The tool tip is always the box: name
// and type, "MP cost" and "Range" when the skill has them, description.
// NCGMSkillItemWnd, the GM view of a player's skills, uses nothing on release.

#include "nwindow/NCItemWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCCoolTimeIcon;

class NCSkillItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCSkillItemWnd, NCItemWnd, 0, 0x1028BC08)

    UTexture* IconBackTex;             // +0x148 "l2ui.nwindow.icon_back" frame of an active skill
    UTexture* IconClickTex;            // +0x14C "l2ui.nwindow.icon_click" while pressed
    UTexture* ToggleTex;               // +0x150 "L2_SkillTime.ToggleEffect.ToggleEffect001"   // C2: offset derived, no evidence
    UBOOL bPressed;                    // +0x154 the left button is down on the grid
    TArray<NCCoolTimeIcon*> CoolTimes; // +0x158 one per item (the console starts them)

    NCSkillItemWnd();                                                // 0x100a2250
    // C3: the cell gaps are arguments too (the skill windows pass 5 and 3), ret 0x18
    NCSkillItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x100a22f0
    // ~NCSkillItemWnd                                               // 0x100840f0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a1050
    virtual INT OnDestroy();                                         // 38 0x100a1fb0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100a1180
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x100e4980
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x100a14f0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100a1070
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100a16c0
    virtual void Clear();                                            // 72 0x100a21a0

    // names ours
    INT AddSkill(NItemInfo* Info);                                   // 0x100a2080 no limit
};
NW_ASSERT_OFFSET(NCSkillItemWnd, IconBackTex, 0x148);
NW_ASSERT_OFFSET(NCSkillItemWnd, bPressed, 0x154);
NW_ASSERT_OFFSET(NCSkillItemWnd, CoolTimes, 0x158);
NW_ASSERT_SIZE(NCSkillItemWnd, 0x164);

// NCGMSkillItemWnd - the grids of NCGMSkillWnd (UClass "CGMSkillItemWnd", 0x134): a click
// only selects.
class NCGMSkillItemWnd : public NCSkillItemWnd
{
    NW_DECLARE_CLASS(NCGMSkillItemWnd, NCSkillItemWnd, 0, 0x1027E3C0)

    NCGMSkillItemWnd();                                              // 0x100848f0
    NCGMSkillItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x10084990
    // ~NCGMSkillItemWnd                                             // 0x10084940

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1007e260
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1007e280 (folded)
};
NW_ASSERT_SIZE(NCGMSkillItemWnd, 0x164);

#pragma pack(pop)