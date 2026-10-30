#pragma once
// NCEquipItemWnd - the inventory's paperdoll (UClass "CEquipItemWnd", 0x254; built like the
// item windows but on NCWnd): the equipped items in 14 slots of a 5 x 3 grid, 35 pixels apart
// (the console tells the slot of each item: NConsoleWnd::GetEquipSlot; 14 and 15 are the
// two-handed weapon and the full armor, drawn over two slots). A double click or the right
// button takes an item off; dragged onto the inventory's page it is taken off too, onto the
// shortcut bar it gets a shortcut, onto the pet inventory it is given to the pet, onto the
// trash / crystallize icons or the ground it asks first (NWM_DIALOG_RESULT answers).

#include "nwindow/NCItemWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

// A tattoo of the paperdoll (name ours; 0x1A44 bytes, `new` in NCEquipItemWnd::AddHenna 0x1008CF00): an item entry
// (its icon, name and tool tip come from the tattoo's FL2HennaData) and the box it is drawn in and hit-tested by
// (an NRectF by value: PtInRect, vslot 40), relative to the window: x 216, y 27 per slot + 8, 24.0 x 24.0.
struct NHennaEntry : public NItemEntry
{
    NRectF Box;                // +0x1A34 (the constructor clears Y first)

    NHennaEntry()
    {
        Box.Y = 0;
        Box.X = 0;
        *(INT*)&Box.W = 0;
        *(INT*)&Box.H = 0;
    }
};
NW_ASSERT_OFFSET(NHennaEntry, Box, 0x1A34);
NW_ASSERT_SIZE(NHennaEntry, 0x1A44);

class NCEquipItemWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCEquipItemWnd, NCWnd, 0, 0x10283108)

    // C2 layout (sizeof 0x250): from the constructor 0x1007F730, OnCreate 0x1007B400 and the other functions
    INT SelectedIndex;               // +0xC8 (-1) index in Items of the item picked up
    UBOOL bNoDrag;                   // +0xCC the next click selects without picking up
    TArray<NItemEntry*> Items;       // +0xD0
    TArray<NHennaEntry*> Hennas;     // +0xDC new in C2 (name ours): the tattoos, drawn in a column of their own
                                     //   (OnPaint: boxes of 26 pixels at x = 108, 27 apart)
    BYTE _UnknownB8[0xF4 - 0xB8];    // +0xE8 (new in C2; the constructor leaves it)   // C2: offset derived, no evidence
    // C3 (OnCreate 0x100887d0): the textures are 8 bytes lower than C2's block for the selection
    UTexture* SelectTex;             // +0x124 "l2ui.NWindow.item_click" the selected item's frame
    UTexture* OutlineDownTex;        // +0x128 "l2ui_ch3.InventoryWnd.Inventory_OutLine_down" the frame under the others
    UTexture* OutlineTex;            // +0x12C "l2ui_ch3.InventoryWnd.Inventory_OutLine" and the one over them
    UTexture* ToolTipTex;            // +0x130 "L2UI.tooltip.mini-tool01"
    UTexture* DualCapTex;            // +0x134 "L2UI.InventoryWnd.Icon_dualcap" over the second
                                     //   slot of a two-slot item
    UTexture* HennaBackTex;          // +0x138 "L2UI_CH3.InventoryWnd.Inventory_Mark" behind a tattoo
    UBOOL bDragging;                 // +0x13C the selected item is being carried
    UBOOL bHasUnknown1C98;           // +0x140 an item with NItemEntry::Unknown1C98 is equipped
                                     //   (AddItem): a bow is then drawn in slot 5
    INT Unknown104;                  // +0x144 (5)   // C2: offset derived, no evidence
    INT Unknown108;                  // +0x148 (3)   // C2: offset derived, no evidence
    INT Unknown10C;                  // +0x14C nothing reads it (the constructor leaves it)   // C2: offset derived, no evidence
    INT Unknown110;                  // +0x150 nothing reads it   // C2: offset derived, no evidence
    INT NumSlots;                    // +0x154 (15 in C3: slot 15 and 16 are the two-slot items) also the most items it keeps
    INT CellSize;                    // +0x158 (constructor's argument, 32)   // C2: offset derived, no evidence
    INT Selected;                    // +0x15C slot, -1 = none
    INT HoverSlot;                   // +0x160 slot under the mouse, -1 = none
    UBOOL bEnchantMode;              // +0x164 (SetEnchantMode; the constructor clears it)
    FVector DropLocation;            // +0x168 the ground point under the mouse when an item is let go over the game   // C2: offset derived, no evidence
                                     //   (OnLButtonUp: UGameEngine::MouseHitLocation; the drop question carries its own copy)
    BYTE _Unknown13C[0x24C - 0x13C]; // +0x174 nothing reads or writes it (C1's pending-question flags are gone)   // C2: offset derived, no evidence
    INT HoverHenna;                  // +0x284 (-1) the tattoo under the mouse (name ours)   // C2: offset derived, no evidence

    NCEquipItemWnd(INT InCellSize = 32);                                  // 0x1008d740
    // ~NCEquipItemWnd                                               // 0x10083fd0 (the two arrays, the base)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10088770
    virtual INT OnCreate();                                          // 37 0x100887d0
    virtual INT OnDestroy();                                         // 38 0x1008cd60
    virtual void ShowWindow();                                       // 44 0x100412c0 (folded)
    virtual void HideWindow();                                       // 45 0x100412d0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10088a40
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x1008d7f0
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x1008a7b0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x1008a620
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1008a9d0
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 64 0x1008b480
    // names ours
    virtual INT GetEquipSlot(INT BodyPart, INT ObjectID);            // 68 0x10088860 NConsoleWnd::GetEquipSlot
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 69 0x1008b900

    // names ours
    void SetEnchantMode(UBOOL bOn);                                  // 0x10088850
    void UnselectAll();                                              // 0x100890b0
    INT HitTest(INT X, INT Y);                                       // 0x100890e0 slot; -1 outside (C1: 0)
    INT HitTestHenna(INT X, INT Y);                                  // 0x10089200 the tattoo, -1 outside (name ours; stock)
    UBOOL UpdateItem(NItemInfo* Info);                               // 0x10089290 0: not here
    __declspec(noinline) UBOOL HasItem(INT ObjectID);                                     // 0x10089390
    __declspec(noinline) UBOOL FindItem(INT ObjectID, INT* Index);                        // 0x100893e0
    UBOOL AddItem(NItemInfo* Info);                                  // 0x1008bb60 0 when full
    UBOOL RemoveItem(INT ObjectID);                                  // 0x1008ce30 deletes the item with this object id
    void AddHenna(INT HennaID, INT bUsable);                         // 0x1008cf00 (names ours; the console's slot 137 goes through
                                                                     //   NCInventoryWnd::AddHenna 0x10080250) a tattoo from the game data
    void ClearHennas();                                              // 0x1008e350 (the console's slot 136, NCInventoryWnd 0x100807a0)
    void Clear();                                                    // 0x1008cea0
};
NW_ASSERT_OFFSET(NCEquipItemWnd, Items, 0xD0);
NW_ASSERT_OFFSET(NCEquipItemWnd, Hennas, 0xDC);
NW_ASSERT_OFFSET(NCEquipItemWnd, SelectTex, 0x124);
NW_ASSERT_OFFSET(NCEquipItemWnd, OutlineTex, 0x12C);
NW_ASSERT_OFFSET(NCEquipItemWnd, HennaBackTex, 0x138);
NW_ASSERT_OFFSET(NCEquipItemWnd, NumSlots, 0x154);
NW_ASSERT_OFFSET(NCEquipItemWnd, bEnchantMode, 0x164);
NW_ASSERT_OFFSET(NCEquipItemWnd, HoverHenna, 0x284);
NW_ASSERT_SIZE(NCEquipItemWnd, 0x288);

#pragma pack(pop)