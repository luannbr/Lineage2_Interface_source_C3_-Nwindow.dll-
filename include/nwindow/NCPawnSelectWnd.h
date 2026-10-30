#pragma once
// NCPawnSelectWnd - the character panel of the character selection (UClass "CPawnSelectWnd",
// 0x1038; NCLobbyWnd +0xC8): up to seven characters (their User records by value, each with the
// actor shown on the stage), a name combo box, and the chosen one's level, class, HP / MP / exp
// bars and numbers. Choosing a character marked for deletion asks whether to restore it.

#include <math.h>
#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCComboBox;

// C2: User::DeleteDays (+0x144) is two things now: the low 16 bits are the days left until the
// deletion, bit 16 (the stock code computes it as pow(2, 16)) locks the character: the character
// selection neither selects it nor starts the game with it (names ours)
#define CHAR_LOCKED_FLAG ((INT)pow(2.0, 16.0))

class NCPawnSelectWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnSelectWnd, NCWnd, 0, 0x10284FF0)

    UTexture* BackTex;               // +0xC8 "L2UI.statusWnd.selectback"   // C2: offset derived, no evidence
    UTexture* HPTex;                 // +0xCC "L2UI.StatusWnd.statusHP1"   // C2: offset derived, no evidence
    UTexture* MPTex;                 // +0xD0 "L2UI.StatusWnd.statusMP1"   // C2: offset derived, no evidence
    UTexture* EXPTex;                // +0xD4 "L2UI.StatusWnd.statusEXP1"   // C2: offset derived, no evidence
    INT UnknownA4;                   // +0xD8 (0)   // C2: offset derived, no evidence
    NCComboBox* NameCombo;           // +0xDC
    User Characters[7];              // +0xE0 the User records by value (0x1F8 each in C2, 0x208 in C1); the actor of each   // C2: offset derived, no evidence
                                     //   is its Pawn (+0x140)
    INT NumCharacters;               // +0x1030
    INT Selected;                    // +0x1034 -1 = none

    NCPawnSelectWnd();                                               // 0x10090c40
    // ~NCPawnSelectWnd                                              // 0x10081140 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1008f350
    virtual INT OnCreate();                                          // 37 0x10090b20
    virtual INT OnDestroy();                                         // 38 0x10090190
    virtual void ShowWindow();                                       // 44 0x100412c0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1008f9c0
    // names ours
    virtual void SelectCharacter(INT Index);                         // 68 0x1008f410
    virtual INT OnSelect(INT Index);                                 // 69 0x10090370
    virtual INT OnRestoreAnswer(INT bYes, INT Unused);               // 70 0x1008f540

    // names ours
    INT AddCharacter(User Info);                                     // 0x10090240
    __declspec(noinline) void SpawnCharacter(User* Info);            // 0x1008f3d0 the actor of a record (C2: inline)
    INT GetSelected();                                               // 0x1008f460
    TCHAR* GetCharacterName(INT Index);                              // 0x1008f470
    void ClearCharacters();                                          // 0x1008f4c0
};
NW_ASSERT_OFFSET(NCPawnSelectWnd, NameCombo, 0xDC);
NW_ASSERT_OFFSET(NCPawnSelectWnd, Characters, 0xE0);
NW_ASSERT_OFFSET(NCPawnSelectWnd, NumCharacters, 0x1030);
NW_ASSERT_SIZE(NCPawnSelectWnd, 0x1038);

#pragma pack(pop)