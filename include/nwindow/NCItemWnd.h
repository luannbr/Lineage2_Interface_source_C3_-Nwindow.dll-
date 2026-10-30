#pragma once
// NCItemWnd - grid of item icons (UClass "CItemWnd", 0x118), base of the item windows:
// inventory, pet inventory, shops, warehouse, private stores, skills, quest items, trade
// (NCTradeItemWnd's UClass names NCWnd as its super, but its constructor calls NCItemWnd's
// and its vtable inherits these bodies). The items are NItemEntry records the window owns
// (Clear frees them), shown as Columns x VisibleRows icons of CellSize pixels, 3 apart,
// scrolled by whole rows. A click selects an item and picks it up (bDragging: the cursor
// becomes its icon); the tool tip over an item is its name, or for the selected item a box
// whose text the subclass draws (DrawItemToolTip).

#include "nwindow/NCScrollWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NItemInfo.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

// An item of an item window (name ours; 0x1A28 bytes in C2, 0x1CAC in C1; created with new from an
// NItemInfo by the subclasses' AddItem and deleted by NCItemWnd::Clear): the NItemInfo fields in
// another order, a display name (sub_10019590, name ours) and the selection.
//
// C2 layout: from the copy of an NItemInfo into an entry (0x1001A9A0, field by field), Reset
// (0x100196F0), the getters (0x10019580..0x100196E0) and the allocation (`new` of 0x1A28 bytes in
// NCInvenItemWnd::AddItem). The strings are the NItemInfo's, shifted by 0xC; the two tool tip strings
// of C1 are gone (C2 keeps an object at +0x19B0 that the tool tip code, 0x10028B20, fills).
struct NItemEntry
{
    UTexture* Icon;            // +0x000
    INT Unknown004;            // +0x004 second icon (GetIcon2: the paperdoll's two-slot items)
    INT Unknown008;            // +0x008 third icon (GetIcon3)
    TCHAR Name[256];           // +0x00C (C1: 128 characters)
    TCHAR AdditionalName[256]; // +0x20C new in C2 (NItemInfo +0x200): shown after the name
    TCHAR IconName[128];       // +0x40C (C1 +0x10C)
    TCHAR Unknown20C[128];     // +0x50C texture of the second icon (C1 +0x20C)
    TCHAR Unknown30C[128];     // +0x60C texture of the third icon (C1 +0x30C)
    TCHAR Description[2048];   // +0x70C (C1 +0x40C, 1024 characters)
    TCHAR DisplayName[256];    // +0x170C new in C2 (name ours): "+N Name AdditionalName", built by sub_10019590
    INT Type;                  // +0x190C (-1) NItemInfo::Param; 0..3: the enchant shows (C1 +0xC0C)
    INT Unknown1910;           // +0x1910 (-1) new in C2 (NItemInfo +0x1710): the kind of an etc item (Type 5): 7 pet collar (its level is Enchant), 0xC / 0xD henna-like (the tool tip lists classes); the display name's "+N" is left out for 7, 0xC, 0xD
    INT ObjectID;              // +0x1914 the server's id of the item (C1 +0xC10)
    INT ID;                    // +0x1918 (C1 +0xC14)
    INT UnknownC18;            // +0x191C (-1) NItemInfo +0x1724 (C1 +0xC18): a skill's level (the tool tip: NToolTipInfo::Level, NConsoleWnd::GetSkillTypeText(ID, this))
    INT Count;                 // +0x1920 (C1 +0xC1C)
    INT UnknownC20;            // +0x1924 NItemInfo +0x1720 (C1 +0xC20): the price (NToolTipInfo::Fill's NTTO_Price line, formatted with commas)
    INT BodyPart;              // +0x1928 (NConsoleWnd::GetItemSlotText) (C1 +0xC24)
    INT Weight;                // +0x192C (C1 +0xC28)
    INT UnknownC2C;            // +0x1930 (-1) (C1 +0xC2C)
    INT UnknownC30;            // +0x1934 (-1) weapon type (NConsoleWnd::GetWeaponTypeName; 6: bow) (C1 +0xC30)
    INT UnknownC34;            // +0x1938 (-1) NItemInfo +0x1798 (C1 +0xC34): the amount wanted (the private store's buy list, NTT_Wanted); a recipe's first number
    INT Enchant;               // +0x193C (C1 +0xC38)
    SWORD UnknownC3C;          // +0x1940 NItemInfo +0x178C: a word in C2 (C1: an INT at +0xC3C)
    SWORD _Unknown1942;        // +0x1942 padding (Reset leaves it)
    UBOOL bStackable;          // +0x1944 the count shows: "Name(Count)" (C1 +0xC40)
    UBOOL bEquipped;           // +0x1948 (NItemInfo +0x1794) (C1 +0xC44)
    SQWORD TradeCount;         // +0x194C C3: 64 bits (the stock compares it as a signed 64-bit and multiplies it by the price with
                               //   __allmul); only the constructors clear it; the amount offered in the trade / shop grids (C1 +0xC48)
    UBOOL bSelected;           // +0x1954 (C1 +0x1C54) (C2: +0x1950)
    INT Unknown1C58;           // +0x1958 NItemInfo +0x1750 (C1 +0x1C58)
    INT CrystalGrade;          // +0x195C (NItemInfo +0x1754) (C1 +0x1C5C)
    INT Unknown1C60;           // +0x1960 (C1 +0x1C60)
    INT Unknown1C64;           // +0x1964 (C1 +0x1C64)
    INT Unknown1C68;           // +0x1968 NItemInfo +0x1760 (C1 +0x1C68)
    INT Unknown1C6C;           // +0x196C attack speed (NConsoleWnd::GetAttackSpeedText) (C1 +0x1C6C)
    INT Unknown1C70;           // +0x1970 (C1 +0x1C70)
    INT SubType;               // +0x1974 (-1) (NConsoleWnd::GetItemSlotText) (C1 +0x1C74)
    INT Unknown1C78;           // +0x1978 a shield's evasion (C1 +0x1C78)
    INT Unknown1C7C;           // +0x197C Copy leaves it (C1 +0x1C7C)
    INT Unknown1C80;           // +0x1980 NItemInfo +0x1738 P.Atk (C1 +0x1C80)
    INT Unknown1C84;           // +0x1984 M.Atk (C1 +0x1C84)
    INT Unknown1C88;           // +0x1988 P.Def (C1 +0x1C88)
    INT Unknown1C8C;           // +0x198C M.Def (C1 +0x1C8C)
    INT Unknown1C90;           // +0x1990 a shield's P.Def (C1 +0x1C90)
    INT Unknown1C94;           // +0x1994 a shield's rate (C1 +0x1C94)
    INT Unknown1C98;           // +0x1998 NItemInfo +0x17A0 (the paperdoll: the item is not taken
                               //   off by a click or a drag; a bow is drawn in one slot) (C1 +0x1C98)
    INT Unknown1998;           // +0x199C NItemInfo +0x17A4 (C2: +0x1998)
    INT Unknown1C9C;           // +0x19A0 NItemInfo +0x177C MP bonus (robes) (C1 +0x1C9C)
    INT Unknown1CA0;           // +0x19A4 soulshots used (C1 +0x1CA0)
    INT Unknown1CA4;           // +0x19A8 spiritshots used (C1 +0x1CA4)
    INT Unknown1CA8;           // +0x19AC NItemInfo +0x17A8; Reset leaves it (C1 +0x1CA8)
    INT Unknown19AC;           // +0x19B0 NItemInfo +0x1788 (GetUnknown19AC: the item's own message)
    INT Unknown19B4;           // +0x19B4 (-1) new in C3: NItemInfo +0x178C (the dword C3 inserted there)
    // +0x19B8 the tool tip's data (0x7C bytes, constructed by 0x100024F0, destroyed by 0x100025A0 through ~NItemEntry
    // 0x10101D80, filled by NToolTipInfo::Fill 0x1002C650(Item, NTT_Item, 0)); the two 1024-character strings of C1
    // (ToolTipName, ToolTipText) are gone. In an anonymous union so that the compiler neither builds nor destroys it:
    // the constructors build it by hand and ~NItemEntry calls the stock destructor (NToolTipInfo's own is not ported
    // yet, and a `delete` of an entry must not reach its NW_UNPORTED() int3)
    union { NToolTipInfo ToolTip; };

    NItemEntry();                                                    // 0x1001D8B0
    NItemEntry(NItemInfo* Src);                                      // 0x1001DA00
    __declspec(noinline) ~NItemEntry();                                                   // 0x10101D80 (destroys ToolTip)

    // The stock code calls these through getters folded with other classes' (0x10033ac0,
    // 0x10033ad0, 0x10033a40, 0x10033a50).
    UTexture* GetIcon() { return Icon; }
    INT GetUnknown004() { return Unknown004; }
    INT GetUnknown008() { return Unknown008; }
    // Unknown004 / Unknown008 are the second and third icons of an item drawn over two slots
    // (from Unknown20C / Unknown30C: NCEquipItemWnd)
    UTexture* GetIcon2() { return (UTexture*)(INT_PTR)Unknown004; }
    UTexture* GetIcon3() { return (UTexture*)(INT_PTR)Unknown008; }

    // C1's tool tip strings: C2 has none (see ToolTip above). The members stay, never defined, so that
    // NItemToolTip.h (C1 code, no C2 counterpart yet) still parses: a function that uses them does not link
    static TCHAR ToolTipName[1024];
    static TCHAR ToolTipText[1024];

    // C2: the stock functions at 0x10019580 / 0x10019630 / 0x10019650 are three getters of one shape (NULL
    // when the string is empty): the name (+0xC), the additional name (+0x20C) and the description (+0x70C)
    // (C1's GetToolTipName / GetToolTipText sat at these addresses: the fmap paired them)
    __declspec(noinline) const TCHAR* GetName();                                          // 0x10019580 NULL when empty
    __declspec(noinline) const TCHAR* GetAdditionalName();                                // 0x10019630 NULL when empty (name ours)
    __declspec(noinline) const TCHAR* GetDescription();                                   // 0x10019650 NULL when empty
    // 0x10019590 (name ours): "+NName-AdditionalName" into DisplayName and returns it (the "+N" only when
    // Enchant > 0 and Unknown1910 is not 7, 0xC, 0xD)
    __declspec(noinline) const TCHAR* MakeDisplayName();
    // names ours
    __declspec(noinline) INT GetUnknownC20();                                             // 0x10019670
    __declspec(noinline) INT GetUnknownC34();                                             // 0x10019680
    __declspec(noinline) void SetUnknownC34(INT Value);                                   // 0x10019690
    __declspec(noinline) void Copy(NItemEntry* Src);                                      // 0x1001AF90
    __declspec(noinline) void Select();                                                   // 0x100196A0
    __declspec(noinline) UBOOL IsSelected();                                              // 0x100196B0
    // declared INT: the stock loops count in eax across the call, which must come back unchanged (check_retval)
    __declspec(noinline) INT Unselect();                                                  // 0x1001C700
    // C3: true for a bStackable of 1, 2 or 3 (C2 returned the field)
    __declspec(noinline) UBOOL IsStackable();                                             // 0x1001C710
    __declspec(noinline) void Reset();                                                    // 0x100196F0
    // 0x10028B20 is NToolTipInfo::Fill (NToolTip.cpp): ToolTip.Fill(this, Flags, Extra). Flags is NTT_* (2, NTT_Item, is the item
    // windows' usual call; the callers also pass 1, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x802), Extra the NTTO_* options (0, or
    // 4 / 8 / 7 for the shops' price lines). Name ours.
    void FillToolTip(INT Flags, INT Extra) { ToolTip.Fill(this, Flags, Extra); }   // inline in the stock code too: `lea ecx, [entry + 0x19B8]; call 0x1002C650`
    __declspec(noinline) INT GetUnknown19AC();                                            // 0x1001C730
};
NW_ASSERT_OFFSET(NItemEntry, Name, 0xC);
NW_ASSERT_OFFSET(NItemEntry, AdditionalName, 0x20C);
NW_ASSERT_OFFSET(NItemEntry, IconName, 0x40C);
NW_ASSERT_OFFSET(NItemEntry, Unknown20C, 0x50C);
NW_ASSERT_OFFSET(NItemEntry, Unknown30C, 0x60C);
NW_ASSERT_OFFSET(NItemEntry, Description, 0x70C);
NW_ASSERT_OFFSET(NItemEntry, DisplayName, 0x170C);
NW_ASSERT_OFFSET(NItemEntry, Type, 0x190C);
NW_ASSERT_OFFSET(NItemEntry, ObjectID, 0x1914);
NW_ASSERT_OFFSET(NItemEntry, Count, 0x1920);
NW_ASSERT_OFFSET(NItemEntry, Enchant, 0x193C);
NW_ASSERT_OFFSET(NItemEntry, bStackable, 0x1944);
NW_ASSERT_OFFSET(NItemEntry, TradeCount, 0x194C);
NW_ASSERT_OFFSET(NItemEntry, bSelected, 0x1954);
NW_ASSERT_OFFSET(NItemEntry, SubType, 0x1974);
NW_ASSERT_OFFSET(NItemEntry, Unknown1C98, 0x1998);
NW_ASSERT_OFFSET(NItemEntry, Unknown19AC, 0x19B0);
NW_ASSERT_OFFSET(NItemEntry, ToolTip, 0x19B8);
NW_ASSERT_SIZE(NItemEntry, 0x1A34);

// C3's NItemInfo, from +0x178C on: a dword was inserted there (it holds -1; NItemEntry::Unknown19B4), so the fields of
// C2's layout that NItemInfo.h names at +0x178C .. +0x17A4 are 4 higher in this client (the constructor 0x10047400 and
// NItemEntry's 0x1001DA00 show it). Read them by offset: +0x1790 word (UnknownC84), +0x1794 bStackable (the item's kind),
// +0x1798 UnknownC8C (worn), +0x179C UnknownC90 (-1), +0x17A0 UnknownC94, +0x17A4 Unknown17A0, +0x17A8 UnknownC98
inline INT& NItemInfoInt(NItemInfo* Info, INT Offset)
{
    return *(INT*)((BYTE*)Info + Offset);
}

// A new entry as the stock code makes one inline: GMalloc->Malloc(sizeof, "new"), then the constructor (NULL stays NULL).
// (`new NItemEntry(Info)` goes through the global operator new, which keeps the "new" tag out of the function.)
inline NItemEntry* NewItemEntry(NItemInfo* Info)
{
    NItemEntry* Entry = (NItemEntry*)appMalloc(sizeof(NItemEntry), TEXT("new"));
    if (Entry)
        Entry = new ((void*)Entry) NItemEntry(Info);
    return Entry;
}

// An item as the item requests to the server take it, by value (name ours): the engine's
// ItemInfo (UE2Engine.h) with NWindow's constructor (Type and Unknown2C -1, Unknown02 not
// set, the rest 0); the destroy and drop requests fill ObjectID and Count.
struct NItemRequest : public ItemInfo
{
    NItemRequest();                                                  // 0x10064290
};
NW_ASSERT_OFFSET(NItemRequest, ObjectID, 0x4);
NW_ASSERT_OFFSET(NItemRequest, Count, 0xC);
NW_ASSERT_SIZE(NItemRequest, 0x30);

// The console's requests that take an NItemRequest by value
inline void NRequestDestroyItem(NItemRequest Request)
{
    GConsole->RequestDestroyItem(Request);
}
inline void NRequestDropItem(NItemRequest Request, FVector Location)
{
    GConsole->RequestDropItem(Request, Location);
}

class NCItemWnd : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCItemWnd, NCScrollWnd, 0, 0x1026CA70)

    // C2 keeps three textures (OnCreate 0x10019E90); C1's TipTopTex / TipMidTex / TipBottomTex went with the
    // tool tip boxes, which the window's skin (NCWnd::ToolTipSkin) draws now
    UTexture* BackTex;               // +0x100 "NWindow.BlackTexture" (the subclasses draw it)
    UTexture* SelectTex;             // +0x104 "l2ui.nwindow.item_click" around the selected item
    UTexture* ToolTipTex;            // +0x108 "L2UI.tooltip.mini-tool01" (not used by this class any more)
    TArray<NItemEntry*> Items;       // +0x10C
    UBOOL bDragging;                 // +0x118 the selected item is being carried
    UBOOL bNoDrag;                   // +0x11C the next click selects without picking up
    INT Columns;                     // +0x120 (5)
    INT VisibleRows;                 // +0x124 (5)
    INT TopRow;                      // +0x128 first row shown
    INT NumRows;                     // +0x12C rows of items (UpdateScrollRange)
    INT MaxItems;                    // +0x130 (25) AddItem refuses more
    INT CellSize;                    // +0x134 (32) icon size
    INT CellGapX;                    // +0x138 C3: pixels between two cells of a row (the 6-argument constructor's 5th; 0x1001E190 leaves it)
    INT CellGapY;                    // +0x13C the same between two rows (HitTest, OnPaint, OnDrawToolTip add it to CellSize)
    INT Selected;                    // +0x140 -1 = none
    INT HoverIndex;                  // +0x144 item under the mouse, -1 = none

    NCItemWnd();                                                     // 0x1001e190
    // C3: the cell gaps are arguments too (ret 0x18)
    NCItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY); // 0x1001e2b0
    // ~NCItemWnd                                                    // 0x100138d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10093f30
    virtual INT OnCreate();                                          // 37 0x1001cee0
    virtual INT OnDestroy();                                         // 38 0x1001c860
    virtual void ShowWindow();                                       // 44 0x1001cc20 (the scroll range follows)
    virtual void HideWindow();                                       // 45 0x1001cad0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1001cf30
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 57 0x1001e350
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x1001d200
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x1001d0f0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1001ca60 the base's, answered 1
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 67 0x1001c9b0
    // by rows instead of NCScrollWnd's pixels
    virtual INT ScrollUp();                                          // 69 0x1001cc50
    virtual INT ScrollDown();                                        // 70 0x1001cc90
    // C3: slot 71 is NCScrollWnd's OnScrollBar (0x100290e0, ret 4); this class overrides it with 0x1001cc30
    // (TopRow = Value), so Clear, UnselectAll and DrawItemToolTip (and every virtual a subclass adds) sit at the
    // stock slots 72, 73, 74, 75...
    virtual INT OnScrollBar(INT Value);                              // 71 0x1001cc30
    // names ours
    virtual void Clear();                                            // 72 0x1001ddc0 deletes the items
    virtual void UnselectAll();                                      // 73 0x1001d270
    // text of the selected item's tool tip box, from (X, Y); nothing here
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 74 0x1001cce0 (folded)

    // names ours
    __declspec(noinline) INT HitTest(INT X, INT Y, INT OffsetX = 0, INT OffsetY = 0);                                       // 0x1001c8e0 item index, -1 outside
    __declspec(noinline) INT GetSelected();                                               // 0x1001c980 -1: none
    __declspec(noinline) INT GetItemCount();                                              // 0x1001c9a0
    __declspec(noinline) void InitScrollBar();                                            // 0x1001cb70
    __declspec(noinline) void UpdateScrollRange();                                        // 0x1001cbb0 after adding items
    __declspec(noinline) void SetVisibleRows(INT Rows);                                   // 0x1001cb60 (shared body: [this+0x124] = Rows)
    // the skill grid's own (NCSkillItemWnd): cells of CellSize + 5 by CellSize + 3 whatever the gaps are
    __declspec(noinline) INT HitTestSkill(INT X, INT Y);                                  // 0x100a10a0 item index, -1 outside (name ours)
};
NW_ASSERT_OFFSET(NCItemWnd, BackTex, 0x100);
NW_ASSERT_OFFSET(NCItemWnd, ToolTipTex, 0x108);
NW_ASSERT_OFFSET(NCItemWnd, Items, 0x10C);
NW_ASSERT_OFFSET(NCItemWnd, Columns, 0x120);
NW_ASSERT_OFFSET(NCItemWnd, MaxItems, 0x130);
NW_ASSERT_OFFSET(NCItemWnd, CellGapX, 0x138);
NW_ASSERT_OFFSET(NCItemWnd, HoverIndex, 0x144);
NW_ASSERT_SIZE(NCItemWnd, 0x148);

#pragma pack(pop)