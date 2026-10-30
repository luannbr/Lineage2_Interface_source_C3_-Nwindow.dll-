/*=============================================================================
    NCBoardWnd.cpp - the community board and its page viewer. Stock: 0x10026420 .. 0x10026488,
    0x10026490 .. 0x100264b0, 0x10026510 .. 0x10026630, 0x100266b0 .. 0x1002699d,
    0x10027460 .. 0x100276d3, 0x10027880 .. 0x10027ac6, 0x10027ad0 .. 0x10027b71,
    0x100296b0 .. 0x1002976f, 0x10029a40 .. 0x10029fbc, 0x1002a890 .. 0x1002a89a (the viewer's
    slot 72 and the board's ShowWindow are bodies the linker folded with other windows').
=============================================================================*/
#include "nwindow/NCBoardWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NHtmlLayout.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <string.h>

enum
{
    NID_BOARD_FRAME = (INT)0xF0F01420,
    NID_BOARD_BUTTON = (INT)0xF0F01422,
    NID_BOARD_TABS = (INT)0xF0F01423,
    NID_BBS_HIDE = (INT)0xF0F01411,
};

enum { STR_BoardButton1 = 377 };  // .. 382
enum { SOUND_Click = 0 };

enum
{
    MSG_IncorrectName = 204,           // "Incorrect name. Please try again. "
    MSG_TitleTooLong = 1207,           // "Please input title using less than 128 characters." (C3: 1207; C2: 328)
    MSG_TooLong3000 = 329,             // "Please input contents using less than 3000 characters."
};

/*-----------------------------------------------------------------------------
    NCBBSHtmlViewer.
-----------------------------------------------------------------------------*/

// 0x10039810 - NCHtmlViewer's, and the piece counts and the lock cleared (the pieces' text buffers are left as allocated)
NCBBSHtmlViewer::NCBBSHtmlViewer()
{
    Unknown110 = 0;
    NumPostPieces = 0;
    NumFormPieces = 0;
    NumPageParts = 0;
    bLocked = 0;
}
NW_HOOK_CTOR(0x10039810, NCBBSHtmlViewer)

// 0x10039880
NW_HOOK_DTOR(0x10039880, NCBBSHtmlViewer)


// 0x10035880 (vslot 25) - the console's NWM_COMMAND that hides the viewer (slot 79, by number: NCScrollWnd.h
// shifts the slots of its children by one for now)
INT NCBBSHtmlViewer::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCHtmlViewer::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == (WPARAM)NID_BBS_HIDE)
        return ::nw::VirtualCall<INT>(this, 79) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10035880, NCBBSHtmlViewer, WindowProc)

// 0x100358e0 (vslot 37) - NCScrollWnd's, not NCHtmlViewer's (CreateWidth / CreateHeight stay 0)
INT NCBBSHtmlViewer::OnCreate()
{
    NCScrollWnd::OnCreate();
    BoardBackTex = FindTexture(TEXT("L2UI.bbs_back"));
    return 1;
}
NW_HOOK_METHOD(0x100358e0, NCBBSHtmlViewer, OnCreate)

// 0x100398d0 (vslot 38) - guarded
INT NCBBSHtmlViewer::OnDestroy()
{
    guard(NCBBSHtmlViewer::OnDestroy);
    NCHtmlViewer::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100398d0, NCBBSHtmlViewer, OnDestroy)

// 0x10036a50 (vslot 44)
void NCBBSHtmlViewer::ShowWindow()
{
    NCHtmlViewer::ShowWindow();
}
NW_HOOK_METHOD(0x10036a50, NCBBSHtmlViewer, ShowWindow)

// 0x10036910 (vslot 56) - NCHtmlViewer's, clipped inside the board's frame (C3: the frame itself is the board's)
INT NCBBSHtmlViewer::OnPaint(NCGDevice* Device)
{
    guard(NCBBSHtmlViewer::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->SetClip(ScreenX + 3, ScreenY + 10, (INT)(Width - 6.0f), (INT)(Height - 20.0f));
    Device->PushOrigin(ScrollX, ScrollY);
    if (Doc && bLoaded)
    {
        Doc->Draw(Device);
        if (bLoaded)
            SetContentHeight(Doc->GetContentHeight());
    }
    Device->PopOrigin();
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10036910, NCBBSHtmlViewer, OnPaint)

// 0x10036a60 (vslot 72) - the value of the control whose caption is Name. A missing value says the title (329: the
// contents; C3: 1207 for a one-line box) is too long; a one-line box may not hold the characters the console rejects
// (204); any value is 2..3000 bytes in the ANSI code page
TCHAR* NCBBSHtmlViewer::GetControlValue(const TCHAR* Name)
{
    ANSICHAR Ansi[6000];
    if (!Name)
        return NULL;
    INT N = Controls.Num();
    if (N < 1)
        return NULL;
    INT i = 0;
    NControl* Control;
    for (;;)
    {
        Control = Controls(i);
        const TCHAR* Caption = Control->GetCaption();
        if (Caption && !wcscmp(Caption, Name))
            break;
        i++;
        if (N <= i)
            return NULL;
    }
    INT Message;
    if (!Control->GetValue())
        Message = Control->IsA(NClass::NCHtmlMultiEdit()) ? MSG_TooLong3000 : MSG_TitleTooLong;
    else
    {
        if (!Control->IsA(NClass::NCHtmlMultiEdit()) && GConsole->HasInvalidChars((TCHAR*)Control->GetValue()))
        {
            GConsole->ShowSystemMessage(MSG_IncorrectName, 0xFFB09B79);
            return NULL;
        }
        WideCharToMultiByte(CP_ACP, 0, Control->GetValue(), -1, Ansi, 6000, NULL, NULL);
        INT Length = (INT)strlen(Ansi);
        if (Length > 0 && Length < 3001)
            return (TCHAR*)Control->GetValue();
        Message = Control->IsA(NClass::NCHtmlMultiEdit()) ? MSG_TooLong3000 : MSG_TitleTooLong;
    }
    GConsole->ShowSystemMessage(Message, 0xFFB09B79);
    return NULL;
}
NW_HOOK_METHOD(0x10036a60, NCBBSHtmlViewer, GetControlValue)

// a link or a button's command to the console (slot 402, by number: see NCBoardWnd's), and the answer: 2 closes
// the viewer (its owner's WM 0xF0000004, else itself), 1 locks it
static FORCEINLINE INT RunViewerCommand(NCBBSHtmlViewer* Viewer, TCHAR* Command)
{
    return ::nw::VirtualCall<INT>(GConsole, 402, Command, Viewer);
}

// 0x10035960 (vslot 74) - locked once the console accepts a command; the sound plays even for an empty link; a
// command also forgets the pieces of a page being received
INT NCBBSHtmlViewer::OnActionLink(TCHAR* Action)
{
    if (!bLocked)
    {
        NumPostPieces = 0;
        NumFormPieces = 0;
        NumPageParts = 0;
        wcscpy(GHtmlLinkTarget, Action);
        if (GHtmlLinkTarget[0])
        {
            INT Result = RunViewerCommand(this, GHtmlLinkTarget);
            if (Result == 2)
            {
                if (Owner)
                    Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
                else
                    HideWindow();
            }
            else if (Result == 1)
                bLocked = 1;
        }
        PlayInterfaceSound(SOUND_Click);
    }
    return 1;
}
NW_HOOK_METHOD(0x10035960, NCBBSHtmlViewer, OnActionLink)

// 0x10035a10 (vslot 75) - the same for a button (no emptiness test, no sound)
INT NCBBSHtmlViewer::OnButton(TCHAR* Action)
{
    if (!bLocked)
    {
        NumPostPieces = 0;
        NumFormPieces = 0;
        NumPageParts = 0;
        wcscpy(GHtmlLinkTarget, Action);
        INT Result = RunViewerCommand(this, GHtmlLinkTarget);
        if (Result == 2)
        {
            if (Owner)
            {
                Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
                return 1;
            }
            HideWindow();
            return 1;
        }
        if (Result == 1)
            bLocked = 1;
    }
    return 1;
}
NW_HOOK_METHOD(0x10035a10, NCBBSHtmlViewer, OnButton)

// stock body shared with NCOptionWnd::OnCancel (0x10059d00)
INT NCBBSHtmlViewer::OnHideCommand()
{
    HideWindow();
    return 1;
}

NW_CHECK_VTABLE(NCBBSHtmlViewer, 0x1017f650, 80)

/*-----------------------------------------------------------------------------
    NCBoardWnd.
-----------------------------------------------------------------------------*/

// 0x10039d30 - C3: only bLocked is cleared
NCBoardWnd::NCBoardWnd()
{
    bLocked = 0;
}
NW_HOOK_CTOR(0x10039d30, NCBoardWnd)

// 0x10039d80
NW_HOOK_DTOR(0x10039d80, NCBoardWnd)


// 0x10035b30 (vslot 25) - the tab control (69 .. 68) and the button
INT NCBoardWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_BOARD_BUTTON)
            return OnButton1() != 0;
        if ((INT)wParam == NID_BOARD_TABS)
            return OnTab((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10035b30, NCBoardWnd, WindowProc)

// the command to the console, sent with the viewer; one it accepts (1) locks the board (the console's slot 402,
// by number: NConsoleWnd.h compiles its new virtuals 76 slots too high for now)
static FORCEINLINE INT RunBoardCommand(NCBoardWnd* Board, TCHAR* Command)
{
    return ::nw::VirtualCall<INT>(GConsole, 402, Command, Board->HtmlViewer);
}

// 0x10035bb0 (vslot 68, name ours) - a tab: its command (0 .. 6); an empty one does nothing
INT NCBoardWnd::OnTab(INT Tab)
{
    TCHAR Command[128];
    Command[0] = 0;
    switch (Tab)
    {
    case 0: appStrcpy(Command, Commands[0]); break;
    case 1: appStrcpy(Command, Commands[1]); break;
    case 2: appStrcpy(Command, Commands[2]); break;
    case 3: appStrcpy(Command, Commands[3]); break;
    case 4: appStrcpy(Command, Commands[4]); break;
    case 5: appStrcpy(Command, Commands[5]); break;
    case 6: appStrcpy(Command, Commands[6]); break;
    }
    if (Command[0])
    {
        if (RunBoardCommand(this, Command) == 1)
        {
            bLocked = 1;
            return 1;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10035bb0, NCBoardWnd, OnTab)

// 0x10035c90 (vslot 69) - the button's command, sent while the board is not locked; an accepted one locks it
INT NCBoardWnd::OnButton1()
{
    if (Commands[7][0] && !bLocked)
    {
        if (RunBoardCommand(this, Commands[7]) == 1)
        {
            bLocked = 1;
            return 1;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10035c90, NCBoardWnd, OnButton1)

// 0x10035db0 (vslot 49) - NCWnd's minimizing with the board's own icon, and no sound
void NCBoardWnd::Minimize()
{
    if (Icon)
        Icon->SetIcon(TEXT("L2UI_CH3.BoardWnd.board_icon"), 0x186, 1);
    if (ScreenX < 0 && Icon)
        Icon->OffsetWindow(-ScreenX, 0);
    HideWindow();
    if (Icon)
    {
        Icon->ShowWindow();
        Icon->SetFocus();
    }
}
NW_HOOK_METHOD(0x10035db0, NCBoardWnd, Minimize)

// 0x10039dd0 (vslot 37) - C3: the viewer (633 x 474 at (8, 51)), a small button (sysstring 380) and the tab control
// with seven tabs (the sysstrings 377, 379, 381, 382 and 403, and the two ids 905 and 904 of the last two)
INT NCBoardWnd::OnCreate()
{
    NCWnd::OnCreate();
    NRectF Rect = {0, 0, 0.0f, 0.0f};
    GetLocalRect(&Rect);
    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameID = NID_BOARD_FRAME;
    FrameCtrl->ID = NID_BOARD_FRAME;
    FrameCtrl->SetPosition(0, 0, (DWORD)(INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI_CH3.BoardWnd.board_back1"));
    BackTex2 = FindTexture(TEXT("L2UI_CH3.BoardWnd.board_back2"));

    HtmlViewer = new NCBBSHtmlViewer();
    HtmlViewer->SetPosition(8, 51, 633, 474);
    CreateChild(HtmlViewer, NWS_VISIBLE);
    HtmlViewer->SetOwner(Entry);

    Button = new NCButton(TEXT("L2UI_CH3.Button.SmallButton2"), TEXT("L2UI_CH3.Button.SmallButton2_down"), 0);
    Button->ID = NID_BOARD_BUTTON;
    Button->SetPosition(574, 25, 66, 21);
    CreateChild(Button, NWS_VISIBLE);
    Button->SetTextID(380);

    TabCtrl = new NCTabControl();
    TabCtrl->ID = NID_BOARD_TABS;
    TabCtrl->SetPosition(12, 25, 538, 23);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(0x179, HtmlViewer, 74, 23, TEXT("L2UI_CH3.BoardWnd.Board_tab2"),
                    TEXT("L2UI_CH3.BoardWnd.Board_tab1"), 1, -1);
    TabCtrl->AddTab(0x17B, HtmlViewer, 74, 23, TEXT("L2UI_CH3.BoardWnd.Board_tab2"),
                    TEXT("L2UI_CH3.BoardWnd.Board_tab1"), 8, -1);
    TabCtrl->AddTab(0x17D, HtmlViewer, 74, 23, TEXT("L2UI_CH3.BoardWnd.Board_tab2"),
                    TEXT("L2UI_CH3.BoardWnd.Board_tab1"), 1, -1);
    TabCtrl->AddTab(0x17E, HtmlViewer, 74, 23, TEXT("L2UI_CH3.BoardWnd.Board_tab2"),
                    TEXT("L2UI_CH3.BoardWnd.Board_tab1"), 8, -1);
    TabCtrl->AddTab(0x193, HtmlViewer, 74, 23, TEXT("L2UI_CH3.BoardWnd.Board_tab2"),
                    TEXT("L2UI_CH3.BoardWnd.Board_tab1"), 1, -1);
    TabCtrl->AddTab(0x389, HtmlViewer, 74, 23, TEXT("L2UI_CH3.BoardWnd.Board_tab2"),
                    TEXT("L2UI_CH3.BoardWnd.Board_tab1"), 1, -1);
    TabCtrl->AddTab(0x388, HtmlViewer, 74, 23, TEXT("L2UI_CH3.BoardWnd.Board_tab2"),
                    TEXT("L2UI_CH3.BoardWnd.Board_tab1"), 0, -1);
    return 1;
}
NW_HOOK_METHOD(0x10039dd0, NCBoardWnd, OnCreate)

// 0x10036dc0 (vslot 38) - the viewer, the button (disabled first) and the tab control go
INT NCBoardWnd::OnDestroy()
{
    guard(NCBoardWnd::OnDestroy);
    if (HtmlViewer)
    {
        RemoveChildWindow(HtmlViewer->Entry);
        delete HtmlViewer;
        HtmlViewer = NULL;
    }
    Button->EnableWindow(1);
    if (Button)
    {
        RemoveChildWindow(Button->Entry);
        delete Button;
        Button = NULL;
    }
    if (TabCtrl)
    {
        RemoveChildWindow(TabCtrl->Entry);
        delete TabCtrl;
        TabCtrl = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10036dc0, NCBoardWnd, OnDestroy)

// 0x100aa860 (vslot 44) - shared with NCSystemMenuWnd's: shown again from its minimized icon
void NCBoardWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Icon)
    {
        SetPosition(Icon->ScreenX, Icon->ScreenY, (DWORD)(INT)Width, (DWORD)(INT)Height);
        Icon->SetPosition(0, 0, 0, 0);
        Icon->HideWindow();
    }
}
// the hook of 0x100aa860 is NCSystemMenuWnd::ShowWindow's (one stock body for both classes: a second hook is a duplicate)

// 0x10036ed0 (vslot 56) - the board's two backgrounds: a 28 pixel strip 20 below the top and the page's 482 pixel
// panel 48 below it, both as wide as the window. (The stock guard() string says NCNPCHtmlViewer::OnPaint: the original
// author copied the function; the body is the board's own, with the two textures at +0xCC and +0xD0)
INT NCBoardWnd::OnPaint(NCGDevice* Device)
{
    guard(NCNPCHtmlViewer::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 20, (INT)Width, 28, 0, 0, (INT)Width, 28, BackTex, 0xFF, 1);
    if (BackTex2)
        Device->DrawTexture(0, 48, (INT)Width, 482, 0, 0, (INT)Width, 482, BackTex2, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10036ed0, NCBoardWnd, OnPaint)

NW_CHECK_VTABLE(NCBoardWnd, 0x1017f920, 70)
