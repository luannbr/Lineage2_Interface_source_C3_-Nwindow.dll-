/*=============================================================================
    NCNoticeWnd.cpp - the announcements box and its text. Stock: 0x10079480 .. 0x1007956d,
    0x100796e0 .. 0x10079d69 (NCWarningWnd's functions sit in between and after).
=============================================================================*/
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <wchar.h>

// the stock code calls the CRT's wcscpy (0x1014c9bb); the intrinsic would copy inline
#pragma function(wcscpy)

// NWM_COMMAND id of OK, title bar id (names ours)
enum
{
    NID_NOTICE_OK = 0xF9,
    NID_NOTICE_FRAME = 0xA000F9,
};

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_Click = 0 };

/*-----------------------------------------------------------------------------
    NCNoticeInfoWnd.
-----------------------------------------------------------------------------*/

// 0x100b17e0 - lines 16 pixels apart in the scroll window, an empty text
NCNoticeInfoWnd::NCNoticeInfoWnd()
{
    Text[0] = 0;
    Unknown0C8 = 16;
}
NW_HOOK_CTOR(0x100b17e0, NCNoticeInfoWnd)

// 0x100b1840
NW_HOOK_DTOR(0x100b1840, NCNoticeInfoWnd)

// vslot 25: the stock slot is 0x10019810, the body shared with NCItemWnd's (hooked there): the base's answer as a
// plain 0 / 1 (C2 has no enabled test here)
INT NCNoticeInfoWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCScrollWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100b09d0 (vslot 37) - C3: the picture of the striped background
INT NCNoticeInfoWnd::OnCreate()
{
    NCScrollWnd::OnCreate();
    BackTex = FindTexture(TEXT("l2ui_ch3.etc.textbackline"));
    return 1;
}
NW_HOOK_METHOD(0x100b09d0, NCNoticeInfoWnd, OnCreate)

// 0x100b0a00 (vslot 38)
INT NCNoticeInfoWnd::OnDestroy()
{
    guard(NCNoticeInfoWnd::OnDestroy);
    NCScrollWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b0a00, NCNoticeInfoWnd, OnDestroy)

// 0x100b0dc0 (vslot 56) - C3: the odd 15 pixel rows of the window get the stripe (a texture 8 x 15 stretched), then
// the text wrapped at the width less 20, shifted by the scroll position (NCGDevice's origin stack); its height sets
// the scroll range. The clip goes on before BeginPaint.
INT NCNoticeInfoWnd::OnPaint(NCGDevice* Device)
{
    guard(NCNoticeInfoWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    for (INT Row = 0; Row < (INT)(Height * 0.06666667f); Row++)
    {
        if (Row % 2 == 1)
            Device->DrawTexture(0, Row * 15, (INT)(Width - 15.0f), 15, 0, 0, 8, 15, BackTex, 0xFF, 1);
    }
    const TCHAR* Str = Text;
    if (Str && *Str)
    {
        Device->PushOrigin(ScrollX, ScrollY);
        Device->DrawText(1, 1, 0xFFDCDCDC, Str, (INT)(Width - 16.0f - 4.0f), 0, 1, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Str, (INT)(Width - 16.0f - 4.0f), 0, 1, 0, 0, L2FONT_Normal);
        SetContentHeight(Size.cy);
        Device->PopOrigin();
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b0dc0, NCNoticeInfoWnd, OnPaint)

// 0x100b0a80 - an empty text leaves the old one (wcscpy, as in the stock code)
__declspec(noinline) void NCNoticeInfoWnd::SetText(const TCHAR* InText)
{
    if (InText && *InText)
        wcscpy(Text, InText);
}
NW_HOOK_METHOD(0x100b0a80, NCNoticeInfoWnd, SetText)

// 0x100b0ab0
__declspec(noinline) void NCNoticeInfoWnd::ClearText()
{
    Text[0] = 0;
}
NW_HOOK_METHOD(0x100b0ab0, NCNoticeInfoWnd, ClearText)

NW_CHECK_VTABLE(NCNoticeInfoWnd, 0x10191550, 72)

/*-----------------------------------------------------------------------------
    NCNoticeWnd.
-----------------------------------------------------------------------------*/











// C2: no vtable for NW_CHECK_VTABLE(NCNoticeWnd, 0x1011d5a8, 68)
