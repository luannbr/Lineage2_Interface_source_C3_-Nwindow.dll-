#pragma once
// NCMagicSkillWnd - the skill window (UClass "CMagicSkillWnd", 0x9C; a page of the character
// window, NCMainWnd): the NCSkillWnd lists on "L2UI.SkillWnd.skillBack". Shown, it empties
// the lists and asks the server for the skill list; hidden, it empties them.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCSkillWnd;
class NCGMSkillWnd;

// NWM_COMMAND ids: the skill list (both windows), the GM window's title bar
enum { NID_MAGICSKILL_LIST = 0x1F01, NID_GMMAGICSKILL_FRAME = 0xF0F117 };

class NCMagicSkillWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMagicSkillWnd, NCWnd, 0, 0x1028B6E8)

    UTexture* BackTex;                     // +0xC8 "L2UI.SkillWnd.skillBack"   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    NCSkillWnd* SkillWnd;                  // +0xCC

    NCMagicSkillWnd();                                               // 0x100a26b0
    // ~NCMagicSkillWnd                                              // 0x10091670 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x100a2750
    virtual INT OnDestroy();                                         // 38 0x100a1bd0 (NCPetActionWnd's)
    virtual void ShowWindow();                                       // 44 0x100a1d20
    virtual void HideWindow();                                       // 45 0x100a1da0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100a1c80

    // names ours
    __declspec(noinline) void ClearSkills();                                              // 0x100a1170
    void ReloadSkillTexts();                                         // 0x100a1db0 new language
};
NW_ASSERT_OFFSET(NCMagicSkillWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCMagicSkillWnd, SkillWnd, 0xCC);
NW_ASSERT_SIZE(NCMagicSkillWnd, 0xD0);

// NCGMMagicSkillWnd - a GM's view of another player's skills (UClass "CGMMagicSkillWnd", 0x9C;
// desktop window): an NCGMSkillWnd under a title bar, on the same background. Hidden, it
// empties the lists; the console fills them.
class NCGMMagicSkillWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGMMagicSkillWnd, NCWnd, 0, 0x1027EE08)

    NCGMSkillWnd* SkillWnd;                // +0xC8
    UTexture* BackTex;                     // +0xCC "L2UI.SkillWnd.skillBack"

    NCGMMagicSkillWnd();                                             // 0x10084cd0
    // ~NCGMMagicSkillWnd                                            // 0x10074730 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x10084d70
    virtual INT OnDestroy();                                         // 38 0x1007f880
    virtual void ShowWindow();                                       // 44 0x100412c0 (folded)
    virtual void HideWindow();                                       // 45 0x1007f9d0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1007f930

    // names ours
    __declspec(noinline) void ClearSkills();                                              // 0x1007e2e0
};
NW_ASSERT_OFFSET(NCGMMagicSkillWnd, SkillWnd, 0xC8);
NW_ASSERT_OFFSET(NCGMMagicSkillWnd, BackTex, 0xCC);
NW_ASSERT_SIZE(NCGMMagicSkillWnd, 0xD0);

#pragma pack(pop)