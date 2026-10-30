#pragma once
// NCGameOptionWnd - the game page of the options window (UClass "CGameOptionWnd", 0xD4):
// language (native or English), which names show over characters (player, monsters, other PCs
// - and among those clan, party, general), camera tracking, the chat input options, a button
// that puts every window back in place, party loot and the game pad. The settings live in Core
// globals (GL2Show*, GIsEnterChatting...) and in Option.ini [Game].

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCComboBox;
class NCButton;
class NCCheckBox;

// NameBoxes[] (names ours): "Other PCs" switches the last three
enum
{
    NAMEBOX_Player,
    NAMEBOX_Monster,
    NAMEBOX_OtherPC,
    NAMEBOX_Clan,
    NAMEBOX_Party,
    NAMEBOX_General,
    NAMEBOX_MAX
};

// NWM_COMMAND ids of the page's controls
enum
{
    NID_GAMEOPT_LANGUAGE = 0x1F15,
    NID_GAMEOPT_PLAYER = (INT)0xF0F01459,
    NID_GAMEOPT_MONSTER = (INT)0xF0F01460,
    NID_GAMEOPT_OTHERPC = (INT)0xF0F01461,
    NID_GAMEOPT_CLAN = (INT)0xF0F01462,
    NID_GAMEOPT_PARTY = (INT)0xF0F01463,
    NID_GAMEOPT_GENERAL = (INT)0xF0F01464,
    NID_GAMEOPT_TRACKING = (INT)0xF0F01470,
    NID_GAMEOPT_ENTERCHAT = (INT)0xF0F01471,
    NID_GAMEOPT_OLDCHAT = (INT)0xF0F01472,
    NID_GAMEOPT_GAMEPAD = (INT)0xF0F01473,
    NID_GAMEOPT_KEYCRYPT = (INT)0xF0F01474,   // new in C2
    NID_GAMEOPT_TRANSPARENCY = (INT)0xF0F01475,   // new in C3 (name ours)
    NID_GAMEOPT_ARROW = (INT)0xF0F01476,          // new in C3 (name ours)
    NID_GAMEOPT_SHOTQUALITY = (INT)0xFFFF1402,    // new in C3 (name ours)
    NID_GAMEOPT_RESET = (INT)0xFFFF1400,
    NID_GAMEOPT_PARTYLOOT = (INT)0xFFFF1401,
};

class NCGameOptionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGameOptionWnd, NCWnd, 0, 0x102951F0)

    NCComboBox* LanguageCombo;             // +0xC8 0 native, 1 English
    NCButton* ResetButton;                 // +0xCC "Initialize"
    NCWnd* Unused9C[3];                    // +0xD0 destroyed, never created
    NCCheckBox* NameBoxes[NAMEBOX_MAX];    // +0xDC
    NCCheckBox* EnterChatBox;              // +0xF4 GIsEnterChatting
    NCCheckBox* OldChatBox;                // +0xF8 GIsOldChat
    NCCheckBox* TrackingBox;               // +0xFC GL2UseAutoTrackingPawn
    NCCheckBox* TransparencyBox;           // +0x100 new in C3 (name ours): GIsTransparencyMode, Option.ini [Game] TransparencyMode
    NCCheckBox* ArrowBox;                  // +0x104 new in C3 (name ours): GIsArrowMode, [Game] ArrowMode
    NCComboBox* PartyLootCombo;            // +0x108 GL2PartyLooting: 0 finders keepers, 1 random
    NCCheckBox* GamePadBox;                // +0x10C only with a game pad (GL2CanUseJoystick)
    NCCheckBox* KeyCryptBox;               // +0x110 new in C2 (name ours): the nProtect key encryption at startup
    NCComboBox* ScreenShotCombo;           // +0x114 new in C3 (name ours): GL2ScreenShotQuality (four levels)

    NCGameOptionWnd();                                               // 0x100bd520
    // ~NCGameOptionWnd                                              // 0x100a32e0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100b71a0
    virtual INT OnCreate();                                          // 37 0x100bc810
    virtual INT OnDestroy();                                         // 38 0x100ba200
    virtual void ShowWindow();                                       // 44 0x100b7240
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100b9f20
    // names ours
    virtual INT OnOtherPCClicked();                                  // 68 0x100bb020
    virtual INT OnReset();                                           // 69 0x100b7220

    // names ours
    void LoadSettings();                                             // 0x100ba610
    void Apply();                                                    // 0x100baab0
    void ClearItems();                                               // 0x100ba530 (C2 only; before InitItems)
    void InitItems();                                                // 0x100bba80
};
NW_ASSERT_OFFSET(NCGameOptionWnd, Unused9C, 0xD0);
NW_ASSERT_OFFSET(NCGameOptionWnd, NameBoxes, 0xDC);
NW_ASSERT_OFFSET(NCGameOptionWnd, EnterChatBox, 0xF4);
NW_ASSERT_OFFSET(NCGameOptionWnd, PartyLootCombo, 0x108);
NW_ASSERT_SIZE(NCGameOptionWnd, 0x118);

#pragma pack(pop)