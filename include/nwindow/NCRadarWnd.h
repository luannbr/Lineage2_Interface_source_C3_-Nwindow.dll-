#pragma once
// NCRadarWnd - the radar (UClass "CRadarWnd", 0xA0): the canvas draws the radar itself
// (NCGDevice::DrawRadar) and a mark per target. The console fills Targets by calling
// NCMinimapWnd::AddTarget / RemoveTarget on this window (the same array, at the same place),
// as it does for the map window.
//
// Desktop (NCConsole) +0x10C is the radar window.

#include "nwindow/NCMinimapWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCRadarWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCRadarWnd, NCWnd, 0, 0x102A2F30)

    TArray<FMinimapTarget> Targets;  // +0xC8 world coordinates

    NCRadarWnd();                                                    // 0x100de840
    // ~NCRadarWnd                                                   // 0x100de8a0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x10046c20
    virtual INT OnDestroy();                                         // 38 0x100de6e0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100de5e0

    __declspec(noinline) void ClearTargets();                        // 0x1009a540 (C1 only: no C2 match) (C1 0x100a1d70) (name ours)
    // the map window's, folded (the console calls them on both windows)
    void AddTarget(INT X, INT Y, INT Z);                             // 0x10079230 (C1 only: no C2 match) (C1 only: no C2 match)
    void RemoveTarget(INT X, INT Y, INT Z);                          // 0x1009a4d0 (C1 only: no C2 match)
};
NW_ASSERT_OFFSET(NCRadarWnd, Targets, 0xC8);
NW_ASSERT_SIZE(NCRadarWnd, 0xD4);

#pragma pack(pop)