#pragma once
// NCPartyRoomFrameCtrl - the title bar of the party room window (UClass "CPartyRoomFrameCtrl", 0x238; new in C3):
// closing it first asks the server to leave the room (what the Close button of the room does too), then closes the
// window as NCFrameCtrl does.

#include "nwindow/NCFrameCtrl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCPartyRoomFrameCtrl : public NCFrameCtrl
{
    NW_DECLARE_CLASS(NCPartyRoomFrameCtrl, NCFrameCtrl, 0, 0x1028D5C0)

    NCPartyRoomFrameCtrl();                                          // 0x100a7a90 (NCFrameCtrl's default constructor)
    // ~NCPartyRoomFrameCtrl                                         // 0x100a7ae0

    virtual INT OnClose();                                           // 70 0x100a66b0
};
NW_ASSERT_SIZE(NCPartyRoomFrameCtrl, 0x238);

#pragma pack(pop)
