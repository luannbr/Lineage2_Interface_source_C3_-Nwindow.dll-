#pragma once
// NCMultiEdit - text box of several lines with word wrap (UClass "CMultiEdit", 0x264): the memo
// of the party matching options, and (as NCHtmlMultiEdit) the <multiedit> boxes of the HTML
// pages. The text is a list of logical lines (NLineList, a line ends with its '\n'); each line
// is wrapped into screen rows at Width - 36 px, TopLine is the first row shown. The caret has a
// logical position (CaretLine, CaretCol) and a screen one (CaretRow, CaretX). The scroll
// buttons and the scroll bar sit outside the box, at x = LocalW + 2. A hidden NCIMEManager child
// gives it an IME, whose private messages (NIM_*) arrive in 67..71; the composition is kept in
// Comp and drawn at the caret, by the painter of the client language (Korean here, the other
// ones not decompiled yet).

#include "nwindow/NControl.h"
#include "nwindow/NCGDevice.h"   // ThaiCharacter
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCIMEManager;
class NCPushButton;
class NCSliderBar;
class NCHtmlPushButton;
class NCHtmlSliderBar;
class NString;

// NWM_COMMAND ids of the scroll controls (names ours)
enum
{
    NID_MULTIEDIT_UP = (INT)0xF0F01409,
    NID_MULTIEDIT_DOWN = (INT)0xF0F01410,
    NID_MULTIEDIT_BAR = (INT)0xF0F01411,
};

// NLineNode - one logical line of NCMultiEdit (0x14 bytes, allocated with the global new; name
// ours). The text is at the start, so a node pointer is used as an FString*.
struct NLineNode
{
    FString Text;              // +0x00
    NLineNode* Prev;           // +0x0C
    NLineNode* Next;           // +0x10

    NLineNode();                                                     // 0x10022790
    ~NLineNode();                                                    // 0x100227e0
};
NW_ASSERT_SIZE(NLineNode, 0x14);

// NLineList - the lines of NCMultiEdit (0x18 bytes, name ours): a doubly linked list with a
// cached cursor (Current, CurIndex) that GetLine walks from, whichever end is nearest.
struct NLineList
{
    INT Count;                 // +0x00
    INT Unknown4;              // +0x04 (0)
    NLineNode* Head;           // +0x08
    NLineNode* Current;        // +0x0C cursor of GetLine
    NLineNode* Tail;           // +0x10
    INT CurIndex;              // +0x14 index of Current

    __declspec(noinline) NLineList();                                // 0x10022f80 one empty line (not inlined: the stock callers call it)
    __declspec(noinline) ~NLineList();                               // 0x10027440 (not inlined: the stock destructors call it)

    // the getter is a folded body the other classes share (0x10033ac0, hooked as NActionItem's)
    INT Num() { return Count; }
    NLineNode* GetLine(INT Index);                                   // 0x10023070
    INT TotalLength();                                               // 0x10023000 characters
    INT Insert(INT Index);                                           // 0x100252b0 -1 = at the end
    void RemoveAt(INT Index);                                        // 0x10025450
    void Clear();                                                    // 0x10025510

    // cursor steps (inlined in the stock code); NULL at an end
    NLineNode* StepBack()
    {
        if (Current && Current->Prev)
        {
            Current = Current->Prev;
            if (CurIndex > 0)
                CurIndex--;
            return Current;
        }
        return NULL;
    }
    NLineNode* StepForward()
    {
        if (Current && Current->Next)
        {
            Current = Current->Next;
            if (CurIndex < Count - 1)
                CurIndex++;
            return Current;
        }
        return NULL;
    }
};
NW_ASSERT_SIZE(NLineList, 0x18);

// (C2 had an NStringArray here, held by the box as Strings; C3 dropped it: see NCMultiEdit.cpp)

class NCMultiEdit : public NControl
{
    NW_DECLARE_CLASS(NCMultiEdit, NControl, 0, 0x1026DF20)

    NCIMEManager* IMEManager;        // +0x1D4
    UTexture* BackTex;               // +0x1D8 "L2UI.EtcWnd.Edit_Back" (C1: "NWindow.chatback"), stretched over the box
    UTexture* WhiteTex;              // +0x1DC "NWindow.WhiteTexture", composing character
    // C2: not placed (no evidence for where it went): NStringArray Strings;            // +0x1B0 see above
    NLineList Lines;                 // +0x1E0
    TArray<INT> IntList;             // +0x1F8 new in C2 (name ours): an array of 4-byte elements (the destructor is
                                     //   TIntArray's, 0x1001FB70); constructed and destroyed, nothing else reads it
    FString Comp;                    // +0x204 IME composition, drawn at the caret
    TCHAR* TextBuf;                  // +0x210 GetText's result (new TCHAR[])
    INT CharH;                       // +0x214 height of the text   // C2: offset derived, no evidence
    INT LineGap;                     // +0x218 2: between the rows, and the text's left margin
    INT CaretTimer;                  // +0x21C SetTimer id (0xFA0D05, 500 ms)
    INT CaretLine;                   // +0x220 logical line of the caret
    INT CaretCol;                    // +0x224 character of the caret in its line
    UBOOL bCaretOn;                  // +0x228 caret blink phase
    INT VisibleRows;                 // +0x22C (Height - 2) / (CharH + 2)
    INT TopLine;                     // +0x230 first screen row shown (scroll position)
    INT NumLines;                    // +0x234 copy of Lines.Count
    INT CaretRow;                    // +0x238 screen row of the caret
    INT CaretX;                      // +0x23C px of the caret in its row
    UBOOL bCompRow;                  // +0x240 GetRowStart/GetRowEnd: the row breaks inside the
                                     //   composition
    // C2: not placed (no evidence for where it went): INT Unknown210;                  // +0x210 RecalcRows's argument
    _WORD Unknown214;                // +0x244 (0)
    _WORD Unknown216;                // +0x246 (0)
    INT CharW;                       // +0x248 width of a Hangul character   // C2: offset derived, no evidence
    INT CharH2;                      // +0x24C CharH again
    INT Unknown220;                  // +0x250 (0)
    NCPushButton* UpButton;          // +0x254 0xF0F01409
    NCPushButton* DownButton;        // +0x258 0xF0F01410
    NCSliderBar* ScrollBar;          // +0x25C 0xF0F01411
    INT IMECaretX;                   // +0x260 answer to NIM_GETCARET
    INT IMECaretY;                   // +0x264
    UBOOL bReading;                  // +0x268 a reading string is shown (NIM_READING)
    FString Reading;                 // +0x26C
    INT ReadingParam;                // +0x278 lParam of NIM_READING
    INT Unknown24C;                  // +0x27C (1)
    INT Unknown250;                  // +0x280 (1)
    INT Unknown254;                  // +0x284 (0)
    INT Unknown258;                  // +0x288 (0)
    INT Unknown25C;                  // +0x28C (16)
    INT TotalRows;                   // +0x290 screen rows of the whole text

    NCMultiEdit();                                                   // 0x10027550
    // ~NCMultiEdit                                                  // 0x100274b0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10021570
    virtual void SetFocus();                                         // 29 0x10021720 (shared)
    virtual void KillFocus();                                        // 30 0x10021760
    virtual void ShowWindow();                                       // 44 0x10021740
    virtual INT vfn032() { return 1; }                               // 33 0x10005fa0 (folded)
    virtual INT OnTimer(INT TimerID);                                // 36 0x10021780
    virtual INT OnCreate();                                          // 37 0x10023d60
    virtual INT OnDestroy();                                         // 38 0x100259b0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10025bc0
    virtual INT OnChar(WPARAM wParam, LPARAM lParam);                // 58 0x10027000
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 59 0x100276b0
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x100225f0
    // names ours
    virtual void UpdateScrollBar();                                  // 69 0x10022750
    virtual INT OnScrollUp();                                        // 70 0x100226a0
    virtual INT OnScrollDown();                                      // 71 0x100226e0
    virtual INT OnScrollBar(INT Value);                              // 72 0x10022730
    virtual INT OnComposition(WPARAM wParam, LPARAM lParam);         // 73 0x10025070
    virtual INT OnResult(WPARAM wParam, LPARAM lParam);              // 74 0x10025020
    virtual INT OnGetCaret(WPARAM wParam, LPARAM lParam);            // 75 0x10022550
    virtual INT OnReading(WPARAM wParam, LPARAM lParam);             // 76 0x10022580
    virtual INT OnAttribute(WPARAM wParam, LPARAM lParam);           // 77 0x100225d0

    // names ours; not inlined, the stock callers call them
    __declspec(noinline) void ScrollToCaret();                       // 0x100216c0
    __declspec(noinline) void SetRepeatDelay(INT Delay);             // 0x10022f10
    __declspec(noinline) void InitScrollBar();                       // 0x10022f40
    __declspec(noinline) INT CountRows(INT Line, UBOOL bComp);       // 0x10023260 (C2: bComp is a second argument)
    __declspec(noinline) void UpdateRowCount(INT Line, INT OldRows, UBOOL bComp); // 0x100234a0 (C1 0x1001c120)
    __declspec(noinline) void RecalcRows(INT Param);                 // 0x100234d0 (C1 0x1001c3d0)
    // new in C2 (names ours)
    __declspec(noinline) INT LineLength(FString Line);               // 0x100224c0 Line's length without its '\n' / '\r'
    __declspec(noinline) UBOOL IsThaiInputValid(UINT Ch);            // 0x10024110 OnChar: Thai spelling test at the caret
    __declspec(noinline) void MoveCaretDown();                       // 0x10027080 OnKeyDown's Down arrow
    __declspec(noinline) void InsertString(TCHAR* Str, UBOOL bResult); // 0x10023a20
    __declspec(noinline) void SetCaretX(INT Line, INT Col);          // 0x10024440
    __declspec(noinline) INT GetRowStart(INT Row);                   // 0x10024590
    __declspec(noinline) INT GetRowEnd(INT Start, INT Line);         // 0x10024930
    __declspec(noinline) INT GetRowLine(INT Row);                    // 0x10024d10
    __declspec(noinline) TCHAR* GetText();                           // 0x100251b0
    __declspec(noinline) void InsertChar(TCHAR Ch);                  // 0x10025540
    void SetText(const TCHAR* Text);                                 // 0x10025960
    void Clear();                                                    // 0x10025b30

    // composition painters of the client languages (names ours): Korean, Japanese, Taiwanese
    // and Chinese
    void PaintCompositionKR(FString Text, INT X, INT Y, NCGDevice* Device);           // 0x10022390
    void PaintCompositionJP(FString Text, INT CompStart, INT X, INT Y, INT RowNum,
                            NCGDevice* Device, INT* Out);                               // 0x1001e1b0
    void PaintCompositionTW(FString Text, INT CompStart, INT X, INT Y, INT RowNum,
                            NCGDevice* Device, INT* Out);                               // 0x1001e670
    void PaintCompositionCN(FString Text, INT CompStart, INT X, INT Y, INT RowNum,
                            NCGDevice* Device, INT* Out);                               // 0x1001f080
};
// NW_ASSERT_OFFSET(NCMultiEdit, Strings, 0x1B0);   // C2: field not mapped
NW_ASSERT_OFFSET(NCMultiEdit, Lines, 0x1E0);
NW_ASSERT_OFFSET(NCMultiEdit, Comp, 0x204);
NW_ASSERT_OFFSET(NCMultiEdit, CaretLine, 0x220);
NW_ASSERT_OFFSET(NCMultiEdit, CharW, 0x248);
NW_ASSERT_OFFSET(NCMultiEdit, Reading, 0x26C);
NW_ASSERT_OFFSET(NCMultiEdit, TotalRows, 0x290);
NW_ASSERT_SIZE(NCMultiEdit, 0x294);

// NCHtmlMultiEdit - the <multiedit> box of the HTML pages (UClass "CHtmlMultiEdit", 0x270): its
// own scroll controls (NCHtmlPushButton, NCHtmlSliderBar) inside the box's right edge, clipped
// like the page's other controls; the value is the text. Its scrolling counts Strings instead of
// the lines (always 1): the scroll buttons do nothing and scrolling down goes back to the top.
class NCHtmlMultiEdit : public NCMultiEdit
{
    NW_DECLARE_CLASS(NCHtmlMultiEdit, NCMultiEdit, 0, 0x10276710)

    NCHtmlPushButton* HtmlUpButton;  // +0x294 NID_MULTIEDIT_UP
    NCHtmlPushButton* HtmlDownButton; // +0x298 NID_MULTIEDIT_DOWN
    NCHtmlSliderBar* HtmlScrollBar;  // +0x29C NID_MULTIEDIT_BAR

    NCHtmlMultiEdit();                                               // 0x100457a0
    // ~NCHtmlMultiEdit                                              // 0x10031050 (C1 only: no C2 match) (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10041580
    virtual INT OnCreate();                                          // 37 0x10045db0
    virtual INT OnDestroy();                                         // 38 0x10042ab0
    virtual INT OnPaint(NCGDevice* Device);                          // C2: slot gone (was 50)
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x10042a60 (shared)
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x100415b0
    virtual const TCHAR* GetValue();                                 // 68 0x100415a0
    virtual void UpdateScrollBar();                                  // 69 0x100416f0
    virtual INT OnScrollUp();                                        // 70 0x10041660
    virtual INT OnScrollDown();                                      // 71 0x100416a0
};
NW_ASSERT_OFFSET(NCHtmlMultiEdit, HtmlUpButton, 0x294);
NW_ASSERT_SIZE(NCHtmlMultiEdit, 0x2A0);

#pragma pack(pop)