/*=============================================================================
    NCReplayFunctionWnd.cpp - the replay menu's four buttons (C3 only; C3 addresses). Stock: 0x100E64F0 .. 0x100E6645
    (WindowProc and the handlers), OnDestroy 0x100E69F0, OnPaint 0x100E6B20, OnCreate 0x100E7270, the constructor
    0x100E77C0 and the destructor 0x100E7830.
=============================================================================*/
#include "nwindow/NCReplayFunctionWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCGodWnd.h"
#include "nwindow/NConsoleWnd.h"

// NWM_COMMAND ids of the buttons (names ours)
enum
{
    NID_REPLAY_PLAY = (INT)0xAFF22011,
    NID_REPLAY_BENCHMARK = (INT)0xAFF22012,
    NID_REPLAY_OPTIONS = (INT)0xAFF22013,
    NID_REPLAY_EXIT = (INT)0xAFF22014,
};

// An object made as the stock code makes it: StaticAllocateObject of its class, then the constructor at Ctor (the
// class's own C++ constructor, hooked there with the same arguments).
// The standalone has no stock code: the constructor that runs is T's own (each one used here is hooked at Ctor with the
// same arguments, so the hook build runs the same one through the stock address).
template <class T, class... A> static FORCEINLINE T* NewStock(DWORD Ctor, A... a)
{
#if NW_HOOK_MODE
    void* Mem = T::operator new(sizeof(T));
    return Mem ? ::nw::StockCallAt<T*>(Ctor, Mem, a...) : NULL;
#else
    (void)Ctor;
    return new T(a...);
#endif
}

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

// 0x100e77c0 - the backdrop and the four buttons come with OnCreate
NCReplayFunctionWnd::NCReplayFunctionWnd()
    : BackTex(NULL), ReplayButton(NULL), BenchmarkButton(NULL), OptionsButton(NULL), ExitButton(NULL)
{
}
NW_HOOK_CTOR(0x100e77c0, NCReplayFunctionWnd)

// 0x100e7830
NW_HOOK_DTOR(0x100e7830, NCReplayFunctionWnd)

// 0x100e64f0 (vslot 25) - the buttons' ids go to the four handlers (virtual slots 68, 69, 70 and 71)
INT NCReplayFunctionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_REPLAY_PLAY)
            return OnReplayBtnClick() != 0;
        if (wParam == NID_REPLAY_BENCHMARK)
            return OnBenchmarkBtnClick() != 0;
        if (wParam == NID_REPLAY_OPTIONS)
            return OnOptionsBtnClick() != 0;
        if (wParam == NID_REPLAY_EXIT)
            return OnExitBtnClick() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100e64f0, NCReplayFunctionWnd, WindowProc)

// 0x100e65a0 (vslot 68; name ours) - the replay: the god window hidden and the manager's state 0 (it plays)
INT NCReplayFunctionWnd::OnReplayBtnClick()
{
    GConsole->GodWnd->HideWindow();
    GConsole->Engine->ReplayManager->SetState(0);
    return 1;
}
NW_HOOK_METHOD(0x100e65a0, NCReplayFunctionWnd, OnReplayBtnClick)

// 0x100e65d0 (vslot 69; name ours) - the benchmark: the mode on, the god window hidden, the manager's state 0 with its
// benchmark data cleared and the random numbers started over
INT NCReplayFunctionWnd::OnBenchmarkBtnClick()
{
    GIsBenchmarking = 1;
    GConsole->GodWnd->HideWindow();
    GConsole->Engine->ReplayManager->SetState(0);
    GConsole->Engine->ReplayManager->ResetBMData();
    appRandInit(0);
    return 1;
}
NW_HOOK_METHOD(0x100e65d0, NCReplayFunctionWnd, OnBenchmarkBtnClick)

// 0x100e6630 (vslot 70; name ours) - the options window of the god window
INT NCReplayFunctionWnd::OnOptionsBtnClick()
{
    static_cast<NCGodWnd*>(GConsole->GodWnd)->ShowOptionWnd();
    return 1;
}
NW_HOOK_METHOD(0x100e6630, NCReplayFunctionWnd, OnOptionsBtnClick)

// 0x10092210 (vslot 71; name ours) - the client ends (the body is NCAuthWnd::OnCancel's, folded by the linker)
#if NW_HOOK_MODE
NW_NAKED INT NCReplayFunctionWnd::OnExitBtnClick() NW_STOCK(0x10092210)
#else
NW_SLOT_IS("?OnExitBtnClick@NCReplayFunctionWnd@@UAEHXZ", "?OnCancel@NCAuthWnd@@UAEHXZ")   // NCReplayFunctionWnd::OnExitBtnClick = NCAuthWnd::OnCancel (0x10092210)
#endif

// 0x100e69f0 (vslot 38, guard: NCReplayFunctionWnd::OnDestroy) - NCWnd's, then the four buttons go
INT NCReplayFunctionWnd::OnDestroy()
{
    guard(NCReplayFunctionWnd::OnDestroy);
    NCWnd::OnDestroy();
    DestroyChild(this, ReplayButton);
    DestroyChild(this, BenchmarkButton);
    DestroyChild(this, OptionsButton);
    DestroyChild(this, ExitButton);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100e69f0, NCReplayFunctionWnd, OnDestroy)

// 0x100e6b20 (vslot 56, guard: NCReplayFunctionWnd::OnPaint) - the backdrop (a 120 x 138 piece)
INT NCReplayFunctionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCReplayFunctionWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, 120, 138, 0, 0, 120, 138, BackTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100e6b20, NCReplayFunctionWnd, OnPaint)

// 0x100e7270 (vslot 37) - the backdrop and the four buttons (96 x 23), one under the other
INT NCReplayFunctionWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.LobbyWnd.lobby_menuback"));

    ReplayButton = NewStock<NCButton>(0x10002a40, TEXT("L2UI_CH3.Button.BigButton"),
                                      TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    ReplayButton->ID = NID_REPLAY_PLAY;
    ReplayButton->SetPosition(13, 20, 96, 23);
    CreateChild(ReplayButton, NWS_VISIBLE);
    ReplayButton->SetTextID(1023);

    BenchmarkButton = NewStock<NCButton>(0x10002a40, TEXT("L2UI_CH3.Button.BigButton"),
                                         TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    BenchmarkButton->ID = NID_REPLAY_BENCHMARK;
    BenchmarkButton->SetPosition(13, 45, 96, 23);
    CreateChild(BenchmarkButton, NWS_VISIBLE);
    BenchmarkButton->SetTextID(1022);

    OptionsButton = NewStock<NCButton>(0x10002a40, TEXT("L2UI_CH3.Button.BigButton"),
                                       TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    OptionsButton->ID = NID_REPLAY_OPTIONS;
    OptionsButton->SetPosition(13, 70, 96, 23);
    CreateChild(OptionsButton, NWS_VISIBLE);
    OptionsButton->SetTextID(156);

    ExitButton = NewStock<NCButton>(0x10002a40, TEXT("L2UI_CH3.Button.BigButton"),
                                    TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    ExitButton->ID = NID_REPLAY_EXIT;
    ExitButton->SetPosition(13, 95, 96, 23);
    CreateChild(ExitButton, NWS_VISIBLE);
    ExitButton->SetTextID(153);
    return 1;
}
NW_HOOK_METHOD(0x100e7270, NCReplayFunctionWnd, OnCreate)

NW_CHECK_VTABLE(NCReplayFunctionWnd, 0x1019adf0, 72)
