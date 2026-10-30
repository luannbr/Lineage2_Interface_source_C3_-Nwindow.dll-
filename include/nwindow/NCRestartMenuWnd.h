#pragma once
// NCRestartMenuWnd - the menu that opens when the character dies (UClass "CRestartMenuWnd",
// 0xC0): a button per place to restart at (nearest village, hideaway, castle, siege
// headquarters, fixed). The server says which of them apply (SetRestartPoints); the others
// hide when the menu shows. A click sends the choice (RequestRestartPoint) and closes it.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

// Buttons[] and what the server gets for each (names ours)
enum
{
    RESTART_Village,
    RESTART_Hideaway,
    RESTART_Castle,
    RESTART_SiegeHQ,
    RESTART_Fixed,
    RESTART_MAX
};

// NWM_COMMAND ids of the buttons
enum
{
    NID_RESTART_VILLAGE = 0xF0F012,
    NID_RESTART_HIDEAWAY = 0xF0F013,
    NID_RESTART_CASTLE = 0xF0F014,
    NID_RESTART_SIEGEHQ = 0xF0F015,
    NID_RESTART_FIXED = 0xF0F020,
};

class NCRestartMenuWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCRestartMenuWnd, NCWnd, 0, 0x1028F9D0)

    UTexture* BackTex;                     // +0xC8 "L2UI.EtcWnd.RestartBack"
    NCButton* Buttons[RESTART_MAX];        // +0xCC RESTART_*
    UBOOL bAvailable[RESTART_MAX];         // +0xE0 (1) from the server (SetRestartPoints)

    NCRestartMenuWnd();                                              // 0x100ab780
    // ~NCRestartMenuWnd                                             // 0x100971b0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a97e0
    virtual INT OnCreate();                                          // 37 0x100aaaf0
    virtual INT OnDestroy();                                         // 38 0x100aa1b0
    virtual void ShowWindow();                                       // 44 0x100aa300
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100aa110
    // names ours
    virtual INT OnVillage();                                         // 68 0x100a9980
    virtual INT OnHideaway();                                        // 69 0x100a9a00
    virtual INT OnCastle();                                          // 70 0x100a9a80
    virtual INT OnSiegeHQ();                                         // 71 0x100a9b00
    virtual INT OnFixed();                                           // 72 0x100a9b80

    // names ours
    void HideUnavailable();                                          // 0x100a98c0
    void SetRestartPoints(UBOOL bVillage, UBOOL bHideaway, UBOOL bCastle, UBOOL bSiegeHQ,
                          UBOOL bFixed);                             // 0x10076930 (the console,
                                                                     //   when the player dies)
};
NW_ASSERT_OFFSET(NCRestartMenuWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCRestartMenuWnd, Buttons, 0xCC);
NW_ASSERT_OFFSET(NCRestartMenuWnd, bAvailable, 0xE0);
NW_ASSERT_SIZE(NCRestartMenuWnd, 0xF4);

#pragma pack(pop)