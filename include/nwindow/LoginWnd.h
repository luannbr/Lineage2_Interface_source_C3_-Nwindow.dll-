#pragma once
// LoginWnd - the messenger's log-in window (UClass "CLoginWnd", 0xE8; C3 only, no C2 source; not the game's NCLoginWnd):
// the account and the password boxes, the "log in" and "clear" buttons and the "version" one, over the messenger picture
// and the logo. The messenger window makes it (0x10139A40) and shows it until the sign-in is under way.
//
// Layout and virtuals from the stock constructor 0x10137620, OnCreate 0x101348C0, OnDestroy 0x101316D0, OnPaint 0x101317C0
// and WindowProc 0x10136530.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class MsnWnd;
class NCEditBox;
class NCButton;

class LoginWnd : public NCWnd
{
    NW_DECLARE_CLASS(LoginWnd, NCWnd, 0, 0x102C50E8)

    NCEditBox* IdEdit;               // +0xC8 the account (100 characters at most), 151 x 17 at (92, 131) (name ours)
    NCEditBox* PasswordEdit;         // +0xCC id 0xA56, the password (16 characters at most, type 2), 151 x 17 at (92, 151) (name ours)
    NCButton* LoginButton;           // +0xD0 id 0xA09, 66 x 21 at (72, 176), system string 140 (name ours)
    NCButton* ClearButton;           // +0xD4 id 0xA0A, 66 x 21 at (142, 176), system string 141 (name ours)
    NCButton* VersionButton;         // +0xD8 id 0xA0B, 134 x 23 at (72, 204), system string 995 (name ours)
    MsnWnd* Owner;                   // +0xDC the messenger window that made it (the constructor's argument) (name ours)
    UTexture* BackTex;               // +0xE0 "L2UI_CH3.Msn.messenger_back1"
    UTexture* LogoTex;               // +0xE4 "L2UI_CH3.Msn.msnlogo"

    LoginWnd(MsnWnd* InOwner = NULL); // 0x10137620 (ret 4); defaults = what the stock InternalConstructor pushes
    // ~LoginWnd                                                     // 0x101376A0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10136530
    virtual INT OnCreate();                                          // 37 0x101348C0
    virtual INT OnDestroy();                                         // 38 0x101316D0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x101317C0

    // names ours (anonymous in the db)
    INT OnLoginButton();                                             // 0x10131990 asks the owner to sign in with both texts
    INT OnClearButton();                                             // 0x10130A20 empties the two boxes
    INT OnVersionButton();                                           // 0x10131A70 hides this window, the owner shows the version box
    INT OnEditNotify(INT Code);                                      // 0x10134D40 (ret 4) code 1 (Enter in the password box) logs in
};
NW_ASSERT_OFFSET(LoginWnd, IdEdit, 0xC8);
NW_ASSERT_OFFSET(LoginWnd, PasswordEdit, 0xCC);
NW_ASSERT_OFFSET(LoginWnd, LoginButton, 0xD0);
NW_ASSERT_OFFSET(LoginWnd, ClearButton, 0xD4);
NW_ASSERT_OFFSET(LoginWnd, VersionButton, 0xD8);
NW_ASSERT_OFFSET(LoginWnd, Owner, 0xDC);
NW_ASSERT_OFFSET(LoginWnd, BackTex, 0xE0);
NW_ASSERT_OFFSET(LoginWnd, LogoTex, 0xE4);
NW_ASSERT_SIZE(LoginWnd, 0xE8);

#pragma pack(pop)
