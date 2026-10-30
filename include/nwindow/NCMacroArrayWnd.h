#pragma once
// NCMacroArrayWnd - the grid of the macros in NCMacroListWnd (C2 only; UClass "CMacroArrayWnd", 0x150): an item window
// whose cells are the macros' icons (6 columns, 32 pixel cells 37 apart across and 35 down from 4 pixels in; the cells
// are painted from the parent's NMacroEntry array, not from NItemEntry records of its own) with the macro's short name
// over the icon (up to four characters, two by two), an outline round each icon and its cool time over it. A click
// selects a macro (and picks the icon up, as in every item window); letting go on the same cell runs the macro, on the
// delete / edit icons of the list window deletes / edits it, on a shortcut slot puts it there.
//
// Layout from the C3 constructors 0x100A00A0 / 0x100A0140, OnCreate 0x1009B320 and the destructor 0x100A00F0.

#include "nwindow/NCItemWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

// the dialog that asks whether to delete the macro (NDialogInfo::ID; name ours)
enum { DIALOG_MACRO_DELETE = 0x44 };

class NCMacroArrayWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCMacroArrayWnd, NCItemWnd, 0, 0x1028B1C0)

    UTexture* OutlineDownTex;        // +0x148 "l2ui_ch3.InventoryWnd.Inventory_OutLine_down": round the selected macro (OnCreate)
    UTexture* OutlineTex;            // +0x14C "l2ui_ch3.InventoryWnd.Inventory_OutLine": round every icon

    NCMacroArrayWnd();                                               // 0x100A00A0
    NCMacroArrayWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize);  // 0x100A0140 (ret 0x10; the item window's gaps 1, 1)
    // ~NCMacroArrayWnd                                              // 0x100A00F0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100DC990 (NCQuestItemWnd's, shared: the dialog's answer goes to vslot 75; a stub here)
    virtual INT OnCreate();                                          // 37 0x1009B320
    virtual INT OnDestroy();                                         // 38 0x1009B360
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1009CB50
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x1009FAF0 (named OnPostPaint in the database)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x1009CF60
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x1009CED0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1009D1B0
    virtual void UnselectAll();                                      // 73 0x1009D6F0
    // names ours
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 75 0x1009D090 the answer to the delete question

    // names ours
    __declspec(noinline) INT HitCell(INT X, INT Y);                                       // 0x1009C8A0 (ret 8) the cell (0..41) under a point, -1 none
};
NW_ASSERT_OFFSET(NCMacroArrayWnd, OutlineDownTex, 0x148);
NW_ASSERT_OFFSET(NCMacroArrayWnd, OutlineTex, 0x14C);
NW_ASSERT_SIZE(NCMacroArrayWnd, 0x150);

#pragma pack(pop)
