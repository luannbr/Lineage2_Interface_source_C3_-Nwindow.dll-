#pragma once
// NCSliderBar - vertical scroll bar thumb (UClass "CSliderBar", 0x1CC), used by the lists,
// edit boxes and scroll windows. SetRange(Total, Page) sizes the thumb (Height * Page /
// Total, at least 15 px); dragging it sets Value (first visible item) and reports it to the
// parent (NWM_COMMAND, lParam = Value).

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCSliderBar : public NControl
{
    NW_DECLARE_CLASS(NCSliderBar, NControl, 0, 0x1026EEB8)

    // C3 layout (SetRange 0x10029550, SetValue 0x10029600, OnLButtonDown 0x10029670, OnMouseMove 0x100296e0, the
    // constructor 0x10029990): C2 had LastY (the mouse of the drag) at +0x1F0 and the fields from Total on one place
    // lower; C3 drags by the offset of the grab inside the thumb
    INT ThumbSize;             // +0x1D4 px (at least 15; the whole height when there is nothing to scroll)
    INT ThumbPos;              // +0x1D8 px from the top
    INT GrabOffset;            // +0x1DC (name ours) where the thumb was grabbed, from its top (OnLButtonDown)
    INT Total;                 // +0x1E0 items (SetRange's first argument; not set by the constructor)
    INT TrackLen;              // +0x1E4 px the thumb can travel (Height - ThumbSize; not set by the constructor)
    INT Value;                 // +0x1E8 first visible item
    INT Page;                  // +0x1EC visible items
    UBOOL bDragging;           // +0x1F0 set by OnLButtonDown on the thumb when there is something to scroll
    UTexture* ThumbTopTex;     // +0x1F4 15 x 8
    UTexture* ThumbMidTex;     // +0x1F8 stretched
    UTexture* ThumbBottomTex;  // +0x1FC 15 x 8

    NCSliderBar();                                                   // 0x10029990
    NCSliderBar(const TCHAR* TopTexName, const TCHAR* MidTexName,
                const TCHAR* BottomTexName);                         // 0x10029a80 (anonymous, the console lane's)
    // ~NCSliderBar                                                  // 0x10029a30

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100127a0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10029810
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x100296e0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x10029670
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100296c0

    // names ours
    void SetRange(INT InTotal, INT InPage);                          // 0x10029550
    void SetValue(INT NewValue);                                     // 0x10029600
    INT Reset();                                                     // 0x100297e0 (returns 0: the stock callers use eax)
    void SetScrollRange(INT InTotal, INT InPage);                    // 0x10029970 (jmp SetRange)
    void SetScrollPos(INT NewValue);                                 // 0x10029980 (jmp SetValue)
    // an empty body the multi-line edit boxes call on their bar (name ours; the "ret 4" many
    // empty one-argument methods were folded into, too small to hook)
    void DoNothing(INT A) {}                                         // 0x1012f080 (folded)
};
NW_ASSERT_OFFSET(NCSliderBar, GrabOffset, 0x1DC);
NW_ASSERT_OFFSET(NCSliderBar, TrackLen, 0x1E4);
NW_ASSERT_OFFSET(NCSliderBar, Value, 0x1E8);
NW_ASSERT_OFFSET(NCSliderBar, bDragging, 0x1F0);
NW_ASSERT_OFFSET(NCSliderBar, ThumbBottomTex, 0x1FC);
NW_ASSERT_SIZE(NCSliderBar, 0x200);

#pragma pack(pop)