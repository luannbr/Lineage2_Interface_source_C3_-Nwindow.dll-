#pragma once
// NCThumbButton - the handle of an NCSliderCtrl (UClass "CThumbButton", 0x264, a plain button
// in size; C2 only): pressing it only marks it pressed (no sound, no hit test), and moving
// the mouse with the left button held reports the pointer's x to the parent (NWM_COMMAND with
// the button's ID).

#include "nwindow/NCButton.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCThumbButton : public NCButton
{
    NW_DECLARE_CLASS(NCThumbButton, NCButton, 0, 0x10261E98)

    NCThumbButton();                                                 // 0x10002d20
    NCThumbButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, UBOOL bInAcceptKey); // 0x10002dc0
    // ~NCThumbButton                                                // 0x10002d70

    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x100014c0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100014a0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100014e0
};
NW_ASSERT_SIZE(NCThumbButton, 0x29C);

#pragma pack(pop)
