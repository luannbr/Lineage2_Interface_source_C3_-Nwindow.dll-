/*=============================================================================
    NCIcon.cpp - icon of a minimized window. Stock (C2): 0x10018e90 .. 0x100193e8.
=============================================================================*/
#include "nwindow/NCIcon.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

enum { SOUND_Click = 0 };

// 0x1001c460 - C3: NControl's constructor is inlined in the stock one (NCWnd's, then the four stores); the rest as C2
NCIcon::NCIcon()
{
    DragY = 0;
    DragX = 0;
    IconTex = NULL;
    bMouseOver = 0;
    Unknown1C0 = -1;
    bToolTipAbove = 0;
    ToolTipTex = FindTexture(TEXT("NWindow.ChatBack"));
    bDragged = 0;
}
NW_HOOK_CTOR(0x1001c460, NCIcon)

// 0x1001c540 - ConditionalDestroy, ~FString (Name), ~NControl
NW_HOOK_DTOR(0x1001c540, NCIcon)

// 0x1001bd70 (vslot 25) - the double click goes to slot 68
INT NCIcon::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1001bd70, NCIcon, WindowProc)


// 0x1001bfe0 (vslot 56) - C3: while it is being dragged the icon follows the cursor (a WM_MOVE of the distance to
// the grab point, sent to itself) before it paints
INT NCIcon::OnPaint(NCGDevice* Device)
{
    guard(NCIcon::OnPaint);
    if (bDragged)
    {
        INT MouseX, MouseY;
        GConsole->GetMousePos(&MouseX, &MouseY);
        INT DX = MouseX - DragX - ScreenX;
        INT DY = MouseY - DragY - ScreenY;
        SendTo(Entry, WM_MOVE, 0, MAKELPARAM((WORD)DX, (WORD)DY));
    }
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (IconTex)
        Device->DrawTexture(0, 0, 32, 32, 0, 0, 32, 32, IconTex, 0xFF, 1);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1001bfe0, NCIcon, OnPaint)

// 0x1001c120 (vslot 57) - the name in a box of its own size above or below the icon (pushed left to stay on the
// screen), light grey text on ToolTipTex. C3: the screen is the desktop's local rectangle, the sizes are FLOATs
INT NCIcon::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (Name.Num() && bMouseOver)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, *Name, 0, 0, 0, 0, 0, L2FONT_Normal);
        NRectF Screen;
        Screen.X = 0;
        Screen.Y = 0;
        Screen.W = 0.0f;
        Screen.H = 0.0f;
        GConsole->Desktop->GetLocalRect(&Screen);
        INT X;
        INT Y;
        INT Right = ScreenX + Size.cx;
        if (bToolTipAbove)
        {
            if (Right > (INT)Screen.W)
            {
                Device->SetClip((INT)(Screen.W - (FLOAT)Size.cx), ScreenY - Size.cy, Size.cx, Size.cy);
                X = (INT)(Screen.W - (FLOAT)Size.cx - (FLOAT)ScreenX);
            }
            else
            {
                Device->SetClip(ScreenX, ScreenY - Size.cy, Size.cx, Size.cy);
                X = 0;
            }
            Y = -Size.cy;
        }
        else
        {
            if (Right > (INT)Screen.W)
            {
                Device->SetClip((INT)(Screen.W - (FLOAT)Size.cx), (INT)((FLOAT)ScreenY + Height), Size.cx, Size.cy);
                X = (INT)(Screen.W - (FLOAT)Size.cx - (FLOAT)ScreenX);
            }
            else
            {
                Device->SetClip(ScreenX, (INT)((FLOAT)ScreenY + Height), Size.cx, Size.cy);
                X = 0;
            }
            Y = (INT)Height;
        }
        if (ToolTipTex)
            Device->DrawStretchedTex(X, Y, Size.cx, Size.cy, ToolTipTex, 1, 0xFF);
        Device->DrawText(X, Y, 0xFFDCDCDC, *Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x1001c120, NCIcon, OnDrawToolTip)

// 0x1001be80 (vslot 60) - dragged: WM_MOVE(delta) to itself (NCWnd::OnMove snaps it);
// DragMouse holds the delta during the call
INT NCIcon::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = (SWORD)LOWORD(lParam);
    INT Y = (SWORD)HIWORD(lParam);
    if (wParam & MK_LBUTTON)
    {
        DragMouseX = X - DragMouseX;
        DragMouseY = Y - DragMouseY;
        SendTo(Entry, WM_MOVE, 0, MAKELPARAM(DragMouseX, DragMouseY));
        DragMouseX = X;
        DragMouseY = Y;
        bDragged = 1;
        return 1;
    }
    bMouseOver = PtInRect(X, Y, Rect) != 0;
    return 1;
}
NW_HOOK_METHOD(0x1001be80, NCIcon, OnMouseMove)

// 0x1001bf30 (vslot 62)
INT NCIcon::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    LockCursor();
    DragMouseX = (SWORD)LOWORD(lParam);
    DragX = DragMouseX - ScreenX;
    DragMouseY = (SWORD)HIWORD(lParam);
    DragY = DragMouseY - ScreenY;
    return 1;
}
NW_HOOK_METHOD(0x1001bf30, NCIcon, OnLButtonDown)

// 0x1001bf70 (vslot 63) - a click without a drag restores the window (C3: no click sound here)
INT NCIcon::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (bDragged)
    {
        UnlockCursor();
        ReleaseFocus();
        bDragged = 0;
        return 1;
    }
    Entry->Parent->Wnd->Restore();
    return 1;
}
NW_HOOK_METHOD(0x1001bf70, NCIcon, OnLButtonUp)

// 0x1001be20 - the tool tip text is a copy of the SysString (C1 copied it into a TCHAR[128])
void NCIcon::SetIcon(const TCHAR* TexName, INT NameID, BYTE bAbove)
{
    if (TexName)
        IconTex = FindTexture(TexName);
    if (NameID >= 0)
        Name = *SysString(NameID);
    bToolTipAbove = bAbove;
}
NW_HOOK_METHOD(0x1001be20, NCIcon, SetIcon)

// 0x1001bfc0 (vslot 45) - C3: hidden, the tool tip goes back below
void NCIcon::HideWindow()
{
    NCWnd::HideWindow();
    bToolTipAbove = 0;
}
NW_HOOK_METHOD(0x1001bfc0, NCIcon, HideWindow)

NW_CHECK_VTABLE(NCIcon, 0x1017c688, 70)
