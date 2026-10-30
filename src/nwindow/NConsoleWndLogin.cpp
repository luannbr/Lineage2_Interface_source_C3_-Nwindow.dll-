/*=============================================================================
    NConsoleWndLogin.cpp - what the engine tells the console before the game and around it
    (UGameEngine::On* handlers, one slot each): the login and its failures, the announcements,
    the server list, the character selection and creation, entering the world, the restart,
    the logout, a closed server, a blocked account, the protocol version.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleWndExt.h"
#include "nwindow/NCGodWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCEulaWnd.h"
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NCWarningWnd.h"
#include "nwindow/NCServerSelectWnd.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCPawnCreateWnd.h"
#include "nwindow/NCLoadingWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCRestartMenuWnd.h"
// (blank lines kept on purpose: the check() __LINE__ values below stay those of the stock build)




// system messages (systemmsg ids)
enum
{
    MSG_Disconnected = 0,              // "Your connection has been disconnected from the server."
    MSG_TooManyCharacters = 77,        // "You cannot create another character. Please delete the
                                       //   existing character and try again."
    MSG_NameExists = 79,               // "Name already exists."
    MSG_NameInvalid = 80,              // "Please insert 1~16 English characters."
    MSG_ProtocolQuit = 88,             // "Protocol version is different. Please quit the program."
    MSG_ProtocolContinue = 89,         // "Protocol version is different. Please continue."
    MSG_DisconnectedRetry = 127,       // "Your connection has been disconnected from the server.
                                       //   Please try again."
    MSG_CreateFailed = 128,            // "You have failed to create a character."
    MSG_DeleteFailed = 306,            // "You have failed to delete the character."
    MSG_AskRevive = 332,               // "Do you want to be restored?"
    MSG_TemporaryPassword = 396,       // "Please login after changing your temporary password."
    MSG_NoTimeLeft = 398,              // "You have no more time left on your account."
    MSG_SystemError = 399,             // "System error."
    MSG_BlockReason = 428,             // .. 443: a reason each, a bit of BlockedAccount's
    MSG_Withdrawn = 444,               // "The following account may not be accessed due to a
                                       //   withdrawal request that has been submitted." and
                                       //   445..447 after it
    MSG_LoginSystemError = 448,        // "System error, please log in again later."
    MSG_WrongPassword = 449,           // "Password does not match this acount."
    MSG_ConfirmAccount = 450,          // "Confirm your account information and log in again later."
    MSG_WrongAccount = 453,            // "Your account information is incorrect."
    MSG_ContactSupport = 454,          // "Please contact the Customer Support Center or the 1: 1
                                       //   inquiry service of the Lineage II official website."
    MSG_AccountInUse = 455,            // "The account is already in use. Access denied."
    MSG_AdultsOnly = 456,              // "Lineage II game services may only be used by adults 18
                                       //   years of age and older."
    MSG_Maintenance = 457,             // "Server under maintenance. Please try again later."
    MSG_TermExpired = 458,             // "Your usage term has been expired." 459, 460 after it
    MSG_AccessFailed = 461,            // "Access failed." 462, 463 after it
    MSG_ClanMemberDelete = 540,        // "Clan leaders cannot be deleted. DD Dissolve the clan and
                                       //   try again."
    MSG_ClanLeaderDelete = 541,        // "You cannot delete a clan member. Withdraw from the clan
                                       //   and try again."
    MSG_KoreaOnly = 621,               // "This server is reserved for players in Korea..."
    MSG_WeekTimeOver = 756,            // "This week's usage time has finished." (no reason of C2's AuthLoginFail has it)
    MSG_LoginReason31 = 1243,          // C2: the reasons 31 and 32 of AuthLoginFail (the security card's;
    MSG_LoginReason32 = 1242,          //   the texts are not known here)
    MSG_LoginReason33 = 1340,          // C3: the reasons 33 and 35 of AuthLoginFail (texts not read here)
    MSG_LoginReason35 = 1407,
};

// the dialog boxes opened here (NDialogInfo::ID; NCDialogBox sends the answer)
enum
{
    DIALOG_DISCONNECTED = 0x1D,        // OK only: the game ends
    DIALOG_REVIVE = 0x20,
};

// a system message's text
static FORCEINLINE const TCHAR* SysMsg(INT ID)
{
    return *GL2GameData.SystemMsg[ID].Message;
}

// the server list of the login screen, C3: the container at +0xD8, its child at +0xC8
#define NLOGIN_SERVERLIST_C3(Login) (nw::Field<NCServerSelectWnd*>(nw::Field<void*>((Login), 0xD8), 0xC8))

// the login screen's windows by their C3 offsets (names ours; NCLoginWnd.h still has the C2 order)
static FORCEINLINE NCWnd* LoginChild(NConsoleWnd* Console, DWORD Offset)
{
    return nw::Field<NCWnd*>(Console->LoginWnd, Offset);
}

// the login box's mode, card and card key cleared, and the account line shown and given the focus (C3: the box is the
// login window's child at +0xC8, its fields +0xC8 / +0xCC / +0xD0, the account line +0xDC)
static FORCEINLINE void ResetAuthWnd(NConsoleWnd* Console)
{
    BYTE* Auth = nw::Field<BYTE*>(Console->LoginWnd, 0xC8);
    nw::Field<INT>(Auth, 0xC8) = 0;
    nw::Field<INT>(Auth, 0xD0) = 0;
    nw::Field<INT>(Auth, 0xCC) = 0;
    nw::Field<NCWnd*>(Auth, 0xDC)->ShowWindow();
    nw::Field<NCWnd*>(Auth, 0xDC)->SetFocus();
}

// back to the login box, the server list and the other boxes of the login screen hidden (after the login failures);
// bReset: the box starts again (account mode)
static FORCEINLINE void ShowAuthWnd(NConsoleWnd* Console, UBOOL bReset = 1)
{
    LoginChild(Console, 0xD8)->HideWindow();
    LoginChild(Console, 0xCC)->HideWindow();
    LoginChild(Console, 0xD0)->HideWindow();
    LoginChild(Console, 0xD4)->HideWindow();
    LoginChild(Console, 0xC8)->ShowWindow();
    if (bReset)
        ResetAuthWnd(Console);
    Console->SetState(L2CS_Login);
}

/*-----------------------------------------------------------------------------
    The login.
-----------------------------------------------------------------------------*/

// 0x10062fd0 (vslot 32) - the login server refused: the reason's message, then the login box again (for the reasons
// without a message the stock shows an uninitialized buffer; empty here). C3: the reason 30 (the week's time) and the
// reasons 33 and 35; 31 leaves the box as it is
void NConsoleWnd::AuthLoginFail(INT Reason)
{
    TCHAR Text[256];
    Text[0] = 0;
    switch (Reason)
    {
    case 1:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginSystemError));
        break;
    case 2:
    case 3:
        appSprintf(Text, TEXT("%s %s"), SysMsg(MSG_WrongPassword), SysMsg(MSG_ConfirmAccount));
        break;
    case 5:
        appSprintf(Text, TEXT("%s %s"), SysMsg(MSG_WrongAccount), SysMsg(MSG_ContactSupport));
        break;
    case 7:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_AccountInUse));
        break;
    case 12:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_AdultsOnly));
        break;
    case 16:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_Maintenance));
        break;
    case 17:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_TemporaryPassword));
        break;
    case 18:
        appSprintf(Text, TEXT("%s %s %s"), SysMsg(MSG_TermExpired), SysMsg(MSG_TermExpired + 1),
                   SysMsg(MSG_TermExpired + 2));
        break;
    case 19:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_NoTimeLeft));
        break;
    case 20:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_SystemError));
        break;
    case 21:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_AccessFailed));
        break;
    case 22:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_KoreaOnly));
        break;
    case 30:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_WeekTimeOver));
        break;
    case 31:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginReason31));
        break;
    case 32:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginReason32));
        break;
    case 33:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginReason33));
        break;
    case 35:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginReason35));
        break;
    case 4:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11:
    case 13:
    case 14:
    case 15:
        appSprintf(Text, TEXT("%s %s %s"), SysMsg(MSG_AccessFailed), SysMsg(MSG_AccessFailed + 1),
                   SysMsg(MSG_AccessFailed + 2));
        break;
    }
    ShowMessageWnd(Text);
    ShowAuthWnd(this, Reason != 31);
}
NW_HOOK_METHOD(0x10062fd0, NConsoleWnd, AuthLoginFail)

// 0x10063370 (vslot 33) - after the login: the announcements, if there are any, else straight to the server list; the
// value after them goes to the box. C3 has three kinds, in this order: the text at GL2GameData +0x77D44 in the
// login screen's window at +0xD0 (the test server's text, +0x77D50, when GL2TestServer is set), a Chinese client's
// window at +0xD4 and, when there is no text, the login box alone: all of them leave the login box in account
// mode and hide it and the server list. The value after the announcements goes to the window's +0xD8 / +0xD0 and
// then, as the stock does, to the server list's +0xC8 child (0x100EDAB0)
void NConsoleWnd::StartNotice(L2ParamStack& Params)
{
    if (GL2GameData.NoticeText.Len() > 0)
    {
        ResetAuthWnd(this);
        LoginChild(this, 0xC8)->HideWindow();
        LoginChild(this, 0xD8)->HideWindow();
        LoginChild(this, 0xD0)->ShowWindow();
        LoginChild(this, 0xD0)->SetFocus();
        static_cast<NCEulaWnd*>(LoginChild(this, 0xD0))->ClearText();
        if (GL2TestServer)
            static_cast<NCEulaWnd*>(LoginChild(this, 0xD0))->SetText(*nw::Field<FString>(&GL2GameData, 0x77D50));
        else
            static_cast<NCEulaWnd*>(LoginChild(this, 0xD0))->SetText(*GL2GameData.NoticeText);
        nw::Field<void*>(LoginChild(this, 0xD0), 0xD8) = Params.Top();
    }
    else if (GLanguageType == 4)
    {
        ResetAuthWnd(this);
        LoginChild(this, 0xC8)->HideWindow();
        LoginChild(this, 0xD8)->HideWindow();
        LoginChild(this, 0xD4)->ShowWindow();
        LoginChild(this, 0xD4)->SetFocus();
        nw::Field<void*>(LoginChild(this, 0xD4), 0xD0) = Params.Top();
    }
    else
    {
        ResetAuthWnd(this);
        ((NCLoginServerWndExt*)nw::Field<NCWnd*>(LoginChild(this, 0xD8), 0xC8))->SetKey(Params.Top());
        ::nw::VirtualCall<void>(this, 390);   // RequestServerList: the header's own virtuals sit after the C3-only
                                              // placeholders, a compiled call would use another slot
        return;
    }
    ((NCLoginServerWndExt*)nw::Field<NCWnd*>(LoginChild(this, 0xD8), 0xC8))->SetKey(Params.Top());
    SetState(L2CS_Login);
}
NW_HOOK_METHOD(0x10063370, NConsoleWnd, StartNotice)

// 0x1005b040 (vslot 44) - the server list follows (AddServerList), with the server last played on: the window at
// +0x3950 (C3: NCGodWnd) is told first, then the login screen starts over and its boxes hide, and the list's
// container takes the server (0x10091AE0). The stock has a guard frame
void NConsoleWnd::ReceiveServerList(INT LastServerID)
{
    guard(NConsoleWnd::ReceiveServerList);
    if (GodWnd)
        static_cast<NCGodWnd*>(GodWnd)->ReceiveServerList(LastServerID);
    LoginWnd->ResetWindows();                                               // NCLoginWnd::Clear (0x10092360)
    LoginChild(this, 0xC8)->HideWindow();
    LoginChild(this, 0xCC)->HideWindow();
    LoginChild(this, 0xD0)->HideWindow();
    LoginChild(this, 0xD4)->HideWindow();
    static_cast<NCLoginServerWnd*>(LoginChild(this, 0xD8))->ReceiveServerList(LastServerID);
    unguard;
}
NW_HOOK_METHOD(0x1005b040, NConsoleWnd, ReceiveServerList)

// 0x1005b140 (vslot 45) - a server of the list goes to the server list window (the container's +0xC8 child)
void NConsoleWnd::AddServerList(L2ParamStack& Params)
{
    guard(NConsoleWnd::AddServerList);
    NLOGIN_SERVERLIST_C3(LoginWnd)->AddServer(Params);
    unguard;
}
NW_HOOK_METHOD(0x1005b140, NConsoleWnd, AddServerList)

// 0x1005b1d0 (vslot 46)
void NConsoleWnd::EndServerList()
{
    guard(NConsoleWnd::EndServerList);
    NLOGIN_SERVERLIST_C3(LoginWnd)->OnServerListEnd();
    unguard;
}
NW_HOOK_METHOD(0x1005b1d0, NConsoleWnd, EndServerList)

// 0x10060600 (vslot 330) - the server list window's SetPing (a tail call), when there is one
void NConsoleWnd::SetServerPing(INT ServerID, DWORD Ping)
{
    if (NLOGIN_SERVERLIST_C3(LoginWnd))
        NLOGIN_SERVERLIST_C3(LoginWnd)->SetPing(ServerID, Ping);
}
NW_HOOK_METHOD(0x10060600, NConsoleWnd, SetServerPing)

// 0x10062c40 (vslot 283) - the account is blocked, with the server's text: the message is built from four lines of
// systemmsg and takes the text
void NConsoleWnd::BlockedAccountWithMsg(TCHAR* Msg)
{
    TCHAR Format[256];
    TCHAR Text[256];
    if (Msg && *Msg)
    {
        appSprintf(Format, TEXT("%s %s %s %s"), SysMsg(MSG_Withdrawn), SysMsg(MSG_Withdrawn + 1),
                   SysMsg(MSG_Withdrawn + 2), SysMsg(MSG_Withdrawn + 3));
        appSprintf(Text, Format, Msg);
        ShowMessageWnd(Text);
    }
    ShowAuthWnd(this);
}
NW_HOOK_METHOD(0x10062c40, NConsoleWnd, BlockedAccountWithMsg)

// 0x10062db0 (vslot 284) - the account is blocked: the message of the lowest reason bit set (0..15), or the ninth's for
// the bits 16 and 17 (C3)
void NConsoleWnd::BlockedAccount(INT Reason)
{
    INT Bit;
    for (Bit = 0; Bit < 16; Bit++)
    {
        if (Reason & (1 << Bit))
        {
            ShowMessageWnd(SysMsg(MSG_BlockReason + Bit));
            break;
        }
    }
    if (Bit == 16 && (Reason & 0x30000))
        ShowMessageWnd(SysMsg(MSG_BlockReason + 9));
    ShowAuthWnd(this);
}
NW_HOOK_METHOD(0x10062db0, NConsoleWnd, BlockedAccount)

// 0x1005b5f0 (vslot 176) - the server's protocol version: 0 when it differs. With [URL] L2VersionCheck (1 when not
// set) the client logs out. C3: a second value is kept (+0x39D0)
void NConsoleWnd::ReceiveVersionCheck(L2ParamStack& Params)
{
    // (blank lines kept on purpose: the check() __LINE__ values below stay those of the stock build)




    BYTE bSame = (BYTE)(INT)Params.Top();
    INT Second = (INT)Params.Top();
    UBOOL bVersionCheck = 1;
    if (!GConfig->GetBool(TEXT("URL"), TEXT("L2VersionCheck"), bVersionCheck, NULL))
        bVersionCheck = 1;
    if (bVersionCheck)
    {
        if (!bSame)
        {
            Engine->NetHandler->SendLogOutPacket();  // RequestLogout's
            ShowMessageWnd(SysMsg(MSG_ProtocolQuit));
        }
    }
    else if (!bSame)
        ShowMessageWnd(SysMsg(MSG_ProtocolContinue));
    Unknown39D0 = Second;
}
NW_HOOK_METHOD(0x1005b5f0, NConsoleWnd, ReceiveVersionCheck)

/*-----------------------------------------------------------------------------
    The character selection and creation, entering the world.
-----------------------------------------------------------------------------*/

// 0x100635e0 (vslot 35) - the character list follows (AddCharacterInfo): the selection replaces the login screen
// (not while in the world): the login screen starts over and hides with the options window, the character
// creation hides, the selection is emptied and shown with the focus, the camera goes to the selection and the
// state is the lobby's. C3: the engine passes one argument (ret 4; include/ue2 still declares none, so the body is
// the non-virtual ReceiveCharacterInfoX hooked at the slot's address and the slot's own stub jumps to it): the
// voice "SystemMsg_k.char_start" is played when it is 0
#if NW_HOOK_MODE
NW_NAKED void NConsoleWnd::ReceiveCharacterInfo() NW_STOCK(0x100635e0)
#else
NW_SLOT_IS("?ReceiveCharacterInfo@NConsoleWnd@@UAEXXZ", "?ReceiveCharacterInfoX@NConsoleWnd@@QAEXH@Z")   // NConsoleWnd::ReceiveCharacterInfo = NConsoleWnd::ReceiveCharacterInfoX (0x100635e0, ret 4)
#endif

void NConsoleWnd::ReceiveCharacterInfoX(INT bNoVoice)
{
    guard(NConsoleWnd::ReceiveCharacterInfo);
    if (State != L2CS_InGame)
    {
        LoginWnd->ResetWindows();
        LoginWnd->HideWindow();
        nw::Field<NCWnd*>(GodWnd, 0xD4)->HideWindow();   // the options window
        PawnCreateWnd->HideWindow();
        LobbyWnd->ClearCharacters();
        LobbyWnd->ShowWindow();
        LobbyWnd->SetFocus();
        MoveCameraByState(L2CS_None, L2CS_Lobby);
        SetState(L2CS_Lobby);
        if (!bNoVoice)
            PlayVoiceName(TEXT("SystemMsg_k.char_start"));
    }
    unguard;
}
NW_HOOK_METHOD(0x100635e0, NConsoleWnd, ReceiveCharacterInfoX)

// 0x1005b360
void NConsoleWnd::AddCharacterInfo(User Info)
{
    guard(NConsoleWnd::AddCharacterInfo);
    check(LobbyWnd);
    if (State != L2CS_InGame)
        LobbyWnd->AddCharacter(Info);
    unguard;
}
NW_HOOK_METHOD(0x1005b360, NConsoleWnd, AddCharacterInfo)

// 0x100636f0 (vslot 37) - into the world: the desktop replaces the loading screen, and the screen takes
// the game's resolution if it differs (the result is not read). C2: the four checks are the stock's own
// (names of its members, lines of Src\NConsoleWindow.cpp), and the game's size follows what the
// viewport got when SetRes succeeded
void NConsoleWnd::AcceptGameStart(INT Result)
{
    guard(NConsoleWnd::AcceptGameStart);
    if (!Desktop)
        appFailAssert("m_pConsole", "Src\\NConsoleWindow.cpp", 0x816);
    if (!LoadingWnd)
        appFailAssert("m_pLoadingWnd", "Src\\NConsoleWindow.cpp", 0x818);
    if (!Engine->Client->Viewports(0))
        appFailAssert("m_Engine->Client->Viewports(0)", "Src\\NConsoleWindow.cpp", 0x819);
    if (!Engine->GRenDev)
        appFailAssert("m_Engine->GRenDev", "Src\\NConsoleWindow.cpp", 0x81A);
    LoadingWnd->HideWindow();
    Desktop->ShowWindow();
    UViewport* Viewport = Engine->Client->Viewports(0);
    INT ScreenX = GSEKScreenX;
    if (Viewport->SizeX != ScreenX)
    {
        if (Engine->GRenDev->SetRes(Viewport, ScreenX, GSEKScreenY, GColorBits / 8, GRefreshRate,
                                    Unknown3720, 1))   // +0x39B8 (C2 +0x3594): fullscreen
        {
            GSEKScreenX = Engine->Client->Viewports(0)->SizeX;
            GSEKScreenY = nw::Field<INT>(Engine->Client->Viewports(0), 0x8C);   // UViewport::SizeY
        }
    }
    SetState(L2CS_InGame);
    unguard;
}
NW_HOOK_METHOD(0x100636f0, NConsoleWnd, AcceptGameStart)

// 0x1005b4a0 - only the failures are answered
void NConsoleWnd::AcceptCharacterDelete(INT Result)
{
    guard(NConsoleWnd::AcceptCharacterDelete);
    switch (Result)
    {
    case 1:
        ShowMessageWnd(SysMsg(MSG_DeleteFailed));
        break;
    case 2:
        ShowMessageWnd(SysMsg(MSG_ClanLeaderDelete));
        break;
    case 3:
        ShowMessageWnd(SysMsg(MSG_ClanMemberDelete));
        break;
    }
    unguard;
}
NW_HOOK_METHOD(0x1005b4a0, NConsoleWnd, AcceptCharacterDelete)

// 0x100638a0 (vslot 39) - the creation may start: its screen replaces the selection, with the creation's voice (C3)
void NConsoleWnd::AcceptNewCharacter(INT Result)
{
    guard(NConsoleWnd::AcceptNewCharacter);
    if (Result)
    {
        LobbyWnd->HideWindow();
        PawnCreateWnd->Reset();
        PawnCreateWnd->ShowWindow();
        MoveCameraByState(L2CS_None, L2CS_Create);
        SetState(L2CS_Create);
        PlayVoiceName(TEXT("SystemMsg_k.char_creation"));
    }
    else
    {
        SetState(L2CS_Lobby);
        ShowMessageWnd(SysMsg(MSG_CreateFailed));
    }
    unguard;
}
NW_HOOK_METHOD(0x100638a0, NConsoleWnd, AcceptNewCharacter)

// 0x100639a0 (vslot 40) - only a failure is answered (C3: the list follows a success with the lobby's voice)
void NConsoleWnd::AcceptCharacterCreate(INT Result)
{
    guard(NConsoleWnd::AcceptCharacterCreate);
    if (!Result)
        ShowMessageWnd(SysMsg(MSG_CreateFailed));
    else
        PlayVoiceName(TEXT("SystemMsg_k.char_lobby"));
    unguard;
}
NW_HOOK_METHOD(0x100639a0, NConsoleWnd, AcceptCharacterCreate)

// 0x1005b260 - the creation was refused: 1 too many characters (the choices start over), 2 the
// name is taken, 3 the name is not valid (the name is typed again)
void NConsoleWnd::CharacterCreateFail(INT Reason)
{
    switch (Reason)
    {
    case 0:
        ShowMessageWnd(SysMsg(MSG_CreateFailed));
        break;
    case 1:
        ShowMessageWnd(SysMsg(MSG_TooManyCharacters));
        PawnCreateWnd->SetupWnd->Reset();
        break;
    case 2:
        ShowMessageWnd(SysMsg(MSG_NameExists));
        PawnCreateWnd->SetupWnd->NameEdit->ClearText();
        PawnCreateWnd->SetupWnd->NameEdit->SetFocus();
        break;
    case 3:
        ShowMessageWnd(SysMsg(MSG_NameInvalid));
        PawnCreateWnd->SetupWnd->NameEdit->ClearText();
        PawnCreateWnd->SetupWnd->NameEdit->SetFocus();
        break;
    }
    SetState(L2CS_Create);
}
NW_HOOK_METHOD(0x1005b260, NConsoleWnd, CharacterCreateFail)

// 0x1005ba30 - the template of a class for the creation
void NConsoleWnd::AddRecommandedData(RecommandedDataStr Data)
{
    guard(NConsoleWnd::AddRecommandedData);
    PawnCreateWnd->SetRecommendedData(Data);
    unguard;
}
NW_HOOK_METHOD(0x1005ba30, NConsoleWnd, AddRecommandedData)

// 0x1005bac0
void NConsoleWnd::EndRecommandedData()
{
    guard(NConsoleWnd::EndRecommandedData);
    PawnCreateWnd->EndRecommendedData();
    unguard;
}
NW_HOOK_METHOD(0x1005bac0, NConsoleWnd, EndRecommandedData)

/*-----------------------------------------------------------------------------
    The restart, the revival, the logout, a closed server.
-----------------------------------------------------------------------------*/

// 0x1005b6a0 (vslot 187) - the restart was accepted (first): the window at +0x3950 is told (C3), the game's windows
// give way to the loading screen
void NConsoleWnd::RestartResetWindows()
{
    if (GodWnd)
        static_cast<NCGodWnd*>(GodWnd)->RestartBegin();
    GetDesktop()->Reset();
    Desktop->HideWindow();
    PetitionWnd->HideWindow();
    WarningDialogBox->HideWindow();
    LoadingWnd->ShowWindow();
}
NW_HOOK_METHOD(0x1005b6a0, NConsoleWnd, RestartResetWindows)

// 0x1005b700 (vslot 188) - (then) back to the character selection: the lobby's child is told first (C3), the engine
// travels to the lobby map ("Lobby?L2Restart", from the [DefaultPlayer] URL) and the selection is shown again
void NConsoleWnd::RestartToLobby()
{
    if (nw::Field<void*>(LobbyWnd, 0xC8))
        nw::Field<NCLobbyWndExt*>(LobbyWnd, 0xC8)->ClearSlots();
    FString Error;
    FURL DefaultURL(NULL);
    DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), TEXT("User"));
    FURL URL(&DefaultURL, TEXT("Lobby"), TRAVEL_Partial);
    URL.AddOption(TEXT("L2Restart"));
    if (!static_cast<UGameEngine*>(Engine)->Browse(URL, NULL, Error))
        GError->Logf(LocalizeError(TEXT("FailedBrowse"), TEXT("Engine")), *Error);
    SpawnDefaultCharacter();
    LoadingWnd->HideWindow();
    LobbyWnd->ClearCharacters();
    LobbyWnd->ShowWindow();
    LobbyWnd->SetFocus();
    SetState(L2CS_Lobby);
    MoveCameraByState(L2CS_None, L2CS_Lobby);
}
NW_HOOK_METHOD(0x1005b700, NConsoleWnd, RestartToLobby)

// 0x1006cb80 (vslot 43) - someone offers to revive the dead player (the offer's two values go back with the answer);
// only in the world. C3: the window at +0x3950 has the restart menu (+0xCC), the dialog is 0x20, the message 0x14C
void NConsoleWnd::ReviveRequest(INT Param1, INT Param2)
{
    if (State != L2CS_InGame)
        return;
    nw::Field<NCWnd*>(GodWnd, 0xCC)->HideWindow();
    BYTE InfoMem[sizeof(NDialogInfo)];      // destroyed out of line (0x10006E20) as the stock does
    NDialogInfo* Info = ::new ((void*)InfoMem) NDialogInfo;
    Info->ID = DIALOG_REVIVE;
    Info->bMainBox = 1;
    Info->bInput = 0;
    Info->Owner = NULL;
    Info->SetParams(Param1, Param2);
    Info->SetSystemMessage(MSG_AskRevive);
    ShowDialog(Info);
    Info->~NDialogInfo();
}
NW_HOOK_METHOD(0x1006cb80, NConsoleWnd, ReviveRequest)

// 0x1006c9f0 - the server let the player go: the window positions are saved and the game
// ends
void NConsoleWnd::AcceptLogOut()
{
    SaveWindowPositions();
    appRequestExit(0);
}
NW_HOOK_METHOD(0x1006c9f0, NConsoleWnd, AcceptLogOut)

#if NW_FEATURES
// phase 6: ReturnToLogin (included here, below the check()s: their __LINE__ stays the faithful
// build's)
#include "features/ReturnToLogin.h"
#endif

// the stock body is AcceptLogOut's (folded)
void NConsoleWnd::QuitGame()
{
#if NW_FEATURES
    // phase 6: ReturnToLogin (NWindow_C1 src/inject/ReturnToLogin.cpp): OK on the
    // "disconnected" box (the only caller) goes back to the login screen from the next Tick
    // instead of ending the game
    if (nwf::ReturnToLogin_QuitGame())
        return;
#endif
    SaveWindowPositions();
    appRequestExit(0);
}

// 0x1006ca00 (vslot 42) - the server closed the connection: in the world a dialog box whose OK ends the game, before
// it the login stages' own box. C3: the desktop's messenger window is closed and hidden first, and the message is
// set for the reasons 0, 1 and 2 only (system message 0x7F)
void NConsoleWnd::ServerClose(INT Reason)
{
    // the stock keeps one dialog info per branch (two 0xA0 byte slots in a frame of 0x140 bytes) and destroys it
    // out of line (0x10006E20); the same here, so the frame is not smaller than the stock one
    BYTE InfoMemA[sizeof(NDialogInfo)];
    BYTE InfoMemB[sizeof(NDialogInfo)];
    if (State == L2CS_InGame)
    {
        ((NCConsoleExt*)Desktop)->LogoutMessenger();
        if (static_cast<NCConsole*>(Desktop)->MessengerWnd)
            static_cast<NCConsole*>(Desktop)->MessengerWnd->HideWindow();
        NDialogInfo* Info = ::new ((void*)InfoMemA) NDialogInfo;
        Info->ID = DIALOG_DISCONNECTED;
        Info->bMainBox = 1;
        Info->bInput = 0;
        Info->Owner = NULL;
        if (Reason == 0 || Reason == 1 || Reason == 2)
            Info->SetSystemMessage(MSG_DisconnectedRetry);
        ShowDialog(Info);
        Info->~NDialogInfo();
    }
    else
    {
        ((NCConsoleExt*)Desktop)->LogoutMessenger();
        if (static_cast<NCConsole*>(Desktop)->MessengerWnd)
            static_cast<NCConsole*>(Desktop)->MessengerWnd->HideWindow();
        NDialogInfo* Info = ::new ((void*)InfoMemB) NDialogInfo;
        Info->ID = DIALOG_DISCONNECTED;
        Info->bInput = 0;
        Info->Owner = NULL;
        if (Reason == 0 || Reason == 1 || Reason == 2)
            Info->SetSystemMessage(MSG_DisconnectedRetry);
        DialogBox->SetInfo(Info);
        DialogBox->ShowWindow();
        DialogBox->SetFocus();
        Info->~NDialogInfo();
    }
}
NW_HOOK_METHOD(0x1006ca00, NConsoleWnd, ServerClose)

// 0x1005b560 - the console's own: the connection is gone (on the message line)
void NConsoleWnd::ReceiveServerClose()
{
    guard(NConsoleWnd::ReceiveServerClose);
    ShowMessageWnd(SysMsg(MSG_Disconnected));
    unguard;
}
NW_HOOK_METHOD(0x1005b560, NConsoleWnd, ReceiveServerClose)
