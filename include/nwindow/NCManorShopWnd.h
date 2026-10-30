#pragma once
// NCManorShopWnd - the manor's seed and crop shop (UClass "CManorShopWnd", 0x110; desktop +0xE4, NCConsole::NewWndE4):
// C2's copy of NCShopWnd (NCShopWnd.h): the same two grids with the arrows between them, the total and the player's
// adena, OK and Cancel, the same texts and requests. The class declares NCShopWnd as its super and then repeats the
// shop's fields (+0xD4 .. +0x10F, the shop's +0x98 .. +0xD3 moved by 0x3C): its constructor runs the shop's (which
// sets the inherited copies) and sets none of its own, and every body below works on the copies. The console's
// manor virtuals (NConsoleWnd 65..68: StartManorBuyList / StartManorSellList, AddManorBuyList / AddManorSellList)
// open it in one mode or the other (Clear, SetMode, SetListID, SetAdena) and fill the grids (AddShopItem /
// AddPlayerItem).
//
// What differs from the shop's code: the grids (NCManorShopItemWnd.h) move whole entries (the NPC's goods do not
// stay), the window procedure repeats the OK button's test after the shop's procedure (which has answered it: the
// repeat is never reached), and there is no ReloadTexts.

#include "nwindow/NCShopWnd.h"
#include "nwindow/NCManorShopItemWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCManorShopWnd : public NCShopWnd
{
    NW_DECLARE_CLASS(NCManorShopWnd, NCShopWnd, 0, 0x1028CB78)

    // the fields repeat the shop's (and hide them: the inherited ones are the shop's, unused here)
    NCManorShopItemWnd* PlayerItems; // +0x104 SHOPSIDE_Player: below when buying, on top when selling (the shop's +0x98)
    NCManorShopItemWnd* ShopItems;   // +0x108 SHOPSIDE_Shop: on top when buying, below when selling (+0x9C)
    INT UnknownDC;                   // +0x10C (+0xA0)   // C2: offset derived, no evidence
    UTexture* BackTex;               // +0x110 "L2UI.StoreWnd.StoreBack" (+0xA4)
    INT UnknownE4;                   // +0x114 (+0xA8)   // C2: offset derived, no evidence
    NCButton* UpButton;              // +0x118 (+0xAC)
    NCButton* DownButton;            // +0x11C (+0xB0)
    NCButton* OKButton;              // +0x120 (+0xB4)
    NCButton* CancelButton;          // +0x124 (+0xB8)
    NCWnd* UnknownF8;                // +0x128 OnDestroy's, never created (+0xBC)
    INT Mode;                        // +0x12C SHOPMODE_* (+0xC0)
    INT ListID;                      // +0x130 (-1 after Clear) the NPC's list, sent back with the request (+0xC4)
    INT Adena;                       // +0x134 the player's (+0xC8)
    SQWORD TotalPrice;               // +0x138 UpdateTotalPrice (+0xCC)

    NCManorShopWnd();                                                // 0x100a4660
    // ~NCManorShopWnd                                               // 0x100a46b0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a28e0
    virtual INT OnCreate();                                          // 37 0x100a4700
    virtual INT OnDestroy();                                         // 38 0x100a3180
    virtual void HideWindow();                                       // 45 0x100a29a0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100a2a00
    // names ours (the same slots as NCShopWnd's)
    virtual INT OnOK();                                              // 68 0x100a2f00
    virtual INT OnCancel();                                          // 69 0x100a29b0
    // slots 70, 71 and 72 are the shop's placeholders (NewVirtual70..72); the bodies below are hooked at their addresses
    virtual void NewVirtual70();                                     // 70 0x100a39e0 (this shop's own OnUp, not NCShopWnd's)
    virtual void NewVirtual71();                                     // 71 0x100a3ca0
    virtual void NewVirtual72();                                     // 72 0x100a29d0
    INT OnUp();                                                      // 0x100a39e0: slot 70
    INT OnDown();                                                    // 0x100a3ca0: slot 71
    INT OnClose();                                                   // 0x100a29d0: slot 72 (OnCancel's work and a sound)

    // names ours (the console calls most of these)
    void SetListID(INT InListID);                                    // 0x100a2940
    void SetAdena(INT InAdena);                                      // 0x100a2950
    __declspec(noinline) void Clear();                               // 0x100a2960 grids emptied, list id -1, adena and total 0
    __declspec(noinline) void UpdateLayout();                        // 0x100a32d0 the grids placed by mode
    __declspec(noinline) void UpdateTotalPrice();                    // 0x100a3450
    void SetMode(INT InMode);                                        // 0x100a3980 the grids' too, then the layout
    UBOOL AddShopItem(NItemInfo* Info);                              // 0x100a39c0
    UBOOL AddPlayerItem(NItemInfo* Info);                            // 0x100a39d0
};
NW_ASSERT_OFFSET(NCManorShopWnd, PlayerItems, 0x104);
NW_ASSERT_OFFSET(NCManorShopWnd, ShopItems, 0x108);
NW_ASSERT_OFFSET(NCManorShopWnd, BackTex, 0x110);
NW_ASSERT_OFFSET(NCManorShopWnd, UpButton, 0x118);
NW_ASSERT_OFFSET(NCManorShopWnd, UnknownF8, 0x128);
NW_ASSERT_OFFSET(NCManorShopWnd, Mode, 0x12C);
NW_ASSERT_OFFSET(NCManorShopWnd, ListID, 0x130);
NW_ASSERT_OFFSET(NCManorShopWnd, Adena, 0x134);
NW_ASSERT_OFFSET(NCManorShopWnd, TotalPrice, 0x138);
NW_ASSERT_SIZE(NCManorShopWnd, 0x140);

#pragma pack(pop)
