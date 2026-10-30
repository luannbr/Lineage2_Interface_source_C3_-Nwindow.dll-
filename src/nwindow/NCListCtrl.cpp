/*=============================================================================
    NCListCtrl.cpp - list with columns: cells, rows, column headers (NCHeaderCtrl) and the
    list itself. Stock (C3): 0x1001f3c0 .. 0x1001f6d0 (cells, column info, rows), 0x1001f6e0 ..
    0x1001f98d (the headers, the list's small members from 0x1001f990), 0x1001fb00 ..
    0x10021540 (the list).
=============================================================================*/
#include "nwindow/NCListCtrl.h"
#include "nwindow/NCListBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCrt.h"

#include <wchar.h>
#include <new>

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_Click = 0, SOUND_ClickDisabled = 2 };

// the rows are 17 pixels high (C3; C2: 12 + LineGap)
enum { ROW_Height = 17 };

/*-----------------------------------------------------------------------------
    NListCell.
-----------------------------------------------------------------------------*/

// 0x1001f3c0
void NListCell::SetText(const TCHAR* InText)
{
    if (InText && *InText)
        appStrcpy(Text, InText);
    else
        Text[0] = 0;
}
NW_HOOK_METHOD(0x1001f3c0, NListCell, SetText)

// 0x1001f3f0
void NListCell::SetNumber(INT Value)
{
    appSprintf(Text, TEXT("%d"), Value);
}
NW_HOOK_METHOD(0x1001f3f0, NListCell, SetNumber)

// 0x1001f410 - a NULL texture keeps the old one (and its size)
void NListCell::SetIcon(UTexture* InIcon, INT W, INT H)
{
    if (InIcon)
    {
        Icon = InIcon;
        IconW = W;
        IconH = H;
    }
}
NW_HOOK_METHOD(0x1001f410, NListCell, SetIcon)

// 0x1001f440
void NListCell::SetText2(const TCHAR* InText)
{
    if (InText && *InText)
        appStrcpy(Text2, InText);
    else
        Text2[0] = 0;
}
NW_HOOK_METHOD(0x1001f440, NListCell, SetText2)

// 0x1001f470
void NListCell::SetParams(INT InParam1, INT InParam2, INT InParam3)
{
    Param1 = InParam1;
    Param2 = InParam2;
    Param3 = InParam3;
}
NW_HOOK_METHOD(0x1001f470, NListCell, SetParams)

// 0x1001f4a0
void NListCell::SetColor(DWORD InColor)
{
    Color = InColor;
}
NW_HOOK_METHOD(0x1001f4a0, NListCell, SetColor)

/*-----------------------------------------------------------------------------
    NListColumnInfo.
-----------------------------------------------------------------------------*/

// 0x1001f4b0 - the title is a SysString
void NListColumnInfo::SetTitleID(INT TextID)
{
    if (*NCObject::SysString(TextID))
        appStrcpy(Title, *NCObject::SysString(TextID));
    else
        Title[0] = 0;
}
NW_HOOK_METHOD(0x1001f4b0, NListColumnInfo, SetTitleID)

// 0x1001f510
void NListColumnInfo::SetTitle(const TCHAR* InTitle)
{
    if (InTitle && *InTitle)
        appStrcpy(Title, InTitle);
    else
        Title[0] = 0;
}
NW_HOOK_METHOD(0x1001f510, NListColumnInfo, SetTitle)

// 0x1001f540 - C3: the normal state has three textures (the caps and the middle), picked by Index (0, 1, 2; another
// Index changes nothing)
void NListColumnInfo::SetNormalTex(const TCHAR* Name, INT Index)
{
    if (Index == 0)
    {
        if (Name && *Name)
            appStrcpy(NormalTex[0], Name);
        else
            NormalTex[0][0] = 0;
    }
    else if (Index == 1)
    {
        if (Name && *Name)
            appStrcpy(NormalTex[1], Name);
        else
            NormalTex[1][0] = 0;
    }
    else if (Index == 2)
    {
        if (Name && *Name)
            appStrcpy(NormalTex[2], Name);
        else
            NormalTex[2][0] = 0;
    }
}
NW_HOOK_METHOD(0x1001f540, NListColumnInfo, SetNormalTex)

// 0x1001f5e0 - the same for the pushed state
void NListColumnInfo::SetPushedTex(const TCHAR* Name, INT Index)
{
    if (Index == 0)
    {
        if (Name && *Name)
            appStrcpy(PushedTex[0], Name);
        else
            PushedTex[0][0] = 0;
    }
    else if (Index == 1)
    {
        if (Name && *Name)
            appStrcpy(PushedTex[1], Name);
        else
            PushedTex[1][0] = 0;
    }
    else if (Index == 2)
    {
        if (Name && *Name)
            appStrcpy(PushedTex[2], Name);
        else
            PushedTex[2][0] = 0;
    }
}
NW_HOOK_METHOD(0x1001f5e0, NListColumnInfo, SetPushedTex)

/*-----------------------------------------------------------------------------
    NListRow.
-----------------------------------------------------------------------------*/

// 0x10020b80 - the tool tip info is constructed first; the cells come from GMalloc in one block (no array header:
// they have no destructor); Param3 is left as allocated
NListRow::NListRow(INT NumCells)
{
    NListCell* Block = (NListCell*)GMalloc->Malloc(NumCells * sizeof(NListCell), TEXT("new"));
    if (Block)
    {
        for (INT i = 0; i < NumCells; i++)
            new (&Block[i]) NListCell();
    }
    Cells = Block;
    Text[0] = 0;
    Param1 = -1;
    Param2 = -1;
}
NW_HOOK_CTOR_ARGS(0x10020b80, NListRow, (INT A), (A))

// 0x10020c60 - the cells are freed, then the tool tip goes
NListRow::~NListRow()
{
    if (Cells)
    {
        GMalloc->Free(Cells);
        Cells = NULL;
    }
}
NW_HOOK_DTOR(0x10020c60, NListRow)

// 0x1001f680
void NListRow::SetText(const TCHAR* InText)
{
    if (InText && *InText)
        appStrcpy(Text, InText);
    else
        Text[0] = 0;
}
NW_HOOK_METHOD(0x1001f680, NListRow, SetText)

// 0x1001f6b0
void NListRow::SetParams(INT InParam1, INT InParam2, INT InParam3)
{
    Param1 = InParam1;
    Param2 = InParam2;
    Param3 = InParam3;
}
NW_HOOK_METHOD(0x1001f6b0, NListRow, SetParams)

// C1 0x10018910: no stock function in C2 (the tool tip is an NToolTipInfo the skin draws): the first line of
// text, NULL when there is none
const TCHAR* NListRow::GetToolTip()
{
    const TCHAR* Tip = *ToolTip.Name;
    if (!Tip || !*Tip)
        return NULL;
    return Tip;
}

/*-----------------------------------------------------------------------------
    NCHeaderCtrl.
-----------------------------------------------------------------------------*/

// 0x10020e00 - NControl's constructor is inlined; the textures set by SetColumnInfo and Title are left as allocated
NCHeaderCtrl::NCHeaderCtrl()
{
    WholeNormalTex = NULL;
    WholePushedTex = NULL;
    bNumeric = 0;
    ColumnWidth = 0;
    ColumnHeight = 0;
    bReleased = 1;
    bClickable = 1;
    bAscending = 1;
}
NW_HOOK_CTOR(0x10020e00, NCHeaderCtrl)

// 0x10020ea0
NW_HOOK_DTOR(0x10020ea0, NCHeaderCtrl)

// stock body shared with NCButton's (0x100127a0)
INT NCHeaderCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x1001f790 (vslot 38) - guarded
INT NCHeaderCtrl::OnDestroy()
{
    guard(NCHeaderCtrl::OnDestroy);
    NControl::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1001f790, NCHeaderCtrl, OnDestroy)

// 0x1001fe90 (vslot 56) - the header's own texture stretched over it (67 x 15 texels), or three: a left cap of 8
// pixels, the middle and a right cap of 8 pixels (19 texels high), one set per state; the title centered
INT NCHeaderCtrl::OnPaint(NCGDevice* Device)
{
    guard(NCHeaderCtrl::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    UTexture* Left = NULL;
    UTexture* Middle = NULL;
    UTexture* Right = NULL;
    if (bReleased)
    {
        if (WholeNormalTex)
            Device->DrawStretchedTex(0, 0, (INT)Width, (INT)Height, 0, 0, 67, 15, WholeNormalTex, 0xFF);
        else
        {
            Left = NormalTex[0];
            Middle = NormalTex[1];
            Right = NormalTex[2];
        }
    }
    else
    {
        if (WholePushedTex)
            Device->DrawStretchedTex(0, 0, (INT)Width, (INT)Height, 0, 0, 67, 15, WholePushedTex, 0xFF);
        else
        {
            Left = PushedTex[0];
            Middle = PushedTex[1];
            Right = PushedTex[2];
        }
    }
    if (Left)
        Device->DrawStretchedTex(0, 0, 8, (INT)Height, 0, 0, 8, 19, Left, 0xFF);
    if (Middle)
        Device->DrawStretchedTex(8, 0, (INT)(Width - 16.0f), (INT)Height, 0, 0, 8, 19, Middle, 0xFF);
    if (Right)
        Device->DrawStretchedTex((INT)(Width - 8.0f), 0, 8, (INT)Height, 0, 0, 8, 19, Right, 0xFF);

    const TCHAR* Text = Title;
    if (Text && *Text)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0, L2FONT_Normal);
        INT X = (INT)((DOUBLE)Width * 0.5 - (DOUBLE)(Size.cx / 2));
        INT Y = (INT)((DOUBLE)Height * 0.5 - (DOUBLE)(Size.cy / 2));
        Device->DrawText(X, Y + 1, 0xFFE6DCBE, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1001fe90, NCHeaderCtrl, OnPaint)

// 0x1001f6e0 (vslot 62) - pushed; the click sound plays wherever the button went down
INT NCHeaderCtrl::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)))
        bReleased = 0;
    PlayInterfaceSound(IsEnabled() ? SOUND_Click : SOUND_ClickDisabled);
    return 1;
}
NW_HOOK_METHOD(0x1001f6e0, NCHeaderCtrl, OnLButtonDown)

// 0x1001f740 (vslot 63) - released over the header: the list sorts by this column
INT NCHeaderCtrl::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)) && bClickable && NotifyID != -1)
        NotifyParent(NotifyID);
    bReleased = 1;
    return 1;
}
NW_HOOK_METHOD(0x1001f740, NCHeaderCtrl, OnLButtonUp)

// 0x1001f810 - the textures come by name; a missing one is the black texture (only the two whole-header names
// stay unset when empty)
void NCHeaderCtrl::SetColumnInfo(NListColumnInfo* Info)
{
    const TCHAR* InTitle = Info->Title;
    if (InTitle && *InTitle)
        appStrcpy(Title, InTitle);
    bClickable = Info->bClickable;
    bNumeric = Info->bNumeric;
    ColumnWidth = Info->Width;
    ColumnHeight = Info->Height;
    const TCHAR* Name = Info->WholeNormalTex;
    if (Name && *Name)
        WholeNormalTex = FindTexture(Name);
    Name = Info->WholePushedTex;
    if (Name && *Name)
        WholePushedTex = FindTexture(Name);
    for (INT i = 0; i < 3; i++)
    {
        Name = Info->NormalTex[i];
        NormalTex[i] = FindTexture(Name && *Name ? Name : TEXT("NWindow.BlackTexture"));
    }
    for (INT i = 0; i < 3; i++)
    {
        Name = Info->PushedTex[i];
        PushedTex[i] = FindTexture(Name && *Name ? Name : TEXT("NWindow.BlackTexture"));
    }
}
NW_HOOK_METHOD(0x1001f810, NCHeaderCtrl, SetColumnInfo)

NW_CHECK_VTABLE(NCHeaderCtrl, 0x1017cff8, 69)

/*-----------------------------------------------------------------------------
    NCListCtrl.
-----------------------------------------------------------------------------*/

// 0x10020ef0 - NControl's constructor is inlined; the argument is bNoScrollBar; TextY starts at 3
NCListCtrl::NCListCtrl(INT bInNoScrollBar)
{
    BackTex = NULL;
    SelectTex = NULL;
    ToolTipTex = NULL;
    TipTopTex = NULL;
    TipMidTex = NULL;
    TipBottomTex = NULL;
    ScrollUpButton = NULL;
    ScrollDownButton = NULL;
    ScrollBar = NULL;
    TopRow = 0;
    NumRows = 0;
    TextX = 0;
    bNoScrollBar = bInNoScrollBar;
    MaxRows = 100;
    VisibleRows = 10;
    MaxColumns = 3;
    LineGap = 1;
    Unknown238 = 16;
    Selected = -1;
    HoverRow = -1;
    TextY = 3;
}
NW_HOOK_CTOR_ARGS(0x10020ef0, NCListCtrl, (INT A), (A))

// 0x10021080 - the fourth argument is never read (the clan window passes 12)
NCListCtrl::NCListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused, INT bInNoScrollBar)
{
    BackTex = NULL;
    SelectTex = NULL;
    ToolTipTex = NULL;
    TipTopTex = NULL;
    TipMidTex = NULL;
    TipBottomTex = NULL;
    MaxRows = InMaxRows;
    ScrollUpButton = NULL;
    ScrollDownButton = NULL;
    ScrollBar = NULL;
    TopRow = 0;
    NumRows = 0;
    TextX = 0;
    VisibleRows = InVisibleRows;
    bNoScrollBar = bInNoScrollBar;
    MaxColumns = InMaxColumns;
    LineGap = 1;
    Unknown238 = 15;
    Selected = -1;
    HoverRow = -1;
    TextY = 3;
}
NW_HOOK_CTOR_ARGS(0x10021080, NCListCtrl, (INT A, INT B, INT C, INT D, INT E), (A, B, C, D, E))

// 0x10021010
NW_HOOK_DTOR(0x10021010, NCListCtrl)

// 0x1001f990 (vslot 25) - the headers, the scroll buttons and the slider report here
INT NCListCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_LISTCTRL_HEADER)
            return SortByColumn((INT)lParam) != 0;
        if ((INT)wParam == NID_LIST_SCROLLUP)
            return ScrollUp() != 0;
        if ((INT)wParam == NID_LIST_SCROLLDOWN)
            return ScrollDown() != 0;
        if ((INT)wParam == NID_LIST_SCROLLBAR)
            return SetTopRow((INT)lParam) != 0;
    }
    else if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1001f990, NCListCtrl, WindowProc)

// 0x1001fa70 (vslot 37) - the scroll controls wait for the first column (AddColumn). The stock code stores each
// texture one call late; the names here are in the order of the calls
INT NCListCtrl::OnCreate()
{
    NControl::OnCreate();
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    TipTopTex = FindTexture(TEXT("L2UI.Tooltip.Tool02"));
    TipMidTex = FindTexture(TEXT("L2UI.Tooltip.Tool01"));
    TipBottomTex = FindTexture(TEXT("L2UI.Tooltip.Tool03"));
    SelectTex = FindTexture(TEXT("L2UI_CH3.ListCtrl.TextSelect"));
    SelectTex2 = FindTexture(TEXT("L2UI_CH3.ListCtrl.TextSelect2"));
    LineTex = FindTexture(TEXT("L2UI_CH3.Etc.textbackline"));
    return 1;
}
NW_HOOK_METHOD(0x1001fa70, NCListCtrl, OnCreate)

// 0x10021170 (vslot 38) - rows (destructor and GMalloc free), columns (children, deleted), the arrays, the scroll
// controls
INT NCListCtrl::OnDestroy()
{
    guard(NCListCtrl::OnDestroy);
    for (INT i = 0; i < Rows.Num(); i++)
    {
        NListRow* Row = Rows(i);
        if (Row)
            delete Row;
    }
    for (INT i = 0; i < Columns.Num(); i++)
    {
        NCHeaderCtrl* Column = Columns(i);
        if (Column)
        {
            RemoveChildWindow(Column->Entry);
            delete Column;
        }
    }
    Rows.Empty();
    Columns.Empty();
    if (ScrollUpButton)
    {
        RemoveChildWindow(ScrollUpButton->Entry);
        delete ScrollUpButton;
        ScrollUpButton = NULL;
    }
    if (ScrollDownButton)
    {
        RemoveChildWindow(ScrollDownButton->Entry);
        delete ScrollDownButton;
        ScrollDownButton = NULL;
    }
    if (ScrollBar)
    {
        RemoveChildWindow(ScrollBar->Entry);
        delete ScrollBar;
        ScrollBar = NULL;
    }
    Selected = -1;
    HoverRow = -1;
    TopRow = 0;
    NumRows = 0;
    NControl::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10021170, NCListCtrl, OnDestroy)

// 0x10020150 (vslot 56) - the background texture, a band under every second row line, a two-part bar under the
// selected row; each cell is clipped to its column (15 high): its icon centered in the column and in a 17 pixel
// row, its text 10 pixels after the icon
INT NCListCtrl::OnPaint(NCGDevice* Device)
{
    guard(NCListCtrl::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, (INT)Width, (INT)Height, BackTex, 0xFF, 1);
    for (INT i = 0; i < VisibleRows + 1; i++)
    {
        if (i % 2 == 1)
            Device->DrawTexture(0, HeaderHeight + i * ROW_Height - 1, (INT)Width, ROW_Height, 0, 0, 8, 15, LineTex,
                                0xFF, 1);
    }
    INT Line = 0;
    for (INT i = TopRow; i < TopRow + VisibleRows && i <= NumRows && i < Rows.Num(); i++, Line++)
    {
        Device->SetClip(ScreenX, ScreenY + HeaderHeight, (INT)Width, (INT)(Height - (FLOAT)HeaderHeight));
        if (i == Selected)
        {
            Device->DrawStretchedTex(1, Line * ROW_Height + HeaderHeight - 1, (INT)(Width - 47.0f - 2.0f), ROW_Height,
                                     0, 0, 16, 13, SelectTex, 0xFF);
            Device->DrawStretchedTex((INT)(Width + 1.0f - 47.0f - 2.0f), Line * ROW_Height + HeaderHeight - 1, 32,
                                     ROW_Height, 0, 0, 32, 13, SelectTex2, 0xFF);
        }
        Device->ReleaseClip();
        INT X = TextX;
        NListRow* Row = Rows(i);
        for (INT c = 0; c < Columns.Num(); c++)
        {
            NCHeaderCtrl* Column = Columns(c);
            INT Y = TextY + Line * ROW_Height + HeaderHeight;
            Device->SetClip(ScreenX + X, ScreenY + Y - 1, Column->ColumnWidth, 15);
            NListCell* Cell = (NListCell*)((BYTE*)Row->Cells + c * sizeof(NListCell));
            if (Cell->Icon)
                Device->DrawTexture(Column->ColumnWidth / 2 - Cell->IconW / 2 + X,
                                    ROW_Height / 2 - Cell->IconH / 2 + Line * ROW_Height + HeaderHeight, Cell->IconW,
                                    Cell->IconH, 0, 0, Cell->IconW, Cell->IconH, Cell->Icon, 0xFF, 1);
            if (Cell->bTextVisible)
                Device->DrawText(Cell->IconW + X + 10, Y, Cell->Color, Cell->Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                                 0, 0, 0, 0);
            X += Column->ColumnWidth;
            Device->ReleaseClip();
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10020150, NCListCtrl, OnPaint)

// 0x10021330 (vslot 57) - C3: the selected row's tool tip is the window skin's full box, at the row's line
INT NCListCtrl::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (HoverRow >= 0 && HoverRow < MaxRows && HoverRow == Selected)
    {
        NListRow* Row = Rows(Selected);
        INT OffsetY = (HoverRow - TopRow) % VisibleRows * ROW_Height + TextY + HeaderHeight;
        if (ToolTipSkin)
        {
            NToolTipRect Wnd;
            Wnd.X = ScreenX;
            Wnd.Y = ScreenY;
            Wnd.W = Width;
            Wnd.H = Height;
            ToolTipSkin->DrawFull(Device, Row->ToolTip, 0, OffsetY, Wnd);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10021330, NCListCtrl, OnDrawToolTip)

// 0x1001fbf0 (vslot 61) - the row is asked through vslot 70 (C3)
INT NCListCtrl::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    HoverRow = CallHitTest(LOWORD(lParam), HIWORD(lParam));
    return 1;
}
NW_HOOK_METHOD(0x1001fbf0, NCListCtrl, OnMouseMove)

// 0x1001fbb0 (vslot 62) - selects without telling the owner (the stock code still tests the row against
// Rows.Num(), with nothing depending on it)
INT NCListCtrl::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Row = CallHitTest(LOWORD(lParam), HIWORD(lParam));
    Selected = Row;
    HoverRow = Row;
    return 1;
}
NW_HOOK_METHOD(0x1001fbb0, NCListCtrl, OnLButtonDown)

// 0x1001fc70 (vslot 67) - one row per wheel notch (120); the slider is optional now
INT NCListCtrl::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    SWORD Notches = (SWORD)((SWORD)HIWORD(wParam) / 120);
    INT Delta = Notches;
    if (Notches > 0)
    {
        if (TopRow > Delta)
            TopRow -= Delta;
        else
            TopRow = 0;
    }
    else if (NumRows >= VisibleRows)
    {
        if (NumRows - TopRow + Delta > VisibleRows)
            TopRow -= Delta;
        else
            TopRow = NumRows - VisibleRows;
    }
    else
    {
        if (NumRows - TopRow + Delta > VisibleRows)
            TopRow -= Delta;
        else
            TopRow = 0;
    }
    if (ScrollBar)
        ScrollBar->SetValue(TopRow);
    return 1;
}
NW_HOOK_METHOD(0x1001fc70, NCListCtrl, OnMouseWheel)

// 0x10020d20 (vslot 69) - rows and columns go; the scroll controls stay
void NCListCtrl::ClearAll()
{
    for (INT i = 0; i < Rows.Num(); i++)
    {
        NListRow* Row = Rows(i);
        if (Row)
            delete Row;
    }
    for (INT i = 0; i < Columns.Num(); i++)
    {
        NCHeaderCtrl* Column = Columns(i);
        if (Column)
        {
            RemoveChildWindow(Column->Entry);
            delete Column;
        }
    }
    Rows.Empty();
    Columns.Empty();
    if (ScrollBar)
        ScrollBar->Reset();
    TopRow = 0;
    NumRows = 0;
    Selected = -1;
    HoverRow = -1;
}
NW_HOOK_METHOD(0x10020d20, NCListCtrl, ClearAll)

// 0x1001fc20 (vslot 71) - the owner gets the row (NWM_COMMAND, lParam = index)
INT NCListCtrl::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    INT Row = CallHitTest(LOWORD(lParam), HIWORD(lParam));
    Selected = Row;
    HoverRow = Row;
    if (Row >= 0 && Row < Rows.Num())
        NotifyParent(Row);
    return 1;
}
NW_HOOK_METHOD(0x1001fc20, NCListCtrl, OnLButtonDblClick)

// 0x1001fd30 (vslot 72)
INT NCListCtrl::ScrollUp()
{
    if (NumRows > VisibleRows)
    {
        if (--TopRow < 0)
            TopRow = 0;
        if (ScrollBar)
            ScrollBar->SetValue(TopRow);
    }
    return 1;
}
NW_HOOK_METHOD(0x1001fd30, NCListCtrl, ScrollUp)

// 0x1001fd70 (vslot 73)
INT NCListCtrl::ScrollDown()
{
    if (NumRows > VisibleRows)
    {
        INT Last = NumRows - VisibleRows;
        if (++TopRow > Last)
            TopRow = Last;
        if (ScrollBar)
            ScrollBar->SetValue(TopRow);
    }
    return 1;
}
NW_HOOK_METHOD(0x1001fd70, NCListCtrl, ScrollDown)

// 0x1001fdc0 (vslot 74) - from the slider
INT NCListCtrl::SetTopRow(INT Row)
{
    TopRow = Row;
    return 1;
}
NW_HOOK_METHOD(0x1001fdc0, NCListCtrl, SetTopRow)

// 0x100205c0 (vslot 75) - bubble sort on the column's text (or value); rows with the cell empty, or next to one,
// stay where they are. The selection follows its row
INT NCListCtrl::SortByColumn(INT Column)
{
    NListRow* SelRow = NULL;
    if (Selected >= 0 && Selected < Rows.Num())
        SelRow = Rows(Selected);
    for (INT Pass = 0; Pass < Rows.Num(); Pass++)
    {
        for (INT i = 0; i < Rows.Num() - Pass && i < Rows.Num() - 1; i++)
        {
            const TCHAR* A = Rows(i)->Cells[Column].Text;
            if (!*A)
                continue;
            const TCHAR* B = Rows(i + 1)->Cells[Column].Text;
            if (!*B)
                continue;
            NCHeaderCtrl* Header = Columns(Column);
            UBOOL bSwap;
            if (Header->bNumeric)
            {
                INT ValueB = NWtol(B);
                INT ValueA = NWtol(A);
                if (Header->bAscending)
                    bSwap = ValueA > ValueB;
                else
                    bSwap = ValueA < ValueB;
            }
            else
            {
                if (Header->bAscending)
                    bSwap = _wcsicmp(A, B) > 0;
                else
                    bSwap = _wcsicmp(A, B) < 0;
            }
            if (bSwap)
            {
                NListRow* Temp = Rows(i);
                Rows(i) = Rows(i + 1);
                Rows(i + 1) = Temp;
            }
        }
    }
    NCHeaderCtrl* Sorted = Columns(Column);
    Sorted->bAscending = (Sorted->bAscending == 0);
    if (SelRow)
    {
        Selected = 0;
        for (; Selected < Rows.Num(); Selected++)
        {
            if (Rows(Selected) == SelRow)
                break;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100205c0, NCListCtrl, SortByColumn)

// 0x1001fb00 (vslot 70) - the row under a screen point, among the lines shown (17 high); -1 outside the rows
INT NCListCtrl::HitTest(INT X, INT Y)
{
    INT RX = X - ScreenX;
    INT RY = Y - ScreenY;
    INT Result = -1;
    if (RX < 0)
        return -1;
    INT W = (INT)Width;
    if (RX > W)
        return -1;
    INT Top = HeaderHeight;
    if (RY < Top)
        return -1;
    if (RY > (INT)Height)
        return -1;
    INT Last = TopRow + VisibleRows;
    for (INT i = TopRow; i < Last && i < NumRows; i++, Top += ROW_Height)
    {
        if (RX < W && RY >= Top && RY < Top + ROW_Height)
        {
            Result = i;
            break;
        }
    }
    return Result;
}
NW_HOOK_METHOD(0x1001fb00, NCListCtrl, HitTest)

// 0x1001fde0
TArray<NListRow*>* NCListCtrl::GetRows()
{
    return &Rows;
}
NW_HOOK_METHOD(0x1001fde0, NCListCtrl, GetRows)

// 0x1001fdf0
INT NCListCtrl::GetRowCount()
{
    return Rows.Num();
}
NW_HOOK_METHOD(0x1001fdf0, NCListCtrl, GetRowCount)

// 0x1001fe00
void NCListCtrl::SetSelected(INT Index)
{
    if (Index >= 0 && Index < Rows.Num())
        Selected = Index;
}
NW_HOOK_METHOD(0x1001fe00, NCListCtrl, SetSelected)

// 0x1001fe20
void NCListCtrl::SetTextOffset(INT X, INT Y)
{
    TextX = X;
    TextY = Y;
}
NW_HOOK_METHOD(0x1001fe20, NCListCtrl, SetTextOffset)

// 0x1001fe40 - C3: the slider is optional
void NCListCtrl::UpdateScrollRange()
{
    NumRows = Rows.Num();
    if (ScrollBar)
    {
        ScrollBar->SetRange(Rows.Num(), VisibleRows);
        ScrollBar->SetValue(TopRow);
    }
}
NW_HOOK_METHOD(0x1001fe40, NCListCtrl, UpdateScrollRange)

// 0x1001fe80 (vslot 44) - NCWnd shows it, then the scroll range is made again (the stock tail-jumps to UpdateScrollRange)
void NCListCtrl::ShowWindow()
{
    NCWnd::ShowWindow();
    UpdateScrollRange();
}
NW_HOOK_METHOD(0x1001fe80, NCListCtrl, ShowWindow)

// 0x10020530
NListRow* NCListCtrl::GetSelectedRow()
{
    if (Selected >= 0 && Selected < Rows.Num())
        return Rows(Selected);
    return NULL;
}
NW_HOOK_METHOD(0x10020530, NCListCtrl, GetSelectedRow)

// 0x10020550 - the scroll buttons are created without auto-repeat, so this has no effect
void NCListCtrl::SetRepeatDelay(INT Delay)
{
    if (ScrollUpButton && ScrollDownButton)
    {
        ScrollUpButton->RepeatDelay = Delay;
        ScrollDownButton->RepeatDelay = Delay;
    }
}
NW_HOOK_METHOD(0x10020550, NCListCtrl, SetRepeatDelay)

// 0x10020580 (name ours) - the range uses TopRow as the total (0 at creation), as in the stock code
void NCListCtrl::ResetScrollBar()
{
    SetRepeatDelay(200);
    if (ScrollBar)
    {
        ScrollBar->SetRange(TopRow, VisibleRows);
        ScrollBar->SetValue(TopRow);
    }
}
NW_HOOK_METHOD(0x10020580, NCListCtrl, ResetScrollBar)

// 0x100207a0 - full lists refuse the row (the caller still owns it then)
INT NCListCtrl::AddRow(NListRow* Row)
{
    if (MaxRows <= Rows.Num())
        return 0;
    if (Row)
        Rows.AddItem(Row);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100207a0, NCListCtrl, AddRow)

// 0x10020c90 (name ours) - the columns stay
void NCListCtrl::ClearRows()
{
    for (INT i = 0; i < Rows.Num(); i++)
    {
        NListRow* Row = Rows(i);
        if (Row)
            delete Row;
    }
    Rows.Empty();
    if (ScrollBar)
        ScrollBar->Reset();
    TopRow = 0;
    NumRows = 0;
    Selected = -1;
    HoverRow = -1;
}
NW_HOOK_METHOD(0x10020c90, NCListCtrl, ClearRows)

// 0x100207f0 (guard name InitScrollBar) - up / down buttons and the slider along the right edge, under the first
// header; none when the list was made with bNoScrollBar
void NCListCtrl::InitScrollBar()
{
    guard(NCListCtrl::InitScrollBar);
    if (!(bNoScrollBar & 1))
    {
        NRectF Rect;
        Rect.X = 0;
        Rect.Y = 0;
        Rect.W = 0.0f;
        Rect.H = 0.0f;
        GetLocalRect(&Rect);
        INT HeaderH = 0;
        if (Columns.Num() > 0)
        {
            NRectF ColRect;
            ColRect.X = 0;
            ColRect.Y = 0;
            ColRect.W = 0.0f;
            ColRect.H = 0.0f;
            Columns(0)->GetLocalRect(&ColRect);
            HeaderH = (INT)ColRect.H;
        }

        ScrollUpButton = new NCPushButton(TEXT("L2UI_CH3.ScrollBar.ScrollBarUpBtn"),
                                          TEXT("L2UI_CH3.ScrollBar.ScrollBarUpOnBtn"));
        ScrollUpButton->ID = NID_LIST_SCROLLUP;
        ScrollUpButton->SetPosition((INT)(Rect.W - 15.0f), HeaderH, 15, 15);
        CreateChild(ScrollUpButton, NWS_VISIBLE);

        ScrollDownButton = new NCPushButton(TEXT("L2UI_CH3.ScrollBar.ScrollBarDownBtn"),
                                            TEXT("L2UI_CH3.ScrollBar.ScrollBarDownOnBtn"));
        ScrollDownButton->ID = NID_LIST_SCROLLDOWN;
        ScrollDownButton->SetPosition((INT)(Rect.W - 15.0f), (INT)(Rect.H - 15.0f), 15, 15);
        CreateChild(ScrollDownButton, NWS_VISIBLE);

        // the slider: allocated as the class does it, then its three-texture constructor (0x10029a80)
        ScrollBar = new NCSliderBar(TEXT("L2UI_CH3.ScrollBar.SliderBarTop"),
                                     TEXT("L2UI_CH3.ScrollBar.SliderBarCenter"), TEXT("L2UI_CH3.ScrollBar.SliderBarBottom"));
        ScrollBar->ID = NID_LIST_SCROLLBAR;
        ScrollBar->SetPosition((INT)(Rect.W - 15.0f), HeaderH + 15, 15, (INT)(Rect.H - 30.0f - (FLOAT)HeaderH));
        CreateChild(ScrollBar, NWS_VISIBLE);

        ResetScrollBar();
    }
    unguard;
}
NW_HOOK_METHOD(0x100207f0, NCListCtrl, InitScrollBar)

// 0x10021430 (name ours) - a header after the existing ones (Index is only checked against MaxColumns: the owners
// pass 0 .. 3); the rows start under the last header added
INT NCListCtrl::AddColumn(INT Index, NListColumnInfo* Info)
{
    if (Index > MaxColumns)
        return 0;
    NCHeaderCtrl* Header = new NCHeaderCtrl;
    Header->SetColumnInfo(Info);
    Header->ID = NID_LISTCTRL_HEADER;
    INT X = 0;
    for (INT i = 0; i < Columns.Num(); i++)
        X += Columns(i)->ColumnWidth;
    Header->SetPosition(X, 0, Info->Width, Info->Height);
    CreateChild(Header, NWS_VISIBLE);
    Header->NotifyID = Columns.Num();
    Columns.AddItem(Header);
    if (!ScrollBar)
        InitScrollBar();
    UpdateScrollRange();
    HeaderHeight = Info->Height;
    return 1;
}
NW_HOOK_METHOD(0x10021430, NCListCtrl, AddColumn)

NW_CHECK_VTABLE(NCListCtrl, 0x1017d110, 76)
