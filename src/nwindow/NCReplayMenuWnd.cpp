/*=============================================================================
    NCReplayMenuWnd.cpp - the replay's menu screen (C3 only; C3 addresses). Stock: OnDestroy 0x100E6C10, OnPaint
    0x100E6CC0, the constructor 0x100E7880, the destructor 0x100E78E0 and OnCreate 0x100E7930.
=============================================================================*/
#include "nwindow/NCReplayMenuWnd.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCReplayFunctionWnd.h"

// 0x100e7880 - the four fields cleared, then the vtable
NCReplayMenuWnd::NCReplayMenuWnd() : FunctionWnd(NULL), UnknownCC(0), UnknownD0(0), BackTex(NULL)
{
}
NW_HOOK_CTOR(0x100e7880, NCReplayMenuWnd)

// 0x100e78e0
NW_HOOK_DTOR(0x100e78e0, NCReplayMenuWnd)

// 0x10119f60 (vslot 25) - NControl's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCReplayMenuWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x10119f60)
#else
NW_SLOT_IS("?WindowProc@NCReplayMenuWnd@@UAEHIIJ@Z", "?WindowProc@NControl@@UAEHIIJ@Z")   // NCReplayMenuWnd::WindowProc = NControl::WindowProc (0x10119f60)
#endif

// 0x100e6c10 (vslot 38, guard: NCReplayMenuWnd::OnDestroy) - NCWnd's, then the function window goes
INT NCReplayMenuWnd::OnDestroy()
{
    guard(NCReplayMenuWnd::OnDestroy);
    NCWnd::OnDestroy();
    if (FunctionWnd)
    {
        RemoveChildWindow(FunctionWnd->Entry);
        delete FunctionWnd;
        FunctionWnd = NULL;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100e6c10, NCReplayMenuWnd, OnDestroy)

// 0x100e6cc0 (vslot 56, guard: NCReplayMenuWnd::OnPaint) - the 1024 x 768 backdrop stretched over the window
INT NCReplayMenuWnd::OnPaint(NCGDevice* Device)
{
    guard(NCReplayMenuWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawStretchedTex(0, 0, (INT)Width, (INT)Height, 0, 0, 0x400, 0x300, BackTex, 0xFF);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100e6cc0, NCReplayMenuWnd, OnPaint)

// 0x100e7930 (vslot 37) - the backdrop, and the function window (120 x 138) 154 pixels from the right edge and 236
// from the bottom one
INT NCReplayMenuWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.BenchMarkWnd.BenchMark_Title"));
    FunctionWnd = new NCReplayFunctionWnd;
    FunctionWnd->SetPosition(GSEKScreenX - 0x9A, GSEKScreenY - 0xEC, 0x78, 0x8A);
    CreateChild(FunctionWnd, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100e7930, NCReplayMenuWnd, OnCreate)

NW_CHECK_VTABLE(NCReplayMenuWnd, 0x1019af10, 68)
