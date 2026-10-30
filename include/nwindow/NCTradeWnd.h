#pragma once
// NCTradeWnd - the trade window (UClass "CTradeWnd", 0x100): the player's inventory on top,
// the player's offer and the partner's under it with their names, a down arrow that offers the
// selected inventory item, OK and Cancel (both answer the server: RequestTradeDone). The
// console fills it: Reset and the names when a trade starts, the three grids from the server's
// lists, UpdateInventoryItem for changes to the inventory; the texts again after a language
// change (ReloadTexts).
//
// NCTradeItemWnd - those grids (UClass "CTradeItemWnd", 0x11C). Its UClass names NCWnd as the
// super, but its constructors call NCItemWnd's and its vtable inherits NCItemWnd's bodies. In
// the inventory grid (Kind 0) an item is offered by carrying it onto the offer grid or by a
// double click (how many first, for a stack); the other two grids only show their items. An
// entry's count there is the amount offered (NItemEntry +0xC48); the tool tips are the
// inventory's (NItemToolTip.h).
//
// Desktop (NCConsole) +0xE0 is this window.

#include "nwindow/NCItemWnd.h"
#include "nwindow/NCFrameCtrl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

// NWM_COMMAND ids (names ours)
enum
{
    NID_TRADE_FRAME = 0xF3,
    NID_TRADE_OK = 0xF4,
    NID_TRADE_CANCEL = 0xF5,
    NID_TRADE_ADD = 0xF6,            // the down arrow
};

// NCTradeItemWnd::Kind (names ours)
enum
{
    TRADEGRID_Inventory = 0,
    TRADEGRID_MyOffer = 1,
    TRADEGRID_PartnerOffer = 2,
};

// NCTradeFrameCtrl - the trade window's title bar (UClass "CTradeFrameCtrl", 0x238; new in C3): the close button
// answers the trade with "no" before it closes the bar
class NCTradeFrameCtrl : public NCFrameCtrl
{
    NW_DECLARE_CLASS(NCTradeFrameCtrl, NCFrameCtrl, 0, 0x102B3FF8)

    NCTradeFrameCtrl();                                              // 0x10106b60 (NCFrameCtrl's and this vtable)
    virtual INT OnClose();                                           // 70 0x10105870
};
NW_ASSERT_SIZE(NCTradeFrameCtrl, 0x238);

class NCTradeItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCTradeItemWnd, NCItemWnd, 0, 0x102B35B0)

    INT Kind;                        // +0x148 TRADEGRID_* (SetKind; the constructors leave it)
    UTexture* IconBoxTex;            // +0x14C "L2ui_ch3.etc.iconbox" behind every icon (the 6-argument constructor loads it)

    NCTradeItemWnd();                                                // 0x10106c00
    NCTradeItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x10106ca0
    // ~NCTradeItemWnd                                               // 0x10106c50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100a2850 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10105a80
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x10105c80
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x10105d00
    virtual void Clear();                                            // 72 0x10106a80
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x10105f40 WM_LBUTTONDBLCLK
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 76 0x10106210

    // names ours
    __declspec(noinline) void SetKind(INT InKind);                   // 0x10088870 (one body with the shop windows' grids')
    UBOOL AddItem(NItemInfo* Info);                                  // 0x101068e0 0 when full
    UBOOL UpdateItem(NItemInfo* Info);                               // 0x10106100 0: not here
    __declspec(noinline) UBOOL RemoveItem(INT ObjectID);             // 0x100cf0e0 (the pet's grid's body, shared)
};
NW_ASSERT_OFFSET(NCTradeItemWnd, Kind, 0x148);
NW_ASSERT_SIZE(NCTradeItemWnd, 0x150);

class NCTradeWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTradeWnd, NCWnd, 0, 0x102B3AD0)

    NCTradeItemWnd* InventoryItems;  // +0xC8 6 x 4 icons, 80 at most
    NCTradeItemWnd* MyItems;         // +0xCC 6 icons
    NCTradeItemWnd* PartnerItems;    // +0xD0 6 icons
    UTexture* BackTex;               // +0xD4 "L2UI.storeWnd.TradeBack"
    NCButton* AddButton;             // +0xD8
    NCButton* OKButton;              // +0xDC
    NCButton* CancelButton;          // +0xE0
    FString MyName;                  // +0xE4 C2: an FString (C1: TCHAR[20])
    FString PartnerName;             // +0xF0 over the partner's (Reset clears it)
    NCTradeFrameCtrl* TradeFrame;    // +0xFC the title bar (C3: made here, the base's FrameCtrl stays empty)

    NCTradeWnd();                                                    // 0x10106d20
    // ~NCTradeWnd                                                   // 0x10106de0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10105890
    virtual INT OnCreate();                                          // 37 0x10106e60
    virtual INT OnDestroy();                                         // 38 0x10106470
    virtual void ShowWindow();                                       // 44 0x10105980
    virtual void HideWindow();                                       // 45 0x100412d0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10106300
    // names ours
    virtual INT OnOK();                                              // 68 0x10105a40
    virtual INT OnCancel();                                          // 69 0x10105a60
    virtual INT OnAdd();                                             // 70 0x101065c0 the down arrow

    // names ours (the console's trade handlers call these)
    void Reset();                                                    // 0x10105930
    void SetMyName(const TCHAR* Name);                               // 0x101059b0
    void SetPartnerName(const TCHAR* Name);                          // 0x10105a10
    UBOOL AddInventoryItem(NItemInfo* Info);                         // 0x10106a50
    UBOOL AddMyItem(NItemInfo* Info);                                // 0x10106a60
    UBOOL AddPartnerItem(NItemInfo* Info);                           // 0x10106a70
    UBOOL UpdateInventoryItem(INT Op, NItemInfo* Info);              // 0x10106af0 1 add, 3 change,
                                                                     //   2 remove
    void ReloadTexts();                                              // 0x10106750 item names and
                                                                     //   descriptions, tool tips
};
NW_ASSERT_OFFSET(NCTradeWnd, BackTex, 0xD4);
NW_ASSERT_OFFSET(NCTradeWnd, MyName, 0xE4);
NW_ASSERT_OFFSET(NCTradeWnd, PartnerName, 0xF0);
NW_ASSERT_SIZE(NCTradeWnd, 0x100);

#pragma pack(pop)