#pragma once
// NTownMapWnd - the map of a town (UClass "TownMapWnd", 0xD0; C2 only; NCConsole makes it): a
// 328 x 329 view into a 512 x 512 picture that the left button drags, with the player's place
// marked by an icon, inside the minimap's frame.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NTownMapWnd : public NCWnd
{
    NW_DECLARE_CLASS(NTownMapWnd, NCWnd, 0, 0x10291440)

    UTexture* BackTex;               // +0xC8 "L2UI.MinimapWnd.MinimapBack": the frame, drawn last
    UTexture* MapTex;                // +0xCC the town's picture (set from outside) (name ours)
    UTexture* IconTex;               // +0xD0 "town_map.icon_i00": the player's mark
    INT IconX;                       // +0xD4 the mark's place in the picture (set from outside) (name ours)
    INT IconY;                       // +0xD8 (name ours)
    INT MapW;                        // +0xDC the picture's size (512) (name ours)
    INT MapH;                        // +0xE0 (512)
    INT ViewW;                       // +0xE4 the part shown (328 x 329) (name ours)
    INT ViewH;                       // +0xE8
    INT DragX;                       // +0xEC where the drag last was, in the view (name ours)   // C2: offset derived, no evidence
    INT DragY;                       // +0xF0   // C2: offset derived, no evidence
    INT ScrollX;                     // +0xF4 the view's corner in the picture (name ours)
    INT ScrollY;                     // +0xF8
    UBOOL bDragging;                 // +0xFC (name ours)

    NTownMapWnd();                                                   // 0x100afed0
    // ~NTownMapWnd                                                  // 0x10099940 (C1 only: no C2 match)

    virtual void KillFocus();                                        // 30 0x100abeb0 (NCMinimapWnd's body)
    virtual INT OnCreate();                                          // 37 0x100af630
    virtual INT OnDestroy();                                         // 38 0x100ac1e0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100ad500
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x100ac400
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100ac340
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100ac3d0

    void SetMap(const TCHAR* TexName, INT X, INT Y);                 // 0x100ac260 (name ours) the picture and the mark's place
};
NW_ASSERT_OFFSET(NTownMapWnd, MapTex, 0xCC);
NW_ASSERT_OFFSET(NTownMapWnd, IconX, 0xD4);
NW_ASSERT_OFFSET(NTownMapWnd, MapW, 0xDC);
NW_ASSERT_OFFSET(NTownMapWnd, DragX, 0xEC);
NW_ASSERT_OFFSET(NTownMapWnd, ScrollX, 0xF4);
NW_ASSERT_OFFSET(NTownMapWnd, bDragging, 0xFC);
NW_ASSERT_SIZE(NTownMapWnd, 0x100);

#pragma pack(pop)
