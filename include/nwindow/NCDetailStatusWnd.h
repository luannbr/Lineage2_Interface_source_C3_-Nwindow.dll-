#pragma once
// NCDetailStatusWnd - the character tab of the status window (UClass "CDetailStatusWnd",
// 0xD4; Alt+T): title and name, clan, level and class, HP / MP / exp / weight bars, the combat
// values, the six stats, karma and PvP, all read from the player's User at each paint.
// NCGMDetailStatusWnd derives from it.
//
// NCMainWnd +0x94 is this window (tab 0, created at 0, 65, 240 x 300).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCDetailStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCDetailStatusWnd, NCWnd, 0, 0x102B2640)

    User* Player;                    // +0xC8 the player (each paint)
    UTexture* BackTex;               // +0xCC "L2UI_CH3.PlayerStatusWnd.myinfo_back"
    UTexture* HPTex;                 // +0xD0 "L2UI_CH3.PlayerStatusWnd.ps_hpbar"
    UTexture* MPTex;                 // +0xD4 "L2UI_CH3.PlayerStatusWnd.ps_mpbar"
    UTexture* EXPTex;                // +0xD8 "L2UI_CH3.PlayerStatusWnd.ps_expbar"
    BYTE _UnknownDC[0xE0 - 0xDC];    // +0xDC not set by C3's OnCreate (C2: SPTex)
    UTexture* WeightTex[5];          // +0xE0 "L2UI_CH3.PlayerStatusWnd.ps_weightBar1" .. 5: up to 50%, 66.6%, 80%, 100% of
                                     //   the weight limit, above (C2 had four at +0xE0 and the name end pieces after them)
    BYTE _UnknownF4[0xFC - 0xF4];    // +0xF4 (C2: NameRightTex, CrestTex; C3 does not touch them in this class)
    UTexture* CPTex;                 // +0xFC "L2UI_CH3.PlayerStatusWnd.ps_cpbar" (new in C3)
    BYTE _Unknown100[0x104 - 0x100]; // +0x100
    INT UnknownCC[17];               // +0x104 .. +0x144 cleared by the constructor (no other stock code touches them) except for
                                     //   six that OnPaint reads as the stats' bonuses, shown "N(+B)": see BONUS_* (names ours;
                                     //   the enum's indexes are the C2 ones, (offset - 0xCC) / 4, and still index this array)
    enum { BONUS_Int = (0xE4 - 0xCC) / 4, BONUS_Str = (0xEC - 0xCC) / 4, BONUS_Con = (0xF4 - 0xCC) / 4,
           BONUS_Men = (0xFC - 0xCC) / 4, BONUS_Dex = (0x104 - 0xCC) / 4, BONUS_Wit = (0x10C - 0xCC) / 4 };
    BYTE _Unknown148[0x154 - 0x148]; // +0x148 (up to sizeof)

    NCDetailStatusWnd();                                             // 0x10103830
    // ~NCDetailStatusWnd                                            // 0x10073b50 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x100fb250
    virtual INT OnDestroy();                                         // 38 0x100fb320 (folded)
    virtual void ShowWindow();                                       // 44 0x100412c0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100fd610
};
NW_ASSERT_OFFSET(NCDetailStatusWnd, Player, 0xC8);
NW_ASSERT_OFFSET(NCDetailStatusWnd, WeightTex, 0xE0);
NW_ASSERT_OFFSET(NCDetailStatusWnd, CPTex, 0xFC);
NW_ASSERT_OFFSET(NCDetailStatusWnd, UnknownCC, 0x104);
NW_ASSERT_SIZE(NCDetailStatusWnd, 0x154);

// NCGMDetailStatusWnd - a GM's view of another player's status (UClass "CGMDetailStatusWnd",
// 0x2DC): the character tab under a title bar, laid out a little lower, drawn from the User it
// keeps (the console fills it with the server's answer). HP and MP in the common font with a
// shadow, exp in percent at the right, a white name (no karma color) with the title before it
// in one string, the crest looked up by its texture name.
//
// Desktop (NCConsole) +0x120 is this window.
class NCGMDetailStatusWnd : public NCDetailStatusWnd
{
    NW_DECLARE_CLASS(NCGMDetailStatusWnd, NCDetailStatusWnd, 0, 0x10280CF8)

    User Target;                     // +0x154 (0x1F8 in C2) the GM's answer; Engine's constructor builds it

    NCGMDetailStatusWnd();                                           // 0x10084ed0
    // ~NCGMDetailStatusWnd                                          // 0x10074ab0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100127a0 (folded)
    virtual INT OnCreate();                                          // 37 0x10082790
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1007f9e0

    void SetTarget(User InTarget);                                   // 0x1007e2f0 (name ours) the
                                                                     //   console's, by value
};
NW_ASSERT_OFFSET(NCGMDetailStatusWnd, Target, 0x154);
NW_ASSERT_SIZE(NCGMDetailStatusWnd, 0x384);

// NWM_COMMAND id of the GM window's title bar
enum { NID_GMDETAIL_FRAME = 0xF0F118 };

#pragma pack(pop)