/*=============================================================================
    NCRestartMenuWnd.cpp - the restart menu of a dead character. Stock: 0x100a97e0 ..
    0x100a9b9f, 0x100aa110 .. 0x100aa30a, 0x100aaaf0 .. 0x100ab888 (the system menu shares
    OnDestroy).
=============================================================================*/
#include "nwindow/NCRestartMenuWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x100ab780 - C3: the textures and the buttons empty (the fifth button's slot is left to OnCreate), every restart
// point available
NCRestartMenuWnd::NCRestartMenuWnd()
{
    BackTex = NULL;
    Buttons[RESTART_Village] = NULL;
    Buttons[RESTART_Hideaway] = NULL;
    Buttons[RESTART_Castle] = NULL;
    Buttons[RESTART_SiegeHQ] = NULL;
    bAvailable[RESTART_Village] = 1;
    bAvailable[RESTART_Hideaway] = 1;
    bAvailable[RESTART_Castle] = 1;
    bAvailable[RESTART_SiegeHQ] = 1;
    bAvailable[RESTART_Fixed] = 1;
}
NW_HOOK_CTOR(0x100ab780, NCRestartMenuWnd)

// 0x100ab810
NW_HOOK_DTOR(0x100ab810, NCRestartMenuWnd)

// 0x100a97e0 (vslot 25) - C2 no longer refuses the messages of a disabled window here
INT NCRestartMenuWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_RESTART_VILLAGE)
            return OnVillage() != 0;
        if (wParam == NID_RESTART_HIDEAWAY)
            return OnHideaway() != 0;
        if (wParam == NID_RESTART_CASTLE)
            return OnCastle() != 0;
        if (wParam == NID_RESTART_SIEGEHQ)
            return OnSiegeHQ() != 0;
        if (wParam == NID_RESTART_FIXED)
            return OnFixed() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100a97e0, NCRestartMenuWnd, WindowProc)

// one button of the menu (repeated inline in the stock OnCreate): the field gets the button before anything else
// is done to it; 96 x 23 at x 16, the CH3 "bigbutton" pair
#define RESTART_BUTTON(Index, WndID, Y, TextID)                                                       \
    Buttons[Index] = new NCButton(TEXT("L2UI_CH3.button.bigbutton"),                                  \
                                  TEXT("L2UI_CH3.button.bigbutton_down"), 0);                         \
    Buttons[Index]->ID = WndID;                                                                       \
    Buttons[Index]->SetPosition(0x10, Y, 0x60, 0x17);                                                 \
    CreateChild(Buttons[Index], NWS_VISIBLE);                                                         \
    Buttons[Index]->SetTextID(TextID);

// 0x100aaaf0 (vslot 37) - C3: the CH3 skin; the buttons 24 px apart from y 6, their texts are not in the order of the
// ids (0x174, 0x176, 0x177, 0x175 and 0x1D1 for the fixed point)
INT NCRestartMenuWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.SystemMenuWnd.system2_back"));
    RESTART_BUTTON(RESTART_Village, NID_RESTART_VILLAGE, 6, 0x174)
    RESTART_BUTTON(RESTART_Hideaway, NID_RESTART_HIDEAWAY, 0x1E, 0x176)
    RESTART_BUTTON(RESTART_Castle, NID_RESTART_CASTLE, 0x36, 0x177)
    RESTART_BUTTON(RESTART_SiegeHQ, NID_RESTART_SIEGEHQ, 0x4E, 0x175)
    RESTART_BUTTON(RESTART_Fixed, NID_RESTART_FIXED, 0x66, 0x1D1)
    return 1;
}
NW_HOOK_METHOD(0x100aaaf0, NCRestartMenuWnd, OnCreate)
#undef RESTART_BUTTON

// 0x100aa1b0 (vslot 38) - guarded; the buttons go, then NCWnd's OnDestroy
INT NCRestartMenuWnd::OnDestroy()
{
    guard(NCRestartMenuWnd::OnDestroy);
    for (INT i = 0; i < RESTART_MAX; i++)
    {
        if (Buttons[i])
        {
            RemoveChildWindow(Buttons[i]->Entry);
            if (Buttons[i])
                delete Buttons[i];
            Buttons[i] = NULL;
        }
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100aa1b0, NCRestartMenuWnd, OnDestroy)

// 0x100aa300 (vslot 44)
void NCRestartMenuWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    HideUnavailable();
}
NW_HOOK_METHOD(0x100aa300, NCRestartMenuWnd, ShowWindow)

// 0x100aa110 (vslot 56) - C3: the back is 141 x 131 (the window's size comes from the texture)
INT NCRestartMenuWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, 0x8D, 0x83, 0, 0, 0x8D, 0x83, BackTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100aa110, NCRestartMenuWnd, OnPaint)

// the choice goes to the server and the menu closes (repeated in each button's slot)
static FORCEINLINE void SendRestartPoint(NCWnd* Menu, INT Point)
{
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)Point);
    GConsole->RequestRestartPoint(Params);
    Menu->HideWindow();
}

// 0x100a9980 (vslot 68)
INT NCRestartMenuWnd::OnVillage()
{
    SendRestartPoint(this, RESTART_Village);
    return 1;
}
NW_HOOK_METHOD(0x100a9980, NCRestartMenuWnd, OnVillage)

// 0x100a9a00 (vslot 69)
INT NCRestartMenuWnd::OnHideaway()
{
    SendRestartPoint(this, RESTART_Hideaway);
    return 1;
}
NW_HOOK_METHOD(0x100a9a00, NCRestartMenuWnd, OnHideaway)

// 0x100a9a80 (vslot 70)
INT NCRestartMenuWnd::OnCastle()
{
    SendRestartPoint(this, RESTART_Castle);
    return 1;
}
NW_HOOK_METHOD(0x100a9a80, NCRestartMenuWnd, OnCastle)

// 0x100a9b00 (vslot 71)
INT NCRestartMenuWnd::OnSiegeHQ()
{
    SendRestartPoint(this, RESTART_SiegeHQ);
    return 1;
}
NW_HOOK_METHOD(0x100a9b00, NCRestartMenuWnd, OnSiegeHQ)

// 0x100a9b80 (vslot 72)
INT NCRestartMenuWnd::OnFixed()
{
    SendRestartPoint(this, RESTART_Fixed);
    return 1;
}
NW_HOOK_METHOD(0x100a9b80, NCRestartMenuWnd, OnFixed)

// 0x100a98c0 - the buttons of the places the server did not offer are hidden
void NCRestartMenuWnd::HideUnavailable()
{
    if (bAvailable[RESTART_Village] == 0)
        Buttons[RESTART_Village]->HideWindow();
    if (bAvailable[RESTART_Hideaway] == 0)
        Buttons[RESTART_Hideaway]->HideWindow();
    if (bAvailable[RESTART_Castle] == 0)
        Buttons[RESTART_Castle]->HideWindow();
    if (bAvailable[RESTART_SiegeHQ] == 0)
        Buttons[RESTART_SiegeHQ]->HideWindow();
    if (bAvailable[RESTART_Fixed] == 0)
        Buttons[RESTART_Fixed]->HideWindow();
}
NW_HOOK_METHOD(0x100a98c0, NCRestartMenuWnd, HideUnavailable)

// 0x100a9940
void NCRestartMenuWnd::SetRestartPoints(UBOOL bVillage, UBOOL bHideaway, UBOOL bCastle,
                                        UBOOL bSiegeHQ, UBOOL bFixed)
{
    bAvailable[RESTART_Village] = bVillage;
    bAvailable[RESTART_Hideaway] = bHideaway;
    bAvailable[RESTART_Castle] = bCastle;
    bAvailable[RESTART_SiegeHQ] = bSiegeHQ;
    bAvailable[RESTART_Fixed] = bFixed;
}
NW_HOOK_METHOD(0x100a9940, NCRestartMenuWnd, SetRestartPoints)

NW_CHECK_VTABLE(NCRestartMenuWnd, 0x101900e8, 73)
