/*=============================================================================
    NCThumbButton.cpp - the handle of the slider control (C2 only). Stock: 0x100013f0 ..
    0x1000143d, 0x10002900 .. 0x10002a2f (between NCExButton's and NCPushButton's constructors).
=============================================================================*/
#include "nwindow/NCThumbButton.h"

// 0x10002d20 - NCButton's, nothing more
NCThumbButton::NCThumbButton()
{
}
NW_HOOK_CTOR(0x10002d20, NCThumbButton)

// 0x10002d70
NW_HOOK_DTOR(0x10002d70, NCThumbButton)

// 0x10002dc0 - NCButton's with the textures
NCThumbButton::NCThumbButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, UBOOL bInAcceptKey)
    : NCButton(NormalTexName, PushedTexName, bInAcceptKey)
{
}
NW_HOOK_CTOR_ARGS(0x10002dc0, NCThumbButton, (const TCHAR* A, const TCHAR* B, UBOOL C), (A, B, C))


// 0x100014c0 (vslot 61) - dragged: the pointer's x goes to the slider
INT NCThumbButton::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if (wParam & MK_LBUTTON)
        NotifyParent(LOWORD(lParam));
    return 1;
}
NW_HOOK_METHOD(0x100014c0, NCThumbButton, OnMouseMove)

// 0x100014a0 (vslot 62)
INT NCThumbButton::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    bReleased = 0;
    return 1;
}
NW_HOOK_METHOD(0x100014a0, NCThumbButton, OnLButtonDown)

// 0x100014e0 (vslot 63)
INT NCThumbButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    bReleased = 1;
    return 1;
}
NW_HOOK_METHOD(0x100014e0, NCThumbButton, OnLButtonUp)

NW_CHECK_VTABLE(NCThumbButton, 0x10176988, 69)
