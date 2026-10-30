#pragma once
// NCRecipeTreeCtrl - the scrolling frame of the recipe tree (UClass "CRecipeTreeCtrl", 0xE8): an NCTreeCtrl (a scroll window
// with a tree in it) that keeps its own NCRecipeTree in +0xE4 instead of NCTreeCtrl's +0xD0 (the C2 source duplicated the
// base class' code with the new tree; every override here is NCTreeCtrl's body on the field at +0xE4). The tree is made
// as wide as the frame less its slider (16 pixels); the scroll bar moves the tree window up and down (MoveWindow) and the
// tree keeps how far in its +0x1B8 (ScrollOffset). Paint resizes the frame's scroll range from what the tree measured
// (+0x1B4 + 21) and asks for the entries to be shown or hidden by their position (vslot 71).
//
// NCTreeCtrl belongs to another group (w2newui: nwindow/NCTreeCtrl.h): until that header is in the tree a stand-in with the same
// layout and names declares it (see NCRecipeTree.h for the same arrangement); __has_include picks the real one.

#include "nwindow/NCScrollWnd.h"
#include "nwindow/NCRecipeTree.h"

#if defined(__has_include)
#if __has_include("nwindow/NCTreeCtrl.h")
#define NW_HAVE_NCTREECTRL_H 1
#endif
#endif

#ifdef NW_HAVE_NCTREECTRL_H
#include "nwindow/NCTreeCtrl.h"
#else
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets
// ---- stand-in for nwindow/NCTreeCtrl.h (w2newui) ----
class NCTreeCtrl : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCTreeCtrl, NCScrollWnd, 0, 0x10270B58)

    NCTree* Tree;                    // +0x100
    BYTE _Unknown104[0x10];  // +0x104 (a C1 pad, its C2 extent)

    virtual INT OnCreate() = 0;                                      // 37 0x1002f940
    virtual INT OnDestroy() = 0;                                     // 38 0x1002e980
    virtual INT OnPaint(NCGDevice* Device) = 0;                      // 56 0x1002ea30
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam) = 0;      // 67 0x1002ed20
    virtual INT ScrollUp() = 0;                                      // 69 0x1002eb10
    virtual INT ScrollDown() = 0;                                    // 70 0x1002ebd0
    virtual INT OnScrollBar(INT Value) = 0;                          // 71 0x1002eca0
    virtual void Clear() = 0;                                        // 72 0x1002ee50
    void ShowVisible(NCTVItem* Item);                    // C3: slot gone (was 71), no longer virtual
};
NW_ASSERT_OFFSET(NCTreeCtrl, Tree, 0x100);
NW_ASSERT_SIZE(NCTreeCtrl, 0x114);
#pragma pack(pop)
#endif

#pragma pack(push, 4)

// C3: class NCRecipeTreeCtrl gone (no counterpart): class NCRecipeTreeCtrl : public NCTreeCtrl
// C3: class NCRecipeTreeCtrl gone (no counterpart): {
// C3: class NCRecipeTreeCtrl gone (no counterpart):     NW_DECLARE_CLASS(NCRecipeTreeCtrl, NCTreeCtrl, 0, 0x101D71C0)

// C3: class NCRecipeTreeCtrl gone (no counterpart):     NCRecipeTree* RecipeTree;        // +0xE4 the tree control, id 0xA00, (0, 0) with the frame's width less 16 (name ours; NCTreeCtrl's own
// C3: class NCRecipeTreeCtrl gone (no counterpart):                                      //   Tree at +0xD0 stays NULL)

// C3: class NCRecipeTreeCtrl gone (no counterpart):     NCRecipeTreeCtrl();                                              // 0x100c5d70 (NCTreeCtrl's constructor 0x1002a9a0, the vtable, +0xCC = 16)
// C3: class NCRecipeTreeCtrl gone (no counterpart):     // ~NCRecipeTreeCtrl                                             // 0x100c5dc0

// C3: class NCRecipeTreeCtrl gone (no counterpart):     virtual INT OnCreate();                                          // 36 0x100c5e10
// C3: class NCRecipeTreeCtrl gone (no counterpart):     virtual INT OnDestroy();                                         // 37 0x100c5ed0
// C3: class NCRecipeTreeCtrl gone (no counterpart):     virtual INT OnPaint(NCGDevice* Device);                          // 55 0x100c5f10
// C3: class NCRecipeTreeCtrl gone (no counterpart):     virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 66 0x100c62c0
// C3: class NCRecipeTreeCtrl gone (no counterpart):     virtual INT ScrollUp();                                          // 67 0x100c60b0
// C3: class NCRecipeTreeCtrl gone (no counterpart):     virtual INT ScrollDown();                                        // 68 0x100c6170
// C3: class NCRecipeTreeCtrl gone (no counterpart):     virtual INT OnScrollBar(INT Value);                              // 69 0x100c6240
// C3: class NCRecipeTreeCtrl gone (no counterpart):     virtual void Clear();                                            // 70 0x100c5fa0 the tree emptied and the view back at the top
// C3: class NCRecipeTreeCtrl gone (no counterpart):     virtual void ShowVisible(NCTVItem* Item);                        // 71 0x100c6010 an entry shown when it is inside the frame, hidden when not; its children too

// C3: class NCRecipeTreeCtrl gone (no counterpart):     // 0x100c6af0 (ret 0x10; name ours): the recipe that makes ItemID at SuccessRate percent, as entries under Parent: the item's
// C3: class NCRecipeTreeCtrl gone (no counterpart):     // entry (icon, name, description, how many are needed and how many the first inventory page holds) and, for each
// C3: class NCRecipeTreeCtrl gone (no counterpart):     // ingredient, the same for the recipe that makes it at 100 percent (down to the items nobody makes). Without a
// C3: class NCRecipeTreeCtrl gone (no counterpart):     // recipe (or with an empty record) the entry is the item's own record and a leaf.
// C3: class NCRecipeTreeCtrl gone (no counterpart):     void AddRecipe(INT ItemID, INT SuccessRate, INT Need, NCTVItem* Parent);
// C3: class NCRecipeTreeCtrl gone (no counterpart): };
// C3: class NCRecipeTreeCtrl gone (no counterpart): NW_ASSERT_OFFSET(NCRecipeTreeCtrl, RecipeTree, 0xE4);
// C3: class NCRecipeTreeCtrl gone (no counterpart): NW_ASSERT_SIZE(NCRecipeTreeCtrl, 0xE8);

#pragma pack(pop)
