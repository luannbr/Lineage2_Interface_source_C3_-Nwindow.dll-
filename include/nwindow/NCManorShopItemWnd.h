#pragma once
// NCManorShopItemWnd - the item grids of the manor's shop (UClass "CManorShopItemWnd", 0x11C; NCManorShopWnd.h): C2's
// copy of NCShopItemWnd (NCShopWnd.h) under NCItemWnd, with its own Target / Mode / Side. What differs from the shop's
// grid: an entry that moves always goes to the Target and leaves this grid (the NPC's goods do not stay), whatever the
// mode and the side; the tool tips of the entries (the price line: C2 passes 5 where the shop passes 7 for the NPC's
// goods when buying); the amount typed is clamped to the entry's in both modes (and the stock code checks the
// selection).

#include "nwindow/NCItemWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCManorShopItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCManorShopItemWnd, NCItemWnd, 0, 0x1028C658)

    NCManorShopItemWnd* Target;      // +0x148 where its items go (the other grid)
    INT Mode;                        // +0x14C the window's (SHOPMODE_*, NCShopWnd.h)
    INT Side;                        // +0x150 SHOPSIDE_* (SetSide 0x100d8ab0 writes it, the tool tip ladder reads it)

    NCManorShopItemWnd();                                            // 0x100a4580
    NCManorShopItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x100a4620
    // ~NCManorShopItemWnd                                           // 0x100a45d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a2850 (shared with the other grids)
    virtual INT OnDestroy();                                         // 38 0x100d7090 (shared)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100a3f40
    // names ours (the same slots as NCShopItemWnd's)
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x100a4210 WM_LBUTTONDBLCLK
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 76 0x100a4470

    // names ours; the three setters are the bodies the shop's grids share (the stock code of the manor's window calls those)
    __declspec(noinline) void SetTarget(NCManorShopItemWnd* InTarget); // 0x10088870 (shared)
    __declspec(noinline) void SetMode(INT InMode);                   // 0x100de910 (shared)
    __declspec(noinline) void SetSide(INT InSide);                   // 0x100a28d0 (shared)
    UBOOL AddItem(NItemInfo* Info);                                  // 0x100a3560 0 when full
    UBOOL AddCopy(NItemEntry* Src, INT Count);                       // 0x100a36b0 0 when full
    void MoveItem(NItemEntry* Item, INT Count);                      // 0x100a37e0 to Target
};
NW_ASSERT_OFFSET(NCManorShopItemWnd, Target, 0x148);
NW_ASSERT_OFFSET(NCManorShopItemWnd, Mode, 0x14C);
NW_ASSERT_OFFSET(NCManorShopItemWnd, Side, 0x150);
NW_ASSERT_SIZE(NCManorShopItemWnd, 0x154);

#pragma pack(pop)
