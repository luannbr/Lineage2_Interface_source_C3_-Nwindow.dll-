#pragma once
// NCThaiIME - the IME of the Thai client (UClass "CThaiIME", 0x61FC; new in C2, GLanguageType 5). It only keeps
// the input context of the game window: no composition is read (the Thai text is typed with the keyboard layout,
// not composed), so the base class's string buffers stay empty and the edit box tests the characters itself
// (NCEditBox::IsThaiInputValid). The class is as big as the Taiwanese IME's (0x620C minus one pointer) but nothing
// in its code touches the fields past NCIME's.

#include "nwindow/NCIME.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCThaiIME : public NCIME
{
    NW_DECLARE_CLASS(NCThaiIME, NCIME, 0, 0x1026BA50)

    BYTE Unknown41FC[0x2000];        // +0x422C not used by any function of this class (allocated zeroed)   // C2: offset derived, no evidence

    NCThaiIME(INT InMode = 0);                                       // 0x10013790 (C1 only: no C2 match)
    // ~NCThaiIME                                                    // 0x100137e0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10015370
    virtual INT OnCreate();                                          // 37 0x10015410
    virtual INT OnDestroy();                                         // 38 0x10015420
    virtual void ShowWindow();                                       // 44 0x100412c0 (shared)
    virtual void Activate();                                         // 69 0x10015580 (NCIME's: a thunk)
    virtual void Deactivate();                                       // 70 0x100154b0
    virtual INT OnImeNotify(WPARAM wParam, LPARAM lParam);           // 71 0x10015440
    virtual INT OnImeSetContext(WPARAM wParam, LPARAM lParam) { return 1; }      // 74 0x1000bd60 (folded)
    virtual INT OnInputLangChange(WPARAM wParam, LPARAM lParam) { return 1; }    // 76 0x1000bd60 (folded)
};
NW_ASSERT_OFFSET(NCThaiIME, Unknown41FC, 0x422C);
NW_ASSERT_SIZE(NCThaiIME, 0x622C);

#pragma pack(pop)
