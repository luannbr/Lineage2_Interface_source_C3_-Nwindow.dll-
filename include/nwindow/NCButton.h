#pragma once
// NCButton and its subclasses (UClass "CButton" 0x21C, "CExButton", "CPushButton",
// "CRadioButton").
//
// A button paints NormalTex, or PushedTex while bReleased is 0, stretched over its rectangle,
// with its label centered (a SysString index, or Text). Clicks play interface sound 0 (2 when
// disabled) and report NWM_COMMAND(ID, NotifyID) to the parent on release inside the button.
// The tool tip (ToolTipID, a SysString index) is drawn above the button, or below it, while
// the mouse is over it.
//   NCExButton     reports 1 on press and 0 on release instead.
//   NCPushButton   reports on press and, while held, again every RepeatDelay ms (timer).
//   NCRadioButton  stays down once clicked; reports Value.

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton : public NControl
{
    NW_DECLARE_CLASS(NCButton, NControl, 0, 0x10261450)

    UBOOL bReleased;           // +0x1D4 0 while pressed (paints PushedTex)
    UBOOL bMouseOver;          // +0x1D8 set by OnMouseMove, shows the tool tip
    UBOOL bToolTipAbove;       // +0x1DC 1 = tool tip above the button, 0 = below
    UTexture* NormalTex;       // +0x1E0
    UTexture* PushedTex;       // +0x1E4
    UTexture* ToolTipTex;      // +0x1E8 "L2UI.tooltip.mini-tool01", stretched under the tip
    INT NormalTexH;            // +0x1EC VSize of NormalTex
    INT NormalTexW;            // +0x1F0 USize
    INT PushedTexH;            // +0x1F4
    INT PushedTexW;            // +0x1F8
    UBOOL bAcceptKey;          // +0x1FC 1: Enter / Space click it while it has the focus
    NToolTipInfo ToolTip;      // +0x200 new in C2 (name ours): the tool tip's data, filled by SetToolTip; OnDrawToolTip
                               //   hands a copy of it to the window's tool tip skin (NCWnd::ToolTipSkin)
    FString Text;              // +0x27C label when TextID < 0 (C1: TCHAR Text[32])
    INT TextID;                // +0x288 SysString index of the label, -1 = Text
    INT ToolTipID;             // +0x28C SysString index of the tool tip, -1 = none
    INT TextOffsetX;           // +0x290 label offset from the center
    INT TextOffsetY;           // +0x294
    UBOOL bClickSound;         // +0x298 (name ours) new in C3: 1 (every constructor): the click sounds (OnLButtonDown, OnKeyDown,
                               //   NCPushButton::OnLButtonDown) play only while it is set

    NCButton();                                                                 // 0x10002940
    NCButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, UBOOL bInAcceptKey); // 0x10002a40
    // ~NCButton                                                                // 0x10002890

    virtual __declspec(noinline) INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam); // 25 0x100127a0
    virtual INT OnDestroy();                                                    // 38 0x10001180 (folded)
    virtual INT OnPaint(NCGDevice* Device);                                     // 56 0x10001b50
    virtual INT OnDrawToolTip(NCGDevice* Device);                               // 57 0x10002b70
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);                        // 59 0x10001210
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);                      // 61 0x100010c0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);                    // 62 0x10001000
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);                      // 63 0x10001070

    // names ours
    void SetTextID(INT ID);                                                     // 0x10001220 (C1 only: no C2 match)
    void SetText(const TCHAR* InText);                                          // 0x100012d0
    UBOOL SetNormalTex(const TCHAR* Name);                                      // 0x10001300
    UBOOL SetPushedTex(const TCHAR* Name);                                      // 0x10001350
    void SetToolTip(INT ID, UBOOL bAbove);                                      // 0x100013a0
    // 0x10001410 (ret 8; anonymous in the db): a tip with a text of its own (the party members' class icons), not a SysString
    void SetToolTipText(const TCHAR* InText, UBOOL bAbove);
};
NW_ASSERT_OFFSET(NCButton, bReleased, 0x1D4);
NW_ASSERT_OFFSET(NCButton, NormalTex, 0x1E0);
NW_ASSERT_OFFSET(NCButton, bAcceptKey, 0x1FC);
NW_ASSERT_OFFSET(NCButton, ToolTip, 0x200);
NW_ASSERT_OFFSET(NCButton, Text, 0x27C);
NW_ASSERT_OFFSET(NCButton, TextID, 0x288);
NW_ASSERT_OFFSET(NCButton, TextOffsetY, 0x294);
NW_ASSERT_OFFSET(NCButton, bClickSound, 0x298);
NW_ASSERT_SIZE(NCButton, 0x29C);

class NCExButton : public NCButton
{
    NW_DECLARE_CLASS(NCExButton, NCButton, 0, 0x10261970)

    NCExButton();                                                               // 0x10002c50
    NCExButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, UBOOL bInAcceptKey); // 0x10002cf0
    // ~NCExButton                                                              // 0x10002ca0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);             // 25 0x10041760 (folded)
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);                    // 62 0x10001440
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);                      // 63 0x10001480
};
NW_ASSERT_SIZE(NCExButton, 0x29C);

class NCPushButton : public NCButton
{
    NW_DECLARE_CLASS(NCPushButton, NCButton, 0, 0x10260A00)

    UBOOL bHeld;               // +0x29C pressed and still inside
    INT RepeatDelay;           // +0x2A0 ms between repeats (100)
    UBOOL bAutoRepeat;         // +0x2A4 (1)

    NCPushButton();                                                             // 0x10002df0
    NCPushButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName);       // 0x10002ea0
    // ~NCPushButton                                                            // 0x10002e50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);             // 25 0x10041760 (folded)
    virtual INT OnTimer(INT TimerID);                                           // 36 0x10001620
    virtual INT OnDestroy();                                                    // 38 0x10001650
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);                      // 61 0x100015b0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);                    // 62 0x100014f0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);                      // 63 0x10001570
};
NW_ASSERT_OFFSET(NCPushButton, bHeld, 0x29C);
NW_ASSERT_SIZE(NCPushButton, 0x2A8);

class NCRadioButton : public NCButton
{
    NW_DECLARE_CLASS(NCRadioButton, NCButton, 0, 0x10260F28)

    // C3 (the constructors 0x10002f00 / 0x10002fc0, OnCreate 0x10001990, OnLButtonDown 0x100016d0): Value is at +0x29C
    // (C2 +0x264), the size at OnCreate follows it; the C2 bToggle (+0x270) is gone: a click always flips bReleased
    INT Value;                 // +0x29C sent to the parent when the button is clicked (-1)
    INT CreateWidth;           // +0x2A0 (INT)Width at OnCreate
    INT CreateHeight;          // +0x2A4 (INT)Height at OnCreate

    NCRadioButton();                                                            // 0x10002f00
    NCRadioButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName);      // 0x10002fc0
    // ~NCRadioButton                                                           // 0x10002f70

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);             // 25 0x10041760 (folded)
    virtual INT OnCreate();                                                     // 37 0x10001990
    virtual INT OnDestroy();                                                    // 38 0x100019c0 (folded)
    virtual INT OnDrawToolTip(NCGDevice* Device);                               // 57 0x100021a0
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);                      // 61 0x10001730
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);                    // 62 0x100016d0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);                      // 63 0x10001710
};
NW_ASSERT_OFFSET(NCRadioButton, Value, 0x29C);
NW_ASSERT_OFFSET(NCRadioButton, CreateHeight, 0x2A4);
NW_ASSERT_SIZE(NCRadioButton, 0x2A8);

#pragma pack(pop)