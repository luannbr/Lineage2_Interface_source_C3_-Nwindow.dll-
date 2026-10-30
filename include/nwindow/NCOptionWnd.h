#pragma once
// NCOptionWnd - the options window (UClass "COptionWnd", 0xB8; system menu > Options): three
// tab pages (video, audio, game) and OK / Cancel / Apply. OK and Apply make every page apply
// its settings and mark the first run done (Option.ini [FirstRun]); OK and Cancel close it.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCTabControl;
class NCVideoOptionWnd;
class NCAudioOptionWnd;
class NCGameOptionWnd;

// NWM_COMMAND ids of the window's controls
enum
{
    NID_OPTION_FRAME = 0xF001,
    NID_OPTION_OK = 0xF002,
    NID_OPTION_CANCEL = 0xF003,
    NID_OPTION_APPLY = 0xF004,
    NID_OPTION_TABS = 0xF006,
};

class NCOptionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCOptionWnd, NCWnd, 0, 0x102947A8)

    UTexture* BackTex;                     // +0xC8 C3: "L2UI_ch3.SSQWnd.ssq_back" (the strip under the title bar)
    UTexture* OptionBackTex;               // +0xCC "L2ui_ch3.option1_back" (the page's backdrop; name ours)
    NCVideoOptionWnd* VideoWnd;            // +0xD0
    NCAudioOptionWnd* AudioWnd;            // +0xD4
    NCGameOptionWnd* GameWnd;              // +0xD8
    NCButton* OKButton;                    // +0xDC
    NCButton* CancelButton;                // +0xE0
    NCButton* ApplyButton;                 // +0xE4
    NCTabControl* TabCtrl;                 // +0xE8
    NCWnd* UnusedB4;                       // +0xEC destroyed, never created   // C2: offset derived, no evidence
    INT bRestricted;                       // +0xF0 (name ours) the constructor's argument: OnCreate disables the game page's
                                           //   tab while it is set (NCGodWnd makes the window with it)

    NCOptionWnd(INT InRestricted = 0);                               // 0x100bd5d0 (ret 4)
    // ~NCOptionWnd                                                  // 0x100a4420 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100b72a0
    virtual INT OnCreate();                                          // 37 0x100bd680
    virtual INT OnDestroy();                                         // 38 0x100bb1b0
    virtual void ShowWindow();                                       // 44 0x100bbd00
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100bb090
    // names ours
    virtual INT OnOK();                                              // 68 0x100bb390
    virtual INT OnCancel();                                          // 69 0x1007bee0 (folded)
    virtual INT OnApply();                                           // 70 0x100bb3f0
};
NW_ASSERT_OFFSET(NCOptionWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCOptionWnd, OKButton, 0xDC);
NW_ASSERT_OFFSET(NCOptionWnd, TabCtrl, 0xE8);
NW_ASSERT_SIZE(NCOptionWnd, 0xF4);

#pragma pack(pop)