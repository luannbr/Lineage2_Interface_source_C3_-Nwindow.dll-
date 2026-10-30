/*=============================================================================
    NCObserverWnd.cpp - the observer mode box (C3 addresses). Stock: WindowProc 0x100AC530, OnExit 0x100AC590, OnPaint
    0x100AD630, OnDestroy 0x100AD6C0, OnCreate 0x100AF700, the constructor 0x100AFFC0 and the destructor 0x100B0010.
=============================================================================*/
#include "nwindow/NCObserverWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// ids (names ours)
enum
{
    NID_OBSERVER_EXIT = 0x1012,
    NID_OBSERVER_FRAME = 0xB000F0,
};
enum { SYSSTR_ObserverExit = 611 };   // the button's text (name ours)

// 0x100affc0 - the button comes with OnCreate
NCObserverWnd::NCObserverWnd()
{
    BackTex = NULL;
    ExitButton = NULL;
}
NW_HOOK_CTOR(0x100affc0, NCObserverWnd)

// 0x100b0010
NW_HOOK_DTOR(0x100b0010, NCObserverWnd)

// 0x100ac530 (vslot 25)
INT NCObserverWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_OBSERVER_EXIT)
        return OnExit() != 0;
    return 0;
}
NW_HOOK_METHOD(0x100ac530, NCObserverWnd, WindowProc)

// 0x100ac590 (vslot 68; name ours) - the exit button: the server is asked to end the observation (an empty packet)
INT NCObserverWnd::OnExit()
{
    L2ParamStack Params(10);
    ::nw::VirtualCall<void>(GConsole, 403, &Params);   // NConsoleWnd::RequestObserverEndPacket (the header's slot number is C2's)
    return 1;
}
NW_HOOK_METHOD(0x100ac590, NCObserverWnd, OnExit)

// 0x100ad6c0 (vslot 38, guard: NCObserverWnd::OnDestroy) - the exit button goes, then NCWnd's
INT NCObserverWnd::OnDestroy()
{
    guard(NCObserverWnd::OnDestroy);
    if (ExitButton)
    {
        RemoveChildWindow(ExitButton->Entry);
        delete ExitButton;
        ExitButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100ad6c0, NCObserverWnd, OnDestroy)

// 0x100ad630 (vslot 56) - the backdrop piece (102 x 32) 12 pixels in; the title bar draws itself
INT NCObserverWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(12, 0, 0x66, 0x20, 0, 0, 0x66, 0x20, BackTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100ad630, NCObserverWnd, OnPaint)

// 0x100af700 (vslot 37) - a title bar 12 wide and as high as the window (the frame is NCWnd's FrameCtrl), the
// backdrop texture and the exit button (76 x 23 at (19, 5))
INT NCObserverWnd::OnCreate()
{
    NCWnd::OnCreate();
    NRectF LocalRect = { 0, 0, 0.0f, 0.0f };
    GetLocalRect(&LocalRect);

    FrameCtrl = new NCFrameCtrl(1, 1, 1);
    FrameCtrl->ID = NID_OBSERVER_FRAME;
    FrameCtrl->SetPosition(0, 0, 12, (INT)Height);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    BackTex = FindTexture(TEXT("L2UI_ch3.etc.broadcast_back"));

    ExitButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    ExitButton->ID = NID_OBSERVER_EXIT;
    ExitButton->SetPosition(19, 5, 76, 23);
    CreateChild(ExitButton, NWS_VISIBLE);
    ExitButton->SetText(*GL2GameData.SysString[SYSSTR_ObserverExit]);
    return 1;
}
NW_HOOK_METHOD(0x100af700, NCObserverWnd, OnCreate)

NW_CHECK_VTABLE(NCObserverWnd, 0x10190e18, 69)
