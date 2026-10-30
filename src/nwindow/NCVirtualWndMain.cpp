/*=============================================================================
    NCVirtualWndMain.cpp - window manager: registration, routing, painting, cursors.
    Stock: 0x10024290 .. 0x10025bb0 (+ ~NCAbstractMain 0x10024690).
=============================================================================*/
#include "nwindow/NCVirtualWndMain.h"
#include "re/NwDebug.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NConsoleWnd.h"
#if NW_FEATURES
#include "features/WindowFix.h"   // phase 6: WindowFix
#endif

#include <math.h>

// Messages DispatchWndMsg accepts (stock table at 0x101F6250, 27 entries, their count in the dword at 0x101F62D8: the
// stock keeps the count in a variable). The ones without a case below (WM_CREATE, WM_DESTROY, WM_TIMER,
// WM_SYSKEYDOWN, NWM_DRAWTOOLTIP) end as "not handled".
#if NW_HOOK_MODE
#define GDispatchedMsgs    ((const UINT*)::nw::Addr(0x101F6250))
#define GNumDispatchedMsgs NW_GLOBAL(UINT, 0x101F62D8)
#else
static const UINT GDispatchedMsgs[] = {
    WM_LBUTTONDBLCLK, WM_RBUTTONDBLCLK, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP,
    WM_MOUSEMOVE, WM_KEYDOWN, WM_KEYUP, WM_CHAR, WM_PAINT, WM_INPUTLANGCHANGE, WM_IME_SETCONTEXT,
    WM_IME_STARTCOMPOSITION, WM_IME_COMPOSITION, WM_IME_ENDCOMPOSITION, WM_IME_COMPOSITIONFULL,
    WM_IME_NOTIFY, WM_IME_CONTROL, WM_CREATE, WM_DESTROY, WM_TIMER, WM_MOUSEWHEEL, WM_MOVE,
    WM_SYSKEYDOWN, WM_SETCURSOR, NWM_DRAWTOOLTIP,
};
static const UINT GNumDispatchedMsgs = ARRAY_COUNT(GDispatchedMsgs);
#endif

// the byte at 0x10271AD8 (C3, name ours): a double click is under way. WM_LBUTTONDBLCLK sets it; the button up that
// follows clears it instead of releasing the cursor lock; a WM_LBUTTONDOWN / WM_RBUTTONDBLCLK that comes while it is
// set is taken without being routed
#if NW_HOOK_MODE
#define GLButtonDblClk NW_GLOBAL(BYTE, 0x10271AD8)
#else
static BYTE GLButtonDblClk;
#endif

// Cursor resources of the package (stock table at 0x1010b2d4), index = CursorType.
static const WORD GCursorIDs[7] = {0x66, 0x65, 0x6a, 0x6b, 0x7f, 0x82, 0x83};

/*-----------------------------------------------------------------------------
    Construction
-----------------------------------------------------------------------------*/


// 0x10034db0 - the arrays and the counters cleared, the paint device made in this object's package (the stock passes
// its Outer, +0x18) and given its initial state (NCGDevice::Init: a stub, the stock one runs)
NCVirtualWndMain::NCVirtualWndMain()
{
    guard(NCVirtualWndMain::NCVirtualWndMain);
    GDevice = NULL;
    NumWindows = 0;
    CursorType = 0;
    bCursorEnabled = 1;
    bColorCursor = 0;
    GDevice = new (GetOuter()) NCGDevice;
    GDevice->Init();
    bToolTipShown = 0;
    bMouseMoveTaken = 0;
    bCursorEnabled = 1;
    unguard;
}
NW_HOOK_CTOR(0x10034db0, NCVirtualWndMain)


// 0x100330c0
void NCVirtualWndMain::DeleteGDevice()
{
    if (GDevice)
    {
        delete GDevice;
        GDevice = NULL;
    }
}
NW_HOOK_METHOD(0x100330c0, NCVirtualWndMain, DeleteGDevice)

// 0x10032d60 - the canvas of the frame being drawn
void NCVirtualWndMain::SetGDevice(UCanvas* Canvas)
{
    guard(NCVirtualWndMain::SetGDevice);
    GDevice->SetCanvas(Canvas);
    unguard;
}
NW_HOOK_METHOD(0x10032d60, NCVirtualWndMain, SetGDevice)

// 0x10032de0
void NCVirtualWndMain::ReleaseGDevice()
{
    guard(NCVirtualWndMain::ReleaseGDevice);
    GDevice->ClearCanvas();
    unguard;
}
NW_HOOK_METHOD(0x10032de0, NCVirtualWndMain, ReleaseGDevice)

/*-----------------------------------------------------------------------------
    Registration
-----------------------------------------------------------------------------*/

// 0x10034310 (vslot 25) - a record for the window (C2/C3: up to 2000 windows) in the list of all, in the topmost list
// when its style says so and in the top-level one when it has no parent; the window takes its style
NWndEntry* NCVirtualWndMain::RegisterNWindow(NCWnd* Wnd, DWORD Style, NWndEntry* Parent)
{
    guard(NCVirtualWndMain::RegisterNWindow);
    if (NumWindows >= 2000)
        return NULL;
    NWndEntry* E = new NWndEntry;
    E->bValid = 1;
    E->Wnd = Wnd;
    Wnd->SetStyle(Style);
    E->Main = this;
    E->Parent = Parent;
    Windows.AddItem(E);
    NumWindows++;
    if (E->Wnd && (E->Wnd->GetStyle() & NWS_TOPMOST))
        TopMost.AddItem(E);
    if (!E->Parent)
        TopLevel.AddItem(E);
    return E;
    unguard;
}
NW_HOOK_METHOD(0x10034310, NCVirtualWndMain, RegisterNWindow)

// 0x10034450 (vslot 26) - the entry leaves the lists; the window gets WM_DESTROY, the entry is freed
void NCVirtualWndMain::UnRegisterNWindow(NWndEntry* Entry)
{
    guard(NCVirtualWndMain::UnRegisterNWindow);
    if (!Entry->Parent)
    {
        for (INT i = 0; i < TopLevel.Num(); i++)
        {
            if (TopLevel(i) == Entry)
            {
                TopLevel.Remove(i);
                break;
            }
        }
    }
    for (INT i = 0; i < TopMost.Num(); i++)   // C2: the topmost list too
    {
        if (TopMost(i) == Entry)
        {
            TopMost.Remove(i);
            break;
        }
    }
    for (INT i = 0; i < Windows.Num(); i++)
    {
        if (Windows(i) == Entry)
        {
            Entry->Wnd->WindowProc(WM_DESTROY, 0, 0);
            Windows.Remove(i);
            delete Entry;
            NumWindows--;
            break;
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x10034450, NCVirtualWndMain, UnRegisterNWindow)

// 0x100342e0 (vslot 29)
void NCVirtualWndMain::AddTopLevel(NWndEntry* Entry)
{
    Entry->Parent = NULL;
    TopLevel.AddItem(Entry);
}
NW_HOOK_METHOD(0x100342e0, NCVirtualWndMain, AddTopLevel)

// 0x10034d20 (vslot 30) - new in C2: a window takes the focus for itself
void NCVirtualWndMain::PushFocusWindow(NWndEntry* Entry)
{
    if (Entry)
        FocusWindows.AddItem(Entry);
}
NW_HOOK_METHOD(0x10034d20, NCVirtualWndMain, PushFocusWindow)

// 0x10034d50 (vslot 31) - the window leaves the stack (every copy of it); the one on top gets the focus back
void NCVirtualWndMain::PopFocusWindow(NWndEntry* Entry)
{
    if (Entry)
    {
        for (INT i = 0; i < FocusWindows.Num(); i++)
        {
            if (FocusWindows(i) == Entry)
            {
                FocusWindows.Remove(i);
                i--;
            }
        }
        if (FocusWindows.Num() > 0)
            FocusWindows.Last()->Wnd->SetFocus();
    }
}
NW_HOOK_METHOD(0x10034d50, NCVirtualWndMain, PopFocusWindow)

// 0x100330e0 (vslot 32)
INT NCVirtualWndMain::NumFocusWindows()
{
    return FocusWindows.Num();
}
NW_HOOK_METHOD(0x100330e0, NCVirtualWndMain, NumFocusWindows)

// 0x10033be0 (vslot 28)
void NCVirtualWndMain::ClearFocus()
{
    guard(NCVirtualWndMain::ClearFocus);
    for (DWORD i = 0; i < NumWindows; i++)
    {
        NWndEntry* E = Windows(i);
        if (E->bValid && E->Wnd->IsFocused())
            Windows(i)->Wnd->KillFocus();
    }
    unguard;
}
NW_HOOK_METHOD(0x10033be0, NCVirtualWndMain, ClearFocus)

/*-----------------------------------------------------------------------------
    Hit tests
-----------------------------------------------------------------------------*/

// 0x10033d10 (vslot 27) - searches the first visible top-level window only
NWndEntry* NCVirtualWndMain::FindNWindow(INT X, INT Y)
{
    guard(NCVirtualWndMain::FindNWindow);
    for (INT i = 0; i < TopLevel.Num(); i++)
    {
        if ((TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE) && TopLevel(i)->Wnd->IsShowWindow() &&
            TopLevel(i)->Wnd->IsInWindow(X, Y))   // C2 asks the point too
            return FindChildWindowAt(TopLevel(i)->Wnd, X, Y);
    }
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x10033d10, NCVirtualWndMain, FindNWindow)

// 0x10033c80 (vslot 33) - deepest window of the tree under (X, Y), front child first
NWndEntry* NCVirtualWndMain::FindChildWindowAt(NCWnd* Wnd, INT X, INT Y)
{
    NWndEntry* Found = Wnd->Entry;
    for (NWndListNode* Node = Wnd->ChildHead; Node; Node = Node->Next)
    {
        NCWnd* Child = Node->Entry->Wnd;
        if (!(Child->GetStyle() & NWS_NOHITTEST) && Child->IsShowWindow() && Child->IsInWindow(X, Y))
            return FindChildWindowAt(Child, X, Y);
    }
    return Found;
}
NW_HOOK_METHOD(0x10033c80, NCVirtualWndMain, FindChildWindowAt)

// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)
// 0x10034260 - is any window at (X, Y)? (clicks elsewhere belong to the game world)
UBOOL NCVirtualWndMain::IsOverWindow(INT X, INT Y)
{
    for (DWORD i = 0; i < NumWindows; i++)
    {
        NWndEntry* E = Windows(i);
        if (E->bValid && !(E->Wnd->GetStyle() & NWS_NOHITTEST) && Windows(i)->Wnd->IsShowWindow() &&
            Windows(i)->Wnd->IsInWindow(X, Y))
            return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x10034260, NCVirtualWndMain, IsOverWindow)

// 0x10033540 - packed position inside the window rectangle
__declspec(noinline) UBOOL NCVirtualWndMain::HitTest(NWndEntry* Entry, LPARAM Pos)
{
    INT X = (SWORD)Pos;
    INT Y = (SWORD)((DWORD)Pos >> 16);
    if (Entry->bValid && Entry->Wnd->IsShowWindow())
    {
        NCWnd* W = Entry->Wnd;
        if (W->PtInRect(X, Y, W->Rect))
            return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x10033540, NCVirtualWndMain, HitTest)

// 0x10033280 - style 0x200 windows only take clicks inside their parent too; a shown style 0x10 window (the
// drop-down list of a combo box, C1 asked IsA(NCHtmlComboBox)) is tested against its own rectangle
UBOOL NCVirtualWndMain::HitTestClick(NWndEntry* Entry, INT X, INT Y)
{
    if (Entry->bValid)
    {
        if (!(Entry->Wnd->GetStyle() & 0x10) || !Entry->Wnd->IsShowWindow())
        {
            if (Entry->bValid && (Entry->Wnd->GetStyle() & 0x200) && Entry->Wnd->IsShowWindow() &&
                Entry->Wnd->IsInWindow(X, Y) && Entry->Parent->Wnd)
            {
                NCWnd* ParentWnd = Entry->Parent->Wnd;
                return Entry->Wnd->PtInRect(X, Y, ParentWnd->Rect) != 0;
            }
        }
        else
        {
            NCWnd* W = Entry->Wnd;
            if (W->PtInRect(X, Y, W->Rect))
                return 1;
        }
    }
    if (Entry->Wnd->IsInWindow(X, Y) && (Entry->Wnd->GetStyle() & NWS_VISIBLE) &&
        !(Entry->Wnd->GetStyle() & NWS_NOHITTEST))
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10033280, NCVirtualWndMain, HitTestClick)

/*-----------------------------------------------------------------------------
    Routing
-----------------------------------------------------------------------------*/

// whether the game window is fullscreen (UViewport vslot 30, the first viewport of the client)
static FORCEINLINE UBOOL ViewportIsFullscreen()
{
    return GConsole->Engine->Client->Viewports(0)->IsFullscreen();
}

// Not a function of the C2 image (C1 0x100243c0 is gone): DispatchWndMsg has the two instructions' worth
// inline in its WM_MOUSEMOVE and WM_LBUTTONDBLCLK cases. A client position packed in an LPARAM -> the
// screen position of the game window, packed the same way.
static FORCEINLINE LPARAM ClientToScreenInline(LPARAM Pos)
{
    POINT P;
    P.x = (SWORD)Pos;
    P.y = (SWORD)((DWORD)Pos >> 16);
    ClientToScreen(GViewportHWnd, &P);
    return (LPARAM)(((DWORD)(_WORD)P.y << 16) | (_WORD)P.x);
}
LPARAM NCVirtualWndMain::ClientToScreenPacked(LPARAM Pos)
{
    return ClientToScreenInline(Pos);
}

// 0x100331b0 - the mouse position the windows work with: a fullscreen game window keeps it, a windowed one
// has the screen position turned into a client one
LPARAM NCVirtualWndMain::CoordCompensate(LPARAM Pos)
{
    guard(NCVirtualWndMain::CoordCompensate);
    if (ViewportIsFullscreen())
        return Pos;
    POINT P;
    P.x = (SWORD)Pos;
    P.y = (SWORD)((DWORD)Pos >> 16);
    ScreenToClient(GViewportHWnd, &P);
    return (LPARAM)(((DWORD)(_WORD)P.y << 16) | (_WORD)P.x);
    unguard;
}
NW_HOOK_METHOD(0x100331b0, NCVirtualWndMain, CoordCompensate)

// 0x10032d30 - the message is one of the table's
UBOOL NCVirtualWndMain::IsDispatchedMsg(UINT Msg)
{
    for (UINT i = 0; i < GNumDispatchedMsgs; i++)
        if (GDispatchedMsgs[i] == Msg)
            return 1;
    return 0;
}
NW_HOOK_METHOD(0x10032d30, NCVirtualWndMain, IsDispatchedMsg)

// 0x100333f0 - the deepest window hit takes the click and the focus (C2: not a style 4 window; a window that
// is not hit still passes the click to its children when its style says so)
INT NCVirtualWndMain::RouteClick(NWndEntry* Entry, INT X, INT Y, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    INT Result = 0;
    if (HitTestClick(Entry, X, Y))
    {
        for (NWndListNode* Node = Entry->Wnd->ChildHead; Node; Node = Node->Next)
        {
            Result = RouteClick(Node->Entry, X, Y, Msg, wParam, lParam);
            if (Result)
                return Result;
        }
        Result = 0;
        if (!(Entry->Wnd->GetStyle() & 4))
        {
            Entry->Wnd->SetFocus();
            Entry->Wnd->WindowProc(Msg, wParam, lParam);
            return 1;
        }
        return Result;
    }
    if (Entry && Entry->Wnd &&
        ((Entry->Wnd->GetStyle() & 8) || (Entry->Wnd->GetStyle() & 4) || (Entry->Wnd->GetStyle() & 0x2000) ||
         (Entry->Wnd->GetStyle() & 0x10000) || (Entry->Wnd->GetStyle() & 0x10)))
    {
        for (NWndListNode* Node = Entry->Wnd->ChildHead; Node; Node = Node->Next)
        {
            Result = RouteClick(Node->Entry, X, Y, Msg, wParam, lParam);
            if (Result)
                break;
        }
    }
    return Result;
}
NW_HOOK_METHOD(0x100333f0, NCVirtualWndMain, RouteClick)

// 0x100335d0 - while the left button is down the focused window gets every move (drags); otherwise the first window
// under the cursor (NCResizeFrame: always). Always returns 1. C3: bMouseOnWindow follows who took the move - cleared
// by a drag and by the desktop, set (with bMouseMoveTaken) by any other window - for NConsoleWnd::UpdateCursor
INT NCVirtualWndMain::RouteMouseMove(NWndEntry* Entry, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    for (NWndListNode* Node = Entry->Wnd->ChildHead; Node; Node = Node->Next)
        RouteMouseMove(Node->Entry, Msg, wParam, lParam);
    UINT bLeftDown = wParam & MK_LBUTTON;
    if (bLeftDown && Entry->Wnd && Entry->Wnd->IsFocused())
    {
        Entry->Wnd->WindowProc(Msg, wParam, lParam);
        bMouseOnWindow = 0;
        return 1;
    }
    if (bMouseMoveTaken)
        return 1;
    if (Entry->Wnd->IsA(NClass::NCResizeFrame()))
    {
        Entry->Wnd->WindowProc(Msg, wParam, lParam);
        return 1;
    }
    // a style 0x40000 window takes the move even when the cursor is not over it
    if ((HitTest(Entry, lParam) && !bLeftDown) || ((Entry->Wnd->GetStyle() & 0x40000) && !bLeftDown))
    {
        Entry->Wnd->WindowProc(Msg, wParam, lParam);
        if (!Entry->Wnd->IsA(NClass::NCConsole()))
        {
            bMouseMoveTaken = 1;
            bMouseOnWindow = 1;
        }
        else
            bMouseOnWindow = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x100335d0, NCVirtualWndMain, RouteMouseMove)

// 0x10032e60 - to the focused window of the tree. IME messages to a window whose vfn032 says
// so go to the grandchildren below the children that answer vfn033 instead.
INT NCVirtualWndMain::SendToFocused(NWndEntry* Entry, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    INT Result = 0;
    if (Entry && Entry->Wnd && Entry->Wnd->IsFocused())
    {
        UBOOL bIme = (Msg >= WM_IME_SETCONTEXT && Msg <= WM_IME_COMPOSITIONFULL) || Msg == WM_IME_CHAR ||
                     Msg == WM_INPUTLANGCHANGE || (Msg >= WM_IME_STARTCOMPOSITION && Msg <= WM_IME_COMPOSITION);
        if (bIme && Entry->Wnd->vfn032())
        {
            // C3: for WM_IME_NOTIFY (0x282) a window that answers its slot 35 gets lParam -1
            if (Msg == 0x282 && Entry->Wnd && ::nw::VirtualCall<INT>(Entry->Wnd, 35))
                lParam = (LPARAM)-1;
            for (NWndListNode* Node = Entry->Wnd->ChildHead; Node; Node = Node->Next)
            {
                if (!Node->Entry->Wnd->vfn033())
                    continue;
                for (NWndListNode* Sub = Node->Entry->Wnd->ChildHead; Sub; Sub = Sub->Next)
                    if (Sub->Entry->Wnd->WindowProc(Msg, wParam, lParam))
                        Result = 1;
            }
            return Result;
        }
        return Entry->Wnd->WindowProc(Msg, wParam, lParam) ? 1 : 0;
    }
    for (NWndListNode* Node = Entry->Wnd->ChildHead; Node; Node = Node->Next)
    {
        Result = SendToFocused(Node->Entry, Msg, wParam, lParam);
        if (Result)
            break;
    }
    return Result;
}
NW_HOOK_METHOD(0x10032e60, NCVirtualWndMain, SendToFocused)

/*-----------------------------------------------------------------------------
    Painting
-----------------------------------------------------------------------------*/

// 0x10033720 - the children back to front (the last of the list first), each with its own children after it. C3:
// the desktop's children but the first of its list (and the ones with style 0x80000) are painted with the device's
// transparency flag (+0x60) up while GIsTransparencyMode is set
void NCVirtualWndMain::PaintChildren(NWndEntry* Entry)
{
    if (!Entry)
        return;
    NCWnd* Wnd = Entry->Wnd;
    if (GConsole && GConsole->Desktop && Entry == GConsole->Desktop->Entry)
    {
        for (NWndListNode* Node = Wnd->ChildTail; Node; Node = Node->Prev)
        {
            if ((!Wnd->ChildHead || Node->Entry != Wnd->ChildHead->Entry) &&
                !(Node->Entry->Wnd->GetStyle() & NWS_NOFOCUSNEXT) && GIsTransparencyMode)
            {
                nw::Field<INT>(GDevice, 0x60) = 1;
                Node->Entry->Wnd->WindowProc(WM_PAINT, (WPARAM)GDevice, 0);
                PaintChildren(Node->Entry);
                nw::Field<INT>(GDevice, 0x60) = 0;
            }
            else
            {
                Node->Entry->Wnd->WindowProc(WM_PAINT, (WPARAM)GDevice, 0);
                PaintChildren(Node->Entry);
            }
        }
    }
    else
    {
        for (NWndListNode* Node = Wnd->ChildTail; Node; Node = Node->Prev)
        {
            Node->Entry->Wnd->WindowProc(WM_PAINT, (WPARAM)GDevice, 0);
            PaintChildren(Node->Entry);
        }
    }
}
NW_HOOK_METHOD(0x10033720, NCVirtualWndMain, PaintChildren)

// 0x10034210 - the mouse position of the game window: the first viewport keeps it as two floats (+0x3C, +0x40:
// UPlayer's WindowsMouseX / WindowsMouseY, not declared in include/ue2). C1 asked the OS (GetCursorPos).
// NConsoleWnd's 0x10056130 tail-jumps here with its WndMain (NCWnd::Snap).
__declspec(noinline) UBOOL NCVirtualWndMain::GetMousePos(INT* X, INT* Y)
{
    UViewport* Viewport = GConsole->Engine->Client->Viewports(0);
    *X = (INT)nw::Field<FLOAT>(Viewport, 0x3C);
    Viewport = GConsole->Engine->Client->Viewports(0);
    *Y = (INT)nw::Field<FLOAT>(Viewport, 0x40);
    return 1;
}
NW_HOOK_METHOD(0x10034210, NCVirtualWndMain, GetMousePos)

// 0x100345d0 - the first window under the cursor (children first) draws its tool tip. C2: the cursor is the
// viewport's own (GetMousePos), and only a visible window with the 0x20 style (NWS_UNKNOWN20: the tool tip
// style) is sent the draw message
void NCVirtualWndMain::PaintToolTips(NWndEntry* Entry)
{
    if (!Entry)
        return;
    NCWnd* Wnd = Entry->Wnd;
    for (NWndListNode* Node = Wnd->ChildHead; Node; Node = Node->Next)
        PaintToolTips(Node->Entry);
    if (bToolTipShown)
        return;
    INT CursorX, CursorY;
    GetMousePos(&CursorX, &CursorY);
    if (!Entry->bValid)
        return;
    if (!Wnd->PtInRect(CursorX, CursorY, Wnd->Rect))
        return;
    if (!Entry->Wnd->IsShowWindow())
        return;
    if ((Entry->Wnd->GetStyle() & NWS_VISIBLE) && (Entry->Wnd->GetStyle() & NWS_UNKNOWN20))
        Wnd->WindowProc(NWM_DRAWTOOLTIP, (WPARAM)GDevice, 0);
    if (Entry->Wnd->IsA(NClass::NCConsole()))
        return;
    bToolTipShown = 1;
}
NW_HOOK_METHOD(0x100345d0, NCVirtualWndMain, PaintToolTips)

/*-----------------------------------------------------------------------------
    Dispatch
-----------------------------------------------------------------------------*/

// 0x10033820 - (name ours) the message of a window of the focus stack: a click is routed through that window's
// tree, a move goes to its children and itself, the rest to the focused window of the tree; the positions are
// compensated for a windowed game window first. Returns 1 when something took the message. (The C3 code is the
// C2 one, instruction for instruction.)
INT NCVirtualWndMain::DispatchToFocusWindow(NWndEntry* Entry, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        lParam = CoordCompensate(lParam);
        // fall through
    case WM_MOVE:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_MOUSEWHEEL:
    case WM_INPUTLANGCHANGE:
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_IME_CONTROL:
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_CHAR:
        if (!(Entry->Wnd->GetStyle() & NWS_VISIBLE))
            return 0;
        return SendToFocused(Entry, Msg, wParam, lParam) ? 1 : 0;
    case WM_MOUSEMOVE:
    {
        LPARAM Pos = CoordCompensate(lParam);
        if (!(Entry->Wnd->GetStyle() & NWS_VISIBLE))
            return 0;
        if (RouteMouseMove(Entry, Msg, wParam, Pos))
            bMouseMoveTaken = 0;
        return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    {
        LPARAM Pos = CoordCompensate(lParam);
        if (!(Entry->Wnd->GetStyle() & NWS_VISIBLE))
            return 0;
        return RouteClick(Entry, (SWORD)Pos, (SWORD)((DWORD)Pos >> 16), Msg, wParam, Pos) ? 1 : 0;
    }
    case WM_RBUTTONDOWN:
    {
        LPARAM Pos = CoordCompensate(lParam);
        if ((Entry->Wnd->GetStyle() & NWS_VISIBLE) &&
            RouteClick(Entry, (SWORD)Pos, (SWORD)((DWORD)Pos >> 16), Msg, wParam, Pos))
            return 1;
        if (!GL2UseColorCursor)
            return 0;
        UnlockCursor();
        ::SetCursor(Cursors[0]);
        return 0;
    }
    case WM_SETCURSOR:
        if (!GL2UseColorCursor)
            return 0;
        if ((SWORD)lParam != HTCLIENT)
            return 0;
        return 1;
    default:
        return 0;
    }
}
NW_HOOK_METHOD(0x10033820, NCVirtualWndMain, DispatchToFocusWindow)

// 0x100346f0 - entry point of every message for the UI. While a window sits on the focus stack every message but
// WM_PAINT goes to it alone; else the top-level windows are tried from the front (last index), except for painting,
// which goes from the back. Positions (the key messages' lParam too, as the stock code does) go through
// CoordCompensate, except the WM_MOUSEMOVE and WM_LBUTTONDBLCLK ones, which a fullscreen game window turns into
// screen positions. C3: the double click's flag (GLButtonDblClk) and WM_LBUTTONUP, which releases the cursor lock
// unless it follows a double click; a key or button up with wParam 0x11 (Ctrl) goes nowhere (WM_KEYDOWN, WM_CHAR,
// WM_RBUTTONUP, WM_LBUTTONUP: the check is on the stock's shared label for those four)
INT NCVirtualWndMain::DispatchWndMsg(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    guard(NCVirtualWndMain::DispatchWndMsg);
    if (!IsDispatchedMsg(Msg))
        return 0;
    if (Msg != WM_PAINT && FocusWindows.Num() > 0)
    {
        DispatchToFocusWindow(FocusWindows.Last(0), Msg, wParam, lParam);
        return 1;
    }
    switch (Msg)
    {
    case WM_PAINT:
    {
        guard(WM_PAINT);
        for (INT i = 0; i < TopLevel.Num(); i++)
        {
            if (TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE)
            {
                TopLevel(i)->Wnd->WindowProc(Msg, (WPARAM)GDevice, lParam);
                PaintChildren(TopLevel(i));
                PaintToolTips(TopLevel(i));
                bToolTipShown = 0;
            }
        }
        if (NW_DEBUG_MARKER())  // proof mode, not stock (re/NwDebug.h)
            ::nw::DrawDebugMarker(GDevice);
        unguard;
        return 0;
    }
    case WM_SETCURSOR:
        if (GL2UseColorCursor && (_WORD)lParam == HTCLIENT)
        {
            SetCursorType(CursorType, NULL);   // the cursor of the moment again (a color cursor is not the OS's)
            return 1;
        }
        return 0;
    case WM_MOUSEMOVE:
        if (ViewportIsFullscreen())
            lParam = ClientToScreenInline(lParam);
        for (INT i = TopLevel.Num() - 1; i >= 0; i--)
            if ((TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE) && RouteMouseMove(TopLevel(i), Msg, wParam, lParam))
                bMouseMoveTaken = 0;
        return 0;
    case WM_LBUTTONDBLCLK:
    {
        GLButtonDblClk = 1;
        if (ViewportIsFullscreen())
            lParam = ClientToScreenInline(lParam);
        INT Y = (SWORD)((DWORD)lParam >> 16);
        for (INT i = TopLevel.Num() - 1; i >= 0; i--)
            if ((TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE) && RouteClick(TopLevel(i), (SWORD)lParam, Y, Msg, wParam, lParam))
                return 1;
        break;
    }
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    {
        if (GLButtonDblClk)
            return 1;
        LPARAM Pos = CoordCompensate(lParam);
        INT Y = (SWORD)((DWORD)Pos >> 16);
        for (INT i = TopLevel.Num() - 1; i >= 0; i--)
            if ((TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE) && RouteClick(TopLevel(i), (SWORD)Pos, Y, Msg, wParam, Pos))
                return 1;
        break;
    }
    case WM_RBUTTONDOWN:
    {
        LPARAM Pos = CoordCompensate(lParam);
        INT Y = (SWORD)((DWORD)Pos >> 16);
        for (INT i = TopLevel.Num() - 1; i >= 0; i--)
            if ((TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE) && RouteClick(TopLevel(i), (SWORD)Pos, Y, Msg, wParam, Pos))
                return 1;
        if (GL2UseColorCursor)
        {
            UnlockCursor();
            ::SetCursor(Cursors[0]);
        }
        break;
    }
    case WM_LBUTTONUP:
        if (!GLButtonDblClk)
            UnlockCursor();
        else
            GLButtonDblClk = 0;
        // fall through
    case WM_RBUTTONUP:
    case WM_KEYDOWN:
    case WM_CHAR:
        if (wParam == 0x11)
            return 0;
        // fall through
    case WM_MOVE:
    case WM_KEYUP:
    case WM_MOUSEWHEEL:
        lParam = CoordCompensate(lParam);
        // fall through
    case WM_INPUTLANGCHANGE:
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_IME_CONTROL:
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_CHAR:
        for (INT i = TopLevel.Num() - 1; i >= 0; i--)
            if ((TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE) && SendToFocused(TopLevel(i), Msg, wParam, lParam))
                return 1;
        return 0;
    default:
        return 0;
    }
    // a click nobody took: whoever has the focus loses it
    if (GFocusEntry)
    {
        GFocusEntry->Wnd->KillFocus();
        GFocusEntry = NULL;
    }
    return 0;
    unguard;
}
NW_HOOK_METHOD(0x100346f0, NCVirtualWndMain, DispatchWndMsg)

/*-----------------------------------------------------------------------------
    Cursors
-----------------------------------------------------------------------------*/

// 0x10032ff0 - resources of this package (the stock module in hook mode)
void NCVirtualWndMain::LoadCursors()
{
    for (INT i = 0; i < 7; i++)
    {
        HCURSOR Cursor;
        if (!GUnicodeOS)
            Cursor = LoadCursorA(GNWindowInstance, MAKEINTRESOURCEA(GCursorIDs[i]));
        else
            Cursor = LoadCursorW(GNWindowInstance, MAKEINTRESOURCEW(GCursorIDs[i]));
        Cursors[i] = Cursor;
        if (i == 0)
            ::SetCursor(Cursor);
    }
    if (GUnicodeOS)
        ArrowCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(0x7f00));  // IDC_ARROW
    else
        ArrowCursor = LoadCursorA(NULL, MAKEINTRESOURCEA(0x7f00));
}
NW_HOOK_METHOD(0x10032ff0, NCVirtualWndMain, LoadCursors)

// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)
// 0x10033070
void NCVirtualWndMain::DestroyCursors()
{
    for (INT i = 0; i < 7; i++)
        DestroyCursor(Cursors[i]);
    if (bColorCursor)
    {
        DestroyCursor(ColorCursor);
        ColorCursor = NULL;
        bColorCursor = 0;
    }
}
NW_HOOK_METHOD(0x10033070, NCVirtualWndMain, DestroyCursors)

// 0x10034190 - type 4 with a texture: a color cursor built from it. C2 only skips the color cursor that is
// already shown (type 4 again): any other type is set again, which is what DispatchWndMsg's WM_SETCURSOR
// relies on (it passes the current type to put the cursor back after the OS changed it)
void NCVirtualWndMain::SetCursorType(INT Type, UTexture* Texture)
{
    if (!bCursorEnabled)
        return;
    if (!GL2UseColorCursor)
    {
        ::SetCursor(ArrowCursor);
        CursorType = -1;
        return;
    }
    if (Type == CursorType && Type == 4)
        return;
    if (Type >= 7)
        return;
    if (Type == 4 && Texture)
    {
        CreateColorCursor(Texture);
        CursorType = Type;
        return;
    }
    ::SetCursor(Cursors[Type]);
    CursorType = Type;
}
NW_HOOK_METHOD(0x10034190, NCVirtualWndMain, SetCursorType)

// 0x10033e00 - a DXT1 / DXT3 texture as the cursor: at 32 bits of color its pixels with an empty
// mask, else RGB565 pixels with a mask of the transparent ones. The mask is never freed, the
// 16-bit pixels only at 16 bits of color (stock leaks).
void NCVirtualWndMain::CreateColorCursor(UTexture* Texture)
{
    if (Texture->Format != TEXF_DXT1 && Texture->Format != TEXF_DXT3)
        return;
    if (bColorCursor)
    {
        DestroyCursor(ColorCursor);
        ColorCursor = NULL;
        bColorCursor = 0;
    }
    INT ColorBits = GColorBits;
    BYTE* Pixels = NULL;
    BYTE* Pixels16 = NULL;
    Texture->ConvertDXT(0, 0, 0, &Pixels);
    INT W = Texture->USize;
    INT H = Texture->VSize;
    // C3: a windowed game takes the desktop's colour depth (the viewport's slot 30 is its fullscreen test)
    if (!ViewportIsFullscreen())
        ColorBits = GetDeviceCaps(GetDC(GetDesktopWindow()), BITSPIXEL);
    if (ColorBits == 32)
    {
        INT Size = H * W * 4;
        BYTE* Mask = (BYTE*)appMalloc(Size, TEXT("new"));
        appMemset(Mask, 0, Size);
        ColorCursor = CreateIcon(NULL, W, H, 1, 32, Mask, Pixels);
    }
    else
    {
        BYTE* Mask = (BYTE*)appMalloc(W / 8 * H, TEXT("new"));
        Pixels16 = (BYTE*)appMalloc(H * W * 2, TEXT("new"));
        BYTE* Pixel = (BYTE*)appMalloc(4, TEXT("new"));
        BYTE Bits = 0;
        for (INT Y = 0; Y < H; Y++)
        {
            INT Bit = 1;
            for (INT X = 0; X < W; X++)
            {
                if (Y < H && X < W)
                    appMemcpy(Pixel, Pixels + (Y * W + X) * 4, 4);
                else
                    appMemset(Pixel, 0, 4);
                DWORD Color;
                if (Pixel[3] == 0)
                    Color = 0;
                else
                    Color = (Pixel[0] >> 3) + (((Pixel[2] & 0xF8) << 5) + (Pixel[1] & 0xFC)) * 8;
                appMemcpy(Pixels16 + (Y * W + X) * 2, &Color, 2);
                // a transparent pixel sets its bit of the mask, the high bit first
                if (Pixel[3] == 0)
                    Bits = (BYTE)(INT)(pow(2.0, (DOUBLE)(8 - Bit)) + Bits);
                Bit++;
                if (Bit == 9)
                {
                    appMemcpy(Mask + Y * W / 8 + X / 8, &Bits, 1);
                    Bit = 1;
                    Bits = 0;
                }
            }
        }
        appFree(Pixel);
        ColorCursor = CreateIcon(NULL, W, H, 1, 16, Mask, Pixels16);
    }
    bColorCursor = 1;
    SetClassLongW(GViewportHWnd, GCL_HCURSOR, (LONG)ColorCursor);
    ::SetCursor(ColorCursor);
    appFree(Pixels);
    if (ColorBits == 16)
        appFree(Pixels16);
}
NW_HOOK_METHOD(0x10033e00, NCVirtualWndMain, CreateColorCursor)

// 0x10024080, the package's DllMain (GNWindowInstance = hInstance; return 1), stays stock: the
// CRT entry that calls it keeps ecx across the call as far as tools/regcheck.py can tell (a
// register call follows), and ours would not preserve it

// 0x10034ef0 - the four window lists go (the stock calls the out-of-line array destructor 0x10002460 for each), then NCAbstractMain
NW_HOOK_DTOR(0x10034ef0, NCVirtualWndMain)

// 0x10002460, 0x10002590 (a `jmp` to the first one) - the destructor instance of the arrays of window pointers
// (NCVirtualWndMain's four lists, NCWnd's stuck lists): the stock compiler emitted it out of line
typedef TArray<NWndEntry*> TWndEntryArray;
NW_HOOK_DTOR(0x10002460, TWndEntryArray)
NW_HOOK_DTOR(0x10002590, TWndEntryArray)

NW_CHECK_VTABLE(NCAbstractMain, 0x1017e3a0, 33)
NW_CHECK_VTABLE(NCVirtualWndMain, 0x1017e618, 34)
