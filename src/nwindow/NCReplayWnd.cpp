/*=============================================================================
    NCReplayWnd.cpp - the replay screen (C3 only; C3 addresses). Stock: SetWindowSize 0x100E6240, ShowWindow
    0x100E62F0, OnDestroy 0x100E68F0, the constructor 0x100E7720, the destructor 0x100E7770 and OnCreate 0x100E7BF0.
=============================================================================*/
#include "nwindow/NCReplayWnd.h"
#include "nwindow/NCBenchMarkResultWnd.h"
#include "nwindow/NCGodWnd.h"
#include "nwindow/NCReplayMenuWnd.h"
#include "nwindow/NCReplayPlayWnd.h"
#include "nwindow/NConsoleWnd.h"

// A child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x100e7720 - the three screens come with OnCreate (the stock clears the first two)
NCReplayWnd::NCReplayWnd() : MenuWnd(NULL), PlayWnd(NULL)
{
}
NW_HOOK_CTOR(0x100e7720, NCReplayWnd)

// 0x100e7770
NW_HOOK_DTOR(0x100e7770, NCReplayWnd)

// 0x10119f60 (vslot 25) - NControl's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCReplayWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x10119f60)
#else
NW_SLOT_IS("?WindowProc@NCReplayWnd@@UAEHIIJ@Z", "?WindowProc@NControl@@UAEHIIJ@Z")   // NCReplayWnd::WindowProc = NControl::WindowProc (0x10119f60)
#endif

// 0x10003360 (vslot 56) - a body shared by many classes: returns 1
#if NW_HOOK_MODE
NW_NAKED INT NCReplayWnd::OnPaint(NCGDevice* Device) NW_STOCK(0x10003360)
#else
NW_SLOT_IS("?OnPaint@NCReplayWnd@@UAEHPAVNCGDevice@@@Z", "?OnPaint@NCScrollWnd@@UAEHPAVNCGDevice@@@Z")   // NCReplayWnd::OnPaint = NCScrollWnd::OnPaint (0x10003360)
#endif

// 0x100e6240 (vslot 43, guard: NCReplayWnd::SetWindowSize; ret 8) - the three screens take the size
void NCReplayWnd::SetWindowSize(FLOAT W, FLOAT H)
{
    guard(NCReplayWnd::SetWindowSize);
    NCWnd::SetWindowSize(W, H);
    MenuWnd->SetWindowSize(W, H);
    PlayWnd->SetWindowSize(W, H);
    BenchWnd->SetWindowSize(W, H);
    unguard;
}
NW_HOOK_METHOD(0x100e6240, NCReplayWnd, SetWindowSize)

// 0x100e62f0 (vslot 44) - the screens that fit the replay manager's state, while a replay runs (GL2ReplayMode):
//   1: the menu, with the god window up and its message line, options and warning box hidden; the desktop hidden
//   5: the benchmark's result alone (the desktop hidden)
//   else: the playing screen; when the manager has its flag at +0x5074 the desktop shows with the chat window and
//         the status, menu and shortcut windows hidden
void NCReplayWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (GL2ReplayMode != 0)
    {
        FL2ReplayManager* Manager = GConsole->Engine->ReplayManager;
        if (Manager)
        {
            INT State = Manager->GetState();
            if (State == 1)
            {
                MenuWnd->ShowWindow();
                GConsole->GodWnd->ShowWindow();
                // the god window's message line, options window and warning box (NCGodWnd +0xD0 / +0xD4 / +0xD8)
                ::nw::Field<NCWnd*>(GConsole->GodWnd, 0xD0)->HideWindow();
                ::nw::Field<NCWnd*>(GConsole->GodWnd, 0xD4)->HideWindow();
                ::nw::Field<NCWnd*>(GConsole->GodWnd, 0xD8)->HideWindow();
                PlayWnd->HideWindow();
                BenchWnd->HideWindow();
                GConsole->Desktop->HideWindow();
                return;
            }
            if (State == 5)
            {
                MenuWnd->HideWindow();
                PlayWnd->HideWindow();
                BenchWnd->ShowWindow();
                GConsole->Desktop->HideWindow();
                return;
            }
            MenuWnd->HideWindow();
            GConsole->GodWnd->HideWindow();
            PlayWnd->ShowWindow();
            BenchWnd->HideWindow();
            if (::nw::Field<INT>(GConsole->Engine->ReplayManager, 0x5074) != 0)
            {
                // the desktop's windows (NCConsole +0xD4 ChatWnd, +0x100 TargetStatusWnd, +0x104 PlayerStatusWnd,
                // +0x120 MenuWnd, +0x138 ShortCutWnd)
                GConsole->Desktop->ShowWindow();
                ::nw::Field<NCWnd*>(GConsole->Desktop, 0xD4)->ShowWindow();
                ::nw::Field<NCWnd*>(GConsole->Desktop, 0x100)->HideWindow();
                ::nw::Field<NCWnd*>(GConsole->Desktop, 0x104)->HideWindow();
                ::nw::Field<NCWnd*>(GConsole->Desktop, 0x120)->HideWindow();
                ::nw::Field<NCWnd*>(GConsole->Desktop, 0x138)->HideWindow();
            }
        }
    }
}
NW_HOOK_METHOD(0x100e62f0, NCReplayWnd, ShowWindow)

// 0x100e68f0 (vslot 38, guard: NCReplayWnd::OnDestroy) - the three screens go, then NCWnd's
INT NCReplayWnd::OnDestroy()
{
    guard(NCReplayWnd::OnDestroy);
    DestroyChild(this, MenuWnd);
    DestroyChild(this, PlayWnd);
    DestroyChild(this, BenchWnd);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100e68f0, NCReplayWnd, OnDestroy)

// 0x100e7bf0 (vslot 37) - the menu (style 0x4002), the playing screen (0x400A: not hit tested) and the benchmark's
// result (0x4002), each the size of this window and hidden
INT NCReplayWnd::OnCreate()
{
    NCWnd::OnCreate();
    MenuWnd = new NCReplayMenuWnd;
    MenuWnd->SetPosition(0, 0, (INT)Width, (INT)Height);
    CreateChild(MenuWnd, NWS_NOCLOSE | NWS_VISIBLE);
    MenuWnd->HideWindow();

    PlayWnd = new NCReplayPlayWnd;
    PlayWnd->SetPosition(0, 0, (INT)Width, (INT)Height);
    CreateChild(PlayWnd, NWS_NOCLOSE | NWS_NOHITTEST | NWS_VISIBLE);
    PlayWnd->HideWindow();

    BenchWnd = new NCBenchMarkResultWnd;
    BenchWnd->SetPosition(0, 0, (INT)Width, (INT)Height);
    CreateChild(BenchWnd, NWS_NOCLOSE | NWS_VISIBLE);
    BenchWnd->HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100e7bf0, NCReplayWnd, OnCreate)

NW_CHECK_VTABLE(NCReplayWnd, 0x1019ace0, 68)
