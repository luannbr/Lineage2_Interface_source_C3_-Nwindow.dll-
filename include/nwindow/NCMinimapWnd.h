#pragma once
// NCMinimapWnd - the map window (UClass "CMinimapWnd", 0x1AC in C3; C2 0xE0). C3 rebuilt it: sixteen maps instead of
// two (0 the Aden continent, 1 the Elmore one, 2..13 the towns of Aden, 14 and 15 those of Elmore; the textures, the
// sizes and the scales are tables of the image), a 328 x 328 piece of one of them under a numbered grid (continent maps
// only), a row of buttons over the towns, the player's mark, the marks the console sets (AddTarget) and a button that
// goes back to the continent from a town.
//
// Everything below is read from the C3 code (no C2 layout is kept). The window is the child at +0xC8 of
// NCMapCoverWnd (the desktop's +0x14C), which owns the continent switch (a combo box at its +0xD8): the map calls
// the cover's 0x100ABE30 with the continent number when it changes it.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

// A position the console marks on the map and the radar (name ours; world coordinates). The map also keeps its fixed
// button positions in this shape (X, Y in map pixels; Z is never written: the stock stores a stack slot there).
struct FMinimapTarget
{
    INT X;
    INT Y;
    INT Z;
};
NW_ASSERT_SIZE(FMinimapTarget, 0xC);

// the scale of a map: world units to map pixels (name ours; the table at 0x1021F620 / 0x1021F660)
struct FMinimapScale
{
    FLOAT X;
    FLOAT Y;
};
NW_ASSERT_SIZE(FMinimapScale, 0x8);

// the maps (names ours)
enum
{
    MINIMAP_Aden = 0,       // the Aden continent
    MINIMAP_Elmore = 1,     // the Elmore continent
    MINIMAP_FirstTown = 2,  // 2..13 the towns of Aden (button ids 0xB01000..0xB0100B), 14 and 15 those of Elmore
                            //   (0xB01100, 0xB01101)
    MINIMAP_NumMaps = 16,
};

// NWM_COMMAND ids of the window's buttons (names ours)
enum
{
    NID_MINIMAP_ZOOMOUT = 0xB000F6,    // from a town back to its continent
    NID_MINIMAP_TOWN = 0xB01000,       // + the town's number: the buttons over the Aden continent (12)
    NID_MINIMAP_ELMORE_TOWN = 0xB01100,  // + the town's number: the buttons over the Elmore continent (2)
};

class NCMinimapWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMinimapWnd, NCWnd, 0, 0x10291960)

    TArray<UTexture*> MapTextures;        // +0xC8 the maps' textures by map number: the two continents (named in
                                          //   localization.ini by the country) then the towns (the strings at 0x10291398)
    TArray<NCButton*> TownButtons;        // +0xD4 the 26 x 26 buttons over the Aden continent (ids 0xB01000 + n)
    TArray<FMinimapTarget> TownPos;       // +0xE0 where they sit on the map (0x1021F5AC / 0x1021F5DC)
    TArray<FMinimapTarget> MarkPos;       // +0xEC the mark the console reports (filled by OnPaint while bShowMark)
    TArray<NToolTipInfo> ToolTips;        // +0xF8 only destroyed (OnDestroy)
    TArray<NCButton*> ElmoreButtons;      // +0x104 the buttons over the Elmore continent (ids 0xB01100 + n)
    TArray<FMinimapTarget> ElmorePos;     // +0x110 where they sit (0x1021F60C / 0x1021F614)
    TArray<FMinimapScale> MapScales;      // +0x11C world to map pixels, by map (0x1021F620 / 0x1021F660)
    TArray<FMinimapTarget> MapOrigins;    // +0x128 the world position of each map's corner (0x1021F6A0 / 0x1021F6E0)
    TArray<FMinimapTarget> Targets;       // +0x134 the marks the console sets (ShowRadar: AddTarget / RemoveTarget)
    BYTE _Unknown140[0x144 - 0x140];      // +0x140 never touched
    UTexture* MyPosTex;                   // +0x144 "L2UI.MinimapWnd.MinimapPos" (the player)
    UTexture* TargetPosTex;               // +0x148 "L2UI.MinimapWnd.TargetPos" (a target)
    UTexture* MarkTex;                    // +0x14C "L2UI_CH3.Minimap.MapIcon_Mark"
    UTexture* ShadowTex;                  // +0x150 "L2UI_CH3.Minimap.MapShadow" (the band under the map)
    UTexture* BarShadowTex;               // +0x154 "L2UI_CH3.FrameCtrl.Bar_Shadow" (only loaded)
    UTexture* MoonTex;                    // +0x158 "L2UI_CH3.Minimap.Map_Moon"
    UTexture* SunTex;                     // +0x15C "L2UI_CH3.Minimap.Map_Sun"
    BYTE _Unknown160[0x164 - 0x160];      // +0x160 never touched
    NCButton* ZoomOutButton;              // +0x164 id 0xB000F6, shown on a town's map only
    INT Continent;                        // +0x168 0 Aden, 1 Elmore (the cover's combo box follows it)
    INT Mode;                             // +0x16C what opened the map (NCConsole::ToggleMinimap's argument, 0..3: the
                                          //   text under the map is SysString[973 + Mode])
    UBOOL bDragging;                      // +0x170 the left button is down on the map
    UBOOL bShowMark;                      // +0x174 (name ours) nothing in the image sets it: the mark of MarkPos is
                                          //   never shown
    UBOOL bMyPosHover;                    // +0x178 the mouse is over the player's mark (OnMouseMove)
    UBOOL bMarkHover;                     // +0x17C the mouse is over the mark of MarkPos
    INT MapIndex;                         // +0x180 the map shown (0..15)
    INT MyX;                              // +0x184 the player on the map, relative to its piece (each paint)
    INT MyY;                              // +0x188
    INT ScrollX;                          // +0x18C the map pixel at the piece's corner
    INT ScrollY;                          // +0x190
    INT SavedScrollX;                     // +0x194 the continent's scroll, kept while a town is shown
    INT SavedScrollY;                     // +0x198
    INT ClickX;                           // +0x19C the last mouse position in the window (a drag moves the map by the
    INT ClickY;                           // +0x1A0   difference)
    BYTE _Unknown1A4[0x1AC - 0x1A4];      // +0x1A4 never touched

    NCMinimapWnd();                                                  // 0x100b00e0
    // ~NCMinimapWnd                                                 // 0x100b0230

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100abec0
    virtual void KillFocus();                                        // 30 0x100abeb0 (shared with NTownMapWnd and NCMapCoverWnd)
    virtual INT OnCreate();                                          // 37 0x100afc40
    virtual INT OnDestroy();                                         // 38 0x100b0320
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100adca0
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x100ae5b0 (the db names it OnPostPaint)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x100aeca0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100ac0d0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100ac150

    // the maps' tables (names ours); the stock calls them with its own `this`, which they never read
    __declspec(noinline) INT GetMapWidth(INT Map);                                        // 0x100ac180 0 out of 0..15
    __declspec(noinline) INT GetMapHeight(INT Map);                                       // 0x100ac1a0
    __declspec(noinline) UBOOL IsAdenMap(INT Map);                                        // 0x100ac1c0 map 0 and 2..13 are Aden's
    // the town buttons of a continent are shown (the other continent's hidden) with their positions at ScrollX/Y;
    // a town's map hides both
    __declspec(noinline) void ShowLayer(INT Map);                                         // 0x100ac800
    // a world position to map pixels of Map (the first map adds its origin, the others subtract theirs)
    __declspec(noinline) void WorldToMapPixels(INT* X, INT* Y, INT Map);                  // 0x100aca50
    __declspec(noinline) void UpdateButtons();                                            // 0x100acb10 after a scroll
    __declspec(noinline) INT GetMapIndex(INT X, INT Y, INT Z);                            // 0x100acc90 the map of the zone a position is in
    __declspec(noinline) void GetMyMapPos(INT* OutX, INT* OutY, INT Map);                 // 0x100acee0 the player on the map
    __declspec(noinline) void WorldToMap(INT WX, INT WY, INT* OutX, INT* OutY, INT Map);  // 0x100ad040 a world position on the map
    __declspec(noinline) void LoadMapData();                                              // 0x100ad770 OnCreate's tables and buttons
    __declspec(noinline) void CenterOnPlayer();                                           // 0x100aeef0 the cover calls it when it is shown
    __declspec(noinline) void ToggleContinent();                                          // 0x100af110 the other continent
    __declspec(noinline) void ShowPosition(INT X, INT Y, INT Z, UBOOL bFollow);           // 0x100af250 the console's "show this place"
    __declspec(noinline) void AddTarget(INT X, INT Y, INT Z);                             // 0x100af450
    __declspec(noinline) void RemoveTarget(INT X, INT Y, INT Z);                          // 0x100af490
    __declspec(noinline) void ClearTargets();                                             // 0x100af610
    // the handlers of the three kinds of button (WindowProc calls them through the slots)
    __declspec(noinline) INT OnZoomOutButton();                                           // 0x100af4f0 slot 68
    __declspec(noinline) INT OnTownButton(UINT ID);                                       // 0x100ad0a0 slot 69
    __declspec(noinline) INT OnElmoreTownButton(UINT ID);                                 // 0x100ad2d0 slot 70
    virtual void NewVirtual68();  // 68 0x100af4f0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual69();  // 69 0x100ad0a0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual70();  // 70 0x100ad2d0 (only this client has it; defined in NNewVirtuals.gen.cpp)
};
NW_ASSERT_OFFSET(NCMinimapWnd, MapTextures, 0xC8);
NW_ASSERT_OFFSET(NCMinimapWnd, TownButtons, 0xD4);
NW_ASSERT_OFFSET(NCMinimapWnd, TownPos, 0xE0);
NW_ASSERT_OFFSET(NCMinimapWnd, MarkPos, 0xEC);
NW_ASSERT_OFFSET(NCMinimapWnd, ToolTips, 0xF8);
NW_ASSERT_OFFSET(NCMinimapWnd, ElmoreButtons, 0x104);
NW_ASSERT_OFFSET(NCMinimapWnd, ElmorePos, 0x110);
NW_ASSERT_OFFSET(NCMinimapWnd, MapScales, 0x11C);
NW_ASSERT_OFFSET(NCMinimapWnd, MapOrigins, 0x128);
NW_ASSERT_OFFSET(NCMinimapWnd, Targets, 0x134);
NW_ASSERT_OFFSET(NCMinimapWnd, MyPosTex, 0x144);
NW_ASSERT_OFFSET(NCMinimapWnd, SunTex, 0x15C);
NW_ASSERT_OFFSET(NCMinimapWnd, ZoomOutButton, 0x164);
NW_ASSERT_OFFSET(NCMinimapWnd, Continent, 0x168);
NW_ASSERT_OFFSET(NCMinimapWnd, Mode, 0x16C);
NW_ASSERT_OFFSET(NCMinimapWnd, bDragging, 0x170);
NW_ASSERT_OFFSET(NCMinimapWnd, MapIndex, 0x180);
NW_ASSERT_OFFSET(NCMinimapWnd, ScrollX, 0x18C);
NW_ASSERT_OFFSET(NCMinimapWnd, ClickX, 0x19C);
NW_ASSERT_SIZE(NCMinimapWnd, 0x1AC);

#pragma pack(pop)
