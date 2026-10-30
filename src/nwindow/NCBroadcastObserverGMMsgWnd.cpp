/*=============================================================================
    NCBroadcastObserverGMMsgWnd.cpp - the GM's message banner of the broadcast observer screen (C3 only; C3
    addresses). Stock: 0x101136B0 .. 0x10113D4D (ShowWindow, SetMsg, OnCreate, OnDestroy, OnTimer, OnPaint, the
    constructor 0x10113BE0 and the destructor 0x10113CD0).
=============================================================================*/
#include "nwindow/NCBroadcastObserverGMMsgWnd.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

enum
{
    TIMER_GMMsgHide = 0xB0000,    // ShowWindow starts it, OnTimer hides the banner (name ours)
    GMMSG_ShowMillis = 5000,
    SYSMSG_GMMsgDefault = 1431,   // GL2GameData + 0x5028C: the text OnCreate starts with
};

// 0x10113be0 (guard: the constructor's own name) - the three textures and the timer cleared, the text an empty string
NCBroadcastObserverGMMsgWnd::NCBroadcastObserverGMMsgWnd()
{
    guard(NCBroadcastObserverGMMsgWnd::NCBroadcastObserverGMMsgWnd);
    for (INT i = 0; i < 3; i++)
        PieceTex[i] = NULL;
    Msg = TEXT("");
    TimerID = 0;
    unguard;
}
NW_HOOK_CTOR(0x10113be0, NCBroadcastObserverGMMsgWnd)

// 0x10113cd0
NW_HOOK_DTOR(0x10113cd0, NCBroadcastObserverGMMsgWnd)

// 0x10119f60 (vslot 25) - NControl's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCBroadcastObserverGMMsgWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x10119f60)
#else
NW_SLOT_IS("?WindowProc@NCBroadcastObserverGMMsgWnd@@UAEHIIJ@Z", "?WindowProc@NControl@@UAEHIIJ@Z")   // NCBroadcastObserverGMMsgWnd::WindowProc = NControl::WindowProc (0x10119f60)
#endif

// 0x101136b0 (vslot 44, guard: NCBroadcastObserverGMMsgWnd::ShowWindow) - NCWnd's, then the five second timer starts
// over
void NCBroadcastObserverGMMsgWnd::ShowWindow()
{
    guard(NCBroadcastObserverGMMsgWnd::ShowWindow);
    NCWnd::ShowWindow();
    if (TimerID)
    {
        KillTimer(TimerID);
        TimerID = 0;
    }
    TimerID = SetTimer(TIMER_GMMsgHide, GMMSG_ShowMillis, 0);
    unguard;
}
NW_HOOK_METHOD(0x101136b0, NCBroadcastObserverGMMsgWnd, ShowWindow)

// 0x10113760 (guard: NCBroadcastObserverGMMsgWnd::SetMsg; ret 0xC: the FString comes by value) - the banner's text
void NCBroadcastObserverGMMsgWnd::SetMsg(FString NewMsg)
{
    guard(NCBroadcastObserverGMMsgWnd::SetMsg);
    Msg = NewMsg;
    unguard;
}
NW_HOOK_METHOD(0x10113760, NCBroadcastObserverGMMsgWnd, SetMsg)

// 0x101137f0 (vslot 37, guard: NCBroadcastObserverGMMsgWnd::OnCreate) - the three pieces of the banner and the
// default text
INT NCBroadcastObserverGMMsgWnd::OnCreate()
{
    guard(NCBroadcastObserverGMMsgWnd::OnCreate);
    NCWnd::OnCreate();
    for (INT i = 0; i < 3; i++)
        PieceTex[i] = FindTexture(*FString::Printf(TEXT("L2UI_CH3.BroadcastObs.br_msg2_back%d"), i + 1));
    Msg = *GL2GameData.SystemMsg[SYSMSG_GMMsgDefault].Message;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x101137f0, NCBroadcastObserverGMMsgWnd, OnCreate)

// 0x101138d0 (vslot 38, guard: NCBroadcastObserverGMMsgWnd::OnDestroy) - nothing of its own to free
INT NCBroadcastObserverGMMsgWnd::OnDestroy()
{
    guard(NCBroadcastObserverGMMsgWnd::OnDestroy);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x101138d0, NCBroadcastObserverGMMsgWnd, OnDestroy)

// 0x10113950 (vslot 36, guard: NCBroadcastObserverGMMsgWnd::OnTimer; ret 4) - the five seconds are over: the timer
// goes and the banner is hidden
INT NCBroadcastObserverGMMsgWnd::OnTimer(INT ID)
{
    guard(NCBroadcastObserverGMMsgWnd::OnTimer);
    if (ID == TIMER_GMMsgHide && TimerID)
    {
        KillTimer(TimerID);
        TimerID = 0;
        HideWindow();
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10113950, NCBroadcastObserverGMMsgWnd, OnTimer)

// 0x101139f0 (vslot 56, guard: NCBroadcastObserverGMMsgWnd::OnPaint) - the three pieces (32 wide, 128 high; the middle
// one stretched between the ends), then the text centered, wrapped at the window's width, in font 7
INT NCBroadcastObserverGMMsgWnd::OnPaint(NCGDevice* Device)
{
    guard(NCBroadcastObserverGMMsgWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, 0x20, 0x80, 0, 0, 0x20, 0x80, PieceTex[0], 0xFF, 1);
    Device->DrawTexture(0x20, 0, (INT)(Width - 64.0f), 0x80, 0, 0, 0x20, 0x80, PieceTex[1], 0xFF, 1);
    Device->DrawTexture((INT)(Width - 32.0f), 0, 0x20, 0x80, 0, 0, 0x20, 0x80, PieceTex[2], 0xFF, 1);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *Msg, (DWORD)(INT)Width, 0, 0, 0, 0, (L2FontType)7);
    Device->DrawText((INT)(Width * 0.5f - Size.cx / 2), (INT)(Height * 0.5f - Size.cy / 2), 0xFFDCDCDC, *Msg,
                     (INT)Width, 0, 0, (L2FontType)7, 0, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x101139f0, NCBroadcastObserverGMMsgWnd, OnPaint)

NW_CHECK_VTABLE(NCBroadcastObserverGMMsgWnd, 0x101a42e8, 68)
