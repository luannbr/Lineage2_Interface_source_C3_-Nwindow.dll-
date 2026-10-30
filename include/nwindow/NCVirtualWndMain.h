#pragma once
// NCAbstractMain / NCVirtualWndMain - owner of every window: registration, z-order lists,
// hit tests, focus, message dispatch, cursors (UClass "CAbstractMain", "CVirtualWndMain").
//
// Input path: the console calls DispatchWndMsg with the Win32 message; mouse coordinates are
// made screen-relative (ClientToScreenPacked), then the top-level windows are tried from the
// front (TopLevel is painted from index 0, so the last one is on top): clicks go to the deepest
// window hit (RouteClick, which also gives it the focus), mouse moves to the one under the
// cursor or to the focused one while the left button is held (RouteMouseMove), keyboard and IME
// messages to the focused window (SendToFocused). WM_PAINT paints each tree back to front and
// then lets the window under the cursor draw its tool tip.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class UTexture;

class NCAbstractMain : public NCObject
{
    NW_DECLARE_ABSTRACT_CLASS(NCAbstractMain, NCObject, 0, 0x10271AE8)
    // constructor: NCObject's (0x10020ca0, identical code folded); ~NCAbstractMain 0x10024690

    virtual NWndEntry* RegisterNWindow(NCWnd* Wnd, DWORD Style, NWndEntry* Parent) = 0;  // 25
    virtual void UnRegisterNWindow(NWndEntry* Entry) = 0;                                 // 26
    virtual NWndEntry* FindNWindow(INT X, INT Y) = 0;                                     // 27
    virtual void ClearFocus() = 0;                                                        // 28
    virtual void AddTopLevel(NWndEntry* Entry) = 0;                                       // 29 (name ours)
    // new in C2 (names ours; bodies 0x1002ef60 / 0x1002ef90 / 0x1002d410): a stack of windows that took the
    // focus for themselves (the array at +0x48 of NCVirtualWndMain): the last one gets the focus back when
    // another leaves
    virtual void PushFocusWindow(NWndEntry* Entry) = 0;                                   // 30
    virtual void PopFocusWindow(NWndEntry* Entry) = 0;                                    // 31
    virtual INT NumFocusWindows() = 0;                                                    // 32
};
NW_ASSERT_SIZE(NCAbstractMain, 0x28);

class NCVirtualWndMain : public NCAbstractMain
{
    NW_DECLARE_CLASS(NCVirtualWndMain, NCAbstractMain, 0, 0x10272008)

    NCGDevice* GDevice;              // +0x28 created by the constructor
    DWORD NumWindows;                // +0x2C registered windows (RegisterNWindow refuses 1000+)
    TArray<NWndEntry*> Windows;      // +0x30 every registered window
    TArray<NWndEntry*> TopLevel;     // +0x3C windows without a parent, back to front
    TArray<NWndEntry*> FocusWindows; // +0x48 new in C2 (name ours): the stack PushFocusWindow / PopFocusWindow keep
    TArray<NWndEntry*> TopMost;      // +0x54 windows registered with NWS_TOPMOST
    HCURSOR Cursors[7];              // +0x60 from this package's resources (LoadCursors)
    HCURSOR ArrowCursor;             // +0x7C IDC_ARROW
    INT CursorType;                  // +0x80 index in Cursors, 4 = color cursor, -1 = arrow
    UBOOL bCursorEnabled;            // +0x84
    HCURSOR ColorCursor;             // +0x88 built from a texture (CreateColorCursor)
    UBOOL bColorCursor;              // +0x8C ColorCursor is ours to destroy
    UBOOL bMouseMoveTaken;           // +0x90 a window already took this WM_MOUSEMOVE
    UBOOL bToolTipShown;             // +0x94 a window already drew its tool tip this frame
    UBOOL bMouseOnWindow;            // +0x98 (new in C3, name ours) the cursor is over a window that took the move: RouteMouseMove sets it
                                     //   with bMouseMoveTaken (not for the desktop), a drag clears it; NConsoleWnd::UpdateCursor
                                     //   shows the normal cursor while it is set

    NCVirtualWndMain();                                                    // 0x10034db0
    // ~NCVirtualWndMain                                                   // 0x10034ef0

    virtual NWndEntry* RegisterNWindow(NCWnd* Wnd, DWORD Style, NWndEntry* Parent); // 25 0x10034310
    virtual void UnRegisterNWindow(NWndEntry* Entry);                      // 26 0x10034450
    virtual NWndEntry* FindNWindow(INT X, INT Y);                          // 27 0x10033d10
    virtual void ClearFocus();                                             // 28 0x10033be0
    virtual void AddTopLevel(NWndEntry* Entry);                            // 29 0x100342e0 (name ours)
    virtual void PushFocusWindow(NWndEntry* Entry);                        // 30 0x10034d20 (name ours)
    virtual void PopFocusWindow(NWndEntry* Entry);                         // 31 0x10034d50 (name ours)
    virtual INT NumFocusWindows();                                         // 32 0x100330e0 (name ours)
    virtual NWndEntry* FindChildWindowAt(NCWnd* Wnd, INT X, INT Y);        // 33 0x10033c80 (name ours)

    // names ours unless marked
    UBOOL IsDispatchedMsg(UINT Msg);                                       // 0x10032d30
    void SetGDevice(UCanvas* Canvas);                                      // 0x10032d60 (guard name)
    void ReleaseGDevice();                                                 // 0x10032de0 (guard name)
    LPARAM ClientToScreenPacked(LPARAM Pos);                               // no C2 function: DispatchWndMsg inlines it (C1 0x100243c0)
    LPARAM CoordCompensate(LPARAM Pos);                                    // 0x100331b0 (guard name)
    INT DispatchToFocusWindow(NWndEntry* Entry, UINT Msg, WPARAM wParam, LPARAM lParam); // 0x10033820 (name ours)
    UBOOL GetMousePos(INT* X, INT* Y);                                     // 0x10034210 (name ours)
    INT SendToFocused(NWndEntry* Entry, UINT Msg, WPARAM wParam, LPARAM lParam); // 0x10032e60
    void PaintChildren(NWndEntry* Entry);                                  // 0x1002d2e0 (C1 only: no C2 match)
    void LoadCursors();                                                    // 0x10032ff0
    void DestroyCursors();                                                 // 0x10033070
    void DeleteGDevice();                                                  // 0x100330c0
    UBOOL HitTestClick(NWndEntry* Entry, INT X, INT Y);                    // 0x10033280
    INT RouteClick(NWndEntry* Entry, INT X, INT Y, UINT Msg, WPARAM wParam, LPARAM lParam); // 0x100333f0
    UBOOL HitTest(NWndEntry* Entry, LPARAM Pos);                           // 0x10033540
    INT RouteMouseMove(NWndEntry* Entry, UINT Msg, WPARAM wParam, LPARAM lParam); // 0x100335d0
    void PaintToolTips(NWndEntry* Entry);                                  // 0x100345d0 (name ours)
    INT DispatchWndMsg(UINT Msg, WPARAM wParam, LPARAM lParam);            // 0x100346f0 (guard name)
    void CreateColorCursor(UTexture* Texture);                             // 0x10033e00
    void SetCursorType(INT Type, UTexture* Texture);                       // 0x10034190
    UBOOL IsOverWindow(INT X, INT Y);                                      // 0x10034260
};
NW_ASSERT_OFFSET(NCVirtualWndMain, Windows, 0x30);
NW_ASSERT_OFFSET(NCVirtualWndMain, TopLevel, 0x3C);
NW_ASSERT_OFFSET(NCVirtualWndMain, FocusWindows, 0x48);
NW_ASSERT_OFFSET(NCVirtualWndMain, TopMost, 0x54);
NW_ASSERT_OFFSET(NCVirtualWndMain, Cursors, 0x60);
NW_ASSERT_OFFSET(NCVirtualWndMain, ArrowCursor, 0x7C);
NW_ASSERT_OFFSET(NCVirtualWndMain, ColorCursor, 0x88);
NW_ASSERT_OFFSET(NCVirtualWndMain, bToolTipShown, 0x94);
NW_ASSERT_OFFSET(NCVirtualWndMain, bMouseOnWindow, 0x98);
NW_ASSERT_SIZE(NCVirtualWndMain, 0x9C);

#pragma pack(pop)