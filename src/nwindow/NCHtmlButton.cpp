/*=============================================================================
    NCHtmlButton.cpp - NCHtmlButton and NCHtmlPushButton, the buttons of the HTML pages.
    Stock: 0x1002fee0 .. 0x1002ff16, 0x10030840 .. 0x10030fb9 (WindowProc is a body the linker
    folded with the other buttons').
=============================================================================*/
#include "nwindow/NCHtmlButton.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCGDevice.h"

// the face and the label as NCButton::OnPaint draws them (C3: whole texture from the corner, no clip offset; the label
// needs a first character in both states; the enabled state is read from the field, the same code in both classes of
// the stock build)
static FORCEINLINE void DrawHtmlButton(NCButton* Button, NCGDevice* Device)
{
    if (Button->bReleased)
        Device->DrawTexture(0, 0, (INT)Button->Width, (INT)Button->Height, 0, 0, (INT)Button->Width,
                            (INT)Button->Height, Button->NormalTex, 0xFF, 1);
    else
        Device->DrawTexture(0, 0, (INT)Button->Width, (INT)Button->Height, 0, 0, (INT)Button->Width,
                            (INT)Button->Height, Button->PushedTex, 0xFF, 1);

    SIZE Size;
    const TCHAR* Label = NULL;
    if (Button->TextID >= 0)
    {
        if (*Button->SysString(Button->TextID))
            Label = *Button->SysString(Button->TextID);
    }
    else if (Button->Text.Num() && *Button->Text.GetCharData())
        Label = *Button->Text;
    if (Label)
    {
        NCGDevice::GetTextExtent(&Size, Label, 0, 0, 0, 0, 0, L2FONT_Normal);
        DWORD Color = Button->bEnabled ? 0xFFE6DCBE : 0xFFA0A0A0;
        INT Y = (INT)((DOUBLE)Button->TextOffsetY + (DOUBLE)Button->Height * 0.5 - (DOUBLE)(Size.cy / 2));
        INT X = (INT)((DOUBLE)Button->TextOffsetX + (DOUBLE)Button->Width * 0.5 - (DOUBLE)(Size.cx / 2));
        Device->DrawText(X, Y, Color, Label, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }
}

/*-----------------------------------------------------------------------------
    NCHtmlButton.
-----------------------------------------------------------------------------*/

// 0x100455a0 - NCButton's, nothing more
NCHtmlButton::NCHtmlButton()
{
}
NW_HOOK_CTOR(0x100455a0, NCHtmlButton)

// 0x100455f0
NW_HOOK_DTOR(0x100455f0, NCHtmlButton)

// 0x10045640
NCHtmlButton::NCHtmlButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName,
                           const TCHAR* InCommand, UBOOL bInAcceptKey)
    : NCButton(NormalTexName, PushedTexName, bInAcceptKey)
{
    Command = InCommand;
}
NW_HOOK_CTOR_ARGS(0x10045640, NCHtmlButton,
                  (const TCHAR* A, const TCHAR* B, const TCHAR* C, UBOOL D), (A, B, C, D))

// 0x10045670 - the textures, Enter and Space do not click it; Command is left as allocated
NCHtmlButton::NCHtmlButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName)
    : NCButton(NormalTexName, PushedTexName, 0)
{
}
NW_HOOK_CTOR_ARGS(0x10045670, NCHtmlButton, (const TCHAR* A, const TCHAR* B), (A, B))


// 0x1003a790 (vslot 25) - the stock body is NCExButton's (hooked in NCButton.cpp); C2: no enabled test
INT NCHtmlButton::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCButton::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10041d60 (vslot 56) - NCButton::OnPaint clipped to the enclosing NCHtmlViewer (no clip when there is none)
INT NCHtmlButton::OnPaint(NCGDevice* Device)
{
    guard(NCHtmlButton::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    NCWnd* Owner = EnclosingHtmlViewer(this);
    if (Owner)
        Device->SetClip(Owner->ScreenX, Owner->ScreenY, (INT)Owner->Width, (INT)Owner->Height);
    DrawHtmlButton(this, Device);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10041d60, NCHtmlButton, OnPaint)

// 0x10041540 (vslot 63) - a release inside reports Command, even when disabled, without sound
INT NCHtmlButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)))
        NotifyParent((LPARAM)Command);
    bReleased = 1;
    return 1;
}
NW_HOOK_METHOD(0x10041540, NCHtmlButton, OnLButtonUp)

NW_CHECK_VTABLE(NCHtmlButton, 0x101806e0, 69)

/*-----------------------------------------------------------------------------
    NCHtmlPushButton.
-----------------------------------------------------------------------------*/

// 0x100456a0 - NCPushButton's, nothing more
NCHtmlPushButton::NCHtmlPushButton()
{
}
NW_HOOK_CTOR(0x100456a0, NCHtmlPushButton)

// 0x100456f0
NW_HOOK_DTOR(0x100456f0, NCHtmlPushButton)

// 0x10045740 - NCPushButton's default constructor, then the textures
NCHtmlPushButton::NCHtmlPushButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName)
{
    SetNormalTex(NormalTexName);
    SetPushedTex(PushedTexName);
}
NW_HOOK_CTOR_ARGS(0x10045740, NCHtmlPushButton, (const TCHAR* A, const TCHAR* B), (A, B))


// 0x100423f0 (vslot 56) - the same, clipped to the first ancestor that is not a control (the scroll buttons of an HTML
// edit box)
INT NCHtmlPushButton::OnPaint(NCGDevice* Device)
{
    guard(NCHtmlPushButton::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    NCWnd* Owner = FirstNonControlParent(this);
    if (Owner)
        Device->SetClip(Owner->ScreenX, Owner->ScreenY, (INT)Owner->Width, (INT)Owner->Height);
    DrawHtmlButton(this, Device);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100423f0, NCHtmlPushButton, OnPaint)

NW_CHECK_VTABLE(NCHtmlPushButton, 0x101807f8, 69)
