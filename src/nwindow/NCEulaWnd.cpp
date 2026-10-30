/*=============================================================================
    NCEulaWnd.cpp - the license agreement box of the login screen (C2 only). Stock:
    0x1009a990 .. 0x1009aaa6 and 0x1009b650 .. 0x1009bdc9.
=============================================================================*/
#include "nwindow/NCEulaWnd.h"
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCAnnounceWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NConsoleWnd.h"

// ids (names ours)
enum
{
    NID_EULA_AGREE = 0xA000FA,
    NID_EULA_DISAGREE = 0xA000FB,
};
enum { SOUND_Click = 0 };

// NConsoleWnd::RequestServerList is the console's virtual 390 (0x1005AF40); the header's declaration lands on another
// slot of our vtable, so it is called by slot
static FORCEINLINE void RequestServerList()
{
    ::nw::VirtualCall<void>(GConsole, 390);
}

// 0x100b1ac0 - NCWnd's constructor and the vtable; only the backdrop is cleared
NCEulaWnd::NCEulaWnd()
{
    BackTex = NULL;
}
NW_HOOK_CTOR(0x100b1ac0, NCEulaWnd)

// 0x100b1b10
NW_HOOK_DTOR(0x100b1b10, NCEulaWnd)

// 0x100b0c00 (vslot 25)
INT NCEulaWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_EULA_AGREE)
            return OnAgree() != 0;
        if (wParam == NID_EULA_DISAGREE)
            return OnDisagree() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100b0c00, NCEulaWnd, WindowProc)

// 0x100b0c80 - like NCWarningWnd::SetText
void NCEulaWnd::SetText(const TCHAR* Text)
{
    if (Text && *Text)
        InfoWnd->SetText(Text);
}
NW_HOOK_METHOD(0x100b0c80, NCEulaWnd, SetText)

// 0x100b0ca0
void NCEulaWnd::ClearText()
{
    InfoWnd->ClearText();
}
NW_HOOK_METHOD(0x100b0ca0, NCEulaWnd, ClearText)

// 0x100b0cb0 (vslot 69) - back to the login box: the connection is dropped, the boxes emptied and the account line
// focused
INT NCEulaWnd::OnDisagree()
{
    GConsole->Engine->NetHandler->AuthReconnect();
    GConsole->LoginWnd->ResetWindows();
    GConsole->LoginWnd->ShowWindow();
    GConsole->LoginWnd->AuthWnd->AccountEdit->SetFocus();
    GConsole->SetState(L2CS_Login);
    return 1;
}
NW_HOOK_METHOD(0x100b0cb0, NCEulaWnd, OnDisagree)

// 0x100b1b60 (vslot 37) - the backdrop, the text (a 536 x 375 notice window) and the two buttons below it; no title
// bar control any more (C3 gives the window its own title)
INT NCEulaWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_ch3.LobbyWnd.lobby3_back"));

    InfoWnd = new NCNoticeInfoWnd;
    InfoWnd->SetPosition(6, 6, 536, 375);
    CreateChild(InfoWnd, NWS_VISIBLE);

    AgreeButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"),
                                     TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    AgreeButton->ID = NID_EULA_AGREE;
    AgreeButton->SetPosition(196, 396, 76, 23);
    CreateChild(AgreeButton, NWS_VISIBLE);
    AgreeButton->SetTextID(599);

    DisagreeButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"),
                                        TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    DisagreeButton->ID = NID_EULA_DISAGREE;
    DisagreeButton->SetPosition(276, 396, 76, 23);
    CreateChild(DisagreeButton, NWS_VISIBLE);
    DisagreeButton->SetTextID(600);
    return 1;
}
NW_HOOK_METHOD(0x100b1b60, NCEulaWnd, OnCreate)

// 0x100b1160 (vslot 38) - guarded in C2; the title bar goes with NCWnd::OnDestroy
INT NCEulaWnd::OnDestroy()
{
    guard(NCEulaWnd::OnDestroy);
    if (InfoWnd)
    {
        RemoveChildWindow(InfoWnd->Entry);
        delete InfoWnd;
        InfoWnd = NULL;
    }
    if (AgreeButton)
    {
        RemoveChildWindow(AgreeButton->Entry);
        delete AgreeButton;
        AgreeButton = NULL;
    }
    if (DisagreeButton)
    {
        RemoveChildWindow(DisagreeButton->Entry);
        delete DisagreeButton;
        DisagreeButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b1160, NCEulaWnd, OnDestroy)

// 0x100b1260 (vslot 56) - the backdrop stretched over the window; the guard carries NCNoticeWnd's name in the stock
// code (the function map had named this NCNoticeWnd::OnPaint)
INT NCEulaWnd::OnPaint(NCGDevice* Device)
{
    guard(NCNoticeWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, (INT)Width, (INT)Height, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b1260, NCEulaWnd, OnPaint)

// 0x100b1360 (vslot 68) - agreeing: in the Chinese mode (GLanguageType 4) the login box is made ready again (the plain
// account and password lines) and the announce box takes over with this window's Type; otherwise the server list is
// asked for, or the warning page shown first when Type has bit 0
INT NCEulaWnd::OnAgree()
{
    if (GLanguageType == 4)
    {
        NCAuthWnd* Auth = GConsole->LoginWnd->AuthWnd;
        Auth->Mode = 0;
        Auth->CardID = 0;
        Auth->CardPos = 0;
        Auth->AccountEdit->ShowWindow();
        Auth->AccountEdit->SetFocus();
        GConsole->LoginWnd->AuthWnd->HideWindow();
        GConsole->LoginWnd->ServerWnd->HideWindow();
        GConsole->LoginWnd->AnnounceWnd->ShowWindow();
        GConsole->LoginWnd->AnnounceWnd->SetFocus();
        GConsole->LoginWnd->AnnounceWnd->Type = Type;
        GConsole->SetState(L2CS_Login);
        HideWindow();
        return 1;
    }
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
NW_HOOK_METHOD(0x100b1360, NCEulaWnd, OnAgree)

// 0x100b0d20 (vslot 59) - Enter agrees, with the click sound
INT NCEulaWnd::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN)
    {
        OnAgree();
        GConsole->PlayInterfaceSound(SOUND_Click);
    }
    NCWnd::OnKeyDown(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x100b0d20, NCEulaWnd, OnKeyDown)

NW_CHECK_VTABLE(NCEulaWnd, 0x101917c0, 70)
