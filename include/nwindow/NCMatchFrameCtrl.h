#pragma once
// NCMatchFrameCtrl - the title bar of the party matching window (UClass "CMatchFrameCtrl", 0x238; new in C3): an
// NCFrameCtrl that does nothing of its own: its close button closes the window as the base's does.

#include "nwindow/NCFrameCtrl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCMatchFrameCtrl : public NCFrameCtrl
{
    NW_DECLARE_CLASS(NCMatchFrameCtrl, NCFrameCtrl, 0, 0x1028EA50)

    NCMatchFrameCtrl();                                              // 0x100a79f0 (NCFrameCtrl's default constructor)
    // ~NCMatchFrameCtrl                                             // 0x100a7a40

    virtual INT OnClose();                                           // 70 0x100a4c30 (a thunk to NCFrameCtrl's)
};
NW_ASSERT_SIZE(NCMatchFrameCtrl, 0x238);

#pragma pack(pop)
