/*=============================================================================
    NCMagicSkillWnd.cpp - the skill window and its GM copy (C3 addresses). Stock: 0x1005d050, 0x100a1170,
    0x100a1bd0 .. 0x100a1f7b, 0x100a26b0 .. 0x100a2750 .. 0x100a281b (WindowProc is NControl's body 0x10119f60,
    OnDestroy NCPetActionWnd's), NCGMMagicSkillWnd's 0x1007e2e0, 0x1007f880 .. 0x1007f9db, 0x10084cd0 ..
    0x10084ec9.
=============================================================================*/
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCSkillWnd.h"
#include "nwindow/NCSkillItemWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <wchar.h>

// 0x1005d050 (sub_1005d050, a method of the console, ret 4) - NetHandler 0x28c (slot 163): the skill list request. This
// client's NConsoleWnd::RequestSkillList (0x1005d040) is NetHandler 0x268, the skill USE request (the names came from
// the C2 map, whose slots are 9 apart here); the window asks for its list with this one. The method is declared on a
// view of NConsoleWnd (its header is not this lane's)
struct NConsoleWndSkillListView : public NConsoleWnd
{
    __declspec(noinline) void RequestSkillListC3(L2ParamStack& Params)
    {
        nw::VirtualCall<void>(Engine->NetHandler, 0x28C / 4, &Params);
    }
};
NW_HOOK_METHOD(0x1005d050, NConsoleWndSkillListView, RequestSkillListC3)

/*-----------------------------------------------------------------------------
    NCMagicSkillWnd
-----------------------------------------------------------------------------*/

// 0x100a26b0 - NCWnd's constructor and nothing else (BackTex and SkillWnd are left as allocated)
NCMagicSkillWnd::NCMagicSkillWnd()
{
}
NW_HOOK_CTOR(0x100a26b0, NCMagicSkillWnd)

// 0x100a2700
NW_HOOK_DTOR(0x100a2700, NCMagicSkillWnd)

// 0x10119f60 (vslot 25) - NControl's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCMagicSkillWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x10119f60)
#else
NW_SLOT_IS("?WindowProc@NCMagicSkillWnd@@UAEHIIJ@Z", "?WindowProc@NControl@@UAEHIIJ@Z")   // NCMagicSkillWnd::WindowProc = NControl::WindowProc (0x10119f60)
#endif

// 0x100a2750 (vslot 37) - the background and the skill list, as big as the window (C3: the window's own size)
INT NCMagicSkillWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.SkillWnd.skill_back"));
    SkillWnd = new NCSkillWnd;
    SkillWnd->ID = NID_MAGICSKILL_LIST;
    SkillWnd->SetPosition(0, 0, (INT)Width, (INT)Height);
    CreateChild(SkillWnd, NWS_VISIBLE | 0x20);
    return 1;
}
NW_HOOK_METHOD(0x100a2750, NCMagicSkillWnd, OnCreate)

// 0x100a1bd0 (vslot 38) - the skill list goes (same body as NCPetActionWnd's, its own copy); BackTex is left
INT NCMagicSkillWnd::OnDestroy()
{
    guard(NCMagicSkillWnd::OnDestroy);
    if (SkillWnd)
    {
        RemoveChildWindow(SkillWnd->Entry);
        delete SkillWnd;
        SkillWnd = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100a1bd0, NCMagicSkillWnd, OnDestroy)

// 0x100a1d20 (vslot 44) - only when hidden: the lists empty, the server is asked for them (0x1005d050)
void NCMagicSkillWnd::ShowWindow()
{
    if (!IsShowWindow())
    {
        ClearSkills();
        L2ParamStack Params(10);
        static_cast<NConsoleWndSkillListView*>(GConsole)->RequestSkillListC3(Params);
        NCWnd::ShowWindow();
    }
}
NW_HOOK_METHOD(0x100a1d20, NCMagicSkillWnd, ShowWindow)

// 0x100a1da0 (vslot 45)
void NCMagicSkillWnd::HideWindow()
{
    ClearSkills();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x100a1da0, NCMagicSkillWnd, HideWindow)

// 0x100a1c80 (vslot 56) - the background over the whole window; C3: the clip is one pixel taller than the window
INT NCMagicSkillWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)(Height + 1.0f));
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, (INT)Width, (INT)Height, BackTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100a1c80, NCMagicSkillWnd, OnPaint)

// 0x100a1170
void NCMagicSkillWnd::ClearSkills()
{
    SkillWnd->ClearLists();
}
NW_HOOK_METHOD(0x100a1170, NCMagicSkillWnd, ClearSkills)

// a list's skills take the new name and description and make their tool tip again (repeated
// inline in the stock ReloadSkillTexts); the level is NItemEntry +0x191C
static FORCEINLINE void ReloadListTexts(NCSkillItemWnd* List)
{
    for (INT i = 0; i < List->Items.Num(); i++)
    {
        NItemEntry* Item = List->Items(i);
        if (!Item)
            continue;
        if (GConsole->GetSkillName(Item->ID, Item->UnknownC18))
            wcscpy(Item->Name, GConsole->GetSkillName(Item->ID, Item->UnknownC18));
        if (GConsole->GetSkillDesc(Item->ID, Item->UnknownC18))
            wcscpy(Item->Description, GConsole->GetSkillDesc(Item->ID, Item->UnknownC18));
        Item->FillToolTip(4, 0);
    }
}

// 0x100a1db0 - after the language changed
void NCMagicSkillWnd::ReloadSkillTexts()
{
    ReloadListTexts(SkillWnd->ActiveList);
    ReloadListTexts(SkillWnd->PassiveList);
}
NW_HOOK_METHOD(0x100a1db0, NCMagicSkillWnd, ReloadSkillTexts)

NW_CHECK_VTABLE(NCMagicSkillWnd, 0x1018e438, 68)

/*-----------------------------------------------------------------------------
    NCGMMagicSkillWnd
-----------------------------------------------------------------------------*/

// 0x10084cd0
NCGMMagicSkillWnd::NCGMMagicSkillWnd()
{
}
NW_HOOK_CTOR(0x10084cd0, NCGMMagicSkillWnd)

// 0x10084d20
NW_HOOK_DTOR(0x10084d20, NCGMMagicSkillWnd)

// 0x10119f60 (vslot 25) - NControl's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCGMMagicSkillWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x10119f60)
#else
NW_SLOT_IS("?WindowProc@NCGMMagicSkillWnd@@UAEHIIJ@Z", "?WindowProc@NControl@@UAEHIIJ@Z")   // NCGMMagicSkillWnd::WindowProc = NControl::WindowProc (0x10119f60)
#endif

// 0x10084d70 (vslot 37) - a title bar (20 high, the window is 256 wide), the GM's skill list under it and the
// background
INT NCGMMagicSkillWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_GMMAGICSKILL_FRAME;
    FrameCtrl->SetPosition(0, 0, 256, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    SkillWnd = new NCGMSkillWnd;
    SkillWnd->ID = NID_MAGICSKILL_LIST;
    SkillWnd->SetPosition(0, 20, 256, 335);
    CreateChild(SkillWnd, NWS_VISIBLE | 0x20);
    BackTex = FindTexture(TEXT("L2UI_CH3.SkillWnd.skill_back"));
    return 1;
}
NW_HOOK_METHOD(0x10084d70, NCGMMagicSkillWnd, OnCreate)

// 0x1007f880 (vslot 38)
INT NCGMMagicSkillWnd::OnDestroy()
{
    guard(NCGMMagicSkillWnd::OnDestroy);
    if (SkillWnd)
    {
        RemoveChildWindow(SkillWnd->Entry);
        delete SkillWnd;
        SkillWnd = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007f880, NCGMMagicSkillWnd, OnDestroy)

// 0x100412c0 (vslot 44) - NCEditBox's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED void NCGMMagicSkillWnd::ShowWindow() NW_STOCK(0x100412c0)
#else
NW_SLOT_IS("?ShowWindow@NCGMMagicSkillWnd@@UAEXXZ", "?ShowWindow@NCEditBox@@UAEXXZ")   // NCGMMagicSkillWnd::ShowWindow = NCEditBox::ShowWindow (0x100412c0)
#endif

// 0x1007f9d0 (vslot 45)
void NCGMMagicSkillWnd::HideWindow()
{
    ClearSkills();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x1007f9d0, NCGMMagicSkillWnd, HideWindow)

// 0x1007f930 (vslot 56) - the background under the title bar
INT NCGMMagicSkillWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 20, 256, 335, 0, 0, 256, 335, BackTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x1007f930, NCGMMagicSkillWnd, OnPaint)

// 0x1007e2e0
void NCGMMagicSkillWnd::ClearSkills()
{
    SkillWnd->ClearLists();
}
NW_HOOK_METHOD(0x1007e2e0, NCGMMagicSkillWnd, ClearSkills)

NW_CHECK_VTABLE(NCGMMagicSkillWnd, 0x101889d0, 68)
