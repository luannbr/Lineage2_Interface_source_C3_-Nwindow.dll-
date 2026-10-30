#pragma once
// LoadingWnd - the messenger's "please wait" box (UClass "CLoadingWnd", 0xD4; C3 only, no C2 source): the messenger
// picture with a small icon and one line of text, shown while the sign-in or the sign-out is under way (system message
// 1324 for a sign-in, 1358 for a sign-out). The messenger window makes it (0x10139A40).
//
// Layout and virtuals from the stock constructor 0x101377A0, OnCreate 0x10130A50 and OnPaint 0x10131C00.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class MsnWnd;

class LoadingWnd : public NCWnd
{
    NW_DECLARE_CLASS(LoadingWnd, NCWnd, 0, 0x102C3208)

    MsnWnd* Owner;                   // +0xC8 the messenger window that made it (the constructor's argument) (name ours)
    UBOOL bSigningIn;                // +0xCC 1: the text says the sign-in is under way, 0: the sign-out (name ours)
    UTexture* BackTex;               // +0xD0 "L2UI_CH3.Msn.messenger_back1"

    LoadingWnd(MsnWnd* InOwner = NULL); // 0x101377A0 (ret 4); defaults = what the stock InternalConstructor pushes
    // ~LoadingWnd                                                   // 0x10137810

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (NControl's body, shared)
    virtual INT OnCreate();                                          // 37 0x10130A50
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10131C00

    void ShowWithMode(UBOOL bInSigningIn);                           // 0x10130A80 (name ours; ret 4) sets the mode, then NCWnd's ShowWindow
};
NW_ASSERT_OFFSET(LoadingWnd, Owner, 0xC8);
NW_ASSERT_OFFSET(LoadingWnd, bSigningIn, 0xCC);
NW_ASSERT_OFFSET(LoadingWnd, BackTex, 0xD0);
NW_ASSERT_SIZE(LoadingWnd, 0xD4);

#pragma pack(pop)
