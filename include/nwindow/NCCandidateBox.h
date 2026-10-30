#pragma once
// NCCandidateBox - the IME candidate list (UClass "CCandidateBox", 0xFC): pages of PageSize
// candidates, RowHeight px each, the selected one on a white bar. The frame textures come in
// three layouts picked by Mode. Up/Down (keys or the buttons) move the selection a page at a
// time when crossing a page edge; PageUp/PageDown show or hide the box. Filled by the IMEs.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCPushButton;
class NCSliderBar;

// a string of the IMEs (names ours; 0x804 bytes): a candidate of NCCandidateBox, a clause of
// the Korean composition (Flag = target clause)
struct NImeText
{
    TCHAR Text[1024];          // +0x000
    INT Flag;                  // +0x800
};
NW_ASSERT_SIZE(NImeText, 0x804);

// NWM_COMMAND ids of the box's own controls
enum
{
    NID_CANDIDATE_SCROLLBAR = (INT)0xF0F01415,
    NID_CANDIDATE_UP = 0xF01416,
    NID_CANDIDATE_DOWN = 0xF01417,
};

class NCCandidateBox : public NCWnd
{
    NW_DECLARE_CLASS(NCCandidateBox, NCWnd, 0, 0x102623C8)

    INT PageSize;                    // +0xC8 candidates per page (9)
    UTexture* LeftEnable1;           // +0xCC frame, modes 0..2
    UTexture* LeftDisable1;          // +0xD0   (rows without a candidate)
    UTexture* LeftEnable2;           // +0xD4 frame, modes 3..5
    UTexture* LeftDisable2;          // +0xD8
    UTexture* Center1;               // +0xDC
    UTexture* Center2;               // +0xE0
    UTexture* Right1;                // +0xE4
    UTexture* Right2;                // +0xE8
    UTexture* WhiteTex;              // +0xEC selection bar
    INT RowHeight;                   // +0xF0 (15)
    TArray<NImeText> Items;          // +0xF4
    INT Selected;                    // +0x100
    INT PageStart;                   // +0x104 first candidate of the page shown
    INT Count;                       // +0x108 candidates
    INT MarginX;                     // +0x10C hit test rectangle of the rows   // C2: offset derived, no evidence
    INT MarginY;                     // +0x110   // C2: offset derived, no evidence
    INT UnknownE0;                   // +0x114
    DWORD TextColor;                 // +0x118 selected candidate (black)
    INT UnknownE8;                   // +0x11C
    // C2: not placed (no evidence for where it went): NCPushButton* UpButton;          // +0xF0
    // C2: not placed (no evidence for where it went): NCPushButton* DownButton;        // +0xF4
    // C2: not placed (no evidence for where it went): NCSliderBar* ScrollBar;          // +0xF8
    INT Mode;                        // +0x120 frame layout: 0..2, 3..4, 5

    NCCandidateBox(INT InPageSize = 9, INT InRowHeight = 15);        // 0x10004330
    // ~NCCandidateBox                                               // 0x100043d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100030a0
    virtual INT OnCreate();                                          // 37 0x10003120
    virtual INT OnDestroy();                                         // 38 0x100041f0
    virtual UBOOL PtInRect(INT X, INT Y, NRectF Rect);               // 40 0x10003370 (NCWnd's, overridden)
    virtual void SetWindowSize(FLOAT W, FLOAT H);   // 43 0x10031e40
    virtual void ShowWindow();                                       // 44 0x100412c0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10003570
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 59 0x10003260
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x10003ad0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x10003330
    // names ours
    // C3: the up and down buttons and the scroll bar are gone (the C2 OnUpButton / OnDownButton with them)
    virtual void NewVirtual68();  // 68 0x100033c0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual INT OnSelectPrev();                                      // 69 0x10003310 (name ours) SelectPrev, 1
    virtual INT OnSelectNext();                                      // 70 0x10003320 (name ours) SelectNext, 1
    virtual INT OnScrollBar(INT Pos) { return 1; }                   // 71 0x10003360 (folded)

    // slot 68 is this (NNewVirtuals.gen.cpp keeps NewVirtual68 as its placeholder, so it is a plain member
    // here and the callers go through the vtable, as the stock code does): the box is at least 70 px wide
    void SetSize(INT W, INT H);                                      // 0x100033c0 (name ours)
    void CallSetSize(INT W, INT H) { ::nw::VirtualCall<void>(this, 68, W, H); }

    __declspec(noinline) void SelectPrev();                          // 0x100031e0 (name ours)
    __declspec(noinline) void SelectNext();                          // 0x10003230 (name ours)
    // the IMEs fill it with these (names ours)
    void SetScroll(INT Total, INT Value);                            // 0x10002fe0 (C1 only: no C2 match) scroll bar, 9 a page
    void SetMode(INT InMode);                                        // 0x10003410 and the height
    void AddItem(const TCHAR* Text, INT Flag);                       // 0x10004140 Flag not stored (0)
    void Clear();                                                    // 0x100041c0
};
NW_ASSERT_OFFSET(NCCandidateBox, Items, 0xF4);
NW_ASSERT_OFFSET(NCCandidateBox, TextColor, 0x118);
NW_ASSERT_SIZE(NCCandidateBox, 0x124);

#pragma pack(pop)