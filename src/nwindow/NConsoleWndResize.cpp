/*=============================================================================
    NConsoleWndResize.cpp - the screen changed size: every window gets the place it has at the new size
    (NConsoleWnd::ResizeWindows, virtual 339 in C3, called by the viewport's ResizeViewport in WinDrv after it
    stored GSEKScreenX / GSEKScreenY).
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCWnd.h"
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCIcon.h"

// NConsoleWnd::ScaleRect: C2 only (no C3 function: 0x10056500 is the middle of NCConsole::OnTimer), nobody calls it
void NConsoleWnd::ScaleRect(INT W0, INT H0, INT W1, INT H1, NResizeRect* Rect)
{
}

// a minimized window back to its size (the stock repeats it inline)
static FORCEINLINE void RestoreIfMinimized(NCWnd* Wnd)
{
    if (Wnd->Icon->IsShowWindow())
        Wnd->Restore();
}

// 0x10069150 (virtual 339, ret 0) - a size change (either side not zero): the minimized windows of the desktop
// restored, its local rectangle read (nothing keeps it), then the screen-sized top windows sized to the screen (the
// login, lobby and character creation ones 52 pixels less in height)
void NConsoleWnd::ResizeWindows()
{
    NCConsole* Desk = GetDesktop();
    if (Desk && (GSEKScreenX != 0 || GSEKScreenY != 0))
    {
        RestoreIfMinimized(::nw::Field<NCWnd*>(Desk, 0xC8));      // MainWnd
        RestoreIfMinimized(::nw::Field<NCWnd*>(Desk, 0x140));     // MatchWnd
        RestoreIfMinimized(::nw::Field<NCWnd*>(Desk, 0x148));     // PartyRoomWnd
        RestoreIfMinimized(::nw::Field<NCWnd*>(Desk, 0x124));     // SystemMenuWnd
        RestoreIfMinimized(::nw::Field<NCWnd*>(Desk, 0x150));     // BoardWnd
        RestoreIfMinimized(::nw::Field<NCWnd*>(Desk, 0x14C));     // MapCoverWnd
        RestoreIfMinimized(::nw::Field<NCWnd*>(Desk, 0x15C));     // PetWnd
        RestoreIfMinimized(::nw::Field<NCWnd*>(Desk, 0x160));     // SummonedWnd
        RestoreIfMinimized(::nw::Field<NCWnd*>(Desk, 0xFC));      // InventoryWnd
        NRectF Local;
        Local.X = 0;
        Local.Y = 0;
        Local.W = 0.0f;
        Local.H = 0.0f;
        Desk->GetLocalRect(&Local);
        NCWnd* Top;
        Top = ::nw::Field<NCWnd*>(this, 0x3950);     // GodWnd
        if (Top)
            Top->SetWindowSize((FLOAT)GSEKScreenX, (FLOAT)GSEKScreenY);
        Top = ::nw::Field<NCWnd*>(this, 0x3954);     // LoginWnd
        if (Top)
            Top->SetWindowSize((FLOAT)GSEKScreenX, (FLOAT)(GSEKScreenY - 0x34));
        Top = ::nw::Field<NCWnd*>(this, 0x395C);     // LobbyWnd
        if (Top)
            Top->SetWindowSize((FLOAT)GSEKScreenX, (FLOAT)(GSEKScreenY - 0x34));
        Top = ::nw::Field<NCWnd*>(this, 0x3960);     // PawnCreateWnd
        if (Top)
            Top->SetWindowSize((FLOAT)GSEKScreenX, (FLOAT)(GSEKScreenY - 0x34));
        Top = ::nw::Field<NCWnd*>(this, 0x3964);     // LoadingWnd
        if (Top)
            Top->SetWindowSize((FLOAT)GSEKScreenX, (FLOAT)GSEKScreenY);
        Top = ::nw::Field<NCWnd*>(this, 0x3958);     // CreditWnd
        if (Top)
            Top->SetWindowSize((FLOAT)GSEKScreenX, (FLOAT)GSEKScreenY);
        Top = ::nw::Field<NCWnd*>(this, 0x3968);     // Desktop
        if (Top)
            Top->SetWindowSize((FLOAT)GSEKScreenX, (FLOAT)GSEKScreenY);
        Top = ::nw::Field<NCWnd*>(this, 0x39E0);     // (new in C3: another screen-sized top window)
        if (Top)
            Top->SetWindowSize((FLOAT)GSEKScreenX, (FLOAT)GSEKScreenY);
    }
}
NW_HOOK_METHOD(0x10069150, NConsoleWnd, ResizeWindows)
