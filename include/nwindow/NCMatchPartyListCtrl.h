#pragma once
// NCMatchPartyListCtrl - the list of the members of a party room (UClass "CMatchPartyListCtrl", 0x24C; new in C3):
// NCPartyRoomWnd's. NCListCtrl's layout, the scroll controls hidden as the matching list's.
//
// Row data (NCPartyRoomWnd::SetMembers / UpdateMember): Cells[0] the name (Param1 the object id of the member),
// Cells[1] the class icon (no text; Param2 of the row the class id), Cells[2] the level range, Cells[3] the place,
// Cells[4] the role.

#include "nwindow/NCListCtrl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCMatchPartyListCtrl : public NCListCtrl
{
    NW_DECLARE_CLASS(NCMatchPartyListCtrl, NCListCtrl, 0, 0x1028DAE8)

    NCMatchPartyListCtrl();                                          // 0x100a8b00
    NCMatchPartyListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused); // 0x100a8ba0 (ret 0x10)
    // ~NCMatchPartyListCtrl                                         // 0x100a8b50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a4c10 (the code of NCMatchListCtrl's)
    virtual void ShowWindow();                                       // 44 0x100a50c0 (the code of NCMatchListCtrl's)

    // names ours: the row of a member by his object id (the first cell's Param1), -1 / NULL when there is none
    INT FindMemberIndex(INT ObjectID);                               // 0x100a6630 (ret 4)
    NListRow* FindMemberRow(INT ObjectID);                           // 0x100a6670 (ret 4)
};
NW_ASSERT_SIZE(NCMatchPartyListCtrl, 0x24C);

#pragma pack(pop)
