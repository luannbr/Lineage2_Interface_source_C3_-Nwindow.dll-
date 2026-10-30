/*=============================================================================
    NCTreeCtrl.cpp - a scrolling tree view (C2 only). Stock: 0x1002a9a0 .. 0x1002b08f (constructor,
    destructor and the virtuals).
=============================================================================*/
#include "nwindow/NCTreeCtrl.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCTree.h"
#include "nwindow/NCGDevice.h"

enum { NID_TREECTRL_TREE = 0xA00001 };   // the inner tree's id (name ours)

// 0x1002f8a0 - NCScrollWnd's constructor, the vtable, the scroll bar 16 px wide (C3 field at +0xFC)
NCTreeCtrl::NCTreeCtrl()
{
    Unknown0C8 = 16;
}
NW_HOOK_CTOR(0x1002f8a0, NCTreeCtrl)

// 0x1002f8f0
NW_HOOK_DTOR(0x1002f8f0, NCTreeCtrl)

// 0x1002f940 (vslot 37) - the tree, as wide as the window less the scroll bar (16), as high as the window
INT NCTreeCtrl::OnCreate()
{
    NCScrollWnd::OnCreate();
    Tree = new NCTree();
    Tree->ID = NID_TREECTRL_TREE;
    Tree->SetPosition(0, 0, (INT)(Width - 16.0f), (INT)Height);
    CreateChild(Tree, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x1002f940, NCTreeCtrl, OnCreate)

// 0x1002e980 (vslot 38)
INT NCTreeCtrl::OnDestroy()
{
    guard(NCTreeCtrl::OnDestroy);
    if (Tree)
    {
        RemoveChildWindow(Tree->Entry);
        delete Tree;
        Tree = NULL;
    }
    NCScrollWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1002e980, NCTreeCtrl, OnDestroy)

// 0x1002ea30 (vslot 56) - the content height follows the tree (and 21 more). C3: the tree's own paint lays its
// entries out
INT NCTreeCtrl::OnPaint(NCGDevice* Device)
{
    guard(NCTreeCtrl::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    SetContentHeightEx(Tree->ContentHeight + 21);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1002ea30, NCTreeCtrl, OnPaint)

// 0x1002eb10 (vslot 69) - one line up: the tree window moves down by a line
INT NCTreeCtrl::ScrollUp()
{
    if (ScrollY < 0)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY += LineStep;
        if (0 < ScrollY)
            ScrollY = 0;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX = DeltaX - ScrollX;
        DeltaY = DeltaY - ScrollY;
        Tree->MoveWindow(0, LineStep);
        Tree->ScrollOffset -= LineStep;
    }
    return 1;
}
NW_HOOK_METHOD(0x1002eb10, NCTreeCtrl, ScrollUp)

// 0x1002ebd0 (vslot 70) - one line down
INT NCTreeCtrl::ScrollDown()
{
    if (MinScrollY < ScrollY)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY -= LineStep;
        if (ScrollY < MinScrollY)
            ScrollY = MinScrollY;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX = DeltaX - ScrollX;
        DeltaY = DeltaY - ScrollY;
        Tree->MoveWindow(0, -LineStep);
        Tree->ScrollOffset += LineStep;
    }
    return 1;
}
NW_HOOK_METHOD(0x1002ebd0, NCTreeCtrl, ScrollDown)

// 0x1002eca0 (vslot 71) - the bar was moved to Value
INT NCTreeCtrl::OnScrollBar(INT Value)
{
    DeltaX = ScrollX;
    DeltaY = ScrollY;
    ScrollY = -Value;
    DeltaY = DeltaY - ScrollY;
    DeltaX = DeltaX - ScrollX;
    Tree->MoveWindow(0, -DeltaY);
    Tree->ScrollOffset += DeltaY;
    return 1;
}
NW_HOOK_METHOD(0x1002eca0, NCTreeCtrl, OnScrollBar)

// 0x1002ed20 (vslot 67) - three lines per notch of the wheel
INT NCTreeCtrl::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    SHORT Notches = (SHORT)((SHORT)HIWORD(wParam) / 120);
    DeltaX = ScrollX;
    DeltaY = ScrollY;
    if (Notches < 1)
    {
        if (MinScrollY < ScrollY)
        {
            ScrollY = ScrollY + LineStep * Notches * 3;
            if (ScrollY < MinScrollY)
                ScrollY = MinScrollY;
            ScrollBar->SetScrollPos(-ScrollY);
        }
    }
    else if (ScrollY < 0)
    {
        ScrollY = ScrollY + LineStep * Notches * 3;
        if (0 < ScrollY)
            ScrollY = 0;
        ScrollBar->SetScrollPos(-ScrollY);
    }
    DeltaY = DeltaY - ScrollY;
    DeltaX = DeltaX - ScrollX;
    Tree->MoveWindow(0, -DeltaY);
    Tree->ScrollOffset += DeltaY;
    return 1;
}
NW_HOOK_METHOD(0x1002ed20, NCTreeCtrl, OnMouseWheel)

// 0x1002ee50 (vslot 72) - the tree emptied and brought back to its place, the scroll at the top
void NCTreeCtrl::Clear()
{
    Tree->Clear();
    NRectF R;
    R.X = 0;
    R.Y = 0;
    R.W = 0.0f;
    R.H = 0.0f;
    Tree->GetLocalRect(&R);
    Tree->MoveWindow(-R.X, -R.Y);
    Tree->ScrollOffset = 0;
    ScrollY = 0;
    ScrollX = 0;
    DeltaY = 0;
    DeltaX = 0;
    MinScrollY = 0;
    ::nw::Field<INT>(ScrollBar, 0x1D8) = 0;
}
NW_HOOK_METHOD(0x1002ee50, NCTreeCtrl, Clear)

NW_CHECK_VTABLE(NCTreeCtrl, 0x1017dc48, 73)
