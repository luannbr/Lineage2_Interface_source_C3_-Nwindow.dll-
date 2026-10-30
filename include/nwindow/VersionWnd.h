#pragma once
// VersionWnd - the messenger's "version" box (UClass "CVersionWnd", 0xD4; C3 only, no C2 source): a picture with three
// lines of text (system messages 1310 .. 1312) and one button that closes it and shows the log-in window again. The
// messenger window makes it (0x10139A40) and shows it from the log-in window's "version" button.
//
// Layout and virtuals from the stock constructor 0x101376F0, OnCreate 0x10134DC0, WindowProc 0x10134D60 and OnPaint
// 0x10131A90.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class MsnWnd;
class NCButton;

class VersionWnd : public NCWnd
{
    NW_DECLARE_CLASS(VersionWnd, NCWnd, 0, 0x102C4BC0)

    MsnWnd* Owner;                   // +0xC8 the messenger window that made it (the constructor's argument) (name ours)
    NCButton* OKButton;              // +0xCC id 0xA54, 96 x 23 at (79, 274), the label is system string 1008
    UTexture* BackTex;               // +0xD0 "L2UI_CH3.Msn.messenger_back1"

    VersionWnd(MsnWnd* InOwner = NULL); // 0x101376F0 (ret 4); defaults = what the stock InternalConstructor pushes
    // ~VersionWnd                                                   // 0x10137750

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10134D60
    virtual INT OnCreate();                                          // 37 0x10134DC0
    virtual INT OnDestroy();                                         // 38 0x100EB490 (NCSSQMainEventWnd's body, shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10131A90

    __declspec(noinline) INT OnOKButton();                                                // 0x10131BE0 (name ours) hides it, the owner shows the log-in window
};
NW_ASSERT_OFFSET(VersionWnd, Owner, 0xC8);
NW_ASSERT_OFFSET(VersionWnd, OKButton, 0xCC);
NW_ASSERT_OFFSET(VersionWnd, BackTex, 0xD0);
NW_ASSERT_SIZE(VersionWnd, 0xD4);

#pragma pack(pop)
