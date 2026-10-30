#pragma once
// NCProgressCtrl - count-down bar (UClass "CProgressCtrl", 0x1C0). While running, each paint
// adds the frame time (ms) to Pos; the bar shrinks from full to empty, and at Max the control
// stops and reports NWM_COMMAND(ID, 0) to the parent.

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCProgressCtrl : public NControl
{
    NW_DECLARE_CLASS(NCProgressCtrl, NControl, 0, 0x102647D8)

    UTexture* BarTex;          // +0x1D4 "NWindow.WhiteTexture" by default
    UTexture* BackTex;         // +0x1D8 "NWindow.BlackTexture" by default
    INT Unknown1A8;            // +0x1DC (1)
    INT Border;                // +0x1E0 bar inset (2)
    UBOOL bStopped;            // +0x1E4 (1: created stopped)
    DWORD Pos;                 // +0x1E8 ms elapsed
    DWORD Max;                 // +0x1EC ms
    INT Unknown1BC;            // +0x1F0   // C2: offset derived, no evidence   // C2: offset derived, no evidence

    NCProgressCtrl();                                                        // 0x100067d0
    NCProgressCtrl(const TCHAR* BackTexName, const TCHAR* BarTexName, INT InBorder); // 0x10006910
    // ~NCProgressCtrl                                                       // 0x100068c0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);          // 25 0x100127a0 (folded)
    virtual INT OnCreate() { return 1; }                                     // 37 0x10005fa0 (folded)
    virtual INT OnDestroy() { return 1; }                                    // 38 0x10005fa0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                                  // 56 0x10006240

    // names ours
    void SetRemaining(DWORD Remaining);                                      // 0x10005fb0
    void SetMax(DWORD InMax);                                                // 0x10005fd0
    void Reset();                                                            // 0x10005fe0
    void Stop();                                                             // 0x10006000
    void Start();                                                            // 0x10006010
};
NW_ASSERT_OFFSET(NCProgressCtrl, Border, 0x1E0);
NW_ASSERT_OFFSET(NCProgressCtrl, Max, 0x1EC);
NW_ASSERT_SIZE(NCProgressCtrl, 0x1F4);

#pragma pack(pop)