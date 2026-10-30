#pragma once
// NCMessageWnd - a line of text on a background (UClass "CMessageWnd", 0x498): the console's
// message window (NConsoleWnd +0x36D8, NConsoleWnd::ShowMessageWnd), e.g. while the login
// waits for the server list. The text is centered, or drawn from (50, 8) and wrapped to the
// screen width when it is wider than the window.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCMessageWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMessageWnd, NCWnd, 0, 0x10290418)

    UTexture* BackTex;               // +0xC8 "sek.cbui141" stretched over the window
    FString Text;                    // +0xCC C2: an FString (C1: TCHAR Text[512])

    NCMessageWnd();                                                  // 0x100abc80
    // ~NCMessageWnd                                                 // 0x100abd10

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (NControl's body)
    virtual INT OnCreate();                                          // 37 0x100aba10
    virtual INT OnDestroy();                                         // 38 0x100aba40 (NCCompositeBox's body)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100abaf0

    // names ours
    void ClearText();                                                // 0x100abac0 (C1 0x10077fd0)
    void SetText(const TCHAR* NewText);                              // 0x100abad0 NULL keeps the text
};
NW_ASSERT_OFFSET(NCMessageWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCMessageWnd, Text, 0xCC);
NW_ASSERT_SIZE(NCMessageWnd, 0xD8);

#pragma pack(pop)