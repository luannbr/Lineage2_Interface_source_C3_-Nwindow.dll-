/*=============================================================================
    NCAnnounceWnd.cpp - the announce box of the login screen (C2 only). Stock: 0x1009aab0 ..
    0x1009ab0f (WindowProc) and 0x1009bdd0 .. 0x1009c39f.
=============================================================================*/
#include "nwindow/NCAnnounceWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NConsoleWnd.h"

// ids (names ours)
enum
{
    NID_ANNOUNCE_OK = 0xA000FC,
};
enum { SOUND_Click = 0 };

// NConsoleWnd::RequestServerList is the console's virtual 390 (0x1005AF40); the header's declaration lands on another
// slot of our vtable, so it is called by slot
static FORCEINLINE void RequestServerList()
{
    ::nw::VirtualCall<void>(GConsole, 390);
}

// 0x100b1d90 - NCWnd's constructor and the vtable; only the backdrop is cleared
NCAnnounceWnd::NCAnnounceWnd()
{
    BackTex = NULL;
}
NW_HOOK_CTOR(0x100b1d90, NCAnnounceWnd)

// 0x100b1de0
NW_HOOK_DTOR(0x100b1de0, NCAnnounceWnd)

// 0x100b0d60 (vslot 25)
INT NCAnnounceWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_ANNOUNCE_OK)
        return OnOK() != 0;
    return 0;
}
NW_HOOK_METHOD(0x100b0d60, NCAnnounceWnd, WindowProc)

// 0x100b16f0 (vslot 37) - the picture and the OK button, centered (38 pixels left of the middle) near the bottom
INT NCAnnounceWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_ch3.LobbyWnd.Announce_cn"));

    OKButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"),
                                  TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    OKButton->ID = NID_ANNOUNCE_OK;
    OKButton->SetPosition((INT)(Width * 0.5f - 38.0f), 396, 76, 23);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(140);                                      // "OK"
    return 1;
}
NW_HOOK_METHOD(0x100b16f0, NCAnnounceWnd, OnCreate)

// 0x100b1490 (vslot 38) - guarded in C2; the title bar goes with NCWnd::OnDestroy
INT NCAnnounceWnd::OnDestroy()
{
    guard(NCAnnounceWnd::OnDestroy);
    if (OKButton)
    {
        RemoveChildWindow(OKButton->Entry);
        delete OKButton;
        OKButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b1490, NCAnnounceWnd, OnDestroy)

// 0x100b1540 (vslot 56) - the backdrop stretched over the window, when there is one
INT NCAnnounceWnd::OnPaint(NCGDevice* Device)
{
    guard(NCAnnounceWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, (INT)Width, (INT)Height, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b1540, NCAnnounceWnd, OnPaint)

// 0x100b1640 (vslot 68) - the server list is asked for, or the warning page shown first when Type has bit 0
INT NCAnnounceWnd::OnOK()
{
    if (Type == 0)
    {
        RequestServerList();
        return 1;
    }
    if (Type & 1)
    {
        GConsole->ShowWarningWnd();
        return 1;
    }
    RequestServerList();
    return 1;
}
NW_HOOK_METHOD(0x100b1640, NCAnnounceWnd, OnOK)

// 0x100b1680 (vslot 59) - Enter does what OK does; only the warning page comes with the click sound and goes on to
// NCWnd's key handling (the two server list paths return at once, as the stock code does)
INT NCAnnounceWnd::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN)
    {
        if (Type == 0)
        {
            RequestServerList();
            return 1;
        }
        if ((Type & 1) == 0)
        {
            RequestServerList();
            return 1;
        }
        GConsole->ShowWarningWnd();
        GConsole->PlayInterfaceSound(SOUND_Click);
    }
    NCWnd::OnKeyDown(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x100b1680, NCAnnounceWnd, OnKeyDown)

NW_CHECK_VTABLE(NCAnnounceWnd, 0x101918d8, 69)
