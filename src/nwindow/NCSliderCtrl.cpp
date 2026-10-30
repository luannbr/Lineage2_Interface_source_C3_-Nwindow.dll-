/*=============================================================================
    NCSliderCtrl.cpp - the slider control (C2 only). Stock: 0x10026140 .. 0x100264ed.
=============================================================================*/
#include "nwindow/NCSliderCtrl.h"
#include "nwindow/NCThumbButton.h"
#include "nwindow/NCGDevice.h"

enum { NID_SLIDER_THUMB = (INT)0xF0F01415 };   // the thumb's id (name ours)

// 0x10029e70 - only NControl's stores (inlined in the stock constructor); the creator sets Max, Pos and the sizes
NCSliderCtrl::NCSliderCtrl()
{
}
NW_HOOK_CTOR(0x10029e70, NCSliderCtrl)

// 0x10029ee0
NW_HOOK_DTOR(0x10029ee0, NCSliderCtrl)


// 0x10029b50 (vslot 25)
INT NCSliderCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_SLIDER_THUMB)
        return OnThumbMove(lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10029b50, NCSliderCtrl, WindowProc)

// 0x10029bb0 (vslot 69) - the thumb was dragged to X (screen): Pos follows, kept in 0..Max, and
// the thumb moves by the difference
INT NCSliderCtrl::OnThumbMove(INT X)
{
    X = X - ScreenX;
    INT Old = Pos;
    Pos = X;
    if (X < Max)
    {
        if (X < 1)
            Pos = 0;
    }
    else
        Pos = Max;
    Thumb->MoveWindow(Pos - Old, 0);
    return 1;
}
NW_HOOK_METHOD(0x10029bb0, NCSliderCtrl, OnThumbMove)

// 0x10029d50 (vslot 37) - the black bar and the thumb button (menu highlight over white), id 0xF0F01415, ThumbW x ThumbH
// at the place Pos / Max of the control's width, centered on it
INT NCSliderCtrl::OnCreate()
{
    NControl::OnCreate();
    BackTex = FindTexture(TEXT("NWindow.BlackTexture"));
    NRectF R;
    R.X = 0;
    R.Y = 0;
    R.W = 0.0f;
    R.H = 0.0f;
    GetLocalRect(&R);
    Thumb = new NCThumbButton(TEXT("NWindow.MenuHighlight"), TEXT("NWindow.WhiteTexture"), 0);
    Thumb->ID = NID_SLIDER_THUMB;
    Thumb->SetPosition((INT)((DOUBLE)Pos * R.W / (DOUBLE)Max - (DOUBLE)(ThumbW / 2)), 0, ThumbW, ThumbH);
    CreateChild(Thumb, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10029d50, NCSliderCtrl, OnCreate)

// 0x10029c10 (vslot 38) - guarded in C2; NControl's OnDestroy is not called
INT NCSliderCtrl::OnDestroy()
{
    guard(NCSliderCtrl::OnDestroy);
    if (Thumb)
    {
        RemoveChildWindow(Thumb->Entry);
        delete Thumb;
        Thumb = NULL;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10029c10, NCSliderCtrl, OnDestroy)

// 0x10029cb0 (vslot 55) - the origin first, then the clip
INT NCSliderCtrl::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10029cb0, NCSliderCtrl, OnPaint)

NW_CHECK_VTABLE(NCSliderCtrl, 0x1017d6a8, 70)
