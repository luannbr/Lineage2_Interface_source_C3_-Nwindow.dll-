/*=============================================================================
    NCFrameCtrl.cpp - title bar and resize edge. Stock (C3): 0x1000f6d0 .. 0x100106a0.
=============================================================================*/
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NClasses.gen.h"

#include <wchar.h>

// interface sound played when a window is closed from its title bar
enum { SOUND_WindowClose = 6 };

/*-----------------------------------------------------------------------------
    NCFrameCtrl.
-----------------------------------------------------------------------------*/

// 0x100102f0 - C3: the title bar's textures are the CH3 set (NControl's constructor is inlined in the stock one)
NCFrameCtrl::NCFrameCtrl()
{
    CloseButton = NULL;
    MinimizeButton = NULL;
    LastMouseY = 0;
    LastMouseX = 0;
    TitleY = 0;
    TitleX = 0;
    GrabY = 0;
    GrabX = 0;
    bMoved = 0;
    BackLeftTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.FrameBackLeft"));
    BackMidTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.FrameBackMid"));
    BackRightTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.FrameBackRight"));
    SmallBarTex[0] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar_h1"));
    SmallBarTex[1] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar_h2"));
    SmallBarTex[2] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar_h3"));
    SmallBarTex[3] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar1"));
    SmallBarTex[4] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar2"));
    SmallBarTex[5] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar3"));
    bDecorated = 1;
}
NW_HOOK_CTOR(0x100102f0, NCFrameCtrl)

// 0x10010450 - the same with the three flags as arguments (C3 only; bVertical / bSmall are not written by the
// default one)
NCFrameCtrl::NCFrameCtrl(INT InDecorated, INT InVertical, INT InSmall)
{
    CloseButton = NULL;
    MinimizeButton = NULL;
    LastMouseY = 0;
    LastMouseX = 0;
    TitleY = 0;
    TitleX = 0;
    bMoved = 0;
    GrabY = 0;
    GrabX = 0;
    BackLeftTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.FrameBackLeft"));
    BackMidTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.FrameBackMid"));
    BackRightTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.FrameBackRight"));
    SmallBarTex[0] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar_h1"));
    SmallBarTex[1] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar_h2"));
    SmallBarTex[2] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar_h3"));
    SmallBarTex[3] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar1"));
    SmallBarTex[4] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar2"));
    SmallBarTex[5] = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar3"));
    bVertical = InVertical;
    bDecorated = InDecorated;
    bSmall = InSmall;
}
NW_HOOK_CTOR_ARGS(0x10010450, NCFrameCtrl, (INT A, INT B, INT C), (A, B, C))

// 0x10010250 - ConditionalDestroy, ~FString (Title), ~NControl
NW_HOOK_DTOR(0x10010250, NCFrameCtrl)

// 0x1000f6d0 (vslot 25) - the frame buttons report here. C2: no enabled test
INT NCFrameCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_FRAME_MINIMIZE)
            return OnMinimize() != 0;
        if (wParam == NID_FRAME_CLOSE)
            return OnClose() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1000f6d0, NCFrameCtrl, WindowProc)

// 0x100100b0 (vslot 37) - the buttons of a decorated frame, right-aligned (C3: 15x15 at y 3, the CH3 button textures)
INT NCFrameCtrl::OnCreate()
{
    NRectF R;
    R.X = 0;
    R.Y = 0;
    R.W = 0.0f;
    R.H = 0.0f;
    GetLocalRect(&R);
    if (bDecorated && Entry && Entry->Parent && Entry->Parent->Wnd)
    {
        if (Entry->Parent->Wnd->GetStyle() & NWS_MINIMIZABLE)
        {
            MinimizeButton = new NCButton(TEXT("L2UI_CH3.FrameCtrl.FrameMiniBtn"),
                                          TEXT("L2UI_CH3.FrameCtrl.FrameMiniOnBtn"), 0);
            MinimizeButton->ID = NID_FRAME_MINIMIZE;
            MinimizeButton->SetPosition((INT)(Width - 40.0f), 3, 15, 15);
            MinimizeButton->AttachEntry(AddChildWindow(MinimizeButton, NWS_VISIBLE));
        }
        if (!(Entry->Parent->Wnd->GetStyle() & NWS_NOCLOSE))
        {
            CloseButton = new NCButton(TEXT("L2UI_CH3.FrameCtrl.FrameCloseBtn"),
                                       TEXT("L2UI_CH3.FrameCtrl.FrameCloseOnBtn"), 0);
            CloseButton->ID = NID_FRAME_CLOSE;
            CloseButton->SetPosition((INT)(Width - 23.0f), 3, 15, 15);
            CloseButton->AttachEntry(AddChildWindow(CloseButton, NWS_VISIBLE));
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100100b0, NCFrameCtrl, OnCreate)

// 0x1000faa0 (vslot 38) - C2 guards it
INT NCFrameCtrl::OnDestroy()
{
    guard(NCFrameCtrl::OnDestroy);
    if (MinimizeButton)
    {
        RemoveChildWindow(MinimizeButton->Entry);
        delete MinimizeButton;
        MinimizeButton = NULL;
    }
    if (CloseButton)
    {
        RemoveChildWindow(CloseButton->Entry);
        delete CloseButton;
        CloseButton = NULL;
    }
    NControl::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1000faa0, NCFrameCtrl, OnDestroy)

// 0x1000fb70 (vslot 56) - C3: the bar is 0x14 high with 16 px caps (or the small bars: 8x12 horizontal, 12x8
// vertical, by the two flags of the constructor); the title is measured once, then painted in grey-blue
INT NCFrameCtrl::OnPaint(NCGDevice* Device)
{
    guard(NCFrameCtrl::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (bDecorated)
    {
        if (!bVertical)
        {
            if (!bSmall)
            {
                if (BackLeftTex)
                    Device->DrawTexture(0, 0, 16, 20, 0, 0, 16, 20, BackLeftTex, 0xFF, 1);
                if (BackRightTex)
                    Device->DrawTexture((INT)(Width - 16.0f), 0, 16, 20, 0, 0, 16, 20, BackRightTex, 0xFF, 1);
                if (BackMidTex)
                    Device->DrawStretchedTex(16, 0, (INT)(Width - 16.0f - 16.0f), 20, 0, 0, 32, 20, BackMidTex, 0xFF);
                if (TitleX == 0 && TitleY == 0)
                {
                    SIZE Size;
                    NCGDevice::GetTextExtent(&Size, *Title, 0, 0, 0, 0, 0, L2FONT_Normal);
                    TitleX = 20;
                    TitleY = (INT)(Height * 0.5f - (Size.cy / 2) + 2.0f);
                }
                if (Title.Num())
                    Device->DrawText(TitleX, TitleY, 0xFFC8D2DC, *Title, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
            }
            else if (bSmall == 1)
            {
                // the stock code tests the first texture and paints the third (and the other way round on the right)
                if (SmallBarTex[0])
                    Device->DrawTexture(0, 0, 8, 12, 0, 0, 8, 12, SmallBarTex[2], 0xFF, 1);
                if (SmallBarTex[1])
                    Device->DrawTexture(8, 0, (INT)(Width - 16.0f), 12, 0, 0, 8, 12, SmallBarTex[1], 0xFF, 1);
                if (SmallBarTex[2])
                    Device->DrawTexture((INT)(Width - 8.0f), 0, 8, 12, 0, 0, 8, 12, SmallBarTex[0], 0xFF, 1);
            }
        }
        else if (bVertical == 1 && bSmall == 1)
        {
            if (SmallBarTex[3])
                Device->DrawTexture(0, 0, 12, 8, 0, 0, 12, 8, SmallBarTex[3], 0xFF, 1);
            if (SmallBarTex[4])
                Device->DrawTexture(0, 8, 12, (INT)(Height - 16.0f), 0, 0, 12, 8, SmallBarTex[4], 0xFF, 1);
            if (SmallBarTex[5])
                Device->DrawTexture(0, (INT)(Height - 8.0f), 12, 8, 0, 0, 12, 8, SmallBarTex[5], 0xFF, 1);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1000fb70, NCFrameCtrl, OnPaint)

// 0x1000fed0 (vslot 61) - the move is sent as a delta; LastMouse holds the delta during the call; C3 also tells the
// console which frame is being dragged (NConsoleWnd +0x39E4, in the tail its header leaves unnamed)
INT NCFrameCtrl::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if (wParam & MK_LBUTTON)
    {
        INT X = (SWORD)LOWORD(lParam);
        INT Y = (SWORD)HIWORD(lParam);
        LastMouseX = X - LastMouseX;
        LastMouseY = Y - LastMouseY;
        if (LastMouseX == 0 && LastMouseY == 0)
            bMoved = 0;
        else
            bMoved = 1;
        SendToParent(WM_MOVE, 0, MAKELPARAM(LastMouseX, LastMouseY));
        LastMouseX = X;
        LastMouseY = Y;
        ::nw::Field<NCFrameCtrl*>(GConsole, 0x39E4) = this;
    }
    return 1;
}
NW_HOOK_METHOD(0x1000fed0, NCFrameCtrl, OnMouseMove)

// 0x1000f840 (vslot 62) - the parent gets the click too (it comes to the front, takes focus)
INT NCFrameCtrl::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    LockCursor();
    LastMouseX = (SWORD)LOWORD(lParam);
    GrabX = LastMouseX - ScreenX;
    LastMouseY = (SWORD)HIWORD(lParam);
    GrabY = LastMouseY - ScreenY;
    SendToParent(WM_LBUTTONDOWN, wParam, lParam);
    bMoved = 0;
    return 1;
}
NW_HOOK_METHOD(0x1000f840, NCFrameCtrl, OnLButtonDown)

// 0x1000f8a0 (vslot 63)
INT NCFrameCtrl::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    UnlockCursor();
    ReleaseFocus();
    bMoved = 0;
    return 1;
}
NW_HOOK_METHOD(0x1000f8a0, NCFrameCtrl, OnLButtonUp)

// 0x1000fa10 (vslot 70) - C3: the stock code still walks the class chain of the parent for four classes (the main,
// inventory, map cover and system menu windows) and does nothing different for them; then the sound, and the
// parent is hidden
INT NCFrameCtrl::OnClose()
{
    NCWnd* Parent = Entry->Parent->Wnd;
    if (!Parent->IsA(NClass::NCMainWnd()))
    {
        if (!Parent->IsA(NClass::NCInventoryWnd()))
        {
            if (!Parent->IsA(NClass::NCMapCoverWnd()))
                Parent->IsA(NClass::NCSystemMenuWnd());
        }
    }
    PlayInterfaceSound(SOUND_WindowClose);
    HideParent();
    return 1;
}
NW_HOOK_METHOD(0x1000fa10, NCFrameCtrl, OnClose)

// 0x1000f750 (vslot 71)
INT NCFrameCtrl::OnMinimize()
{
    Entry->Parent->Wnd->Minimize();
    return 1;
}
NW_HOOK_METHOD(0x1000f750, NCFrameCtrl, OnMinimize)

// 0x1000f8d0 - called by NCWnd::SetTitle; OnPaint measures the title again (TitleX / TitleY back to 0)
void NCFrameCtrl::SetTitleText(const TCHAR* InTitle)
{
    if (InTitle)
    {
        Title = InTitle;
        TitleX = 0;
        TitleY = 0;
    }
}
NW_HOOK_METHOD(0x1000f8d0, NCFrameCtrl, SetTitleText)

// 0x1000f820 (ret 8; anonymous in the db, name ours) - the two shape flags of the constructor set again
void NCFrameCtrl::SetBarShape(INT InVertical, INT InSmall)
{
    bVertical = InVertical;
    bSmall = InSmall;
}
NW_HOOK_METHOD(0x1000f820, NCFrameCtrl, SetBarShape)

// 0x1000f770 (vslot 69, name ours) - the bar's size, from the window that owns it (NCWnd::SetWindowSize)
void NCFrameCtrl::SetFrameSize(DWORD W, DWORD H)
{
    NCWnd::SetWindowSize((FLOAT)W, (FLOAT)H);
}
NW_HOOK_METHOD(0x1000f770, NCFrameCtrl, SetFrameSize)

NW_CHECK_VTABLE(NCFrameCtrl, 0x10179058, 72)

/*-----------------------------------------------------------------------------
    NCResizeFrame.
-----------------------------------------------------------------------------*/

// 0x100105c0 - undecorated; InternalConstructor passes 0
NCResizeFrame::NCResizeFrame(INT InDirection)
{
    ResizeGrabY = 0;
    ResizeGrabX = 0;
    bDecorated = 0;
    Unknown248 = 0;
    bCursorLocked = 0;
    bDragging = 0;
    Direction = InDirection;
}
NW_HOOK_CTOR_ARGS(0x100105c0, NCResizeFrame, (INT A), (A))

// 0x10010640
NW_HOOK_DTOR(0x10010640, NCResizeFrame)

// 0x1000f900 (vslot 25) - C2: NCFrameCtrl's alone (no enabled test)
INT NCResizeFrame::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCFrameCtrl::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1000f900, NCResizeFrame, WindowProc)

// 0x1000f9f0 (vslot 30)
void NCResizeFrame::KillFocus()
{
    NCWnd::KillFocus();
    bDragging = 0;
    bPressed = 0;
}
NW_HOOK_METHOD(0x1000f9f0, NCResizeFrame, KillFocus)

// 0x1000ff70 (vslot 61) - resize cursor while over the edge; while dragging, the delta along the resize direction
// goes to the parent as NWM_COMMAND(ID, delta). C3: the delta is measured from where the edge was grabbed
INT NCResizeFrame::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = (SWORD)LOWORD(lParam);
    INT Y = (SWORD)HIWORD(lParam);
    if (PtInRect(X, Y, Rect))
    {
        if (Direction == RESIZE_Vertical)
            SetCursor(5, NULL);
        else if (Direction == RESIZE_Horizontal)
            SetCursor(6, NULL);
        LockCursor();
        bCursorLocked = 1;
        if (!(wParam & MK_LBUTTON) || !bDragging)
            return 1;
    }
    else if (!bDragging || !(wParam & MK_LBUTTON))
    {
        if (bCursorLocked)
        {
            bCursorLocked = 0;
            UnlockCursor();
        }
        return 1;
    }
    LastMouseX = X - ResizeGrabX - ScreenX;
    LastMouseY = Y - ResizeGrabY - ScreenY;
    if (Direction == RESIZE_Vertical)
        NotifyParent(LastMouseY);
    else if (Direction == RESIZE_Horizontal)
        NotifyParent(LastMouseX);
    LastMouseX = X;
    LastMouseY = Y;
    return 1;
}
NW_HOOK_METHOD(0x1000ff70, NCResizeFrame, OnMouseMove)

// 0x1000f920 (vslot 62)
INT NCResizeFrame::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT X = (SWORD)LOWORD(lParam);
    INT Y = (SWORD)HIWORD(lParam);
    LastMouseX = X;
    LastMouseY = Y;
    if (PtInRect(X, Y, Rect))
    {
        ResizeGrabX = LastMouseX - ScreenX;
        ResizeGrabY = LastMouseY - ScreenY;
        bDragging = 1;
        bPressed = 1;
    }
    return 1;
}
NW_HOOK_METHOD(0x1000f920, NCResizeFrame, OnLButtonDown)

// 0x1000f9c0 (vslot 63)
INT NCResizeFrame::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    bPressed = 0;
    bDragging = 0;
    ResizeGrabX = 0;
    ResizeGrabY = 0;
    return 1;
}
NW_HOOK_METHOD(0x1000f9c0, NCResizeFrame, OnLButtonUp)

NW_CHECK_VTABLE(NCResizeFrame, 0x101793c0, 72)
