/*=============================================================================
    NCKeyCryptWarningDialogBox.cpp - the keyboard protection warning (C2 only). Stock:
    0x100827a0 .. 0x100827f5 (WindowProc), 0x10082800 .. 0x1008280a and 0x10082ce0 .. 0x10083037.
=============================================================================*/
#include "nwindow/NCKeyCryptWarningDialogBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

enum { SYSMSG_KeyCryptWarning = 1239 };   // the text of the box (name ours)

// 0x100930e0 - NCDialogBox's constructor, the picture and the button cleared
NCKeyCryptWarningDialogBox::NCKeyCryptWarningDialogBox()
{
    BackTex = NULL;
    OKButton = NULL;
}
NW_HOOK_CTOR(0x100930e0, NCKeyCryptWarningDialogBox)

// 0x10093130
NW_HOOK_DTOR(0x10093130, NCKeyCryptWarningDialogBox)

// 0x10091f80 (vslot 25) - the stock body is shared with NCWarningDialogBox's
INT NCKeyCryptWarningDialogBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCDialogBox::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_DIALOG_OK)
        return OnOK() != 0;
    return 0;
}
NW_HOOK_METHOD(0x10091f80, NCKeyCryptWarningDialogBox, WindowProc)

// 0x10091fe0 (vslot 69) - OK closes the box
INT NCKeyCryptWarningDialogBox::OnOK()
{
    HideModal();
    return 1;
}
NW_HOOK_METHOD(0x10091fe0, NCKeyCryptWarningDialogBox, OnOK)

// 0x10092c30 (vslot 37) - the OK button (74 x 21, 60 pixels down) centered in the box's local width
INT NCKeyCryptWarningDialogBox::OnCreate()
{
    NCDialogBox::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_ch3.dialog.system_back"));
    NRectF Local = { 0, 0, 0, 0 };
    GetLocalRect(&Local);

    OKButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);   // NCButton's constructor 0x10002a40
    OKButton->ID = NID_DIALOG_OK;
    OKButton->SetPosition((INT)(Local.W * 0.5f - 37.0f), 60, 74, 21);
    OKButton->SetText(*SysString(140));                            // "OK"
    CreateChild(OKButton, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10092c30, NCKeyCryptWarningDialogBox, OnCreate)

// 0x10092400 (vslot 38) - guarded in C2
INT NCKeyCryptWarningDialogBox::OnDestroy()
{
    guard(NCKeyCryptWarningDialogBox::OnDestroy);
    if (OKButton)
    {
        RemoveChildWindow(OKButton->Entry);
        delete OKButton;
        OKButton = NULL;
    }
    NCDialogBox::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10092400, NCKeyCryptWarningDialogBox, OnDestroy)

// 0x100924b0 (vslot 56) - the popup picture (a 205 x 87 source) and the system message at (10, 10), wrapped 20
// pixels short of the width
INT NCKeyCryptWarningDialogBox::OnPaint(NCGDevice* Device)
{
    guard(NCKeyCryptWarningDialogBox::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, 205, 87, BackTex);
    Device->DrawText(10, 10, 0xFFDCDCDC, *GL2GameData.SystemMsg[SYSMSG_KeyCryptWarning].Message,
                     (DWORD)(Width - 20.0f), 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100924b0, NCKeyCryptWarningDialogBox, OnPaint)

NW_CHECK_VTABLE(NCKeyCryptWarningDialogBox, 0x1018bb08, 70)
