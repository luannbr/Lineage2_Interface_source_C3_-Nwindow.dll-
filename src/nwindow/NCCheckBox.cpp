/*=============================================================================
    NCCheckBox.cpp - check box. Stock (C2): 0x10004530 .. 0x10004b90.
=============================================================================*/
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCGDevice.h"

#include <wchar.h>

// 0x100049e0 - the label by text (an empty text leaves it empty), a 12 x 12 box; C3 has a fourth argument, DrawText's
// fifth, which the label is drawn with. NControl's constructor is inlined in the stock one (the same stores); the
// disabled textures are left as allocated
NCCheckBox::NCCheckBox(const TCHAR* InLabel, UBOOL bInChecked, UBOOL bInLabelLeft, INT InTextParam)
{
    if (InLabel && *InLabel)
        Label = InLabel;
    BoxW = 12;
    BoxH = 12;
    CheckedTex = NULL;
    UncheckedTex = NULL;
    bDisabled = 0;
    bChecked = bInChecked;
    bLabelLeft = bInLabelLeft;
    TextParam = InTextParam;
}
NW_HOOK_CTOR_ARGS(0x100049e0, NCCheckBox, (const TCHAR* A, UBOOL B, UBOOL C, INT D), (A, B, C, D))

// 0x10004b50 - the label by system string (the text "Check Box" when there is none); TextParam is left as allocated
NCCheckBox::NCCheckBox(INT LabelID, UBOOL bInChecked, UBOOL bInLabelLeft)
{
    const TCHAR* Text = *SysString(LabelID);
    Label = Text ? Text : TEXT("Check Box");
    BoxW = 12;
    BoxH = 12;
    bLabelLeft = bInLabelLeft;
    CheckedTex = NULL;
    UncheckedTex = NULL;
    bDisabled = 0;
    bChecked = bInChecked;
}
NW_HOOK_CTOR_ARGS(0x10004b50, NCCheckBox, (INT A, UBOOL B, UBOOL C), (A, B, C))

// 0x10004ae0 - the label is an FString (destroyed with the object)
NW_HOOK_DTOR(0x10004ae0, NCCheckBox)


// 0x10090b60 (vslot 25) - the stock body is NCButton's
INT NCCheckBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10004600 (vslot 37) - NControl's is not called: no Disable texture
INT NCCheckBox::OnCreate()
{
    CheckedTex = FindTexture(TEXT("L2UI.Control.CheckBox_Checked"));
    UncheckedTex = FindTexture(TEXT("L2UI.Control.CheckBox"));
    UncheckedDisabledTex = FindTexture(TEXT("L2UI.Control.CheckBox_Unable"));
    CheckedDisabledTex = FindTexture(TEXT("L2UI.Control.CheckBox_Checked_unable"));
    return 1;
}
NW_HOOK_METHOD(0x10004600, NCCheckBox, OnCreate)

// 0x100046b0 (vslot 38) - guarded in C2 and C3 (C1: NCButton's body)
INT NCCheckBox::OnDestroy()
{
    guard(NCCheckBox::OnDestroy);
    NControl::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100046b0, NCCheckBox, OnDestroy)

// 0x10004760 (vslot 56) - the box (the texture of its state: checked or not, disabled or not) and the label on its
// right, or its left (after the label's width and 4 pixels). C3: the label is light grey (grey when disabled) and is
// drawn with TextParam
INT NCCheckBox::OnPaint(NCGDevice* Device)
{
    guard(NCCheckBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    UTexture* Box;
    if (bDisabled)
        Box = bChecked ? CheckedDisabledTex : UncheckedDisabledTex;
    else
        Box = bChecked ? CheckedTex : UncheckedTex;
    DWORD Color = bDisabled ? 0xFF808080 : 0xFFDCDCDC;
    if (bLabelLeft)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, *Label, 0, 0, 0, 0, 0, L2FONT_Normal);
        Device->DrawTexture(Size.cx + 4, 0, BoxW, BoxH, 0, 0, BoxW, BoxH, Box, 0xFF, 1);
        Device->DrawText(0, 0, Color, *Label, TextParam, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }
    else
    {
        Device->DrawTexture(0, 0, BoxW, BoxH, 0, 0, BoxW, BoxH, Box, 0xFF, 1);
        Device->DrawText(BoxW + 4, 0, Color, *Label, TextParam, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10004760, NCCheckBox, OnPaint)

// 0x10004730 (vslot 62)
INT NCCheckBox::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (!bDisabled)
    {
        bChecked = !bChecked;
        NotifyParent(bChecked);
    }
    return 1;
}
NW_HOOK_METHOD(0x10004730, NCCheckBox, OnLButtonDown)

NW_CHECK_VTABLE(NCCheckBox, 0x10177408, 69)
