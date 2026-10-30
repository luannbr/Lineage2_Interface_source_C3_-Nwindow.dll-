#pragma once
// NCListBox - scrolling list of colored text lines (UClass "CListBox", 0x1E8) with scroll up /
// down buttons and a slider on the right. At most MaxLines are kept (the oldest goes first);
// a click selects a line and reports its index (NWM_COMMAND, lParam = index).

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NString;
class NCPushButton;
class NCSliderBar;

struct NListItem
{
    NString* Text;             // Color = line color
    INT Param;
    INT Unknown8;              // +0x08 new in C3 (AddString 0x1001EFD0 stores 0; the element is 12 bytes)
};
NW_ASSERT_SIZE(NListItem, 0xC);

// NWM_COMMAND ids of the list's own controls
enum
{
    NID_LIST_SCROLLUP = (INT)0xF0F01406,
    NID_LIST_SCROLLDOWN = (INT)0xF0F01407,
    NID_LIST_SCROLLBAR = (INT)0xF0F01408,
};

class NCListBox : public NControl
{
    NW_DECLARE_CLASS(NCListBox, NControl, 0, 0x1026CFB0)

    UTexture* BackTex;               // +0x1D4 set by the owner
    UTexture* HighlightTex;          // +0x1D8 "NWindow.MenuHighlight" (not drawn here)
    UTexture* LineTex;               // +0x1DC (name ours) "L2UI_ch3.etc.textbackline" (OnCreate; not drawn here)
    TArray<NListItem> Items;         // +0x1E0
    NCPushButton* ScrollUpButton;    // +0x1EC
    NCPushButton* ScrollDownButton;  // +0x1F0
    NCSliderBar* ScrollBar;          // +0x1F4
    INT VisibleLines;                // +0x1F8 (10)
    INT MaxLines;                    // +0x1FC (100)
    INT TopIndex;                    // +0x200 first line shown
    INT NumLines;                    // +0x204
    INT Selected;                    // +0x208 -1 = none
    INT Unknown20C;                  // +0x20C (15; C2: 12), never read
    INT TextX;                       // +0x210
    INT TextY;                       // +0x214
    INT LineGap;                     // +0x218 lines are 12 + LineGap high
    UBOOL bAutoScroll;               // +0x21C keep the last line in view

    NCListBox();                                                     // 0x1001f1c0
    NCListBox(INT InMaxLines, INT InVisibleLines, INT Unused);       // 0x1001f300 (3rd never read) (ret 0xC)
    // ~NCListBox                                                    // 0x1001f2a0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1001e4b0
    virtual INT OnCreate();                                          // 37 0x1001eb40
    virtual void ShowWindow();                                       // 44 0x1001e850 (C3)
    virtual INT OnDestroy();                                         // 38 0x1001edd0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1001e860
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x1001e670
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x1001e6b0
    // names ours
    virtual INT HitTest(INT X, INT Y);                               // 69 0x1001e590
    virtual void AddString(const TCHAR* Text, DWORD Color, INT Param); // 70 0x1001efd0
    virtual void Clear();                                            // 71 0x1001ef30
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam) { return 1; } // 72 0x1000bd60 (folded)
    virtual INT ScrollUp();                                          // 73 0x1001e760
    virtual INT ScrollDown();                                        // 74 0x1001e7a0
    virtual INT SetTopIndex(INT Index);                              // 75 0x1001e7f0

    void UpdateScrollBar();                                          // 0x1001e810
    void SetRepeatDelay(INT Delay);                                  // 0x1001eaa0
    void InitScrollBar();                                            // 0x1001ead0
    INT GetItemParam(INT Index);                                     // 0x1001eb10
};
NW_ASSERT_OFFSET(NCListBox, Items, 0x1E0);
NW_ASSERT_OFFSET(NCListBox, ScrollBar, 0x1F4);
NW_ASSERT_OFFSET(NCListBox, Selected, 0x208);
NW_ASSERT_OFFSET(NCListBox, bAutoScroll, 0x21C);
NW_ASSERT_SIZE(NCListBox, 0x220);

#pragma pack(pop)