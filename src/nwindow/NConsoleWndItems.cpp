/*=============================================================================
    NConsoleWndItems.cpp - what the engine tells the console about items (UGameEngine::On*
    handlers, one slot each): the inventory, the trade, the NPC shops, the warehouse, the
    freight, the private stores, the VIP shop, the quest items, the enchant scrolls. Each item
    the server lists becomes an NItemInfo for its window: the texts and the icon from the game
    data, the values of the item from ItemInfo and those of its kind (weapon, armor or etc
    item) from the item table. Every list takes its own part of those.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleWndExt.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCTradeWnd.h"
#include "nwindow/NCShopWnd.h"
#include "nwindow/NCShopPreviewWnd.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NCDeliverWnd.h"
#include "nwindow/NCVIPShopWnd.h"
#include "nwindow/NCPrivateShopWnd.h"
#include "nwindow/NCPrivateBuyWnd.h"
#include "nwindow/NCPetWnd.h"
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCQuestWnd.h"
#include "nwindow/NCGMWnd.h"
#include "nwindow/NCNPCHtmlViewer.h"
#include "nwindow/NCTrainWnd.h"
#include "nwindow/NCManorShopWnd.h"

// system messages (systemmsg ids)
enum
{
    MSG_AskTrade = 100,                // "$s1 has requested a trade. Do you wish to continue?"
    MSG_WarehouseFailed = 307,         // "You have failed to trade with the warehouse."
};

// the dialog boxes opened here (NDialogInfo::ID; NCDialogBox sends the answer)
enum { DIALOG_TRADE_REQUEST = 7 };

// NItemInfo as this client lays it out: the constructor (0x10047400) stores a new dword (-1) at +0x178C and every field of
// the C2 layout from there on sits 4 bytes later (the word at +0x1790, then +0x1794 .. +0x17A8). include/nwindow/NItemInfo.h
// still has the C2 offsets (db/struct_sizes.csv has no shift for it, only the 4 bytes of padding at the end), so these
// fields are reached by their C3 offset here; the names are the header's.
#define NINFO_C3(Info, Type, Off) (*(Type*)((BYTE*)&(Info) + (Off)))
static FORCEINLINE INT& InfoNew178C(NItemInfo& I) { return NINFO_C3(I, INT, 0x178C); }      // C3 only (-1)
static FORCEINLINE SWORD& InfoC84(NItemInfo& I) { return NINFO_C3(I, SWORD, 0x1790); }     // C2 +0x178C (engine ItemInfo +0x1C)
static FORCEINLINE UBOOL& InfoStackable(NItemInfo& I) { return NINFO_C3(I, UBOOL, 0x1794); }  // C2 +0x1790
static FORCEINLINE INT& InfoEquipped(NItemInfo& I) { return NINFO_C3(I, INT, 0x1798); }    // C2 +0x1794 (UnknownC8C: worn)
static FORCEINLINE INT& InfoC90(NItemInfo& I) { return NINFO_C3(I, INT, 0x179C); }         // C2 +0x1798 (-1)
static FORCEINLINE INT& InfoC94(NItemInfo& I) { return NINFO_C3(I, INT, 0x17A0); }         // C2 +0x179C
static FORCEINLINE INT& InfoC17A0(NItemInfo& I) { return NINFO_C3(I, INT, 0x17A4); }       // C2 +0x17A0
static FORCEINLINE INT& InfoC98(NItemInfo& I) { return NINFO_C3(I, INT, 0x17A8); }         // C2 +0x17A4

// the texts of the item and its icon's name
static FORCEINLINE void SetItemTexts(NItemInfo& Info, INT ItemID, NViewItemData* Data)
{
    appStrcpy(Info.Name, GL2GameData.GetItemName(ItemID));
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(ItemID));
}

// the values of a weapon, an armor or an etc item from the item table (the NItemInfo fields
// the item windows copy to NItemEntry's P.Atk, M.Atk, P.Def, M.Def, the shield's values, the
// crystal grade, the attack speed, the armor type, the MP bonus and the shots used)
static FORCEINLINE void SetWeaponValues(NItemInfo& Info, NViewItemData* Data)
{
    Info.UnknownC28 = Data->Weight;
    Info.UnknownC2C = Data->Unknown48;
    Info.UnknownC30 = Data->WeaponType;
    Info.UnknownC34[0] = Data->PAtk;
    Info.UnknownC34[1] = Data->MAtk;
    Info.UnknownC34[4] = Data->ShieldPDef;
    Info.UnknownC34[5] = Data->ShieldRate;
    Info.UnknownC34[6] = Data->Unknown40;
    Info.UnknownC34[7] = Data->CrystalGrade;
    Info.UnknownC34[8] = Data->Unknown90;
    Info.UnknownC34[9] = Data->UnknownA4;
    Info.UnknownC34[10] = Data->UnknownA8;
    Info.UnknownC34[11] = Data->AttackSpeed;
    Info.UnknownC34[12] = Data->UnknownBC;
    Info.UnknownC6C = Data->Evasion;
    Info.UnknownC7C = Data->SoulShots;
    Info.UnknownC80 = Data->SpiritShots;
}
static FORCEINLINE void SetArmorValues(NItemInfo& Info, NViewItemData* Data)
{
    Info.UnknownC28 = Data->Weight;
    Info.UnknownC2C = Data->Unknown48;
    Info.UnknownC34[6] = Data->Unknown40;
    Info.UnknownC34[7] = Data->ArmorCrystalGrade;
    Info.SubType = Data->ArmorType;
    Info.UnknownC6C = Data->ArmorEvasion;
    Info.UnknownC34[2] = Data->PDef;
    Info.UnknownC34[3] = Data->MDef;
    Info.UnknownC78 = Data->MPBonus;
}
static FORCEINLINE void SetEtcValues(NItemInfo& Info, NViewItemData* Data)
{
    Info.UnknownC28 = Data->Weight;
    Info.UnknownC2C = Data->Unknown48;
    Info.UnknownC34[6] = Data->Unknown40;
    InfoStackable(Info) = Data->bStackable;
}
static FORCEINLINE void SetClassValues(NItemInfo& Info, INT Class, NViewItemData* Data)
{
    if (Class == ITEMCLASS_Weapon)
        SetWeaponValues(Info, Data);
    else if (Class == ITEMCLASS_Armor)
        SetArmorValues(Info, Data);
    else if (Class == ITEMCLASS_Etc)
        SetEtcValues(Info, Data);
}

// The weapon's values without Unknown40 (the inventory and pet updates)
static FORCEINLINE void SetUpdatedWeaponValues(NItemInfo& Info, NViewItemData* Data)
{
    Info.UnknownC28 = Data->Weight;
    Info.UnknownC2C = Data->Unknown48;
    Info.UnknownC30 = Data->WeaponType;
    Info.UnknownC34[0] = Data->PAtk;
    Info.UnknownC34[1] = Data->MAtk;
    Info.UnknownC34[4] = Data->ShieldPDef;
    Info.UnknownC34[5] = Data->ShieldRate;
    Info.UnknownC34[7] = Data->CrystalGrade;
    Info.UnknownC34[8] = Data->Unknown90;
    Info.UnknownC34[9] = Data->UnknownA4;
    Info.UnknownC34[10] = Data->UnknownA8;
    Info.UnknownC34[11] = Data->AttackSpeed;
    Info.UnknownC34[12] = Data->UnknownBC;
    Info.UnknownC6C = Data->Evasion;
    Info.UnknownC7C = Data->SoulShots;
    Info.UnknownC80 = Data->SpiritShots;
}

// The weapon's values without the shots, the armor's without the MP bonus (the GM's warehouse,
// the trainer's required items)
static FORCEINLINE void SetWeaponValuesNoShots(NItemInfo& Info, NViewItemData* Data)
{
    Info.UnknownC28 = Data->Weight;
    Info.UnknownC2C = Data->Unknown48;
    Info.UnknownC30 = Data->WeaponType;
    Info.UnknownC34[0] = Data->PAtk;
    Info.UnknownC34[1] = Data->MAtk;
    Info.UnknownC34[4] = Data->ShieldPDef;
    Info.UnknownC34[5] = Data->ShieldRate;
    Info.UnknownC34[6] = Data->Unknown40;
    Info.UnknownC34[7] = Data->CrystalGrade;
    Info.UnknownC34[8] = Data->Unknown90;
    Info.UnknownC34[9] = Data->UnknownA4;
    Info.UnknownC34[10] = Data->UnknownA8;
    Info.UnknownC34[11] = Data->AttackSpeed;
    Info.UnknownC34[12] = Data->UnknownBC;
    Info.UnknownC6C = Data->Evasion;
}
static FORCEINLINE void SetArmorValuesNoMP(NItemInfo& Info, NViewItemData* Data)
{
    Info.UnknownC28 = Data->Weight;
    Info.UnknownC2C = Data->Unknown48;
    Info.UnknownC34[6] = Data->Unknown40;
    Info.UnknownC34[7] = Data->ArmorCrystalGrade;
    Info.SubType = Data->ArmorType;
    Info.UnknownC6C = Data->ArmorEvasion;
    Info.UnknownC34[2] = Data->PDef;
    Info.UnknownC34[3] = Data->MDef;
}

// C2: a field of the item record the header has not placed (+0x54, +0x8C), by offset
static FORCEINLINE INT ItemRecordInt(NViewItemData* Data, INT Offset)
{
    return *(INT*)((BYTE*)Data + Offset);
}

// the inventories' etc items (C2: also the value at +0x8C, the etc type for the entry's display name
// (Unknown1710), and the type 5 flag; type 2: NItemEntry +0x1C98, the paperdoll keeps them)
static FORCEINLINE void SetInventoryEtcValues(NItemInfo& Info, NViewItemData* Data)
{
    SetEtcValues(Info, Data);
    Info.UnknownC34[7] = ItemRecordInt(Data, 0x8C);
    Info.Unknown1710 = Data->EtcType;
    if (Data->EtcType == 2)
        InfoC94(Info) = 1;
    else if (Data->EtcType == 5)
        InfoC17A0(Info) = 1;
}

// The etc values of the pet's and the GM's item lists (C2): the item record's +0x8C and the etc type (the
// inventory's own also flags type 5)
static FORCEINLINE void SetTypedEtcValues(NItemInfo& Info, NViewItemData* Data)
{
    SetEtcValues(Info, Data);
    Info.UnknownC34[7] = ItemRecordInt(Data, 0x8C);
    Info.Unknown1710 = Data->EtcType;
    if (Data->EtcType == 2)
        InfoC94(Info) = 1;
}

// the class values with the typed etc values (C2: the trade lists)
static FORCEINLINE void SetClassValuesTyped(NItemInfo& Info, INT Class, NViewItemData* Data)
{
    if (Class == ITEMCLASS_Weapon)
        SetWeaponValues(Info, Data);
    else if (Class == ITEMCLASS_Armor)
        SetArmorValues(Info, Data);
    else if (Class == ITEMCLASS_Etc)
        SetTypedEtcValues(Info, Data);
}

/*-----------------------------------------------------------------------------
    The inventory.
-----------------------------------------------------------------------------*/

// 0x1005bcb0 - a new list follows (AddInventoryItem); shown when the server says so
void NConsoleWnd::ReceiveItemList(INT bShow)
{
    guard(NConsoleWnd::ReceiveItemList);
    GetDesktop()->InventoryWnd->ClearItems();
    if (bShow)
    {
        GetDesktop()->InventoryWnd->ShowWindow();
        GetDesktop()->InventoryWnd->SetFocus();
    }
    unguard;
}
NW_HOOK_METHOD(0x1005bcb0, NConsoleWnd, ReceiveItemList)

// 0x1005bd60 - (no handler)
void NConsoleWnd::ReceiveUpdateItemList()
{
    guard(NConsoleWnd::ReceiveUpdateItemList);
    GetDesktop()->InventoryWnd->ClearItems();
    unguard;
}
NW_HOOK_METHOD(0x1005bd60, NConsoleWnd, ReceiveUpdateItemList)

// 0x1006cc50 - with the two-slot items' other icons. C2: the name comes from the item record (+0x40), with its
// additional name (+0x44) and the record's +0x54 (Unknown1788)
void NConsoleWnd::AddInventoryItem(ItemInfo& Item)
{
    if (GetDesktop()->InventoryWnd)
    {
        NItemInfo Info;
        NViewItemData* Data = NItemTable::Find(&Item.ItemID);
        if (Item.ItemID >= 0 && Data)
        {
            INT Class = Data->Class;
            appStrcpy(Info.Name, *Data->Name);
            appStrcpy(Info.AdditionalName, *Data->AdditionalName);
            appStrcpy(Info.IconName, *Data->IconName);
            appStrcpy(Info.Unknown200, *Data->IconName2);
            appStrcpy(Info.Unknown300, *Data->IconName3);
            appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
            Info.ID = Item.ItemID;
            Info.Param = Item.Type;
            Info.ObjectID = Item.ObjectID;
            Info.Enchant = Item.Enchant;
            Info.Count = Item.Count;
            Info.BodyPart = Item.BodyPart;
            InfoEquipped(Info) = Item.bEquipped;
            InfoC84(Info) = Item.Unknown1C;
            Info.UnknownC70 = Item.Unknown24;
            Info.Unknown1788 = ItemRecordInt(Data, 0x54);
            if (Class == ITEMCLASS_Weapon)
                SetWeaponValues(Info, Data);
            else if (Class == ITEMCLASS_Armor)
                SetArmorValues(Info, Data);
            else if (Class == ITEMCLASS_Etc)
                SetInventoryEtcValues(Info, Data);
            GetDesktop()->InventoryWnd->AddItem(&Info);
        }
    }
}
NW_HOOK_METHOD(0x1006cc50, NConsoleWnd, AddInventoryItem)

// 0x1006cf80 - an item added (1), changed (2) or removed (3). C2: the name from the game data, the additional
// name and the two other icons from the item record, and its +0x54 (Unknown1788)
void NConsoleWnd::UpdateInventoryItem(INT Change, ItemInfo& Item)
{
    if (!GetDesktop()->InventoryWnd)
        return;
    NItemInfo Info;
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID < 0 || !Data)
        return;
    INT Class = Data->Class;
    appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
    appStrcpy(Info.AdditionalName, *Data->AdditionalName);
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Unknown200, *Data->IconName2);
    appStrcpy(Info.Unknown300, *Data->IconName3);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
    Info.Param = Item.Type;
    Info.ObjectID = Item.ObjectID;
    Info.ID = Item.ItemID;
    Info.Count = Item.Count;
    Info.BodyPart = Item.BodyPart;
    Info.Enchant = Item.Enchant;
    InfoC84(Info) = Item.Unknown1C;
    InfoEquipped(Info) = Item.bEquipped;
    Info.UnknownC70 = Item.Unknown24;
    Info.Unknown1788 = ItemRecordInt(Data, 0x54);
    if (Class == ITEMCLASS_Weapon)
        SetUpdatedWeaponValues(Info, Data);
    else if (Class == ITEMCLASS_Armor)
        SetArmorValues(Info, Data);
    else if (Class == ITEMCLASS_Etc)
        SetInventoryEtcValues(Info, Data);
    GetDesktop()->InventoryWnd->UpdateItem(&Info, Change);
}
NW_HOOK_METHOD(0x1006cf80, NConsoleWnd, UpdateInventoryItem)

// 0x1005be00 - an enchant scroll was used: the inventory waits for the item to enchant (C2: and it also
// calls its virtual 51, SetItemSelect)
void NConsoleWnd::SelectItemToEnchant()
{
    GetDesktop()->InventoryWnd->SetEnchantMode(1);
    GetDesktop()->InventoryWnd->SetItemSelect();
}
NW_HOOK_METHOD(0x1005be00, NConsoleWnd, SelectItemToEnchant)

// 0x1005beb0 - the enchant mode ends whatever the result (the stock switches on 0, 1 and 2,
// the same in each case)
void NConsoleWnd::ReceiveEnchantResult(INT Result)
{
    GetDesktop()->InventoryWnd->SetEnchantMode(0);
}
NW_HOOK_METHOD(0x1005beb0, NConsoleWnd, ReceiveEnchantResult)

// 0x1005bf00 - its three values are not used
void NConsoleWnd::UpdateEquipItem(L2ParamStack& Params)
{
    Params.Top();
    Params.Top();
    Params.Top();
}
NW_HOOK_METHOD(0x1005bf00, NConsoleWnd, UpdateEquipItem)

// 0x1005d1f0 - (no handler) the inventory shown or hidden; shown, it asks for the item list
// first, which shows it (ReceiveItemList)
void NConsoleWnd::ToggleInventory()
{
    if (GetDesktop()->InventoryWnd->IsShowWindow())
        GetDesktop()->InventoryWnd->HideWindow();
    else
        RequestItemList();
}
NW_HOOK_METHOD(0x1005d1f0, NConsoleWnd, ToggleInventory)

/*-----------------------------------------------------------------------------
    The trade.
-----------------------------------------------------------------------------*/

// 0x1006d2d0 - someone asks for a trade: refused at once when not known
void NConsoleWnd::ReceiveTradeDemand(L2ParamStack& Params)
{
    User* Trader = FindUserByID((INT)Params.Top());
    if (Trader)
    {
        NDialogInfo Info;
        Info.ID = DIALOG_TRADE_REQUEST;
        Info.bMainBox = 0;
        Info.bInput = 0;
        Info.SetSystemMessage(MSG_AskTrade);
        Info.SetButtons(Trader->GetName(), NULL);
        Info.bProgress = 1;
        ShowDialog(&Info);
    }
    else
    {
        L2ParamStack Answer(10);
        Answer.PushBack(NULL);
        // virtual 397 of this client (NetHandler +0x294: the answer to a trade offer); called by slot: the header's
        // NewVirtualN placeholders sit after slot 389 there, which moves AnswerTradeRequest
        nw::VirtualCall<void>(this, 397, &Answer);
    }
}
NW_HOOK_METHOD(0x1006d2d0, NConsoleWnd, ReceiveTradeDemand)

// 0x1006d3e0 - the trade window opens (instead of the inventory) with both names. C3: a missing player or partner
// ends the trade (RequestTradeDone(0)) instead of dereferencing them, and the window takes the focus
void NConsoleWnd::ReceiveTradeStart(INT ObjectID)
{
    guard(NConsoleWnd::ReceiveTradeStart);
    if (GetDesktop()->InventoryWnd->IsShowWindow())
        GetDesktop()->InventoryWnd->HideWindow();
    GetDesktop()->TradeWnd->Reset();
    if (GetMyUser())
    {
        GetDesktop()->TradeWnd->SetMyName(GetMyUser()->GetName());
        if (FindUserByID(ObjectID))
        {
            GetDesktop()->TradeWnd->SetPartnerName(FindUserByID(ObjectID)->GetName());
            GetDesktop()->TradeWnd->ShowWindow();
            GetDesktop()->TradeWnd->SetFocus();
            return;
        }
    }
    RequestTradeDone(0);
    unguard;
}
NW_HOOK_METHOD(0x1006d3e0, NConsoleWnd, ReceiveTradeStart)

// The three trade lists, from an item by value: the inventory's, the player's offer and the
// partner's. C2: the additional name from the item record (+0x44) and the typed etc values
static FORCEINLINE UBOOL MakeTradeItem(NItemInfo& Info, NItemRequest& Item)
{
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID < 0 || !Data)
        return 0;
    INT Class = Data->Class;
    appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
    appStrcpy(Info.AdditionalName, *Data->AdditionalName);
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
    Info.ID = Item.ItemID;
    Info.Param = Item.Type;
    Info.ObjectID = Item.ObjectID;
    Info.Enchant = Item.Enchant;
    Info.Count = Item.Count;
    Info.BodyPart = Item.BodyPart;
    InfoEquipped(Info) = Item.bEquipped;
    InfoC84(Info) = Item.Unknown1C;
    Info.UnknownC70 = Item.Unknown24;
    SetClassValuesTyped(Info, Class, Data);
    return 1;
}

// 0x1006d530
void NConsoleWnd::ReceiveAddTradeInventoryItem(NItemRequest Item)
{
    NItemInfo Info;
    if (MakeTradeItem(Info, Item))
        GetDesktop()->TradeWnd->AddInventoryItem(&Info);
}
NW_HOOK_METHOD(0x1006d530, NConsoleWnd, ReceiveAddTradeInventoryItem)

// 0x1006d830
void NConsoleWnd::ReceiveAddTradeOwnItem(NItemRequest Item)
{
    NItemInfo Info;
    if (MakeTradeItem(Info, Item))
        GetDesktop()->TradeWnd->AddMyItem(&Info);
}
NW_HOOK_METHOD(0x1006d830, NConsoleWnd, ReceiveAddTradeOwnItem)

// 0x1006db30
void NConsoleWnd::ReceiveAddTradeOtherItem(NItemRequest Item)
{
    NItemInfo Info;
    if (MakeTradeItem(Info, Item))
        GetDesktop()->TradeWnd->AddPartnerItem(&Info);
}
NW_HOOK_METHOD(0x1006db30, NConsoleWnd, ReceiveAddTradeOtherItem)

// 0x1006de30 - the inventory list of the trade changed (1 add, 3 change...)
void NConsoleWnd::ReceiveTradeUpdate(INT Change, ItemInfo& Item)
{
    if (!GetDesktop()->TradeWnd)
        return;
    NItemInfo Info;
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID < 0 || !Data)
        return;
    INT Class = Data->Class;
    appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
    appStrcpy(Info.AdditionalName, *Data->AdditionalName);
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
    Info.ID = Item.ItemID;
    Info.Param = Item.Type;
    Info.ObjectID = Item.ObjectID;
    Info.Enchant = Item.Enchant;
    InfoEquipped(Info) = Item.bEquipped;
    Info.Count = Item.Count;
    Info.BodyPart = Item.BodyPart;
    InfoC84(Info) = Item.Unknown1C;
    Info.UnknownC70 = Item.Unknown24;
    SetClassValuesTyped(Info, Class, Data);
    GetDesktop()->TradeWnd->UpdateInventoryItem(Change, &Info);
}
NW_HOOK_METHOD(0x1006de30, NConsoleWnd, ReceiveTradeUpdate)

// 0x1005c160 - the trade ended (the result is not read)
void NConsoleWnd::ReceiveTradeDoneMsg(INT Result)
{
    guard(NConsoleWnd::ReceiveTradeDoneMsg);
    GetDesktop()->TradeWnd->Reset();
    GetDesktop()->TradeWnd->HideWindow();
    unguard;
}
NW_HOOK_METHOD(0x1005c160, NConsoleWnd, ReceiveTradeDoneMsg)

/*-----------------------------------------------------------------------------
    The NPC shops and the warehouse: the window opens in its mode, the lists follow.
-----------------------------------------------------------------------------*/

// the shops' items (the NPC's and the player's). C2: the additional name from the item record (+0x44) and the
// typed etc values
static FORCEINLINE UBOOL MakeShopItem(NItemInfo& Info, ItemInfo& Item)
{
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID < 0 || !Data)
        return 0;
    INT Class = Data->Class;
    appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
    appStrcpy(Info.AdditionalName, *Data->AdditionalName);
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
    Info.Param = Item.Type;
    Info.ID = Item.ItemID;
    Info.ObjectID = Item.ObjectID;
    Info.Count = Item.Count;
    Info.BodyPart = Item.BodyPart;
    Info.Enchant = Item.Enchant;
    Info.UnknownC70 = Item.Unknown24;
    Info.UnknownC1C = Item.Unknown10;
    InfoEquipped(Info) = Item.bEquipped;
    InfoC84(Info) = Item.Unknown1C;
    SetClassValuesTyped(Info, Class, Data);
    return 1;
}

// the warehouse's items (and the freight's). C2: as the shops' with Unknown2C instead of Unknown10
static FORCEINLINE UBOOL MakeStoreItem(NItemInfo& Info, ItemInfo& Item)
{
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID < 0 || !Data)
        return 0;
    INT Class = Data->Class;
    appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
    appStrcpy(Info.AdditionalName, *Data->AdditionalName);
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
    Info.Param = Item.Type;
    Info.ID = Item.ItemID;
    Info.ObjectID = Item.ObjectID;
    Info.Count = Item.Count;
    Info.BodyPart = Item.BodyPart;
    Info.Enchant = Item.Enchant;
    Info.UnknownC70 = Item.Unknown24;
    InfoEquipped(Info) = Item.bEquipped;
    InfoC84(Info) = Item.Unknown1C;
    InfoC90(Info) = Item.Unknown2C;
    SetClassValuesTyped(Info, Class, Data);
    return 1;
}

// 0x1005c550 - the NPC shop in its buy mode: the list's id and the player's adena. C3: the desktop's window at +0x114
// (new in C3: its slot is not placed in NCConsole.h) is hidden with the NPC dialog, and the shop takes the focus
void NConsoleWnd::StartBuyList(INT Adena, INT ListID)
{
    GetDesktop()->NpcHtmlWnd->HideWindow();
    nw::Field<NCWnd*>(GetDesktop(), 0x114)->HideWindow();
    GetDesktop()->ShopWnd->Clear();
    GetDesktop()->ShopWnd->SetMode(SHOPMODE_Buy);
    GetDesktop()->ShopWnd->SetListID(ListID);
    GetDesktop()->ShopWnd->SetAdena(Adena);
    GetDesktop()->ShopWnd->ShowWindow();
    GetDesktop()->ShopWnd->SetFocus();
}
NW_HOOK_METHOD(0x1005c550, NConsoleWnd, StartBuyList)

// 0x1006e710 - the NPC shop's buy list. C3: the weapons' and armors' slot index (item record +0x6C) goes into the new
// dword of the NItemInfo (+0x178C), the etc items' -1
void NConsoleWnd::AddBuyList(ItemInfo& Item)
{
    NItemInfo Info;
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID < 0 || !Data)
        return;
    INT Class = Data->Class;
    appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
    appStrcpy(Info.AdditionalName, *Data->AdditionalName);
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
    Info.Param = Item.Type;
    Info.ID = Item.ItemID;
    Info.ObjectID = Item.ObjectID;
    Info.Count = Item.Count;
    Info.BodyPart = Item.BodyPart;
    Info.Enchant = Item.Enchant;
    Info.UnknownC70 = Item.Unknown24;
    Info.UnknownC1C = Item.Unknown10;
    InfoEquipped(Info) = Item.bEquipped;
    InfoC84(Info) = Item.Unknown1C;
    if (Class == ITEMCLASS_Weapon)
    {
        SetWeaponValues(Info, Data);
        InfoNew178C(Info) = Data->BodyPart;
    }
    else if (Class == ITEMCLASS_Armor)
    {
        SetArmorValues(Info, Data);
        InfoNew178C(Info) = Data->BodyPart;
    }
    else if (Class == ITEMCLASS_Etc)
    {
        // the etc item's values with its type but without the type 2 flag (the other lists set UnknownC94 there)
        SetEtcValues(Info, Data);
        Info.UnknownC34[7] = ItemRecordInt(Data, 0x8C);
        Info.Unknown1710 = Data->EtcType;
        InfoNew178C(Info) = -1;
    }
    GetDesktop()->ShopWnd->AddShopItem(&Info);
}
NW_HOOK_METHOD(0x1006e710, NConsoleWnd, AddBuyList)

// 0x1006ea20 (C3 virtual 62) - a good of the try-on shop (the NPC's list, SHOPMODE_Preview): AddBuyList's record without
// the object id, the enchant, the worn flag and the extra words, into the window at +0x114 (NCShopPreviewWnd)
void NConsoleWnd::AddPreviewItem(ItemInfo& Item)
{
    NItemInfo Info;
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID < 0 || !Data)
        return;
    INT Class = Data->Class;
    appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
    appStrcpy(Info.AdditionalName, *Data->AdditionalName);
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
    Info.Param = Item.Type;
    Info.Count = Item.Count;
    Info.ID = Item.ItemID;
    Info.UnknownC1C = Item.Unknown10;
    Info.BodyPart = Item.BodyPart;
    if (Class == ITEMCLASS_Weapon)
    {
        SetWeaponValues(Info, Data);
        InfoNew178C(Info) = Data->BodyPart;
    }
    else if (Class == ITEMCLASS_Armor)
    {
        SetArmorValues(Info, Data);
        InfoNew178C(Info) = Data->BodyPart;
    }
    else if (Class == ITEMCLASS_Etc)
    {
        SetEtcValues(Info, Data);
        Info.UnknownC34[7] = ItemRecordInt(Data, 0x8C);
        Info.Unknown1710 = Data->EtcType;
        InfoNew178C(Info) = -1;
    }
    static_cast<NCShopPreviewWnd*>(GetDesktop()->ShopPreviewWnd)->AddShopItem(&Info);
}
NW_HOOK_METHOD(0x1006ea20, NConsoleWnd, AddPreviewItem)

// 0x1005c680 (C3 virtual 61) - the try-on shop an NPC opened (the NPC's object id, the player's adena and the list's id):
// the NPC dialog and the ordinary shop hide, the preview window (+0x114) is emptied, takes its mode and the numbers,
// then shows with the focus
void NConsoleWnd::StartPreviewList(INT OwnerID, INT Adena, INT ListID)
{
    NCShopPreviewWnd* Preview = static_cast<NCShopPreviewWnd*>(GetDesktop()->ShopPreviewWnd);
    GetDesktop()->NpcHtmlWnd->HideWindow();
    GetDesktop()->ShopWnd->HideWindow();
    Preview->Clear();
    Preview->SetMode(SHOPMODE_Preview);
    Preview->SetListID(ListID);
    Preview->SetOwnerID(OwnerID);
    Preview->SetAdena(Adena);
    Preview->ShowWindow();
    Preview->SetFocus();
}
NW_HOOK_METHOD(0x1005c680, NConsoleWnd, StartPreviewList)

// 0x10073aa0 (guard name) - the texts of an item entry from the item table by its id: name, additional name, icon and
// description (nothing without an entry or when the table has no such item)
void NConsoleWnd::MakeItemInfo(NItemEntry* Entry)
{
    guard(NConsoleWnd::MakeItemInfo);
    if (Entry)
    {
        NViewItemData* Data = NItemTable::Find(&Entry->ID);
        if (Data)
        {
            appStrcpy(Entry->Name, *Data->Name);
            appStrcpy(Entry->AdditionalName, *Data->AdditionalName);
            appStrcpy(Entry->IconName, *Data->IconName);
            appStrcpy(Entry->Description, *Data->Description);
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x10073aa0, NConsoleWnd, MakeItemInfo)

// 0x1005c740 - the sell mode, instead of the inventory (C3: as StartBuyList, the +0x114 window and the focus)
void NConsoleWnd::StartSellList(INT Adena, INT ListID)
{
    GetDesktop()->NpcHtmlWnd->HideWindow();
    nw::Field<NCWnd*>(GetDesktop(), 0x114)->HideWindow();
    if (GetDesktop()->InventoryWnd->IsShowWindow())
        GetDesktop()->InventoryWnd->HideWindow();
    GetDesktop()->ShopWnd->Clear();
    GetDesktop()->ShopWnd->SetMode(SHOPMODE_Sell);
    GetDesktop()->ShopWnd->SetListID(ListID);
    GetDesktop()->ShopWnd->SetAdena(Adena);
    GetDesktop()->ShopWnd->ShowWindow();
    GetDesktop()->ShopWnd->SetFocus();
}
NW_HOOK_METHOD(0x1005c740, NConsoleWnd, StartSellList)

// 0x1006ecf0 (vslot 66)
void NConsoleWnd::AddSellList(ItemInfo& Item)
{
    NItemInfo Info;
    if (MakeShopItem(Info, Item))
        GetDesktop()->ShopWnd->AddPlayerItem(&Info);
}
NW_HOOK_METHOD(0x1006ecf0, NConsoleWnd, AddSellList)

// 0x1005c300 (vslot 67) - the manor's shop window (the desktop's +0x118) in its buy mode: the same as StartBuyList
// on another window (C3: the window takes the focus)
void NConsoleWnd::StartManorBuyList(INT Adena, INT ListID)
{
    GetDesktop()->NpcHtmlWnd->HideWindow();
    NCManorShopWnd* Manor = static_cast<NCManorShopWnd*>(GetDesktop()->NewWndE4);
    Manor->Clear();
    Manor->SetMode(0);
    Manor->SetListID(ListID);
    Manor->SetAdena(Adena);
    GetDesktop()->NewWndE4->ShowWindow();
    GetDesktop()->NewWndE4->SetFocus();
}
NW_HOOK_METHOD(0x1005c300, NConsoleWnd, StartManorBuyList)

// 0x10065290 (vslot 66) - C2 only: the seeds the manor buys (AddBuyList's twin: the window's AddShopItem is
// 0x1006e130)
void NConsoleWnd::AddManorBuyList(ItemInfo& Item)
{
    NItemInfo Info;
    if (MakeShopItem(Info, Item))
        static_cast<NCManorShopWnd*>(GetDesktop()->NewWndE4)->AddShopItem(&Info);
}
NW_HOOK_METHOD(0x1006e130, NConsoleWnd, AddManorBuyList)

// 0x1005c390 (vslot 69) - the sell mode (instead of the inventory), as StartSellList (C3: the window takes the focus)
void NConsoleWnd::StartManorSellList(INT Adena, INT ListID)
{
    GetDesktop()->NpcHtmlWnd->HideWindow();
    if (GetDesktop()->InventoryWnd->IsShowWindow())
        GetDesktop()->InventoryWnd->HideWindow();
    NCManorShopWnd* Manor = static_cast<NCManorShopWnd*>(GetDesktop()->NewWndE4);
    Manor->Clear();
    Manor->SetMode(1);
    Manor->SetListID(ListID);
    Manor->SetAdena(Adena);
    GetDesktop()->NewWndE4->ShowWindow();
    GetDesktop()->NewWndE4->SetFocus();
}
NW_HOOK_METHOD(0x1005c390, NConsoleWnd, StartManorSellList)

// 0x1006e420 (vslot 70) - C2 only: what the player can sell the manor (AddSellList's twin: 0x100932e0)
void NConsoleWnd::AddManorSellList(ItemInfo& Item)
{
    NItemInfo Info;
    if (MakeShopItem(Info, Item))
        static_cast<NCManorShopWnd*>(GetDesktop()->NewWndE4)->AddPlayerItem(&Info);
}
NW_HOOK_METHOD(0x1006e420, NConsoleWnd, AddManorSellList)

// 0x1005c830 - the warehouse in its deposit mode, instead of the inventory (C3: the window takes the focus)
void NConsoleWnd::StartDepositList(INT Adena)
{
    GetDesktop()->NpcHtmlWnd->HideWindow();
    if (GetDesktop()->InventoryWnd->IsShowWindow())
        GetDesktop()->InventoryWnd->HideWindow();
    GetDesktop()->StoreWnd->Clear();
    GetDesktop()->StoreWnd->SetMode(STOREMODE_Deposit);
    GetDesktop()->StoreWnd->SetAdena(Adena);
    GetDesktop()->StoreWnd->ShowWindow();
    GetDesktop()->StoreWnd->SetFocus();
}
NW_HOOK_METHOD(0x1005c830, NConsoleWnd, StartDepositList)

// 0x1006efe0
void NConsoleWnd::AddDepositList(ItemInfo& Item)
{
    NItemInfo Info;
    if (MakeStoreItem(Info, Item))
        GetDesktop()->StoreWnd->AddInventoryItem(&Info);
}
NW_HOOK_METHOD(0x1006efe0, NConsoleWnd, AddDepositList)

// 0x1005c8e0 - the withdraw mode (C3: the window takes the focus)
void NConsoleWnd::StartWithdrawList(INT Adena)
{
    GetDesktop()->NpcHtmlWnd->HideWindow();
    GetDesktop()->StoreWnd->Clear();
    GetDesktop()->StoreWnd->SetMode(STOREMODE_Withdraw);
    GetDesktop()->StoreWnd->SetAdena(Adena);
    GetDesktop()->StoreWnd->ShowWindow();
    GetDesktop()->StoreWnd->SetFocus();
}
NW_HOOK_METHOD(0x1005c8e0, NConsoleWnd, StartWithdrawList)

// 0x1006f2d0
void NConsoleWnd::AddWithdrawList(ItemInfo& Item)
{
    NItemInfo Info;
    if (MakeStoreItem(Info, Item))
        GetDesktop()->StoreWnd->AddStoredItem(&Info);
}
NW_HOOK_METHOD(0x1006f2d0, NConsoleWnd, AddWithdrawList)

// 0x10075ff0 (vslot 75) - said only when it failed
void NConsoleWnd::ReceiveWareHouseDone(L2ParamStack& Params)
{
    if (!Params.Top())
        ShowSystemMessage(MSG_WarehouseFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10075ff0, NConsoleWnd, ReceiveWareHouseDone)

/*-----------------------------------------------------------------------------
    The freight.
-----------------------------------------------------------------------------*/

// 0x1005d8e0 - a character the freight can go to (its object id and name); the first one (index 0) opens the window
// (C3: and gives it the focus)
void NConsoleWnd::AddPackageToList(L2ParamStack& Params)
{
    if (!Params.Top())
    {
        GetDesktop()->SelectDeliverWnd->ShowWindow();
        GetDesktop()->SelectDeliverWnd->SetFocus();
    }
    INT ID = (INT)Params.Top();
    const TCHAR* Name = (const TCHAR*)Params.Top();
    GetDesktop()->SelectDeliverWnd->AddTarget(ID, Name);
}
NW_HOOK_METHOD(0x1005d8e0, NConsoleWnd, AddPackageToList)

// 0x10070c20 - the items of the freight window (the additional name from the game data, the typed etc values); Index 0
// opens the window (C3: with the focus) with the adena and the target
void NConsoleWnd::AddSendableItemList(INT Index, INT TargetID, INT Adena, ItemInfo& Item)
{
    if (!Index)
    {
        GetDesktop()->DeliverWnd->ShowWindow();
        GetDesktop()->DeliverWnd->SetFocus();
        GetDesktop()->DeliverWnd->SetAdena(Adena);
        GetDesktop()->DeliverWnd->SetTargetID(TargetID);
    }
    NItemInfo Info;
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID >= 0 && Data)
    {
        INT Class = Data->Class;
        appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
        appStrcpy(Info.AdditionalName, GL2GameData.GetItemAdditionalName(Item.ItemID));
        appStrcpy(Info.IconName, *Data->IconName);
        appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
        Info.Param = Item.Type;
        Info.ObjectID = Item.ObjectID;
        Info.ID = Item.ItemID;
        Info.BodyPart = Item.BodyPart;
        Info.Count = Item.Count;
        Info.Enchant = Item.Enchant;
        Info.UnknownC70 = Item.Unknown24;
        InfoC84(Info) = Item.Unknown1C;
        InfoEquipped(Info) = Item.bEquipped;
        InfoC90(Info) = Item.Unknown2C;
        SetClassValuesTyped(Info, Class, Data);
        GetDesktop()->DeliverWnd->AddInventoryItem(&Info);
    }
}
NW_HOOK_METHOD(0x10070c20, NConsoleWnd, AddSendableItemList)

/*-----------------------------------------------------------------------------
    The private stores: the window opens in its mode (1 one's own, 0 someone's) when not
    shown yet, then takes each item.
-----------------------------------------------------------------------------*/

// a store's items as the selling window shows them (C2: the additional name from the game data, the typed
// etc values)
static FORCEINLINE UBOOL MakeSellItem(NItemInfo& Info, ItemInfo& Item)
{
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID < 0 || !Data)
        return 0;
    INT Class = Data->Class;
    appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
    appStrcpy(Info.AdditionalName, GL2GameData.GetItemAdditionalName(Item.ItemID));
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
    Info.ObjectID = Item.ObjectID;
    Info.ID = Item.ItemID;
    Info.Count = Item.Count;
    Info.Enchant = Item.Enchant;
    InfoC98(Info) = Item.Unknown28;
    InfoC84(Info) = Item.Unknown1C;
    Info.UnknownC70 = Item.Unknown24;
    Info.Param = Item.Type;
    Info.BodyPart = Item.BodyPart;
    Info.UnknownC1C = Item.Unknown10;
    SetClassValuesTyped(Info, Class, Data);
    return 1;
}

// 0x10071fa0 - one's own selling store: an item of the inventory and one on sale (the texts and the typed etc values
// as the other lists). C3: the window takes the focus when it opens
void NConsoleWnd::PrivateStoreManageList(ItemInfo& InventoryItem, ItemInfo& StoreItem, INT OwnerID, INT Adena)
{
    if (!GetDesktop()->PrivateShopWnd->IsShowWindow())
    {
        GetDesktop()->PrivateShopWnd->Reset();
        GetDesktop()->PrivateShopWnd->SetMode(1);
        GetDesktop()->PrivateShopWnd->ShowWindow();
        GetDesktop()->PrivateShopWnd->SetFocus();
    }
    ((NCShopWnd*)GetDesktop()->PrivateShopWnd)->SetAdena(OwnerID);   // 0x100F0150: the owner id, at the place the shop window keeps its adena
    GetDesktop()->PrivateShopWnd->SetAdena(Adena);
    NItemInfo PlayerInfo;
    NItemInfo StoreInfo;
    NViewItemData* Data = NItemTable::Find(&InventoryItem.ItemID);
    if (InventoryItem.ItemID >= 0 && Data)
    {
        INT Class = Data->Class;
        appStrcpy(PlayerInfo.Name, GL2GameData.GetItemName(InventoryItem.ItemID));
        appStrcpy(PlayerInfo.AdditionalName, GL2GameData.GetItemAdditionalName(InventoryItem.ItemID));
        appStrcpy(PlayerInfo.IconName, *Data->IconName);
        appStrcpy(PlayerInfo.Description, GL2GameData.GetItemDescription(InventoryItem.ItemID));
        PlayerInfo.Count = InventoryItem.Count;
        PlayerInfo.ID = InventoryItem.ItemID;
        PlayerInfo.ObjectID = InventoryItem.ObjectID;
        PlayerInfo.UnknownC70 = InventoryItem.Unknown24;
        PlayerInfo.Enchant = InventoryItem.Enchant;
        InfoC84(PlayerInfo) = InventoryItem.Unknown1C;
        PlayerInfo.Param = InventoryItem.Type;
        InfoC98(PlayerInfo) = InventoryItem.Unknown28;
        PlayerInfo.BodyPart = InventoryItem.BodyPart;
        SetClassValuesTyped(PlayerInfo, Class, Data);
        GetDesktop()->PrivateShopWnd->AddPlayerItem(&PlayerInfo);
    }
    if (MakeSellItem(StoreInfo, StoreItem))
        GetDesktop()->PrivateShopWnd->AddStoreItem(&StoreInfo);
}
NW_HOOK_METHOD(0x10071fa0, NConsoleWnd, PrivateStoreManageList)

// 0x10072620 - someone's selling store: the window opens in its buy mode (C3: with the focus) when not shown yet,
// then takes the owner, the adena and each item
void NConsoleWnd::PrivateStoreList(ItemInfo& Item, INT OwnerID, INT Adena)
{
    if (!GetDesktop()->PrivateShopWnd->IsShowWindow())
    {
        GetDesktop()->PrivateShopWnd->Reset();
        GetDesktop()->PrivateShopWnd->SetMode(0);
        GetDesktop()->PrivateShopWnd->ShowWindow();
        GetDesktop()->PrivateShopWnd->SetFocus();
    }
    ((NCShopWnd*)GetDesktop()->PrivateShopWnd)->SetAdena(OwnerID);   // 0x100F0150: the owner id, at the place the shop window keeps its adena
    GetDesktop()->PrivateShopWnd->SetAdena(Adena);
    NItemInfo Info;
    if (MakeSellItem(Info, Item))
        GetDesktop()->PrivateShopWnd->AddStoreItem(&Info);
}
NW_HOOK_METHOD(0x10072620, NConsoleWnd, PrivateStoreList)

// 0x100729a0 - one's own buying store: an item of the inventory (by its item id) and one bought (the texts and the
// typed etc values as the other lists). C3: the window takes the focus when it opens
void NConsoleWnd::PrivateStoreBuyManageList(ItemInfo& InventoryItem, ItemInfo& StoreItem, INT OwnerID, INT Adena)
{
    if (!GetDesktop()->PrivateBuyWnd->IsShowWindow())
    {
        ((NCPrivateShopWnd*)GetDesktop()->PrivateBuyWnd)->Reset();   // 0x100D3E40: the selling window's (one body for both)
        GetDesktop()->PrivateBuyWnd->SetMode(1);
        GetDesktop()->PrivateBuyWnd->ShowWindow();
        GetDesktop()->PrivateBuyWnd->SetFocus();
    }
    ((NCShopWnd*)GetDesktop()->PrivateBuyWnd)->SetAdena(OwnerID);   // 0x100F0150: the owner id (as the selling window's)
    ((NCPrivateShopWnd*)GetDesktop()->PrivateBuyWnd)->SetAdena(Adena);   // 0x100D71C0: the selling window's (one body for both)
    NItemInfo PlayerInfo;
    NItemInfo StoreInfo;
    NViewItemData* Data = NItemTable::Find(&InventoryItem.ItemID);
    if (InventoryItem.ItemID >= 0 && Data)
    {
        INT Class = Data->Class;
        appStrcpy(PlayerInfo.Name, GL2GameData.GetItemName(InventoryItem.ItemID));
        appStrcpy(PlayerInfo.AdditionalName, GL2GameData.GetItemAdditionalName(InventoryItem.ItemID));
        appStrcpy(PlayerInfo.IconName, *Data->IconName);
        appStrcpy(PlayerInfo.Description, GL2GameData.GetItemDescription(InventoryItem.ItemID));
        PlayerInfo.ID = InventoryItem.ItemID;
        PlayerInfo.Count = InventoryItem.Count;
        PlayerInfo.Enchant = InventoryItem.Enchant;
        PlayerInfo.UnknownC70 = InventoryItem.Unknown24;
        PlayerInfo.Param = InventoryItem.Type;
        InfoC98(PlayerInfo) = InventoryItem.Unknown28;
        PlayerInfo.BodyPart = InventoryItem.BodyPart;
        SetClassValuesTyped(PlayerInfo, Class, Data);
        GetDesktop()->PrivateBuyWnd->AddPlayerItem(&PlayerInfo);
    }
    Data = NItemTable::Find(&StoreItem.ItemID);
    if (StoreItem.ItemID >= 0 && Data)
    {
        INT Class = Data->Class;
        appStrcpy(StoreInfo.Name, GL2GameData.GetItemName(StoreItem.ItemID));
        appStrcpy(StoreInfo.AdditionalName, GL2GameData.GetItemAdditionalName(StoreItem.ItemID));
        appStrcpy(StoreInfo.IconName, *Data->IconName);
        appStrcpy(StoreInfo.Description, GL2GameData.GetItemDescription(StoreItem.ItemID));
        StoreInfo.Count = StoreItem.Count;
        StoreInfo.ID = StoreItem.ItemID;
        StoreInfo.Enchant = StoreItem.Enchant;
        InfoC98(StoreInfo) = StoreItem.Unknown28;
        StoreInfo.UnknownC70 = StoreItem.Unknown24;
        StoreInfo.BodyPart = StoreItem.BodyPart;
        StoreInfo.Param = StoreItem.Type;
        StoreInfo.UnknownC1C = StoreItem.Unknown10;
        InfoC90(StoreInfo) = StoreItem.Unknown2C;
        SetClassValuesTyped(StoreInfo, Class, Data);
        GetDesktop()->PrivateBuyWnd->AddStoreItem(&StoreInfo);
    }
}
NW_HOOK_METHOD(0x100729a0, NConsoleWnd, PrivateStoreBuyManageList)

// 0x10072fc0 - someone's buying store (the same item values as the selling one, plus Unknown2C). C3: the window
// takes the focus when it opens
void NConsoleWnd::PrivateStoreBuyList(ItemInfo& Item, INT OwnerID, INT Adena)
{
    if (!GetDesktop()->PrivateBuyWnd->IsShowWindow())
    {
        ((NCPrivateShopWnd*)GetDesktop()->PrivateBuyWnd)->Reset();   // 0x100D3E40: the selling window's (one body for both)
        GetDesktop()->PrivateBuyWnd->SetMode(0);
        GetDesktop()->PrivateBuyWnd->ShowWindow();
        GetDesktop()->PrivateBuyWnd->SetFocus();
    }
    ((NCShopWnd*)GetDesktop()->PrivateBuyWnd)->SetAdena(OwnerID);   // 0x100F0150: the owner id (as the selling window's)
    ((NCPrivateShopWnd*)GetDesktop()->PrivateBuyWnd)->SetAdena(Adena);   // 0x100D71C0: the selling window's (one body for both)
    NItemInfo Info;
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID >= 0 && Data)
    {
        INT Class = Data->Class;
        appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
        appStrcpy(Info.AdditionalName, GL2GameData.GetItemAdditionalName(Item.ItemID));
        appStrcpy(Info.IconName, *Data->IconName);
        appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
        Info.ObjectID = Item.ObjectID;
        Info.ID = Item.ItemID;
        Info.Count = Item.Count;
        Info.Enchant = Item.Enchant;
        InfoC84(Info) = Item.Unknown1C;
        InfoC98(Info) = Item.Unknown28;
        Info.UnknownC70 = Item.Unknown24;
        Info.BodyPart = Item.BodyPart;
        Info.Param = Item.Type;
        Info.UnknownC1C = Item.Unknown10;
        InfoC90(Info) = Item.Unknown2C;
        SetClassValuesTyped(Info, Class, Data);
        GetDesktop()->PrivateBuyWnd->AddStoreItem(&Info);
    }
}
NW_HOOK_METHOD(0x10072fc0, NConsoleWnd, PrivateStoreBuyList)

// 0x1005e8e0 - a selling store's lists follow (PrivateStoreManageList): the window is emptied
// and hidden until then
void NConsoleWnd::StartPrivateStoreManageList()
{
    GetDesktop()->PrivateShopWnd->Reset();
    GetDesktop()->PrivateShopWnd->HideWindow();
}
NW_HOOK_METHOD(0x1005e8e0, NConsoleWnd, StartPrivateStoreManageList)

// the stock body is StartPrivateStoreManageList's (folded)
void NConsoleWnd::StartPrivateStoreList()
{
    GetDesktop()->PrivateShopWnd->Reset();
    GetDesktop()->PrivateShopWnd->HideWindow();
}

// 0x1005e970 - the same for a buying store (PrivateStoreBuyManageList)
void NConsoleWnd::StartPrivateStoreBuyManageList()
{
    GetDesktop()->PrivateBuyWnd->Reset();
    GetDesktop()->PrivateBuyWnd->HideWindow();
}
NW_HOOK_METHOD(0x1005e970, NConsoleWnd, StartPrivateStoreBuyManageList)

// the stock body is StartPrivateStoreBuyManageList's (folded)
void NConsoleWnd::StartPrivateStoreBuyList()
{
    GetDesktop()->PrivateBuyWnd->Reset();
    GetDesktop()->PrivateBuyWnd->HideWindow();
}

/*-----------------------------------------------------------------------------
    The VIP shop.
-----------------------------------------------------------------------------*/

// 0x1005e9a0 - its list follows (AddVIPItemList, AddVIPNeedItemList)
void NConsoleWnd::SetVIPShopID(INT ShopID)
{
    GetDesktop()->VIPShopWnd->Open(ShopID);
}
NW_HOOK_METHOD(0x1005e9a0, NConsoleWnd, SetVIPShopID)

// 0x10073350 - a product: the item and its slot line (a weapon's type before it); the item table's entry is not
// tested. C3: one more value (the sixth argument) goes into the entry at +0x518, which also grew by +0x51C (0)
void NConsoleWnd::AddVIPItemList(INT ProductID, INT ItemID, INT BodyPart, INT Type, INT Count, INT Param6)
{
    NVIPShopEntry Entry;
    NViewItemData* Data = NItemTable::Find(&ItemID);
    appStrcpy(Entry.Name, GL2GameData.GetItemName(ItemID));
    Entry.ItemID = ItemID;
    Entry.ProductID = ProductID;
    Entry.BodyPart = BodyPart;
    Entry.Type = Type;
    Entry.Count = Count;
    nw::Field<INT>(&Entry, 0x518) = Param6;     // C3: the entry has 8 bytes more (db/struct_sizes.csv NVIPShopEntry)
    nw::Field<INT>(&Entry, 0x51C) = 0;
    appStrcpy(Entry.IconName, *Data->IconName);
    FString SlotText;
    if (Data->Class == ITEMCLASS_Armor || Type == 1)
    {
        GetItemSlotText(Type, BodyPart, SlotText, Data->ArmorType);
        appStrcpy(Entry.Desc, *SlotText);
    }
    else if (Data->Class == ITEMCLASS_Weapon)
    {
        GetItemSlotText(Type, BodyPart, SlotText, -1);
        appSprintf(Entry.Desc, TEXT("%s / %s"), GetWeaponTypeName(Data->WeaponType), *SlotText);
    }
    else
    {
        GetItemSlotText(Type, BodyPart, SlotText, -1);
        appStrcpy(Entry.Desc, *SlotText);
    }
    GetDesktop()->VIPShopWnd->AddRecord(&Entry);
}
NW_HOOK_METHOD(0x10073350, NConsoleWnd, AddVIPItemList)

// 0x10073550 - an item a product costs (NVIPNeededItem is 0x318 bytes: C3 passes one more value, stored at +0x30C,
// and the entry's +0x310 is cleared)
void NConsoleWnd::AddVIPNeedItemList(INT ProductID, INT ItemID, INT Count, INT Param4)
{
    NVIPNeededItem Needed;
    nw::Field<INT>(&Needed, 0x30C) = 0;
    nw::Field<INT>(&Needed, 0x310) = 0;
    NViewItemData* Data = GL2GameData.ItemTable->FindRef(ItemID);
    appStrcpy(Needed.Name, GL2GameData.GetItemName(ItemID));
    Needed.ItemID = ItemID;
    appStrcpy(Needed.IconName, *Data->IconName);
    Needed.Count = Count;
    nw::Field<INT>(&Needed, 0x30C) = Param4;
    GetDesktop()->VIPShopWnd->AddNeededItem(ProductID, &Needed);
}
NW_HOOK_METHOD(0x10073550, NConsoleWnd, AddVIPNeedItemList)

// 0x1005e9c0 (vslot 211) - C3: the window is shown and focused, then its list is built (ShowList: a tail call)
void NConsoleWnd::VIPItemListEnd()
{
    GetDesktop()->VIPShopWnd->ShowWindow();
    GetDesktop()->VIPShopWnd->SetFocus();
    GetDesktop()->VIPShopWnd->ShowList();
}
NW_HOOK_METHOD(0x1005e9c0, NConsoleWnd, VIPItemListEnd)

/*-----------------------------------------------------------------------------
    The pet's inventory, the quest items.
-----------------------------------------------------------------------------*/

// 0x10071370 - (no handler) a pet's item. C2: the additional name comes from the game data
void NConsoleWnd::AddPetInventoryItem(ItemInfo& Item)
{
    NCPetWnd* PetWnd = GetDesktop()->PetWnd;
    if (PetWnd && PetWnd->InventoryWnd)          // C3: the pet inventory window is at +0x1A4 (C2: +0x170)
    {
        NItemInfo Info;
        NViewItemData* Data = NItemTable::Find(&Item.ItemID);
        if (Item.ItemID >= 0 && Data)
        {
            INT Class = Data->Class;
            appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
            appStrcpy(Info.AdditionalName, GL2GameData.GetItemAdditionalName(Item.ItemID));
            appStrcpy(Info.IconName, *Data->IconName);
            appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
            Info.Param = Item.Type;
            Info.ID = Item.ItemID;
            Info.ObjectID = Item.ObjectID;
            Info.Count = Item.Count;
            Info.Enchant = Item.Enchant;
            InfoC84(Info) = Item.Unknown1C;
            Info.BodyPart = Item.BodyPart;
            InfoEquipped(Info) = Item.bEquipped;
            Info.UnknownC70 = Item.Unknown24;
            if (Class == ITEMCLASS_Weapon)
                SetWeaponValues(Info, Data);
            else if (Class == ITEMCLASS_Armor)
                SetArmorValues(Info, Data);
            else if (Class == ITEMCLASS_Etc)
                SetTypedEtcValues(Info, Data);
            GetDesktop()->PetWnd->AddInventoryItem(&Info);
        }
    }
}
NW_HOOK_METHOD(0x10071370, NConsoleWnd, AddPetInventoryItem)

// 0x10071680 - as UpdateInventoryItem, for the pet's (C2: the same text and value changes as AddPetInventoryItem)
void NConsoleWnd::UpdatePetInventoryItem(INT Change, ItemInfo& Item)
{
    NCPetWnd* PetWnd = GetDesktop()->PetWnd;
    if (!PetWnd || !PetWnd->InventoryWnd)
        return;
    NItemInfo Info;
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID < 0 || !Data)
        return;
    INT Class = Data->Class;
    appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
    appStrcpy(Info.AdditionalName, GL2GameData.GetItemAdditionalName(Item.ItemID));
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
    Info.ObjectID = Item.ObjectID;
    Info.Param = Item.Type;
    Info.ID = Item.ItemID;
    Info.BodyPart = Item.BodyPart;
    Info.Enchant = Item.Enchant;
    Info.UnknownC70 = Item.Unknown24;
    Info.Count = Item.Count;
    InfoEquipped(Info) = Item.bEquipped;
    InfoC84(Info) = Item.Unknown1C;
    if (Class == ITEMCLASS_Weapon)
        SetUpdatedWeaponValues(Info, Data);
    else if (Class == ITEMCLASS_Armor)
        SetArmorValues(Info, Data);
    else if (Class == ITEMCLASS_Etc)
        SetTypedEtcValues(Info, Data);
    GetDesktop()->PetWnd->UpdateInventoryItem(Change, &Info);
}
NW_HOOK_METHOD(0x10071680, NConsoleWnd, UpdatePetInventoryItem)

// 0x1006f5c0 - an item a skill of the trainer needs: its type, item id, count and one more
// value (as the body part). C2: the additional name from the item record, the typed etc values
void NConsoleWnd::AddAcquireSkillExtendedInfo(L2ParamStack& Params)
{
    INT Type = (INT)Params.Top();
    INT ItemID = (INT)Params.Top();
    INT Count = (INT)Params.Top();
    INT BodyPart = (INT)Params.Top();
    NItemInfo Info;
    NViewItemData* Data = NItemTable::Find(&ItemID);
    if (ItemID < 0 || !Data)
        return;
    INT Class = Data->Class;
    appStrcpy(Info.Name, GL2GameData.GetItemName(ItemID));
    appStrcpy(Info.AdditionalName, *Data->AdditionalName);
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(ItemID));
    if (Class == ITEMCLASS_Weapon)
        SetWeaponValuesNoShots(Info, Data);
    else if (Class == ITEMCLASS_Armor)
        SetArmorValuesNoMP(Info, Data);
    else if (Class == ITEMCLASS_Etc)
        SetTypedEtcValues(Info, Data);
    Info.Param = Type;
    Info.ID = ItemID;
    Info.Count = Count;
    Info.BodyPart = BodyPart;
    GetDesktop()->TrainWnd->AddRequiredItem(&Info);
}
NW_HOOK_METHOD(0x1006f5c0, NConsoleWnd, AddAcquireSkillExtendedInfo)

// the quest items (type 3), with the etc item's values
static FORCEINLINE void MakeQuestItem(NItemInfo& Info, ItemInfo& Item, NViewItemData* Data)
{
    SetItemTexts(Info, Item.ItemID, Data);
    Info.ObjectID = Item.ObjectID;
    Info.ID = Item.ItemID;
    Info.Count = Item.Count;
    Info.BodyPart = Item.BodyPart;
    Info.Param = 3;
    SetEtcValues(Info, Data);
}

// 0x1012f080 (vslot 173, ret 4) - empty in C3: the body is a bare `ret 4` that the linker folded into every one-argument virtual
// that does nothing (ReceiveFriendAdd / Remove / List / Status, ReceiveBlockList, ReceiveMagicList, vfn271, vfn318 ... and the
// slot the db names NCCmdTarget::SetStyle), so the hook serves them all
void NConsoleWnd::AddQuestItem(ItemInfo& Item)
{
}
NW_HOOK_METHOD(0x1012f080, NConsoleWnd, AddQuestItem)

/*-----------------------------------------------------------------------------
    A GM's views of a character's items.
-----------------------------------------------------------------------------*/

// 0x1005e470
void NConsoleWnd::GMViewReceiveItemList(INT bShow)
{
    guard(NConsoleWnd::ReceiveItemList);
    GetDesktop()->GMInventoryWnd->ClearItems();
    if (bShow)
    {
        GetDesktop()->GMInventoryWnd->ShowWindow();
        GetDesktop()->GMInventoryWnd->SetFocus();
    }
    unguard;
}
NW_HOOK_METHOD(0x1005e470, NConsoleWnd, GMViewReceiveItemList)

// 0x100719a0 - as AddInventoryItem, for the GM's inventory window (C2: the additional name from the game
// data, no Unknown1788, the typed etc values)
void NConsoleWnd::GMViewAddInventoryItem(ItemInfo& Item)
{
    if (GetDesktop()->GMInventoryWnd)
    {
        NItemInfo Info;
        NViewItemData* Data = NItemTable::Find(&Item.ItemID);
        if (Item.ItemID >= 0 && Data)
        {
            INT Class = Data->Class;
            appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
            appStrcpy(Info.AdditionalName, GL2GameData.GetItemAdditionalName(Item.ItemID));
            appStrcpy(Info.IconName, *Data->IconName);
            appStrcpy(Info.Unknown200, *Data->IconName2);
            appStrcpy(Info.Unknown300, *Data->IconName3);
            appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
            Info.ObjectID = Item.ObjectID;
            Info.Param = Item.Type;
            Info.ID = Item.ItemID;
            Info.BodyPart = Item.BodyPart;
            Info.Enchant = Item.Enchant;
            Info.UnknownC70 = Item.Unknown24;
            Info.Count = Item.Count;
            InfoEquipped(Info) = Item.bEquipped;
            InfoC84(Info) = Item.Unknown1C;
            if (Class == ITEMCLASS_Weapon)
                SetWeaponValues(Info, Data);
            else if (Class == ITEMCLASS_Armor)
                SetArmorValues(Info, Data);
            else if (Class == ITEMCLASS_Etc)
                SetTypedEtcValues(Info, Data);
            GetDesktop()->GMInventoryWnd->AddItem(&Info);
        }
    }
}
NW_HOOK_METHOD(0x100719a0, NConsoleWnd, GMViewAddInventoryItem)

// 0x1005e520 - the GM's warehouse window, in the withdraw mode (C3: with the focus)
void NConsoleWnd::GMViewStartWithdrawList(INT Adena)
{
    GetDesktop()->GMStoreWnd->Clear();
    GetDesktop()->GMStoreWnd->SetMode(STOREMODE_Withdraw);
    GetDesktop()->GMStoreWnd->SetAdena(Adena);
    GetDesktop()->GMStoreWnd->ShowWindow();
    GetDesktop()->GMStoreWnd->SetFocus();
}
NW_HOOK_METHOD(0x1005e520, NConsoleWnd, GMViewStartWithdrawList)

// 0x10071cd0 - as AddWithdrawList, without the shots and the MP bonus (and the additional name from the game
// data)
void NConsoleWnd::GMViewAddWithdrawList(ItemInfo& Item)
{
    NItemInfo Info;
    NViewItemData* Data = NItemTable::Find(&Item.ItemID);
    if (Item.ItemID < 0 || !Data)
        return;
    INT Class = Data->Class;
    appStrcpy(Info.Name, GL2GameData.GetItemName(Item.ItemID));
    appStrcpy(Info.AdditionalName, GL2GameData.GetItemAdditionalName(Item.ItemID));
    appStrcpy(Info.IconName, *Data->IconName);
    appStrcpy(Info.Description, GL2GameData.GetItemDescription(Item.ItemID));
    Info.Param = Item.Type;
    Info.ID = Item.ItemID;
    Info.ObjectID = Item.ObjectID;
    Info.Count = Item.Count;
    Info.BodyPart = Item.BodyPart;
    Info.Enchant = Item.Enchant;
    Info.UnknownC70 = Item.Unknown24;
    InfoEquipped(Info) = Item.bEquipped;
    InfoC84(Info) = Item.Unknown1C;
    InfoC90(Info) = Item.Unknown2C;
    if (Class == ITEMCLASS_Weapon)
        SetWeaponValuesNoShots(Info, Data);
    else if (Class == ITEMCLASS_Armor)
        SetArmorValuesNoMP(Info, Data);
    else if (Class == ITEMCLASS_Etc)
        SetTypedEtcValues(Info, Data);
    GetDesktop()->GMStoreWnd->AddStoredItem(&Info);
}
NW_HOOK_METHOD(0x10071cd0, NConsoleWnd, GMViewAddWithdrawList)

// 0x1005e440 - C3: the GM's quest window takes the item's id and count (its unnamed method 0x10082840); the item
// table is not read
void NConsoleWnd::GMViewAddQuestItem(ItemInfo& Item)
{
    if (GetDesktop() && GetDesktop()->GMQuestWnd)
        ((NCGMQuestWndExt*)GetDesktop()->GMQuestWnd)->AddQuestItem(Item.ItemID, Item.Count);
}
NW_HOOK_METHOD(0x1005e440, NConsoleWnd, GMViewAddQuestItem)
