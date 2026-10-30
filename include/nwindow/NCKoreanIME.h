#pragma once
// NCKoreanIME - the IME of Korean and English clients (UClass "CKoreanIME", 0x4204). Keeps the
// Hangul/alphanumeric mode across focus changes: while the chat line has the focus the mode is
// saved on Deactivate and restored on Activate. A composition with clause information is split
// into Clauses (Flag = target clause) for the edit box.

#include "nwindow/NCIME.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCKoreanIME : public NCIME
{
    NW_DECLARE_CLASS(NCKoreanIME, NCIME, 0, 0x1026AAD8)

    TArray<NImeText> Clauses;        // +0x422C   // C2: offset derived, no evidence

    NCKoreanIME(INT InMode = 0);                                     // 0x10017ee0 (C1 only: no C2 match)
    // ~NCKoreanIME                                                  // 0x10017f50 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10014400
    virtual INT OnCreate();                                          // 37 0x10014480
    virtual INT OnDestroy();                                         // 38 0x100144e0
    virtual void Activate();                                         // 69 0x10014580
    virtual void Deactivate();                                       // 70 0x10014620
    virtual INT OnImeNotify(WPARAM wParam, LPARAM lParam);           // 71 0x10014500
    virtual INT OnImeComposition(WPARAM wParam, LPARAM lParam);      // 75 0x10016120
};
NW_ASSERT_OFFSET(NCKoreanIME, Clauses, 0x422C);
NW_ASSERT_SIZE(NCKoreanIME, 0x4238);

#pragma pack(pop)