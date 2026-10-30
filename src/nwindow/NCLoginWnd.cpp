/*=============================================================================
    NCLoginWnd.cpp - the login screen. Stock: 0x10082a90 .. 0x10082cdd (0x10082c50 is a body
    the linker folded with NCSystemMenuWnd's), 0x100838c0 .. 0x1008462f (NCAuthWnd's functions sit in
    between).
=============================================================================*/
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCAnnounceWnd.h"
#include "nwindow/NCEulaWnd.h"
#include "nwindow/NCKeyCryptWarningDialogBox.h"
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NCServerSelectWnd.h"
#include "nwindow/NKeyCrypt.h"
#include "nwindow/NCWarningWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCreditWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x10093250 - only the logo is cleared; the rest is left as allocated (the stock reads the layout of the boxes it makes
// itself)
NCLoginWnd::NCLoginWnd()
{
    LogoTex = NULL;
}
NW_HOOK_CTOR(0x10093250, NCLoginWnd)

// 0x100932a0
NW_HOOK_DTOR(0x100932a0, NCLoginWnd)

// (slot 25, WindowProc, is NCWnd's own body, 0x1002fa20: the stock vtable of this class points at it, so the class has no
// override)

// 0x100932f0 (vslot 37) - the logos by the country's names in localization.ini, the boxes centered on the window (the
// sizes halved as floats: C3), the server window and the function (buttons) window in the lower right corner
INT NCLoginWnd::OnCreate()
{
    NCWnd::OnCreate();
    FString LogoName;
    TCHAR Key[192];
    appSprintf(Key, TEXT("%s_LogoTexture"), GCountryName);
    if (!GConfig->GetString(GCountryName, Key, LogoName, TEXT("localization.ini")))
        LogoName = TEXT("L2Font.start_logo-k");
    LogoTex = FindTexture(*LogoName);
    appSprintf(Key, TEXT("%s_MiniLogoTexture"), GCountryName);
    if (!GConfig->GetString(GCountryName, Key, LogoName, TEXT("localization.ini")))
        LogoName = TEXT("L2Font.start_minilogo-k");
    MiniLogoTex = FindTexture(*LogoName);

    AuthWnd = new NCAuthWnd;
    AuthWnd->SetPosition((INT)(Width * 0.5f - 128.0f), (INT)(Height * 0.5f - 54.0f), 256, 108);
    CreateChild(AuthWnd, NWS_NOCLOSE | NWS_VISIBLE);
    AuthWnd->SetTitle(*GL2GameData.SysString[149]);            // "Log In"
    AuthWnd->SetAnchor(5, 0, 0);

    WarningWnd = new NCWarningWnd;
    WarningWnd->SetPosition((INT)(Width * 0.5f - 273.0f), (INT)(Height * 0.5f - 212.0f), 546, 425);
    CreateChild(WarningWnd, NWS_NOFOCUSSEARCH | NWS_NOCLOSE | NWS_VISIBLE);
    WarningWnd->SetTitle(*GL2GameData.SysString[538]);         // "Warning"
    WarningWnd->SetAnchor(5, 0, 0);

    EulaWnd = new NCEulaWnd;
    EulaWnd->SetPosition((INT)(Width * 0.5f - 273.0f), (INT)(Height * 0.5f - 212.0f), 546, 425);
    CreateChild(EulaWnd, NWS_NOFOCUSSEARCH | NWS_NOCLOSE | NWS_VISIBLE);
    EulaWnd->SetTitle(*GL2GameData.SysString[598]);            // the EULA's title
    EulaWnd->SetAnchor(5, 0, 0);

    AnnounceWnd = new NCAnnounceWnd;
    AnnounceWnd->SetPosition((INT)(Width * 0.5f - 273.0f), (INT)(Height * 0.5f - 212.0f), 546, 425);
    CreateChild(AnnounceWnd, NWS_NOFOCUSSEARCH | NWS_NOCLOSE | NWS_VISIBLE);
    AnnounceWnd->SetAnchor(5, 0, 0);

    // NCLoginServerWnd and NCLoginFunctionWnd (C3: new classes; the stock makes each inline: NCWnd's constructor and
    // the vtable)
    ServerWnd = new NCLoginServerWnd;
    ServerWnd->SetPosition(GSEKScreenX / 2 - 158, GSEKScreenY / 2 - 271, 316, 542);
    CreateChild(ServerWnd, NWS_VISIBLE);
    ServerWnd->SetAnchor(5, 0, 0);

    FunctionWnd = new NCLoginFunctionWnd;
    FunctionWnd->SetPosition(GSEKScreenX - 154, GSEKScreenY - 200, 120, 138);
    CreateChild(FunctionWnd, NWS_VISIBLE);
    FunctionWnd->SetAnchor(9, -34, -10);

    KeyCryptWnd = new NCKeyCryptWarningDialogBox;
    KeyCryptWnd->SetPosition(GSEKScreenX / 2 - 101, GSEKScreenY / 2 - 43, 205, 87);
    CreateChild(KeyCryptWnd, NWS_VISIBLE);
    KeyCryptWnd->HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100932f0, NCLoginWnd, OnCreate)

// 0x10092950 (vslot 38)
INT NCLoginWnd::OnDestroy()
{
    if (AuthWnd)
    {
        RemoveChildWindow(AuthWnd->Entry);
        delete AuthWnd;
        AuthWnd = NULL;
    }
    if (WarningWnd)
    {
        RemoveChildWindow(WarningWnd->Entry);
        delete WarningWnd;
        WarningWnd = NULL;
    }
    if (EulaWnd)
    {
        RemoveChildWindow(EulaWnd->Entry);
        delete EulaWnd;
        EulaWnd = NULL;
    }
    if (AnnounceWnd)
    {
        RemoveChildWindow(AnnounceWnd->Entry);
        delete AnnounceWnd;
        AnnounceWnd = NULL;
    }
    if (FunctionWnd)
    {
        RemoveChildWindow(FunctionWnd->Entry);
        delete FunctionWnd;
        FunctionWnd = NULL;
    }
    if (ServerWnd)
    {
        RemoveChildWindow(ServerWnd->Entry);
        delete ServerWnd;
        ServerWnd = NULL;
    }
    if (KeyCryptWnd)
    {
        RemoveChildWindow(KeyCryptWnd->Entry);
        delete KeyCryptWnd;
        KeyCryptWnd = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10092950, NCLoginWnd, OnDestroy)

// 0x10092290 (vslot 44) - only the login box stays up; the key crypt box is shown (and takes the focus) while the key
// crypt module (GL2UseKeyCrypt) is on and not yet registered. The module is at NConsoleWnd +0x39C0 here (its header
// has it 4 bytes further)
void NCLoginWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    WarningWnd->HideWindow();
    EulaWnd->HideWindow();
    AnnounceWnd->HideWindow();
    ServerWnd->HideWindow();
    if (GL2UseKeyCrypt)
    {
        NKeyCrypt* Crypt = *(NKeyCrypt**)((BYTE*)GConsole + 0x39C0);
        if (!Crypt || !Crypt->Open() || !(*(NKeyCrypt**)((BYTE*)GConsole + 0x39C0))->RegisterMessage())
        {
            KeyCryptWnd->DoModal();                                  // NCDialogBox::DoModal, 0x10007470
            KeyCryptWnd->SetFocus();
        }
        else
            KeyCryptWnd->HideWindow();
    }
    else
        KeyCryptWnd->HideWindow();
}
NW_HOOK_METHOD(0x10092290, NCLoginWnd, ShowWindow)

// 0x10092330 (vslot 45) - with the key crypt module on, the driver is closed along with the screen
// (NKeyCrypt::Close on the object at NConsoleWnd +0x39C0)
void NCLoginWnd::HideWindow()
{
    NCWnd::HideWindow();
    if (GL2UseKeyCrypt)
    {
        NKeyCrypt* Crypt = *(NKeyCrypt**)((BYTE*)GConsole + 0x39C0);
        if (Crypt)
            Crypt->Close();
    }
}
NW_HOOK_METHOD(0x10092330, NCLoginWnd, HideWindow)

// 0x10092b00 (vslot 56) - the small logo in the upper left corner (232 x 120), the big one centered at a twentieth of
// the height (neither is checked for NULL)
INT NCLoginWnd::OnPaint(NCGDevice* Device)
{
    guard(NCLoginWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, 232, 120, 0, 0, 232, 120, MiniLogoTex);
    Device->DrawTexture((INT)(Width * 0.5f - 128.0f), (INT)(Height * 0.05f), 256, 256, 0, 0, 256, 256, LogoTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10092b00, NCLoginWnd, OnPaint)

// (C2 only: "Options" and "Credits" were buttons of this window; C3 keeps the buttons in NCLoginFunctionWnd)

// 0x10092360 - back to an empty login screen (failed login, server list cancelled...): the password, the warning and
// EULA texts and the server list
void NCLoginWnd::ResetWindows()
{
    AuthWnd->ClearPassword();
    WarningWnd->ClearText();                                          // 0x100b0b40
    EulaWnd->ClearText();                                             // 0x100b0ca0
    ((NCLoginServerWnd*)ServerWnd)->ServerSelectWnd->ClearServers();   // 0x100ed760
}
NW_HOOK_METHOD(0x10092360, NCLoginWnd, ResetWindows)

NW_CHECK_VTABLE(NCLoginWnd, 0x1018bd40, 68)

/*-----------------------------------------------------------------------------
    NCLoginFunctionWnd (C3): the four buttons of the lower right corner.
-----------------------------------------------------------------------------*/

// 0x10091a30 - NCWnd's constructor and the vtable (the stock one returns this)
NCLoginFunctionWnd::NCLoginFunctionWnd()
{
}
NW_HOOK_CTOR(0x10091a30, NCLoginFunctionWnd)

// 0x10091a80
NW_HOOK_DTOR(0x10091a80, NCLoginFunctionWnd)

// 0x10091190 (vslot 25) - the buttons' ids go to the four handlers (virtual slots 68, 71, 69 and 70)
INT NCLoginFunctionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_LOGIN_NEWACCOUNT)
            return OnNewAccountBtnClick() != 0;
        if (wParam == NID_LOGIN_LOSTACCOUNT)
            return OnLostAccountBtnClick() != 0;
        if (wParam == NID_LOGIN_OPTIONS)
            return OnOptionsBtnClick() != 0;
        if (wParam == NID_LOGIN_CREDITS)
            return OnCreditsBtnClick() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10091190, NCLoginFunctionWnd, WindowProc)

// 0x100916e0 (vslot 37) - the backdrop and the four buttons, 96 x 23 each, one under the other (NCWnd::OnCreate is
// called)
INT NCLoginFunctionWnd::OnCreate()
{
    guard(NCLoginFunctionWnd::OnCreate);
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.LobbyWnd.lobby_menuback"));

    NewAccountButton = new NCButton(TEXT("L2UI_CH3.Button.BigButton"),
                                          TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    NewAccountButton->ID = NID_LOGIN_NEWACCOUNT;
    NewAccountButton->SetPosition(13, 20, 96, 23);
    CreateChild(NewAccountButton, NWS_VISIBLE);
    NewAccountButton->SetTextID(154);

    LostAccountButton = new NCButton(TEXT("L2UI_CH3.Button.BigButton"),
                                           TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    LostAccountButton->ID = NID_LOGIN_LOSTACCOUNT;
    LostAccountButton->SetPosition(13, 45, 96, 23);
    CreateChild(LostAccountButton, NWS_VISIBLE);
    LostAccountButton->SetTextID(155);

    OptionsButton = new NCButton(TEXT("L2UI_CH3.Button.BigButton"),
                                       TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    OptionsButton->ID = NID_LOGIN_OPTIONS;
    OptionsButton->SetPosition(13, 70, 96, 23);
    CreateChild(OptionsButton, NWS_VISIBLE);
    OptionsButton->SetTextID(156);

    CreditsButton = new NCButton(TEXT("L2UI_CH3.Button.BigButton"),
                                       TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    CreditsButton->ID = NID_LOGIN_CREDITS;
    CreditsButton->SetPosition(13, 95, 96, 23);
    CreateChild(CreditsButton, NWS_VISIBLE);
    CreditsButton->SetTextID(805);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100916e0, NCLoginFunctionWnd, OnCreate)

// 0x100915b0 (vslot 38) - the four buttons go, then NCWnd's OnDestroy
INT NCLoginFunctionWnd::OnDestroy()
{
    guard(NCLoginFunctionWnd::OnDestroy);
    if (NewAccountButton)
    {
        RemoveChildWindow(NewAccountButton->Entry);
        delete NewAccountButton;
        NewAccountButton = NULL;
    }
    if (LostAccountButton)
    {
        RemoveChildWindow(LostAccountButton->Entry);
        delete LostAccountButton;
        LostAccountButton = NULL;
    }
    if (OptionsButton)
    {
        RemoveChildWindow(OptionsButton->Entry);
        delete OptionsButton;
        OptionsButton = NULL;
    }
    if (CreditsButton)
    {
        RemoveChildWindow(CreditsButton->Entry);
        delete CreditsButton;
        CreditsButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100915b0, NCLoginFunctionWnd, OnDestroy)

// 0x100914c0 (vslot 56) - the backdrop (a 120 x 138 piece) stretched over the window
INT NCLoginFunctionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCLoginFunctionWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, 120, 138, 0, 0, 120, 138, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100914c0, NCLoginFunctionWnd, OnPaint)

// 0x10091240 (vslot 68) - the message window shows systemmsg 1186 (where to make an account)
INT NCLoginFunctionWnd::OnNewAccountBtnClick()
{
    guard(NCLoginFunctionWnd::OnNewAccountBtnClick);
    GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[1186].Message);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10091240, NCLoginFunctionWnd, OnNewAccountBtnClick)

// 0x100912d0 (vslot 71) - systemmsg 1187 (what to do about a lost account)
INT NCLoginFunctionWnd::OnLostAccountBtnClick()
{
    guard(NCLoginFunctionWnd::OnNewAccountBtnClick);
    GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[1187].Message);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100912d0, NCLoginFunctionWnd, OnLostAccountBtnClick)

// 0x10091360 (vslot 69) - the options window
INT NCLoginFunctionWnd::OnOptionsBtnClick()
{
    guard(NCLoginFunctionWnd::OnNewAccountBtnClick);
    GConsole->ToggleOptionWnd();                                    // 0x1005F120

    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10091360, NCLoginFunctionWnd, OnOptionsBtnClick)

// 0x100913e0 (vslot 70) - the credits: the login screen and the window of the god window's +0xD0 (the line under
// the login screens in C2) go away, the credit window takes the screen and the console's slot 324 follows
INT NCLoginFunctionWnd::OnCreditsBtnClick()
{
    guard(NCLoginFunctionWnd::OnNewAccountBtnClick);
    GConsole->LoginWnd->HideWindow();
    (*(NCWnd**)((BYTE*)GConsole->GodWnd + 0xD0))->HideWindow();
    GConsole->CreditWnd->ShowWindow();
    ((NCCreditWnd*)GConsole->CreditWnd)->Start();
    GConsole->CreditWnd->SetFocus();
    ::nw::VirtualCall<void>(GConsole, 324);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100913e0, NCLoginFunctionWnd, OnCreditsBtnClick)

NW_CHECK_VTABLE(NCLoginFunctionWnd, 0x1018b4a8, 72)

/*-----------------------------------------------------------------------------
    NCLoginServerWnd (C3): the server list and its help page.
-----------------------------------------------------------------------------*/

// 0x10091ce0 - NCWnd's constructor and the vtable (the stock one returns this)
NCLoginServerWnd::NCLoginServerWnd()
{
}
NW_HOOK_CTOR(0x10091ce0, NCLoginServerWnd)

// 0x10091d30
NW_HOOK_DTOR(0x10091d30, NCLoginServerWnd)

// 0x10091d80 (vslot 37) - the list (a 316 x 330 window of style 0x200002, titled) over the help page (316 x 205 at
// (0, 337)); NCWnd::OnCreate is not called. The page's constructor is NCWnd's, inline, and the vtable
INT NCLoginServerWnd::OnCreate()
{
    guard(NCLoginServerWnd::OnCreate);
    ServerSelectWnd = new NCServerSelectWnd;
    ServerSelectWnd->SetPosition(0, 0, 316, 330);
    CreateChild(ServerSelectWnd, NWS_NOFOCUSSEARCH | NWS_VISIBLE);
    ServerSelectWnd->SetTitle(*GL2GameData.SysString[449]);

    ServerInfoWnd = new NCServerInfoWnd;
    ServerInfoWnd->SetPosition(0, 337, 316, 205);
    CreateChild(ServerInfoWnd, NWS_VISIBLE);
    ServerInfoWnd->SetTitle(*GL2GameData.SysString[857]);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10091d80, NCLoginServerWnd, OnCreate)

// 0x10091b70 (vslot 38) - the list and the page go; NCWnd's OnDestroy is not called
INT NCLoginServerWnd::OnDestroy()
{
    guard(NCLoginServerWnd::OnDestroy);
    if (ServerSelectWnd)
    {
        RemoveChildWindow(ServerSelectWnd->Entry);
        delete ServerSelectWnd;
        ServerSelectWnd = NULL;
    }
    if (ServerInfoWnd)
    {
        RemoveChildWindow(ServerInfoWnd->Entry);
        delete ServerInfoWnd;
        ServerInfoWnd = NULL;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10091b70, NCLoginServerWnd, OnDestroy)

// 0x10091ae0 (ret 4) - the window is shown, the list learns the server played last (the stock body is NCServerSelectWnd's
// SetLastServerID, shared with other classes' setters) and takes the focus
void NCLoginServerWnd::ReceiveServerList(INT LastServerID)
{
    guard(NCLoginServerWnd::ReceiveServerList);
    ShowWindow();
    ServerSelectWnd->LastServerID = LastServerID;   // the stock calls 0x100dec90: [this+0xDC] = arg, ret 4 (NCRecipeBuyListWnd::SetShopOwner's body)
    ServerSelectWnd->SetFocus();
    unguard;
}
NW_HOOK_METHOD(0x10091ae0, NCLoginServerWnd, ReceiveServerList)

NW_CHECK_VTABLE(NCLoginServerWnd, 0x1018b780, 68)
