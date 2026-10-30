#pragma once
// NCSSQPreWinnerWnd - the previous period's winners list of the Seven Signs status page (UClass "CSSQPreWinnerWnd",
// 0x128; new in C3): a scrolling list, a block per seal: its name, the winner (a cabal's name, or a "-" line) and a
// text line of the system messages; every second row sits on the band texture. The console fills the three
// arrays (a seal id, the winning cabal and a system message id, one entry each).

#include "nwindow/NCScrollWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCSSQPreWinnerWnd : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCSSQPreWinnerWnd, NCScrollWnd, 0, 0x102A9100)

    UTexture* LineTex;               // +0x100 "L2UI_CH3.Etc.TextBackline" (name ours)
    TArray<INT> Seals;               // +0x104 SSQSealName's argument (name ours)
    TArray<INT> Winners;             // +0x110 1 / 2 the winning cabal (SSQCabalName), anything else "no winner" (name ours)
    TArray<INT> Messages;            // +0x11C a system message id: its text under the seal's name (name ours)

    NCSSQPreWinnerWnd();                                             // 0x100E9940
    // ~NCSSQPreWinnerWnd                                            // 0x100E9A00 (the three arrays)

    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100E91A0
};
NW_ASSERT_OFFSET(NCSSQPreWinnerWnd, LineTex, 0x100);
NW_ASSERT_OFFSET(NCSSQPreWinnerWnd, Seals, 0x104);
NW_ASSERT_OFFSET(NCSSQPreWinnerWnd, Winners, 0x110);
NW_ASSERT_OFFSET(NCSSQPreWinnerWnd, Messages, 0x11C);
NW_ASSERT_SIZE(NCSSQPreWinnerWnd, 0x128);

#pragma pack(pop)
