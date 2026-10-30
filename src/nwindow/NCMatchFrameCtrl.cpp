/*=============================================================================
    NCMatchFrameCtrl.cpp - the title bar of the party matching window. C3 stock: 0x100a79f0 (constructor),
    0x100a4c30 (OnClose), 0x100a7a40 (destructor).
=============================================================================*/
#include "nwindow/NCMatchFrameCtrl.h"

// 0x100a79f0 - NCFrameCtrl's default constructor (decorated), then this class' vtable
NCMatchFrameCtrl::NCMatchFrameCtrl()
{
}
NW_HOOK_CTOR(0x100a79f0, NCMatchFrameCtrl)

// 0x100a7a40
NW_HOOK_DTOR(0x100a7a40, NCMatchFrameCtrl)

// 0x100a4c30 (vslot 70) - a thunk: NCFrameCtrl's OnClose
INT NCMatchFrameCtrl::OnClose()
{
    return NCFrameCtrl::OnClose();
}
NW_HOOK_METHOD(0x100a4c30, NCMatchFrameCtrl, OnClose)

NW_CHECK_VTABLE(NCMatchFrameCtrl, 0x1018edf0, 72)
