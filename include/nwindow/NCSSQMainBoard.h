#pragma once
// NCSSQMainBoard - the Seven Signs window (UClass "CSSQMainBoard", 0xE0; new in C3): three tabs (the status, the event
// list and the seal list: NCSSQStatusWnd, NCSSQMainEventWnd, NCSSQSealStatusListWnd, each 256 x 353 under a 20 pixel
// title bar) on the "SSQ_back" panel. The first visit of the event and seal tabs asks the console for their lists
// (once, until the window is hidden).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCSSQMainEventWnd;
class NCSSQSealStatusListWnd;
class NCSSQStatusWnd;
class NCTabControl;

class NCSSQMainBoard : public NCWnd
{
    NW_DECLARE_CLASS(NCSSQMainBoard, NCWnd, 0, 0x102A9B48)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.SSQWnd.SSQ_back"
    NCTabControl* TabCtrl;           // +0xCC id 0xFFFA0FF (names ours)
    NCSSQStatusWnd* StatusWnd;       // +0xD0
    NCSSQMainEventWnd* EventWnd;     // +0xD4
    NCSSQSealStatusListWnd* SealWnd; // +0xD8
    BYTE bEventAsked;                // +0xDC the event list was asked for (name ours)
    BYTE bSealAsked;                 // +0xDD the seal list was asked for (name ours)

    NCSSQMainBoard();                                                // 0x100E9E10
    // ~NCSSQMainBoard                                               // 0x100E9E60

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100E9070
    virtual INT OnCreate();                                          // 37 0x100E9EB0
    virtual INT OnDestroy();                                         // 38 0x100E9630 (db: NCMainWnd::OnDestroy)
    virtual void ShowWindow();                                       // 44 0x100E90D0
    virtual void HideWindow();                                       // 45 0x100E9100
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100E9760
    virtual INT OnTab(INT Index);                                    // 68 0x100E9140 (name ours)
};
NW_ASSERT_OFFSET(NCSSQMainBoard, TabCtrl, 0xCC);
NW_ASSERT_OFFSET(NCSSQMainBoard, SealWnd, 0xD8);
NW_ASSERT_OFFSET(NCSSQMainBoard, bEventAsked, 0xDC);
NW_ASSERT_SIZE(NCSSQMainBoard, 0xE0);

#pragma pack(pop)
