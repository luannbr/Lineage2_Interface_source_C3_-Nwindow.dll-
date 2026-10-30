/*=============================================================================
    NCMatchListCtrl.cpp - the list of the party matching window. C3 stock: 0x100a4c10 (WindowProc), 0x100a50c0
    (ShowWindow), 0x100a5720 (OnLButtonDblClick), 0x100a7b30 / 0x100a7bd0 (constructors), 0x100a7b80 (destructor).
=============================================================================*/
#include "nwindow/NCMatchListCtrl.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleFns.h"

// 0x100a7b30 - NCListCtrl's, with the scroll controls
NCMatchListCtrl::NCMatchListCtrl()
    : NCListCtrl(0)
{
}
NW_HOOK_CTOR(0x100a7b30, NCMatchListCtrl)

// 0x100a7bd0 (ret 0x10) - the rows, the visible rows, the columns and the unused one as NCListCtrl's constructor
// takes them, the scroll controls on
NCMatchListCtrl::NCMatchListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused)
    : NCListCtrl(InMaxRows, InVisibleRows, InMaxColumns, Unused, 0)
{
}
NW_HOOK_CTOR_ARGS(0x100a7bd0, NCMatchListCtrl, (INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused),
                  (InMaxRows, InVisibleRows, InMaxColumns, Unused))

// 0x100a7b80
NW_HOOK_DTOR(0x100a7b80, NCMatchListCtrl)

// 0x100a4c10 (vslot 25) - NCListCtrl's, as a flag (the stock body is shared with NCMatchPartyListCtrl's)
INT NCMatchListCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCListCtrl::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x100a4c10, NCMatchListCtrl, WindowProc)

// 0x100a50c0 (vslot 44) - NCListCtrl's (0x1001fe80: NCWnd's and the scroll range), then the three scroll controls are
// hidden again (the stock body is shared with NCMatchPartyListCtrl's)
void NCMatchListCtrl::ShowWindow()
{
    NCListCtrl::ShowWindow();   // 0x1001fe80
    ScrollUpButton->HideWindow();
    ScrollDownButton->HideWindow();
    ScrollBar->HideWindow();
}
NW_HOOK_METHOD(0x100a50c0, NCMatchListCtrl, ShowWindow)

// 0x100a5720 (vslot 71) - the row under the mouse is selected, and the server is asked to let the player join its
// room (the number of the room is the text of the row's first cell)
INT NCMatchListCtrl::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    INT Index = CallHitTest(LOWORD(lParam), HIWORD(lParam));
    Selected = Index;
    HoverRow = Index;
    if (Index >= 0 && Index < Rows.Num())
    {
        NListRow* Row = Rows(Index);
        L2ParamStack Params(10);
        Params.PushBack((void*)appAtoi(Row->Cells->Text));
        ((NConsoleForwardFns*)GConsole)->RequestJoinPartyRoom(Params);   // NConsoleWnd::RequestJoinPartyRoom (name ours)
    }
    return 1;
}
NW_HOOK_METHOD(0x100a5720, NCMatchListCtrl, OnLButtonDblClick)

NW_CHECK_VTABLE(NCMatchListCtrl, 0x1018f030, 76)
