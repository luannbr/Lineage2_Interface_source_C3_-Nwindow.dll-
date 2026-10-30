#pragma once
// NCRecipeBookWnd - the recipe book (UClass "CRecipeBookWnd", 0x170; new in C2): the grid of the recipes the
// character knows (NCRecipeBookItemWnd, "(n/50)" in the corner) and a trash icon to forget one. The console fills
// it when the server sends the book (RequestRecipeBookOpen asks for it) through AddRecipe.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCRecipeBookItemWnd;
struct NItemInfo;

class NCRecipeBookWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCRecipeBookWnd, NCWnd, 0, 0x102A67D8)

    UTexture* BackTex;                // +0xC8 "L2UI_CH3.RecipeWnd.Recipe2_Back"
    UTexture* TrashTex;               // +0xCC "L2UI_CH3.InventoryWnd.Inventory_trash"
    UTexture* TrashActiveTex;         // +0xD0 "L2UI_CH3.InventoryWnd.Inventory_trash_Drag" while the drag is over it
    NToolTipInfo TrashToolTip;        // +0xD4 (C3) the trash icon's tool tip (SysString 890, style 0x400; OnCreate)
    NCRecipeBookItemWnd* Items;       // +0x150 (name ours) the grid
    INT TrashX;                       // +0x154 (name ours) the trash icon's place in the window (0xD0)
    INT TrashY;                       // +0x158 (0x15B)
    FLOAT TrashW;                     // +0x15C (34.0)
    FLOAT TrashH;                     // +0x160 (34.0)
    UBOOL bTrashActive;               // +0x164 (name ours) the grid's OnMouseMove sets it: a recipe is dragged over the icon
    UBOOL bOverTrash;                 // +0x168 (name ours; new in C3) the mouse is over the icon (OnMouseMove): its tool tip shows
    INT Unknown0BC;                   // +0x16C (name ours) the constructor and Clear zero it, nothing here reads it

    NCRecipeBookWnd();                                               // 0x100E4C00
    // ~NCRecipeBookWnd                                              // 0x100E4CC0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (NControl's, shared, hooked there; a stub here)
    virtual INT OnCreate();                                          // 37 0x100E4D20
    virtual INT OnDestroy();                                         // 38 0x100E2B40
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100E2B80
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x100E4EC0 (the db's OnPostPaint)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x100E2410

    // names ours (the console calls them: slots 123 and 124)
    void Clear();                                                    // 0x100E24C0 the grid emptied
    UBOOL HitTrash(INT X, INT Y);                                    // 0x100E24E0 (ret 8; sub_ in the db) the point is on the trash icon (the
                                                                     //   bounds are inclusive, the window's screen origin added)
    INT AddRecipe(NItemInfo* Info);                                  // 0x100E4530 into the grid, always 1
};
NW_ASSERT_OFFSET(NCRecipeBookWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCRecipeBookWnd, TrashToolTip, 0xD4);
NW_ASSERT_OFFSET(NCRecipeBookWnd, Items, 0x150);
NW_ASSERT_OFFSET(NCRecipeBookWnd, bTrashActive, 0x164);
NW_ASSERT_OFFSET(NCRecipeBookWnd, bOverTrash, 0x168);
NW_ASSERT_OFFSET(NCRecipeBookWnd, Unknown0BC, 0x16C);
NW_ASSERT_SIZE(NCRecipeBookWnd, 0x170);

#pragma pack(pop)
