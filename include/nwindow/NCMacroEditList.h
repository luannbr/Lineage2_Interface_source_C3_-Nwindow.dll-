#pragma once
// NCMacroEditList - the lines of the macro editor (C2 only; UClass "CMacroEditList", 0x530): a scroll window that holds
// twelve single-line edit boxes (one per line of a macro, ids 0x1506..0x1511) of which nine show at a time (C3; C2
// eight), with the line numbers painted down its left side; the slider scrolls by lines. It keeps the command parser
// it checks the lines with (ValidateLines) and the number of lines shown.
//
// Layout from the C3 constructors 0x100A0170 / 0x100A0290, OnCreate 0x1009EE90 and the scroll functions.

#include "nwindow/NCScrollWnd.h"
#include "nwindow/NCommandParser.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCEditBox;

enum
{
    NMACRO_EditLines = 12,         // the line boxes (NMACRO_MaxLines)
    NMACRO_ShownLines = 9,         // shown at a time (C3; C2 8)
};

class NCMacroEditList : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCMacroEditList, NCScrollWnd, 0, 0x1028A248)

    TArray<NCEditBox*> Edits;        // +0x100 the twelve line boxes (OnCreate)
    INT VisibleLines;                // +0x10C the lines shown (9: the constructor's first argument; 0 in the default one)
    INT TopLine;                     // +0x110 the first line shown: the slider's value
    INT TotalLines;                  // +0x114 the slider's range (the constructor's second argument, 12; UpdateSlider sets 12)
    NCommandParser Parser;           // +0x118 recognizes the commands of the lines (ValidateLines)   // C2: offset derived, no evidence

    NCMacroEditList();                                               // 0x100A0170
    NCMacroEditList(INT InVisibleLines, INT InTotalLines);           // 0x100A0290 (ret 8; names ours; C2 had one argument)
    // ~NCMacroEditList                                              // 0x100A0220

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10093F30, the body of NCItemWnd::WindowProc (shared, hooked there; a stub here)
    virtual INT OnCreate();                                          // 37 0x1009EE90
    virtual INT OnDestroy();                                         // 38 0x1009F180
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1009F020
    virtual INT ScrollUp();                                          // 69 0x1009D940
    virtual INT ScrollDown();                                        // 70 0x1009D980
    virtual INT OnScrollBar(INT Value);                              // 71 0x1009D910

    // names ours
    __declspec(noinline) INT GetFocusLine();                                              // 0x1009D790 the line whose box has the focus, -1 none
    __declspec(noinline) void LayoutLines(INT NewTopLine);                                // 0x1009D7D0 (ret 4; C3) hides the boxes that leave, shows the nine from NewTopLine
    __declspec(noinline) void UpdateSlider();                                             // 0x1009B420 the range and the value
    __declspec(noinline) void InitSlider();                                               // 0x1009B3E0 first setup (repeat delay 200 ms)
    __declspec(noinline) INT HitLine(INT X, INT Y);                                       // 0x1009D730 the line box under a point, -1 none
    __declspec(noinline) UBOOL ValidateLines();                                           // 0x1009D9D0 the lines hold valid commands, no gap
};
NW_ASSERT_OFFSET(NCMacroEditList, Edits, 0x100);
NW_ASSERT_OFFSET(NCMacroEditList, VisibleLines, 0x10C);
NW_ASSERT_OFFSET(NCMacroEditList, TopLine, 0x110);
NW_ASSERT_OFFSET(NCMacroEditList, Parser, 0x118);
NW_ASSERT_SIZE(NCMacroEditList, 0x530);

#pragma pack(pop)
