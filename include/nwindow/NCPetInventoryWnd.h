#pragma once
// NCPetInventoryWnd - the Items page of the pet window (UClass "CPetInventoryWnd", 0x9C): the
// pet's inventory in an NCPetInvenItemWnd grid, 6 x 2 icons of 32 pixels (160 at most). The
// console fills it through NCPetWnd (AddInventoryItem / UpdateInventoryItem / ClearInventory).
//
// NCPetInvenItemWnd - that grid (UClass "CPetInvenItemWnd", 0x134): equipped items get a frame
// behind their icon; a double click or the right button uses an item (the pet equips / eats
// it), dragging it inside the grid changes its place, onto the player's inventory or paperdoll
// gives it back (how many, for a stack), onto the shortcut bar registers it. The tool tips are
// the inventory's (NItemToolTip.h).

#include "nwindow/NCItemWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCPetInvenItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCPetInvenItemWnd, NCItemWnd, 0, 0x1029AE90)

    UTexture* EquippedTex;           // +0x148 "l2ui_ch3.PetWnd.petitem_click" over equipped items (C3 draws it over the icon)
    // the questions asked last (names ours; OnLButtonUp sets them)
    UBOOL bDropPending;              // +0x14C on the ground (nothing here sets it: the   // C2: offset derived, no evidence
                                     //   inventory's code, kept)
    UBOOL bToOwnerPending;           // +0x150 back to the player   // C2: offset derived, no evidence
    UBOOL bDblClicked;               // +0x154 the release after a double click moves nothing
    UTexture* OutlineDownTex;        // +0x158 "l2ui_ch3.InventoryWnd.Inventory_OutLine_down" (loaded, never drawn)
    UTexture* OutlineTex;            // +0x15C "l2ui_ch3.InventoryWnd.Inventory_OutLine" around every icon
    FVector DropLocation;            // +0x160 (never written here: the constructor leaves it)

    NCPetInvenItemWnd();                                             // 0x100cf3b0
    NCPetInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x100cf460
    // ~NCPetInvenItemWnd                                            // 0x100b6210 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100cba90
    virtual INT OnCreate();                                          // 37 0x100cbaf0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100cc7a0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100cc9c0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100ceb00
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 64 0x100ccae0
    virtual void Clear();                                            // 72 0x100cf150
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x100cca50 WM_LBUTTONDBLCLK

    // names ours
    __declspec(noinline) UBOOL AddItem(NItemInfo* Info);             // 0x100ceef0 (C1 0x10093e50) 0 when full
    UBOOL UpdateItem(NItemInfo* Info);                               // 0x100ccb40 0: not here
    __declspec(noinline) UBOOL RemoveItem(INT ObjectID);                                  // 0x100cf0e0 (NCTradeItemWnd's too)
    __declspec(noinline) UBOOL HasItem(INT ObjectID);                                     // 0x100ccc40
    __declspec(noinline) UBOOL FindItem(INT ObjectID, INT* Index);                        // 0x100ccc90
};
NW_ASSERT_OFFSET(NCPetInvenItemWnd, EquippedTex, 0x148);
NW_ASSERT_OFFSET(NCPetInvenItemWnd, OutlineTex, 0x15C);
NW_ASSERT_OFFSET(NCPetInvenItemWnd, DropLocation, 0x160);
NW_ASSERT_SIZE(NCPetInvenItemWnd, 0x16C);

class NCPetInventoryWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetInventoryWnd, NCWnd, 0, 0x1029B8D8)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.PetWnd.Petinterface_back33"
    NCPetInvenItemWnd* Items;        // +0xCC

    NCPetInventoryWnd();                                             // 0x100cf4b0
    // ~NCPetInventoryWnd                                            // 0x100b36a0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x100cf550
    virtual INT OnDestroy();                                         // 38 0x100ccce0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // C2: slot gone (was 50) (folded)

    // names ours (NCPetWnd passes the console's calls on)
    UBOOL AddItem(NItemInfo* Info);                                  // 0x100cefe0 (C1 0x10094010)
    UBOOL UpdateItem(INT Op, NItemInfo* Info);                       // 0x100cf1b0 (C1 0x10094030) 1 add, 2 change,
                                                                     //   3 remove
    NItemEntry* GetItem(INT ObjectID);                               // 0x100ccd90 NULL: none
    void ClearItems();                                               // 0x100b2b90 (C1 only: no C2 match) (C1 0x100bab50) (shared with
                                                                     //   NCTrainListWnd's Clear)
};
NW_ASSERT_OFFSET(NCPetInventoryWnd, Items, 0xCC);
NW_ASSERT_SIZE(NCPetInventoryWnd, 0xD0);

#pragma pack(pop)