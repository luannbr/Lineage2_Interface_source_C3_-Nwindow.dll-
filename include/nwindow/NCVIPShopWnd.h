#pragma once
// NCVIPShopWnd - the VIP shop (UClass "CVIPShopWnd", 0xC4; desktop +0xD4, title "Store"): two
// pages. The list page (NCVIPShopItemListWnd) shows the products; a click on one opens the item
// page: the item's information (NCVIPShopItemInfoWnd) over the items it costs
// (NCVIPShopNeededItemWnd), with OK (buy) and Cancel (back to the list). The console fills it
// through its virtuals 158..161: Open (a list id, everything emptied), AddRecord for each
// product, AddNeededItem for each item a product costs, then the window shown on the list
// (ShowList). OK sends the list id and the product id (console virtual 162, the network
// handler's slot 186). Hiding the window empties it.
//
// In the game's own terms this is the multisell window (an exchange list: a product costs the
// "items required", chosen by list id + product id); the name VIP shop is the decompilation's.
//
// The three pages are NCWnd / NCScrollWnd children laid over one another; the item page is the
// window's own background with "Item information" and "Items required" (sysstring 564, 565).

#include "nwindow/NCItemWnd.h"
#include "nwindow/NCVIPShopItemListBox.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

// NWM_COMMAND ids (names ours)
enum
{
    NID_VIPSHOP_FRAME = 0xF0E400,
    NID_VIPSHOP_LIST = 0xF0E401,
    NID_VIPSHOP_OK = 0xF0E402,
    NID_VIPSHOP_CANCEL = 0xF0E403,
};

// An item a product costs (name ours; 0x30C bytes in C2, 0x20C in C1: Name is 256 characters now; built on the
// stack by the console's virtual 200 (AddVIPNeedItemList) and copied by NCVIPShopWnd::AddNeededItem, then by
// the needed items' page). Layout from AddNeededItem 0x100873f0 (the inlined constructor and the copy),
// NCVIPShopWnd::ShowItem 0x10087e40 and NCVIPShopNeededItemWnd::OnPaint 0x100867b0.
struct NVIPNeededItem
{
    INT ItemID;                // +0x000
    TCHAR Name[256];           // +0x004 the item's
    INT Count;                 // +0x204 how many
    UTexture* Icon;            // +0x208 (AddNeededItem finds it by IconName)
    TCHAR IconName[128];       // +0x20C

    NVIPNeededItem()
    {
        Name[0] = 0;
        IconName[0] = 0;
        Count = 0;
        Icon = NULL;
        ItemID = 0;
        Enchant = 0;
        Grade = 0;
    }
    INT Enchant;       // +0x30C new in C3 (name ours): the "+N" of the name (the paint's 8th argument)
    INT Grade;         // +0x310 new in C3 (name ours): the crystal grade, 0 .. 4 (the paint's 9th)
    INT Type;          // +0x314 new in C3 (name ours): NItemEntry::Type (0 .. 3: the enchant shows; the paint's 7th); the
                       //   constructor leaves it
};
NW_ASSERT_OFFSET(NVIPNeededItem, Name, 0x4);
NW_ASSERT_OFFSET(NVIPNeededItem, Count, 0x204);
NW_ASSERT_OFFSET(NVIPNeededItem, Icon, 0x208);
NW_ASSERT_OFFSET(NVIPNeededItem, IconName, 0x20C);
NW_ASSERT_SIZE(NVIPNeededItem, 0x318);

// A product of the shop (name ours; 0x524 bytes, NCVIPShopWnd::AddRecord): the list row and the
// items it costs (never freed: Reset only empties the window's array)
struct NVIPShopRecord : public NVIPShopEntry
{
    TArray<NVIPNeededItem*> NeededItems;   // +0x518

    NVIPShopRecord();                                                // 0x10096d80
};
NW_ASSERT_OFFSET(NVIPShopRecord, NeededItems, 0x520);
NW_ASSERT_SIZE(NVIPShopRecord, 0x52C);

// the list page (UClass "CVIPShopItemListWnd", 0x9C)
class NCVIPShopItemListWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCVIPShopItemListWnd, NCWnd, 0, 0x10287E48)

    NCVIPShopItemListBox* ListBox;   // +0xC8
    UTexture* BackTex;               // +0xCC "L2UI.VipWnd.VipWndBack1" (OnCreate 0x10087720, OnPaint 0x10086690)

    NCVIPShopItemListWnd();                                          // 0x10097350
    // ~NCVIPShopItemListWnd                                         // 0x10086590 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10093e90
    virtual INT OnCreate();                                          // 37 0x100973f0
    virtual INT OnDestroy();                                         // 38 0x10095a80
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10095b30
    virtual INT OnSelect(INT Index);                                 // 68 0x100974c0 (name ours; C1 slot 62)

    // names ours
    __declspec(noinline) void Clear();                               // 0x10093ef0 (ListBox->Clear(), shared; the stock
                                                                     //   callers call it, they do not inline it)
    void AddEntry(const NVIPShopEntry* Src);                         // 0x100965a0
};
NW_ASSERT_OFFSET(NCVIPShopItemListWnd, BackTex, 0xCC);
NW_ASSERT_SIZE(NCVIPShopItemListWnd, 0xD0);

// the item page's information (UClass "CVIPShopItemInfoWnd", 0x1D7C): the product's item as an
// item window entry, completed from the item table
class NCVIPShopItemInfoWnd : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCVIPShopItemInfoWnd, NCScrollWnd, 0, 0x10288898)

    NItemEntry Item;                 // +0x100 (0x1A34 bytes in C3; the constructor 0x100975c0 calls NItemEntry's, the destructor
                                     //   0x10097640 inlines ~NItemEntry: ToolTip at +0x1AB8)
    BYTE _Unknown1B34[0x1B38 - 0x1B34]; // +0x1B34 (the constructor does not touch it)
    UTexture* IconBoxTex;            // +0x1B38 C3: "L2ui_ch3.etc.iconbox", the frame under the icon (OnCreate 0x10093f50)

    NCVIPShopItemInfoWnd();                                          // 0x100975c0
    // ~NCVIPShopItemInfoWnd                                         // 0x10097640

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10093f30 (shared)
    virtual INT OnCreate();                                          // 37 0x10093f50
    virtual INT OnDestroy();                                         // 38 0x10093f80
    virtual void ShowWindow();                                       // 44 0x10094000 (the scroll back at the top)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10095e10

    // names ours
    __declspec(noinline) void Clear();                               // 0x10095720 (called, not inlined, by the stock code)
    void SetItem(NItemEntry* Src);                                   // 0x100967a0
    INT DrawItemInfo(NCGDevice* Device, NItemEntry* Entry, INT X, INT Y); // 0x10094030 the Y under the texts
};
NW_ASSERT_OFFSET(NCVIPShopItemInfoWnd, Item, 0x100);
NW_ASSERT_OFFSET(NCVIPShopItemInfoWnd, IconBoxTex, 0x1B38);
NW_ASSERT_SIZE(NCVIPShopItemInfoWnd, 0x1B3C);

// the item page's needed items (UClass "CVIPShopNeededItemWnd", 0xE4): icon, name and "X count",
// 34 pixels a row
class NCVIPShopNeededItemWnd : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCVIPShopNeededItemWnd, NCScrollWnd, 0, 0x10288370)

    TArray<NVIPNeededItem*> Items;   // +0x100
    INT UnknownD8;                   // +0x10C (the constructor 0x10097500 leaves it)   // C2: offset derived, no evidence
    UTexture* IconBoxTex;            // +0x110 C3: "L2ui_ch3.etc.iconbox", the frame under each icon (OnCreate 0x10093f00)
    INT UnknownDC;                   // +0x114 (Clear: 0)
    INT UnknownE0;                   // +0x118 (Clear: 0)

    NCVIPShopNeededItemWnd();                                        // 0x10097500
    // ~NCVIPShopNeededItemWnd                                       // 0x10097560

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10093f30 (shared)
    virtual INT OnCreate();                                          // 37 0x10093f00
    virtual INT OnDestroy();                                         // 38 0x100965b0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10095bf0

    // names ours
    void AddItem(const NVIPNeededItem* Src);                         // 0x10096680
    __declspec(noinline) void Clear();                               // 0x10096740 (called by the stock code)
};
NW_ASSERT_OFFSET(NCVIPShopNeededItemWnd, Items, 0x100);
NW_ASSERT_SIZE(NCVIPShopNeededItemWnd, 0x11C);

// NCVIPShopWnd::Page (names ours)
enum
{
    VIPSHOPPAGE_List = 0,
    VIPSHOPPAGE_Item = 1,
};

class NCVIPShopWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCVIPShopWnd, NCWnd, 0, 0x10287400)

    NCVIPShopItemListWnd* ListWnd;   // +0xC8
    NCVIPShopItemInfoWnd* InfoWnd;   // +0xCC
    NCVIPShopNeededItemWnd* NeededWnd; // +0xD0
    NCButton* OKButton;              // +0xD4 the item page only
    NCButton* CancelButton;          // +0xD8 the item page only
    UTexture* ListBackTex;           // +0xDC C3: "L2UI_CH3.StoreWnd.store2_back" the list page's
    UTexture* BackTex;               // +0xE0 C3: "L2UI_CH3.StoreWnd.store3_back" the item page
    INT Selected;                    // +0xE4 the product shown (index in Records), -1 none
    INT ListID;                      // +0xE8 (Open) sent back with the purchase
    INT Page;                        // +0xEC VIPSHOPPAGE_*
    TArray<NVIPShopRecord*> Records; // +0xF0

    NCVIPShopWnd();                                                  // 0x100976a0
    // ~NCVIPShopWnd                                                 // 0x10097700

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10095730
    virtual INT OnCreate();                                          // 37 0x10097760
    virtual INT OnDestroy();                                         // 38 0x10095f10
    virtual void HideWindow();                                       // 45 0x100970d0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10096060
    // names ours
    virtual INT OnOK();                                              // 68 0x10096240 (C3: asks first, dialog 0x51)
    virtual INT OnCancel();                                          // 69 0x10096cb0

    // names ours (the console calls the last four)
    __declspec(noinline) void ShowList();                            // 0x10096a20 (the stock OnCancel and the console call it)
    __declspec(noinline) void ShowItem();                            // 0x10096e10 the item page of Records(Selected)
    __declspec(noinline) void Reset();                               // 0x10096af0 (Open and HideWindow call it)
    void AddNeededItem(INT ProductID, const NVIPNeededItem* Src);    // 0x10096b40
    void Open(INT InListID);                                         // 0x10096cd0
    void AddRecord(const NVIPShopEntry* Src);                        // 0x100970e0 (the console lane's)
};
NW_ASSERT_OFFSET(NCVIPShopWnd, BackTex, 0xE0);
NW_ASSERT_OFFSET(NCVIPShopWnd, Records, 0xF0);
NW_ASSERT_SIZE(NCVIPShopWnd, 0xFC);

#pragma pack(pop)