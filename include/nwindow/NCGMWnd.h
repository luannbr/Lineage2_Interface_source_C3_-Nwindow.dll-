#pragma once
// The GM's windows. NCGMWnd (UClass "CGMWnd", 0x118; desktop +0x190, //gmwnd): a player's name
// and the buttons that ask the server about that player and open the matching view window of
// the desktop (NCConsole::ToggleGM*): Status (NCGMDetailStatusWnd), Inventory, Skills
// (NCGMMagicSkillWnd), Quest, Warehouse (NCGMStoreWnd) and Clan; the builder commands of the other
// buttons (teleportto, add_peti_chat, debug, instant_move), the searches of an NPC, an item and a
// skill by name, and one that closes the view windows.
//
// The view windows are the player's windows under a title bar, filled by the console with the
// server's answer and made read-only: nothing is dragged out of their grids (the item windows'
// release and right button only put the carried item down), the quest window's Cancel is
// disabled, the clan page's buttons are all disabled.
//   NCGMInventoryWnd (0xE4; desktop +0x194): the inventory's paperdoll and two pages with the
//     adena, no weight bar, no trash or crystallize icons.
//   NCGMQuestWnd (0x18C; desktop +0x1A0; C3: NCQuestGaraTreeWnd's 0x180 bytes and a list of item counts),
//     NCGMActionWnd (0xCC; desktop +0x1A8, the clan page NCGMPledgeWnd only).

#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCPledgeWnd.h"
#include "nwindow/NCQuestGaraTreeWnd.h"
#include "nwindow/NCQuestWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCEditBox;
class NCTabControl;
struct NItemInfo;

// NWM_COMMAND ids (names ours)
enum
{
    NID_GM_NAME = 0x1505,
    NID_GM_INVENTORY = 0xF0F006,
    NID_GM_STATUS = 0xF0F007,
    NID_GM_SKILLS = 0xF0F008,
    NID_GM_QUEST = 0xF0F00A,
    NID_GM_CLAN = 0xF0F00B,
    NID_GM_WAREHOUSE = 0xF0F00C,
    NID_GM_FRAME = 0xF0F015,
    NID_GM_CLOSE = 0xF0F016,
    NID_GM_NEW1 = 0xF0F00D,           // new in C2: teleportto
    NID_GM_NEW7 = 0xF0F00E,           // C3, name ours: the dialog 0x4B (slot 76)
    NID_GM_NEW2 = 0xF0F03F,           // add_peti_chat (slot 82)
    NID_GM_NEW8 = 0xF0F03D,           // instant_move (slot 80)
    NID_GM_NEW3 = 0xF0F05D,           // debug (slot 83)
    NID_GM_NEW9 = 0xF0F03E,           // the dialog 0x4A (slot 81)
    NID_GM_NEW4 = 0xF0F01C,
    NID_GM_NEW5 = 0xF0F02C,
    NID_GM_NEW6 = 0xF0F03C,
    NID_GMINVENTORY_FRAME = 0xF0F00D,
    NID_GMINVENTORY_TABS = 0xF0F00E,
    NID_GMACTION_FRAME = 0xF0F018,
    NID_GMQUEST_FRAME = 0xF0F119,
    NID_GMQUEST_TABS = 0xF0F120,
    NID_GMQUEST_CANCEL = 0xF0F121,     // disabled, and the window procedure ignores it
    NID_GMQUEST_LIST = 0xF0F122,       // not handled: a quest clicked shows nothing
};

class NCGMWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGMWnd, NCWnd, 0, 0x1027F850)

    UTexture* TopTex;                // +0xC8 "sek.ui02"
    UTexture* MidTex;                // +0xCC "sek.cbui157"
    UTexture* BottomTex;             // +0xD0 "sek.ui03"
    NCEditBox* NameEdit;             // +0xD4 the player's name (ToggleGMWnd focuses it)
    NCButton* InventoryButton;       // +0xD8 id 0xF0F006, sysstring 138
    NCButton* SkillsButton;          // +0xDC id 0xF0F008, sysstring 119
    NCButton* StatusButton;          // +0xE0 id 0xF0F007, sysstring 433
    NCButton* QuestButton;           // +0xE4 id 0xF0F00A, sysstring 118
    NCButton* WarehouseButton;       // +0xE8 id 0xF0F00C, sysstring 131
    NCButton* ClanButton;            // +0xEC id 0xF0F00B, sysstring 439
    // C3: the buttons the C2 client added are fewer / more: nine, in the order OnCreate makes them (names ours)
    NCButton* NewButton1;            // +0xF0 id 0xF0F00D, sysstring 899 (slot 75: teleportto)
    NCButton* NewButton7;            // +0xF4 id 0xF0F00E, text 688 (slot 76)
    NCButton* NewButton2;            // +0xF8 id 0xF0F03F, sysstring 902 (slot 82: add_peti_chat)
    NCButton* NewButton8;            // +0xFC id 0xF0F03D, sysstring 900 (slot 80: instant_move)
    NCButton* NewButton3;            // +0x100 id 0xF0F05D, text 901 (slot 83: debug)
    NCButton* NewButton9;            // +0x104 id 0xF0F03E, text 903 (slot 81)
    NCButton* NewButton4;            // +0x108 id 0xF0F01C, text 690 (slot 77: NPC by name)
    NCButton* NewButton5;            // +0x10C id 0xF0F02C, text 691 (slot 78: item by name)
    NCButton* NewButton6;            // +0x110 id 0xF0F03C, text 692 (slot 79: skill by name)
    NCButton* CloseButton;           // +0x114 id 0xF0F016 (slot 74), "sek.cbui27" when pushed

    NCGMWnd();                                                       // 0x10085490
    // ~NCGMWnd                                                      // 0x10085510

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1007e560
    virtual INT OnCreate();                                          // 37 0x100828b0
    virtual INT OnDestroy();                                         // 38 0x10081f10
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10082270
    // the buttons (names ours)
    virtual INT OnInventory();                                       // 68 0x1007e780
    virtual INT OnSkills();                                          // 69 0x1007e7e0
    virtual INT OnStatus();                                          // 70 0x1007e840
    virtual INT OnQuest();                                           // 71 0x1007e8a0
    virtual INT OnWarehouse();                                       // 72 0x1007e900
    virtual INT OnClan();                                            // 73 0x1007e960
    virtual INT OnClose();                                           // 74 0x1007e9c0
    // the builder commands "teleportto", "add_peti_chat", "debug" and "instant_move" for the name typed (1, 2, 3 and 8), two
    // dialog boxes asking about it (7: sysmsg 0x4C4, 9: 0x4C5) and the searches of an NPC, an item and a skill by
    // name (4 .. 6: the match's id and name in a dialog box)
    virtual INT OnNewButton1();                                      // 75 0x1007eb20 (id 0xF0F00D)
    virtual void NewVirtual76();  // 76 0x100823b0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual INT OnNewButton4();                                      // 77 0x10083ba0 (id 0xF0F01C) NPC table (GL2GameData +0x180C)
    virtual INT OnNewButton5();                                      // 78 0x10083d10 (id 0xF0F02C) item table (+0x1808)
    virtual INT OnNewButton6();                                      // 79 0x10083e70 (id 0xF0F03C) skill table (+0x3CDC)
    virtual void NewVirtual80();  // 80 0x1007ebe0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual81();  // 81 0x10082470 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual82();  // 82 0x1007ec80 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual83();  // 83 0x1007ed40 (only this client has it; defined in NNewVirtuals.gen.cpp)

    // the bodies behind the placeholders (slots 76, 80 .. 83; WindowProc calls the slots)
    INT OnNewButton7();                                              // 0x100823b0 (name ours) slot 76: dialog 0x4B, sysmsg 0x4C4 with the name
    INT OnNewButton8();                                              // 0x1007ebe0 (name ours) slot 80: the builder command instant_move
    INT OnNewButton9();                                              // 0x10082470 (name ours) slot 81: dialog 0x4A, sysmsg 0x4C5 with the name
    INT OnNewButton2();                                              // 0x1007ec80 (id 0xF0F03F) slot 82
    INT OnNewButton3();                                              // 0x1007ed40 (id 0xF0F05D) slot 83
};
NW_ASSERT_OFFSET(NCGMWnd, NameEdit, 0xD4);
NW_ASSERT_OFFSET(NCGMWnd, ClanButton, 0xEC);
NW_ASSERT_OFFSET(NCGMWnd, NewButton3, 0x100);
NW_ASSERT_OFFSET(NCGMWnd, CloseButton, 0x114);
NW_ASSERT_SIZE(NCGMWnd, 0x118);

// the GM inventory's pages (UClass "CGMInvenItemWnd", 0x164)
class NCGMInvenItemWnd : public NCInvenItemWnd
{
    NW_DECLARE_CLASS(NCGMInvenItemWnd, NCInvenItemWnd, 0, 0x102807D0)

    NCGMInvenItemWnd();                                              // 0x100843e0
    // C3: the four-argument one is NCInvenItemWnd's six-argument constructor with the gaps 1 and 1 (ret 0x10)
    NCGMInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x10084480
    // ~NCGMInvenItemWnd                                             // 0x10084430

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1007e1b0
    // NCGMSkillItemWnd::OnLButtonUp's body (0x1007e280, hooked there): unhooked copies
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1007e280 (folded)
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 64 0x1007e280 (folded)
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 75 0x1007e280 (folded)
};
NW_ASSERT_SIZE(NCGMInvenItemWnd, 0x164);

// the GM inventory's paperdoll (UClass "CGMEquipItemWnd", 0x28C)
class NCGMEquipItemWnd : public NCEquipItemWnd
{
    NW_DECLARE_CLASS(NCGMEquipItemWnd, NCEquipItemWnd, 0, 0x102802A8)
    BYTE bFirstRingTaken;                 // +0x288 (name ours; 0 from the constructor and from NCGMInventoryWnd::ClearItems) AddItem:
                                          //   the first item with the body part bit 2 / 4 (the earrings) keeps bit 4 and sets it
    BYTE bFirstFingerTaken;               // +0x289 (the same) bits 0x10 / 0x20 (the rings)
    BYTE _UnknownTail28A[0x28C - 0x28A];  // +0x28A padding up to sizeof

    NCGMEquipItemWnd(INT InCellSize = 32);                           // 0x10084330
    // ~NCGMEquipItemWnd                                             // 0x10084390

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1007e070
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1007ee00
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1007e090
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 64 0x1007e090 (folded)
    virtual INT GetEquipSlot(INT BodyPart, INT ObjectID);            // 68 0x1007e0b0 the slot by the body part bits alone
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 69 0x1007e090 (folded)

    // names ours (the console's adds go through NCGMInventoryWnd::AddItem)
    UBOOL AddItem(NItemInfo* Info);                                  // 0x10082530 NCEquipItemWnd's, but a second ring (earring)
                                                                     //   is shown on the other slot
};
NW_ASSERT_OFFSET(NCGMEquipItemWnd, bFirstRingTaken, 0x288);
NW_ASSERT_SIZE(NCGMEquipItemWnd, 0x28C);

class NCGMInventoryWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGMInventoryWnd, NCWnd, 0, 0x10281740)

    UTexture* BackTex;               // +0xC8 "L2UI_CH3.InventoryWnd.Inventory_Back"
    UTexture* AdenaTex;              // +0xCC "L2UI.WindowIcon.WindowIconAdena"
    UTexture* WeightTex;             // +0xD0 "L2UI.WindowIcon.Weight" (name ours; drawn at the bottom, no bar)
    NCGMInvenItemWnd* ItemWnd[2];    // +0xD4 the pages: 0 "Item", 1 "Quest"
    NCGMEquipItemWnd* EquipWnd;      // +0xDC
    NCTabControl* TabCtrl;           // +0xE0

    NCGMInventoryWnd();                                              // 0x100844b0
    // ~NCGMInventoryWnd                                             // 0x10084510

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x10084560
    virtual INT OnDestroy();                                         // 38 0x1007f650
    virtual void ShowWindow();                                       // 44 0x1007e220
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1007f470

    // the console's calls (names ours)
    void ClearItems();                                               // 0x1007e1d0
    UBOOL AddItem(NItemInfo* Info);                                  // 0x10082740
    void AddHenna(INT HennaID, INT bUsable);                         // 0x1007e240 (name ours) the paperdoll's (a tail jump)
    void ClearHennas();                                              // 0x1007e250 (name ours) the paperdoll's (a tail jump)
};
NW_ASSERT_OFFSET(NCGMInventoryWnd, ItemWnd, 0xD4);
NW_ASSERT_OFFSET(NCGMInventoryWnd, TabCtrl, 0xE0);
NW_ASSERT_SIZE(NCGMInventoryWnd, 0xE4);

// An item the GM's quest window counts (+0x180's elements; names ours)
struct NGMQuestCount
{
    INT ItemID;            // +0x00 compared with the quest's required item ids
    INT Count;             // +0x04 the amount the player holds (the console adds the entries)
};

class NCGMQuestWnd : public NCQuestGaraTreeWnd
{
    NW_DECLARE_CLASS(NCGMQuestWnd, NCQuestGaraTreeWnd, 0, 0x1027F330)

    TArray<NGMQuestCount> Counts;         // +0x180 (C3, GM only) empty after OnCreate; the console fills it (AddQuestItem)

    NCGMQuestWnd();                                                  // 0x10084FA0 (the base's, then +0x180 cleared)
    // ~NCGMQuestWnd                                                 // 0x10085000 (Counts, then the base's 0x10084280)

    // the base's overrides it replaces
    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1007E310 (NCQuestGaraTreeWnd's as 0 / 1)
    virtual INT OnCreate();                                          // 37 0x100836D0
    virtual INT OnDestroy();                                         // 38 0x100EB490 (folded: NCSSQMainEventWnd's)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10081780
    // 68 0x10082870 the base's ClearList with the item counts emptied and the number of quests cleared. ShowWindow
    // (44, 0x100DA310), the tool tip (57, 0x100DC8A0), the mouse move (61, 0x100DB6D0) and the two buttons (69 / 70,
    // 0x100DB4A0 / 0x100DC050) are the base's
    virtual void ClearList();
};
NW_ASSERT_OFFSET(NCGMQuestWnd, Counts, 0x180);
NW_ASSERT_SIZE(NCGMQuestWnd, 0x18C);

// the GM's clan page (UClass "CGMPledgeWnd", 0x964): drawn 17 pixels lower than the player's
class NCGMPledgeWnd : public NCPledgeWnd
{
    NW_DECLARE_CLASS(NCGMPledgeWnd, NCPledgeWnd, 0, 0x10281220)

    NCGMPledgeWnd();                                                 // 0x10085100
    // ~NCGMPledgeWnd                                                // 0x10085150

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1007e3a0
    virtual INT OnDestroy();                                         // 38 0x1007e3c0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10081a40
};
NW_ASSERT_SIZE(NCGMPledgeWnd, 0x964);

class NCGMActionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGMActionWnd, NCWnd, 0, 0x1027DEA0)

    NCGMPledgeWnd* PledgeWnd;        // +0xC8 the clan page under the title bar

    NCGMActionWnd();                                                 // 0x100851a0
    // ~NCGMActionWnd                                                // 0x100851f0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119f60 (folded)
    virtual INT OnCreate();                                          // 37 0x10085240
    virtual INT OnDestroy();                                         // 38 0x10081e60
    virtual void ShowWindow();                                       // 44 0x1007e440
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10081e00
};
NW_ASSERT_OFFSET(NCGMActionWnd, PledgeWnd, 0xC8);
NW_ASSERT_SIZE(NCGMActionWnd, 0xCC);

#pragma pack(pop)
