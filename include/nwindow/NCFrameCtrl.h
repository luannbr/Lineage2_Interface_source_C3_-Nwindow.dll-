#pragma once
// NCFrameCtrl - title bar of a window (UClass "CFrameCtrl", 0x238) and NCResizeFrame, the
// edge that resizes a window (UClass "CResizeFrame", 0x254).
//
// Dragging the bar sends WM_MOVE(delta) to the parent, which moves (and snaps). A decorated
// frame paints its textures (the 0x14 high bar, or the small horizontal / vertical bars) and
// the title, and adds a minimize button (parent NWS_MINIMIZABLE) and a close button (unless
// NWS_NOCLOSE); NCWnd::OnCreate makes undecorated ones: an invisible 15 px drag area above
// windows with NWS_TITLEBAR.
//
// C3 layout (0x238), from the constructors 0x100102f0 / 0x10010450 and OnCreate / OnPaint /
// OnMouseMove / OnLButtonDown / SetTitleText: C2 had six dwords less in the texture list and
// kept the title position and the drag state at other offsets.

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

// NWM_COMMAND ids of the frame buttons
enum { NID_FRAME_MINIMIZE = 0xF0F0, NID_FRAME_CLOSE = 0xF0F1 };

class NCFrameCtrl : public NControl
{
    NW_DECLARE_CLASS(NCFrameCtrl, NControl, 0, 0x10266198)

    FString Title;             // +0x1D4 the window's title (FString: Num at +0x1D8 tells whether there is one)
    INT Unknown1E0;            // +0x1E0 (nothing writes or reads it)
    UTexture* BackLeftTex;     // +0x1E4 "L2UI_CH3.FrameCtrl.FrameBackLeft", 16x20
    UTexture* BackMidTex;      // +0x1E8 "L2UI_CH3.FrameCtrl.FrameBackMid", stretched (the source is 32x20)
    UTexture* BackRightTex;    // +0x1EC "L2UI_CH3.FrameCtrl.FrameBackRight", 16x20
    NCButton* CloseButton;     // +0x1F0
    NCButton* MinimizeButton;  // +0x1F4
    UTexture* SmallBarTex[6];  // +0x1F8 "L2UI_CH3.FrameCtrl.smallbar_h1" .. "_h3" (the 8x12 horizontal bar: [0] and
                               //   [2] are its two caps, [1] the middle) then "smallbar1" .. "smallbar3" (the 12x8
                               //   vertical one: [3] top, [4] middle, [5] bottom)
    INT LastMouseX;            // +0x210 mouse position of the drag
    INT LastMouseY;            // +0x214
    INT TitleX;                // +0x218 the title's position; 0, 0 until OnPaint measures it (SetTitleText clears it)
    INT TitleY;                // +0x21C
    INT bVertical;             // +0x220 (name ours) constructor argument 2: the small bar is the vertical one
    INT bSmall;                // +0x224 (name ours) constructor argument 3: the small bar instead of the title bar
    INT GrabX;                 // +0x228 where the drag grabbed the bar (NCWnd::Snap)
    INT GrabY;                 // +0x22C
    INT bDecorated;            // +0x230 constructor argument 1 (1 for the default constructor): textures and buttons
    INT bMoved;                // +0x234 (name ours) the last mouse move of the drag moved the window

    NCFrameCtrl();                                                   // 0x100102f0 (bDecorated 1)
    // 0x10010450: A is bDecorated, B bVertical, C bSmall (the C2 client had the one-argument constructor only)
    NCFrameCtrl(INT InDecorated, INT InVertical = 0, INT InSmall = 0);
    // ~NCFrameCtrl                                                  // 0x10010250

    virtual __declspec(noinline) INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1000f6d0
    virtual INT OnCreate();                                          // 37 0x100100b0
    virtual INT OnDestroy();                                         // 38 0x1000faa0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1000fb70
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x1000fed0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x1000f840
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1000f8a0
    virtual void SetFrameSize(DWORD W, DWORD H);                     // 69 0x1000f770 (name ours)
    virtual INT OnClose();                                           // 70 0x1000fa10 (name ours)
    virtual INT OnMinimize();                                        // 71 0x1000f750 (name ours)

    void SetTitleText(const TCHAR* InTitle);                         // 0x1000f8d0 (name ours)
    // 0x1000f820 (ret 8; anonymous in the db, name ours): the bar's shape, constructor arguments 2 and 3 after the fact (the
    // shortcut window turns its bar vertical and small as the window is laid out)
    void SetBarShape(INT InVertical, INT InSmall);
};
NW_ASSERT_OFFSET(NCFrameCtrl, Title, 0x1D4);
NW_ASSERT_OFFSET(NCFrameCtrl, BackLeftTex, 0x1E4);
NW_ASSERT_OFFSET(NCFrameCtrl, CloseButton, 0x1F0);
NW_ASSERT_OFFSET(NCFrameCtrl, MinimizeButton, 0x1F4);
NW_ASSERT_OFFSET(NCFrameCtrl, SmallBarTex, 0x1F8);
NW_ASSERT_OFFSET(NCFrameCtrl, LastMouseX, 0x210);
NW_ASSERT_OFFSET(NCFrameCtrl, TitleX, 0x218);
NW_ASSERT_OFFSET(NCFrameCtrl, bVertical, 0x220);
NW_ASSERT_OFFSET(NCFrameCtrl, GrabX, 0x228);
NW_ASSERT_OFFSET(NCFrameCtrl, bDecorated, 0x230);
NW_ASSERT_OFFSET(NCFrameCtrl, bMoved, 0x234);
NW_ASSERT_SIZE(NCFrameCtrl, 0x238);

// resize direction: which mouse delta goes to the parent (NWM_COMMAND, lParam = delta)
enum { RESIZE_Horizontal = 1, RESIZE_Vertical = 2 };

class NCResizeFrame : public NCFrameCtrl
{
    NW_DECLARE_CLASS(NCResizeFrame, NCFrameCtrl, 0, 0x102666B8)

    INT ResizeGrabX;           // +0x238 where the edge was grabbed, relative to the window (OnLButtonDown)
    INT ResizeGrabY;           // +0x23C
    UBOOL bPressed;            // +0x240 cleared with bDragging, never read here
    INT Direction;             // +0x244 RESIZE_*
    INT Unknown248;            // +0x248 (the constructor clears it; nothing reads it)
    UBOOL bDragging;           // +0x24C button went down inside
    UBOOL bCursorLocked;       // +0x250 resize cursor shown and locked

    NCResizeFrame(INT InDirection = 0);                              // 0x100105c0
    // ~NCResizeFrame                                                // 0x10010640

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1000f900
    virtual void KillFocus();                                        // 30 0x1000f9f0
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x1000ff70
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x1000f920
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1000f9c0
};
NW_ASSERT_OFFSET(NCResizeFrame, ResizeGrabX, 0x238);
NW_ASSERT_OFFSET(NCResizeFrame, Direction, 0x244);
NW_ASSERT_OFFSET(NCResizeFrame, bCursorLocked, 0x250);
NW_ASSERT_SIZE(NCResizeFrame, 0x254);

#pragma pack(pop)
