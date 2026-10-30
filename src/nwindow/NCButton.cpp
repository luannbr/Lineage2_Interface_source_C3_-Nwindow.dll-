/*=============================================================================
    NCButton.cpp - NCButton, NCExButton, NCPushButton, NCRadioButton.
    Stock: 0x10001000 .. 0x10002550, plus WindowProc 0x100051f0 / 0x100301f0 and
    OnDestroy 0x100189e0 (bodies the linker folded with other classes).
=============================================================================*/
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <wchar.h>

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_Click = 0, SOUND_ClickDisabled = 2 };

// timer of the push button auto-repeat
enum { TIMER_PushRepeat = 0xFF000000 };

/*-----------------------------------------------------------------------------
    NCButton.
-----------------------------------------------------------------------------*/

// 0x10002940 - no textures, label from Text, the tool tip below the button (NControl's constructor is inlined in the
// stock one: the same stores). C3: the click sounds are on (bClickSound)
NCButton::NCButton()
{
    bReleased = 1;
    NormalTex = NULL;
    PushedTex = NULL;
    bAcceptKey = 0;
    NormalTexH = 0;
    NormalTexW = 0;
    PushedTexH = 0;
    PushedTexW = 0;
    bMouseOver = 0;
    bToolTipAbove = 1;
    TextID = -1;
    ToolTipID = -1;
    TextOffsetX = 0;
    TextOffsetY = 0;
    bClickSound = 1;
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
}
NW_HOOK_CTOR(0x10002940, NCButton)

// 0x10002a40 - the two textures by name (no test: the stock code reads their sizes at once)
NCButton::NCButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, UBOOL bInAcceptKey)
{
    bReleased = 1;
    NormalTex = FindTexture(NormalTexName);
    NormalTexW = NormalTex->USize;
    NormalTexH = NormalTex->VSize;
    PushedTex = FindTexture(PushedTexName);
    PushedTexW = PushedTex->USize;
    PushedTexH = PushedTex->VSize;
    bAcceptKey = bInAcceptKey;
    bMouseOver = 0;
    bToolTipAbove = 1;
    TextID = -1;
    ToolTipID = -1;
    TextOffsetX = 0;
    TextOffsetY = 0;
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    bClickSound = 1;
}
NW_HOOK_CTOR_ARGS(0x10002a40, NCButton, (const TCHAR* A, const TCHAR* B, UBOOL C), (A, B, C))

// 0x10002890 - the label (+0x27C) and the tool tip data (+0x200) go, then NControl's destructor (0x100027d0)
NW_HOOK_DTOR(0x10002890, NCButton)

// 0x100127a0 (vslot 25) - one body for the buttons (C2 dropped the enabled test)
INT NCButton::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x100127a0, NCButton, WindowProc)

// 0x10001180 (vslot 38) - C2 guards it and empties the label
INT NCButton::OnDestroy()
{
    guard(NCButton::OnDestroy);
    NControl::OnDestroy();
    Text.Empty();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10001180, NCButton, OnDestroy)

// 0x10001b50 (vslot 56) - the texture over the button (C3: from the clip offset, so a button clipped by its parent is
// cut, not squeezed), the label centered (grey when disabled). Width and Height are FLOATs; the label is placed
// with the halves in floating point. The two texture branches are the stock code's own copies: only the pushed one
// looks at the first character of Text
INT NCButton::OnPaint(NCGDevice* Device)
{
    guard(NCButton::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (bReleased)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, ClipOffsetX, ClipOffsetY, (INT)Width, (INT)Height,
                            NormalTex, 0xFF, 1);
    else
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, ClipOffsetX, ClipOffsetY, (INT)Width, (INT)Height,
                            PushedTex, 0xFF, 1);

    const TCHAR* Label = NULL;
    if (TextID >= 0)
    {
        if (*SysString(TextID))
            Label = *SysString(TextID);
    }
    else if (Text.Num() && (bReleased || *Text.GetCharData()))
        Label = *Text;
    if (Label)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Label, 0, 0, 0, 0, 0, L2FONT_Normal);
        DWORD Color = IsEnabled() ? 0xFFE6DCBE : 0xFFA0A0A0;
        INT Y = (INT)((DOUBLE)TextOffsetY + (DOUBLE)Height * 0.5 - (DOUBLE)(Size.cy / 2));
        INT X = (INT)((DOUBLE)TextOffsetX + (DOUBLE)Width * 0.5 - (DOUBLE)(Size.cx / 2));
        Device->DrawText(X, Y, Color, Label, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10001b50, NCButton, OnPaint)

// 0x10002b70 (vslot 57) - the tool tip is the window skin's (the button's ToolTip by value, drawn at the window's
// corner: X and Y are 0 now). C3 measures nothing: the local rectangle of the desktop is asked for and not used
INT NCButton::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (bMouseOver)
    {
        NRectF Screen;
        Screen.X = 0;
        Screen.Y = 0;
        Screen.W = 0.0f;
        Screen.H = 0.0f;
        GConsole->Desktop->GetLocalRect(&Screen);
        if (ToolTipSkin)
        {
            NToolTipRect Wnd;
            Wnd.X = ScreenX;
            Wnd.Y = ScreenY;
            Wnd.W = Width;
            Wnd.H = Height;
            ToolTipSkin->Draw(Device, ToolTip, 0, 0, Wnd);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10002b70, NCButton, OnDrawToolTip)

// 0x10001210 (vslot 59) - Enter / Space click it while it has the focus (C3: the sound only with bClickSound)
INT NCButton::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if ((wParam == VK_RETURN || wParam == VK_SPACE) && bAcceptKey == 1)
    {
        if (bClickSound)
            PlayInterfaceSound(IsEnabled() ? SOUND_Click : SOUND_ClickDisabled);
        NotifyParent(0);
    }
    return 1;
}
NW_HOOK_METHOD(0x10001210, NCButton, OnKeyDown)

// 0x100010c0 (vslot 61) - dragging out of the button releases it without a click; C3 marks the window the mouse is
// over (vslots 52 / 53: the NWS_ITEMSELECT bit) while the pointer is inside
INT NCButton::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if ((wParam & MK_LBUTTON) && IsInWindow(LOWORD(lParam), HIWORD(lParam)))
        bReleased = 0;
    else
        bReleased = 1;
    if (PtInRect(LOWORD(lParam), HIWORD(lParam), Rect))
    {
        bMouseOver = 1;
        SetItemSelect();
        return 1;
    }
    bMouseOver = 0;
    ClearItemSelect();
    return 1;
}
NW_HOOK_METHOD(0x100010c0, NCButton, OnMouseMove)

// 0x10001000 (vslot 62) - C3: the click sound is optional (bClickSound)
INT NCButton::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)))
    {
        bReleased = 0;
        if (bClickSound)
            PlayInterfaceSound(IsEnabled() ? SOUND_Click : SOUND_ClickDisabled);
    }
    return 1;
}
NW_HOOK_METHOD(0x10001000, NCButton, OnLButtonDown)

// 0x10001070 (vslot 63) - the click: released inside, enabled
INT NCButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)) && IsEnabled())
        NotifyParent(NotifyID == -1 ? ID : NotifyID);
    bReleased = 1;
    return 1;
}
NW_HOOK_METHOD(0x10001070, NCButton, OnLButtonUp)

// 0x10001270 (name ours) - the label is the SysString (copied into Text too)
void NCButton::SetTextID(INT InID)
{
    if (*SysString(InID))
    {
        TextID = InID;
        Text = *SysString(InID);
    }
}
NW_HOOK_METHOD(0x10001270, NCButton, SetTextID)

// 0x100012d0 - C2: Text is an FString
void NCButton::SetText(const TCHAR* InText)
{
    if (InText && *InText)
    {
        TextID = -1;
        Text = InText;
    }
}
NW_HOOK_METHOD(0x100012d0, NCButton, SetText)

// 0x10001300
UBOOL NCButton::SetNormalTex(const TCHAR* Name)
{
    if (Name && *Name)
    {
        NormalTex = FindTexture(Name);
        NormalTexW = NormalTex->USize;
        NormalTexH = NormalTex->VSize;
        return 1;
    }
    NormalTex = NULL;
    return 1;
}
NW_HOOK_METHOD(0x10001300, NCButton, SetNormalTex)

// 0x10001350 - unlike SetNormalTex, an empty name still looks the texture up
UBOOL NCButton::SetPushedTex(const TCHAR* Name)
{
    if (Name)
    {
        PushedTex = FindTexture(Name);
        PushedTexW = PushedTex->USize;
        PushedTexH = PushedTex->VSize;
        return 1;
    }
    PushedTex = NULL;
    return 1;
}
NW_HOOK_METHOD(0x10001350, NCButton, SetPushedTex)

// 0x100013a0 - the tip's text is a SysString; the tool tip data gets it as a plain tip (NTT_Plain)
void NCButton::SetToolTip(INT InID, UBOOL bAbove)
{
    if (InID >= 0)
    {
        if (*SysString(InID))
        {
            bToolTipAbove = bAbove;
            ToolTipID = InID;
            ToolTip.SetText(*SysString(InID), NTT_Plain);
        }
    }
}
NW_HOOK_METHOD(0x100013a0, NCButton, SetToolTip)

// 0x10001410 (ret 8; anonymous in the db) - the same with a text: nothing for NULL, else the tip's side and the text as a
// plain tip (the stock code tail-jumps into the tool tip data's SetText with the style 0x400)
void NCButton::SetToolTipText(const TCHAR* InText, UBOOL bAbove)
{
    if (InText)
    {
        bToolTipAbove = bAbove;
        ToolTip.SetText(InText, NTT_Plain);
    }
}
NW_HOOK_METHOD(0x10001410, NCButton, SetToolTipText)

NW_CHECK_VTABLE(NCButton, 0x10176728, 69)

/*-----------------------------------------------------------------------------
    NCExButton: reports press (1) and release (0).
-----------------------------------------------------------------------------*/

// 0x10002c50 - NCButton's, only the vtable differs
NCExButton::NCExButton()
{
}
NW_HOOK_CTOR(0x10002c50, NCExButton)

// 0x10002cf0
NCExButton::NCExButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, UBOOL bInAcceptKey)
    : NCButton(NormalTexName, PushedTexName, bInAcceptKey)
{
}
NW_HOOK_CTOR_ARGS(0x10002cf0, NCExButton, (const TCHAR* A, const TCHAR* B, UBOOL C), (A, B, C))

// 0x10002ca0
NW_HOOK_DTOR(0x10002ca0, NCExButton)

// 0x10041760 (vslot 25) - one body for NCExButton, NCPushButton and NCRadioButton
INT NCExButton::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCButton::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x10041760, NCExButton, WindowProc)

// 0x10001440 (vslot 62) - reports even when the press is outside the button
INT NCExButton::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)))
        bReleased = 0;
    NotifyParent(1);
    return 1;
}
NW_HOOK_METHOD(0x10001440, NCExButton, OnLButtonDown)

// 0x10001480 (vslot 63)
INT NCExButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    NotifyParent(0);
    bReleased = 1;
    return 1;
}
NW_HOOK_METHOD(0x10001480, NCExButton, OnLButtonUp)

NW_CHECK_VTABLE(NCExButton, 0x10176870, 69)

/*-----------------------------------------------------------------------------
    NCPushButton: reports on press, then every RepeatDelay ms while held.
-----------------------------------------------------------------------------*/

// 0x10002df0
NCPushButton::NCPushButton()
{
    bHeld = 0;
    RepeatDelay = 100;
    bAutoRepeat = 1;
}
NW_HOOK_CTOR(0x10002df0, NCPushButton)

// 0x10002ea0 - RepeatDelay and bAutoRepeat stay as allocated (zero): no auto-repeat
NCPushButton::NCPushButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName)
{
    bHeld = 0;
    SetNormalTex(NormalTexName);
    SetPushedTex(PushedTexName);
}
NW_HOOK_CTOR_ARGS(0x10002ea0, NCPushButton, (const TCHAR* A, const TCHAR* B), (A, B))

// 0x10002e50
NW_HOOK_DTOR(0x10002e50, NCPushButton)

// vslot 25: the stock slot holds NCExButton's body above (0x1003a790, one body for the three)
INT NCPushButton::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCButton::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10001620 (vslot 36)
INT NCPushButton::OnTimer(INT TimerID)
{
    if (TimerID == (INT)TIMER_PushRepeat && bHeld)
        NotifyParent(0);
    return 1;
}
NW_HOOK_METHOD(0x10001620, NCPushButton, OnTimer)

// 0x10001650 (vslot 38) - guarded; NCButton's (the stock one calls it, 0x10001180: no inline copy)
INT NCPushButton::OnDestroy()
{
    guard(NCPushButton::OnDestroy);
    NCButton::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10001650, NCPushButton, OnDestroy)

// 0x100015b0 (vslot 61)
INT NCPushButton::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if ((wParam & MK_LBUTTON) && IsFocused() && IsInWindow(LOWORD(lParam), HIWORD(lParam)))
    {
        bHeld = 1;
        bReleased = 0;
        return 1;
    }
    bHeld = 0;
    bReleased = 1;
    return 1;
}
NW_HOOK_METHOD(0x100015b0, NCPushButton, OnMouseMove)

// 0x100014f0 (vslot 62) - no hit test; NotifyID is sent as is (-1 is not replaced by ID); the sound is optional
INT NCPushButton::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    bReleased = 0;
    bHeld = 1;
    if (bClickSound)
        PlayInterfaceSound(IsEnabled() ? SOUND_Click : SOUND_ClickDisabled);
    NotifyParent(NotifyID);
    if (bAutoRepeat)
        SetTimer(TIMER_PushRepeat, RepeatDelay, NULL);
    return 1;
}
NW_HOOK_METHOD(0x100014f0, NCPushButton, OnLButtonDown)

// 0x10001570 (vslot 63)
INT NCPushButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    bReleased = 1;
    bHeld = 0;
    if (bAutoRepeat)
        KillTimer(TIMER_PushRepeat);
    ReleaseFocus();
    return 1;
}
NW_HOOK_METHOD(0x10001570, NCPushButton, OnLButtonUp)

NW_CHECK_VTABLE(NCPushButton, 0x10176aa0, 69)

/*-----------------------------------------------------------------------------
    NCRadioButton: stays down once clicked and reports Value.
-----------------------------------------------------------------------------*/

// 0x10002f00
NCRadioButton::NCRadioButton()
{
    CreateHeight = 0;
    CreateWidth = 0;
    bReleased = 1;
    Value = -1;
}
NW_HOOK_CTOR(0x10002f00, NCRadioButton)

// 0x10002fc0 - sets the textures twice (base constructor, then the setters), as the stock code
NCRadioButton::NCRadioButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName)
    : NCButton(NormalTexName, PushedTexName, 0)
{
    Value = -1;
    CreateHeight = 0;
    CreateWidth = 0;
    SetNormalTex(NormalTexName);
    SetPushedTex(PushedTexName);
}
NW_HOOK_CTOR_ARGS(0x10002fc0, NCRadioButton, (const TCHAR* A, const TCHAR* B), (A, B))

// 0x10002f70
NW_HOOK_DTOR(0x10002f70, NCRadioButton)

// vslot 25: the stock slot holds NCExButton's body above (0x1003a790, one body for the three)
INT NCRadioButton::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCButton::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10001990 (vslot 37) - remembers the size it was created with
INT NCRadioButton::OnCreate()
{
    NControl::OnCreate();
    CreateWidth = (INT)Width;
    CreateHeight = (INT)Height;
    return 1;
}
NW_HOOK_METHOD(0x10001990, NCRadioButton, OnCreate)

// 0x100019c0 (vslot 38) - NCButton's, a bare call
INT NCRadioButton::OnDestroy()
{
    NCButton::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100019c0, NCRadioButton, OnDestroy)

// 0x100021a0 (vslot 57) - the tip covers the button itself, sized to its text (the plain NCButton draws through the
// skin). C3 asks for the desktop's local rectangle and does not use it; the text is light grey
INT NCRadioButton::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (ToolTipID >= 0)
    {
        if (*SysString(ToolTipID) && bMouseOver)
        {
            SIZE Size;
            NCGDevice::GetTextExtent(&Size, *SysString(ToolTipID), 0, 0, 0, 0, 0, L2FONT_Normal);
            NRectF Screen;
            Screen.X = 0;
            Screen.Y = 0;
            Screen.W = 0.0f;
            Screen.H = 0.0f;
            GConsole->Desktop->GetLocalRect(&Screen);
            Device->SetClip(ScreenX, ScreenY, Size.cx, Size.cy);
            if (ToolTipTex)
                Device->DrawStretchedTex(0, 0, Size.cx, Size.cy, ToolTipTex, 1, 0xFF);
            Device->DrawText(0, 0, 0xFFDCDCDC, *SysString(ToolTipID), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
            Device->ReleaseClip();
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x100021a0, NCRadioButton, OnDrawToolTip)

// 0x10001730 (vslot 61) - only while enabled
INT NCRadioButton::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if (IsEnabled())
        bMouseOver = PtInRect(LOWORD(lParam), HIWORD(lParam), Rect);
    return 1;
}
NW_HOOK_METHOD(0x10001730, NCRadioButton, OnMouseMove)

// 0x100016d0 (vslot 62) - C3: every click flips the button and reports Value (C2 had the bToggle mode and the
// no-label test)
INT NCRadioButton::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsEnabled())
    {
        bReleased = (bReleased == 0);
        NotifyParent(Value);
    }
    return 1;
}
NW_HOOK_METHOD(0x100016d0, NCRadioButton, OnLButtonDown)

// 0x10001710 (vslot 63) - only while enabled
INT NCRadioButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (IsEnabled())
        ReleaseFocus();
    return 1;
}
NW_HOOK_METHOD(0x10001710, NCRadioButton, OnLButtonUp)

NW_CHECK_VTABLE(NCRadioButton, 0x10176bb8, 69)
