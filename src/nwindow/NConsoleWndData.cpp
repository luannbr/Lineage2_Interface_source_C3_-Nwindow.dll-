/*=============================================================================
    NConsoleWndData.cpp - the console's getters of game data: names and texts of the skills,
    actions, items, classes and castles, the tool tip lines, the enchant bonus. They read the
    engine's tables (GL2GameData) and never the console itself, except the equipment slots
    (the player) and the buffs (the desktop's abnormal status window).
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCSkillWnd.h"
#include "nwindow/NCSkillItemWnd.h"

// texture names of the crystal grades: "none", "graded", "gradec", "gradeb", "gradea", "grades"
#if NW_HOOK_MODE
#define GGradeNames (&NW_GLOBAL(const TCHAR*, 0x1020567C))
#else
extern const TCHAR* GGradeNames[];
#endif

// a line of questname-*.dat as this client lays it out: 0x70 bytes, 1000 of them at +0x3D18 of the game data (C2: 0x24
// at +0x4CF4, GQuestNames in NConsoleWnd.h); the text is the FString at +0x24 (NConsoleWndEvents.cpp has the same view)
struct NQuestLineC3
{
    INT bValid;               // +0x00 0: no quest here
    INT QuestID;              // +0x04
    INT Step;                 // +0x08
    FString Name;             // +0x0C the quest's name (GetQuestName; C2 +0x0C too)
    BYTE _Unknown18[0x24 - 0x18];
    FString Text;             // +0x24 the step's text (GetQuestText)
    BYTE _Unknown30[0x70 - 0x30];
};
NW_ASSERT_SIZE(NQuestLineC3, 0x70);
NW_ASSERT_OFFSET(NQuestLineC3, Name, 0x0C);
#define GQuestLinesC3 ((NQuestLineC3*)((BYTE*)&GL2GameData + 0x3D18))

// A level of a skill (FL2GameData::GetMSData) as this client lays it out: the engine header (include/ue2/UE2Engine.h,
// FL2MagicSkillData) still has C2's, where these fields sit elsewhere (C2 offsets in brackets). The getters below read
// it through this view; the offsets are the stock getters' (0x1005F630 .. 0x1005F7C0).
struct NSkillDataC3
{
    FString Name;                       // +0x00 (C2 +0x28)
    FString Description;                // +0x0C (C2 +0x34)
    BYTE _Unknown18[0x20 - 0x18];
    INT OperateType;                    // +0x20 2: passive, 3: toggle, else active (C2 +0x0C)
    INT MpCost;                         // +0x24 (C2 +0x14)
    INT Range;                          // +0x28 (C2 +0x18)
    BYTE _Unknown2C[0x38 - 0x2C];
    INT bMagic;                         // +0x38 shown as "Magic" instead (C2 +0x10)
    BYTE _Unknown3C[0x44 - 0x3C];
    L2FName IconName;                   // +0x44 (C2 +0x40)
};
NW_ASSERT_OFFSET(NSkillDataC3, OperateType, 0x20);
NW_ASSERT_OFFSET(NSkillDataC3, bMagic, 0x38);
NW_ASSERT_OFFSET(NSkillDataC3, IconName, 0x44);

// attack / defense added per enchant step, by crystal grade (none, D, C, B, A, S)
static const DOUBLE GWeaponEnchantBonus[6] = { 0.0, 2.0, 3.0, 3.0, 4.0, 5.0 };
static const DOUBLE GArmorEnchantBonus[6] = { 0.0, 1.0, 1.0, 1.0, 1.0, 1.0 };

/*-----------------------------------------------------------------------------
    Quests.
-----------------------------------------------------------------------------*/

// 0x1005cd20 - the quest's text at that step (NCQuestInfoWnd); the stock copied the guard of the
// virtual AddQuestID (slot 124)
const TCHAR* NConsoleWnd::GetQuestText(INT QuestID, INT Step)
{
    guard(NConsoleWnd::AddQuestID);
    for (INT i = 0; i < 1000; i++)
    {
        NQuestLineC3& Data = GQuestLinesC3[i];
        if (Data.bValid && Data.QuestID == QuestID && Data.Step == Step)
            return *Data.Text;
    }
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x1005cd20, NConsoleWnd, GetQuestText)

// 0x1005cc60 - the quest's name (NCMinimapWnd's list, the quest windows): the first line of questname-*.dat with that
// quest id, whatever its step; NULL when there is none (the stock loop reads 1000 lines of 0x70 bytes)
TCHAR* NConsoleWnd::GetQuestName(INT QuestID)
{
    guard(NConsoleWnd::GetQuestName);
    for (INT i = 0; i < 1000; i++)
    {
        NQuestLineC3& Data = GQuestLinesC3[i];
        if (Data.bValid && Data.QuestID == QuestID)
            return (TCHAR*)*Data.Name;
    }
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x1005cc60, NConsoleWnd, GetQuestName)

/*-----------------------------------------------------------------------------
    The zone the player is in: the engine's data, asked through the console.
-----------------------------------------------------------------------------*/

// 0x1005cdf0 (guard name)
TCHAR* NConsoleWnd::GetCurrentZoneName()
{
    guard(NConsoleWnd::GetCurrentZoneName);
    return GL2GameData.GetCurrentZoneName();
    unguard;
}
NW_HOOK_METHOD(0x1005cdf0, NConsoleWnd, GetCurrentZoneName)

// 0x1005ce70 (guard name)
INT NConsoleWnd::GetCurrentZoneID()
{
    guard(NConsoleWnd::GetCurrentZoneID);
    return GL2GameData.GetCurrentZoneID();
    unguard;
}
NW_HOOK_METHOD(0x1005ce70, NConsoleWnd, GetCurrentZoneID)

// 0x1005cef0 (guard name) - the zone at a world position (three floats, pushed as they came)
INT NConsoleWnd::GetZoneID(FLOAT X, FLOAT Y, FLOAT Z)
{
    guard(NConsoleWnd::GetZoneID);
    return GL2GameData.GetZoneID(X, Y, Z);
    unguard;
}
NW_HOOK_METHOD(0x1005cef0, NConsoleWnd, GetZoneID)

/*-----------------------------------------------------------------------------
    Skills and actions.
-----------------------------------------------------------------------------*/

// 0x1005f630
const TCHAR* NConsoleWnd::GetSkillIconName(INT SkillID, INT Level)
{
    NSkillDataC3* Data = (NSkillDataC3*)GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return NULL;
    return *Data->IconName;
}
NW_HOOK_METHOD(0x1005f630, NConsoleWnd, GetSkillIconName)

// 0x1005f660 - the name is the FString at the start of the record
TCHAR* NConsoleWnd::GetSkillName(INT SkillID, INT Level)
{
    NSkillDataC3* Data = (NSkillDataC3*)GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return NULL;
    return (TCHAR*)*Data->Name;
}
NW_HOOK_METHOD(0x1005f660, NConsoleWnd, GetSkillName)

// 0x1005f690
TCHAR* NConsoleWnd::GetSkillDesc(INT SkillID, INT Level)
{
    NSkillDataC3* Data = (NSkillDataC3*)GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return NULL;
    return (TCHAR*)*Data->Description;
}
NW_HOOK_METHOD(0x1005f690, NConsoleWnd, GetSkillDesc)

// 0x1005f6f0
UBOOL NConsoleWnd::IsToggleSkill(INT SkillID, INT Level)
{
    NSkillDataC3* Data = (NSkillDataC3*)GL2GameData.GetMSData(SkillID, Level);
    if (Data && Data->OperateType == 3)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x1005f6f0, NConsoleWnd, IsToggleSkill)

// 0x1005f720 - sysstring 313 "Magic", 312 "Passive Skill" or 311 "Active Skill"
const TCHAR* NConsoleWnd::GetSkillTypeText(INT SkillID, INT Level)
{
    NSkillDataC3* Data = (NSkillDataC3*)GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return NULL;
    if (Data->bMagic)
        return *GL2GameData.SysString[313];
    if (Data->OperateType == 2)
        return *GL2GameData.SysString[312];
    return *GL2GameData.SysString[311];
}
NW_HOOK_METHOD(0x1005f720, NConsoleWnd, GetSkillTypeText)

// 0x1005f790
INT NConsoleWnd::GetSkillMpCost(INT SkillID, INT Level)
{
    NSkillDataC3* Data = (NSkillDataC3*)GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return 0;
    return Data->MpCost;
}
NW_HOOK_METHOD(0x1005f790, NConsoleWnd, GetSkillMpCost)

// 0x1005f7c0
INT NConsoleWnd::GetSkillRange(INT SkillID, INT Level)
{
    NSkillDataC3* Data = (NSkillDataC3*)GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return 0;
    return Data->Range;
}
NW_HOOK_METHOD(0x1005f7c0, NConsoleWnd, GetSkillRange)

// 0x1006a3e0 - the icon of an action: the FString at +0x28 of its entry in the table (C2: heap entries in a
// TArray of pointers; the first dword says the entry is set; C1 had a fixed array at +0x4CD8)
const TCHAR* NConsoleWnd::GetActionIconName(INT ActionID)
{
    for (INT i = 0; i < GL2GameData.ActionTable.Num(); i++)
    {
        FL2ActionEntry* Entry = GL2GameData.ActionTable(i);
        if (Entry->bValid && Entry->ActionID == ActionID)
            return **(FString*)((BYTE*)Entry + 0x28);
    }
    return NULL;
}
NW_HOOK_METHOD(0x1006a3e0, NConsoleWnd, GetActionIconName)

// 0x1006a430 - the name of an action: the FString at +0x1C of its entry
const TCHAR* NConsoleWnd::GetActionName(INT ActionID)
{
    for (INT i = 0; i < GL2GameData.ActionTable.Num(); i++)
    {
        FL2ActionEntry* Entry = GL2GameData.ActionTable(i);
        if (Entry->bValid && Entry->ActionID == ActionID)
            return **(FString*)((BYTE*)Entry + 0x1C);
    }
    return NULL;
}
NW_HOOK_METHOD(0x1006a430, NConsoleWnd, GetActionName)

/*-----------------------------------------------------------------------------
    Items.
-----------------------------------------------------------------------------*/

// C1 only (2375, 3500 .. 3502): the C3 image keeps no such table; an inert body (no item is a collar)
UBOOL NConsoleWnd::IsPetCollar(INT ItemID)
{
    return 0;
}

// 0x1005f850 - sysstring 43..49, 504 by weapon type
const TCHAR* NConsoleWnd::GetWeaponTypeName(INT Type)
{
    switch (Type)
    {
    case 1: return *GL2GameData.SysString[43];
    case 2: return *GL2GameData.SysString[44];
    case 3: return *GL2GameData.SysString[45];
    case 4: return *GL2GameData.SysString[46];
    case 6: return *GL2GameData.SysString[48];
    case 5: return *GL2GameData.SysString[47];
    case 7: return *GL2GameData.SysString[49];
    case 8: return *GL2GameData.SysString[504];
    }
    return NULL;
}
NW_HOOK_METHOD(0x1005f850, NConsoleWnd, GetWeaponTypeName)

// 0x1005f940 - the slot line of an item's tool tip, by item type (0 weapon, 1 armor, 2 accessory, 3 and 4) and body
// part bits; the armors' SubType adds the material. Result is left alone for the combinations without a text. C3: an
// armor of body part 0x10000 has a line of its own (sysstring 1024)
void NConsoleWnd::GetItemSlotText(INT Type, INT BodyPart, FString& Result, INT SubType)
{
    TCHAR Separator[10];
    if (Type == 0)
    {
        if (BodyPart == 0x80 || BodyPart == 0x100)
            Result = *GL2GameData.SysString[242];
        else if (BodyPart == 0x4000)
            Result = *GL2GameData.SysString[243];
    }
    else if (Type == 1)
    {
        switch (BodyPart)
        {
        case 0x1:
            Result = *GL2GameData.SysString[229];
            return;
        case 0x40:
            Result = *GL2GameData.SysString[230];
            return;
        case 0x80:
        case 0x100:
            Result = *GL2GameData.SysString[231];
            return;
        case 0x200:
            Result = *GL2GameData.SysString[232];
            return;
        case 0x1000:
            Result = *GL2GameData.SysString[233];
            return;
        case 0x2000:
            Result = *GL2GameData.SysString[234];
            return;
        case 0x400:
            appStrcpy(Separator, TEXT(" / "));
            Result = *GL2GameData.SysString[235];
            break;
        case 0x800:
            appStrcpy(Separator, TEXT(" / "));
            Result = *GL2GameData.SysString[236];
            break;
        case 0x8000:
            appStrcpy(Separator, TEXT(" / "));
            Result = *GL2GameData.SysString[542];
            break;
        case 0x10000:
            Result = *GL2GameData.SysString[1024];
            return;
        default:
            return;
        }
        Result += Separator;
        switch (SubType)
        {
        case 1:
            Result += *GL2GameData.SysString[245];
            break;
        case 2:
            Result += *GL2GameData.SysString[246];
            break;
        case 3:
            Result += *GL2GameData.SysString[244];
            break;
        }
    }
    else if (Type == 2)
    {
        if ((BodyPart & 4) || (BodyPart & 2))
            Result = *GL2GameData.SysString[237];
        else if (BodyPart & 8)
            Result = *GL2GameData.SysString[238];
        else if ((BodyPart & 0x20) || (BodyPart & 0x10))
            Result = *GL2GameData.SysString[239];
    }
    else if (Type == 3)
        Result = *GL2GameData.SysString[240];
    else if (Type == 4)
        Result = *GL2GameData.SysString[241];
}
NW_HOOK_METHOD(0x1005f940, NConsoleWnd, GetItemSlotText)

// 0x1005fc30 - sysstring 310 (fastest) .. 306 by the attack speed (C2: the limits are 260, 309, 352
// and 406; C1 had 245, 280, 330 and 380)
const TCHAR* NConsoleWnd::GetAttackSpeedText(INT Speed)
{
    if (Speed < 260)
        return *GL2GameData.SysString[310];
    if (Speed < 309)
        return *GL2GameData.SysString[309];
    if (Speed < 352)
        return *GL2GameData.SysString[308];
    if (Speed < 406)
        return *GL2GameData.SysString[307];
    return *GL2GameData.SysString[306];
}
NW_HOOK_METHOD(0x1005fc30, NConsoleWnd, GetAttackSpeedText)

/*-----------------------------------------------------------------------------
    Castles and classes.
-----------------------------------------------------------------------------*/

// 0x1005fcb0
const TCHAR* NConsoleWnd::GetCastleName(INT CastleID)
{
    for (INT i = 0; i < 100; i++)
    {
        FL2CastleNameData& Data = GL2GameData.CastleNames[i];
        if (Data.bValid && Data.CastleID == CastleID)
            return *Data.Name;
    }
    return NULL;
}
NW_HOOK_METHOD(0x1005fcb0, NConsoleWnd, GetCastleName)

// 0x1005fd00 - the castle's second text (the location)
const TCHAR* NConsoleWnd::GetCastleLocation(INT CastleID)
{
    for (INT i = 0; i < 100; i++)
    {
        FL2CastleNameData& Data = GL2GameData.CastleNames[i];
        if (Data.bValid && Data.CastleID == CastleID)
            return *Data.Location;
    }
    return NULL;
}
NW_HOOK_METHOD(0x1005fd00, NConsoleWnd, GetCastleLocation)

// 0x1005fd50
const TCHAR* NConsoleWnd::GetCastleDescription(INT CastleID)
{
    for (INT i = 0; i < 100; i++)
    {
        FL2CastleNameData& Data = GL2GameData.CastleNames[i];
        if (Data.bValid && Data.CastleID == CastleID)
            return *Data.Description;
    }
    return NULL;
}
NW_HOOK_METHOD(0x1005fd50, NConsoleWnd, GetCastleDescription)

// 0x1005fec0
const TCHAR* NConsoleWnd::GetClassNameString(INT ClassID)
{
    return *GL2GameData.SysString[247 + ClassID];
}
NW_HOOK_METHOD(0x1005fec0, NConsoleWnd, GetClassNameString)

// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)
// 0x1005fee0 - no upper bound (grades are 0..5)
const TCHAR* NConsoleWnd::GetGradeText(INT Grade)
{
    if (Grade < 0)
        return NULL;
    return GGradeNames[Grade];
}
NW_HOOK_METHOD(0x1005fee0, NConsoleWnd, GetGradeText)

// 0x10060120 - enchant levels past +3 count twice on a weapon
INT NConsoleWnd::GetWeaponEnchantSteps(INT Enchant)
{
    if (Enchant > 3)
        return 2 * Enchant - 3;
    return Enchant;
}
NW_HOOK_METHOD(0x10060120, NConsoleWnd, GetWeaponEnchantSteps)

// 0x10060130 - and three times on an armor
INT NConsoleWnd::GetArmorEnchantSteps(INT Enchant)
{
    if (Enchant > 3)
        return 3 * Enchant - 6;
    return Enchant;
}
NW_HOOK_METHOD(0x10060130, NConsoleWnd, GetArmorEnchantSteps)

// C1 only (castle guards, the siege's NPCs): no function of this name in the C3 image; an inert body (none is)
BYTE NConsoleWnd::IsSiegeNpc(INT NpcID)
{
    return 0;
}

// 0x10062370 (name ours) - the soul / spirit shots (0x5B7..0x5BB, 0x72B, 0x9CD..0x9D2, 0xF6B..0xF70, 0x169D, 0x169E): the ones
// whose auto use the shortcut slot's right click turns on (NCShortCutWnd::OnRButtonDown, SetShortCut). The stock tests
// the ranges from the top down; the result is a plain 0 / 1
UBOOL NConsoleWnd::IsShotItem(INT ItemID)
{
    if (ItemID > 0x9D2)
    {
        if (ItemID > 0x169E)
            return 0;
        if (ItemID >= 0x169D)
            return 1;
        if (ItemID < 0xF6B)
            return 0;
        if (ItemID > 0xF70)
            return 0;
        return 1;
    }
    if (ItemID >= 0x9CD)
        return 1;
    if (ItemID < 0x5B7)
        return 0;
    if (ItemID <= 0x5BB)
        return 1;
    if (ItemID == 0x72B)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10062370, NConsoleWnd, IsShotItem)

// 0x10060190 (name ours) - the items the shortcut slot's key repeats for every 200 ms (NCShortCutWnd::SetShortCut): the
// healing potions, the mana potions and the shots, by their ids: 0x2DE..0x2DF, 0x424..0x425, 0x55E..0x55F, 0x5B7..0x5BB,
// 0x603..0x604, 0x727..0x72B, 0x9CD..0x9D2, 0xF6B..0xF70, 0x1245..0x1248, 0x1472, 0x15D7..0x15D8, 0x169D..0x169E,
// 0x1793..0x1794 (the stock has these as nested range tests and, above 0x15D8, a byte table indexed by id - 0x169D of which
// only the entries 0, 1, 0xF6 and 0xF7 are set)
UBOOL NConsoleWnd::IsQuickUseItem(INT ItemID)
{
    if (ItemID > 0x9D2)
    {
        if (ItemID > 0x15D8)
            return ItemID == 0x169D || ItemID == 0x169E || ItemID == 0x1793 || ItemID == 0x1794;
        if (ItemID >= 0x15D7)
            return 1;
        if (ItemID > 0x1248)
            return ItemID == 0x1472;
        if (ItemID >= 0x1245)
            return 1;
        if (ItemID < 0xF6B)
            return 0;
        return ItemID <= 0xF70;
    }
    if (ItemID >= 0x9CD)
        return 1;
    if (ItemID > 0x5BB)
    {
        if (ItemID > 0x72B)
            return 0;
        if (ItemID >= 0x727)
            return 1;
        if (ItemID < 0x603)
            return 0;
        return ItemID <= 0x604;
    }
    if (ItemID >= 0x5B7)
        return 1;
    if (ItemID > 0x425)
    {
        if (ItemID < 0x55E)
            return 0;
        return ItemID <= 0x55F;
    }
    if (ItemID >= 0x424)
        return 1;
    if (ItemID < 0x2DE)
        return 0;
    return ItemID <= 0x2DF;
}
NW_HOOK_METHOD(0x10060190, NConsoleWnd, IsQuickUseItem)

// 0x10062360 (name ours) - the auto use of a shot on / off: NetHandler slot 275 with the parameter stack the caller
// built (the shot's id, on / off); the stock jumps there with the caller's arguments (ret 4)
void NConsoleWnd::RequestAutoShot(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAutoSoulShot(Params);
}
NW_HOOK_METHOD(0x10062360, NConsoleWnd, RequestAutoShot)

/*-----------------------------------------------------------------------------
    The cool time of a skill, for the shortcut slots that start with a skill already running: the skill window's list
    (the character window's skill page) keeps the numbers of every icon (C3: two doubles at +0x48 and +0x50 of the cool
    time icon, the size it grew by).
-----------------------------------------------------------------------------*/

// The windows of the skill page are other lanes' classes; these views carry the functions of theirs that this port
// reaches (the member's `this` is the window), hooked as members at the stock addresses (names ours; db/names.csv)
struct NCSkillItemWndExt
{
    __declspec(noinline) void GetCoolTime(INT SkillID, DOUBLE* Total, DOUBLE* Remaining);   // 0x100a1a20
};
struct NCSkillWndExt
{
    __declspec(noinline) void GetCoolTime(INT SkillID, DOUBLE* Total, DOUBLE* Remaining);   // 0x100a1ba0
};
struct NCMagicSkillWndExt
{
    __declspec(noinline) void GetCoolTime(INT SkillID, DOUBLE* Total, DOUBLE* Remaining);   // 0x100a1f80
};

// 0x100a1a20 (name ours) - NCSkillItemWnd: the cool time of the icon of the skill, (0, 0) when it has none
void NCSkillItemWndExt::GetCoolTime(INT SkillID, DOUBLE* Total, DOUBLE* Remaining)
{
    NCSkillItemWnd* This = (NCSkillItemWnd*)this;
    DWORD* Out1 = (DWORD*)Total;
    DWORD* Out2 = (DWORD*)Remaining;
    Out1[0] = 0;
    Out1[1] = 0;
    Out2[0] = 0;
    Out2[1] = 0;
    for (INT i = 0; i < This->Items.Num(); i++)
    {
        if (This->Items(i)->ID == SkillID)
        {
            DWORD* Icon = (DWORD*)This->CoolTimes(i);
            Out1[0] = Icon[0x48 / 4];
            Out1[1] = Icon[0x4C / 4];
            Out2[0] = Icon[0x50 / 4];
            Out2[1] = Icon[0x54 / 4];
            return;
        }
    }
}
NW_HOOK_METHOD(0x100A1A20, NCSkillItemWndExt, GetCoolTime)

// 0x100a1ba0 (name ours) - NCSkillWnd: its Active list's (+0x1D4), (0, 0) without the list
void NCSkillWndExt::GetCoolTime(INT SkillID, DOUBLE* Total, DOUBLE* Remaining)
{
    NCSkillWnd* This = (NCSkillWnd*)this;
    if (This->ActiveList)
    {
        ((NCSkillItemWndExt*)This->ActiveList)->GetCoolTime(SkillID, Total, Remaining);
        return;
    }
    DWORD* Out1 = (DWORD*)Total;
    DWORD* Out2 = (DWORD*)Remaining;
    Out1[0] = 0;
    Out1[1] = 0;
    Out2[0] = 0;
    Out2[1] = 0;
}
NW_HOOK_METHOD(0x100A1BA0, NCSkillWndExt, GetCoolTime)

// 0x100a1f80 (name ours) - NCMagicSkillWnd: its skill list's (+0xCC), (0, 0) without it
void NCMagicSkillWndExt::GetCoolTime(INT SkillID, DOUBLE* Total, DOUBLE* Remaining)
{
    NCMagicSkillWnd* This = (NCMagicSkillWnd*)this;
    if (This->SkillWnd)
    {
        ((NCSkillWndExt*)This->SkillWnd)->GetCoolTime(SkillID, Total, Remaining);
        return;
    }
    DWORD* Out1 = (DWORD*)Total;
    DWORD* Out2 = (DWORD*)Remaining;
    Out1[0] = 0;
    Out1[1] = 0;
    Out2[0] = 0;
    Out2[1] = 0;
}
NW_HOOK_METHOD(0x100A1F80, NCMagicSkillWndExt, GetCoolTime)

// 0x10060150 (name ours) - (SkillID, Total, Remaining) through the character window's skill page; zeros without the
// desktop, the window or the page. NCShortCutWnd::SetShortCut resumes a slot's cool time with it
void NConsoleWnd::GetSkillCoolTime(INT SkillID, DOUBLE* Total, DOUBLE* Remaining)
{
    NCWnd* Page = NULL;
    if (Desktop && GetDesktop()->MainWnd)
        Page = GetDesktop()->MainWnd->SkillWnd;
    if (Page)
    {
        ((NCMagicSkillWndExt*)Page)->GetCoolTime(SkillID, Total, Remaining);
        return;
    }
    DWORD* Out1 = (DWORD*)Total;
    DWORD* Out2 = (DWORD*)Remaining;
    Out1[0] = 0;
    Out1[1] = 0;
    Out2[0] = 0;
    Out2[1] = 0;
}
NW_HOOK_METHOD(0x10060150, NConsoleWnd, GetSkillCoolTime)

// The C2 weapons' version (GWeaponEnchantBonus): this client has two of its own, with five arguments (the weapon type
// and body part choose the table: sub_1006A5D0 and sub_1006A670, called by NToolTipInfo::Fill); never hooked. The
// stock function the map paired it with (0x1006A710) is the armors' one below. Hook build: a jump to 0x1006A710, which is
// hooked to GetEnchantedDefense; standalone: the symbol is an alias of GetEnchantedDefense (the item tool tips call it)
#if NW_HOOK_MODE
NW_NAKED INT NConsoleWnd::GetEnchantedAttack(INT Grade, INT Enchant, INT Base) NW_STOCK(0x1006a710)
#else
NW_SLOT_IS("?GetEnchantedAttack@NConsoleWnd@@QAEHHHH@Z", "?GetEnchantedDefense@NConsoleWnd@@QAEHHHH@Z")   // NConsoleWnd::GetEnchantedAttack = NConsoleWnd::GetEnchantedDefense (0x1006a710)
#endif

// 0x1006a710 - the armors' enchant bonus (the map called it GetEnchantedAttack): the armor steps times the grade's
// bonus (the table at 0x10183CA0: 0, 1, 1, 1, 1, 1) plus the base
INT NConsoleWnd::GetEnchantedDefense(INT Grade, INT Enchant, INT Base)
{
    return (INT)(GetArmorEnchantSteps(Enchant) * GArmorEnchantBonus[Grade] + Base);
}
NW_HOOK_METHOD(0x1006a710, NConsoleWnd, GetEnchantedDefense)

// 0x1006a7c0 - the paperdoll slot of an item by its body part bits; the two-slot ones (ears, fingers) by the object id
// the player wears in each. C3: the numbering is the paperdoll's of this client, a missing player record is skipped,
// and 0x10000 is the new slot 3
INT NConsoleWnd::GetEquipSlot(INT BodyPart, INT ObjectID)
{
    if (BodyPart & 6)
    {
        User* Me = GetMyUser();
        if (Me)
        {
            if (Me->Equipped[1] == ObjectID)
                return 8;
            if (Me->Equipped[2] == ObjectID)
                return 9;
        }
    }
    if (BodyPart & 0x30)
    {
        User* Me = GetMyUser();
        if (Me)
        {
            if (Me->Equipped[4] == ObjectID)
                return 13;
            if (Me->Equipped[5] == ObjectID)
                return 14;
        }
    }
    switch (BodyPart)
    {
    case 0x1: return 0;
    case 0x8: return 4;
    case 0x40: return 1;
    case 0x80: return 5;
    case 0x100: return 7;
    case 0x200: return 10;
    case 0x400: return 6;
    case 0x800: return 11;
    case 0x1000: return 12;
    case 0x2000: return 2;
    case 0x4000: return 16;
    case 0x8000: return 15;
    case 0x10000: return 3;
    }
    return -1;
}
NW_HOOK_METHOD(0x1006a7c0, NConsoleWnd, GetEquipSlot)

// 0x1006aa90 - the skill's effect is among the player's buffs. The status window keeps POINTERS to its records (the
// array at +0xC8, its count at +0xCC) and the skill id is at +0x1918 of a record
UBOOL NConsoleWnd::HasAbnormal(INT SkillID)
{
    NCAbnormalStatusWnd* Wnd = GetDesktop()->AbnormalStatusWnd;
    BYTE** Statuses = nw::Field<BYTE**>(Wnd, 0xC8);
    INT Count = nw::Field<INT>(Wnd, 0xCC);
    for (INT i = 0; i < Count; i++)
        if (SkillID == *(INT*)(Statuses[i] + 0x1918))
            return 1;
    return 0;
}
NW_HOOK_METHOD(0x1006aa90, NConsoleWnd, HasAbnormal)

// GetIconName: C2 only; the map gave it 0x1006AA90, which is HasAbnormal above. Not hooked, never called; an empty
// name
const TCHAR* NConsoleWnd::GetIconName(INT ItemID)
{
    return TEXT("");
}

// 0x100739c0
const TCHAR* NConsoleWnd::GetItemName(INT ItemID)
{
    guard(NConsoleWnd::GetItemName);
    NViewItemData* Data = NItemTable::Find(&ItemID);
    if (ItemID >= 0 && Data)
        return *Data->Name;
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x100739c0, NConsoleWnd, GetItemName)

// 0x10073a70 - armor type 3 (the MP robes)
UBOOL NConsoleWnd::IsMagicArmor(INT ItemID)
{
    NViewItemData* Data = NItemTable::Find(&ItemID);
    if (Data && Data->ArmorType == 3)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10073a70, NConsoleWnd, IsMagicArmor)

/*-----------------------------------------------------------------------------
    System messages.
-----------------------------------------------------------------------------*/

// 0x10078ea0 - a time as system message 713 (year, month, day, hour, minute). C2: the year is a number
// parameter as in C1, the other four are texts made with "%2d" (C1 pushed numbers)
void NConsoleWnd::FormatDateTime(INT Time, TCHAR* Out)
{
    INT Year, Month, Day, Hour, Min;
    TCHAR MonthText[4], DayText[4], HourText[4], MinText[4];
    appConvertIntToTime(Time, Year, Month, Day, Hour, Min);
    L2ParamStack Params(11);
    Params.PushBack((void*)713);
    Params.PushBack((void*)5);
    Params.PushBack((void*)1);
    Params.PushBack((void*)(INT_PTR)Year);
    appSprintf(MonthText, TEXT("%2d"), Month);
    Params.PushBack((void*)0);
    Params.PushBack(MonthText);
    appSprintf(DayText, TEXT("%2d"), Day);
    Params.PushBack((void*)0);
    Params.PushBack(DayText);
    appSprintf(HourText, TEXT("%2d"), Hour);
    Params.PushBack((void*)0);
    Params.PushBack(HourText);
    appSprintf(MinText, TEXT("%2d"), Min);
    Params.PushBack((void*)0);
    Params.PushBack(MinText);
    MakeSystemMessage(Params, Out);
}
NW_HOOK_METHOD(0x10078ea0, NConsoleWnd, FormatDateTime)
