#pragma once
// NCRecipeGaraTree - the ingredient tree of the recipe information window (UClass "CRecipeGaraTree", 0x208; C2: NCRecipeTree
// on the NCTree view control): an NCGaraTree that draws each entry as a 32 x 32 frame with the ingredient's icon, its name
// and "(have/need)", a plus / minus sign for an entry with children and a dimming square for an ingredient the player lacks.
// NCRecipeGaraTreeCtrl (the scrolling frame) owns it; NCRecipeGaraTreeWnd shows both.
//
// The entries are NCGaraTVItem. What the recipe code stores in the record NGaraTVItemData (names after their use here):
// IconSet is how many the recipe needs, LevelMin how many the player carries.

#include "nwindow/NCGaraTree.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCRecipeGaraTree : public NCGaraTree
{
    NW_DECLARE_CLASS(NCRecipeGaraTree, NCGaraTree, 0, 0x102A6CF8)

    UTexture* FrameTex;              // +0x1EC "L2UI.RecipeWnd.RecipeTreeIconBack": an entry with children, collapsed
    UTexture* FrameOpenTex;          // +0x1F0 "L2UI.RecipeWnd.RecipeTreeIconBack_click": expanded
    UTexture* FrameLeafTex;          // +0x1F4 "L2UI.RecipeWnd.RecipeTreeIconDisableBack": a leaf
    UTexture* PlusTex;               // +0x1F8 "L2UI.RecipeWnd.TreePlus"
    UTexture* MinusTex;              // +0x1FC "L2UI.RecipeWnd.TreeMinus"
    BYTE _Unknown200[0x208 - 0x200]; // +0x200 never touched

    NCRecipeGaraTree();                                              // 0x100E5960
    // ~NCRecipeGaraTree                                             // 0x100E59B0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10041760 (folded with NCExButton's)
    virtual INT OnCreate();                                          // 37 0x100E26A0
    virtual INT OnDestroy();                                         // 38 0x100E2710
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100E38E0
    // 72 0x100E39D0 (ret 0x10): draws Item at (X, Y) and its children below / beside it and returns the height used
    virtual INT DrawItem(NCGDevice* Device, NCGaraTVItem* Item, INT X, INT Y);
};
NW_ASSERT_OFFSET(NCRecipeGaraTree, FrameTex, 0x1EC);
NW_ASSERT_OFFSET(NCRecipeGaraTree, MinusTex, 0x1FC);
NW_ASSERT_SIZE(NCRecipeGaraTree, 0x208);

#pragma pack(pop)
