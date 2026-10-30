#pragma once
// NCCompositeBox - the IME composition window (UClass "CCompositeBox", 0xB0): the string
// being composed, drawn one character at a time from FirstChar until about 100 px, with a
// "|" caret before the character at CaretPos while bShowCaret is set. Filled by the IMEs.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCCompositeBox : public NCWnd
{
    NW_DECLARE_CLASS(NCCompositeBox, NCWnd, 0, 0x102628E8)

    UTexture* BackTex;         // +0xC8 NWindow.BlackTexture
    FString Text;              // +0xCC composition string
    INT FirstChar;             // +0xD8 first character drawn
    INT CaretPos;              // +0xDC
    UBOOL bShowCaret;          // +0xE0

    NCCompositeBox();                                                // 0x10004430
    // the Chinese IME's (ret 0x14): size and place given (BackTex is left as allocated)
    NCCompositeBox(INT InW, INT InH, INT InX, INT InY, UBOOL bInShowCaret); // 0x10004540
    // ~NCCompositeBox                                               // 0x10003b60 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (NControl's)
    virtual INT OnCreate();                                          // 37 0x10003450
    virtual INT OnDestroy();                                         // 38 0x100034f0 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10003bf0

    // names ours; the text scrolls so that the caret stays within 97 px
    void CaretLeft();                                                // 0x10003480
    void CaretRight();                                               // 0x10003df0
    void SetText(const TCHAR* InText);                               // 0x10003f60
    FString GetText();                                               // 0x100034b0 a copy
    void SetShowCaret(UBOOL bShow);                                  // 0x100034e0
};
NW_ASSERT_OFFSET(NCCompositeBox, Text, 0xCC);
NW_ASSERT_SIZE(NCCompositeBox, 0xE4);

#pragma pack(pop)