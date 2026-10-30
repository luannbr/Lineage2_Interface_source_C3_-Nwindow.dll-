/*=============================================================================
    NCMatchPartyListCtrl.cpp - the list of the members of a party room. C3 stock: 0x100a8b00 / 0x100a8ba0
    (constructors), 0x100a8b50 (destructor), 0x100a6630 / 0x100a6670 (the member searches); WindowProc and
    ShowWindow are the code of NCMatchListCtrl's (0x100a4c10, 0x100a50c0: the linker kept one copy).
=============================================================================*/
#include "nwindow/NCMatchPartyListCtrl.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"

// 0x100a8b00 - NCListCtrl's, with the scroll controls
NCMatchPartyListCtrl::NCMatchPartyListCtrl()
    : NCListCtrl(0)
{
}
NW_HOOK_CTOR(0x100a8b00, NCMatchPartyListCtrl)

// 0x100a8ba0 (ret 0x10) - the rows, the visible rows, the columns and the unused one as NCListCtrl's constructor
// takes them
NCMatchPartyListCtrl::NCMatchPartyListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused)
    : NCListCtrl(InMaxRows, InVisibleRows, InMaxColumns, Unused, 0)
{
}
NW_HOOK_CTOR_ARGS(0x100a8ba0, NCMatchPartyListCtrl, (INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused),

                  (InMaxRows, InVisibleRows, InMaxColumns, Unused))

// 0x100a8b50
NW_HOOK_DTOR(0x100a8b50, NCMatchPartyListCtrl)

// stock body shared with NCMatchListCtrl's (0x100a4c10): NCListCtrl's, as a flag
INT NCMatchPartyListCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCListCtrl::WindowProc(Msg, wParam, lParam) != 0;
}

// stock body shared with NCMatchListCtrl's (0x100a50c0): NCListCtrl's, then the three scroll controls hidden again
void NCMatchPartyListCtrl::ShowWindow()
{
    NCListCtrl::ShowWindow();   // 0x1001fe80
    ScrollUpButton->HideWindow();
    ScrollDownButton->HideWindow();
    ScrollBar->HideWindow();
}

// 0x100a6630 (ret 4) - the index of the row whose first cell holds the object id, -1 when no row does
INT NCMatchPartyListCtrl::FindMemberIndex(INT ObjectID)
{
    for (INT i = 0; i < Rows.Num(); i++)
    {
        NListRow* Row = Rows(i);
        if (Row && Row->Cells->Param1 == ObjectID)
            return i;
    }
    return -1;
}
NW_HOOK_METHOD(0x100a6630, NCMatchPartyListCtrl, FindMemberIndex)

// 0x100a6670 (ret 4) - the row itself, NULL when no row has it
NListRow* NCMatchPartyListCtrl::FindMemberRow(INT ObjectID)
{
    for (INT i = 0; i < Rows.Num(); i++)
    {
        NListRow* Row = Rows(i);
        if (Row && Row->Cells->Param1 == ObjectID)
            return Row;
    }
    return NULL;
}
NW_HOOK_METHOD(0x100a6670, NCMatchPartyListCtrl, FindMemberRow)

NW_CHECK_VTABLE(NCMatchPartyListCtrl, 0x1018f3f0, 76)
