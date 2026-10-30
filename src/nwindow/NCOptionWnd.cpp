/*=============================================================================
    NCOptionWnd.cpp - the options window. Stock: 0x1007de70 .. 0x1007df36, 0x100817c0 ..
    0x100820fa, plus OnCancel 0x10059d00 (the body NCMatchOptionWnd and two other windows
    share).
=============================================================================*/
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCVideoOptionWnd.h"
#include "nwindow/NCAudioOptionWnd.h"
#include "nwindow/NCGameOptionWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x100bd5d0 (ret 4) - NCWnd's constructor; the argument (the page that cannot be used, see OnCreate) and the
// backdrop cleared
NCOptionWnd::NCOptionWnd(INT InRestricted)
{
    bRestricted = InRestricted;
    BackTex = NULL;
}
NW_HOOK_CTOR_ARGS(0x100bd5d0, NCOptionWnd, (INT A), (A))

// 0x100bd630
NW_HOOK_DTOR(0x100bd630, NCOptionWnd)

// 0x100b72a0 (vslot 25) - C2 no longer refuses the messages of a disabled window here
INT NCOptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_OPTION_OK)
            return OnOK() != 0;
        if (wParam == NID_OPTION_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_OPTION_APPLY)
            return OnApply() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100b72a0, NCOptionWnd, WindowProc)

// 0x1002a230 (ret 8; name ours - NCTabControl's, the options window is its only caller) - the tab button of Index is
// disabled (bDisabled) or enabled again (the stock tests the count first, then calls the button's EnableWindow, slot 42,
// with bDisabled == 0). A stand-in class carries it, so that it is hooked as a method
struct NTabControlFns : public NCTabControl
{
    __declspec(noinline) void SetTabDisabled(INT Index, INT bDisabled);
};
void NTabControlFns::SetTabDisabled(INT Index, INT bDisabled)
{
    if (Tabs.Num() > 0)
        Tabs(Index)->EnableWindow(bDisabled == 0);
}
NW_HOOK_METHOD(0x1002a230, NTabControlFns, SetTabDisabled)

// 0x100bd680 (vslot 37) - the title bar (a frame control, 20 high), the backdrops, the three pages (each 256 x 317 under
// the tabs; the video and game ones with the style bit 0x10), the tab control with its three tabs (the game page's tab
// disabled when the window was made restricted) and the OK / Cancel / Apply buttons
INT NCOptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    OptionBackTex = FindTexture(TEXT("L2ui_ch3.option1_back"));

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameID = NID_OPTION_FRAME;
    FrameCtrl->ID = NID_OPTION_FRAME;
    FrameCtrl->SetPosition(0, 0, (INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    BackTex = FindTexture(TEXT("L2UI_ch3.SSQWnd.ssq_back"));

    VideoWnd = new NCVideoOptionWnd;
    VideoWnd->SetPosition(0, 48, 256, 317);
    CreateChild(VideoWnd, NWS_VISIBLE | 0x10);

    AudioWnd = new NCAudioOptionWnd;
    AudioWnd->SetPosition(0, 48, 256, 317);
    CreateChild(AudioWnd, NWS_VISIBLE);

    GameWnd = new NCGameOptionWnd;
    GameWnd->SetPosition(0, 48, 256, 317);
    CreateChild(GameWnd, NWS_VISIBLE | 0x10);

    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_OPTION_TABS;
    TabCtrl->SetPosition(12, 25, 224, 23);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(206, VideoWnd, 74, 23, TEXT("l2ui_ch3.tabbutton.normal_tab_on"),
                    TEXT("l2ui_ch3.tabbutton.normal_tab"), 1, -1);
    TabCtrl->AddTab(207, AudioWnd, 74, 23, TEXT("l2ui_ch3.tabbutton.normal_tab_on"),
                    TEXT("l2ui_ch3.tabbutton.normal_tab"), 1, -1);
    TabCtrl->AddTab(208, GameWnd, 74, 23, TEXT("l2ui_ch3.tabbutton.normal_tab_on"),
                    TEXT("l2ui_ch3.tabbutton.normal_tab"), 0, -1);
    if (bRestricted)
        ((NTabControlFns*)TabCtrl)->SetTabDisabled(2, 1);

    OKButton = new NCButton(TEXT("L2UI_CH3.BUTTON.BTN1_normal"),
                                  TEXT("L2UI_CH3.BUTTON.BTN1_normalon"), 0);
    OKButton->ID = NID_OPTION_OK;
    OKButton->SetPosition(11, 372, 76, 23);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(140);

    CancelButton = new NCButton(TEXT("L2UI_CH3.BUTTON.BTN1_normal"),
                                      TEXT("L2UI_CH3.BUTTON.BTN1_normalon"), 0);
    CancelButton->ID = NID_OPTION_CANCEL;
    CancelButton->SetPosition(91, 372, 76, 23);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(141);

    ApplyButton = new NCButton(TEXT("L2UI_CH3.BUTTON.BTN1_normal"),
                                     TEXT("L2UI_CH3.BUTTON.BTN1_normalon"), 0);
    ApplyButton->ID = NID_OPTION_APPLY;
    ApplyButton->SetPosition(171, 372, 76, 23);
    CreateChild(ApplyButton, NWS_VISIBLE);
    ApplyButton->SetTextID(218);
    return 1;
}
NW_HOOK_METHOD(0x100bd680, NCOptionWnd, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x100bb1b0 (vslot 38)
INT NCOptionWnd::OnDestroy()
{
    DestroyChild(this, VideoWnd);
    DestroyChild(this, AudioWnd);
    DestroyChild(this, GameWnd);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    DestroyChild(this, ApplyButton);
    DestroyChild(this, TabCtrl);
    DestroyChild(this, UnusedB4);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100bb1b0, NCOptionWnd, OnDestroy)

// 0x100bbd00 (vslot 44) - C2: each page first empties its lists and clears its boxes (ClearItems), then
// fills its controls (InitItems / InitVolumes); the tab control shows the current page last
void NCOptionWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    VideoWnd->ClearItems();
    AudioWnd->ClearItems();
    GameWnd->ClearItems();
    VideoWnd->InitItems();
    AudioWnd->InitVolumes();
    GameWnd->InitItems();
    TabCtrl->ShowCurrentPage();
}
NW_HOOK_METHOD(0x100bbd00, NCOptionWnd, ShowWindow)

// 0x100bb090 (vslot 56) - the header strip (256 x 28 at y 20) and the page's backdrop (256 x 353 at y 48)
INT NCOptionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCOptionWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 20, 256, 28, 0, 0, 256, 28, BackTex);
    if (OptionBackTex)
        Device->DrawTexture(0, 48, 256, 353, 0, 0, 256, 353, OptionBackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100bb090, NCOptionWnd, OnPaint)

// 0x100bb390 (vslot 68) - every page applies its settings (video, audio, game), the first run is marked done ("2" in C3)
// and the window closes
INT NCOptionWnd::OnOK()
{
    VideoWnd->Apply();
    AudioWnd->Apply();
    GameWnd->Apply();
    GConfig->SetString(TEXT("FirstRun"), TEXT("FirstRun"), TEXT("2"), TEXT("Option.ini"));
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100bb390, NCOptionWnd, OnOK)

// 0x1007bee0 (vslot 69) - body shared with NCMatchOptionWnd's slot 63 and two other windows'
// (slots 64 and 72)
INT NCOptionWnd::OnCancel()
{
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1007bee0, NCOptionWnd, OnCancel)

// 0x100bb3f0 (vslot 70) - the same, and the window stays
INT NCOptionWnd::OnApply()
{
    VideoWnd->Apply();
    AudioWnd->Apply();
    GameWnd->Apply();
    GConfig->SetString(TEXT("FirstRun"), TEXT("FirstRun"), TEXT("2"), TEXT("Option.ini"));
    return 1;
}
NW_HOOK_METHOD(0x100bb3f0, NCOptionWnd, OnApply)


NW_CHECK_VTABLE(NCOptionWnd, 0x10193360, 71)
