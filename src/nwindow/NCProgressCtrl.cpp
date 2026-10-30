/*=============================================================================
    NCProgressCtrl.cpp - count-down bar. Stock: 0x10005240 .. 0x100052ab,
    0x10005730 .. 0x10005a7e.
=============================================================================*/
#include "nwindow/NCProgressCtrl.h"
#include "nwindow/NCGDevice.h"

// 0x100067d0 - white bar on black, 2 pixels of border, created stopped
NCProgressCtrl::NCProgressCtrl()
{
    Unknown1A8 = 1;
    bStopped = 1;
    BarTex = NULL;
    BackTex = NULL;
    Border = 2;
    Pos = 0;
    Max = 0;
    BackTex = FindTexture(TEXT("NWindow.BlackTexture"));
    BarTex = FindTexture(TEXT("NWindow.WhiteTexture"));
}
NW_HOOK_CTOR(0x100067d0, NCProgressCtrl)

// 0x10006910 - the textures by name (NULL: the defaults) and the border
NCProgressCtrl::NCProgressCtrl(const TCHAR* BackTexName, const TCHAR* BarTexName, INT InBorder)
{
    Unknown1A8 = 1;
    bStopped = 1;
    BarTex = NULL;
    BackTex = NULL;
    Border = 2;
    Pos = 0;
    Max = 0;
    BackTex = FindTexture(BackTexName ? BackTexName : TEXT("NWindow.BlackTexture"));
    BarTex = FindTexture(BarTexName ? BarTexName : TEXT("NWindow.WhiteTexture"));
    Border = InBorder;
}
NW_HOOK_CTOR_ARGS(0x10006910, NCProgressCtrl, (const TCHAR* A, const TCHAR* B, INT C), (A, B, C))

// 0x100068c0
NW_HOOK_DTOR(0x100068c0, NCProgressCtrl)

// stock body shared with NCButton's (0x100051f0)
INT NCProgressCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10005fa0 - `return 1` shared by 6 slots of several classes
NW_HOOK_METHOD(0x10005fa0, NCProgressCtrl, OnCreate)

// 0x10006240 (vslot 56) - the time since the last frame (ms) is added to Pos unless it is stopped; at Max the parent is told
// (NWM_COMMAND, 0) and it stops. The bar is the part of the inside (the width less the borders) that is left: it
// shrinks from full to empty. The share is an unsigned division by Max (Max 0 faults, as in the stock code)
INT NCProgressCtrl::OnPaint(NCGDevice* Device)
{
    guard(NCProgressCtrl::OnPaint);
    DWORD Delta = (DWORD)(INT)(GetDeltaTime() * 1000.0f);
    INT Inner = (INT)(Width - (FLOAT)(Border * 2));
    if (!bStopped)
    {
        Pos += Delta;
        if (Pos >= Max)
        {
            NotifyParent(0);
            Pos = Max;
            bStopped = 1;
        }
    }
    INT Filled = (INT)((DWORD)(Pos * (DWORD)Inner) / Max);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, (INT)Width, (INT)Height, BackTex, 0xFF, 1);
    if (BarTex)
        Device->DrawTexture(Border, Border, Inner - Filled, (INT)(Height - (FLOAT)(Border * 2)), 0, 0,
                            (INT)Width, (INT)Height, BarTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10006240, NCProgressCtrl, OnPaint)

// 0x10005fb0
void NCProgressCtrl::SetRemaining(DWORD Remaining)
{
    Pos = Max - Remaining;
}
NW_HOOK_METHOD(0x10005fb0, NCProgressCtrl, SetRemaining)

// 0x10005fd0
void NCProgressCtrl::SetMax(DWORD InMax)
{
    Max = InMax;
}
NW_HOOK_METHOD(0x10005fd0, NCProgressCtrl, SetMax)

// 0x10005fe0
void NCProgressCtrl::Reset()
{
    Pos = 0;
    bStopped = 1;
}
NW_HOOK_METHOD(0x10005fe0, NCProgressCtrl, Reset)

// 0x10006000
void NCProgressCtrl::Stop()
{
    bStopped = 1;
}
NW_HOOK_METHOD(0x10006000, NCProgressCtrl, Stop)

// 0x10006010
void NCProgressCtrl::Start()
{
    bStopped = 0;
}
NW_HOOK_METHOD(0x10006010, NCProgressCtrl, Start)

NW_CHECK_VTABLE(NCProgressCtrl, 0x10177b88, 69)
