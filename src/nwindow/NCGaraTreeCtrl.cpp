/*=============================================================================
    NCGaraTreeCtrl.cpp - the scrolling window of one NCGaraTree. C3 stock: 0x10012F30 .. 0x10013500 (the methods),
    0x10013B20 .. 0x10013C7F (constructor, destructor, OnCreate).
=============================================================================*/
#include "nwindow/NCGaraTreeCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"

// 0x10013B20 - the base's, with a bigger Unknown0C8 (16)
NCGaraTreeCtrl::NCGaraTreeCtrl()
{
    Unknown0C8 = 16;
}
NW_HOOK_CTOR(0x10013B20, NCGaraTreeCtrl)

// 0x10013B70
NW_HOOK_DTOR(0x10013B70, NCGaraTreeCtrl)

// 0x10093F30 (vslot 25) - the stock body is the one the linker shared with NCItemWnd's: NCScrollWnd's, as a flag
INT NCGaraTreeCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCScrollWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10013BC0 (vslot 37) - the tree (id 0xA00001), the window's width less 16 pixels (the bar's)
INT NCGaraTreeCtrl::OnCreate()
{
    NCScrollWnd::OnCreate();
    Tree = new NCGaraTree();
    Tree->ID = 0xA00001;
    Tree->SetPosition(0, 0, (INT)(Width - 16.0f), (INT)Height);
    CreateChild(Tree, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10013BC0, NCGaraTreeCtrl, OnCreate)

// 0x10012F30 (vslot 38)
INT NCGaraTreeCtrl::OnDestroy()
{
    guard(NCGaraTreeCtrl::OnDestroy);
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
NW_HOOK_METHOD(0x10012F30, NCGaraTreeCtrl, OnDestroy)

// 0x10012FE0 (vslot 56) - the scroll range follows the tree's height (and 21), and the entries are put where they
// show
INT NCGaraTreeCtrl::OnPaint(NCGDevice* Device)
{
    guard(NCGaraTreeCtrl::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    SetContentHeightEx(Tree->ContentHeight + 21);
    if (Tree->Root)
        ArrangeItem(Tree->Root);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10012FE0, NCGaraTreeCtrl, OnPaint)

// 0x100130D0 (vslot 73, name ours) - an entry whose box (its size from OnCreate, at the scroll offset) is inside the
// window is shown, the others hidden; then the ones under it
void NCGaraTreeCtrl::ArrangeItem(NCGaraTVItem* Item)
{
    if (!Item)
        return;
    INT Right = Item->CopyW + ScrollX;
    INT Bottom = Item->CopyH + ScrollY;
    NRectF Local;
    Local.X = 0;
    Local.Y = 0;
    Local.W = 0.0f;
    Local.H = 0.0f;
    Tree->GetLocalRect(&Local);
    if (Right >= 0 && Right <= (INT)Local.W && Bottom >= 0 && Bottom <= (INT)Local.H)
        Item->ShowWindow();
    else
        Item->HideWindow();
    for (INT i = 0; i < Item->Children.Num(); i++)
        ArrangeItem(Item->Children(i));
}
NW_HOOK_METHOD(0x100130D0, NCGaraTreeCtrl, ArrangeItem)

// 0x100131A0 (vslot 69) - NCScrollWnd's up by one line step, the tree follows
INT NCGaraTreeCtrl::ScrollUp()
{
    if (ScrollY < 0)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY += LineStep;
        if (ScrollY > 0)
            ScrollY = 0;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX -= ScrollX;
        DeltaY -= ScrollY;
        Tree->MoveWindow(0, LineStep);
        Tree->ScrollOffset -= LineStep;
    }
    return 1;
}
NW_HOOK_METHOD(0x100131A0, NCGaraTreeCtrl, ScrollUp)

// 0x10013260 (vslot 70) - the same down
INT NCGaraTreeCtrl::ScrollDown()
{
    if (ScrollY > MinScrollY)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY -= LineStep;
        if (ScrollY < MinScrollY)
            ScrollY = MinScrollY;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX -= ScrollX;
        DeltaY -= ScrollY;
        Tree->MoveWindow(0, -LineStep);
        Tree->ScrollOffset += LineStep;
    }
    return 1;
}
NW_HOOK_METHOD(0x10013260, NCGaraTreeCtrl, ScrollDown)

// 0x10013330 (vslot 71) - the slider's value is the position as it comes; the tree moves by the difference
INT NCGaraTreeCtrl::OnScrollBar(INT Value)
{
    DeltaX = ScrollX;
    DeltaY = ScrollY;
    ScrollY = -Value;
    DeltaX -= ScrollX;
    DeltaY -= ScrollY;
    Tree->MoveWindow(0, -DeltaY);
    Tree->ScrollOffset += DeltaY;
    return 1;
}
NW_HOOK_METHOD(0x10013330, NCGaraTreeCtrl, OnScrollBar)

// 0x100133B0 (vslot 67) - three line steps per notch (120), up or down
INT NCGaraTreeCtrl::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    SHORT Notches = (SHORT)((SHORT)(wParam >> 16) / 120);
    DeltaX = ScrollX;
    DeltaY = ScrollY;
    if (Notches > 0)
    {
        if (ScrollY < 0)
        {
            ScrollY += LineStep * Notches * 3;
            if (ScrollY > 0)
                ScrollY = 0;
            ScrollBar->SetScrollPos(-ScrollY);
        }
    }
    else
    {
        if (ScrollY > MinScrollY)
        {
            ScrollY += LineStep * Notches * 3;
            if (ScrollY < MinScrollY)
                ScrollY = MinScrollY;
            ScrollBar->SetScrollPos(-ScrollY);
        }
    }
    DeltaX -= ScrollX;
    DeltaY -= ScrollY;
    Tree->MoveWindow(0, -DeltaY);
    Tree->ScrollOffset += DeltaY;
    return 1;
}
NW_HOOK_METHOD(0x100133B0, NCGaraTreeCtrl, OnMouseWheel)

// 0x100134E0 (vslot 72, name ours) - the tree is emptied and put back at the origin of the window, the scroll starts
// over
void NCGaraTreeCtrl::ResetScroll()
{
    Tree->Clear();
    NRectF Local;
    Local.X = 0;
    Local.Y = 0;
    Local.W = 0.0f;
    Local.H = 0.0f;
    Tree->GetLocalRect(&Local);
    Tree->MoveWindow(-Local.X, -Local.Y);
    Tree->ScrollOffset = 0;
    ScrollY = 0;
    ScrollX = 0;
    DeltaY = 0;
    DeltaX = 0;
    MinScrollY = 0;
    ScrollBar->ThumbPos = 0;
}
NW_HOOK_METHOD(0x100134E0, NCGaraTreeCtrl, ResetScroll)

NW_CHECK_VTABLE(NCGaraTreeCtrl, 0x10179A68, 74)
