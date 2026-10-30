#pragma once
// NCHennaListWnd - the tattoo lists of the symbol maker (UClass "CHennaListWnd", 0xAC): the tattoos that can be drawn
// (Mode 0, the title "Select the symbol to add") or taken off (Mode 1), in an NCHennaListBox, and the player's adena
// at the bottom. A click on a row asks the server for its information (RequestHennaItemInfo / RequestHennaUnequipInfo):
// the answer fills NCHennaInfoWnd. The console fills it (NConsoleWnd's slots 135..144: it keeps this window at
// NCConsole +0x14C and the information window at +0x150).

#include "nwindow/NCHennaListBox.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

// NWM_COMMAND ids (names ours)
enum
{
    NID_HENNALIST_FRAME = 0xF01,
    NID_HENNALIST_LIST = 0x1A06,
};

class NCHennaListWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCHennaListWnd, NCWnd, 0, 0x10282BD8)

    NCHennaListBox* ListBox;         // +0xC8 100 rows kept, 5 seen
    UTexture* BackTex;               // +0xCC "L2UI.HennaWnd.HennaListBack"
    UTexture* AdenaTex;              // +0xD0 "L2UI.WindowIcon.WindowIconAdena"   // C2: offset derived, no evidence
    INT Mode;                        // +0xD4 0: to add, 1: to take off (set by the console; not by the constructor)
    INT Adena;                       // +0xD8 the player's adena, shown at the bottom (the console sets it)

    NCHennaListWnd();                                                // 0x100883b0
    // ~NCHennaListWnd                                               // 0x10079900 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10086340
    virtual INT OnCreate();                                          // 37 0x10088460
    virtual INT OnDestroy();                                         // 38 0x10086ba0
    virtual void ShowWindow();                                       // 44 0x100863c0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10086c50 (the db names it NCRecipeListWnd's: copied code)
    // names ours
    virtual INT OnSelect(INT Index);                                 // 68 0x10086e00 a row clicked

    // names ours (the console calls them)
    void Clear();                                                    // 0x100863a0 the rows and the adena
    void AddEntry(const NHennaListEntry* Src);                       // 0x10087fe0 (a tail jump into the list box's)
};
NW_ASSERT_OFFSET(NCHennaListWnd, BackTex, 0xCC);
NW_ASSERT_OFFSET(NCHennaListWnd, Mode, 0xD4);
NW_ASSERT_OFFSET(NCHennaListWnd, Adena, 0xD8);
NW_ASSERT_SIZE(NCHennaListWnd, 0xDC);

#pragma pack(pop)
