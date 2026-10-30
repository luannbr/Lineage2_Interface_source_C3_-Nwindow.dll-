#pragma once
// NCSSQStatusWnd - the Seven Signs status page (UClass "CSSQStatusWnd", 0x144; new in C3; the first tab of
// NCSSQMainBoard): the period's text and the cabals' numbers (the console fills +0xC8 .. +0x104 from the server's
// Seven Signs status, two pages: the current one, painted by PaintCurrent, and the previous period's winners, painted
// by PaintPrevious with the NCSSQPreWinnerWnd list under it). Two buttons: the first toggles the page, the second asks
// the console for the page again.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCSSQPreWinnerWnd;

class NCSSQStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSSQStatusWnd, NCWnd, 0, 0x102A9620)

    INT Unknown0C8;                  // +0xC8 (PaintCurrent: 3 picks the text of the end of the period) (names ours)
    INT Cabal;                       // +0xCC the cabal that leads (SSQCabalName)
    INT Seal;                        // +0xD0 (SSQSealName)
    INT Number;                      // +0xD4 shown as "%d" and a unit text
    INT UnknownD8;                   // +0xD8
    INT UnknownDC;                   // +0xDC
    INT UnknownE0;                   // +0xE0
    INT UnknownE4;                   // +0xE4
    INT UnknownE8;                   // +0xE8 shown as a share bar (x 1.5 pixels) of the cabal 1 on the current page
    INT UnknownEC;                   // +0xEC the same for the cabal 2
    INT UnknownF0;                   // +0xF0 a number in the boxes
    INT UnknownF4;                   // +0xF4 a number in the boxes
    INT UnknownF8;                   // +0xF8 shown as "%d <text>" in the title line
    INT UnknownFC;                   // +0xFC a system message line when > 0 (SystemMsg[n])
    INT Unknown100;                  // +0x100 the same, second
    INT Unknown104;                  // +0x104 shown as "(%d<unit>)"
    NCSSQPreWinnerWnd* PreWinnerWnd; // +0x108 the previous winners' list (8, 103, 240 x 212)
    INT Mode;                        // +0x10C the page of PaintPrevious: 1 or 2 names the cabal, 0 none
    BYTE _Unknown110[0x114 - 0x110]; // +0x110 never touched
    UBOOL bShowPrevious;             // +0x114 the previous period's page is shown (name ours)
    NCButton* PageButton;            // +0x118 id 0xFF0F0F11 (text 0x3A9 / 0x3AB) (name ours)
    NCButton* RefreshButton;         // +0x11C id 0xFF0F0F12 (text 0x3AA) (name ours)
    UTexture* BackTex;               // +0x120 "L2UI_CH3.SSQWnd.SSQ_back1" (names ours)
    UTexture* BackTex2;              // +0x124 "...SSQ_back1_1"
    UTexture* Bar22Tex;              // +0x128 "...ssq_bar22"
    UTexture* Bar12Tex;              // +0x12C "...ssq_bar12"
    UTexture* Bar2BackTex;           // +0x130 "...ssq_bar2back"
    UTexture* Bar1BackTex;           // +0x134 "...ssq_bar1back"
    UTexture* TextBox1Tex;           // +0x138 "L2UI_CH3.Etc.TextBox1"
    UTexture* TextBox2Tex;           // +0x13C "...TextBox2"
    UTexture* TextBox3Tex;           // +0x140 "...TextBox3"

    NCSSQStatusWnd();                                                // 0x100E9A80
    // ~NCSSQStatusWnd                                               // 0x100E9BA0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100E7E10
    virtual INT OnCreate();                                          // 37 0x100E9BF0
    virtual void ShowWindow();                                       // 44 0x100E9020
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100E9550
    // names ours
    virtual INT OnTogglePage();                                      // 68 0x100E8F60 (0xFF0F0F11)
    virtual INT OnRefresh();                                         // 69 0x100E8FE0 (0xFF0F0F12)

    void ResetStatus();                                              // 0x100E8F00 the numbers cleared, page 1
    void PaintCurrent(NCGDevice* Device);                            // 0x100E7E90 (ret 4, 3395 bytes)
    void PaintPrevious(NCGDevice* Device);                           // 0x100E8BE0 (ret 4)
};
NW_ASSERT_OFFSET(NCSSQStatusWnd, Cabal, 0xCC);
NW_ASSERT_OFFSET(NCSSQStatusWnd, UnknownF8, 0xF8);
NW_ASSERT_OFFSET(NCSSQStatusWnd, PreWinnerWnd, 0x108);
NW_ASSERT_OFFSET(NCSSQStatusWnd, Mode, 0x10C);
NW_ASSERT_OFFSET(NCSSQStatusWnd, bShowPrevious, 0x114);
NW_ASSERT_OFFSET(NCSSQStatusWnd, BackTex, 0x120);
NW_ASSERT_SIZE(NCSSQStatusWnd, 0x144);

#pragma pack(pop)
