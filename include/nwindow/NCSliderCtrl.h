#pragma once
// NCSliderCtrl - a slider (UClass "CSliderCtrl", 0x1BC; C2 only): a black bar with an
// NCThumbButton that follows the mouse while it is dragged; Pos runs from 0 to Max over the
// control's width. Whoever creates it sets Max, Pos and the thumb's size before OnCreate;
// nothing in this class reports the position to its parent.

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCThumbButton;

class NCSliderCtrl : public NControl
{
    NW_DECLARE_CLASS(NCSliderCtrl, NControl, 0, 0x1026F3E8)

    UTexture* BackTex;               // +0x1D4 "NWindow.BlackTexture"
    NCThumbButton* Thumb;            // +0x1D8 "NWindow.MenuHighlight" / "NWindow.WhiteTexture"
    INT Max;                         // +0x1DC (name ours) the largest Pos   // C2: offset derived, no evidence
    INT Pos;                         // +0x1E0 (name ours) the thumb's place, 0..Max   // C2: offset derived, no evidence
    INT ThumbW;                      // +0x1E4 (name ours)   // C2: offset derived, no evidence
    INT ThumbH;                      // +0x1E8 (name ours)   // C2: offset derived, no evidence

    NCSliderCtrl();                                                  // 0x10029e70
    // ~NCSliderCtrl                                                 // 0x10026270 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10029b50
    virtual INT OnCreate();                                          // 37 0x10029d50
    virtual INT OnDestroy();                                         // 38 0x10029c10
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10029cb0
    virtual INT OnThumbMove(INT X);                                  // 69 0x10029bb0 (name ours)
};
NW_ASSERT_OFFSET(NCSliderCtrl, BackTex, 0x1D4);
NW_ASSERT_OFFSET(NCSliderCtrl, Thumb, 0x1D8);
NW_ASSERT_OFFSET(NCSliderCtrl, Max, 0x1DC);
NW_ASSERT_OFFSET(NCSliderCtrl, ThumbH, 0x1E8);
NW_ASSERT_SIZE(NCSliderCtrl, 0x1EC);

#pragma pack(pop)
