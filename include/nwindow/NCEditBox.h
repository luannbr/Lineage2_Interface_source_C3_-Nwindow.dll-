#pragma once
// NCEditBox - single line text input (UClass "CEditBox", 0x238): the chat line, login fields,
// amounts in dialogs... The text is kept in pieces around the caret: Left (before it), Comp
// (the IME composition, drawn at the caret) and Right (after it); arrows move characters
// between Left and Right. The text may grow ExtraWidth px past the box (0: the box width)
// and scrolls from ViewStart. A hidden NCIMEManager child gives it an IME, whose private
// messages (NIM_*) arrive in 65..72.
//
// Type: 0 plain, 1 chat (" first and Up/Down are reported to the parent), 2 password (drawn
// as '*'), 3 digits only. The parent gets NWM_COMMAND: 0 Enter on an empty box, 1 Enter,
// 3 '"' typed first, 4 Up, 5 Down (chat).

#include "nwindow/NControl.h"
#include "nwindow/NCGDevice.h"   // ThaiCharacter
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCIMEManager;

class NCEditBox : public NControl
{
    NW_DECLARE_CLASS(NCEditBox, NControl, 0, 0x10265C70)

    NCIMEManager* IMEManager;        // +0x1D4
    UTexture* Edit1;                 // +0x1D8 focused frame: left, middle, right   // C2: offset derived, no evidence
    UTexture* Edit2;                 // +0x1DC   // C2: offset derived, no evidence
    UTexture* Edit3;                 // +0x1E0   // C2: offset derived, no evidence
    UTexture* EditDisable1;          // +0x1E4 frame without the focus   // C2: offset derived, no evidence
    UTexture* EditDisable2;          // +0x1E8   // C2: offset derived, no evidence
    UTexture* EditDisable3;          // +0x1EC   // C2: offset derived, no evidence
    UTexture* HighlightTex;          // +0x1F0 new in C2 (name ours): "L2UI.MacroWnd.Macro_Line_HighLight"   // C2: offset derived, no evidence
    INT Type;                        // +0x1F4 see above (C1: +0x1BC)
    INT Unknown1C8;                  // +0x1F8 new in C2 (name ours): the constructor's fifth argument, 1 when
                                     //   GLanguageType is 1; NIM_CURSORPOS keeps only printable codes when it is 1
    INT Unknown1CC;                  // +0x1FC new in C2 (1): OnPaint draws the caret only while it is set (nothing
                                     //   clears it in the stock code we know)
    UBOOL bCanInsert;                // +0x200 the text is narrower than MaxWidth - 12
    UBOOL bCaretOn;                  // +0x204 caret blink phase
    INT Unknown1D8;                  // +0x208 new in C2 (0): OnPaint draws HighlightTex over the box while it is set
                                     //   (the macro line editor of the new macro windows)
    INT CaretTimer;                  // +0x20C SetTimer id (500 ms)
    UTexture* BackTex;               // +0x210 constructor argument (not drawn here)
    UTexture* WhiteTex;              // +0x214 composing character background
    UBOOL bIMEPosFixed;              // +0x218 constructor argument: IME window placement
    UBOOL bReading;                  // +0x21C a reading string is shown (NIM_READING)
    INT Unknown1F0;                  // +0x220 (C1 Unknown1DC)
    INT ViewStart;                   // +0x224 first character of Left drawn
    INT ExtraWidth;                  // +0x228 px the text may grow past the box
    INT MaxWidth;                    // +0x22C Width + ExtraWidth (OnPaint)
    INT ViewStart2;                  // +0x230 copy of ViewStart restored by End
    INT MaxLen;                      // +0x234 new in C2 (name ours): the most characters the text may hold, 0 = no limit
    FString Left;                    // +0x238
    FString Right;                   // +0x244
    FString Commit;                  // +0x250 new in C3 (name ours): CommitText's result, Left + Right
    TArray<ThaiCharacter> ThaiLeft;  // +0x25C new in C2 (name ours): the Thai clusters of Left (GLanguageType 5)
    TArray<ThaiCharacter> ThaiRight; // +0x268   and of Right
    FString Comp;                    // +0x274
    FString Reading;                 // +0x280
    DWORD CaretX;                    // +0x28C caret position in the box (OnPaint)
    INT Unknown224;                  // +0x290
    INT IMECaretX;                   // +0x294 answer to NIM_GETCARET
    INT IMECaretY;                   // +0x298
    INT CursorPos;                   // +0x29C NIM_CURSORPOS
    INT ReadingParam;                // +0x2A0 lParam of NIM_READING

    NCEditBox(INT InType = 0, const TCHAR* BackTexName = NULL, UBOOL bInIMEPosFixed = 1,
              INT InExtraWidth = 0, INT InUnknown1C8 = 0);           // 0x1000d5d0 (C2: five arguments)
    // ~NCEditBox                                                    // 0x1000f5e0

    // WindowProc: the stock body is shared with NCIMEManager (hooked there)
    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10013cb0
    virtual void SetFocus();                                         // 29 0x1000b450
    virtual void KillFocus();                                        // 30 0x1000e380
    virtual INT vfn032() { return 1; }                               // 33 0x10005fa0 (folded)
    virtual void NewVirtual35();                                     // 35 0x1000f5a0 IsEnglishInput (NCWnd's is the folded no-op placeholder)
    virtual INT OnTimer(INT TimerID);                                // 36 0x1000b410
    virtual INT OnCreate();                                          // 37 0x1000d720
    virtual INT OnDestroy();                                         // 38 0x1000bfe0
    virtual void ShowWindow();                                       // 44 0x100412c0 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1000d930
    virtual INT OnChar(WPARAM wParam, LPARAM lParam);                // 58 0x1000d3b0
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 59 0x1000e510
    // names ours
    virtual const TCHAR* CommitText();                               // 69 0x1000ae60
    virtual void ClearText();                                        // 70 0x1000afa0
    virtual INT OnComposition(WPARAM wParam, LPARAM lParam);         // 71 0x1000b710
    virtual INT OnResult(WPARAM wParam, LPARAM lParam);              // 72 0x1000b470
    virtual INT OnAttribute(WPARAM wParam, LPARAM lParam);           // 73 0x1000bab0
    virtual INT OnReading(WPARAM wParam, LPARAM lParam);             // 74 0x1000bc10
    virtual INT On14(WPARAM wParam, LPARAM lParam) { return 1; }     // 75 0x1000bd60 (folded)
    virtual INT OnGetCaret(WPARAM wParam, LPARAM lParam);            // 76 0x1000bd70
    virtual INT OnEndComposition(WPARAM wParam, LPARAM lParam);      // 77 0x1000bda0
    // slot 78 is NIM_CURSORPOS's handler, OnCursorPos below (hooked at its address; the placeholder keeps the name
    // the generated NNewVirtuals file knows, WindowProc calls the slot through nw::VirtualCall)
    virtual void NewVirtual78();  // 78 0x1000bdd0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    INT IsEnglishInput();                                    // 0x1000f5a0 (name ours) Unknown1C8 == 1; vslot 35 of this class
    INT OnCursorPos(WPARAM wParam, LPARAM lParam);           // 0x1000bdd0 CursorPos = wParam (C1: slot 72)

    // names ours
    void SetText(const TCHAR* InText);                               // 0x1000ae50 Left only
    void SetMaxLength(INT InMaxLen);                                 // 0x1000a2a0 (C2 only; name ours) the longest text, when positive
    void InsertText(const TCHAR* InText, UBOOL bAppend);             // 0x1000ce80 (C1 0x10008c40)
    void DeleteChar(INT Index);                                      // 0x1000d0d0 (C1 0x10008d30)
    // new in C2 (names ours); NCCalcEditCtrl and the HTML box call ReformatNumber too
    void ReformatNumber();                                           // 0x1000afe0 Type 3: thousands separators
    UBOOL IsThaiInputValid(UINT Ch);                                 // 0x1000b140 Thai spelling test, Thai keyboard
    // not inlined: the stock callers call it (and it destroys the by-value argument itself)
    __declspec(noinline) INT GetTextWidth(FString Text);             // 0x1000af30
    void SetType(INT InType);                                        // 0x1000b440
    INT GetCompCaretX();                                             // 0x1000bdf0 (Japanese)

    // composition drawing of the Korean client (names ours)
    void PaintComposingKR(INT X, INT Y, INT TextW, INT CompW, NCGDevice* Device);   // 0x1000a2c0
    void PaintCompositionKR(INT X, INT Y, INT TextW, INT Offset, NCGDevice* Device); // 0x1000aa30
    // the same for the other IMEs (names ours)
    void PaintComposingTW(INT X, INT Y, INT TextW, INT CompW, INT ClauseW, INT ReadingW,
                          NCGDevice* Device);                                        // 0x10008180
    void PaintCompositionJP(INT X, INT Y, INT TextW, INT Offset, NCGDevice* Device); // 0x1000a800
    void PaintCompositionTW(INT X, INT Y, INT TextW, INT* ClauseW, NCGDevice* Device); // 0x1000abe0
    void PaintComposingJP(INT X, INT Y, INT TextW, INT CompW, NCGDevice* Device);   // 0x1000c0a0
    void PaintComposingCN(INT X, INT Y, INT TextW, INT CompW, NCGDevice* Device);   // 0x1000c370
    void PaintCompositionCN(INT X, INT Y, INT TextW, NCGDevice* Device);            // 0x1000ca60
};
NW_ASSERT_OFFSET(NCEditBox, IMEManager, 0x1D4);
NW_ASSERT_OFFSET(NCEditBox, Type, 0x1F4);
NW_ASSERT_OFFSET(NCEditBox, CaretTimer, 0x20C);
NW_ASSERT_OFFSET(NCEditBox, WhiteTex, 0x214);
NW_ASSERT_OFFSET(NCEditBox, ThaiLeft, 0x25C);
NW_ASSERT_OFFSET(NCEditBox, Comp, 0x274);
NW_ASSERT_OFFSET(NCEditBox, ViewStart, 0x224);
NW_ASSERT_OFFSET(NCEditBox, Left, 0x238);
NW_ASSERT_OFFSET(NCEditBox, Commit, 0x250);
NW_ASSERT_OFFSET(NCEditBox, Reading, 0x280);
NW_ASSERT_OFFSET(NCEditBox, CaretX, 0x28C);
NW_ASSERT_SIZE(NCEditBox, 0x2A4);

#pragma pack(pop)