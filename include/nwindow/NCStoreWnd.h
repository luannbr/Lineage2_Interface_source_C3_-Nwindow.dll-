#pragma once
// NCStoreWnd - the warehouse window (UClass "CStoreWnd", 0xC8): two grids with the arrows
// between them, the fee and the player's adena under them, OK and Cancel. Items go from one
// grid to the other with the arrows, a drag or a double click (how many first, for a stack).
// Mode 0 deposits: the inventory on top, what goes into the warehouse below (30 adena a slot);
// mode 1 withdraws: the warehouse's items on top, what comes back below (no fee). OK sends the
// lower grid's list to the server. The console opens the window in one mode or the other
// (Clear, SetMode, SetAdena) and fills the grids (AddInventoryItem / AddStoredItem).
//
// NCStoreItemWnd - its grids (UClass "CStoreItemWnd", 0x124): an item goes to Target, the
// other grid (a drop onto another warehouse grid makes that one the target); an entry's count
// is the amount it holds (NItemEntry::TradeCount). The tool tips are the inventory's
// (NItemToolTip.h).
//
// Desktop (NCConsole) +0xCC is this window.
//
// NCGMStoreWnd - the same window for a GM looking at a character's warehouse (UClass
// "CGMStoreWnd", 0xC8): the arrows, OK and Cancel are disabled.

#include "nwindow/NCItemWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

// NWM_COMMAND ids (names ours; NCShopWnd shares this window procedure)
enum
{
    NID_STORE_FRAME = 0xF3,
    NID_STORE_OK = 0xF4,
    NID_STORE_CANCEL = 0xF5,
    NID_STORE_UP = 0xF6,
    NID_STORE_DOWN = 0xF7,
    NID_STORE_CLOSE = 0xF8,          // handled, but no control of the window sends it
};

// NCStoreWnd::Mode (names ours)
enum
{
    STOREMODE_Deposit = 0,
    STOREMODE_Withdraw = 1,
};

class NCStoreItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCStoreItemWnd, NCItemWnd, 0, 0x102B2B68)

    NCStoreItemWnd* Target;          // +0x148 where its items go (the other grid)
    INT Mode;                        // +0x14C the window's (STOREMODE_*); the shop's grid reads   // C2: offset derived, no evidence   // C2: offset derived, no evidence
                                     //   its copy, this one does not
    INT Side;                        // +0x150 0 the warehouse's grid, 1 the inventory's (not   // C2: offset derived, no evidence
                                     //   read here either)

    NCStoreItemWnd();                                                // 0x10105220
    NCStoreItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x101052c0
    // ~NCStoreItemWnd                                               // 0x10105270

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a2850 (shared)
    virtual INT OnDestroy();                                         // 38 0x100d7090 (shared)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x10104e80
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x10105050 WM_LBUTTONDBLCLK
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 76 0x101051c0

    // names ours; the three setters are one body each with other grids' (shops, private
    // stores), the last two hooked here
    __declspec(noinline) void SetTarget(NCStoreItemWnd* InTarget);   // 0x10088870 (shared)
    __declspec(noinline) void SetMode(INT InMode);                                        // 0x100de910 (shared with the shops' and the private stores' grids)
    __declspec(noinline) void SetSide(INT InSide);                   // 0x100a28d0 (shared)
    UBOOL AddItem(NItemInfo* Info);                                  // 0x101045c0 0 when full
    UBOOL AddCopy(NItemEntry* Src, INT Count);                       // 0x101046d0 0 when full
    void MoveItem(NItemEntry* Item, INT Count);                      // 0x101047b0 to Target
};
NW_ASSERT_OFFSET(NCStoreItemWnd, Target, 0x148);
NW_ASSERT_OFFSET(NCStoreItemWnd, Side, 0x150);
NW_ASSERT_SIZE(NCStoreItemWnd, 0x154);

class NCStoreWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCStoreWnd, NCWnd, 0, 0x102B3088)

    NCStoreItemWnd* InventoryItems;  // +0xC8 the inventory's side: on top when depositing,
                                     //   below (what comes back) when withdrawing
    NCStoreItemWnd* StoredItems;     // +0xCC the warehouse's side: below (what goes in) when
                                     //   depositing, on top when withdrawing
    INT Unknown9C;                   // +0xD0   // C2: offset derived, no evidence
    UTexture* BackTex;               // +0xD4 "L2UI.StoreWnd.StoreBack"
    INT UnknownA4;                   // +0xD8   // C2: offset derived, no evidence
    NCButton* UpButton;              // +0xDC
    NCButton* DownButton;            // +0xE0
    NCButton* OKButton;              // +0xE4
    NCButton* CancelButton;          // +0xE8
    NCWnd* UnknownB8;                // +0xEC OnDestroy's, never created   // C2: offset derived, no evidence
    INT Mode;                        // +0xF0 STOREMODE_*
    INT Adena;                       // +0xF4 the player's
    INT Fee;                         // +0xF8 UpdateFee

    NCStoreWnd();                                                    // 0x10105300
    // ~NCStoreWnd                                                   // 0x10073bd0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100f0070 (shared)
    virtual INT OnCreate();                                          // 37 0x10105360
    virtual INT OnDestroy();                                         // 38 0x100f0920 (shared)
    virtual void ShowWindow();                                       // 44 0x10104250
    virtual void HideWindow();                                       // 45 0x10103c50
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10103ce0
    // names ours
    virtual INT OnOK();                                              // 68 0x10104260
    virtual INT OnCancel();                                          // 69 0x10103c60
    virtual INT OnUp();                                              // 70 0x10104920
    virtual INT OnDown();                                            // 71 0x10104bd0
    virtual INT OnClose();                                           // 72 0x10103cb0 (OnCancel's body)

    // names ours (the console calls most of these)
    void SetMode(INT InMode);                                        // 0x10103bf0 the grids' too
    void SetAdena(INT InAdena);                                      // 0x10116b20 (shared)
    void Clear();                                                    // 0x10103c20 grids emptied,
                                                                     //   adena and fee 0
    __declspec(noinline) void UpdateFee();                                                // 0x10103c80
    INT GetMode();                                                   // 0x101189a0 (shared)
    __declspec(noinline) void UpdateLayout();                        // 0x101040c0 the grids placed
                                                                     //   by mode
    void ReloadTexts();                                              // 0x101044b0 item names and
                                                                     //   descriptions, tool tips
    UBOOL AddInventoryItem(NItemInfo* Info);                         // 0x10104900
    UBOOL AddStoredItem(NItemInfo* Info);                            // 0x10104910
};
NW_ASSERT_OFFSET(NCStoreWnd, BackTex, 0xD4);
NW_ASSERT_OFFSET(NCStoreWnd, Mode, 0xF0);
NW_ASSERT_SIZE(NCStoreWnd, 0xFC);

class NCGMStoreWnd : public NCStoreWnd
{
    NW_DECLARE_CLASS(NCGMStoreWnd, NCStoreWnd, 0, 0x1027E8E0)

    NCGMStoreWnd();                                                  // 0x10085060
    // ~NCGMStoreWnd                                                 // 0x100766a0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1007e330
    virtual INT OnCreate();                                          // 37 0x1007e350
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10081970
};
NW_ASSERT_SIZE(NCGMStoreWnd, 0xFC);

#pragma pack(pop)