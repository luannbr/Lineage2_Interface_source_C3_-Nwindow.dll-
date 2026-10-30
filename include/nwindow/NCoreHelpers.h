#pragma once
// NCoreHelpers.h - the stand-in classes of src/nwindow/NCoreHelpers.cpp that other files call (declared once here:
// docs/notas/C3_Standalone.md section 2b).

#include "nwindow/NControl.h"

// The control of the messenger's and the broadcast GM window's title bars (MessengerWnd::OnPaint / MinimizeWindow,
// NCBroadcastGMWnd::MinimizeWindow): its fields from +0x1D4 (the picture; the text at +0x1E0; the flag at +0x1F4). The
// function is its setter; a stand-in class so that it is hooked as a method (thiscall)
struct NCoreSkinControl : public NControl
{
    void SetSkin(const TCHAR* TexName, const TCHAR* Text, ANSICHAR Flag);   // 0x1001bdd0 (ret 0xc)
};
