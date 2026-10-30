#pragma once
// NCShopWnd - an NPC's shop (UClass "CShopWnd", 0xD0): two grids with the arrows between
// them, the total and the player's adena under them, OK and Cancel. Mode 0 buys: the NPC's
// goods on top (they stay there; a stack asks how many whatever its count), what is bought
// below; mode 1 sells: the inventory on top, what is sold below (at most what there is). The
// total is the price times the amount of each item below, in 64 bits (drawn with "%d": its
// low half). OK sends the lower grid's list to the server (RequestBuyItem /
// RequestSellItem). The console opens the window in one mode or the other (Clear, SetMode,
// SetListID, SetAdena) and fills the grids (AddShopItem / AddPlayerItem).
//
// NCShopItemWnd - its grids (UClass "CShopItemWnd", 0x124): the warehouse's (NCStoreWnd.h)
// with the prices (NItemEntry +0xC20) in the tool tips, and the rules of the mode and side
// when an item moves.
//
// Desktop (NCConsole) +0xD0 is this window.

#include "nwindow/NCItemWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

// NWM_COMMAND ids (names ours): NCStoreWnd's (NID_STORE_*), the window procedure is one body

// NCShopWnd::Mode, NCShopItemWnd::Mode and Side (names ours)
enum
{
    SHOPMODE_Buy = 0,
    SHOPMODE_Sell = 1,
    SHOPMODE_Preview = 2,            // C3 (name ours): the preview window's (NCShopPreviewWnd.h): the goods move between
                                     //   the lists - tried on, not bought - and leave the NPC's list while they are below
    SHOPSIDE_Shop = 0,               // the NPC's goods when buying, what is sold when selling
    SHOPSIDE_Player = 1,             // what is bought when buying, the inventory when selling
};

class NCShopItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCShopItemWnd, NCItemWnd, 0, 0x102AD3E8)

    NCShopItemWnd* Target;           // +0x148 where its items go (the other grid)   // C2: offset derived, no evidence
    INT Mode;                        // +0x14C the window's (SHOPMODE_*)   // C2: offset derived, no evidence
    INT Side;                        // +0x150 SHOPSIDE_*   // C2: offset derived, no evidence   // C2: offset derived, no evidence

    NCShopItemWnd();                                                 // 0x100f28f0
    NCShopItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x100f2990
    // ~NCShopItemWnd                                                // 0x100f2940

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a2850 (shared with the other grids)
    virtual INT OnDestroy();                                         // 38 0x100d7090 (shared with the other grids)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100f0200
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100f2370
    // slots 75 and 76 are placeholders (the stock WindowProc calls them); the bodies below are hooked at their addresses
    // names ours
    INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 0x100f2620: slot 75, WM_LBUTTONDBLCLK
    INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 0x100f2890: slot 76

    // names ours
    __declspec(noinline) void SetTarget(NCShopItemWnd* InTarget);    // 0x10088870 (shared)
    __declspec(noinline) void SetMode(INT InMode);                   // 0x100de910 (shared)
    __declspec(noinline) void SetSide(INT InSide);                   // 0x100a28d0 (shared)
    UBOOL AddItem(NItemInfo* Info);                                  // 0x100f1290 0 when full
    UBOOL AddCopy(NItemEntry* Src, INT Count);                       // 0x100f13f0 0 when full
    void MoveItem(NItemEntry* Item, INT Count);                      // 0x100f1520 to Target
    virtual void NewVirtual75();  // 75 0x100f2620 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual76();  // 76 0x100f2890 (only this client has it; defined in NNewVirtuals.gen.cpp)
};
NW_ASSERT_OFFSET(NCShopItemWnd, Target, 0x148);
NW_ASSERT_OFFSET(NCShopItemWnd, Side, 0x150);
NW_ASSERT_SIZE(NCShopItemWnd, 0x154);

class NCShopWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCShopWnd, NCWnd, 0, 0x102AD908)

    NCShopItemWnd* PlayerItems;      // +0xC8 SHOPSIDE_Player: below when buying, on top when
                                     //   selling
    NCShopItemWnd* ShopItems;        // +0xCC SHOPSIDE_Shop: on top when buying, below when
                                     //   selling
    INT Unknown9C;                   // +0xD0
    UTexture* BackTex;               // +0xD4 "L2UI.StoreWnd.StoreBack"
    INT UnknownA4;                   // +0xD8
    NCButton* UpButton;              // +0xDC
    NCButton* DownButton;            // +0xE0
    NCButton* OKButton;              // +0xE4
    NCButton* CancelButton;          // +0xE8
    NCWnd* UnknownB8;                // +0xEC OnDestroy's, never created
    INT Mode;                        // +0xF0 SHOPMODE_*   // C2: offset derived, no evidence
    INT ListID;                      // +0xF4 (-1) the NPC's list, sent back with the request
    INT Adena;                       // +0xF8 the player's
    SQWORD TotalPrice;               // +0xFC UpdateTotalPrice

    NCShopWnd();                                                     // 0x100f29d0
    // ~NCShopWnd                                                    // 0x100a4500

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100f0070 (shared)
    INT OnCreate();                                          // 37 0x100f2a40
    INT OnDestroy();                                         // 38 0x100f0920
    virtual void ShowWindow();                                       // 44 0x100412c0 (folded)
    virtual void HideWindow();                                       // 45 0x100f01a0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100f0420
    // names ours
    virtual INT OnOK();                                              // 68 0x100f0c00
    virtual INT OnCancel();                                          // 69 0x100f01b0
    virtual void NewVirtual70();  // 70 0x100f1e10 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual71();  // 71 0x100f20d0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual72();  // 72 0x100f01d0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    // slots 70, 71 and 72 are placeholders; the bodies below are hooked at their addresses (the stock WindowProc, the
    // one of NCStoreWnd, calls them through the slots)
    INT OnUp();                                              // 0x100f1e10: slot 70
    INT OnDown();                                            // 0x100f20d0: slot 71
    INT OnClose();                                           // 0x100f01d0: slot 72 (OnCancel's work and a sound)

    // names ours (the console calls most of these)
    void SetAdena(INT InAdena);                                      // 0x100f0150
    void SetListID(INT InListID);                                    // 0x10116b20 (shared)
    void Clear();                                                    // 0x100f0160 grids emptied,
                                                                     //   list id -1, adena and
                                                                     //   total 0
    __declspec(noinline) void UpdateLayout();                        // 0x100f0a70 the grids placed
                                                                     //   by mode
    __declspec(noinline) void UpdateTotalPrice();                                         // 0x100f0fa0 (the preview's too)
    void ReloadTexts();                                              // 0x100f10b0 item names and
                                                                     //   descriptions, tool tips
    INT GetMode();                                                   // 0x101189a0 (folded with NCStoreWnd's)
    void SetMode(INT InMode);                                        // 0x100f1db0 the grids' too,
                                                                     //   then the layout
    UBOOL AddShopItem(NItemInfo* Info);                              // 0x100f1df0
    UBOOL AddPlayerItem(NItemInfo* Info);                            // 0x100f1e00
};
NW_ASSERT_OFFSET(NCShopWnd, BackTex, 0xD4);
NW_ASSERT_OFFSET(NCShopWnd, Mode, 0xF0);
NW_ASSERT_OFFSET(NCShopWnd, TotalPrice, 0xFC);
NW_ASSERT_SIZE(NCShopWnd, 0x104);

#pragma pack(pop)