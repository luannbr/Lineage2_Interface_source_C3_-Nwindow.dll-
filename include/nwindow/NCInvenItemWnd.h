#pragma once
// NCInvenItemWnd - a page of the inventory's items (UClass "CInvenItemWnd", 0x13C; the
// inventory has two: "Item" and "Quest", 6 x 4 icons of 32 pixels, 160 at most). An item is
// used with a double click or the right button; dragged inside the page it changes place,
// onto the paperdoll it is equipped, onto the shortcut bar it gets a shortcut, onto the pet
// window it is given to the pet, onto the inventory's trash / crystallize icons or the ground
// it asks first (the answer comes back as NWM_DIALOG_RESULT). With an enchant scroll in use
// (SetEnchantMode) the next item released is the one to enchant.

#include "nwindow/NCItemWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCInvenItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCInvenItemWnd, NCItemWnd, 0, 0x10283628)

    // C2 (sizeof 0x124): the four "question pending" flags of C1 are gone; the questions go through
    // NDialogInfo / NConsoleWnd::ShowDialog and the answers come back through the dialog box itself
    UBOOL bEnchantMode;              // +0x148 (SetEnchantMode) the cursor stays the scroll's   // C2: offset derived, no evidence
    UBOOL bDblClicked;               // +0x14C the release after a double click moves nothing
    FVector DropLocation;            // +0x150 the ground point under the mouse (OnLButtonUp: UGameEngine::MouseHitLocation +0x200)
    UTexture* OutlineDownTex;        // +0x15C "l2ui_ch3.InventoryWnd.Inventory_OutLine_down" (the 6-argument constructor)
    UTexture* OutlineTex;            // +0x160 "l2ui_ch3.InventoryWnd.Inventory_OutLine": the frames of the icons that are not selected

    NCInvenItemWnd();                                                // 0x1008da50
    // C3: the cell gaps too (ret 0x18)
    NCInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x1008da70
    // ~NCInvenItemWnd                                               // 0x100782e0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100cba90 (folded with NCPetInvenItemWnd's)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10089a80
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x10089ca0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100896a0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1008bd10
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 64 0x10089430
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x10089730 (new in C2) WM_LBUTTONDBLCLK

    // names ours
    void SetEnchantMode(UBOOL bOn);                                  // 0x100d8aa0 (C1 only: no C2 match) (folded with NCStoreItemWnd::SetMode: not hooked here)
    UBOOL UpdateItem(NItemInfo* Info);                               // 0x10089950 0: not here
    INT GetAdena();                                                  // 0x10089a50
    UBOOL AddItem(NItemInfo* Info);                                  // 0x1008c830 0 when full
    UBOOL RemoveItem(INT ObjectID);                                  // 0x1008d070
    // NCPetInvenItemWnd's bodies (0x100b35b0, 0x100b3600): unhooked copies
    __declspec(noinline) UBOOL HasItem(INT ObjectID);
    __declspec(noinline) UBOOL FindItem(INT ObjectID, INT* Index);
};
NW_ASSERT_OFFSET(NCInvenItemWnd, bEnchantMode, 0x148);
NW_ASSERT_OFFSET(NCInvenItemWnd, bDblClicked, 0x14C);
NW_ASSERT_OFFSET(NCInvenItemWnd, DropLocation, 0x150);
NW_ASSERT_SIZE(NCInvenItemWnd, 0x164);

#pragma pack(pop)