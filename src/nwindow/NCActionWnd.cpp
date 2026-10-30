/*=============================================================================
    NCActionWnd.cpp - the action window (Basic and Clan tabs). Stock: 0x100364c0 ..
    0x1003679e, 0x10036870 .. 0x1003687a, 0x10036e20 .. 0x10036e61, 0x10037250 ..
    0x1003735b, 0x1005f200 .. 0x1005f24f (the C1 addresses; C2: OnDestroy is 0x10042df0).
=============================================================================*/
#include "nwindow/NCActionWnd.h"
#include "nwindow/NCActionItemListWnd.h"
#include "nwindow/NCActionItemWnd.h"
#include "nwindow/NCPledgeWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x1004aed0 - only the background texture is cleared (the page pointers stay as the allocation left them)
NCActionWnd::NCActionWnd()
{
    BackTex = NULL;
}
NW_HOOK_CTOR(0x1004aed0, NCActionWnd)

// 0x1004af20
NW_HOOK_DTOR(0x1004af20, NCActionWnd)

// stock body shared with NControl's (0x10004e90)
INT NCActionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x1004af70 (vslot 37) - C3: the Basic page alone (no tabs and no clan page), as large as the window, on a new
// background texture
INT NCActionWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.ActionWnd.Action_Back"));
    ActionList = new NCActionItemListWnd;
    ActionList->SetPosition(0, 0, (INT)Width, (INT)Height);
    CreateChild(ActionList, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x1004af70, NCActionWnd, OnCreate)

// 0x10049170 (vslot 38) - the page goes (and the clan page, which C3 never creates: the pointer stays NULL)
INT NCActionWnd::OnDestroy()
{
    guard(NCActionWnd::OnDestroy);
    if (ActionList)
    {
        RemoveChildWindow(ActionList->Entry);
        delete ActionList;
        ActionList = NULL;
    }
    if (PledgeWnd)
    {
        RemoveChildWindow(PledgeWnd->Entry);
        delete PledgeWnd;
        PledgeWnd = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10049170, NCActionWnd, OnDestroy)

// 0x1004b1b0 (vslot 44) - the page's icons are reloaded (so that the dwarves' and the pets' actions follow the state
// of the game)
void NCActionWnd::ShowWindow()
{
    ResetPledgeWnd();
    NCWnd::ShowWindow();
    ActionList->LoadActions();
}
NW_HOOK_METHOD(0x1004b1b0, NCActionWnd, ShowWindow)

// 0x100490d0 (vslot 56) - the background over the whole window
INT NCActionWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, (INT)Width, (INT)Height, BackTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100490d0, NCActionWnd, OnPaint)

// 0x1004b030 - the page's grids emptied (the name is the C2 one: C3 has no clan page to reset)
void NCActionWnd::ResetPledgeWnd()
{
    ActionList->ClearGrids();
}
NW_HOOK_METHOD(0x1004b030, NCActionWnd, ResetPledgeWnd)

// 0x1004b040 - after a language change (NConsoleWnd::ReloadGameTexts)
void NCActionWnd::ReloadActions()
{
    ActionList->BasicGrid->ClearItems();
    ActionList->PartyGrid->ClearItems();
    ActionList->SocialGrid->ClearItems();
    ActionList->LoadActions();
}
NW_HOOK_METHOD(0x1004b040, NCActionWnd, ReloadActions)

NW_CHECK_VTABLE(NCActionWnd, 0x10181ac8, 68)
