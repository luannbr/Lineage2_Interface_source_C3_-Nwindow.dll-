/*=============================================================================
    NCHtmlSliderBar.cpp - the slider of the HTML pages. Stock: 0x10032fc0 .. 0x1003305f,
    0x10033060 .. 0x1003321e (WindowProc is a body the linker folded with the buttons').
=============================================================================*/
#include "nwindow/NCHtmlSliderBar.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCGDevice.h"

// 0x10045a10 - NCSliderBar's, nothing more
NCHtmlSliderBar::NCHtmlSliderBar()
{
}
NW_HOOK_CTOR(0x10045a10, NCHtmlSliderBar)

// 0x10045a60
NW_HOOK_DTOR(0x10045a60, NCHtmlSliderBar)

// 0x10045ab0 (anonymous in the stock image) - the three textures of the thumb by name
NCHtmlSliderBar::NCHtmlSliderBar(const TCHAR* TopTexName, const TCHAR* MidTexName, const TCHAR* BottomTexName)
{
    ThumbTopTex = FindTexture(TopTexName);
    ThumbMidTex = FindTexture(MidTexName);
    ThumbBottomTex = FindTexture(BottomTexName);
}
NW_HOOK_CTOR_ARGS(0x10045ab0, NCHtmlSliderBar, (const TCHAR* A, const TCHAR* B, const TCHAR* C), (A, B, C))


// stock body shared with NCExButton's (0x100301f0)
INT NCHtmlSliderBar::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCSliderBar::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10044b30 (vslot 56) - NCSliderBar::OnPaint with the clip of the page (the first ancestor that is not a control);
// the origin goes on before the clip. The thumb's caps are 15 x 8, as the bar's own
INT NCHtmlSliderBar::OnPaint(NCGDevice* Device)
{
    guard(NCHtmlSliderBar::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    NCWnd* Owner = FirstNonControlParent(this);
    if (Owner)
        Device->SetClip(Owner->ScreenX, Owner->ScreenY, (INT)Owner->Width, (INT)Owner->Height);
    if (ThumbSize > 0)
    {
        if (ThumbTopTex)
            Device->DrawTexture(0, ThumbPos, 15, 8, 0, 0, 15, 8, ThumbTopTex, 0xFF, 1);
        if (ThumbBottomTex)
            Device->DrawTexture(0, ThumbPos + ThumbSize - 8, 15, 8, 0, 0, 15, 8, ThumbBottomTex, 0xFF, 1);
        if (ThumbMidTex)
            Device->DrawStretchedTex(0, ThumbPos + 8, 15, ThumbSize - 16, 0, 0, 15, 8, ThumbMidTex, 0xFF);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10044b30, NCHtmlSliderBar, OnPaint)

NW_CHECK_VTABLE(NCHtmlSliderBar, 0x10180cc8, 69)
