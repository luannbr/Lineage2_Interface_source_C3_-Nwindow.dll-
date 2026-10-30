#pragma once
// NCStatusBarCtrl - the gauge of the player's status window ("ps_hpbar": HP, MP, CP; UClass "CStatusBarCtrl", 0x244;
// new in C3, the C2 window drew its bars by hand): a back, a fill as wide as Cur / MaxValue of the control, a lighter
// "ghost" strip after it for the amount just lost (a timer shrinks it) and an overlay; the label at the left and
// "Cur / MaxValue" in the middle. Cur follows Target by timer 0xFFF002 (the status window sets Target and starts it),
// the ghost's width by timer 0xFFF001.
//
// Layout from the constructor 0x101024d0, OnPaint 0x100fb3a0 and OnTimer 0x100fa3a0 (names ours).

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

enum
{
    NTIMER_STATUSBAR_GHOST = 0xFFF001,   // the ghost strip shrinks (name ours)
    NTIMER_STATUSBAR_VALUE = 0xFFF002,   // Cur moves towards Target (name ours)
};

class NCStatusBarCtrl : public NControl
{
    NW_DECLARE_CLASS(NCStatusBarCtrl, NControl, 0, 0x102B16D0)

    UTexture* FillTex;               // +0x1D4 the fill
    UTexture* BackTex;               // +0x1D8 the empty gauge
    UTexture* GhostTex;              // +0x1DC drawn at half strength after the fill (the amount just lost)
    UTexture* OverlayTex;            // +0x1E0 drawn over the fill when bOverlay
    INT TexWidth;                    // +0x1E4 the textures' size (the source rectangle of every draw)
    INT TexHeight;                   // +0x1E8
    INT MaxValue;                    // +0x1EC the full gauge
    INT Cur;                         // +0x1F0 what the fill shows
    INT Target;                      // +0x1F4 what Cur moves to
    DOUBLE GhostScale;               // +0x1F8 starts at the ghost's timer; OnTimer 0xFFF001 lowers it by Rate * 0.666
    INT Rate;                        // +0x200
    DOUBLE GhostStep;                // +0x204 taken off the ghost's width on every tick of the ghost's timer
    DOUBLE Ghost;                    // +0x20C the ghost strip's amount (0: none; the timer is killed when it is spent)
    INT Unknown214;                  // +0x214 (0) nothing here reads it
    DOUBLE LastTime;                 // +0x218 appSeconds of the last StartAnim / UpdateTrail (name ours)
    UBOOL bOverlay;                  // +0x220 SetValues: set while Cur is below 3/5 of the maximum (the low warning)
    UBOOL bGhost;                    // +0x224 the ghost strip is drawn
    UBOOL bGrow;                     // +0x228 Cur goes up to Target (else down)
    UBOOL bReached;                  // +0x22C Cur is Target (the value timer is done)
    INT GhostTimer;                  // +0x230 timer id of the ghost's, 0 none
    INT ValueTimer;                  // +0x234 timer id of the value's, 0 none
    FString Text;                    // +0x238 the label drawn at (9, 2)

    NCStatusBarCtrl();                                               // 0x101024d0
    // the status window's (ret 0x1c): the label, the textures' size, the fill and back textures, the ghost and the
    // overlay ones (or NULL for none)
    NCStatusBarCtrl(const TCHAR* InLabel, INT InTexW, INT InTexH, const TCHAR* FillName, const TCHAR* BackName,
                    const TCHAR* GhostName, const TCHAR* OverlayName); // 0x101025e0
    // ~NCStatusBarCtrl                                              // 0x10102570 (the label, then the control)

    // names ours
    void SetValues(INT InCur, INT InMax);                            // 0x100ff4b0 (ret 8) the player status window's
    __declspec(noinline) void ClearTrail();                          // 0x100fa370 no strip left (out of line, as SetValues calls it)
    __declspec(noinline) void UpdateTrail();                         // 0x100fb7c0 the strip shortened by the time passed

    virtual INT OnTimer(INT TimerID);                                // 36 0x100fa3a0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100fb3a0
};
NW_ASSERT_OFFSET(NCStatusBarCtrl, FillTex, 0x1D4);
NW_ASSERT_OFFSET(NCStatusBarCtrl, MaxValue, 0x1EC);
NW_ASSERT_OFFSET(NCStatusBarCtrl, GhostScale, 0x1F8);
NW_ASSERT_OFFSET(NCStatusBarCtrl, Rate, 0x200);
NW_ASSERT_OFFSET(NCStatusBarCtrl, GhostStep, 0x204);
NW_ASSERT_OFFSET(NCStatusBarCtrl, Ghost, 0x20C);
NW_ASSERT_OFFSET(NCStatusBarCtrl, LastTime, 0x218);
NW_ASSERT_OFFSET(NCStatusBarCtrl, bOverlay, 0x220);
NW_ASSERT_OFFSET(NCStatusBarCtrl, GhostTimer, 0x230);
NW_ASSERT_OFFSET(NCStatusBarCtrl, Text, 0x238);
NW_ASSERT_SIZE(NCStatusBarCtrl, 0x244);

#pragma pack(pop)
