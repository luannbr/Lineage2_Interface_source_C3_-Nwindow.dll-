#pragma once
// NCLoadingWnd - the loading screen while entering the world (UClass "CLoadingWnd", 0x98): a
// 1024 x 768 picture stretched over the whole window. NConsoleWnd::RequestGameStart picks the
// picture: in Korean one per age rating of the chosen server, elsewhere the country's entry of
// FontInfo.ini.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCLoadingWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCLoadingWnd, NCWnd, 0, 0x10284078)

    UTexture* LoadingTex;            // +0xC8

    NCLoadingWnd();                                                  // 0x1008e620
    // ~NCLoadingWnd                                                 // 0x1008e670

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate() { return 1; }                             // 37 0x10005fa0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1008e580

    void SetLoadingTexture(INT ScreenID);                            // 0x1008e3c0 (name ours)
};
NW_ASSERT_OFFSET(NCLoadingWnd, LoadingTex, 0xC8);
NW_ASSERT_SIZE(NCLoadingWnd, 0xCC);

#pragma pack(pop)