/*=============================================================================
    NCBroadcastObserverPartyListWnd.cpp - one team's list of player rows (C3 only; C3 addresses). Stock: GetUser
    0x10114CC0, GetTeam 0x10114D60, SetSelectedUser 0x10114D70, IsSelected 0x10114E00, RefreshShowStatus 0x10114EA0,
    ShowWindow 0x10114F70, IsShowClassOrName 0x10114FF0, OnDestroy 0x10115080, OnCreate 0x10115140, the constructor
    0x10115260 and the destructor 0x101152F0.
=============================================================================*/
#include "nwindow/NCBroadcastObserverPartyListWnd.h"
#include "nwindow/NCBroadcastObserverPartyWnd.h"
#include "nwindow/NCBroadcastObserverWnd.h"
#include "nwindow/NCShopWnd.h"
#include "nwindow/NConsoleWnd.h"

// 0x10115260 (ret 8) - the nine rows cleared, the team and the owner kept
NCBroadcastObserverPartyListWnd::NCBroadcastObserverPartyListWnd(INT InTeam, NCBroadcastObserverWnd* InOwner)
{
    for (INT i = 0; i < 9; i++)
        Rows[i] = NULL;
    Owner = InOwner;
    Team = InTeam;
}
NW_HOOK_CTOR_ARGS(0x10115260, NCBroadcastObserverPartyListWnd, (INT InTeam, NCBroadcastObserverWnd* InOwner), (InTeam, InOwner))

// 0x101152f0
NW_HOOK_DTOR(0x101152f0, NCBroadcastObserverPartyListWnd)

// 0x10119f60 (vslot 25) - NControl's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCBroadcastObserverPartyListWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x10119f60)
#else
NW_SLOT_IS("?WindowProc@NCBroadcastObserverPartyListWnd@@UAEHIIJ@Z", "?WindowProc@NControl@@UAEHIIJ@Z")   // NCBroadcastObserverPartyListWnd::WindowProc = NControl::WindowProc (0x10119f60)
#endif

// 0x10114d60 (anonymous) - the team it lists
INT NCBroadcastObserverPartyListWnd::GetTeam()
{
    return Team;
}
NW_HOOK_METHOD(0x10114d60, NCBroadcastObserverPartyListWnd, GetTeam)

// 0x10114cc0 (guard: NCBroadcastObserverPartyListWnd::GetUser; ret 4) - the roster line of a row's slot, through the owner
NBroadcastUser* NCBroadcastObserverPartyListWnd::GetUser(INT Slot)
{
    guard(NCBroadcastObserverPartyListWnd::GetUser);
    if (!Owner)
        return NULL;
    return Owner->GetUser(Team, Slot);
    unguard;
}
NW_HOOK_METHOD(0x10114cc0, NCBroadcastObserverPartyListWnd, GetUser)

// 0x10114d70 (guard: NCBroadcastObserverPartyListWnd::SetSelectedUser; ret 4) - the owner follows the row's player
void NCBroadcastObserverPartyListWnd::SetSelectedUser(INT Slot)
{
    guard(NCBroadcastObserverPartyListWnd::SetSelectedUser);
    if (Owner)
        Owner->SetSelecteUser(Team, Slot);
    unguard;
}
NW_HOOK_METHOD(0x10114d70, NCBroadcastObserverPartyListWnd, SetSelectedUser)

// 0x10114e00 (guard: NCBroadcastObserverPartyListWnd::IsSelected; ret 4)
INT NCBroadcastObserverPartyListWnd::IsSelected(INT Slot)
{
    guard(NCBroadcastObserverPartyListWnd::IsSelected);
    if (!Owner)
        return 0;
    return Owner->IsSelected(Team, Slot);
    unguard;
}
NW_HOOK_METHOD(0x10114e00, NCBroadcastObserverPartyListWnd, IsSelected)

// 0x10114ea0 (guard: NCBroadcastObserverPartyListWnd::RefreshShowStatus) - the rows whose slot has a player are shown, the
// others hidden; the window is 70 pixels high per shown row and 280 wide
void NCBroadcastObserverPartyListWnd::RefreshShowStatus()
{
    guard(NCBroadcastObserverPartyListWnd::RefreshShowStatus);
    INT Shown = 0;
    for (INT i = 0; i < 9; i++)
    {
        if (Owner->GetUser(Team, i))
        {
            Rows[i]->ShowWindow();
            Shown++;
        }
        else
            Rows[i]->HideWindow();
    }
    SetWindowSize(280.0f, (FLOAT)(Shown * 70));
    unguard;
}
NW_HOOK_METHOD(0x10114ea0, NCBroadcastObserverPartyListWnd, RefreshShowStatus)

// 0x10114f70 (vslot 44, guard: NCBroadcastObserverPartyListWnd::ShowWindow) - NCWnd's, then the rows are set
void NCBroadcastObserverPartyListWnd::ShowWindow()
{
    guard(NCBroadcastObserverPartyListWnd::ShowWindow);
    NCWnd::ShowWindow();
    RefreshShowStatus();
    unguard;
}
NW_HOOK_METHOD(0x10114f70, NCBroadcastObserverPartyListWnd, ShowWindow)

// 0x10114ff0 (guard: NCBroadcastObserverPartyListWnd::IsShowClassOrName) - the owner's display mode of the rows (the
// stock calls the folded getter 0x101189A0 over the field at +0xF0)
INT NCBroadcastObserverPartyListWnd::IsShowClassOrName()
{
    guard(NCBroadcastObserverPartyListWnd::IsShowClassOrName);
    if (!Owner)
        return 0;
    return ((NCShopWnd*)(NCWnd*)Owner)->GetMode();   // the folded getter of the field at +0xF0 (PartyShowMode here)
    unguard;
}
NW_HOOK_METHOD(0x10114ff0, NCBroadcastObserverPartyListWnd, IsShowClassOrName)

// 0x10115080 (vslot 38, guard: NCBroadcastObserverPartyListWnd::OnDestroy) - the nine rows go, then NCWnd's
INT NCBroadcastObserverPartyListWnd::OnDestroy()
{
    guard(NCBroadcastObserverPartyListWnd::OnDestroy);
    for (INT i = 0; i < 9; i++)
    {
        if (Rows[i])
        {
            RemoveChildWindow(Rows[i]->Entry);
            delete Rows[i];
            Rows[i] = NULL;
        }
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10115080, NCBroadcastObserverPartyListWnd, OnDestroy)

// 0x10115140 (vslot 37, guard: NCBroadcastObserverPartyListWnd::OnCreate) - the nine rows (280 x 70), one under the other
INT NCBroadcastObserverPartyListWnd::OnCreate()
{
    guard(NCBroadcastObserverPartyListWnd::OnCreate);
    NCWnd::OnCreate();
    for (INT i = 0; i < 9; i++)
    {
        Rows[i] = new NCBroadcastObserverPartyWnd(this, i);
        Rows[i]->SetPosition(0, i * 70, 280, 70);
        CreateChild(Rows[i], NWS_VISIBLE);
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10115140, NCBroadcastObserverPartyListWnd, OnCreate)

NW_CHECK_VTABLE(NCBroadcastObserverPartyListWnd, 0x101a4af8, 68)
