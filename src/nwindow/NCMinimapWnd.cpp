/*=============================================================================
    NCMinimapWnd.cpp - the map window (C3). Stock: 0x100ac0d0 .. 0x100ac16e (the left button), 0x100ac180 ..
    0x100ac1de (the maps' tables), 0x100ac800 .. 0x100ad09c (the buttons and the coordinates), 0x100ad0a0 ..
    0x100ad4fb (the town buttons), 0x100ad770 .. 0x100adc9f (the tables), 0x100adca0 .. 0x100ae57d (paint),
    0x100ae5b0 .. 0x100aec9d (the tool tips), 0x100aeca0 .. 0x100af606 (mouse, continent, positions), 0x100afc40,
    0x100b00e0 .. 0x100b01f5, 0x100b0230, 0x100b0320 .. 0x100b058e, and WindowProc 0x100abec0.
    The stock helpers that are ECX-agnostic (the map tables) are called with the window's `this`, which they ignore.
=============================================================================*/
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCRadarWnd.h"
#include "nwindow/NCMapCoverWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleFns.h"

// the tables of the stock image (hook mode: they live in its .data; nothing writes them)
typedef INT NMinimapIntTable[16];
typedef FLOAT NMinimapFloatTable[16];
typedef FString NMinimapTownNames[14];
#if NW_HOOK_MODE
#define GMapWidths     NW_GLOBAL(NMinimapIntTable, 0x1021F720)    // 1642, 1312, 653, 768 ...
#define GMapHeights     NW_GLOBAL(NMinimapIntTable, 0x1021F760)    // 1476, 984, 626, 706 ...
#define GTownPosX       NW_GLOBAL(NMinimapIntTable, 0x1021F5AC)    // the first 12: Aden's town buttons
#define GTownPosY       NW_GLOBAL(NMinimapIntTable, 0x1021F5DC)
#define GElmorePosX     NW_GLOBAL(NMinimapIntTable, 0x1021F60C)    // the first 2: Elmore's
#define GElmorePosY     NW_GLOBAL(NMinimapIntTable, 0x1021F614)
#define GMapScaleX      NW_GLOBAL(NMinimapFloatTable, 0x1021F620)
#define GMapScaleY      NW_GLOBAL(NMinimapFloatTable, 0x1021F660)
#define GMapOriginX     NW_GLOBAL(NMinimapIntTable, 0x1021F6A0)
#define GMapOriginY     NW_GLOBAL(NMinimapIntTable, 0x1021F6E0)
#define GTownMapNames   NW_GLOBAL(NMinimapTownNames, 0x10291398)  // "map_gludin_village" ... turned into the
                                                                   // textures' names by LoadMapData
#else
// the standalone's copies (src/standalone/NwGlobals.cpp, tools/gen_globals.py), with the stock initial values and the stock
// extents: 12 Aden town buttons, 2 Elmore ones, 16 maps, 14 town map names
extern INT GTownPosX[12];
extern INT GTownPosY[12];
extern INT GElmorePosX[2];
extern INT GElmorePosY[2];
extern FLOAT GMapScaleX[16];
extern FLOAT GMapScaleY[16];
extern INT GMapOriginX[16];
extern INT GMapOriginY[16];
extern INT GMapWidths[16];
extern INT GMapHeights[16];
extern FString GTownMapNames[14];
#endif

// the player's world position as the stock reads it inline (the first viewport's controller's pawn; 0 without one):
// UE2 inlined `GetMyLocation()` into the map's functions, one coordinate at a time
static FORCEINLINE FVector MyLocation()
{
    FVector Result(0.0f, 0.0f, 0.0f);
    APlayerController* Controller = GConsole->Engine->Client->Viewports(0)->Actor;
    if (Controller && Controller->Pawn)
        Result = Controller->Pawn->Location;
    return Result;
}

static FORCEINLINE INT NAbs(INT Value)
{
    return Value < 0 ? -Value : Value;
}

// the region of the world map where the harbor's mark stands (the map shows it at (100, 1334)), in world coordinates
static FORCEINLINE UBOOL InHarborBox(INT X, INT Y)
{
    return X >= 0x9E0B && X <= 0xC3FC && Y >= 0x3AFAD && Y <= 0x3D4E7;
}

// the desktop's tracked quest (0x100AC600: the id the quest window's list has picked, the console itself when there is
// none) and a quest's name (0x1005CC60: NULL when the id is unknown): stock functions of the console
static FORCEINLINE INT CurrentQuestID()
{
    return ((NConsoleLeftFns*)GConsole)->GetTrackedQuestID();
}
static FORCEINLINE TCHAR* GetQuestName(INT QuestID)
{
    return GConsole->GetQuestName(QuestID);   // 0x1005CC60
}

// the tool tip's nine pieces from the skin (the stock inlines this into both tool tips): the corners 8 x 8, the edges
// and the fill stretched to W x H
static FORCEINLINE void DrawToolTipSkin(NCGDevice* Device, NToolTipSkin* Skin, INT X, INT Y, INT W, INT H)
{
    if (!Skin)
        return;
    if (Skin->Tex[0])
        Device->DrawTexture(X, Y, 8, 8, 0, 0, 8, 8, Skin->Tex[0], 0xFF, 1);
    if (Skin->Tex[1])
        Device->DrawStretchedTex(X + 8, Y, W, 8, Skin->Tex[1], 1, 0xFF);
    if (Skin->Tex[2])
        Device->DrawTexture(W + X + 8, Y, 8, 8, 0, 0, 8, 8, Skin->Tex[2], 0xFF, 1);
    if (Skin->Tex[3])
        Device->DrawStretchedTex(X, Y + 8, 8, H, Skin->Tex[3], 1, 0xFF);
    if (Skin->Tex[4])
        Device->DrawStretchedTex(X + 8, Y + 8, W, H, Skin->Tex[4], 1, 0xFF);
    if (Skin->Tex[5])
        Device->DrawStretchedTex(W + X + 8, Y + 8, 8, H, Skin->Tex[5], 1, 0xFF);
    if (Skin->Tex[6])
        Device->DrawTexture(X, H + Y + 8, 8, 8, 0, 0, 8, 8, Skin->Tex[6], 0xFF, 1);
    if (Skin->Tex[7])
        Device->DrawStretchedTex(X + 8, H + Y + 8, W, 8, Skin->Tex[7], 1, 0xFF);
    if (Skin->Tex[8])
        Device->DrawTexture(W + X + 8, H + Y + 8, 8, 8, 0, 0, 8, 8, Skin->Tex[8], 0xFF, 1);
}

// 0x100b00e0 - the arrays empty, the scroll at the corner; Mode and the click position are left to the allocator's
// zeroes (the stock constructor does not store them)
NCMinimapWnd::NCMinimapWnd()
{
    MyPosTex = NULL;
    MyX = 0;
    MyY = 0;
    ScrollX = 0;
    ScrollY = 0;
    SavedScrollX = 0;
    SavedScrollY = 0;
    Continent = 0;
    MapIndex = 0;
    bDragging = 0;
    bShowMark = 0;
    bMyPosHover = 0;
    bMarkHover = 0;
}
NW_HOOK_CTOR(0x100b00e0, NCMinimapWnd)

// 0x100b0230 - the arrays go with the members
NW_HOOK_DTOR(0x100b0230, NCMinimapWnd)

// 0x100abec0 (vslot 25) - the cover's buttons: the zoom out, the 12 towns of Aden and the 2 of Elmore, each through its
// own slot (68, 69, 70) that answers 1 when it took the command
INT NCMinimapWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        switch (wParam)
        {
        case NID_MINIMAP_ZOOMOUT:
            return ::nw::VirtualCall<INT>(this, 68) != 0;
        case NID_MINIMAP_TOWN + 0:
        case NID_MINIMAP_TOWN + 1:
        case NID_MINIMAP_TOWN + 2:
        case NID_MINIMAP_TOWN + 3:
        case NID_MINIMAP_TOWN + 4:
        case NID_MINIMAP_TOWN + 5:
        case NID_MINIMAP_TOWN + 6:
        case NID_MINIMAP_TOWN + 7:
        case NID_MINIMAP_TOWN + 8:
        case NID_MINIMAP_TOWN + 9:
        case NID_MINIMAP_TOWN + 10:
        case NID_MINIMAP_TOWN + 11:
            return ::nw::VirtualCall<INT>(this, 69, wParam) != 0;
        case NID_MINIMAP_ELMORE_TOWN + 0:
        case NID_MINIMAP_ELMORE_TOWN + 1:
            return ::nw::VirtualCall<INT>(this, 70, wParam) != 0;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x100abec0, NCMinimapWnd, WindowProc)

// 0x100ac0d0 (vslot 62) - a press inside the map starts a drag: the click is kept, the cursor is the hand (3) and
// the console locks it
INT NCMinimapWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT X = (SHORT)LOWORD(lParam) - ScreenX;
    INT Y = (SHORT)HIWORD(lParam) - ScreenY;
    if (X >= 1 && X <= (INT)Width && Y >= 1 && Y <= (INT)Height)
    {
        ClickX = X;
        ClickY = Y;
        bDragging = 1;
        SetCursor(3, NULL);
        GConsole->LockCursor();
    }
    return 1;
}
NW_HOOK_METHOD(0x100ac0d0, NCMinimapWnd, OnLButtonDown)

// 0x100ac150 (vslot 63)
INT NCMinimapWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    GConsole->UnlockCursor();
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100ac150, NCMinimapWnd, OnLButtonUp)

// 0x100ac180 - the map's width in pixels (0 for a number out of 0..15)
INT NCMinimapWnd::GetMapWidth(INT Map)
{
    if (Map >= 0 && Map <= 15)
        return GMapWidths[Map];
    return 0;
}
NW_HOOK_METHOD(0x100ac180, NCMinimapWnd, GetMapWidth)

// 0x100ac1a0 - the same for the height
INT NCMinimapWnd::GetMapHeight(INT Map)
{
    if (Map >= 0 && Map <= 15)
        return GMapHeights[Map];
    return 0;
}
NW_HOOK_METHOD(0x100ac1a0, NCMinimapWnd, GetMapHeight)

// 0x100ac1c0 - the maps of the Aden continent: 0 and 2..13 (1, 14 and 15 are Elmore's)
UBOOL NCMinimapWnd::IsAdenMap(INT Map)
{
    if ((Map >= 2 && Map <= 13) || Map == 0)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x100ac1c0, NCMinimapWnd, IsAdenMap)

// 0x100ac800 - what a map shows of the town buttons: a continent's own buttons shown at their positions less the
// scroll, the other continent's hidden; a town's map hides both
void NCMinimapWnd::ShowLayer(INT Map)
{
    INT i;
    if (Map == 0)
    {
        for (i = 0; i < TownButtons.Num(); i++)
        {
            if (!TownButtons(i)->IsShowWindow())
                TownButtons(i)->ShowWindow();
        }
        for (i = 0; i < TownPos.Num(); i++)
            TownButtons(i)->SetPosition(TownPos(i).X - ScrollX, TownPos(i).Y - ScrollY, 0x1A, 0x1A);
        for (i = 0; i < ElmoreButtons.Num(); i++)
        {
            if (ElmoreButtons(i)->IsShowWindow())
                ElmoreButtons(i)->HideWindow();
        }
    }
    else if (Map == 1)
    {
        for (i = 0; i < ElmoreButtons.Num(); i++)
        {
            if (!ElmoreButtons(i)->IsShowWindow())
                ElmoreButtons(i)->ShowWindow();
        }
        for (i = 0; i < ElmorePos.Num(); i++)
            ElmoreButtons(i)->SetPosition(ElmorePos(i).X - ScrollX, ElmorePos(i).Y - ScrollY, 0x1A, 0x1A);
        for (i = 0; i < TownButtons.Num(); i++)
        {
            if (TownButtons(i)->IsShowWindow())
                TownButtons(i)->HideWindow();
        }
    }
    else
    {
        for (i = 0; i < ElmoreButtons.Num(); i++)
        {
            if (ElmoreButtons(i)->IsShowWindow())
                ElmoreButtons(i)->HideWindow();
        }
        for (i = 0; i < TownButtons.Num(); i++)
        {
            if (TownButtons(i)->IsShowWindow())
                TownButtons(i)->HideWindow();
        }
    }
}
NW_HOOK_METHOD(0x100ac800, NCMinimapWnd, ShowLayer)

// 0x100aca50 - a world position as map pixels: the first map adds its origin before the scale, the others subtract
// theirs (the scale is a float: the product of the int and the float is exact in the x87)
void NCMinimapWnd::WorldToMapPixels(INT* X, INT* Y, INT Map)
{
    if (Map == 0)
    {
        *X = (INT)((DOUBLE)(MapOrigins(0).X + *X) * MapScales(0).X);
        *Y = (INT)((DOUBLE)(MapOrigins(0).Y + *Y) * MapScales(0).Y);
        return;
    }
    *X = (INT)((DOUBLE)(*X - MapOrigins(Map).X) * MapScales(Map).X);
    *Y = (INT)((DOUBLE)(*Y - MapOrigins(Map).Y) * MapScales(Map).Y);
}
NW_HOOK_METHOD(0x100aca50, NCMinimapWnd, WorldToMapPixels)

// 0x100acb10 - after a scroll: the buttons of the continent shown again at their places, the other continent's
// hidden (nothing for a town)
void NCMinimapWnd::UpdateButtons()
{
    INT i;
    if (MapIndex == 0)
    {
        for (i = 0; i < TownPos.Num(); i++)
        {
            if (i >= TownButtons.Num())
                return;
            TownButtons(i)->ShowWindow();
            TownButtons(i)->SetPosition(TownPos(i).X - ScrollX, TownPos(i).Y - ScrollY, 0x1A, 0x1A);
        }
        for (i = 0; i < ElmorePos.Num(); i++)
        {
            if (ElmoreButtons(i)->IsShowWindow())
                ElmoreButtons(i)->HideWindow();
        }
    }
    else if (MapIndex == 1)
    {
        for (i = 0; i < ElmorePos.Num(); i++)
        {
            if (i >= ElmoreButtons.Num())
                return;
            ElmoreButtons(i)->ShowWindow();
            ElmoreButtons(i)->SetPosition(ElmorePos(i).X - ScrollX, ElmorePos(i).Y - ScrollY, 0x1A, 0x1A);
        }
        for (i = 0; i < TownPos.Num(); i++)
        {
            if (TownButtons(i)->IsShowWindow())
                TownButtons(i)->HideWindow();
        }
    }
}
NW_HOOK_METHOD(0x100acb10, NCMinimapWnd, UpdateButtons)

// 0x100acc90 - the map a position belongs to: the zone the game data gives for it (the player's own zone when the
// position is the player's) by the table of the zones that have a town map, else Aden's or Elmore's by the Y
INT NCMinimapWnd::GetMapIndex(INT X, INT Y, INT Z)
{
    INT Zone;
    if (X == (INT)MyLocation().X && Y == (INT)GConsole->GetMyLocation().Y && Z == (INT)GConsole->GetMyLocation().Z)
        Zone = GConsole->GetCurrentZoneID();   // 0x1005CE70: the zone the player is in
    else
        Zone = GConsole->GetZoneID((FLOAT)X, (FLOAT)Y, (FLOAT)Z);   // 0x1005CEF0: the zone at a position
    if (Zone < 0)
        return 0;
    switch (Zone)
    {
    case 0x14:
        return 4;
    case 0x15:
        return 3;
    case 0x23:
        return 2;
    case 0x2B:
        return 10;
    case 0x30:
        return 6;
    case 0x3A:
        return 7;
    case 0x3E:
        return 14;
    case 0x40:
        return 15;
    case 0x4A:
        return 11;
    case 0x4D:
        return 12;
    case 0x65:
        return 8;
    case 0x6B:
        return 5;
    case 0x71:
        return 9;
    case 0x76:
        return 13;
    }
    return Y < -0xC000;
}
NW_HOOK_METHOD(0x100acc90, NCMinimapWnd, GetMapIndex)

// 0x100acee0 - where the player is on a map: the harbor's box (the mark at (100, 1334)) or the pawn's position by
// WorldToMapPixels
void NCMinimapWnd::GetMyMapPos(INT* OutX, INT* OutY, INT Map)
{
    INT PX = (INT)MyLocation().X;
    INT PY = (INT)MyLocation().Y;
    INT PZ = (INT)MyLocation().Z;
    if (InHarborBox(PX, PY) && PZ <= -0x130A && PZ >= -0x1ADA)
    {
        *OutX = 100;
        *OutY = 0x536;
        return;
    }
    WorldToMapPixels(&PX, &PY, Map);
    *OutX = PX;
    *OutY = PY;
}
NW_HOOK_METHOD(0x100acee0, NCMinimapWnd, GetMyMapPos)

// 0x100ad040 - a world position on a map (the harbor's box has its own mark)
void NCMinimapWnd::WorldToMap(INT WX, INT WY, INT* OutX, INT* OutY, INT Map)
{
    if (InHarborBox(WX, WY))
    {
        *OutX = 100;
        *OutY = 0x536;
        return;
    }
    *OutX = WX;
    *OutY = WY;
    WorldToMapPixels(OutX, OutY, Map);
}
NW_HOOK_METHOD(0x100ad040, NCMinimapWnd, WorldToMap)

// 0x100ad0a0 (slot 69) - a town button of Aden: its map (the ids start at map 2), the continent's scroll kept, the
// buttons hidden, the zoom out shown, the piece centered on the player when he is on that map
INT NCMinimapWnd::OnTownButton(UINT ID)
{
    if (ID < 0xB01000 || ID > 0xB0100B)
        return 0;
    MapIndex = ID - 0xB00FFE;
    SavedScrollX = ScrollX;
    SavedScrollY = ScrollY;
    for (INT i = 0; i < TownButtons.Num(); i++)
        TownButtons(i)->HideWindow();
    ZoomOutButton->ShowWindow();
    INT PX = (INT)MyLocation().X;
    INT PY = (INT)MyLocation().Y;
    INT Map = MapIndex;
    INT W = GetMapWidth(Map);
    INT H = GetMapHeight(Map);
    if (W > 0 && H > 0)
    {
        WorldToMapPixels(&PX, &PY, Map);
        if (PX < 0 || PX > GMapWidths[Map] || PY < 0 || PY > GMapHeights[Map])
        {
            ScrollX = 0;
            ScrollY = 0;
        }
        else
        {
            if (PX < 0xA4)
                ScrollX = 0;
            else if (PX > W - 0xA4)
                ScrollX = W - 0x148;
            else
                ScrollX = PX - 0xA4;
            if (PY < 0xA4)
                ScrollY = 0;
            else if (PY > H - 0xA4)
                ScrollY = H - 0x148;
            else
                ScrollY = PY - 0xA4;
        }
        SetFocus();
        return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x100ad0a0, NCMinimapWnd, OnTownButton)

// 0x100ad2d0 (slot 70) - the same for the two towns of Elmore (the maps 14 and 15)
INT NCMinimapWnd::OnElmoreTownButton(UINT ID)
{
    if (ID < 0xB01100 || ID > 0xB01101)
        return 0;
    MapIndex = ID - 0xB01100 + 14;
    SavedScrollX = ScrollX;
    SavedScrollY = ScrollY;
    for (INT i = 0; i < ElmoreButtons.Num(); i++)
        ElmoreButtons(i)->HideWindow();
    ZoomOutButton->ShowWindow();
    INT PX = (INT)MyLocation().X;
    INT PY = (INT)MyLocation().Y;
    INT Map = MapIndex;
    INT W = GetMapWidth(Map);
    INT H = GetMapHeight(Map);
    if (W > 0 && H > 0)
    {
        WorldToMapPixels(&PX, &PY, Map);
        if (PX < 0 || PX > GMapWidths[Map] || PY < 0 || PY > GMapHeights[Map])
        {
            ScrollX = 0;
            ScrollY = 0;
        }
        else
        {
            if (PX < 0xA4)
                ScrollX = 0;
            else if (PX > W - 0xA4)
                ScrollX = W - 0x148;
            else
                ScrollX = PX - 0xA4;
            if (PY < 0xA4)
                ScrollY = 0;
            else if (PY > H - 0xA4)
                ScrollY = H - 0x148;
            else
                ScrollY = PY - 0xA4;
        }
        SetFocus();
        return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x100ad2d0, NCMinimapWnd, OnElmoreTownButton)

// 0x100ad770 - OnCreate's tables and buttons: the positions of the town buttons and the maps' scales and origins from
// the image's tables, a 26 x 26 button per town (ids 0xB01000 + n, 0xB01100 + n) and the textures of the maps: the two
// continents' (named in localization.ini by the country, with defaults) and the towns' ("<TownMap>.<name>": the
// strings of the image are overwritten by the full names)
void NCMinimapWnd::LoadMapData()
{
    INT i;
    for (i = 0; i < 12; i++)
    {
        FMinimapTarget P;
        P.X = GTownPosX[i];
        P.Y = GTownPosY[i];
        P.Z = 0;
        TownPos.AddItem(P);
    }
    for (i = 0; i < 2; i++)
    {
        FMinimapTarget P;
        P.X = GElmorePosX[i];
        P.Y = GElmorePosY[i];
        P.Z = 0;
        ElmorePos.AddItem(P);
    }
    for (i = 0; i < 16; i++)
    {
        FMinimapScale S;
        S.X = GMapScaleX[i];
        S.Y = GMapScaleY[i];
        MapScales.AddItem(S);
    }
    for (i = 0; i < 16; i++)
    {
        FMinimapTarget O;
        O.X = GMapOriginX[i];
        O.Y = GMapOriginY[i];
        O.Z = 0;
        MapOrigins.AddItem(O);
    }
    for (i = 0; i < TownPos.Num(); i++)
    {
        NCButton* Button = new NCButton(TEXT("L2UI_CH3.Minimap.MapButton_ZoomIn1"),
                                        TEXT("L2UI_CH3.Minimap.MapButton_ZoomIn2"), 0);
        Button->ID = NID_MINIMAP_TOWN + i;
        CreateChild(Button, NWS_VISIBLE | NWS_CLIPTOPARENT);
        Button->SetPosition(TownPos(i).X - ScrollX, TownPos(i).Y - ScrollY, 0x1A, 0x1A);
        TownButtons.AddItem(Button);
    }
    for (i = 0; i < ElmorePos.Num(); i++)
    {
        NCButton* Button = new NCButton(TEXT("L2UI_CH3.Minimap.MapButton_ZoomIn1"),
                                        TEXT("L2UI_CH3.Minimap.MapButton_ZoomIn2"), 0);
        Button->ID = NID_MINIMAP_ELMORE_TOWN + i;
        CreateChild(Button, NWS_VISIBLE | NWS_CLIPTOPARENT);
        Button->SetPosition(ElmorePos(i).X - ScrollX, ElmorePos(i).Y - ScrollY, 0x1A, 0x1A);
        ElmoreButtons.AddItem(Button);
    }

    FString Name;
    TCHAR Key[256];
    appSprintf(Key, TEXT("%s_AdenMinimap"), GCountryName);
    if (!GConfig->GetString(GCountryName, Key, Name, TEXT("localization.ini")))
        Name = TEXT("L2font.Minimap.aden_map-k");
    MapTextures.AddItem(FindTexture(*Name));
    appSprintf(Key, TEXT("%s_ElmoreMinimap"), GCountryName);
    if (!GConfig->GetString(GCountryName, Key, Name, TEXT("localization.ini")))
        Name = TEXT("L2font.Minimap.elmore_map-k");
    MapTextures.AddItem(FindTexture(*Name));
    appSprintf(Key, TEXT("%s_TownMap"), GCountryName);
    if (!GConfig->GetString(GCountryName, Key, Name, TEXT("localization.ini")))
        Name = TEXT("L2Font.TownMap");
    for (i = 0; i < 14; i++)
    {
        GTownMapNames[i] = Name + TEXT(".") + GTownMapNames[i];
        MapTextures.AddItem(FindTexture(*GTownMapNames[i]));
    }
}
NW_HOOK_METHOD(0x100ad770, NCMinimapWnd, LoadMapData)

// 0x100adca0 (vslot 56) - the piece of the map (the continents with their grid: the column and row numbers from the
// map's origin, every cell of the scale's width), the shadow band, the player's mark and the targets, the place's
// name, the mode text and the game's clock (the sun from 6 to 24 o'clock, else the moon)
INT NCMinimapWnd::OnPaint(NCGDevice* Device)
{
    guard(NCMinimapWnd::OnPaint);
    TCHAR Buf[256];
    SIZE Size;
    Size.cx = 0;
    Size.cy = 0;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    UTexture* MapTex = MapTextures(MapIndex);
    INT OffX = 0;
    INT OffY = 0;
    if (!MapTex)
        return 0;
    if (MapIndex == 0)
    {
        OffX = 0x30000;
        OffY = 0x38000;
    }
    else if (MapIndex == 1)
    {
        OffX = 0x40000;
        OffY = 0x8000;
    }
    OffX = (INT)((DOUBLE)OffX * MapScales(MapIndex).X);
    OffY = (INT)((DOUBLE)OffY * MapScales(MapIndex).Y);
    // NCGDevice's 0x10010CD0 (UCanvas::DrawMinimapTex): X, Y, XL, YL, U, V, UL, VL, texture
    Device->DrawMinimapTex(0, 0, 0x148, 0x148, ScrollX, ScrollY, 0x148, 0x148, MapTex);
    if (MapIndex == 0 || MapIndex == 1)
    {
        INT CellW = (INT)((DOUBLE)MapScales(MapIndex).X * 12288.0);
        INT CellH = (INT)((DOUBLE)MapScales(MapIndex).Y * 12288.0);
        INT Label = (OffX + ScrollX) / CellW;
        INT X = CellW - (OffX + ScrollX) % CellW;
        INT Row = (OffY + ScrollY) / CellH;
        INT Y = CellH - (OffY + ScrollY) % CellH;
        Label += 20;
        INT RowLabel = Row + 20;
        for (INT k = 0; k <= 0x148 / CellW; k++, X += CellW, Label++)
        {
            if (Label >= 0)
            {
                appSprintf(Buf, TEXT("%d"), Label);
                NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
                if (X > 0xF)
                    Device->DrawText(X - Size.cx / 2, 0x18, 0xFFDCDCDC, Buf, 0, 0, 0, (L2FontType)1, 0, 0, 0, 0, 0, 0, 0);
            }
            Device->DrawLine(X, Size.cy, X, 0x148, 0x40000000, 1);
        }
        for (INT j = 0; j <= 0x149 / CellH; j++, Y += CellH, RowLabel++)
        {
            if (RowLabel >= 0)
            {
                appSprintf(Buf, TEXT("%d"), RowLabel + 1);
                NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
                if (Y > 0x14)
                    Device->DrawText(0, Y - Size.cy / 2, 0xFFDCDCDC, Buf, 0, 0, 0, (L2FontType)1, 0, 0, 0, 0, 0, 0, 0);
            }
            Device->DrawLine(Size.cx, Y, 0x148, Y, 0x40000000, 1);
        }
    }

    // the player
    GetMyMapPos(&MyX, &MyY, MapIndex);
    MyX -= ScrollX;
    MyY -= ScrollY;
    if (MyPosTex && MyX >= 0xC && MyX <= (INT)Width - 0xC && MyY >= 0xC && MyY < (INT)Height - 0xC)
        Device->DrawMinimapLoc(MyX, MyY, MyPosTex);

    // the targets the console marked
    for (INT i = 0; i < Targets.Num(); i++)
    {
        FMinimapTarget T = Targets(i);
        INT TX;
        INT TY;
        WorldToMap(T.X, T.Y, &TX, &TY, MapIndex);
        TX -= ScrollX;
        TY -= ScrollY;
        if (TargetPosTex && TX >= 0xC && TX <= (INT)Width - 0xC && TY >= 0xC && TY < (INT)Height - 0xC)
            Device->DrawMinimapLoc(TX, TY, TargetPosTex);
    }

    // the console's mark (its virtual 345 gives a name and a position): asked for once, drawn as a pin
    if (bShowMark)
    {
        if (MarkPos.Num() == 0)
        {
            FString MarkName;
            FVector MarkLoc;
            ::nw::VirtualCall<void>(GConsole, 345, &MarkName, &MarkLoc);
            MarkPos.Empty();
            INT MX = (INT)MarkLoc.X;
            INT MY = (INT)MarkLoc.Y;
            INT MZ = (INT)MarkLoc.Z;
            INT Index = MarkPos.Add();
            MarkPos(Index).X = MX;
            MarkPos(Index).Y = MY;
            MarkPos(Index).Z = MZ;
        }
        for (INT i = 0; i < MarkPos.Num(); i++)
        {
            FMinimapTarget T = MarkPos(i);
            INT MX;
            INT MY;
            WorldToMap(T.X, T.Y, &MX, &MY, MapIndex);
            MX -= ScrollX;
            MY -= ScrollY;
            Device->DrawTexture(MX, MY - 0x14, 0x20, 0x20, 0, 0, 0x20, 0x20, MarkTex, 0xFF, 1);
        }
    }
    else
        MarkPos.Empty();

    // the band at the bottom with the place and the mode
    Device->DrawTexture(0, 0x136, 0x149, 0x17, 0, 0, 0x149, 0x17, ShadowTex, 0xFF, 1);
    appSprintf(Buf, TEXT("%s : %s"), *GL2GameData.SysString[471], GConsole->GetCurrentZoneName());   // 0x1005CDF0
    Device->DrawText(0x16, (INT)(Height - 15.0f), 0xFFDCDCDC, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    switch (Mode)
    {
    case 0:
        appSprintf(Buf, TEXT("%s : %s"), *GL2GameData.SysString[833], *GL2GameData.SysString[973]);
        Device->DrawText(0x16, (INT)(Height - 31.0f), 0xFFDCDCDC, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        break;
    case 1:
        appSprintf(Buf, TEXT("%s : %s"), *GL2GameData.SysString[833], *GL2GameData.SysString[974]);
        Device->DrawText(0x16, (INT)(Height - 31.0f), 0xFFDCDCDC, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        break;
    case 2:
        appSprintf(Buf, TEXT("%s : %s"), *GL2GameData.SysString[833], *GL2GameData.SysString[975]);
        Device->DrawText(0x16, (INT)(Height - 31.0f), 0xFFDCDCDC, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        break;
    case 3:
        appSprintf(Buf, TEXT("%s : %s"), *GL2GameData.SysString[833], *GL2GameData.SysString[976]);
        Device->DrawText(0x16, (INT)(Height - 31.0f), 0xFFDCDCDC, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        break;
    }

    // the game's clock: hours (and the fraction of the hour as minutes) of the day, the sun or the moon, "AM" / "PM"
    TCHAR Suffix[3] = TEXT("AM");
    DOUBLE GameTime = ::nw::Field<DOUBLE>(::nw::Field<BYTE*>(GConsole->Engine, 0x64), 0x128);
    INT Hour = (INT)GameTime;
    INT Minute = (INT)((GameTime - (DOUBLE)Hour) * 60.0);
    if (Minute >= 60)
    {
        Minute -= 60;
        Hour++;
    }
    if (Hour >= 6 && Hour <= 24)
        Device->DrawTexture(4, 6, 0x19, 0x19, 0, 0, 0x19, 0x19, SunTex, 0xFF, 1);
    else
        Device->DrawTexture(4, 6, 0x19, 0x19, 0, 0, 0x19, 0x19, MoonTex, 0xFF, 1);
    if (Hour >= 12)
    {
        appStrcpy(Suffix, TEXT("PM"));
        Hour -= 12;
    }
    if (Hour == 0 || Hour >= 10)
    {
        if (Hour == 0)
            Hour = 12;
        if (Minute < 10)
            appSprintf(Buf, TEXT("%s %d : %d%d"), Suffix, Hour, 0, Minute);
        else
            appSprintf(Buf, TEXT("%s %d : %d"), Suffix, Hour, Minute);
    }
    else
    {
        if (Minute < 10)
            appSprintf(Buf, TEXT("%s %d%d : %d%d"), Suffix, 0, Hour, 0, Minute);
        else
            appSprintf(Buf, TEXT("%s %d%d : %d"), Suffix, 0, Hour, Minute);
    }
    Device->DrawText(0x1D, 0x13, 0xFFDCDCDC, Buf, 0, 0, 0, (L2FontType)1, 0, 0, 0, 0, 0, 0, 0);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100adca0, NCMinimapWnd, OnPaint)

// 0x100ae5b0 (vslot 57, the db's OnPostPaint) - the tool tip over the player's mark (his name) and over the console's
// mark (its name, under it the tracked quest's), a box from the window's skin kept inside the window
INT NCMinimapWnd::OnDrawToolTip(NCGDevice* Device)
{
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (bMyPosHover)
    {
        NCGDevice::GetTextExtent(&Size, GConsole->GetMyUser()->GetName(), 0, 0, 0, 0, 0);
        INT X = MyX;
        INT Y = MyY + 10;
        if (Size.cx + X + 16 > (INT)Width - 5)
            X = (INT)((DOUBLE)Width - 5.0 - 16.0 - (DOUBLE)Size.cx);
        if (Size.cy + Y + 16 > (INT)Height - 5)
            Y = (INT)((DOUBLE)Height - 5.0 - 16.0 - (DOUBLE)Size.cy);
        DrawToolTipSkin(Device, ToolTipSkin, X, Y, Size.cx, Size.cy);
        Device->DrawText(X + 8, Y + 8, 0xFFDCDCDC, GConsole->GetMyUser()->GetName(), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }
    if (bMarkHover && MarkPos.Num() != 0)
    {
        FMinimapTarget T = MarkPos(0);
        INT OutX;
        INT OutY;
        WorldToMap(T.X, T.Y, &OutX, &OutY, MapIndex);
        INT PX = OutX - ScrollX;
        INT PY = OutY - ScrollY;
        SIZE Quest;
        Quest.cx = 0;
        Quest.cy = 0;
        {
            // the mark's name by the console's virtual 355 (a FString by value, built by the callee in our buffer)
            BYTE NameBuffer[sizeof(FString)];
            FString& MarkName = *(FString*)NameBuffer;
            ::nw::VirtualCall<FString*>(GConsole, 355, &MarkName);
            NCGDevice::GetTextExtent(&Size, *MarkName, 0, 0, 0, 0, 0);
            MarkName.~FString();
        }
        // the tracked quest of the desktop's first window's list; an uninitialized local in the stock code, which
        // the compiler gave the slot of OutX
        INT QuestID = OutX;
        if (GConsole->Desktop)
        {
            NCWnd* Window = ::nw::Field<NCWnd*>(GConsole->Desktop, 0xC8);
            if (Window)
            {
                BYTE* List = ::nw::Field<BYTE*>(Window, 0xD8);
                if (List)
                    QuestID = ::nw::Field<INT>(List, 0xFC);
            }
        }
        if (QuestID > 0)
            NCGDevice::GetTextExtent(&Quest, GetQuestName(CurrentQuestID()), 0, 0, 0, 0, 0);
        if (Size.cx > Quest.cx)
            Quest.cx = Size.cx;
        INT BoxW = Quest.cx;
        INT BoxH = Quest.cy + Size.cy + 2;
        PX += -5;
        if (PX < 5)
            PX = 5;
        if (PY - 5 < 5)
            PY = 5;
        else
            PY += 5;
        INT WindowW = (INT)Width;
        if (BoxW + PX + 0x10 > WindowW - 5)
            PX = WindowW - BoxW - 0x15;
        INT WindowH = (INT)Height;
        if (BoxH + PY + 0x10 > WindowH - 5)
            PY = WindowH - BoxH - 0x15;
        DrawToolTipSkin(Device, ToolTipSkin, PX, PY, BoxW, BoxH);
        {
            BYTE NameBuffer[sizeof(FString)];
            FString& MarkName = *(FString*)NameBuffer;
            ::nw::VirtualCall<FString*>(GConsole, 355, &MarkName);
            Device->DrawText(PX + 8, PY + 8, 0xFFDCDCDC, *MarkName, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
            MarkName.~FString();
        }
        INT QuestID2 = OutX;
        if (GConsole->Desktop)
        {
            NCWnd* Window = ::nw::Field<NCWnd*>(GConsole->Desktop, 0xC8);
            if (Window)
            {
                BYTE* List = ::nw::Field<BYTE*>(Window, 0xD8);
                if (List)
                    QuestID2 = ::nw::Field<INT>(List, 0xFC);
            }
        }
        Device->DrawText(PX + 8, PY + Size.cy + 10, 0xFFDCDCDC, GetQuestName(QuestID2), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x100ae5b0, NCMinimapWnd, OnDrawToolTip)

// 0x100aeca0 (vslot 61) - a drag moves the map by the mouse's travel (kept inside it), then the hover tests of the
// two marks (the player's: 10 pixels around; the console's: the pin's box)
INT NCMinimapWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = (SHORT)LOWORD(lParam) - ScreenX;
    INT Y = (SHORT)HIWORD(lParam) - ScreenY;
    if (bDragging && X >= 1 && X <= (INT)Width && Y >= 1 && Y <= (INT)Height)
    {
        INT DX = ClickX - X;
        INT DY = ClickY - Y;
        INT W = GetMapWidth(MapIndex);
        INT H = GetMapHeight(MapIndex);
        if (DX > 0)
        {
            ScrollX += DX;
            if (ScrollX > W - 0x148)
                ScrollX = W - 0x148;
        }
        else
        {
            ScrollX -= NAbs(DX);
            if (ScrollX < 0)
                ScrollX = 0;
        }
        if (DY > 0)
        {
            ScrollY += DY;
            if (ScrollY > H - 0x148)
                ScrollY = H - 0x148;
        }
        else
        {
            ScrollY -= NAbs(DY);
            if (ScrollY < 0)
                ScrollY = 0;
        }
        UpdateButtons();
    }
    if (MyX >= 0xC && MyX <= (INT)Width - 0xC && MyY >= 0xC && MyY < (INT)Height - 0xC && MyX - 10 <= X &&
        X <= MyX + 10 && MyY - 10 <= Y && Y <= MyY + 10)
        bMyPosHover = 1;
    else
        bMyPosHover = 0;
    if (bShowMark)
    {
        FMinimapTarget T = MarkPos(0);
        INT OutX;
        INT OutY;
        WorldToMap(T.X, T.Y, &OutX, &OutY, MapIndex);
        INT PX = OutX - ScrollX;
        INT PY = OutY - ScrollY;
        if (PX >= 5 && PX <= (INT)Width - 5 && PY >= 5 && PY < (INT)Height - 5 && PX <= X && X <= PX + 0x14 &&
            PY - 0x14 <= Y && Y <= PY)
            bMarkHover = 1;
        else
            bMarkHover = 0;
    }
    ClickX = X;
    ClickY = Y;
    return 1;
}
NW_HOOK_METHOD(0x100aeca0, NCMinimapWnd, OnMouseMove)

// 0x100aeef0 - the cover shows the window: the player's map (the continent's or his town's), the continent switch of
// the cover following it, the player's mark centered (the piece kept inside the map)
void NCMinimapWnd::CenterOnPlayer()
{
    INT PX = (INT)MyLocation().X;
    INT PY = (INT)MyLocation().Y;
    INT PZ = (INT)MyLocation().Z;
    MapIndex = GetMapIndex(PX, PY, PZ);
    if (IsAdenMap(MapIndex))
        Continent = 0;
    else
        Continent = 1;
    ((NCMapCoverWnd*)Entry->Parent->Wnd)->SetContinent(Continent);   // 0x100ABE30: the cover's continent switch
    INT W = GetMapWidth(MapIndex);
    INT H = GetMapHeight(MapIndex);
    if (W > 0 && H > 0)
    {
        if (MapIndex == 0 || MapIndex == 1)
            ZoomOutButton->HideWindow();
        else
            ZoomOutButton->ShowWindow();
        INT MX;
        INT MY;
        GetMyMapPos(&MX, &MY, MapIndex);
        if (MX < 0xA4)
            ScrollX = 0;
        else if (MX > W - 0xA4)
            ScrollX = W - 0x148;
        else
            ScrollX = MX - 0xA4;
        if (MY < 0xA4)
        {
            ScrollY = 0;
            ShowLayer(MapIndex);
            return;
        }
        if (MY > H - 0xA4)
            ScrollY = H - 0x148;
        else
            ScrollY = MY - 0xA4;
        ShowLayer(MapIndex);
    }
}
NW_HOOK_METHOD(0x100aeef0, NCMinimapWnd, CenterOnPlayer)

// 0x100af110 - the cover's continent button: the other continent's map centered on the player's position there
void NCMinimapWnd::ToggleContinent()
{
    if (Continent == 0)
        Continent = 1;
    else if (Continent == 1)
        Continent = 0;
    MapIndex = Continent;
    INT W = GetMapWidth(Continent);
    INT H = GetMapHeight(Continent);
    if (W > 0 && H > 0)
    {
        if (MapIndex == 0 || MapIndex == 1)
            ZoomOutButton->HideWindow();
        else
            ZoomOutButton->ShowWindow();
        INT MX;
        INT MY;
        GetMyMapPos(&MX, &MY, MapIndex);
        if (MX < 0xA4)
            ScrollX = 0;
        else if (MX > W - 0xA4)
            ScrollX = W - 0x148;
        else
            ScrollX = MX - 0xA4;
        if (MY < 0xA4)
        {
            ScrollY = 0;
            ShowLayer(MapIndex);
            return;
        }
        if (MY > H - 0xA4)
            ScrollY = H - 0x148;
        else
            ScrollY = MY - 0xA4;
        ShowLayer(MapIndex);
    }
}
NW_HOOK_METHOD(0x100af110, NCMinimapWnd, ToggleContinent)

// 0x100af250 - show a place of the world: its map (bFollow 0: the continent's scroll kept first; 1: the continent
// only when it is not the one shown), the piece centered on it (the harbor's box has its own mark)
void NCMinimapWnd::ShowPosition(INT X, INT Y, INT Z, UBOOL bFollow)
{
    INT Zone = GetMapIndex(X, Y, Z);
    if (bFollow == 0)
    {
        if (MapIndex == 0 || MapIndex == 1)
        {
            SavedScrollX = ScrollX;
            SavedScrollY = ScrollY;
        }
        MapIndex = Zone;
        if (IsAdenMap(Zone))
            Continent = 0;
        else
            Continent = 1;
    }
    else if (MapIndex != Zone)
    {
        if (IsAdenMap(Zone))
        {
            Continent = 0;
            MapIndex = 0;
        }
        else
        {
            Continent = 1;
            MapIndex = 1;
        }
    }
    ((NCMapCoverWnd*)Entry->Parent->Wnd)->SetContinent(Continent);   // 0x100ABE30: the cover's continent switch
    INT W = GetMapWidth(MapIndex);
    INT H = GetMapHeight(MapIndex);
    if (W > 0 && H > 0)
    {
        if (MapIndex == 0 || MapIndex == 1)
            ZoomOutButton->HideWindow();
        else
            ZoomOutButton->ShowWindow();
        INT PY;
        if (InHarborBox(X, Y) && Z <= -0x130A && Z >= -0x1ADA)
        {
            ScrollX = 0;
            PY = 0x536;
        }
        else
        {
            WorldToMapPixels(&X, &Y, MapIndex);
            PY = Y;
            if (X < 0xA4)
                ScrollX = 0;
            else if (X > W - 0xA4)
                ScrollX = W - 0x148;
            else
                ScrollX = X - 0xA4;
        }
        if (PY < 0xA4)
            ScrollY = 0;
        else if (PY > H - 0xA4)
            ScrollY = H - 0x148;
        else
            ScrollY = PY - 0xA4;
        ShowLayer(MapIndex);
    }
}
NW_HOOK_METHOD(0x100af250, NCMinimapWnd, ShowPosition)

// 0x100af450 - a mark of the console (ShowRadar)
void NCMinimapWnd::AddTarget(INT X, INT Y, INT Z)
{
    INT i = Targets.Add();
    Targets(i).X = X;
    Targets(i).Y = Y;
    Targets(i).Z = Z;
}
NW_HOOK_METHOD(0x100af450, NCMinimapWnd, AddTarget)

// 0x100af490 - every mark at that position goes (the entry after a removed one is skipped, as in the stock loop)
void NCMinimapWnd::RemoveTarget(INT X, INT Y, INT Z)
{
    for (INT i = 0; i < Targets.Num(); i++)
    {
        FMinimapTarget T = Targets(i);
        if (T.X == X && T.Y == Y && T.Z == Z)
            Targets.Remove(i, 1);
    }
}
NW_HOOK_METHOD(0x100af490, NCMinimapWnd, RemoveTarget)

// 0x100af610 - the marks emptied
void NCMinimapWnd::ClearTargets()
{
    Targets.Empty();
}
NW_HOOK_METHOD(0x100af610, NCMinimapWnd, ClearTargets)

// 0x100af4f0 (slot 68) - the zoom out button, on a town's map: back to the continent it is on, the scroll it had, the
// button hidden; the player's position shown when he is on that continent
INT NCMinimapWnd::OnZoomOutButton()
{
    if (MapIndex != 0 && MapIndex != 1)
    {
        if (Continent == 0)
            MapIndex = Continent;
        else if (Continent == 1)
            MapIndex = Continent;
        ScrollY = SavedScrollY;
        ScrollX = SavedScrollX;
        ZoomOutButton->HideWindow();
        INT PX = (INT)GConsole->GetMyLocation().X;
        INT PY = (INT)GConsole->GetMyLocation().Y;
        INT PZ = (INT)GConsole->GetMyLocation().Z;
        if ((IsAdenMap(GetMapIndex(PX, PY, PZ)) && Continent == 0) ||
            (!IsAdenMap(GetMapIndex(PX, PY, PZ)) && Continent == 1))
            ShowPosition(PX, PY, PZ, 1);
        ShowLayer(MapIndex);
    }
    SetFocus();
    return 1;
}
NW_HOOK_METHOD(0x100af4f0, NCMinimapWnd, OnZoomOutButton)

// 0x100afc40 (vslot 37) - the textures, the zoom out button (24 x 24 at (301, 301)) and the tables
INT NCMinimapWnd::OnCreate()
{
    NCWnd::OnCreate();
    MyPosTex = FindTexture(TEXT("L2UI.MinimapWnd.MinimapPos"));
    TargetPosTex = FindTexture(TEXT("L2UI.MinimapWnd.TargetPos"));
    MarkTex = FindTexture(TEXT("L2UI_CH3.Minimap.MapIcon_Mark"));
    BarShadowTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.Bar_Shadow"));
    ShadowTex = FindTexture(TEXT("L2UI_CH3.Minimap.MapShadow"));
    ZoomOutButton = new NCButton(TEXT("L2UI_CH3.Minimap.MapButton_ZoomOut1"),
                                 TEXT("L2UI_CH3.Minimap.MapButton_ZoomOut2"), 0);
    ZoomOutButton->ID = NID_MINIMAP_ZOOMOUT;
    ZoomOutButton->SetPosition(0x12D, 0x12D, 0x18, 0x18);
    CreateChild(ZoomOutButton, NWS_VISIBLE);
    MoonTex = FindTexture(TEXT("L2UI_CH3.Minimap.Map_Moon"));
    SunTex = FindTexture(TEXT("L2UI_CH3.Minimap.Map_Sun"));
    LoadMapData();
    return 1;
}
NW_HOOK_METHOD(0x100afc40, NCMinimapWnd, OnCreate)

// 0x100b0320 (vslot 38) - the buttons go (each leaves its parent's list first), then every array is emptied
INT NCMinimapWnd::OnDestroy()
{
    guard(NCMinimapWnd::OnDestroy);
    INT i;
    if (ZoomOutButton)
    {
        RemoveChildWindow(ZoomOutButton->Entry);
        delete ZoomOutButton;
        ZoomOutButton = NULL;
    }
    for (i = 0; i < TownButtons.Num(); i++)
    {
        if (TownButtons(i))
        {
            RemoveChildWindow(TownButtons(i)->Entry);
            delete TownButtons(i);
            TownButtons(i) = NULL;
        }
    }
    TownButtons.Empty();
    for (i = 0; i < ElmoreButtons.Num(); i++)
    {
        if (ElmoreButtons(i))
        {
            RemoveChildWindow(ElmoreButtons(i)->Entry);
            delete ElmoreButtons(i);
            ElmoreButtons(i) = NULL;
        }
    }
    ElmoreButtons.Empty();
    TownPos.Empty();
    MapTextures.Empty();
    MarkPos.Empty();
    ToolTips.Empty();
    ElmorePos.Empty();
    MapScales.Empty();
    MapOrigins.Empty();
    Targets.Empty();
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b0320, NCMinimapWnd, OnDestroy)

// 0x100de780 - the radar's mark (the stock body at this address serves the radar only: the console's ShowRadar calls
// it on the radar window and 0x100AF450 on the map; the db gave it the map's name)
NW_HOOK_METHOD(0x100de780, NCRadarWnd, AddTarget)

// 0x100abeb0 (vslot 30) - a drag in progress lets the cursor go. That body is the slot of the stock NCMapCoverWnd and
// NTownMapWnd vtables; the stock NCMinimapWnd vtable (0x10190f30) has NCWnd::KillFocus (0x1002fd50) at slot 30. The hook
// build keeps this override (it is where 0x100abeb0 is hooked); the standalone slot is NCWnd's, as the stock one
#if NW_HOOK_MODE
void NCMinimapWnd::KillFocus()
{
    UnlockCursor();
    NCWnd::KillFocus();
}
NW_HOOK_METHOD(0x100abeb0, NCMinimapWnd, KillFocus)
#else
NW_SLOT_IS("?KillFocus@NCMinimapWnd@@UAEXXZ", "?KillFocus@NCWnd@@UAEXXZ")   // NCMinimapWnd::KillFocus = NCWnd::KillFocus (0x1002fd50)
#endif

// the destructor instances of the minimap's arrays (the stock compiler emitted each out of line; 0x100af8a0 is the
// radar window's too)
typedef TArray<FMinimapTarget> TMinimapTargetArray;
typedef TArray<FMinimapScale> TMinimapScaleArray;
typedef TArray<NCButton*> TMinimapButtonArray;
typedef TArray<NToolTipInfo> TMinimapToolTipArray;
// 0x100af8a0 - 12 byte elements (the town, mark and target positions)
NW_HOOK_DTOR(0x100af8a0, TMinimapTargetArray)
// 0x100af930 - 4 byte elements (the textures, the buttons)
NW_HOOK_DTOR(0x100af930, TMinimapButtonArray)
// 0x100af9c0 - 8 byte elements (the scales)
NW_HOOK_DTOR(0x100af9c0, TMinimapScaleArray)
// 0x100afd90 (ret 8) - Remove(Index, Count) of the tool tips: each element's destructor, then the engine's Remove
NW_HOOK_METHOD(0x100AFD90, TMinimapToolTipArray, Remove)
// 0x100b0060 - the tool tips' array: all of them removed
NW_HOOK_DTOR(0x100b0060, TMinimapToolTipArray)

NW_CHECK_VTABLE(NCMinimapWnd, 0x10190f30, 71)
