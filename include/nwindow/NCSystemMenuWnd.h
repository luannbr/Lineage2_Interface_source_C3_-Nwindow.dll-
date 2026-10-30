#pragma once
// NCSystemMenuWnd - the system menu (UClass "CSystemMenuWnd", 0xB8; Alt+X): Help, Options,
// Restart and Exit Game, and two more buttons in C2 (one column of six). Restart and Exit ask first (dialogs 0x1B / 0x1C); the dialog box
// answers with NWM_DIALOG_RESULT and the dialog id, and the menu sends the request.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

// Buttons[] (names ours); the fifth slot is never created
enum
{
    SYSMENU_Help,
    SYSMENU_Options,
    SYSMENU_Restart,
    SYSMENU_Exit,
    SYSMENU_Unused,
    SYSMENU_MAX
};

// NWM_COMMAND ids of the buttons and title bar, and the dialogs the menu opens
enum
{
    NID_SYSMENU_HELP = 0xF0F006,
    NID_SYSMENU_OPTIONS = 0xF0F007,
    NID_SYSMENU_RESTART = 0xF0F008,
    NID_SYSMENU_EXIT = 0xF0F009,
    NID_SYSMENU_FRAME = 0xF0F011,
    NID_SYSMENU_BUTTON_A = 0xF0F022,       // (new in C2, names ours) text sysstring 711, above Help
    NID_SYSMENU_BUTTON_B = 0xF0F023,       // (new in C2) text id 470, under Help
    NID_SYSMENU_BUTTON_C = 0xF0F024,       // (new in C3, name ours) the first icon: the board / community window
    DIALOG_SYSMENU_RESTART = 0x1B,
    DIALOG_SYSMENU_EXIT = 0x1C,
};

class NCSystemMenuWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSystemMenuWnd, NCWnd, 0, 0x1028FEF0)

    UTexture* BackTex;                     // +0xC8 "L2UI.etcWnd.systemBack"   // C2: offset derived, no evidence
    NCButton* Buttons[SYSMENU_MAX];        // +0xCC SYSMENU_*
    NCButton* ButtonA;                     // +0xE0 (new in C2, name ours) NID_SYSMENU_BUTTON_A, OnHelpWindow
    NCButton* ButtonB;                     // +0xE4 (new in C2, name ours) NID_SYSMENU_BUTTON_B, OnHelpPage
    NCButton* ButtonC;                     // +0xE8 (new in C3, name ours) NID_SYSMENU_BUTTON_C, OnBoard (vslot 74)

    NCSystemMenuWnd();                                               // 0x100ab860
    // ~NCSystemMenuWnd                                              // 0x10097800 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a9c00
    virtual INT OnCreate();                                          // 37 0x100aae90
    virtual INT OnDestroy();                                         // 38 0x100aa540 (folded)
    virtual void ShowWindow();                                       // 44 0x100aa860 (folded)
    virtual void Minimize();                                         // 49 0x100a9eb0 (the db says MinimizeWindow)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100aa310
    // names ours
    virtual INT OnHelp();                                            // 68 0x100a9d40 (C2: not the C1 body, see below)
    virtual INT OnOptions();                                         // 69 0x100a9d60 (folded)
    virtual INT OnRestart();                                         // 70 0x100aa720
    virtual INT OnHelpWindow();                                      // 71 0x100a9d80 (the C1 OnHelp: NCConsole::ToggleHelp)
    virtual INT OnExit();                                            // 72 0x100aa7c0
    virtual INT OnHelpPage();                                        // 73 0x100a9f10 (new in C2)
    virtual void NewVirtual74();  // 74 0x100a9da0 (only this client has it; defined in NNewVirtuals.gen.cpp); its body is
                                  // OnBoard below (the name NewVirtual74 stays: the generated file defines it)
    virtual INT OnDialogResult(WPARAM DialogID, LPARAM Unused);      // 75 0x100a9dc0
    INT OnBoard();                                                   // 0x100a9da0 the body of vslot 74 (ToggleBoard)
};
NW_ASSERT_OFFSET(NCSystemMenuWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCSystemMenuWnd, Buttons, 0xCC);
NW_ASSERT_OFFSET(NCSystemMenuWnd, ButtonA, 0xE0);
NW_ASSERT_SIZE(NCSystemMenuWnd, 0xEC);

#pragma pack(pop)