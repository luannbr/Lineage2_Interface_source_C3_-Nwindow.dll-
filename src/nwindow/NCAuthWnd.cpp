/*=============================================================================
    NCAuthWnd.cpp - the login box. Stock: 0x10082810 .. 0x10082a8f (0x100828b0 included),
    0x10083030 .. 0x10083893 (NCLoginWnd's functions sit in between). OnCancel and
    OnPasswordNotify are bodies the linker folded with other classes' (0x1004b820, 0x10088c40).
=============================================================================*/
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIME.h"
#include "nwindow/NCIMEManager.h"
#include "nwindow/NConsoleWnd.h"

// NWM_COMMAND ids of the box's controls, timer id (names ours)
enum
{
    NID_AUTH_OK = 0x1006,
    NID_AUTH_CANCEL = 0x1007,
    NID_AUTH_ACCOUNT = 0x1502,
    NID_AUTH_PASSWORD = 0x1503,
    NID_AUTH_FRAME = (INT)0xAFF12010,
    TIMER_AUTH_RETRY = (INT)0xAFF12013,   // 1 s (C1: 3 s): the next Log In may ask again
};

// systemmsg lines (names ours)
enum
{
    MSG_EnterAccount = 86,     // "Please insert your ID."
    MSG_EnterPassword = 87,    // "Please insert your password."
    MSG_CannotConnect = 90,    // "Unable to connect to server."
    MSG_EnterCardKey = 1236,   // (C2) the card key line is empty
    MSG_CardKeyPrompt = 1237,  // (C2) the prompt of the card key: its argument is the position asked
};

// sysstring lines (names ours): the labels of the two lines
enum
{
    STR_Account = 150,         // "ID"
    STR_Password = 151,        // "PWD"
};

// interface sounds (NCObject::PlayInterfaceSound)
enum { SOUND_Click = 0 };

// 0x10093180 - the timer result is left as allocated
NCAuthWnd::NCAuthWnd()
{
    Unknown94 = 0;
    BackTex = NULL;
    AccountEdit = NULL;
    PasswordEdit = NULL;
    OKButton = NULL;
    CancelButton = NULL;
    bRequested = 0;
    Mode = 0;
    CardPos = 0;
    CardID = 0;
}
NW_HOOK_CTOR(0x10093180, NCAuthWnd)

// 0x10093200
NW_HOOK_DTOR(0x10093200, NCAuthWnd)

// 0x10091ff0 (vslot 25) - Log In, Exit, Enter in the password line
INT NCAuthWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_AUTH_OK)
            return OnOK() != 0;
        if (wParam == NID_AUTH_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_AUTH_PASSWORD)
            return OnPasswordNotify((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10091ff0, NCAuthWnd, WindowProc)

// 0x10092270 (vslot 36)
INT NCAuthWnd::OnTimer(INT TimerID)
{
    if (TimerID == TIMER_AUTH_RETRY)
        bRequested = 0;
    return 1;
}
NW_HOOK_METHOD(0x10092270, NCAuthWnd, OnTimer)

// 0x10092d50 (vslot 37) - the background, the timer, the account and password lines and the two buttons (C3: no title
// bar, other textures and places, no IME status); TAB cycles account, password, Log In, Exit and back (the account
// line is its own predecessor, as in the stock code)
INT NCAuthWnd::OnCreate()
{
    NCWnd::OnCreate();

    BackTex = FindTexture(TEXT("L2UI_CH3.LoginWnd.logon_back"));
    TimerResult = SetTimer(TIMER_AUTH_RETRY, 1000, NULL);

    AccountEdit = new NCEditBox(6, (const TCHAR*)NULL, 1, 128, 1);
    AccountEdit->ID = NID_AUTH_ACCOUNT;
    AccountEdit->SetPosition(84, 20, 128, 17);
    CreateChild(AccountEdit, NWS_VISIBLE);

    PasswordEdit = new NCEditBox(2, (const TCHAR*)NULL, 1, 128, 1);
    PasswordEdit->ID = NID_AUTH_PASSWORD;
    PasswordEdit->SetPosition(84, 42, 128, 17);
    CreateChild(PasswordEdit, NWS_VISIBLE);

    OKButton = new NCButton(TEXT("L2UI_CH3.BUTTON.bigbutton"),
                                  TEXT("L2UI_CH3.BUTTON.bigbutton_down"), 0);
    OKButton->ID = NID_AUTH_OK;
    OKButton->SetPosition(31, 72, 96, 23);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(152);        // "Log In"

    CancelButton = new NCButton(TEXT("L2UI_CH3.BUTTON.bigbutton"),
                                      TEXT("L2UI_CH3.BUTTON.bigbutton_down"), 0);
    CancelButton->ID = NID_AUTH_CANCEL;
    CancelButton->SetPosition(131, 72, 96, 23);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(153);    // "Exit"

    SetTabOrder(AccountEdit->Entry, AccountEdit->Entry);
    AccountEdit->SetTabOrder(PasswordEdit->Entry, CancelButton->Entry);
    PasswordEdit->SetTabOrder(OKButton->Entry, AccountEdit->Entry);
    OKButton->SetTabOrder(CancelButton->Entry, PasswordEdit->Entry);
    CancelButton->SetTabOrder(AccountEdit->Entry, OKButton->Entry);
    return 1;
}
NW_HOOK_METHOD(0x10092d50, NCAuthWnd, OnCreate)

// 0x10092820 (vslot 38)
INT NCAuthWnd::OnDestroy()
{
    if (AccountEdit)
    {
        RemoveChildWindow(AccountEdit->Entry);
        delete AccountEdit;
        AccountEdit = NULL;
    }
    if (PasswordEdit)
    {
        RemoveChildWindow(PasswordEdit->Entry);
        delete PasswordEdit;
        PasswordEdit = NULL;
    }
    if (OKButton)
    {
        RemoveChildWindow(OKButton->Entry);
        delete OKButton;
        OKButton = NULL;
    }
    if (CancelButton)
    {
        RemoveChildWindow(CancelButton->Entry);
        delete CancelButton;
        CancelButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10092820, NCAuthWnd, OnDestroy)

// 0x10092230 (vslot 44) - a new try: the account line (the card key line in Mode 1) gets the focus
void NCAuthWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Mode == 0)
        AccountEdit->SetFocus();
    else
    {
        AccountEdit->HideWindow();
        PasswordEdit->ClearText();
        PasswordEdit->SetFocus();
    }
}
NW_HOOK_METHOD(0x10092230, NCAuthWnd, ShowWindow)

// 0x100925f0 (vslot 56) - the background over the whole window; the clip goes on before BeginPaint. The labels are
// right-aligned at x = 78; Mode 1 draws the card key prompt instead
INT NCAuthWnd::OnPaint(NCGDevice* Device)
{
    guard(NCAuthWnd::OnPaint);
    TCHAR Prompt[516];
    SIZE Size;
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, (INT)Width, (INT)Height, BackTex);
    if (Mode == 0)
    {
        if (*GL2GameData.SysString[STR_Account])
        {
            NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_Account], 0, 0, 0, 0, 0, L2FONT_Normal);
            Device->DrawText(78 - Size.cx, 23, 0xFFDCDCDC, *GL2GameData.SysString[STR_Account], 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        }
        if (*GL2GameData.SysString[STR_Password])
        {
            NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_Password], 0, 0, 0, 0, 0, L2FONT_Normal);
            Device->DrawText(78 - Size.cx, 45, 0xFFDCDCDC, *GL2GameData.SysString[STR_Password], 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        }
    }
    else if (Mode == 1)
    {
        GConsole->GetNumberMessage((INT)MSG_CardKeyPrompt, CardPos, Prompt);
        Device->DrawText(30, 23, 0xFFDCDCDC, Prompt, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100925f0, NCAuthWnd, OnPaint)

// 0x100920a0 (vslot 68) - Log In (Mode 1: the card key goes to RequestAuthCardKeyLogin as a number)
INT NCAuthWnd::OnOK()
{
    if (!Mode)
    {
        if (!AccountEdit->CommitText())
            GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_EnterAccount].Message);
        else if (AccountEdit->CommitText() && !PasswordEdit->CommitText())
            GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_EnterPassword].Message);
        else if (!bRequested)
        {
            if (!GConsole->RequestAuthLogin(AccountEdit->CommitText(), PasswordEdit->CommitText()))
                GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_CannotConnect].Message);
            bRequested = 1;
        }
    }
    else if (!PasswordEdit->CommitText())
        GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_EnterCardKey].Message);
    else if (!bRequested)
    {
        if (!GConsole->RequestAuthCardKeyLogin(CardID, appAtoi(PasswordEdit->CommitText())))
            GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_CannotConnect].Message);
        bRequested = 1;
    }
    GConsole->PlayInterfaceSound(SOUND_Click);
    return 1;
}
NW_HOOK_METHOD(0x100920a0, NCAuthWnd, OnOK)

// 0x10092210 (vslot 69, and 71 in the C3 vtable) - Exit. The stock body is shared by every Exit slot (the console's
// OnQuit stub in NCConsole.cpp jumps to it): it is hooked here, once, for all of them
INT NCAuthWnd::OnCancel()
{
    appRequestExit(0);
    return 1;
}
NW_HOOK_METHOD(0x10092210, NCAuthWnd, OnCancel)


// 0x100b2360 (vslot 70) - Enter in the password line (code 1) logs in (a body shared with other windows': it
// calls virtual slot 68 of whatever it is called on)
INT NCAuthWnd::OnPasswordNotify(INT Code)
{
    if (Code == 1)
        OnOK();
    return 1;
}
NW_HOOK_METHOD(0x100b2360, NCAuthWnd, OnPasswordNotify)

// 0x10092090 - after a failed login (NCLoginWnd::ResetWindows)
void NCAuthWnd::ClearPassword()
{
    PasswordEdit->ClearText();
}
NW_HOOK_METHOD(0x10092090, NCAuthWnd, ClearPassword)

// (C2 only: the box started over with the account and password lines; nothing in this client calls it)
void NCAuthWnd::ResetInput()
{
    Mode = 0;
    CardID = 0;
    CardPos = 0;
    AccountEdit->ShowWindow();
    AccountEdit->SetFocus();
}

// 0x10062830 - the server asks for the security card key (NConsoleWnd::SCCheckStart): the box goes to Mode 1 with the
// card's id and the position it asks for, the account line is hidden and the password line cleared and focused
void NCAuthWnd::SetCardKey(INT InCardID, INT InCardPos)
{
    CardPos = InCardPos;
    CardID = InCardID;
    Mode = 1;
    AccountEdit->HideWindow();
    PasswordEdit->ClearText();
    PasswordEdit->SetFocus();
}
NW_HOOK_METHOD(0x10062830, NCAuthWnd, SetCardKey)

NW_CHECK_VTABLE(NCAuthWnd, 0x1018bc20, 71)
