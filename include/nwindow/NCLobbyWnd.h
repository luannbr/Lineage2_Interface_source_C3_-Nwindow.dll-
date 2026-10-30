#pragma once
// NCLobbyWnd - the character selection screen (UClass "CLobbyWnd", 0xE0; NConsoleWnd +0x36CC):
// the character panel (NCPawnSelectWnd), the characters' names floating over the stage, a
// yes / no box, and two windows of this client at the bottom and the right (C3: the "Start"
// button and the "Create", "Delete", "Previous" buttons are not made here any more). At most
// seven characters; Delete and Start wait for a choice.

#include "nwindow/NCWnd.h"
#include "nwindow/NCGaraDialogBox.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCPawnSelectWnd;
class NCButton;

// NCLobbyStartWnd - C3: the "Start" button in a window of its own at the bottom (UClass "CLobbyStartWnd", 0xD0). The
// button starts disabled; NCLobbyWnd enables it once a character is chosen. Its parent is the NCLobbyWnd.
class NCLobbyStartWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCLobbyStartWnd, NCWnd, 0, 0x10284AC8)

    NCButton* StartButton;           // +0xC8 id 0x1009, sysstring 160 "Start" (disabled until a choice)
    UTexture* BackTex;               // +0xCC "L2UI_CH3.LobbyWnd.lobby_startbtnback" (140 x 49)

    NCLobbyStartWnd();                                               // 0x1008f2a0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1008eed0
    virtual INT OnCreate();                                          // 37 0x1008f150
    virtual INT OnDestroy();                                         // 38 0x1008ef30
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1008efd0
    // the guard's name; the lobby's OnStartClick (slot 70) is called
    virtual INT OnGameStartBtnClick();                               // 68 0x1008f0c0
};
NW_ASSERT_OFFSET(NCLobbyStartWnd, StartButton, 0xC8);
NW_ASSERT_OFFSET(NCLobbyStartWnd, BackTex, 0xCC);
NW_ASSERT_SIZE(NCLobbyStartWnd, 0xD0);

// NCLobbyFunctionWnd - C3: the "Create", "Delete" and "Previous" buttons in a window of their own in the lower
// right corner (UClass "CLobbyFunctionWnd", 0xD8). "Delete" starts disabled. Its parent is the NCLobbyWnd; the
// virtuals 68 .. 70 call the lobby's 71 .. 73.
class NCLobbyFunctionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCLobbyFunctionWnd, NCWnd, 0, 0x102845A0)

    NCButton* PreviousButton;        // +0xC8 id 0x100A, sysstring 884
    NCButton* CreateButton;          // +0xCC id 0x100B, sysstring 158 "Create"
    NCButton* DeleteButton;          // +0xD0 id 0x100C, sysstring 159 "Delete" (disabled until a choice)
    UTexture* BackTex;               // +0xD4 "L2UI_CH3.LobbyWnd.lobby_menuback" (120 x 138)

    NCLobbyFunctionWnd();                                            // 0x1008ee20

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1008e6d0
    virtual INT OnCreate();                                          // 37 0x1008eb70
    virtual INT OnDestroy();                                         // 38 0x1008e770
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1008e870
    // the guards' names (copied from the login window's function window)
    virtual INT OnGameCancelBtnClick();                              // 68 0x1008e9c0 the lobby's OnPreviousClick
    virtual INT OnNewCharacterBtnClick();                            // 69 0x1008ea50 OnCreateCharacter
    virtual INT OnDelCharacterBtnClick();                            // 70 0x1008eae0 OnDelete
};
NW_ASSERT_OFFSET(NCLobbyFunctionWnd, PreviousButton, 0xC8);
NW_ASSERT_OFFSET(NCLobbyFunctionWnd, BackTex, 0xD4);
NW_ASSERT_SIZE(NCLobbyFunctionWnd, 0xD8);

class NCLobbyWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCLobbyWnd, NCWnd, 0, 0x10285518)

    NCPawnSelectWnd* PawnSelectWnd;  // +0xC8
    NCGaraDialogBox* DialogBox;      // +0xCC deletion / restoration questions (class 0x10265748, ctor 0x10009F80)
    NCLobbyStartWnd* StartWnd;       // +0xD0 "Start" (OnCreate gives it the bottom, anchor 8)
    NCLobbyFunctionWnd* FunctionWnd; // +0xD4 "Create", "Delete", "Previous" (OnCreate gives it the lower right
                                     //   corner, anchor 9)
    UTexture* ChatTex;               // +0xD8 "NWindow.Chat" (the constructor looks it up; nothing in the class reads it)
    INT NumCharacters;               // +0xDC

    NCLobbyWnd();                                                    // 0x10090d60
    // ~NCLobbyWnd                                                   // 0x10081e70 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1008f600
    virtual INT OnCreate();                                          // 37 0x10090e40
    virtual INT OnDestroy();                                         // 38 0x10090910
    virtual void ShowWindow();                                       // 44 0x1008f900
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100906a0
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 59 0x1008f990 (Enter starts)
    // names ours
    virtual INT OnCommand10001() { return 1; }                       // 68 0x10005fa0 (folded)
    virtual INT OnDialogResult(INT bYes, INT Unused);                // 69 0x1008f840 (NWM_DIALOG_RESULT, from WindowProc)
    virtual INT OnStartClick();                                      // 70 0x1008f770 (C3: called by NCLobbyStartWnd)
    virtual INT OnPreviousClick();                                   // 71 0x1008f7b0 (C3: called by NCLobbyFunctionWnd)
    virtual INT OnCreateCharacter();                                 // 72 0x1008f7f0
    virtual INT OnDelete();                                          // 73 0x100909e0

    // names ours
    void OnStart();                                                  // 0x1008f6a0 (not virtual in C3)
    INT AddCharacter(User Info);                                     // 0x10090650
    void ClearCharacters();                                          // 0x1008f680
};
NW_ASSERT_OFFSET(NCLobbyWnd, PawnSelectWnd, 0xC8);
NW_ASSERT_OFFSET(NCLobbyWnd, DialogBox, 0xCC);
NW_ASSERT_OFFSET(NCLobbyWnd, StartWnd, 0xD0);
NW_ASSERT_OFFSET(NCLobbyWnd, FunctionWnd, 0xD4);
NW_ASSERT_OFFSET(NCLobbyWnd, ChatTex, 0xD8);
NW_ASSERT_OFFSET(NCLobbyWnd, NumCharacters, 0xDC);
NW_ASSERT_SIZE(NCLobbyWnd, 0xE0);

#pragma pack(pop)
