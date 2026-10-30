#pragma once
// NCMenuWnd - the main menu bar (UClass "CMenuWnd", 0xA8): four big icons, each opening or
// closing one of the desktop's windows (Character Status Alt+I, Inventory Alt+V, Community
// Alt+B, System Alt+X), and a thin title bar on the left to drag it.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

// NWM_COMMAND ids of the buttons and the title bar
enum
{
    NID_MENU_INVENTORY = 0xF0F000,
    NID_MENU_CHARACTER = 0xF0F001,
    NID_MENU_SYSTEM = 0xF0F003,
    NID_MENU_COMMUNITY = 0xF0F006,
    NID_MENU_FRAME = 0xF0F021,
};

class NCMenuWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMenuWnd, NCWnd, 0, 0x1028F4A8)

    UTexture* BackTex;                     // +0xC8 "L2UI_ch3.SmallWnd.Smallwindow_back2" (C3): the middle, stretched
    UTexture* BackLeftTex;                 // +0xCC "L2UI_ch3.SmallWnd.Smallwindow_back1": the left end, at (12, 0) 16 x 46
    UTexture* BackRightTex;                // +0xD0 "L2UI_ch3.SmallWnd.Smallwindow_back3": the right end, at Width - 16
    BYTE _UnknownD4[0xD8 - 0xD4]; // +0xD4 (nothing touches it)
    NCButton* SystemButton;                // +0xD8
    NCButton* InventoryButton;             // +0xDC
    NCButton* CharacterButton;             // +0xE0
    NCButton* CommunityButton;             // +0xE4

    NCMenuWnd();                                                     // 0x100ab920
    // ~NCMenuWnd                                                    // 0x10098130 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a9f90
    virtual INT OnCreate();                                          // 37 0x100ab3c0
    virtual INT OnDestroy();                                         // 38 0x100aa9c0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100aa8c0
    // names ours
    virtual INT OnSystem();                                          // 68 0x100aa040
    virtual INT OnCharacter();                                       // 69 0x100aa060
    virtual INT OnInventory();                                       // 70 0x100aa080
    // 71 0x100aa0a0 is the Community button's handler (WindowProc calls it with vcall +0x11c); it keeps the
    // NewVirtual71 name because NNewVirtuals.gen.cpp (generated) defines it; the body is OnCommunity below
    virtual void NewVirtual71();  // 71 0x100aa0a0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    INT OnCommunity();                                       // 0x100aa0a0: the body of vslot 71 (non-virtual here, see above)
};
NW_ASSERT_OFFSET(NCMenuWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCMenuWnd, CommunityButton, 0xE4);
NW_ASSERT_SIZE(NCMenuWnd, 0xE8);

#pragma pack(pop)