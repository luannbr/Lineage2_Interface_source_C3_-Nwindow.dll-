#pragma once
// NHtmlAnalyzer - an empty scroll window (UClass "HtmlAnalyzer", 0x140): four point deques
// (like NCGDevice's origin stack) and a few fields no code of NWindow reads. Its paint only
// runs the base one; nothing in NWindow creates it.

#include "nwindow/NCScrollWnd.h"
#include "nwindow/NStdLib.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NHtmlAnalyzer : public NCScrollWnd
{
    NW_DECLARE_CLASS(NHtmlAnalyzer, NCScrollWnd, 0, 0x102715B8)

    INT UnknownCC;                   // +0x100 (0)
    INT UnknownD0;                   // +0x104 (0)
    INT UnknownD4;                   // +0x108 (0)
    NPointDeque Points1;             // +0x10C
    NPointDeque Points2;             // +0x120
    INT Unknown100;                  // +0x134 (0)
    INT Unknown104;                  // +0x138 (0)
    INT Unknown108;                  // +0x13C (5)
    NPointDeque Points3;             // +0x140
    NPointDeque Points4;             // +0x154
    INT Unknown134[3];               // +0x168 never set   // C2: offset derived, no evidence   // C2: offset derived, no evidence

    NHtmlAnalyzer();                                                 // 0x10032bc0
    // ~NHtmlAnalyzer                                                // 0x10032c90

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10093f30 (shared)
    virtual INT OnCreate();                                          // 37 0x10032b10 (shared)
    virtual INT OnDestroy();                                         // 38 0x10032b20 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10032b30
};
NW_ASSERT_OFFSET(NHtmlAnalyzer, Points1, 0x10C);
NW_ASSERT_OFFSET(NHtmlAnalyzer, Unknown108, 0x13C);
NW_ASSERT_OFFSET(NHtmlAnalyzer, Points4, 0x154);
NW_ASSERT_SIZE(NHtmlAnalyzer, 0x174);

#pragma pack(pop)