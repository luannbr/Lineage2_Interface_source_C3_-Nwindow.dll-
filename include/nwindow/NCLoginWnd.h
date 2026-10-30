#pragma once
// NCLoginWnd - the login screen (UClass "CLoginWnd", 0xCC; NConsoleWnd +0x3528): the game
// logo, the login box (NCAuthWnd), the announcement, warning, EULA and announce boxes, the server
// list (NCServerSelectWnd), the key crypt warning box and four buttons in the lower right corner:
// "New Account" and "Lost Account" (created disabled), "Options" and (new in C2) "Credits".

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCAuthWnd;
class NCWarningWnd;
class NCEulaWnd;
class NCAnnounceWnd;
class NCKeyCryptWarningDialogBox;

class NCLoginWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCLoginWnd, NCWnd, 0, 0x10286488)

    // C3 layout from OnCreate (0x100932f0) and OnDestroy (0x10092950): the notice box is gone, the server list sits in
    // a window of its own (NCLoginServerWnd, new) and the four buttons in another (NCLoginFunctionWnd, new)
    NCAuthWnd* AuthWnd;              // +0xC8 "Log In" (title SysString 149)
    NCWarningWnd* WarningWnd;        // +0xCC "Warning" (title SysString 538)
    NCEulaWnd* EulaWnd;              // +0xD0 (title SysString 598)
    NCAnnounceWnd* AnnounceWnd;      // +0xD4
    NCWnd* ServerWnd;                // +0xD8 NCLoginServerWnd (0xD0 bytes; its +0xC8 is the NCServerSelectWnd*)
    NCWnd* FunctionWnd;              // +0xDC NCLoginFunctionWnd (0xDC bytes: the buttons)
    NCKeyCryptWarningDialogBox* KeyCryptWnd; // +0xE0 NCKeyCryptWarningDialogBox
    UTexture* LogoTex;               // +0xE4 localization.ini "<country>_LogoTexture"
    UTexture* MiniLogoTex;           // +0xE8 localization.ini "<country>_MiniLogoTexture"

    NCLoginWnd();                                                    // 0x10093250
    // ~NCLoginWnd                                                   // 0x10093ac0 (C1 only: no C2 match)

    // 25 WindowProc: NCWnd's own (0x1002fa20), no override
    virtual INT OnCreate();                                          // 37 0x100932f0
    virtual INT OnDestroy();                                         // 38 0x10092950
    virtual void ShowWindow();                                       // 44 0x10092290
    virtual void HideWindow();                                       // 45 0x10092330
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10092b00

    void ResetWindows();                                             // 0x10092360 (name ours)
};
NW_ASSERT_OFFSET(NCLoginWnd, AuthWnd, 0xC8);
NW_ASSERT_OFFSET(NCLoginWnd, ServerWnd, 0xD8);
NW_ASSERT_OFFSET(NCLoginWnd, KeyCryptWnd, 0xE0);
NW_ASSERT_OFFSET(NCLoginWnd, LogoTex, 0xE4);
NW_ASSERT_SIZE(NCLoginWnd, 0xEC);


// NWM_COMMAND ids of the function window's buttons (names ours)
enum
{
    NID_LOGIN_OPTIONS = (INT)0xAFF12011,
    NID_LOGIN_NEWACCOUNT = (INT)0xAFF12012,
    NID_LOGIN_CREDITS = (INT)0xAFF12014,
    NID_LOGIN_LOSTACCOUNT = (INT)0xAFF12015,
};

class NCButton;
class NCServerSelectWnd;
class NCServerInfoWnd;

// NCLoginFunctionWnd - the four buttons in the lower right corner of the login screen (UClass "CLoginFunctionWnd",
// 0xDC; NCLoginWnd +0xDC, new in C3: C2 had the buttons in NCLoginWnd itself). "New Account" and "Lost Account" show
// the systemmsg lines 1186 and 1187 (the addresses to visit) in the message window, "Options" toggles the options
// window and "Credits" hides the login screen and starts the credits. The same shape as NCLobbyFunctionWnd (it is the
// window the lobby's guard names were copied from).
class NCLoginFunctionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCLoginFunctionWnd, NCWnd, 0, 0x10285A40)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.LobbyWnd.lobby_menuback" (120 x 138)
    NCButton* NewAccountButton;      // +0xCC id 0xAFF12012, sysstring 154 (names ours)
    NCButton* LostAccountButton;     // +0xD0 id 0xAFF12015, sysstring 155
    NCButton* OptionsButton;         // +0xD4 id 0xAFF12011, sysstring 156
    NCButton* CreditsButton;         // +0xD8 id 0xAFF12014, sysstring 805

    NCLoginFunctionWnd();                                            // 0x10091a30 (NCWnd's constructor and the vtable)
    // ~NCLoginFunctionWnd                                           // 0x10091a80

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10091190
    virtual INT OnCreate();                                          // 37 0x100916e0
    virtual INT OnDestroy();                                         // 38 0x100915b0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100914c0
    // the four guards carry the same name in the stock code (OnNewAccountBtnClick); names of the slots ours
    virtual INT OnNewAccountBtnClick();                              // 68 0x10091240 systemmsg 1186 in the message window
    virtual INT OnOptionsBtnClick();                                 // 69 0x10091360 the options window
    virtual INT OnCreditsBtnClick();                                 // 70 0x100913e0 the credits
    virtual INT OnLostAccountBtnClick();                             // 71 0x100912d0 systemmsg 1187 in the message window
};
NW_ASSERT_OFFSET(NCLoginFunctionWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCLoginFunctionWnd, CreditsButton, 0xD8);
NW_ASSERT_SIZE(NCLoginFunctionWnd, 0xDC);

// NCLoginServerWnd - the server list and the help page under it, one window (UClass "CLoginServerWnd", 0xD0;
// NCLoginWnd +0xD8, new in C3): the list (NCServerSelectWnd, titled with sysstring 449) over the page of
// ..\l2text\server_help.htm (NCServerInfoWnd, sysstring 857). The console's ReceiveServerList lands here.
class NCLoginServerWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCLoginServerWnd, NCWnd, 0, 0x10285F60)

    NCServerSelectWnd* ServerSelectWnd;   // +0xC8 316 x 330 at the top left corner
    NCServerInfoWnd* ServerInfoWnd;       // +0xCC 316 x 205 at (0, 337), under it (names ours)

    NCLoginServerWnd();                                              // 0x10091ce0 (NCWnd's constructor and the vtable)
    // ~NCLoginServerWnd                                             // 0x10091d30

    virtual INT OnCreate();                                          // 37 0x10091d80
    virtual INT OnDestroy();                                         // 38 0x10091b70

    // (name ours; ret 4) the server the account played on last goes to the list, which is shown (the window too)
    void ReceiveServerList(INT LastServerID);                        // 0x10091ae0
};
NW_ASSERT_OFFSET(NCLoginServerWnd, ServerSelectWnd, 0xC8);
NW_ASSERT_OFFSET(NCLoginServerWnd, ServerInfoWnd, 0xCC);
NW_ASSERT_SIZE(NCLoginServerWnd, 0xD0);
#pragma pack(pop)
