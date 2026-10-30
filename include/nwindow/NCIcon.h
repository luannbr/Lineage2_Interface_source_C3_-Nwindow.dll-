#pragma once
// NCIcon - the 32x32 icon a minimized window leaves on the screen (UClass "CIcon", 0x2C0).
// NCWnd::OnCreate creates one for NWS_MINIMIZABLE windows; NCWnd::Minimize picks its texture
// and tool tip. It can be dragged (WM_MOVE to itself, sticky like windows); a click without a
// drag restores the window.

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCIcon : public NControl
{
    NW_DECLARE_CLASS(NCIcon, NControl, 0, 0x1026C540)

    UTexture* IconTex;         // +0x1D4
    INT DragX;                 // +0x1D8 where the drag grabbed the icon (NCWnd::Snap)
    INT DragY;                 // +0x1DC
    FString Name;              // +0x1E0 the tool tip text (C1: TCHAR Name[128])
    BYTE bMouseOver;           // +0x1EC
    INT Unknown1C0;            // +0x1F0 (-1; only the constructor writes it; C1 name Unknown2B0)   // C2: offset derived, no evidence
    UBOOL bToolTipAbove;       // +0x1F4 (SetIcon)
    UTexture* ToolTipTex;      // +0x1F8 "NWindow.ChatBack"   // C2: offset derived, no evidence
    UBOOL bDragged;            // +0x1FC moved since the button went down

    NCIcon();                                                        // 0x1001c460
    // ~NCIcon                                                       // 0x10019180 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1001bd70
    virtual void HideWindow();                                       // 45 0x1001bfc0 (C3)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1001bfe0
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x1001c120
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x1001be80
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x1001bf30
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1001bf70
    // slot 68, new in C2 (name ours; WM_LBUTTONDBLCLK goes there): the stock body is `mov eax, 1; ret 8`
    // folded with the other classes' (0x100d2e70)
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam) { return 1; }  // 69 0x1000bd60 (folded)

    void SetIcon(const TCHAR* TexName, INT NameID, BYTE bAbove);     // 0x1001be20 (name ours)
};
NW_ASSERT_OFFSET(NCIcon, Name, 0x1E0);
NW_ASSERT_OFFSET(NCIcon, bMouseOver, 0x1EC);
NW_ASSERT_OFFSET(NCIcon, bToolTipAbove, 0x1F4);
NW_ASSERT_OFFSET(NCIcon, bDragged, 0x1FC);
NW_ASSERT_SIZE(NCIcon, 0x200);

#pragma pack(pop)