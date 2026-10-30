#pragma once
// NCScrollWnd - window whose content scrolls vertically (UClass "CScrollWnd", 0xCC): base of
// the HTML viewers, item windows and quest/notice info windows. ScrollY is the content offset
// (0 at the top, down to MinScrollY = Height - content height); Delta* is how much the last
// change moved it, for the subclasses that repaint incrementally. Scroll up / down buttons and
// a slider sit on the right edge; the keyboard (PageUp/PageDown/arrows) and the wheel scroll.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCPushButton;
class NCSliderBar;

#if NW_HOOK_MODE
#define GNextScrollCtrlID NW_GLOBAL(INT, 0x101EBECC)   // ids of the scroll controls, from 0x1200 (C3 address)
#else
extern INT GNextScrollCtrlID;
#endif

class NCScrollWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCScrollWnd, NCWnd, 0, 0x1026E998)

    INT ScrollX;                     // +0xC8 never changed here
    INT ScrollY;                     // +0xCC <= 0
    INT DeltaX;                      // +0xD0 previous - current, after each scroll
    INT DeltaY;                      // +0xD4
    INT PageStep;                    // +0xD8 (100) PageUp / PageDown
    INT LineStep;                    // +0xDC (15) arrows, buttons, wheel (3 per notch)
    INT MinScrollY;                  // +0xE0 Height - content height (SetContentHeight)
    NCPushButton* ScrollUpButton;    // +0xE4
    NCPushButton* ScrollDownButton;  // +0xE8
    INT ScrollUpID;                  // +0xEC
    INT ScrollDownID;                // +0xF0
    NCSliderBar* ScrollBar;          // +0xF4
    INT ScrollBarID;                 // +0xF8
    INT Unknown0C8;                  // +0xFC (12; named after its C1 offset)   // C2: offset derived, no evidence

    NCScrollWnd();                                                   // 0x100294f0
    // ~NCScrollWnd                                                  // 0x10010250

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10028ab0
    virtual INT OnCreate();                                          // 37 0x10029250
    virtual INT OnDestroy();                                         // 38 0x10028d70
    virtual INT OnPaint(NCGDevice* Device) { return 1; }             // 56 0x10003360 (folded)
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 59 0x10028e70
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x10029150
    // C3: a virtual of this class (C2 overrode NCWnd's SetSize): the size as DWORDs, then the controls follow the
    // right edge (name ours)
    virtual void SetSize(DWORD W, DWORD H);                          // 68 0x10028b50
    virtual INT ScrollUp();                                          // 69 0x10028fb0
    virtual INT ScrollDown();                                        // 70 0x10029040
    virtual INT OnScrollBar(INT Value);                              // 71 0x100290e0 (the slider's value, a line step multiple)
    // not virtual: kept so that NNewVirtuals.gen.cpp (generated: the placeholders of slots 68 and 71) still compiles
    void NewVirtual68();
    void NewVirtual71();

    void ShowBars(INT bShow);                                        // 0x10028bd0 (name ours)
    void SetContentHeight(INT ContentHeight);                        // 0x10028c40
    // C3 (name ours): the other setter of the content height (the tree, lists and item windows call it): a bar whose
    // thumb already passes Height - 30 is reset to 0, else the range is the whole content height
    void SetContentHeightEx(INT ContentHeight);                      // 0x10028ca0
    void SetRepeatDelay(INT Delay);                                  // 0x10028d40
};
NW_ASSERT_OFFSET(NCScrollWnd, ScrollY, 0xCC);
NW_ASSERT_OFFSET(NCScrollWnd, MinScrollY, 0xE0);
NW_ASSERT_OFFSET(NCScrollWnd, ScrollBar, 0xF4);
NW_ASSERT_SIZE(NCScrollWnd, 0x100);

#pragma pack(pop)