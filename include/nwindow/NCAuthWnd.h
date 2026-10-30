#pragma once
// NCAuthWnd - the login box of the login screen (UClass "CAuthWnd", 0xB4): account and password
// lines, "Log In" and "Exit" (NCLoginWnd creates it). Log In, or Enter in the password line,
// checks that both lines are filled and asks NConsoleWnd::RequestAuthLogin - once, until the
// box is shown again or its 3 s timer fires. Exit quits the game.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCEditBox;
class NCButton;

class NCAuthWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCAuthWnd, NCWnd, 0, 0x10286ED0)

    INT Mode;                        // +0xC8 (new in C2, name ours) 0 account and password, 1 the card key
                                     //   (the security card: only the password line is used, with the
                                     //   prompt of systemmsg 1237 for CardPos)
    INT CardPos;                     // +0xCC (new in C2, name ours) the argument of systemmsg 1237 (Mode 1)
    INT CardID;                      // +0xD0 (new in C2, name ours) first argument of RequestAuthCardKeyLogin
    INT Unknown94;                   // +0xD4 (0, never used)   // C2: offset derived, no evidence
    UTexture* BackTex;               // +0xD8 "L2UI.LobbyWnd.loginBack"   // C2: offset derived, no evidence
    NCEditBox* AccountEdit;          // +0xDC
    NCEditBox* PasswordEdit;         // +0xE0
    NCButton* OKButton;              // +0xE4 "Log In"
    NCButton* CancelButton;          // +0xE8 "Exit"
    INT TimerResult;                 // +0xEC what SetTimer returned (never read)   // C2: offset derived, no evidence
    UBOOL bRequested;                // +0xF0 a login request went out

    NCAuthWnd();                                                     // 0x10093180
    // ~NCAuthWnd                                                    // 0x100830b0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10091ff0
    virtual INT OnTimer(INT TimerID);                                // 36 0x10092270
    virtual INT OnCreate();                                          // 37 0x10092d50
    virtual INT OnDestroy();                                         // 38 0x10092820
    virtual void ShowWindow();                                       // 44 0x10092230
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100925f0
    // names ours
    virtual INT OnOK();                                              // 68 0x100920a0
    virtual INT OnCancel();                                          // 69 0x10092210 Exit
    virtual INT OnPasswordNotify(INT Code);                          // 70 0x100b2360 Enter in the password line (the body is shared)

    void ClearPassword();                                            // 0x10092090 (name ours)
    void ResetInput();                                               // (C2 only: 0x10057cd0 there; no such function here, nothing calls it) back to the account and password lines
    void SetCardKey(INT InCardID, INT InCardPos);                    // 0x10062830 (name ours) the card key prompt (Mode 1)
};
NW_ASSERT_OFFSET(NCAuthWnd, Mode, 0xC8);
NW_ASSERT_OFFSET(NCAuthWnd, CardID, 0xD0);
NW_ASSERT_OFFSET(NCAuthWnd, BackTex, 0xD8);
NW_ASSERT_OFFSET(NCAuthWnd, AccountEdit, 0xDC);
NW_ASSERT_OFFSET(NCAuthWnd, OKButton, 0xE4);
NW_ASSERT_OFFSET(NCAuthWnd, bRequested, 0xF0);
NW_ASSERT_SIZE(NCAuthWnd, 0xF4);

#pragma pack(pop)