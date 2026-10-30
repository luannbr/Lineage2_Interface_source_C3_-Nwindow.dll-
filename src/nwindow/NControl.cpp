/*=============================================================================
    NControl.cpp - base control. Stock: 0x10001810 .. 0x100018a0, 0x10004e90 .. 0x10005080.
=============================================================================*/
#include "nwindow/NControl.h"

#include <wchar.h>

// the stock code calls the CRT's wcscpy (0x100e78e8); the intrinsic would copy inline
#pragma function(wcscpy)

// 0x10002820 - NCWnd's constructor, then the control's own fields: no ID, no disable texture, an empty caption and
// NotifyID -1 (the stock constructors of the controls inline this)
NControl::NControl()
{
    ID = 0;
    DisableTexture = NULL;
    Caption[0] = 0;
    NotifyID = -1;
}
NW_HOOK_CTOR(0x10002820, NControl)

// 0x100027d0
NW_HOOK_DTOR(0x100027d0, NControl)

// 0x10119f60 (vslot 25) - C2: NCWnd's alone (C1 repeated its enabled test first)
INT NControl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x10119f60, NControl, WindowProc)

// 0x10005d60 (vslot 37)
INT NControl::OnCreate()
{
    NCWnd::OnCreate();
    if (!DisableTexture)
        DisableTexture = FindTexture(TEXT("NWindow.Disable"));
    return 1;
}
NW_HOOK_METHOD(0x10005d60, NControl, OnCreate)

// 0x10005d90 (vslot 38) - guarded; NCWnd's OnDestroy, then 1
INT NControl::OnDestroy()
{
    guard(NControl::OnDestroy);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10005d90, NControl, OnDestroy)

/*-----------------------------------------------------------------------------
    Parent communication (names ours).
-----------------------------------------------------------------------------*/

// 0x10005c40 - NWM_COMMAND(ID, Code) to the parent
UBOOL NControl::NotifyParent(LPARAM Code)
{
    NCWnd* Parent = ParentWnd();
    if (!Parent)
        return 0;
    Parent->WindowProc(NWM_COMMAND, ID, Code);
    return 1;
}
NW_HOOK_METHOD(0x10005c40, NControl, NotifyParent)

// 0x10005c80 - the parent takes the focus back
void NControl::ReleaseFocus()
{
    if (NCWnd* Parent = ParentWnd())
        Parent->SetFocus();
    KillFocus();
}
NW_HOOK_METHOD(0x10005c80, NControl, ReleaseFocus)

// 0x10005cb0
void NControl::HideParent()
{
    if (NCWnd* Parent = ParentWnd())
        Parent->HideWindow();
    KillFocus();
}
NW_HOOK_METHOD(0x10005cb0, NControl, HideParent)

// 0x10005ce0 - does not use this
UBOOL NControl::SendTo(NWndEntry* Target, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (!Target || !Target->Wnd)
        return 0;
    return Target->Wnd->WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x10005ce0, NControl, SendTo)

// 0x10005d20 - the parent's answer is dropped
UBOOL NControl::SendToParent(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    NCWnd* Parent = ParentWnd();
    if (!Parent)
        return 0;
    Parent->WindowProc(Msg, wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x10005d20, NControl, SendToParent)

// 0x10005e10 - the caption copied (nothing for a NULL text)
void NControl::SetCaption(const TCHAR* Text)
{
    if (Text)
        wcscpy(Caption, Text);
}
NW_HOOK_METHOD(0x10005e10, NControl, SetCaption)

// 0x10005e30 - NULL when empty
const TCHAR* NControl::GetCaption()
{
    const TCHAR* Text = Caption;
    if (!Text || !*Text)
        return NULL;
    return Text;
}
NW_HOOK_METHOD(0x10005e30, NControl, GetCaption)

NW_CHECK_VTABLE(NControl, 0x10176610, 69)
