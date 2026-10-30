/*=============================================================================
    NCWnd.cpp - base window: tree, geometry, focus, visibility, message routing.
    Stock (C2): 0x1002b500 .. 0x1002cdc9, plus the defaults folded at 0x100016e0.. and
    DefWindowProc (NCCmdTarget::WindowProc, 0x10004bb0).
=============================================================================*/
#include "nwindow/NCVirtualWndMain.h"
#include "nwindow/NControl.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCShopWnd.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NCTradeWnd.h"
#include "nwindow/NCDeliverWnd.h"
#include "nwindow/NClasses.gen.h"
#if NW_FEATURES
#include "features/WindowFix.h"   // phase 6: WindowFix
#endif

#include <wchar.h>

// The tool tip skin and info (NToolTipInfo / NToolTipSkin, 0x10002230 .. 0x1002a4a1) are in NToolTip.cpp.

/*-----------------------------------------------------------------------------
    NCCmdTarget
-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
    Construction
-----------------------------------------------------------------------------*/

// 0x10031d80 - NCObject's constructor first (folded with NCCmdTarget's); the four stuck lists are built with it
NCWnd::NCWnd()
{
    ChildHead = NULL;
    ChildTail = NULL;
    bFocus = 0;
    ScreenY = 0;
    ScreenX = 0;
    Width = 0;
    Height = 0;
    Style = 0;
    Entry = NULL;
    bEnabled = 1;
    DragMouseY = 0;
    DragMouseX = 0;
    NextTab = NULL;
    PrevTab = NULL;
    AnchorOffsetY = 0;
    AnchorOffsetX = 0;
    InitStuckState();
    ClipOffsetX = 0;
    ClipOffsetY = 0;
    bMouseDown = 0;
    AnchorType = 0;
    AnchorOffsetX = 0;
    AnchorOffsetY = 0;
}
NW_HOOK_CTOR(0x10031d80, NCWnd)

// 0x10002700 - NW_DECLARE_CLASS destructor (ConditionalDestroy, then the four lists)
NW_HOOK_DTOR(0x10002700, NCWnd)


// Accessors and the defaults the linker folded (one stock body serves several slots/classes).
NW_HOOK_METHOD(0x10002770, NCWnd, IsEnabled)
NW_HOOK_METHOD(0x10002780, NCWnd, IsFocused)
NW_HOOK_METHOD(0x1002fd70, NCWnd, SetStyle)
NW_HOOK_METHOD(0x10002790, NCWnd, GetStyle)
NW_HOOK_METHOD(0x10001af0, NCWnd, vfn032)   // xor eax,eax; ret    - also vslot 33, NControl vslot 62

// 0x10030fc0 (vslot 37) - creates the title bar (NWS_TITLEBAR: the undecorated NCFrameCtrl, 15 px above the window,
// as wide as the window) and the minimized icon (NWS_MINIMIZABLE); then the tool tip skin of every window: nine
// textures "L2UI_ch3.Tooltip.Tooltip1" .. "Tooltip9". C3: the window's width comes from GetLocalRect (a FLOAT now)
INT NCWnd::OnCreate()
{
    NRectF R;
    R.W = 0.0f;
    GetLocalRect(&R);
    FrameCtrl = NULL;
    Icon = NULL;
    bMouseDown = 0;
    if (Style & NWS_TITLEBAR)
    {
        FrameCtrl = new NCFrameCtrl(0, 0, 0);
        FrameID = GNextFrameID++;
        FrameCtrl->ID = FrameID;
        FrameCtrl->SetPosition(0, -15, (DWORD)(INT)R.W, 15);
        CreateChild(FrameCtrl, NWS_VISIBLE);
    }
    if (Style & NWS_MINIMIZABLE)
    {
        Icon = new NCIcon;
        Icon->ID = 15;
        Icon->SetPosition(0, 0, 32, 32);
        CreateChild(Icon, NWS_ICON);
        Icon->HideWindow();
    }
    ToolTipSkin = new NToolTipSkin;
    for (INT i = 1; i < 10; i++)
    {
        TCHAR Name[32];
        appSprintf(Name, TEXT("L2UI_ch3.Tooltip.Tooltip%d"), i);
        ToolTipSkin->Tex[i - 1] = FindTexture(Name);
    }
    return 1;
}
NW_HOOK_METHOD(0x10030fc0, NCWnd, OnCreate)

// 0x10031310 (vslot 38) - guarded; the tool tip skin goes between the title bar and the icon
INT NCWnd::OnDestroy()
{
    guard(NCWnd::OnDestroy);
    if (FrameCtrl)
    {
        RemoveChildWindow(FrameCtrl->Entry);
        delete FrameCtrl;
        FrameCtrl = NULL;
    }
    if (ToolTipSkin)
    {
        delete ToolTipSkin;
        ToolTipSkin = NULL;
    }
    if (Icon)
    {
        RemoveChildWindow(Icon->Entry);
        delete Icon;
        Icon = NULL;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10031310, NCWnd, OnDestroy)

/*-----------------------------------------------------------------------------
    Messages
-----------------------------------------------------------------------------*/

// 0x10004c70 (NCCmdTarget::WindowProc, vslot 25 of NCCmdTarget) - the creation, destruction and timer messages
INT NCWnd::DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == WM_CREATE)
        return OnCreate() != 0;
    if (Msg == WM_DESTROY)
        return OnDestroy() != 0;
    if (Msg == WM_TIMER)
        return OnTimer((INT)wParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10004c70, NCWnd, DefWindowProc)

// 0x1002fa20 (vslot 25) - WM_PAINT carries the NCGDevice in wParam. C1 returned 0 first for a disabled window
// and any message but WM_PAINT: C2 does not (the test is gone from NControl::WindowProc too)
INT NCWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (DefWindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_PAINT)
    {
        if (Style & NWS_VISIBLE)
            OnPaint((NCGDevice*)wParam);
        return 1;
    }
    if (Msg == WM_CHAR)
        return OnChar(wParam, lParam) != 0;
    if (Msg == WM_KEYDOWN)
        return OnKeyDown(wParam, lParam) != 0;
    if (Msg == WM_KEYUP)
        return OnKeyUp(wParam, lParam) != 0;
    if (Msg == WM_MOUSEMOVE)
        return OnMouseMove(wParam, lParam) != 0;
    if (Msg == WM_LBUTTONDOWN)
        return OnLButtonDown(wParam, lParam) != 0;
    if (Msg == WM_RBUTTONDOWN)
        return OnRButtonDown(wParam, lParam) != 0;
    if (Msg == WM_LBUTTONUP)
        return OnLButtonUp(wParam, lParam) != 0;
    if (Msg == WM_RBUTTONUP)
        return OnRButtonUp(wParam, lParam) != 0;
    if (Msg == WM_MOVE)
        return OnMove(wParam, lParam) != 0;
    if (Msg == WM_MOUSEWHEEL)
        return OnMouseWheel(wParam, lParam) != 0;
    if (Msg == NWM_DRAWTOOLTIP)
    {
        if (Style & NWS_VISIBLE)
            OnDrawToolTip((NCGDevice*)wParam);
        return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x1002fa20, NCWnd, WindowProc)

// 0x1002feb0 (vslot 59) - TAB / Shift+TAB move the focus along NextTab / PrevTab
INT NCWnd::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_TAB)
    {
        NWndEntry* Target;
        if (GShiftDown)
        {
            if (!PrevTab)
                return vfn032() != 0;
            KillFocus();
            Target = PrevTab;
        }
        else
        {
            if (!NextTab)
                return vfn032() != 0;
            KillFocus();
            Target = NextTab;
        }
        if (Target->Wnd)
            Target->Wnd->SetFocus();
    }
    else if (wParam == VK_SHIFT)
    {
        GShiftDown = 1;
    }
    return vfn032() != 0;
}
NW_HOOK_METHOD(0x1002feb0, NCWnd, OnKeyDown)

// 0x1002ff30 (vslot 60)
INT NCWnd::OnKeyUp(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_SHIFT)
        GShiftDown = 0;
    return vfn032() != 0;
}
NW_HOOK_METHOD(0x1002ff30, NCWnd, OnKeyUp)

// 0x1002fe80 (vslot 66) - lParam holds a signed (dx, dy) pair
INT NCWnd::OnMove(WPARAM wParam, LPARAM lParam)
{
    MoveWindow((SWORD)(lParam & 0xFFFF), (SWORD)((DWORD)lParam >> 16));
    return 1;
}
NW_HOOK_METHOD(0x1002fe80, NCWnd, OnMove)

/*-----------------------------------------------------------------------------
    Focus
-----------------------------------------------------------------------------*/

// 0x10030c60 (vslot 29) - C3: a window with no input of its own hands the focus to the first window below it that
// takes keyboard input; a window that holds a slider bar keeps it
void NCWnd::SetFocus()
{
    if (GFocusEntry == Entry)
        return;
    if (GFocusEntry)
    {
        GFocusEntry->Wnd->KillFocus();
        GFocusEntry = NULL;
    }
    UBOOL bGiven;
    if (!Entry->Wnd->vfn032())
        bGiven = FocusFirstInput();
    else
        bGiven = 0;
    if (!(Style & NWS_NOFOCUSSEARCH))
    {
        if (bGiven)
            return;
        if (!Entry->Wnd->IsA(NClass::NCSliderBar()) && !HasSliderBarChild())
            bGiven = FocusSliderBarParent();
    }
    if (bGiven)
        return;
    bFocus = 1;
    GFocusEntry = Entry;
    if (Entry->Parent)
        Entry->Parent->Wnd->BringChildToTop(Entry);
}
NW_HOOK_METHOD(0x10030c60, NCWnd, SetFocus)

// 0x1002fd50 (vslot 30) - shared with NCMinimapWnd (the linker folded the two)
void NCWnd::KillFocus()
{
    if (GFocusEntry == Entry)
        GFocusEntry = NULL;
    bFocus = 0;
}
NW_HOOK_METHOD(0x1002fd50, NCWnd, KillFocus)

// 0x1002fbe0 (vslot 41)
void NCWnd::SetTabOrder(NWndEntry* Next, NWndEntry* Prev)
{
    if (Next)
        NextTab = Next;
    if (Prev)
        PrevTab = Prev;
}
NW_HOOK_METHOD(0x1002fbe0, NCWnd, SetTabOrder)

/*-----------------------------------------------------------------------------
    Visibility and state
-----------------------------------------------------------------------------*/

// 0x1002fc00 (vslot 42)
void NCWnd::EnableWindow(UBOOL bEnable)
{
    bEnabled = bEnable;
    for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
        Node->Entry->Wnd->EnableWindow(bEnable);
}
NW_HOOK_METHOD(0x1002fc00, NCWnd, EnableWindow)

// 0x1002fc70 (vslot 39)
UBOOL NCWnd::IsShowWindow()
{
    return (Style & NWS_VISIBLE) ? 1 : 0;
}
NW_HOOK_METHOD(0x1002fc70, NCWnd, IsShowWindow)

// 0x1002fc30 (vslot 44) - a minimized window's icon goes away when the window shows
void NCWnd::ShowWindow()
{
    if (Style & NWS_MINIMIZABLE)
        Icon->HideWindow();
    Style |= NWS_VISIBLE;
    for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
        Node->Entry->Wnd->ShowWindow();
}
NW_HOOK_METHOD(0x1002fc30, NCWnd, ShowWindow)

// 0x10030c00 (vslot 45) - C3: a focused window that goes away passes the focus on
void NCWnd::HideWindow()
{
    Style &= ~NWS_VISIBLE;
    for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
        Node->Entry->Wnd->HideWindow();
    if (IsFocused())
    {
        if (GFocusEntry == Entry)
            GFocusEntry = NULL;
        bFocus = 0;
        FocusNextWindow(Entry);
    }
}
NW_HOOK_METHOD(0x10030c00, NCWnd, HideWindow)

// 0x100311d0 (vslot 48) - back from the icon, at the icon's place, and the window takes the focus. The inventory stays
// an icon while the NPC shop (bought from), the warehouse (depositing: mode 0), the trade or the freight window is open
void NCWnd::Restore()
{
    PlayInterfaceSound(5);
    if (Entry->Wnd->IsA(NClass::NCInventoryWnd()))
    {
        NCConsole* Desk = (NCConsole*)GConsole->Desktop;
        if ((Desk->ShopWnd->IsShowWindow() && Desk->ShopWnd->GetMode() == SHOPMODE_Sell) ||
            (Desk->StoreWnd->IsShowWindow() && Desk->StoreWnd->GetMode() == 0) ||
            Desk->TradeWnd->IsShowWindow() ||
            Desk->DeliverWnd->IsShowWindow())
            return;
    }
    SetPosition(Icon->ScreenX, Icon->ScreenY, (DWORD)(INT)Width, (DWORD)(INT)Height);
    Icon->SetPosition(0, 0, 0, 0);
    ShowWindow();
    SetFocus();
    Icon->HideWindow();
}
NW_HOOK_METHOD(0x100311d0, NCWnd, Restore)

// 0x1002fe30 (vslot 49) - C3: no icon picture here any more (the icon picks its own), the sound, the focus goes to it
void NCWnd::Minimize()
{
    PlayInterfaceSound(6);
    if (ScreenX < 0 && Icon)
        Icon->OffsetWindow(-ScreenX, 0);
    HideWindow();
    if (Icon)
    {
        Icon->ShowWindow();
        Icon->SetFocus();
    }
}
NW_HOOK_METHOD(0x1002fe30, NCWnd, Minimize)

// 0x1002ff60 (vslot 51)
void NCWnd::SetTitle(const TCHAR* Title)
{
    if (FrameCtrl && Title && *Title)
        FrameCtrl->SetTitleText(Title);
}
NW_HOOK_METHOD(0x1002ff60, NCWnd, SetTitle)

/*-----------------------------------------------------------------------------
    Geometry
-----------------------------------------------------------------------------*/

// 0x1002fd80 (vslot 40) - edges included; the rectangle comes by value (16 bytes: the same four arguments of the stack)
UBOOL NCWnd::PtInRect(INT X, INT Y, NRectF R)
{
    if (X < R.X || X > (INT)((FLOAT)R.X + R.W))
        return 0;
    if (Y < R.Y || Y > R.Y + (INT)R.H)
        return 0;
    return 1;
}
NW_HOOK_METHOD(0x1002fd80, NCWnd, PtInRect)

// 0x1002fdd0 (vslot 27)
UBOOL NCWnd::IsInWindow(INT X, INT Y)
{
    if (!(Style & NWS_VISIBLE))
        return 0;
    return PtInRect(X, Y, Rect);
}
NW_HOOK_METHOD(0x1002fdd0, NCWnd, IsInWindow)

// 0x1002fe20
NWndEntry* NCWnd::FindWindowAt(INT X, INT Y)
{
    return Entry->Main->FindNWindow(X, Y);
}
NW_HOOK_METHOD(0x1002fe20, NCWnd, FindWindowAt)

// 0x1004d460 (vslot 47) - C3: the subtree moves with MoveChildWindow
void NCWnd::OffsetWindow(INT DX, INT DY)
{
    MoveChildWindow(DX, DY);
}
NW_HOOK_METHOD(0x1004d460, NCWnd, OffsetWindow)

// 0x10032aa0 (vslot 46) - with the sticky-window option the move snaps to the edges first
void NCWnd::MoveWindow(INT DX, INT DY)
{
    if (DX || DY)
    {
        POINT Delta;
        Delta.x = DX;
        Delta.y = DY;
        if (GIsStickyWindow)
            Snap(&Delta);
        MoveChildWindow(Delta.x, Delta.y);
    }
}
NW_HOOK_METHOD(0x10032aa0, NCWnd, MoveWindow)

// abs() the way the stock code inlines it: (x ^ (x >> 31)) - (x >> 31)
static inline INT SnapAbs(INT X)
{
    return (X ^ (X >> 31)) - (X >> 31);
}

// 0x100321d0 - sticky windows (GIsStickyWindow): while a NWS_TOPMOST window is dragged, its edges stick to the screen
// edges and to the edges of the other top-most windows closer than 10 px, and come loose once the cursor pulls 10 px
// away. C3: what is stuck to what is kept in the four StuckWnds lists (IsStuck / StickWnd / UnstickWnd, a NULL entry
// is the screen edge) and the sizes are FLOATs (the stock code converts them where it compares). Delta is the move
// being applied, in and out; the cursor comes from the console's mouse position.
void NCWnd::Snap(POINT* Delta)
{
    if (!(GetStyle() & NWS_TOPMOST))
        return;
    const INT L = ScreenX;
    const INT T = ScreenY;
    const FLOAT W = Width;
    const FLOAT H = Height;

    // where the drag grabbed the window: the minimized icon's own grab point, else the title bar's (in the parent's
    // coordinates: the frame's grab offset plus its position)
    INT AnchorX, AnchorY;
    if (IsA(NCIcon::StaticClass()))
    {
        AnchorX = static_cast<NCIcon*>(this)->DragX;
        AnchorY = static_cast<NCIcon*>(this)->DragY;
    }
    else if (FrameCtrl)
    {
        NRectF R;
        R.X = 0;
        R.Y = 0;
        FrameCtrl->GetLocalRect(&R);
        AnchorX = FrameCtrl->GrabX + R.X;
        AnchorY = FrameCtrl->GrabY + R.Y;
    }
    else
        return;

    INT CursorX, CursorY;
    GConsole->GetMousePos(&CursorX, &CursorY);
    const INT CurX = CursorX;
    const INT CurY = CursorY;

    // stuck to a screen edge: keep still until the cursor is far enough
    if (IsStuck(SD_LEFT, NULL))
    {
        INT D = CurX - AnchorX;
        if (SnapAbs(D) >= 10)
        {
            Delta->x = D;
            UnstickWnd(SD_LEFT, NULL);
            return;
        }
        Delta->x = 0;
    }
    if (IsStuck(SD_TOP, NULL))
    {
        INT D = CurY - AnchorY;
        if (SnapAbs(D) >= 10)
        {
            Delta->y = D;
            UnstickWnd(SD_TOP, NULL);
            return;
        }
        Delta->y = 0;
    }
    if (IsStuck(SD_RIGHT, NULL))
    {
        INT D = CurX - ScreenX - AnchorX;
        if (SnapAbs(D) >= 10)
        {
            Delta->x = D;
            UnstickWnd(SD_RIGHT, NULL);
            return;
        }
        Delta->x = 0;
    }
    if (IsStuck(SD_BOTTOM, NULL))
    {
        INT D = CurY - ScreenY - AnchorY;
        if (SnapAbs(D) > 10)
        {
            Delta->y = D;
            UnstickWnd(SD_BOTTOM, NULL);
            return;
        }
        Delta->y = 0;
    }

    // stick to a screen edge
    if (!IsStuck(SD_LEFT, NULL) && L > -10 && L < 10)
    {
        Delta->x = -L;
        StickWnd(SD_LEFT, NULL);
    }
    if (!IsStuck(SD_TOP, NULL) && T > -10 && T < 10)
    {
        Delta->y = -T;
        StickWnd(SD_TOP, NULL);
    }
    if (!IsStuck(SD_RIGHT, NULL))
    {
        INT SW = GSEKScreenX;
        INT Right = (INT)W + L;
        if (SW - 10 < Right && Right < SW + 10)
        {
            Delta->x = (INT)((FLOAT)SW - ((FLOAT)L + W));
            StickWnd(SD_RIGHT, NULL);
        }
    }
    if (!IsStuck(SD_BOTTOM, NULL))
    {
        INT SH = GSEKScreenY;
        INT Bottom = (INT)H + T;
        if (SH - 10 < Bottom && Bottom < SH + 10)
        {
            Delta->y = (INT)((FLOAT)SH - ((FLOAT)T + H));
            StickWnd(SD_BOTTOM, NULL);
        }
    }

    // the other top-most windows (the window itself included, as in the stock code)
    NCVirtualWndMain* Main = Entry->Main;
    const INT Count = Main->TopMost.Num();
    for (INT i = 0; i < Count; i++)
    {
        NWndEntry* E = Main->TopMost(i);
        NCWnd* O = E->Wnd;
        const FLOAT OH = O->Height;
        const INT OL = O->ScreenX;
        const INT OT = O->ScreenY;
        const FLOAT OW = O->Width;
        if (!O->IsShowWindow())
            continue;

        // stuck to this window: come loose when the cursor pulls away or the windows no longer overlap along the
        // other axis
        if (IsStuck(SD_LEFT, E))
        {
            INT D = (INT)((FLOAT)CurX - ((FLOAT)(OL + AnchorX) + OW));
            if (SnapAbs(D) >= 10)
            {
                Delta->x = (INT)((FLOAT)(CurX - OL - AnchorX) - OW);
                UnstickWnd(SD_LEFT, E);
                continue;
            }
            const INT OHi = (INT)OH;
            const INT Hi = (INT)H;
            if (OHi + Hi < SnapAbs(OT - T + OHi) || OHi + Hi < SnapAbs(T - OT + Hi))
            {
                Delta->x = (INT)((FLOAT)(CurX - OL - AnchorX) - OW);
                UnstickWnd(SD_LEFT, E);
                continue;
            }
            Delta->x = 0;
        }
        if (IsStuck(SD_TOP, E))
        {
            INT D = (INT)((FLOAT)CurY - ((FLOAT)(AnchorY + OT) + OH));
            if (SnapAbs(D) >= 10)
            {
                Delta->y = (INT)((FLOAT)(CurY - OT - AnchorY) - OH);
                UnstickWnd(SD_TOP, E);
                continue;
            }
            const INT OWi = (INT)OW;
            const INT Wi = (INT)W;
            if (Wi + OWi < SnapAbs(OWi - L + OL) || Wi + OWi < SnapAbs(Wi - OL + L))
            {
                Delta->y = (INT)((FLOAT)(CurY - OT - AnchorY) - OH);
                UnstickWnd(SD_TOP, E);
                continue;
            }
            Delta->y = 0;
        }
        if (IsStuck(SD_RIGHT, E))
        {
            INT D = CurX - ScreenX - AnchorX;
            if (SnapAbs(D) >= 10)
            {
                Delta->x = D;
                UnstickWnd(SD_RIGHT, E);
                continue;
            }
            const INT OHi = (INT)OH;
            const INT Hi = (INT)H;
            if (OHi + Hi < SnapAbs(OT - T + OHi) || OHi + Hi < SnapAbs(T - OT + Hi))
            {
                Delta->x = D;
                UnstickWnd(SD_RIGHT, E);
                continue;
            }
            Delta->x = 0;
        }
        if (IsStuck(SD_BOTTOM, E))
        {
            INT D = CurY - ScreenY - AnchorY;
            if (SnapAbs(D) >= 10)
            {
                Delta->y = D;
                UnstickWnd(SD_BOTTOM, E);
                continue;
            }
            const INT OWi = (INT)OW;
            const INT Wi = (INT)W;
            if (Wi + OWi < SnapAbs(OWi - L + OL) || Wi + OWi < SnapAbs(Wi - OL + L))
            {
                Delta->y = D;
                UnstickWnd(SD_BOTTOM, E);
                continue;
            }
            Delta->y = 0;
        }

        // stick: left edge to its right edge, top to its bottom, right to its left, bottom to its top (the first
        // test is the stock one, on the bottom side: the stock code does not look at the left)
        if (!IsStuck(SD_BOTTOM, E))
        {
            const INT Edge = (INT)OW + OL;
            if (Edge - 10 < L && L < Edge + 10)
            {
                const INT OHi = (INT)OH;
                const INT Hi = (INT)H;
                if (SnapAbs(OT - T + OHi) <= OHi + Hi && SnapAbs(T - OT + Hi) <= OHi + Hi)
                {
                    Delta->x = (INT)(((FLOAT)OL + OW) - (FLOAT)L);
                    StickWnd(SD_LEFT, E);
                }
            }
        }
        if (!IsStuck(SD_TOP, E))
        {
            const INT Edge = (INT)OH + OT;
            if (Edge - 10 < T && T < Edge + 10)
            {
                const INT OWi = (INT)OW;
                const INT Wi = (INT)W;
                if (SnapAbs(OWi - L + OL) <= OWi + Wi && SnapAbs(Wi - OL + L) <= OWi + Wi)
                {
                    Delta->y = (INT)(((FLOAT)OT + OH) - (FLOAT)T);
                    StickWnd(SD_TOP, E);
                }
            }
        }
        if (!IsStuck(SD_RIGHT, E) && !IsStuck(SD_LEFT, E))
        {
            const INT Right = (INT)Width + L;
            if (OL - 10 < Right && Right < OL + 10)
            {
                const INT OHi = (INT)OH;
                const INT Hi = (INT)H;
                if (SnapAbs(OT - T + OHi) <= OHi + Hi && SnapAbs(T - OT + Hi) <= OHi + Hi)
                {
                    Delta->x = (INT)((FLOAT)OL - ((FLOAT)L + W));
                    StickWnd(SD_RIGHT, E);
                }
            }
        }
        if (!IsStuck(SD_BOTTOM, E) && !IsStuck(SD_TOP, E))
        {
            const INT Bottom = (INT)H + T;
            if (OT - 10 < Bottom && Bottom < OT + 10)
            {
                const INT OWi = (INT)OW;
                const INT Wi = (INT)W;
                if (SnapAbs(OWi - L + OL) <= OWi + Wi && SnapAbs(Wi - OL + L) <= OWi + Wi)
                {
                    Delta->y = (INT)((FLOAT)OT - ((FLOAT)T + H));
                    StickWnd(SD_BOTTOM, E);
                }
            }
        }
    }
}
NW_HOOK_METHOD(0x100321d0, NCWnd, Snap)

// 0x10031e40 (vslot 43, guard: SetWindowSize) - sizes at least 0; on the main windows of the game (the desktop, the
// login / lobby / creation / loading / credit / replay windows) the children follow the change (RearrangePosition)
void NCWnd::SetWindowSize(FLOAT W, FLOAT H)
{
    guard(NCWnd::SetWindowSize);
    if (W < 0.0f)
        W = 0.0f;
    if (H < 0.0f)
        H = 0.0f;
    InitStuckState();
    if ((Style & NWS_TITLEBAR) && FrameCtrl)
    {
        NRectF Local;
        GetLocalRect(&Local);
        FrameCtrl->SetFrameSize((DWORD)(INT)W, (DWORD)(INT)Local.H);
    }
    FLOAT OldW = Width;
    FLOAT OldH = Height;
    Width = W;
    Height = H;
    if (IsAnchored())
        MoveChildWindow(0, 0);
    if (IsA(NClass::NCGodWnd()) || IsA(NClass::NCLoginWnd()) || IsA(NClass::NCLobbyWnd()) ||
        IsA(NClass::NCPawnCreateWnd()) || IsA(NClass::NCLoadingWnd()) || IsA(NClass::NCCreditWnd()) ||
        IsA(NClass::NCConsole()) || IsA(NClass::NCReplayWnd()) || IsA(NClass::NCBenchMarkResultWnd()) ||
        IsA(NClass::NCReplayMenuWnd()))
    {
        for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
            CastChecked<NCWnd>(Node->Entry->Wnd)->ClearPosFlag(POS_REARRANGED | POS_SIZED);
        for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
            CastChecked<NCWnd>(Node->Entry->Wnd)->RearrangePosition(OldW, OldH, W, H);
    }
    unguard;
}
NW_HOOK_METHOD(0x10031e40, NCWnd, SetWindowSize)

// 0x10030db0 (guard: SetPosition) - X, Y relative to the parent; W or H 0 keeps the size. The children move by the
// difference between the new position and the current screen position
void NCWnd::SetPosition(INT X, INT Y, DWORD W, DWORD H)
{
    guard(NCWnd::SetPosition);
    InitStuckState();
    INT DX = X - ScreenX;
    INT DY = Y - ScreenY;
    ScreenX = X;
    ScreenY = Y;
    if (W && H)
    {
        Width = (FLOAT)W;
        Height = (FLOAT)H;
    }
    for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
        Node->Entry->Wnd->OffsetWindow(DX, DY);
    if (Entry && Entry->Parent && Entry->Parent->Wnd)
    {
        NCWnd* Parent = Entry->Parent->Wnd;
        ScreenX += Parent->ScreenX;
        ScreenY += Parent->ScreenY;
    }
    if (Style & NWS_CLIPTOPARENT)
    {
        NRectF Clip;
        Clip.X = 0;
        Clip.Y = 0;
        Clip.W = 0.0f;
        Clip.H = 0.0f;
        ClipToParent(&Clip);
    }
    unguard;
}
NW_HOOK_METHOD(0x10030db0, NCWnd, SetPosition)

// 0x10005530 - CreateChild's tail, on the child
void NCWnd::AttachEntry(NWndEntry* E)
{
    Entry = E;
    if (E->Parent)
    {
        NCWnd* Parent = E->Parent->Wnd;
        ScreenX += Parent->ScreenX;
        ScreenY += Parent->ScreenY;
    }
    WindowProc(WM_CREATE, 0, 0);
}
NW_HOOK_METHOD(0x10005530, NCWnd, AttachEntry)

// 0x10030ef0 - screen position Origin + (X, Y)
void NCWnd::SetScreenPos(INT X, INT Y, const POINT* Origin)
{
    InitStuckState();
    INT DY = ScreenY - Origin->y;
    OffsetWindow(Origin->x - ScreenX + X, 0);
    OffsetWindow(0, Y - DY);
}
NW_HOOK_METHOD(0x10030ef0, NCWnd, SetScreenPos)

// 0x10030f40 (vslot 50) - horizontally centered in the parent
void NCWnd::CenterInParent()
{
    InitStuckState();
    if (Entry->Parent && Entry->Parent->Wnd)
    {
        NRectF ParentRect;
        NRectF Local;
        ParentRect.X = 0;
        ParentRect.Y = 0;
        ParentRect.W = 0.0f;
        ParentRect.H = 0.0f;
        Entry->Parent->Wnd->GetLocalRect(&ParentRect);
        GetLocalRect(&Local);
        OffsetWindow((INT)(ParentRect.W * 0.5f - Width * 0.5f - (FLOAT)Local.X), 0);
    }
}
NW_HOOK_METHOD(0x10030f40, NCWnd, CenterInParent)

/*-----------------------------------------------------------------------------
    The four virtuals new in C2 (slots 51-54).
-----------------------------------------------------------------------------*/

// 0x10030000 (vslot 52)
void NCWnd::SetItemSelect()
{
    if (!(Style & NWS_ITEMSELECT))
        Style |= NWS_ITEMSELECT;
}
NW_HOOK_METHOD(0x10030000, NCWnd, SetItemSelect)

// 0x10030020 (vslot 53)
void NCWnd::ClearItemSelect()
{
    if (Style & NWS_ITEMSELECT)
        Style &= ~NWS_ITEMSELECT;
}
NW_HOOK_METHOD(0x10030020, NCWnd, ClearItemSelect)

// 0x1002ffc0 (vslot 54) - NCVirtualWndMain's vslot 30 (not declared there), then the modal bit
void NCWnd::SetModal(NWndEntry* E)
{
    nw::VirtualCall<void>(Entry->Main, 30, E);
    Style |= NWS_MODAL;
}
NW_HOOK_METHOD(0x1002ffc0, NCWnd, SetModal)

// 0x1002ffe0 (vslot 55) - NCVirtualWndMain's vslot 31 (not declared there), then the modal bit goes
void NCWnd::ClearModal(NWndEntry* E)
{
    nw::VirtualCall<void>(Entry->Main, 31, E);
    Style &= ~NWS_MODAL;
}
NW_HOOK_METHOD(0x1002ffe0, NCWnd, ClearModal)

/*-----------------------------------------------------------------------------
    Children
-----------------------------------------------------------------------------*/

// 0x100303c0
UBOOL NCWnd::AddChildNode(NWndEntry* Child)
{
    NWndListNode* Node = new NWndListNode;
    Node->Entry = Child;
    if (!ChildTail && !ChildHead)
    {
        ChildHead = Node;
        ChildTail = Node;
        return 1;
    }
    ChildTail->Next = Node;
    Node->Prev = ChildTail;
    ChildTail = Node;
    return 1;
}
NW_HOOK_METHOD(0x100303c0, NCWnd, AddChildNode)

// 0x10030420
void NCWnd::RemoveChildNode(NWndEntry* Child)
{
    NWndListNode* Node = ChildHead;
    while (Node && Node->Entry != Child)
        Node = Node->Next;
    if (!Node)
        return;
    if (Node->Prev)
        Node->Prev->Next = Node->Next;
    if (Node->Next)
        Node->Next->Prev = Node->Prev;
    if (ChildHead == Node)
        ChildHead = Node->Next;
    if (ChildTail == Node)
        ChildTail = Node->Prev;
    delete Node;
}
NW_HOOK_METHOD(0x10030420, NCWnd, RemoveChildNode)

// 0x1002fc80 - moves the child to the front of this list, then this window to the front of
// its parent's, up to the top level. The list must not be empty (no check in the stock code).
void NCWnd::BringChildToTop(NWndEntry* Child)
{
    NWndListNode* Node = ChildHead;
    if (Node->Entry != Child)
    {
        for (Node = Node->Next; Node && Node->Entry != Child; Node = Node->Next)
        {
        }
        if (Node)
        {
            if (Node->Next)
            {
                Node->Next->Prev = Node->Prev;
                Node->Prev->Next = Node->Next;
            }
            else
            {
                ChildTail = Node->Prev;
                Node->Prev->Next = NULL;
            }
            ChildHead->Prev = Node;
            Node->Next = ChildHead;
            ChildHead = Node;
            Node->Prev = NULL;
        }
    }
    if (Entry->Parent)
        Entry->Parent->Wnd->BringChildToTop(Entry);
}
NW_HOOK_METHOD(0x1002fc80, NCWnd, BringChildToTop)

// 0x10030990
NWndEntry* NCWnd::AddChildWindow(NCWnd* Child, DWORD ChildStyle)
{
    if (!Entry)
        return NULL;
    NWndEntry* E = Entry->Main->RegisterNWindow(Child, ChildStyle, Entry);
    E->Parent = Entry;
    AddChildNode(E);
    return E;
}
NW_HOOK_METHOD(0x10030990, NCWnd, AddChildWindow)

// 0x100309d0
void NCWnd::RemoveChildWindow(NWndEntry* Child)
{
    if (Child == GFocusEntry)
        GFocusEntry = NULL;
    if (!(Style & NWS_NOTINPARENTLIST))
        RemoveChildNode(Child);
    Entry->Main->UnRegisterNWindow(Child);
}
NW_HOOK_METHOD(0x100309d0, NCWnd, RemoveChildWindow)

/*-----------------------------------------------------------------------------
    Helpers
-----------------------------------------------------------------------------*/

// 0x10030040 - "1234567" -> "1,234,567" (does not use the window)
void NCWnd::FormatNumber(TCHAR* Out, const TCHAR* Digits)
{
    if (!Digits || !*Digits)
        return;
    INT Len = (INT)wcslen(Digits);
    INT i = Len - 1;
    INT OutLen = i / 3 + Len;
    Out[OutLen] = 0;
    TCHAR* Dst = &Out[OutLen - 1];
    for (INT n = 0; i > -1; i--, n++)
    {
        if (n % 3 == 0 && i < (INT)wcslen(Digits) - 1)
            *Dst-- = ',';
        *Dst-- = Digits[i];
    }
}
NW_HOOK_METHOD(0x10030040, NCWnd, FormatNumber)

// ---------------------------------------------------------------------------------------------------------------
// Mouse drag, focus helpers (C3)
// ---------------------------------------------------------------------------------------------------------------

// 0x100313f0 (vslot 61, guard) - a drag-able window follows the mouse; the others hand the move to the drag-able
// ancestor
INT NCWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    guard(NCWnd::OnMouseMove);
    if (!IsDragable())
    {
        NCWnd* Drag = GetDragableParentWnd();
        if (!Drag)
            return 0;
        Drag->OnMouseMove(wParam, lParam);
    }
    else if (bMouseDown)
    {
        INT X = (SWORD)lParam;
        INT Y = (SWORD)((DWORD)lParam >> 16);
        DragMouseX = X - DragMouseX;
        DragMouseY = Y - DragMouseY;
        WindowProc(WM_MOVE, 0, (DragMouseY << 16) | (WORD)DragMouseX);
        DragMouseX = X;
        DragMouseY = Y;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100313f0, NCWnd, OnMouseMove)

// 0x100314f0 (vslot 62, guard) - the press starts the drag of a drag-able window (the cursor is locked), else the
// ancestor gets it
INT NCWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    guard(NCWnd::OnLButtonDown);
    bMouseDown = 1;
    if (!(Style & NWS_DRAGABLE))
    {
        NCWnd* Drag = GetDragableParentWnd();
        if (!Drag)
            return 0;
        Drag->OnLButtonDown(wParam, lParam);
    }
    else
    {
        LockCursor();
        DragMouseX = (SWORD)lParam;
        DragMouseY = (SWORD)((DWORD)lParam >> 16);
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100314f0, NCWnd, OnLButtonDown)

// 0x100315c0 (vslot 63; its guard says OnLButtonDown)
INT NCWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    guard(NCWnd::OnLButtonDown);
    bMouseDown = 0;
    if (!(Style & NWS_DRAGABLE))
    {
        NCWnd* Drag = GetDragableParentWnd();
        if (!Drag)
            return 0;
        Drag->OnLButtonUp(wParam, lParam);
    }
    else
    {
        UnlockCursor();
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100315c0, NCWnd, OnLButtonUp)

// 0x10030230 (name ours)
UBOOL NCWnd::IsAnchored()
{
    return AnchorType != 0;
}
NW_HOOK_METHOD(0x10030230, NCWnd, IsAnchored)

// 0x10030240 (name ours)
UBOOL NCWnd::IsDragable()
{
    return (Style >> 22) & 1;
}
NW_HOOK_METHOD(0x10030240, NCWnd, IsDragable)

// 0x1002ff90 (name ours)
void NCWnd::AddPosFlag(DWORD Flag)
{
    PosFlags |= Flag;
}
NW_HOOK_METHOD(0x1002ff90, NCWnd, AddPosFlag)

// 0x1002ffa0 (name ours)
void NCWnd::ClearPosFlag(DWORD Flag)
{
    PosFlags &= ~Flag;
}
NW_HOOK_METHOD(0x1002ffa0, NCWnd, ClearPosFlag)

// 0x1012b140 (name ours) - the same two instructions are some other class's getter, folded by the linker: the
// hook serves them all
DWORD NCWnd::GetPosFlags()
{
    return PosFlags;
}
NW_HOOK_METHOD(0x1012b140, NCWnd, GetPosFlags)

// 0x10030ab0 (guard) - the nearest ancestor that can be dragged
NCWnd* NCWnd::GetDragableParentWnd()
{
    guard(NCWnd::GetDragableParentWnd);
    NCWnd* Wnd = NULL;
    if (Entry && Entry->Parent && Entry->Parent->Wnd)
        Wnd = Entry->Parent->Wnd;
    while (Wnd)
    {
        if (Wnd->IsDragable())
            return Wnd;
        NCWnd* Next = NULL;
        if (Wnd->Entry && Wnd->Entry->Parent && Wnd->Entry->Parent->Wnd)
            Next = Wnd->Entry->Parent->Wnd;
        Wnd = Next;
    }
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x10030ab0, NCWnd, GetDragableParentWnd)

// 0x10030900 - the rectangle relative to the parent; Out comes back (the stock callers use it: eax)
NRectF* NCWnd::GetLocalRect(NRectF* Out)
{
    INT ParentX = 0;
    INT ParentY = 0;
    if (Entry && Entry->Parent && Entry->Parent->Wnd)
    {
        ParentX = Entry->Parent->Wnd->ScreenX;
        ParentY = Entry->Parent->Wnd->ScreenY;
    }
    Out->X = ScreenX - ParentX;
    Out->Y = ScreenY - ParentY;
    Out->H = Height;
    Out->W = Width;
    return Out;
}
NW_HOOK_METHOD(0x10030900, NCWnd, GetLocalRect)

// 0x10030950 - the position relative to the parent; Out comes back (NCHtmlViewer::ShowWindow reads it in eax)
INT* NCWnd::GetLocalPos(INT* Out)
{
    INT ParentX = 0;
    INT ParentY = 0;
    if (Entry && Entry->Parent && Entry->Parent->Wnd)
    {
        ParentX = Entry->Parent->Wnd->ScreenX;
        ParentY = Entry->Parent->Wnd->ScreenY;
    }
    Out[0] = ScreenX - ParentX;
    Out[1] = ScreenY - ParentY;
    return Out;
}
NW_HOOK_METHOD(0x10030950, NCWnd, GetLocalPos)

// 0x1002fcf0 - the first window of the subtree (this one first) that takes keyboard input and shows takes the focus
UBOOL NCWnd::FocusFirstInput()
{
    NCWnd* Wnd = Entry->Wnd;
    if (Wnd && Wnd->vfn032() && Wnd->IsShowWindow())
    {
        Wnd->SetFocus();
        return 1;
    }
    for (NWndListNode* Node = Wnd->ChildHead; Node; Node = Node->Next)
    {
        if (Node->Entry->Wnd->FocusFirstInput())
            return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x1002fcf0, NCWnd, FocusFirstInput)

// 0x10030310 - a child of the window is a slider bar
UBOOL NCWnd::HasSliderBarChild()
{
    for (NWndListNode* Node = Entry->Wnd->ChildHead; Node; Node = Node->Next)
    {
        if (Node->Entry->Wnd->IsA(NClass::NCSliderBar()))
            return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x10030310, NCWnd, HasSliderBarChild)

// 0x10030350 - a slider bar in the subtree hands the focus to its parent
UBOOL NCWnd::FocusSliderBarParent()
{
    NCWnd* Wnd = Entry->Wnd;
    if (Wnd && Wnd->IsA(NClass::NCSliderBar()))
    {
        if (Entry->Parent)
            Entry->Parent->Wnd->SetFocus();
        return 1;
    }
    for (NWndListNode* Node = Wnd->ChildHead; Node; Node = Node->Next)
    {
        if (Node->Entry->Wnd->FocusSliderBarParent())
            return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x10030350, NCWnd, FocusSliderBarParent)

// 0x10030250 - after a window went away: the focus goes to the first visible window of the desktop (after the
// first one) that has not NWS_NOFOCUSNEXT
void NCWnd::FocusNextWindow(NWndEntry* Hidden)
{
    if (!Hidden)
        return;
    NWndEntry* Top = Hidden;
    for (;;)
    {
        NWndEntry* Up = Top->Parent;
        if (!Up || Up->Wnd == GConsole->Desktop)
        {
            if (!Top->Parent)
                Top = NULL;
            break;
        }
        Top = Up;
    }
    if (GConsole->Desktop->IsShowWindow() && Top && GConsole && GConsole->Desktop && GConsole->Desktop->Entry)
    {
        NWndListNode* Node = GConsole->Desktop->Entry->Wnd->ChildHead;
        if (!Node)
            return;
        for (;;)
        {
            Node = Node->Next;
            if (!Node)
                return;
            DWORD NodeStyle = Node->Entry->Wnd->GetStyle();
            UBOOL bNotMarked = (NodeStyle & NWS_NOFOCUSNEXT) != NWS_NOFOCUSNEXT;
            if (Node->Entry->Wnd->IsShowWindow() && bNotMarked)
                break;
        }
        Node->Entry->Wnd->SetFocus();
    }
}
NW_HOOK_METHOD(0x10030250, NCWnd, FocusNextWindow)

// ---------------------------------------------------------------------------------------------------------------
// Sticky windows and anchors (Src\NWnd.cpp, C3)
// ---------------------------------------------------------------------------------------------------------------

// 0x10030d10 (guard)
void NCWnd::InitStuckState()
{
    guard(NCWnd::InitStuckState);
    for (INT i = 0; i < SD_MAX; i++)
        StuckWnds[i].Empty();
    unguard;
}
NW_HOOK_METHOD(0x10030d10, NCWnd, InitStuckState)

// 0x10030170 (guard) - the opposite side: top <-> bottom, left <-> right
INT NCWnd::GetOppStickDirection(INT Direction)
{
    guard(NCWnd::GetOppStickDirection);
    INT Opp = -1;
    switch (Direction)
    {
    case SD_TOP:
        Opp = SD_BOTTOM;
        break;
    case SD_LEFT:
        Opp = SD_RIGHT;
        break;
    case SD_RIGHT:
        Opp = SD_LEFT;
        break;
    case SD_BOTTOM:
        Opp = SD_TOP;
        break;
    default:
        NW_CHECK_AT(0, "Src\\NWnd.cpp", 0x67e);
    }
    return Opp;
    unguard;
}
NW_HOOK_METHOD(0x10030170, NCWnd, GetOppStickDirection)

// 0x10030a10 (guard) - holds the window at one of nine places of its parent, Type 1..9, at the offset
void NCWnd::SetAnchor(INT Type, INT OffsetX, INT OffsetY)
{
    guard(NCWnd::SetAnchor);
    AnchorType = Type;
    AnchorOffsetX = OffsetX;
    AnchorOffsetY = OffsetY;
    if (IsAnchored())
        MoveChildWindow(0, 0);
    unguard;
}
NW_HOOK_METHOD(0x10030a10, NCWnd, SetAnchor)

// 0x10030490 (guard) - moves the window by (DX, DY) and everything below it. An anchored window goes to the place
// of its anchor in the parent instead (Type 1..9 = the cells of a 3 x 3 grid, numeric-keypad order), plus the offset
void NCWnd::MoveChildWindow(INT DX, INT DY)
{
    guard(NCWnd::MoveChildWindow);
    if (IsAnchored() && Entry && Entry->Parent && Entry->Parent->Wnd)
    {
        NCWnd* Parent = Entry->Parent->Wnd;
        FLOAT ParentW = Parent->Width;
        FLOAT ParentH = Parent->Height;
        INT ParentX = Parent->ScreenX;
        INT ParentY = Parent->ScreenY;
        NRectF Own = Rect;
        INT LocalX = ScreenX - ParentX;
        INT LocalY = ScreenY - ParentY;
        INT PX;
        INT PY;
        switch (AnchorType)
        {
        case 1:
            PX = 0;
            PY = 0;
            break;
        case 2:
            PX = (INT)(ParentW * 0.5f - Own.W * 0.5f);
            PY = 0;
            break;
        case 3:
            PX = (INT)(ParentW - Own.W);
            PY = 0;
            break;
        case 4:
            PX = 0;
            PY = (INT)(ParentH * 0.5f - Own.H * 0.5f);
            break;
        case 5:
            PX = (INT)(ParentW * 0.5f - Own.W * 0.5f);
            PY = (INT)(ParentH * 0.5f - Own.H * 0.5f);
            break;
        case 6:
            PX = (INT)(ParentW - Own.W);
            PY = (INT)(ParentH * 0.5f - Own.H * 0.5f);
            break;
        case 7:
            PX = 0;
            PY = (INT)(ParentH - Own.H);
            break;
        case 8:
            PX = (INT)(ParentW * 0.5f - Own.W * 0.5f);
            PY = (INT)(ParentH - Own.H);
            break;
        case 9:
            PX = (INT)(ParentW - Own.W);
            PY = (INT)(ParentH - Own.H);
            break;
        default:
            PX = DY;
            PY = DY;
            break;
        }
        DX = PX + AnchorOffsetX - LocalX;
        DY = PY + AnchorOffsetY - LocalY;
    }
    ScreenX += DX;
    ScreenY += DY;
    for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
        Node->Entry->Wnd->MoveChildWindow(DX, DY);
    unguard;
}
NW_HOOK_METHOD(0x10030490, NCWnd, MoveChildWindow)

// 0x100306d0 - keeps the window inside its parent: the part of it that lies outside is cut off (Out gets the offset
// of the cut at the left and top, and the new size). A window that lies completely outside hides.
void NCWnd::ClipToParent(NRectF* Out)
{
    INT Left = ScreenX;
    INT Top = ScreenY;
    INT W = (INT)Width;
    INT H = (INT)Height;
    if (Entry && Entry->Parent && Entry->Parent->Wnd)
    {
        NCWnd* Parent = Entry->Parent->Wnd;
        INT ParentLeft = Parent->ScreenX;
        INT ParentTop = Parent->ScreenY;
        INT ParentW = (INT)Parent->Width;
        if (Left <= ParentW + ParentLeft && ParentLeft <= W + Left)
        {
            INT ParentH = (INT)Parent->Height;
            if (Top <= ParentH + ParentTop && ParentTop <= H + Top)
            {
                FLOAT NewW;
                FLOAT NewH;
                if (Left < ParentLeft)
                {
                    Out->X = ParentLeft - Left;
                    ClipOffsetX = ParentLeft - Left;
                    W = W - Out->X;
                    NewW = (FLOAT)W;
                    Left = ParentLeft;
                }
                else
                {
                    ClipOffsetX = 0;
                    NewW = (FLOAT)W;
                }
                INT FullW = (INT)Width;
                if (ParentW + ParentLeft < FullW + ScreenX)
                    NewW = (FLOAT)(W + ((ParentW - FullW) - ScreenX) + ParentLeft);
                Out->W = NewW;
                if (ScreenY < ParentTop)
                {
                    Out->Y = ParentTop - ScreenY;
                    ClipOffsetY = Out->Y;
                    H = H - Out->Y;
                    Top = ParentTop;
                }
                else
                {
                    ClipOffsetY = 0;
                }
                FLOAT ResultH = (FLOAT)H;
                INT FullH = (INT)Height;
                if (ParentH + ParentTop < FullH + ScreenY)
                    ResultH = (FLOAT)(H + ((ParentH - FullH) - ScreenY) + ParentTop);
                Out->H = ResultH;
                if (Out->W == 0.0f && ResultH == 0.0f)
                {
                    Out->X = 0;
                    Out->Y = 0;
                    Out->W = Width;
                    Out->H = Height;
                    return;
                }
                Height = ResultH;
                ScreenY = Top;
                ScreenX = Left;
                Width = NewW;
                return;
            }
        }
        HideWindow();
    }
}
NW_HOOK_METHOD(0x100306d0, NCWnd, ClipToParent)

// 0x10031680 (guard) - this window and everything stuck to it, directly or not, each once
void NCWnd::GetStuckWndList(TArray<NCWnd*>& a_StuckWndList)
{
    guard(NCWnd::GetStuckWndList);
    NW_CHECK_AT(&a_StuckWndList != NULL, "Src\\NWnd.cpp", 0x5cb);
    if (a_StuckWndList.FindItemIndex(this) != INDEX_NONE)
        return;
    a_StuckWndList.AddItem(this);
    for (INT i = 0; i < SD_MAX; i++)
    {
        for (INT j = 0; j < StuckWnds[i].Num(); j++)
        {
            if (StuckWnds[i](j))
                StuckWnds[i](j)->Wnd->GetStuckWndList(a_StuckWndList);
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x10031680, NCWnd, GetStuckWndList)

// 0x10031770 (guard) - Direction -1: on any side. Wnd -1: anything is stuck there, -2: something other than NULL
UBOOL NCWnd::IsStuck(INT Direction, NWndEntry* Wnd)
{
    guard(NCWnd::IsStuck);
    if (Direction != -1)
    {
        NW_CHECK_AT(0 <= Direction && Direction < SD_MAX, "Src\\NWnd.cpp", 0x610);
        TArray<NWndEntry*>& List = StuckWnds[Direction];
        if ((INT)Wnd == -1)
            return List.Num() != 0;
        if ((INT)Wnd == -2)
        {
            if (List.Num() < 1)
                return 0;
            if (List.Num() != 1)
                return 1;
            return List(0) != NULL;
        }
        return List.FindItemIndex(Wnd) != INDEX_NONE;
    }
    for (INT i = 0; i < SD_MAX; i++)
    {
        TArray<NWndEntry*>& List = StuckWnds[i];
        if ((INT)Wnd == -1)
        {
            if (List.Num() != 0)
                return 1;
        }
        else if ((INT)Wnd == -2)
        {
            if (List.Num() > 0)
            {
                if (List.Num() != 1)
                    return 1;
                if (List(0) != NULL)
                    return 1;
            }
        }
        else
        {
            for (INT j = 0; j < List.Num(); j++)
            {
                if (List(j) == Wnd)
                    return 1;
            }
        }
    }
    return 0;
    unguard;
}
NW_HOOK_METHOD(0x10031770, NCWnd, IsStuck)

// 0x10032090 (guard) - sticks Wnd to this window's side Direction, and this window to Wnd's opposite side
void NCWnd::StickWnd(INT Direction, NWndEntry* Wnd)
{
    guard(NCWnd::StickWnd);
    NW_CHECK_AT(0 <= Direction && Direction < SD_MAX, "Src\\NWnd.cpp", 0x634);
    if (IsStuck(Direction, Wnd))
        return;
    if (Wnd)
    {
        for (INT i = 0; i < SD_MAX; i++)
        {
            if (i != Direction && IsStuck(i, Wnd))
                UnstickWnd(i, Wnd);
        }
    }
    StuckWnds[Direction].AddItem(Wnd);
    if (Wnd)
    {
        NCWnd* Other = CastChecked<NCWnd>(Wnd->Wnd);
        INT Opp = GetOppStickDirection(Direction);
        if (!Other->IsStuck(Opp, Entry))
            Other->StickWnd(Opp, Entry);
    }
    unguard;
}
NW_HOOK_METHOD(0x10032090, NCWnd, StickWnd)

// 0x10031ca0 (guard) - the reverse: Wnd leaves this window's side Direction, and this window the opposite side of Wnd
void NCWnd::UnstickWnd(INT Direction, NWndEntry* Wnd)
{
    guard(NCWnd::UnstickWnd);
    if (IsStuck(Direction, Wnd))
    {
        StuckWnds[Direction].RemoveItem(Wnd);
        if (Wnd)
        {
            NCWnd* Other = CastChecked<NCWnd>(Wnd->Wnd);
            INT Opp = GetOppStickDirection(Direction);
            if (Other->IsStuck(Opp, Entry))
                Other->UnstickWnd(Opp, Entry);
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x10031ca0, NCWnd, UnstickWnd)

// 0x10031920 (guard) - the bounding rectangle of this window and everything stuck to it (a_pRect comes in with the
// window's own rectangle), and the windows that make each side of it
void NCWnd::GetPositioningRect(NRectF* a_pRect, NCWnd** ppLeft, NCWnd** ppRight, NCWnd** ppTop, NCWnd** ppBottom)
{
    guard(NCWnd::GetPositioningRect);
    if (!a_pRect)
        NW_CHECK_AT(0, "Src\\NWnd.cpp", 0x55b);
    TArray<NCWnd*> StuckWndList;
    GetStuckWndList(StuckWndList);
    for (INT i = 0; i < StuckWndList.Num(); i++)
    {
        NCWnd* pStuckWnd = StuckWndList(i);
        if (!pStuckWnd)
            NW_CHECK_AT(0, "Src\\NWnd.cpp", 0x563);
        INT WndX = pStuckWnd->ScreenX;
        FLOAT WndH = pStuckWnd->Height;
        INT WndY = pStuckWnd->ScreenY;
        FLOAT WndW = pStuckWnd->Width;
        if (WndX < a_pRect->X)
        {
            INT OldX = a_pRect->X;
            a_pRect->X = WndX;
            a_pRect->W = (FLOAT)(OldX - WndX) + a_pRect->W;
            *ppLeft = pStuckWnd;
        }
        if (WndY < a_pRect->Y)
        {
            INT OldY = a_pRect->Y;
            a_pRect->Y = WndY;
            a_pRect->H = (FLOAT)(OldY - WndY) + a_pRect->H;
            *ppTop = pStuckWnd;
        }
        FLOAT Right = WndW + (FLOAT)WndX;
        if ((FLOAT)a_pRect->X + a_pRect->W < Right)
        {
            a_pRect->W = Right - (FLOAT)a_pRect->X;
            *ppRight = pStuckWnd;
        }
        FLOAT Bottom = WndH + (FLOAT)WndY;
        if ((FLOAT)a_pRect->Y + a_pRect->H < Bottom)
        {
            a_pRect->H = Bottom - (FLOAT)a_pRect->Y;
            *ppBottom = pStuckWnd;
        }
    }
    StuckWndList.Empty();
    unguard;
}
NW_HOOK_METHOD(0x10031920, NCWnd, GetPositioningRect)

// 0x10031ac0 (guard) - the parent changed its size from (OldW, OldH) to (NewW, NewH): this window and the ones
// stuck to it move along, in proportion to where the group sat in the free space (ScaleX / ScaleY remember it)
void NCWnd::RearrangePosition(FLOAT OldW, FLOAT OldH, FLOAT NewW, FLOAT NewH)
{
    guard(NCWnd::RearrangePosition);
    if ((GetPosFlags() & POS_REARRANGED) == 0)
    {
        NRectF Group;
        Group.X = ScreenX;
        Group.Y = ScreenY;
        Group.W = Width;
        Group.H = Height;
        NCWnd* pLeft = this;
        NCWnd* pTop = this;
        NCWnd* pRight;
        NCWnd* pBottom;
        GetPositioningRect(&Group, &pLeft, &pRight, &pTop, &pBottom);
        FLOAT FromLeft = (FLOAT)pLeft->ScreenX;
        FLOAT FromTop = (FLOAT)pTop->ScreenY;
        FLOAT ToLeft = (FLOAT)-pLeft->ScreenX;
        FLOAT ToTop = (FLOAT)-pTop->ScreenY;
        if (Group.W + 200.0f < OldW)
            ScaleX = FromLeft / (OldW - Group.W);
        if (Group.H + 200.0f < OldH)
            ScaleY = FromTop / (OldH - Group.H);
        if (Group.W < NewW)
            ToLeft = (NewW - Group.W) * ScaleX - FromLeft;
        if (Group.H < NewH)
            ToTop = (NewH - Group.H) * ScaleY - FromTop;
        TArray<NCWnd*> StuckWndList;
        GetStuckWndList(StuckWndList);
        for (INT i = 0; i < StuckWndList.Num(); i++)
        {
            NCWnd* pStuckWnd = StuckWndList(i);
            if (!pStuckWnd)
                NW_CHECK_AT(0, "Src\\NWnd.cpp", 0x5be);
            pStuckWnd->AddPosFlag(POS_REARRANGED);
            pStuckWnd->MoveChildWindow((INT)ToLeft, (INT)ToTop);
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x10031ac0, NCWnd, RearrangePosition)

// NCCmdTarget's own vtable is the one its destructor stores (its constructor is NCObject's,
// folded by the linker). The original's has 37 slots: 24..36 are declared in NCWnd here.
// 0x1007d6b0 (vslots 27, 58, 63, 64, 65, 67 of many classes) - `xor eax, eax; ret 8`: the body the linker folded for
// every handler that does nothing and returns 0
NW_HOOK_METHOD(0x1007D6B0, NCWnd, OnRButtonDown)

// 0x10030ba0 - CastChecked<NCWnd>, the stock's out-of-line instance (NWnd.cpp's casts of windows)
static NCWnd* CastCheckedNCWnd(UObject* Object)
{
    return CastChecked<NCWnd>(Object);
}
NW_HOOK_FUNC(0x10030BA0, CastCheckedNCWnd)

NW_CHECK_VTABLE(NCCmdTarget, 0x101763d0, 39)
NW_CHECK_VTABLE(NCWnd, 0x10176500, 68)
