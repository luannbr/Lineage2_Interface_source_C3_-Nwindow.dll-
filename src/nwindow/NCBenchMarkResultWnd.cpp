/*=============================================================================
    NCBenchMarkResultWnd.cpp - the benchmark's result page (C3 only; C3 addresses). Stock: 0x100E6810 .. 0x100E68BF
    (WindowProc, ShowWindow, OnOK), OnDestroy 0x100E6F80, OnPaint 0x100E7030, OnCreate 0x100E7560, the constructor
    0x100E7AA0 and the destructor 0x100E7B30.
=============================================================================*/
#include "nwindow/NCBenchMarkResultWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

enum
{
    NID_BENCHMARK_OK = (INT)0xAFF22015,   // the OK button's id (name ours)
    SYSSTR_BenchmarkOK = 140,             // GL2GameData + 0x6611C
};

// 0x100e7aa0 - the textures and the button come with OnCreate; the score text starts as an empty string
NCBenchMarkResultWnd::NCBenchMarkResultWnd() : Score(TEXT(""))
{
    BackTex = NULL;
    MiniLogoTex = NULL;
}
NW_HOOK_CTOR(0x100e7aa0, NCBenchMarkResultWnd)

// 0x100e7b30 - the score text goes with the members
NW_HOOK_DTOR(0x100e7b30, NCBenchMarkResultWnd)

// 0x100e6810 (vslot 25) - the OK button's command
INT NCBenchMarkResultWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_BENCHMARK_OK)
        return OnOK() != 0;
    return 0;
}
NW_HOOK_METHOD(0x100e6810, NCBenchMarkResultWnd, WindowProc)

// 0x100e6870 (vslot 44) - the replay manager's score text is taken before the window is shown
void NCBenchMarkResultWnd::ShowWindow()
{
    Score = GConsole->Engine->ReplayManager->GetBMScore();
    NCWnd::ShowWindow();
}
NW_HOOK_METHOD(0x100e6870, NCBenchMarkResultWnd, ShowWindow)

// 0x100e68a0 (vslot 68; name ours) - OK: the benchmark mode ends, the manager's data is cleared and it goes back to
// state 1, and the window the console keeps at +0x39E0 (the replay window) is shown
INT NCBenchMarkResultWnd::OnOK()
{
    GIsBenchmarking = 0;
    GConsole->Engine->ReplayManager->ResetBMData();
    GConsole->Engine->ReplayManager->SetState(1);
    ::nw::Field<NCWnd*>(GConsole, 0x39E0)->ShowWindow();
    return 1;
}
NW_HOOK_METHOD(0x100e68a0, NCBenchMarkResultWnd, OnOK)

// 0x100e6f80 (vslot 38; the guard string is NCReplayPlayWnd::OnDestroy, copied from the neighbour) - NCWnd's, then the
// button goes
INT NCBenchMarkResultWnd::OnDestroy()
{
    guard(NCReplayPlayWnd::OnDestroy);
    NCWnd::OnDestroy();
    if (OkButton)
    {
        RemoveChildWindow(OkButton->Entry);
        delete OkButton;
        OkButton = NULL;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100e6f80, NCBenchMarkResultWnd, OnDestroy)

// 0x100e7030 (vslot 56; the guard string is NCLoginWnd::OnPaint, copied from the login screen's paint) - the backdrop
// stretched over the window, the logo, the score's title and detail (the score text splits at " -- ") and the machine
// report (OS, CPU, video) in font 5
INT NCBenchMarkResultWnd::OnPaint(NCGDevice* Device)
{
    guard(NCLoginWnd::OnPaint);
    TCHAR Title[260];
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawStretchedTex(0, 0, (INT)Width, (INT)Height, 0, 0, 0x400, 0x300, BackTex, 0xFF);
    Device->DrawTexture(0, 0, 0xE8, 0x78, 0, 0, 0xE8, 0x78, MiniLogoTex, 0xFF, 1);
    appStrcpy(Title, *Score);
    if (appStrstr(Title, TEXT(" -- ")))
        *appStrstr(Title, TEXT(" -- ")) = 0;
    Device->DrawText(100, 150, 0xFFFFFFFF, Title, 0, 0, 0, (L2FontType)5, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(170, 180, 0xFFFFFFFF, appStrstr(*Score, TEXT(" -- ")), 0, 0, 0, (L2FontType)5, 0, 0, 0, 0, 0, 0, 0);
    // the machine report is a character array in Core.dll: the text is the variable's own address
    Device->DrawText(100, 230, 0xFFFFFFFF, (TCHAR*)&GMachineOS, 0, 0, 0, (L2FontType)5, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(100, 260, 0xFFFFFFFF, (TCHAR*)&GMachineCPU, 0, 0, 0, (L2FontType)5, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(100, 290, 0xFFFFFFFF, (TCHAR*)&GMachineVideo, 0, 0, 0, (L2FontType)5, 0, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100e7030, NCBenchMarkResultWnd, OnPaint)

// 0x100e7560 (vslot 37) - the backdrop and the small logo (localization.ini, as the login screen's), and the OK button
// (96 x 23) in the lower right corner
INT NCBenchMarkResultWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.BenchMarkWnd.BenchMark_Result"));
    FString LogoName;
    TCHAR Key[256];
    appSprintf(Key, TEXT("%s_MiniLogoTexture"), GCountryName);
    if (!GConfig->GetString(GCountryName, Key, LogoName, TEXT("localization.ini")))
        LogoName = TEXT("L2Font.start_minilogo-k");
    MiniLogoTex = FindTexture(*LogoName);

    OkButton = new NCButton(TEXT("L2UI_CH3.Button.BigButton"), TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    OkButton->ID = NID_BENCHMARK_OK;
    OkButton->SetPosition(GSEKScreenX - 0x92, GSEKScreenY - 0x7B, 0x60, 0x17);
    CreateChild(OkButton, NWS_VISIBLE);
    OkButton->SetText(*GL2GameData.SysString[SYSSTR_BenchmarkOK]);
    return 1;
}
NW_HOOK_METHOD(0x100e7560, NCBenchMarkResultWnd, OnCreate)

NW_CHECK_VTABLE(NCBenchMarkResultWnd, 0x1019b180, 69)
