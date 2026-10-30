#pragma once
// NCMapCoverWnd - the world map window (UClass "CMapCoverWnd", 0xDC; new in C3; opened by NCConsole's map button):
// the frame around an NCMinimapWnd (the map itself, 328 x 328 pixels under a title bar) with a button that shows the
// mark the console reports, a button that centers the map on the player and a combo box that picks the continent
// (Aden / Elmore: it only toggles the map's Continent). The window is created minimized into an icon like the
// others of the main window (NCWnd::Minimize with the menu icon 0x1BF).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCComboBox;
class NCMinimapWnd;

class NCMapCoverWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMapCoverWnd, NCWnd, 0, 0x10290E78)

    NCMinimapWnd* Map;               // +0xC8 the map (child, 3, 21)
    UTexture* BackTex;               // +0xCC "L2UI_CH3.Minimap.MapBack" (the constructor clears it, 0x14 pixels down)
    NCButton* MarkButton;            // +0xD0 id 0xB000F5, SysString 886 (name ours): shows the console's mark
    NCButton* MyPosButton;           // +0xD4 id 0xB00F00, SysString 887 (name ours): the player's place
    NCComboBox* ContinentCombo;      // +0xD8 id 0x1F21 (name ours): "<885> - <888>", "<885> - <889>"

    NCMapCoverWnd();                                                 // 0x100afe30
    // ~NCMapCoverWnd                                                // 0x100afe80

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100abd90
    virtual void KillFocus();                                        // 30 0x100abeb0 (NCMinimapWnd's body, folded: the
                                                                     //   cursor lock goes, then NCWnd's)
    virtual INT OnCreate();                                          // 37 0x100b05d0
    virtual INT OnDestroy();                                         // 38 0x100ac6d0
    virtual void ShowWindow();                                       // 44 0x100afbb0
    virtual void Minimize();                                         // 49 0x100abe50 (the db says MinimizeWindow)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100ac630
    // names ours: the three controls' commands
    virtual INT OnShowMark();                                        // 68 0x100afb70 (0xB000F5)
    virtual INT OnShowMyPosition();                                  // 69 0x100afa50 (0xB00F00)
    virtual INT OnContinent(INT Index);                              // 70 0x100afb40 (0x1F21: the combo's item)

    // 0x100abe30 (ret 4; anonymous in the db, name ours): the combo box shows the continent (0 or 1; the map's own switch
    // calls it on its parent); another number is ignored
    void SetContinent(INT Continent);
};
NW_ASSERT_OFFSET(NCMapCoverWnd, Map, 0xC8);
NW_ASSERT_OFFSET(NCMapCoverWnd, BackTex, 0xCC);
NW_ASSERT_OFFSET(NCMapCoverWnd, ContinentCombo, 0xD8);
NW_ASSERT_SIZE(NCMapCoverWnd, 0xDC);

#pragma pack(pop)
