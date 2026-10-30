/*=============================================================================
    NCReplayPlayWnd.cpp - the playing screen of the replay (C3 only; C3 addresses). Stock: OnCreate 0x100E6650,
    OnDestroy 0x100E6790, OnPaint 0x100E6DC0, the constructor 0x100E7A00 and the destructor 0x100E7A50.
=============================================================================*/
#include "nwindow/NCReplayPlayWnd.h"
#include "nwindow/NCGDevice.h"

// 0x100e7a00 - the two logos come with OnCreate
NCReplayPlayWnd::NCReplayPlayWnd() : LogoTex(NULL), MiniLogoTex(NULL)
{
}
NW_HOOK_CTOR(0x100e7a00, NCReplayPlayWnd)

// 0x100e7a50
NW_HOOK_DTOR(0x100e7a50, NCReplayPlayWnd)

// 0x10119f60 (vslot 25) - NControl's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCReplayPlayWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x10119f60)
#else
NW_SLOT_IS("?WindowProc@NCReplayPlayWnd@@UAEHIIJ@Z", "?WindowProc@NControl@@UAEHIIJ@Z")   // NCReplayPlayWnd::WindowProc = NControl::WindowProc (0x10119f60)
#endif

// 0x100e6790 (vslot 38, guard: NCReplayPlayWnd::OnDestroy) - nothing of its own to free
INT NCReplayPlayWnd::OnDestroy()
{
    guard(NCReplayPlayWnd::OnDestroy);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100e6790, NCReplayPlayWnd, OnDestroy)

// 0x100e6650 (vslot 37) - the logos by the country's names in localization.ini (the replay's own, then the small one
// the login screen uses)
INT NCReplayPlayWnd::OnCreate()
{
    NCWnd::OnCreate();
    FString LogoName;
    TCHAR Key[256];
    appSprintf(Key, TEXT("%s_ReplayLogoTexture"), GCountryName);
    if (!GConfig->GetString(GCountryName, Key, LogoName, TEXT("localization.ini")))
        LogoName = TEXT("L2Font.replay_logo-k");
    LogoTex = FindTexture(*LogoName);
    appSprintf(Key, TEXT("%s_MiniLogoTexture"), GCountryName);
    if (!GConfig->GetString(GCountryName, Key, LogoName, TEXT("localization.ini")))
        LogoName = TEXT("L2Font.start_minilogo-k");
    MiniLogoTex = FindTexture(*LogoName);
    return 1;
}
NW_HOOK_METHOD(0x100e6650, NCReplayPlayWnd, OnCreate)

// 0x100e6dc0 (vslot 56; the guard string is NCLoginWnd::OnPaint, copied from the login screen's paint) - the scale is
// the larger of height / 768 and width / 1024: the small logo (232 x 120) sits in the lower right corner at 0.8 of it
// and the replay logo (256 x 64) in the upper left one at 0.7 of it
INT NCReplayPlayWnd::OnPaint(NCGDevice* Device)
{
    guard(NCLoginWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    FLOAT MiniScale = (FLOAT)(__max(Width * 0.0009765625, Height * 0.0013020833333333333) * 0.8);
    FLOAT LogoScale = (FLOAT)(__max(Width * 0.0009765625, Height * 0.0013020833333333333) * 0.7);
    Device->DrawTexture((INT)(Width - MiniScale * 256.0f), (INT)(Height - MiniScale * 120.0f),
                        (INT)(MiniScale * 232.0f), (INT)(MiniScale * 120.0f), 0, 0, 0xE8, 0x78, MiniLogoTex, 0xFF, 1);
    Device->DrawTexture((INT)(LogoScale * 30.0f), (INT)(LogoScale * 30.0f), (INT)(LogoScale * 256.0f),
                        (INT)(LogoScale * 64.0f), 0, 0, 0x100, 0x40, LogoTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100e6dc0, NCReplayPlayWnd, OnPaint)

NW_CHECK_VTABLE(NCReplayPlayWnd, 0x1019b070, 68)
