#pragma once
// NCAbnormalStatusWnd - the buffs and debuffs on the player (UClass "CAbnormalStatusWnd",
// 0xD0): up to 31 skill icons, ten per row, behind a vertical title bar on the left. The
// console adds one per server notice (AddStatus) and clears them all (ClearStatus); a one
// second timer counts their time down, and an icon blinks during its last ten seconds. The
// tool tip of the icon under the mouse shows its name, level and description (the skin's full box).
//
// Desktop (NCConsole) +0xC8 is this window.

#include "nwindow/NCWnd.h"
#include "nwindow/NCItemWnd.h"     // NItemEntry: the window's records are built on it
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

// What the console hands to AddStatus (name ours; 0x2310 bytes in C2, 0x1214 in C1: the strings are longer): the
// console builds it on its stack (AddAbnormalStatus 0x10059A00: Name from GetSkillName, IconName from
// GetSkillIconName, Desc from GetSkillDesc; Icon, ToolTip left empty) and the window copies what it needs into its
// own record (NAbnormalEntry).
struct FAbnormalStatus
{
    TCHAR Name[256];                 // +0x0000 the skill's name (C1: 128)
    TCHAR IconName[128];             // +0x0200 texture name
    TCHAR Desc[2048];                // +0x0300 the skill's description (escaped newlines) (C1: 1024)
    TCHAR ToolTip[2048];             // +0x1300 never filled (C1: "name\ndescription")
    UTexture* Icon;                  // +0x2300 NULL
    INT RemainTime;                  // +0x2304 seconds
    INT SkillID;                     // +0x2308
    INT Level;                       // +0x230C

    // inlined in the console's AddAbnormalStatus (Desc is left)
    FAbnormalStatus()
    {
        Name[0] = 0;
        IconName[0] = 0;
        ToolTip[0] = 0;
        Icon = NULL;
    }
};
NW_ASSERT_OFFSET(FAbnormalStatus, IconName, 0x200);
NW_ASSERT_OFFSET(FAbnormalStatus, Desc, 0x300);
NW_ASSERT_OFFSET(FAbnormalStatus, ToolTip, 0x1300);
NW_ASSERT_OFFSET(FAbnormalStatus, Icon, 0x2300);
NW_ASSERT_OFFSET(FAbnormalStatus, SkillID, 0x2308);
NW_ASSERT_SIZE(FAbnormalStatus, 0x2310);

// One buff on the window (name ours; allocated one by one: the window keeps pointers). An item entry for the skill:
// Icon (+0x0), Name (+0xC), Description (+0x70C), ID = the skill id (+0x1918), UnknownC18 = its level (+0x191C), the
// tool tip (+0x19B8, filled with NTT_SkillName), and the fields below. C3: 0x1A4C bytes (the base is 0x1A34); the
// constructor is 0x10101D40 (NItemEntry's, then bVisible = bFadeOut = 1, RemainTime = 0, Alpha = 255).
struct NAbnormalEntry : public NItemEntry
{
    INT IconX;                       // +0x1A34 (name ours) the icon's place, left by each paint (the tool tip uses it)
    INT IconY;                       // +0x1A38 (name ours)
    INT RemainTime;                  // +0x1A3C seconds (the timer counts it down)
    INT Alpha;                       // +0x1A40 (255) the icon's alpha: in the last ten seconds it goes up and down by 25
                                     //   every 100 ms (TIMER_AbnormalFade)
    UBOOL bVisible;                  // +0x1A44 (1) the icon is drawn
    UBOOL bFadeOut;                  // +0x1A48 (1) the direction of the fade

    NAbnormalEntry() : RemainTime(0), Alpha(255), bVisible(1), bFadeOut(1) {}   // IconX, IconY are left as allocated (as in the stock constructor)
};
NW_ASSERT_OFFSET(NAbnormalEntry, RemainTime, 0x1A3C);
NW_ASSERT_OFFSET(NAbnormalEntry, Alpha, 0x1A40);
NW_ASSERT_OFFSET(NAbnormalEntry, bFadeOut, 0x1A48);
NW_ASSERT_SIZE(NAbnormalEntry, 0x1A4C);

enum { TIMER_AbnormalStatus = 0xF9, TIMER_AbnormalFade = 0xE9 };

class NCAbnormalStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCAbnormalStatusWnd, NCWnd, 0, 0x102B2120)

    TArray<NAbnormalEntry*> Statuses; // +0xC8 the buffs, 31 at most
    TArray<NAbnormalEntry*> Statuses2; // +0xD4 (new in C3, name ours) a second list the paint draws after the first
                                     //   (its count is +0xD8; OnPaint, OnDrawToolTip and vslot 68 read it)
    NAbnormalEntry* ExtraStatus;     // +0xE0 (new in C3, name ours) one more icon, drawn after the lists and timed like them
    INT TimerID;                     // +0xE4 SetTimer(0xF9, 1000): the time runs down
    INT FadeTimerID;                 // +0xE8 SetTimer(0xE9, 100): the blink of the last ten seconds
    UTexture* BackTex;               // +0xEC "L2UI.EtcWndBack.AbnormalBack" under each icon
    UTexture* DebuffTex;             // +0xF0 "L2UI_CH3.PlayerStatusWnd.debuff" over the icons of a debuff (C3)
    UTexture* ToolTipTex;            // +0xF4 "L2UI.tooltip.mini-tool01" (not drawn here)
    UTexture* TipTopTex;             // +0xF8 "L2UI.Tooltip.Tool02" (C1: the box of the tool tip; C2 draws it with the skin)
    UTexture* TipMidTex;             // +0xFC "L2UI.Tooltip.Tool01"
    UTexture* TipBottomTex;          // +0x100 "L2UI.Tooltip.Tool03"
    INT IconSize;                    // +0x104 constructor argument (24); HitTest uses it
    INT HoverIndex;                  // +0x108 icon under the mouse, -1 = none

    NCAbnormalStatusWnd(INT InIconSize = 24);                             // 0x100d8560 (C1 only: no C2 match)
    // ~NCAbnormalStatusWnd                                          // 0x100d85f0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (NControl's, folded)
    virtual INT OnTimer(INT InTimerID);                              // 36 0x100fc2b0
    virtual INT OnCreate();                                          // 37 0x100ff510
    virtual INT OnDestroy();                                         // 38 0x10103950
    virtual void HideWindow();                                       // 45 0x100412d0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100fb880
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x10102820 (the db calls it OnPostPaint)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x100fc210

    // names ours
    INT HitTest(INT X, INT Y);                                       // 0x100fa4f0
    INT AddStatus(FAbnormalStatus* Status);                          // 0x10101d90 0 when full
    void ClearStatus();                                              // 0x10102be0
    virtual void UpdateLayout();                                     // 68 0x100fa550 (only this client has it) the size from the icons held
};
NW_ASSERT_OFFSET(NCAbnormalStatusWnd, Statuses, 0xC8);
NW_ASSERT_OFFSET(NCAbnormalStatusWnd, Statuses2, 0xD4);
NW_ASSERT_OFFSET(NCAbnormalStatusWnd, ExtraStatus, 0xE0);
NW_ASSERT_OFFSET(NCAbnormalStatusWnd, TimerID, 0xE4);
NW_ASSERT_OFFSET(NCAbnormalStatusWnd, BackTex, 0xEC);
NW_ASSERT_OFFSET(NCAbnormalStatusWnd, TipTopTex, 0xF8);
NW_ASSERT_OFFSET(NCAbnormalStatusWnd, HoverIndex, 0x108);
NW_ASSERT_SIZE(NCAbnormalStatusWnd, 0x10C);

// 0x1005f6c0 (anonymous, name ours; ret 8, `this` is not used) - whether a skill is a debuff: its record in the skill
// table has a value at +0x48 (the buff windows draw the "debuff" frame over the icon). Defined in
// NCBroadcastObserverBuffWnd.cpp; called on GConsole
struct NConsoleWndSkillExt
{
    __declspec(noinline) INT IsDebuffSkill(INT SkillID, INT Level);
};

#pragma pack(pop)
