#pragma once
// NCPrivateBuyWnd - the private store (buy) window (UClass "CPrivateBuyWnd", 0xD4): the sell
// window's twin (NCPrivateShopWnd.h, the same fields at the same places; several bodies are
// shared) for a store that buys. Setting one's own up (Mode 1): the inventory on top as a
// catalog, the wish list below with the prices offered and the amounts wanted; at someone's
// (Mode 0): the items the store wants on top, the ones sold to it below. The desktop keeps it
// at +0x134.
//
// NCPrivateBuyItemWnd - its two grids (UClass "CPrivateBuyItemWnd", 0x12C), Side and Mode as in
// NCPrivateShopItemWnd; NItemEntry::UnknownC34 is the amount the store still wants. At
// someone's store the inventory's items the player lacks (count 0) are covered.

#include "nwindow/NCItemWnd.h"
#include "nwindow/NCFrameCtrl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

// what the dialog box sends the buy store windows (NCDialogBox::OnOK), names ours
enum
{
    NWM_BUYSTORE_PRICE = (INT)0xF000000B,      // dialog 0x26: the price typed
    NWM_BUYSTORE_AMOUNT = (INT)0xF000000C,     // dialog 0x27: the amount typed
    NWM_BUYSTORE_PRICE_OK = (INT)0xF000000D,   // dialog 0x29: the unusual price confirmed
    NWM_BUYSTORE_PRICE_CANCEL = (INT)0xF000000E, // never sent
    NWM_BUYSTORE_SELL_OK = (INT)0xF000000F,    // dialog 0x2A: selling at unusual prices confirmed
};

// NCPrivateBuyFrameCtrl - the buy store's title bar (UClass "CPrivateBuyFrameCtrl", 0x238; new in C3): the close button
// does the window's own close (the store is told to quit when it is one's own, the lists are emptied)
class NCPrivateBuyFrameCtrl : public NCFrameCtrl
{
    NW_DECLARE_CLASS(NCPrivateBuyFrameCtrl, NCFrameCtrl, 0, 0x1029FBB8)

    NCPrivateBuyFrameCtrl();                                         // 0x100d66d0 (NCFrameCtrl's and this vtable; the
                                                                     //   InternalConstructor calls it, as the stock one)
    virtual INT OnClose();                                           // 70 0x100d44e0
};
NW_ASSERT_SIZE(NCPrivateBuyFrameCtrl, 0x238);

class NCPrivateBuyItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCPrivateBuyItemWnd, NCItemWnd, 0, 0x1029F698)

    UTexture* CoverTex;              // +0x148 "NWindow.Chat", then "NWindow.ChatBack" (OnPaint)
                                     //   over the items the player lacks
    NCPrivateBuyItemWnd* Partner;    // +0x14C the other grid (SetPartner), or the one an item
                                     //   was carried onto
    INT Mode;                        // +0x150 the store window's (SetMode)
    INT Side;                        // +0x154 0: the store's list, 1: the player's side (SetSide)
    INT Unknown128;                  // +0x158 cleared, never read here

    __declspec(noinline) NCPrivateBuyItemWnd();                      // 0x100d6770 (the stock OnCreate calls it: not inlined there)
    __declspec(noinline) NCPrivateBuyItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x100d6860
    // ~NCPrivateBuyItemWnd                                          // 0x100d6810

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100d3c30
    virtual INT OnDestroy();                                         // 38 0x100d7090 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100d3ef0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100d5fb0
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x100d6330
    virtual INT OnPriceCancel(WPARAM wParam, LPARAM lParam) { return 1; } // 76 0x1000bd60 (folded)
    virtual INT OnPrice(WPARAM wParam, LPARAM lParam);               // 77 0x100d42f0
    virtual INT OnAmount(WPARAM wParam, LPARAM lParam);              // 78 0x100d6650
    virtual INT OnPriceOK(WPARAM wParam, LPARAM lParam);             // 79 0x100d41f0

    // names ours
    __declspec(noinline) void SetPartner(NCPrivateBuyItemWnd* InPartner);                 // 0x100de910 (shared with the
                                                                     //   NPC shops' grids)
    __declspec(noinline) void SetMode(INT InMode);                   // 0x100a28d0 (shared: the stock callers call it, [this+0x150] = InMode)
    __declspec(noinline) void SetSide(INT InSide);                   // 0x100d3d10 (called by the stock OnCreate)
    UBOOL AddItem(NItemInfo* Info);                                  // 0x100d5290 0 when full
    __declspec(noinline) UBOOL AddEntry(NItemEntry* Src, INT Count);                      // 0x100d53c0 a copy, 0 when full
    // to Partner; 1, or 0 when the wish list would pass the limit (OnAmount returns it)
    __declspec(noinline) INT MoveItem(NItemEntry* Item, INT Count);                       // 0x100d54c0
};
NW_ASSERT_OFFSET(NCPrivateBuyItemWnd, Partner, 0x14C);
NW_ASSERT_OFFSET(NCPrivateBuyItemWnd, Side, 0x154);
NW_ASSERT_SIZE(NCPrivateBuyItemWnd, 0x15C);

// 4-byte packing: TotalPrice sits at +0xCC
#pragma pack(push, 4)
class NCPrivateBuyWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPrivateBuyWnd, NCWnd, 0, 0x1029F170)

    NCPrivateBuyItemWnd* PlayerItems;    // +0xC8 Side 1: the inventory (Mode 1), the items sold
                                         //   (Mode 0)
    NCPrivateBuyItemWnd* StoreItems;     // +0xCC Side 0: the items wanted
    INT Unknown9C;                       // +0xD0   // C2: offset derived, no evidence
    UTexture* BackTex;                   // +0xD4 "L2UI.StoreWnd.StoreBack"
    INT UnknownA4;                       // +0xD8   // C2: offset derived, no evidence
    NCButton* UpButton;                  // +0xDC
    NCButton* DownButton;                // +0xE0
    NCButton* StartButton;               // +0xE4 "Start" (Mode 1) / "OK" (Mode 0)
    NCButton* MessageButton;             // +0xE8 Mode 1 only
    NCButton* AbortButton;               // +0xEC Mode 1 only
    NCWnd* UnknownBC;                    // +0xF0 a child the destructor would remove; not made   // C2: offset derived, no evidence
    INT Mode;                            // +0xF4 0: selling to someone's store, 1: one's own
    INT OwnerID;                         // +0xF8 (-1) object id of the store's owner (Mode 0)
    INT Adena;                           // +0xFC the player's (SetAdena)   // C2: offset derived, no evidence
    SQWORD TotalPrice;                   // +0x100 the list's (UpdateTotal)
    NCPrivateBuyFrameCtrl* BuyFrame;     // +0x108 the title bar (C3: made here, not the base's FrameCtrl)

    NCPrivateBuyWnd();                                               // 0x100d68f0
    // ~NCPrivateBuyWnd                                              // 0x100d6990

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100d3d20
    virtual INT OnCreate();                                          // 37 0x100d69e0
    virtual INT OnDestroy();                                         // 38 0x100d4a30
    virtual void ShowWindow();                                       // 44 0x100d71d0
    virtual void HideWindow();                                       // 45 0x100d7250
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100d4530
    // names ours
    virtual INT OnStart();                                           // 68 0x100d4c30
    virtual INT OnUp();                                              // 69 0x100d5ab0
    virtual INT OnDown();                                            // 70 0x100d5d80
    virtual INT OnClose();                                           // 71 0x100d3e80
    virtual INT OnMessage();                                         // 72 0x100d4ff0
    virtual INT OnAbort();                                           // 73 0x100d3ec0
    virtual INT OnSellConfirmed(WPARAM wParam, LPARAM lParam);       // 74 0x100d50d0

    // names ours
    void SetAdena(INT InAdena);                                      // 0x1009d910 (C1 only: no C2 match) (C1 only: no C2 match) (hooked as the
                                                                     //   sell window's)
    void SetOwnerID(INT InOwnerID);                                  // 0x100efa00 (NCShopWnd::
                                                                     //   SetAdena's body)
    __declspec(noinline) void Reset();                                                    // 0x1009d920 (C1 only: no C2 match) (C1 only: no C2 match) (the sell window's)
    __declspec(noinline) void UpdateLayout();                                             // 0x100d79c0 (the sell window's body: one function)
    void SetMode(INT InMode);                                        // 0x100d5a30
    void AddStoreItem(NItemInfo* Info);                              // 0x100d5a70
    UBOOL AddPlayerItem(NItemInfo* Info);                            // 0x100d5aa0
    __declspec(noinline) UBOOL CheckTotal(INT ItemID, INT Count, INT Price);              // 0x100d7ea0 (name ours; the grids call it on the desktop's buy window)
    __declspec(noinline) void UpdateTotal();                                              // 0x100d7f70
};
#pragma pack(pop)
NW_ASSERT_OFFSET(NCPrivateBuyWnd, BackTex, 0xD4);
NW_ASSERT_OFFSET(NCPrivateBuyWnd, Mode, 0xF4);
NW_ASSERT_OFFSET(NCPrivateBuyWnd, TotalPrice, 0x100);
NW_ASSERT_OFFSET(NCPrivateBuyWnd, BuyFrame, 0x108);
NW_ASSERT_SIZE(NCPrivateBuyWnd, 0x10C);

#pragma pack(pop)