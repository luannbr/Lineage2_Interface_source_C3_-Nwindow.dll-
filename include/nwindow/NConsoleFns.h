#pragma once
// NConsoleFns.h - stand-ins for the owners of small stock functions nobody named (the names of the owners are not in the
// headers): the functions are members of them, so that they are hooked as methods (thiscall, `this` in ecx) and called as
// typed methods by the windows. Declared ONCE here (docs/notas/C3_Standalone.md section 2b); the members are defined
// where they were: NComboItemFns and NConsoleForwardFns in NTemplates.cpp, NConsoleLeftFns in NLeftHelpers.cpp.

#include "nwindow/NCComboBox.h"
#include "nwindow/NConsoleWnd.h"

// NCComboBox's items with a data word (NString::Color)
struct NComboItemFns : public NCComboBox
{
    INT GetItemData(INT Index);                                      // 0x100054d0 (ret 4)
    void AddItemData(const TCHAR* Text, INT Data);                   // 0x10005a00 (ret 8)
};

// the console's party room requests (tail jumps into the network handler's slots)
struct NConsoleForwardFns : public NConsoleWnd
{
    void RequestListPartyWating(L2ParamStack& Params);               // 0x10061eb0 (ret 4)
    void RequestManagePartyRoom(L2ParamStack& Params);               // 0x10061ec0 (ret 4)
    void RequestJoinPartyRoom(L2ParamStack& Params);                 // 0x10061ed0 (ret 4)
    void RequestOustFromPartyRoom(L2ParamStack& Params);             // 0x10061ee0 (ret 4)
    void RequestDismissPartyRoom(L2ParamStack& Params);              // 0x10061ef0 (ret 4)
    void RequestWithdrawPartyRoom(L2ParamStack& Params);             // 0x10061f00 (ret 4)
    void RequestHandOverPartyMaster(L2ParamStack& Params);           // 0x10061ea0 (ret 4)
};

// The console's small functions nobody named (NConsoleWnd's methods: `this` is the console)
struct NConsoleLeftFns : public NConsoleWnd
{
    INT GetClassGroup(INT ClassID);                                  // 0x10060400 (ret 4)
    const TCHAR* GetPartyRoomAreaName(INT Area);                     // 0x10061f10 (ret 4)
    const TCHAR* GetLootRuleName(INT Rule);                          // 0x10062070 (ret 4)
    BYTE NpcShowsBars(INT NpcID);                                    // 0x10073b90 (ret 4)
    INT GetEtcItemStackKind(INT ItemID);                             // 0x100725f0 (ret 4)
    UBOOL IsDwarf();                                                 // 0x1006b6c0
    void ShowMapMark(INT bShow);                                     // 0x100076c0 (ret 4)
    INT GetTrackedQuestID();                                         // 0x100ac600
    INT GetEnchantedPAtk(INT WeaponType, INT BodyPart, INT CrystalGrade, INT Enchant, INT BaseAtk);   // 0x1006a5d0 (ret 0x14)
    INT GetEnchantedMAtk(INT WeaponType, INT BodyPart, INT CrystalGrade, INT Enchant, INT BaseAtk);   // 0x1006a670 (ret 0x14)
    void RemovePledgeCrest();                                        // 0x1005d490
};
