/*=============================================================================
    NCMenuWnd.cpp - the main menu bar. Stock: 0x10076db0 .. 0x10076ef5, 0x10077ab0 ..
    0x10077f67, plus OnDestroy 0x100367a0 (the body NCActionWnd and NCServerSelectWnd share).
=============================================================================*/
#include "nwindow/NCMenuWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x100ab920
NCMenuWnd::NCMenuWnd()
{
    BackTex = NULL;
    SystemButton = NULL;
    InventoryButton = NULL;
    CharacterButton = NULL;
    CommunityButton = NULL;
}
NW_HOOK_CTOR(0x100ab920, NCMenuWnd)

// 0x100ab990
NW_HOOK_DTOR(0x100ab990, NCMenuWnd)

// 0x100a9f90 (vslot 25) - C3: the Community button's handler is virtual slot 71 (NewVirtual71 here, whose body is
// OnCommunity: it always returns 1)
INT NCMenuWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_MENU_INVENTORY)
            return OnInventory() != 0;
        if (wParam == NID_MENU_CHARACTER)
            return OnCharacter() != 0;
        if (wParam == NID_MENU_SYSTEM)
            return OnSystem() != 0;
        if (wParam == NID_MENU_COMMUNITY)
        {
            NewVirtual71();
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x100a9f90, NCMenuWnd, WindowProc)

// one icon of the bar (repeated inline in the stock OnCreate)
static FORCEINLINE NCButton* CreateMenuIcon(NCWnd* Parent, const TCHAR* NormalTexName,
                                const TCHAR* PushedTexName, INT ID, INT X, INT ToolTipID)
{
    NCButton* Button = new NCButton(NormalTexName, PushedTexName, 0);
    Button->ID = ID;
    Button->SetPosition(X, 6, 34, 34);   // C3: y 6 (C2: 5)
    Parent->CreateChild(Button, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    Button->SetToolTip(ToolTipID, 1);
    return Button;
}

// 0x100ab3c0 (vslot 37) - NCWnd::OnCreate is not called. C3: three textures of the small window skin, a bar of 12 x 46
// (the frame control takes three arguments now: the stock constructor 0x10010450 is called) and four 34 x 34 icons at
// y 6, each with its tool tip; after each one the stock clears a field of the CHARACTER button (+0x298: the source
// names the first button every time)
INT NCMenuWnd::OnCreate()
{
    BackTex = FindTexture(TEXT("L2UI_ch3.SmallWnd.Smallwindow_back2"));
    BackLeftTex = FindTexture(TEXT("L2UI_ch3.SmallWnd.Smallwindow_back1"));
    BackRightTex = FindTexture(TEXT("L2UI_ch3.SmallWnd.Smallwindow_back3"));
    FrameCtrl = new NCFrameCtrl(1, 1, 1);
    FrameID = NID_MENU_FRAME;
    FrameCtrl->ID = NID_MENU_FRAME;
    FrameCtrl->SetPosition(0, 0, 12, 46);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    CharacterButton = CreateMenuIcon(this, TEXT("L2UI_ch3.MenuIcon.MenuButton1"),
                                     TEXT("L2UI_ch3.MenuIcon.MenuButton1_down"), NID_MENU_CHARACTER, 19, 0x1B2);
    ::nw::Field<INT>(CharacterButton, 0x298) = 0;
    InventoryButton = CreateMenuIcon(this, TEXT("L2UI_ch3.MenuIcon.MenuButton2"),
                                     TEXT("L2UI_ch3.MenuIcon.MenuButton2_down"), NID_MENU_INVENTORY, 56, 0xC3);
    ::nw::Field<INT>(CharacterButton, 0x298) = 0;
    CommunityButton = CreateMenuIcon(this, TEXT("L2UI_ch3.MenuIcon.MenuButton3"),
                                     TEXT("L2UI_ch3.MenuIcon.MenuButton3_down"), NID_MENU_COMMUNITY, 93, 0x39D);
    ::nw::Field<INT>(CharacterButton, 0x298) = 0;
    SystemButton = CreateMenuIcon(this, TEXT("L2UI_ch3.MenuIcon.MenuButton4"),
                                  TEXT("L2UI_ch3.MenuIcon.MenuButton4_down"), NID_MENU_SYSTEM, 130, 0xC1);
    ::nw::Field<INT>(CharacterButton, 0x298) = 0;
    return 1;
}
NW_HOOK_METHOD(0x100ab3c0, NCMenuWnd, OnCreate)

// 0x100aa9c0 (vslot 38) - C2: its own copy of the body, with a guard (C1 shared one with NCActionWnd and
// NCServerSelectWnd); the four buttons go in the order of their fields
INT NCMenuWnd::OnDestroy()
{
    guard(NCMenuWnd::OnDestroy);
    if (SystemButton)
    {
        RemoveChildWindow(SystemButton->Entry);
        delete SystemButton;
        SystemButton = NULL;
    }
    if (InventoryButton)
    {
        RemoveChildWindow(InventoryButton->Entry);
        delete InventoryButton;
        InventoryButton = NULL;
    }
    if (CharacterButton)
    {
        RemoveChildWindow(CharacterButton->Entry);
        delete CharacterButton;
        CharacterButton = NULL;
    }
    if (CommunityButton)
    {
        RemoveChildWindow(CommunityButton->Entry);
        delete CommunityButton;
        CommunityButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100aa9c0, NCMenuWnd, OnDestroy)

// 0x100aa8c0 (vslot 56) - C3: the bar is three pieces of 46 px: the left end at 12, the middle stretched from 28 to
// Width - 16 (the width less 12 and 32) and the right end at Width - 16
INT NCMenuWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(12, 0, 16, 46, 0, 0, 16, 46, BackLeftTex);
    Device->DrawTexture(28, 0, (INT)(Width - 12.0f - 32.0f), 46, 0, 0, 16, 46, BackTex);
    Device->DrawTexture((INT)(Width - 16.0f), 0, 16, 46, 0, 0, 16, 46, BackRightTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100aa8c0, NCMenuWnd, OnPaint)

// 0x100aa040 (vslot 68)
INT NCMenuWnd::OnSystem()
{
    GDesktop->ToggleSystemMenu();
    return 1;
}
NW_HOOK_METHOD(0x100aa040, NCMenuWnd, OnSystem)

// 0x100aa060 (vslot 69)
INT NCMenuWnd::OnCharacter()
{
    GDesktop->ToggleCharacter(-1);
    return 1;
}
NW_HOOK_METHOD(0x100aa060, NCMenuWnd, OnCharacter)

// 0x100aa080 (vslot 70)
INT NCMenuWnd::OnInventory()
{
    GDesktop->ToggleInventory();
    return 1;
}
NW_HOOK_METHOD(0x100aa080, NCMenuWnd, OnInventory)

// 0x100aa0a0 (vslot 71, NewVirtual71) - the Community button: a request to the server with an empty parameter stack
// of 10 (NConsoleWnd 0x1005d020: the header calls it RequestDeleteMacro, its name is a guess)
INT NCMenuWnd::OnCommunity()
{
    L2ParamStack Params(10);
    GConsole->RequestDeleteMacro(Params);
    return 1;
}
NW_HOOK_METHOD(0x100aa0a0, NCMenuWnd, OnCommunity)

NW_CHECK_VTABLE(NCMenuWnd, 0x10190340, 72)
