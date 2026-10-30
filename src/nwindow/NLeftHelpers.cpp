/*=============================================================================
    NLeftHelpers.cpp - small anonymous stock functions that belong to no class file (wave 3 leftovers). Names ours.
=============================================================================*/
#include "nwindow/NWindow.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCMapCoverWnd.h"
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCQuestGaraTreeWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NGameData.h"
#include "nwindow/NConsoleFns.h"
#include "nwindow/NConsoleWndExt.h"   // NItemMapView (one definition)

// 0x1000bd60 (ret 8; the db names it NCMacroListWnd::OnLButtonDown) - the body the linker folded for the default of a
// dozen mouse and key slots of unrelated windows (OnLButtonUp, OnMouseMove ... "take it, nothing else"): 1. `this` is
// not read
static INT __fastcall DefaultHandled(void* This, void* Unused, WPARAM wParam, LPARAM lParam)
{
    return 1;
}
NW_HOOK_FUNC(0x1000bd60, DefaultHandled)

// 0x10001b10 (ret 4; the db names it NCCmdTarget::OnTimer) - the body the linker folded for the default of OnTimer, OnPaint and
// OnDrawToolTip (one argument, "not handled"): 0. `this` is not read
static INT __fastcall DefaultNotHandled(void* This, void* Unused, INT Argument)
{
    return 0;
}
NW_HOOK_FUNC(0x10001b10, DefaultNotHandled)

// 0x10003400 (ret 8; the db names it Shared::EmptyTwoArgs_ICF) - the empty body the linker folded for the empty virtuals that take
// two arguments (NConsoleWnd's slot 278, the stock vtable still lists it; NCJapaneseIME / NCChineseIME::FillCandidateBox call it):
// nothing, and `this` is not read
void __fastcall EmptyTwoArgs(void* This, void* Unused, INT A, INT B)   // not static: the standalone aliases slot 278 to it
{
}
NW_HOOK_FUNC(0x10003400, EmptyTwoArgs)
#if !NW_HOOK_MODE
// NConsoleWnd slot 278 (PrivateStoreMsg, declared without a body in the standalone): the stock slot holds this body
NW_SLOT_IS("?PrivateStoreMsg@NConsoleWnd@@UAEXPAGH@Z", "?EmptyTwoArgs@@YIXPAX0HH@Z")
#endif

// 0x10001790 (cdecl (Dest, Src, Count); nothing is returned: eax is whatever the copy left) - UE2's inline appMemcpy emitted out of
// line: the MMX / SSE copy of the headers the stock was built with (GIsSSE set: 64-byte blocks of movq, a movsb sled entered by a
// computed jump to align the destination, db/not_functions.csv 0x100017e0; clear: rep movsd), a forward copy of Count bytes
static void __cdecl NAppMemcpy(void* Dest, const void* Src, UINT Count)
{
    memcpy(Dest, Src, Count);
}
NW_HOOK_FUNC(0x10001790, NAppMemcpy)

// 0x1005a470 (HasNoFinalConsonant: the character comes in AX, nothing is popped) stays stock: regcheck refuses a hook of a
// function that reads its argument from a register at entry. All three stock callers are ported functions that carry the
// logic inline (NConsoleWnd.h's HasNoFinalConsonant), so the stock body is never reached (db/morto.csv).

// 0x1012b460 (ret 4; anonymous in the db) - TMapBase::FindRef of the item table (the stock windows' item lookups call it
// out of line, many of ours inline it): NItemMapView::FindByID, declared and defined once in NConsoleWndExt.h
NW_HOOK_METHOD(0x1012b460, NItemMapView, FindByID)

// 0x10004f20 (cdecl; anonymous in the db) - NCButton's operator new out of line (NCFrameCtrl::OnCreate makes its two
// buttons with it): the object of the class in the package given
static void* NewButtonObject(size_t Size, UObject* Outer, FName Name, DWORD SetFlags)
{
    return UObject::StaticAllocateObject(NCButton::StaticClass(), Outer, Name, SetFlags);
}
NW_HOOK_FUNC(0x10004f20, NewButtonObject)

/*-----------------------------------------------------------------------------
    The console's small functions nobody named (NConsoleWnd's methods: `this` is the console).
-----------------------------------------------------------------------------*/

// NConsoleLeftFns: nwindow/NConsoleFns.h

// 0x10060400 (ret 4; `this` is not read) - the group of a class by its id: 0 for the fighters, 1 the mystics ... 8 (the
// races' own: the NCPartyStatusWnd's class icons and the tool tip "<group> - <class>" use it); an id nobody lists is 0
INT NConsoleLeftFns::GetClassGroup(INT ClassID)
{
    if (ClassID == 2 || ClassID == 3 || ClassID == 8 || ClassID == 0x17 || ClassID == 0x24 || ClassID == 0x2E ||
        ClassID == 0x30 || ClassID == 0x37 || ClassID == 0x39)
        return 0;
    if (ClassID == 9 || ClassID == 0x18 || ClassID == 0x25)
        return 1;
    if (ClassID == 5 || ClassID == 6 || ClassID == 0x14 || ClassID == 0x21)
        return 2;
    if (ClassID == 0x15 || ClassID == 0x22)
        return 3;
    if (ClassID == 0xC || ClassID == 0xD || ClassID == 0x1B || ClassID == 0x28)
        return 4;
    if (ClassID == 0x10 || ClassID == 0x11 || ClassID == 0x1E || ClassID == 0x2B || ClassID == 0x34 || ClassID == 0x33)
        return 5;
    if (ClassID == 0xE || ClassID == 0x1C || ClassID == 0x29)
        return 6;
    if (ClassID == 0 || ClassID == 1 || ClassID == 4 || ClassID == 7 || ClassID == 0x12 || ClassID == 0x13 ||
        ClassID == 0x16 || ClassID == 0x1F || ClassID == 0x20 || ClassID == 0x23 || ClassID == 0x2C || ClassID == 0x2D ||
        ClassID == 0x2F || ClassID == 0x35 || ClassID == 0x36 || ClassID == 0x38)
        return 7;
    if (ClassID == 0x31 || ClassID == 0x32 || ClassID == 0x26 || ClassID == 0x27 || ClassID == 0x2A || ClassID == 0x19 ||
        ClassID == 0x1A || ClassID == 0x1D || ClassID == 0xA || ClassID == 0xB || ClassID == 0xF)
        return 8;
    return 0;
}
NW_HOOK_METHOD(0x10060400, NConsoleLeftFns, GetClassGroup)

// 0x10061f10 (ret 4; `this` is not read) - the name of a hunting area of the party matching (1 .. 13: system strings
// 1048 .. 1060); the other numbers have none (NULL: the callers show an empty place)
const TCHAR* NConsoleLeftFns::GetPartyRoomAreaName(INT Area)
{
    if (Area >= 1 && Area <= 13)
        return *GL2GameData.SysString[1047 + Area];
    return NULL;
}
NW_HOOK_METHOD(0x10061f10, NConsoleLeftFns, GetPartyRoomAreaName)

// 0x10062070 (ret 4; `this` is not read) - the name of a party loot rule (0 .. 4: system strings 487, 488, 798, 799,
// 800: finders keepers, random, random including spoil, by turn, by turn including spoil)
const TCHAR* NConsoleLeftFns::GetLootRuleName(INT Rule)
{
    switch (Rule)
    {
    case 0: return *GL2GameData.SysString[487];
    case 1: return *GL2GameData.SysString[488];
    case 2: return *GL2GameData.SysString[798];
    case 3: return *GL2GameData.SysString[799];
    case 4: return *GL2GameData.SysString[800];
    }
    return NULL;
}
NW_HOOK_METHOD(0x10062070, NConsoleLeftFns, GetLootRuleName)

// 0x10073b90 (ret 4) - whether the NPC's record has its +0xCC set (NCStatusWnd's target bars: the HP / MP bars show for
// a non-player target when it is); 0 without a record
BYTE NConsoleLeftFns::NpcShowsBars(INT NpcID)
{
    NViewNpcData* Npc = GL2GameData.NpcTable->FindRef(NpcID);
    if (Npc)
        return ::nw::Field<INT>(Npc, 0xCC) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10073b90, NConsoleLeftFns, NpcShowsBars)

// 0x100725f0 (ret 4) - what an etc item is for the amount box (NCGaraDialogBox::OnOK): the record's +0x84 (stackable: its
// kind 2 or 3), 0 for any other class of item or an unknown id
INT NConsoleLeftFns::GetEtcItemStackKind(INT ItemID)
{
    NViewItemData* Item = GL2GameData.ItemTable->FindRef(ItemID);
    if (Item && Item->Class == ITEMCLASS_Etc)
        return ::nw::Field<INT>(Item, 0x84);
    return 0;
}
NW_HOOK_METHOD(0x100725f0, NConsoleLeftFns, GetEtcItemStackKind)

// 0x1006b6c0 - whether the player's own record has one of the classes 0x35 .. 0x39 (the dwarves): the manufacture
// action and the item warnings are theirs. The stock reads the record's +0x200 without a check of its own
UBOOL NConsoleLeftFns::IsDwarf()
{
    User* Me = GetMyUser();
    if (Me)
    {
        INT Class = ::nw::Field<INT>(Me, 0x200);
        if (Class == 0x35 || Class == 0x36 || Class == 0x38 || Class == 0x37 || Class == 0x39)
            return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x1006b6c0, NConsoleLeftFns, IsDwarf)

// 0x100076c0 (ret 4) - the desktop's map window's map shows (1) or hides (0) the mark the console reported
void NConsoleLeftFns::ShowMapMark(INT bShow)
{
    NCConsole* Desk = (NCConsole*)Desktop;
    if (Desk)
    {
        NCMapCoverWnd* Cover = (NCMapCoverWnd*)Desk->MapCoverWnd;
        if (Cover && Cover->Map)
            Cover->Map->bShowMark = bShow;
    }
}
NW_HOOK_METHOD(0x100076c0, NConsoleLeftFns, ShowMapMark)

// the multiplier of an enchantment step of a weapon's attack by its type (1 .. 8), its body part (0x4000: both hands) and the
// crystal grade, from three tables of doubles by grade (the stock's: two for the P. Atk at 0x10183c10 / 0x10183c40 /
// 0x10183c70, three for the M. Atk at 0x10205908 / 0x10205938 / 0x10205968): the types 1 and 2 take the second table
// with both hands and the first with one, 3, 4 and 7 the first, 5 and 8 the second, 6 the third; another type none (0).
// The stock reads the grade's entry without a check (grades 0 .. 5: none, D, C, B, A, S). The hook build reads the stock
// tables, the standalone its copies of them (the values of the C3 image)
#if NW_HOOK_MODE
#define ENCHANT_TABLE(va, Ours) ((const DOUBLE*)::nw::Addr(va))
#else
static const DOUBLE GPAtkStep1[6] = {0.0, 2.0, 3.0, 3.0, 4.0, 5.0};    // 0x10183c10
static const DOUBLE GPAtkStep2[6] = {0.0, 2.0, 4.0, 4.0, 5.0, 6.0};    // 0x10183c40
static const DOUBLE GPAtkStep3[6] = {0.0, 4.0, 6.0, 6.0, 8.0, 10.0};   // 0x10183c70
static const DOUBLE GMAtkStep1[6] = {0.0, 2.0, 3.0, 3.0, 3.0, 4.0};    // 0x10205908
static const DOUBLE GMAtkStep2[6] = {0.0, 2.0, 3.0, 3.0, 3.0, 4.0};    // 0x10205938
static const DOUBLE GMAtkStep3[6] = {0.0, 2.0, 3.0, 3.0, 3.0, 4.0};    // 0x10205968
#define ENCHANT_TABLE(va, Ours) (Ours)
#endif
static FORCEINLINE DOUBLE EnchantStepFactor(INT WeaponType, INT BodyPart, INT CrystalGrade, const DOUBLE* Table1,
                                            const DOUBLE* Table2, const DOUBLE* Table3)
{
    DOUBLE Factor = 0.0;   // the stock loads it from 0x1017C7F8 (0.0)
    switch (WeaponType - 1)
    {
    case 0:
    case 1:
        if (BodyPart == 0x4000)
            Factor = Table2[CrystalGrade];
        else
            Factor = Table1[CrystalGrade];
        break;
    case 2:
    case 3:
    case 6:
        Factor = Table1[CrystalGrade];
        break;
    case 4:
    case 7:
        Factor = Table2[CrystalGrade];
        break;
    case 5:
        Factor = Table3[CrystalGrade];
        break;
    }
    return Factor;
}

// 0x1006a5d0 (ret 0x14; NToolTipInfo::Fill and the viewer's tip) - the P. Atk shown for an enchanted weapon: the base plus
// the steps (GetWeaponEnchantSteps: past +3 each one counts twice) times the factor
INT NConsoleLeftFns::GetEnchantedPAtk(INT WeaponType, INT BodyPart, INT CrystalGrade, INT Enchant, INT BaseAtk)
{
    DOUBLE Factor = EnchantStepFactor(WeaponType, BodyPart, CrystalGrade, ENCHANT_TABLE(0x10183C10, GPAtkStep1),
                                      ENCHANT_TABLE(0x10183C40, GPAtkStep2), ENCHANT_TABLE(0x10183C70, GPAtkStep3));
    return (INT)((DOUBLE)GetWeaponEnchantSteps(Enchant) * Factor + (DOUBLE)BaseAtk);
}
NW_HOOK_METHOD(0x1006a5d0, NConsoleLeftFns, GetEnchantedPAtk)

// 0x1006a670 (ret 0x14) - the M. Atk of the same, with its own three tables
INT NConsoleLeftFns::GetEnchantedMAtk(INT WeaponType, INT BodyPart, INT CrystalGrade, INT Enchant, INT BaseAtk)
{
    DOUBLE Factor = EnchantStepFactor(WeaponType, BodyPart, CrystalGrade, ENCHANT_TABLE(0x10205908, GMAtkStep1),
                                      ENCHANT_TABLE(0x10205938, GMAtkStep2), ENCHANT_TABLE(0x10205968, GMAtkStep3));
    return (INT)((DOUBLE)GetWeaponEnchantSteps(Enchant) * Factor + (DOUBLE)BaseAtk);
}
NW_HOOK_METHOD(0x1006a670, NConsoleLeftFns, GetEnchantedMAtk)

// 0x100ac600 - the quest the map's mark belongs to: the main window's quest tab's tracked quest. The stock has no value of its
// own without the tab (the desktop, the main window or the tab missing): it comes back with the register it was called with,
// the console
INT NConsoleLeftFns::GetTrackedQuestID()
{
    NCConsole* Desk = (NCConsole*)Desktop;
    if (Desk && Desk->MainWnd && Desk->MainWnd->QuestWnd)
        return ((NCQuestGaraTreeWnd*)Desk->MainWnd->QuestWnd)->GetTrackedQuestID();
    return (INT)this;
}
NW_HOOK_METHOD(0x100ac600, NConsoleLeftFns, GetTrackedQuestID)

// 0x1005d490 - the clan's crest goes: the network handler's slot 108 (RequestSetPledgeCrest) with no data
void NConsoleLeftFns::RemovePledgeCrest()
{
    ::nw::VirtualCall<void>(Engine->NetHandler, 108, 0, 0);
}
NW_HOOK_METHOD(0x1005d490, NConsoleLeftFns, RemovePledgeCrest)
