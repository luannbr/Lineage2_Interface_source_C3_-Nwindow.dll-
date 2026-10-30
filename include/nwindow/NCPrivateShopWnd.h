#pragma once
// NCPrivateShopWnd - the private store (sell) window (UClass "CPrivateShopWnd", 0xD4), in two
// modes: setting one's own store up (Mode 1: the inventory on top, the items on sale with
// their prices below; Start, Message, Abort) or buying at someone's (Mode 0: the store's items
// on top, the ones chosen below; OK). The arrows and drags move items between the two grids
// (how many first, for a stack; the price first, going on sale), and the window adds up the
// price of the list that is sent. The console fills it (SetMode, the items, SetAdena); the
// desktop (NCConsole) keeps it at +0x130.
//
// NCPrivateShopItemWnd - its two grids (UClass "CPrivateShopItemWnd", 0x128): Side 0 is the
// store's list, Side 1 the player's side. An entry's count is the amount there (NItemEntry::
// TradeCount), its price NItemEntry::UnknownC20 (Unknown1CA8: the item's reference price, a
// price 5 times away from it asks first). The questions go through the desktop's dialog box,
// which answers with the NWM_STORE_* messages.

#include "nwindow/NCItemWnd.h"
#include "nwindow/NCFrameCtrl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

// what the dialog box sends the store windows (NCDialogBox::OnOK / OnCancel), names ours
enum
{
    NWM_STORE_PRICE = (INT)0xF0000006,         // dialog 0x21: the price typed
    NWM_STORE_AMOUNT = (INT)0xF0000007,        // dialog 0x22: the amount typed
    NWM_STORE_PRICE_OK = (INT)0xF0000008,      // dialog 0x24: the unusual price confirmed
    NWM_STORE_PRICE_CANCEL = (INT)0xF0000009,  // dialog 0x24 cancelled
    NWM_STORE_BUY_OK = (INT)0xF000000A,        // dialog 0x25: buying at unusual prices confirmed
};

// NCPrivateShopFrameCtrl - the sell store's title bar (UClass "CPrivateShopFrameCtrl", 0x238; new in C3): the close
// button does the window's own close
class NCPrivateShopFrameCtrl : public NCFrameCtrl
{
    NW_DECLARE_CLASS(NCPrivateShopFrameCtrl, NCFrameCtrl, 0, 0x102A0B28)

    NCPrivateShopFrameCtrl();                                        // 0x100d9930 (NCFrameCtrl's and this vtable)
    virtual INT OnClose();                                           // 70 0x100d72d0
};
NW_ASSERT_SIZE(NCPrivateShopFrameCtrl, 0x238);

class NCPrivateShopItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCPrivateShopItemWnd, NCItemWnd, 0, 0x102A0608)

    NCPrivateShopItemWnd* Partner;   // +0x148 the other grid (SetPartner), or the one an item   // C2: offset derived, no evidence
                                     //   was carried onto
    INT Mode;                        // +0x14C the store window's (SetMode)   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    INT Side;                        // +0x150 0: the store's list, 1: the player's side (SetSide)   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    INT Unknown124;                  // +0x154 cleared, never read here   // C2: offset derived, no evidence

    __declspec(noinline) NCPrivateShopItemWnd();                     // 0x100d99d0 (the stock OnCreate calls it: not inlined there)
    __declspec(noinline) NCPrivateShopItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x100d9a70
    // ~NCPrivateShopItemWnd                                         // 0x100d9a20

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100d6fb0
    virtual INT OnDestroy();                                         // 38 0x100d7090 (shared)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100d8ed0
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x100d9230
    virtual INT OnPriceCancel(WPARAM wParam, LPARAM lParam) { return 1; } // 76 0x1000bd60 (folded)
    virtual void NewVirtual77();  // 77 0x100d9660 = OnPrice below (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual78();  // 78 0x100d9880 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual79();  // 79 0x100d9520 (only this client has it; defined in NNewVirtuals.gen.cpp)
    INT OnPrice(WPARAM wParam, LPARAM lParam);               // 0x100d9660: the body of slot 77 (called through the slot)
    INT OnAmount(WPARAM wParam, LPARAM lParam);              // 0x100d9880: slot 78
    INT OnPriceOK(WPARAM wParam, LPARAM lParam);             // 0x100d9520: slot 79

    // names ours
    __declspec(noinline) void SetPartner(NCPrivateShopItemWnd* InPartner);                // 0x10088870 (hooked as
                                                                     //   NCTradeItemWnd::SetKind)
    __declspec(noinline) void SetMode(INT InMode);                                        // 0x100de910 (shared with the
                                                                     //   NPC shops' grids)
    __declspec(noinline) void SetSide(INT InSide);                                        // 0x100a28d0 (shared)
    UBOOL AddItem(NItemInfo* Info);                                  // 0x100d82e0 0 when full
    __declspec(noinline) UBOOL AddEntry(NItemEntry* Src, INT Count);                      // 0x100d8400 a copy, 0 when full
    // to Partner; 1, or 0 when the list would pass the limit (OnAmount, OnPrice return it)
    __declspec(noinline) INT MoveItem(NItemEntry* Item, INT Count);                       // 0x100d84f0
};
NW_ASSERT_OFFSET(NCPrivateShopItemWnd, Partner, 0x148);
NW_ASSERT_OFFSET(NCPrivateShopItemWnd, Side, 0x150);
NW_ASSERT_SIZE(NCPrivateShopItemWnd, 0x158);

// 4-byte packing: TotalPrice sits at +0xCC
#pragma pack(push, 4)
class NCPrivateShopWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPrivateShopWnd, NCWnd, 0, 0x102A00E0)

    NCPrivateShopItemWnd* PlayerItems;   // +0xC8 Side 1: the inventory (Mode 1), the items
                                         //   chosen (Mode 0)
    NCPrivateShopItemWnd* StoreItems;    // +0xCC Side 0: the items on sale
    INT Unknown9C;                       // +0xD0   // C2: offset derived, no evidence
    UTexture* BackTex;                   // +0xD4 "L2UI.StoreWnd.StoreBack"
    INT UnknownA4;                       // +0xD8   // C2: offset derived, no evidence
    NCButton* UpButton;                  // +0xDC
    NCButton* DownButton;                // +0xE0
    NCButton* StartButton;               // +0xE4 "Start" (Mode 1) / "OK" (Mode 0)
    NCButton* MessageButton;             // +0xE8 Mode 1 only
    NCButton* AbortButton;               // +0xEC Mode 1 only
    NCWnd* UnknownBC;                    // +0xF0 a child the destructor would remove; not made   // C2: offset derived, no evidence
    INT Mode;                            // +0xF4 0: buying at someone's store, 1: one's own (SetMode)
    INT OwnerID;                         // +0xF8 (-1) object id of the store's owner (Mode 0)
    INT Adena;                           // +0xFC the player's (SetAdena)   // C2: offset derived, no evidence
    SQWORD TotalPrice;                   // +0x100 the list's (UpdateTotal)   // C2: offset derived, no evidence
    NCPrivateShopFrameCtrl* ShopFrame;   // +0x108 the title bar (C3: made here, not the base's FrameCtrl)

    NCPrivateShopWnd();                                              // 0x100d9ac0
    // ~NCPrivateShopWnd                                             // 0x100d9b70

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100d70a0
    virtual INT OnCreate();                                          // 37 0x100d9bc0
    virtual INT OnDestroy();                                         // 38 0x100d7820 (shared)
    virtual void ShowWindow();                                       // 44 0x100d71d0 (shared)
    virtual void HideWindow();                                       // 45 0x100d7250 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100d7320
    // names ours
    virtual INT OnStart();                                           // 68 0x100d7b40
    virtual INT OnUp();                                              // 69 0x100d89f0
    virtual INT OnDown();                                            // 70 0x100d8cb0
    virtual INT OnClose();                                           // 71 0x100d7260
    virtual INT OnMessage();                                         // 72 0x100d8080
    virtual INT OnAbort();                                           // 73 0x100d72a0
    virtual void NewVirtual74();  // 74 0x100d8160 (only this client has it; defined in NNewVirtuals.gen.cpp)
    INT OnBuyConfirmed(WPARAM wParam, LPARAM lParam);        // 0x100d8160: the body of slot 74 (called through it)

    // names ours
    void SetAdena(INT InAdena);                                      // 0x100d71c0
    void SetOwnerID(INT InOwnerID);                                  // 0x100efa00 (NCShopWnd::
                                                                     //   SetAdena's body)
    __declspec(noinline) void Reset();                                                    // 0x1009d920 (C1 only: no C2 match) (C1 only: no C2 match) (also the buy
                                                                     //   window's)
    __declspec(noinline) void UpdateLayout();                                             // 0x1009df20 (C1 only: no C2 match) (C1 only: no C2 match)
    void SetMode(INT InMode);                                        // 0x100d8970
    void AddStoreItem(NItemInfo* Info);                              // 0x100d89b0
    UBOOL AddPlayerItem(NItemInfo* Info);                            // 0x100d89e0
    __declspec(noinline) UBOOL CheckTotal(INT ItemID, INT Count, INT Price);              // 0x100d7ea0 (one body with the buy window's, hooked there)
    __declspec(noinline) void UpdateTotal();                                              // 0x100d7f70 (shared)
};
#pragma pack(pop)
NW_ASSERT_OFFSET(NCPrivateShopWnd, BackTex, 0xD4);
NW_ASSERT_OFFSET(NCPrivateShopWnd, Mode, 0xF4);
NW_ASSERT_OFFSET(NCPrivateShopWnd, TotalPrice, 0x100);
NW_ASSERT_OFFSET(NCPrivateShopWnd, ShopFrame, 0x108);
NW_ASSERT_SIZE(NCPrivateShopWnd, 0x10C);

#pragma pack(pop)