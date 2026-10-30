/*=============================================================================
    NConsoleWndMisc.cpp - the small services of the console that the windows call: the player's
    pawn and place, the macros the macro window keeps (the list, a macro by its id), the tattoo
    slots by the class, the additional name of an item, the party window's names, a few tail calls
    into the window manager and the network handler. Every one of them was an anonymous function of
    the C2 image (names ours).
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCDeliverWnd.h"
#include "nwindow/NCVIPShopWnd.h"
#include "nwindow/NGameData.h"
#include "nwindow/NItemInfo.h"
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCMenuWnd.h"
#include "nwindow/NCRadarWnd.h"
#include "nwindow/NCSystemMenuWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCMacroListWnd.h"
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCMatchWnd.h"
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCLoadingWnd.h"
#include "nwindow/NCTrainWnd.h"
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCTree.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCZoneTitleWnd.h"
#include "nwindow/NCVirtualWndMain.h"
#include "nwindow/NGameGuard.h"
#include "nwindow/NMacroEntry.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGodWnd.h"
#include "nwindow/NCPartyRoomWnd.h"
#include "nwindow/NCBroadcastGMWnd.h"
#include "nwindow/NCBroadcastObserverWnd.h"
#include "nwindow/NCBroadcastObserverGMMsgWnd.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NCSSQMainBoard.h"
#include "nwindow/NCSSQStatusWnd.h"
#include "nwindow/NCSSQMainEventWnd.h"
#include "nwindow/NCSSQSealStatusListWnd.h"
#include "nwindow/NCSSQMainEventGaraTreeCtrl.h"
#include "nwindow/NCSSQSealStatusGaraTreeCtrl.h"
#include "nwindow/NCGaraTVItem.h"
#include "nwindow/NControl.h"
#include "nwindow/NCVideoOptionWnd.h"
#include "nwindow/L2Friend.h"
#include "nwindow/L2ChatWnd.h"
#include "nwindow/L2ChatBox.h"
#include "nwindow/L2TaskWnd.h"
#include "nwindow/L2FriendWnd.h"
#include "nwindow/TaskWnd.h"
#include "nwindow/FriendList.h"
#include "nwindow/MsnWnd.h"
#include "nwindow/Msn.h"
#include "nwindow/MsnUiStock.h"         // NL2Book, NChatTaskListBox, NChatL2TaskWnd (sa-msn-ui)
#include "nwindow/NConsoleWndExt.h"      // the console's stand-ins (sa-console-b; one definition each)
#include "nwindow/NMsnTree.h"
#include <intrin.h>

// the nProtect GameGuard library's answer to a good start (StartGameGuard): 0x755, read-only data of the stock image
// (name ours); the standalone build defines it in src/standalone/NwGlobals.cpp
#if NW_HOOK_MODE
#define GNPGameMonSuccess NW_GLOBAL(const INT, 0x101ABA98)
#else
extern const INT GNPGameMonSuccess;
#endif

/*-----------------------------------------------------------------------------
    The function holders of this file (views of other lanes' windows: `this` is the window; names ours), declared
    up front: the console's functions below call them by type (stage 5: no call by address is left)
-----------------------------------------------------------------------------*/

// The windows below are other lanes' classes (or have no header yet); these views carry the functions of theirs that this
// port reaches: the member's `this` is the window, hooked as a member at the stock address (names ours; db/names.csv)
struct NCLoadingWndExt
{
    __declspec(noinline) void SetTextureName(FString Name);       // 0x1008E4D0
};
struct NCPartyWndExt
{
    __declspec(noinline) void ClearMemberBuffs(INT MemberID);     // 0x10101C60
    __declspec(noinline) void AddMemberBuff(INT MemberID, INT SkillID);   // 0x10101C10
};

struct NIntPair
{
    INT A;
    INT B;
};

struct NPledgeInfoExt
{
    __declspec(noinline) NPledgeInfoExt* Clear();                             // 0x100475A0
};
struct NCDeliverWndExt
{
    __declspec(noinline) UBOOL AddInventoryItem(NItemInfo* Info);             // 0x1007AD80
};
struct NCDeliverItemWndExt
{
    __declspec(noinline) UBOOL AddItem(NItemInfo* Info);                      // 0x1007AA40
};
struct NCGodWndExt
{
    __declspec(noinline) INT GetFlag();                                       // 0x10085C50
};
struct NCVIPShopWndExt
{
    __declspec(noinline) void AddRecord(const NVIPShopEntry* Src);            // 0x100970E0
};
struct NCZoneTitleWndExt
{
    __declspec(noinline) void SetTexts(const TCHAR* Text1, const TCHAR* Text2, const TCHAR* Text3);   // 0x1010B320
};
struct NCQuestTreeWndExt
{
    __declspec(noinline) void MakeRoot();                                     // 0x100DB380
};
struct NCSSQEventTreeWndExt
{
    __declspec(noinline) void MakeRoot();                                     // 0x100EB150
};
struct NCSSQListTreeWndExt
{
    __declspec(noinline) void MakeRoot();                                     // 0x100ECC20
};

// (NL2FriendArg and NMsnShellWnd, the messenger window as the console sees it, are declared in NCConsole.h)

// the player's windows of the console that the macro functions go through
static FORCEINLINE NCMacroListWnd* MacroWnd(NConsoleWnd* Console)
{
    return static_cast<NCMacroListWnd*>(static_cast<NCConsole*>(Console->Desktop)->NewWndF8);
}

// 0x1006a290 - the player's pawn: the first viewport's controller's pawn. The stock reads through without a check
// (the macro executor asks it only once a viewport exists)
APawn* NConsoleWnd::GetMyPawn()
{
    return Engine->Client->Viewports(0)->Actor->Pawn;
}
NW_HOOK_METHOD(0x1006a290, NConsoleWnd, GetMyPawn)

// 0x10062940 - where the player's pawn stands: (0, 0, 0) without a controller or a pawn. (C2: anonymous; its C1 twin,
// GetMyLocation, was 0x10038540)
FVector NConsoleWnd::GetMyLocation()
{
    FVector Result(0.f, 0.f, 0.f);
    APlayerController* Controller = Engine->Client->Viewports(0)->Actor;
    if (Controller && Controller->Pawn)
        Result = Controller->Pawn->Location;
    return Result;
}
NW_HOOK_METHOD(0x10062940, NConsoleWnd, GetMyLocation)

/*-----------------------------------------------------------------------------
    The macros (the macro window's list: TArray<NMacroEntry*>, +0xC4 of the window).
-----------------------------------------------------------------------------*/

// 0x1005f830 - how many macros the macro window keeps (0 without the desktop or the window). The stock callers that
// kept ecx across a call to it (HookShortCutKey, GetMacro) are ours
INT NConsoleWnd::GetMacroCount()
{
    if (Desktop && MacroWnd(this))
        return MacroWnd(this)->Macros.Num();
    return 0;
}
NW_HOOK_METHOD(0x1005f830, NConsoleWnd, GetMacroCount)

// 0x1006a480 - the macro at Index; NULL out of range
NMacroEntry* NConsoleWnd::GetMacro(INT Index)
{
    if (GetMacroCount() > Index && Index >= 0)
        return MacroWnd(this)->Macros(Index);
    return NULL;
}
NW_HOOK_METHOD(0x1006a480, NConsoleWnd, GetMacro)

// 0x1006a4b0 - whether a macro of the list is running (HookShortCutKey: a key press stops it first)
UBOOL NConsoleWnd::IsAnyMacroRunning()
{
    INT i = 0;
    if (GetMacroCount() > 0)
    {
        do
        {
            if (GetMacro(i) && GetMacro(i)->bRunning)
                break;
            i++;
        } while (i < GetMacroCount());
    }
    return i < GetMacroCount();
}
NW_HOOK_METHOD(0x1006a4b0, NConsoleWnd, IsAnyMacroRunning)

// 0x1006a510 - the name of the macro with the server's id MacroID, NULL when the list has none
TCHAR* NConsoleWnd::GetMacroName(INT MacroID)
{
    if (GetMacroCount())
    {
        INT i = 0;
        if (GetMacroCount() > 0)
        {
            do
            {
                if (GetMacro(i) && GetMacro(i)->ID == MacroID)
                    return GetMacro(i)->Name;
                i++;
            } while (i < GetMacroCount());
        }
    }
    return NULL;
}
NW_HOOK_METHOD(0x1006a510, NConsoleWnd, GetMacroName)

// 0x1006a570 - the icon number of the macro with the id, -1 when the list has none
INT NConsoleWnd::GetMacroIconType(INT MacroID)
{
    if (GetMacroCount())
    {
        INT i = 0;
        if (GetMacroCount() > 0)
        {
            do
            {
                if (GetMacro(i) && GetMacro(i)->ID == MacroID)
                    return GetMacro(i)->IconType;
                i++;
            } while (i < GetMacroCount());
        }
    }
    return -1;
}
NW_HOOK_METHOD(0x1006a570, NConsoleWnd, GetMacroIconType)

// 0x1006b040 - a macro slot of the shortcut bar is used: the macro with that id starts running (its flag; the tick
// runs its lines)
void NConsoleWnd::StartMacro(INT MacroID)
{
    INT i = 0;
    if (GetMacroCount() > 0)
    {
        do
        {
            if (GetMacro(i) && GetMacro(i)->ID == MacroID)
            {
                GetMacro(i)->bRunning = 1;
                return;
            }
            i++;
        } while (i < GetMacroCount());
    }
}
NW_HOOK_METHOD(0x1006b040, NConsoleWnd, StartMacro)

// 0x1005f7f0 - a macro object (made by ReceiveMacroListInfo) goes into the macro window's list; 0 when there is no
// object or no window
UBOOL NConsoleWnd::AddMacro(NMacroEntry* Macro)
{
    if (Macro && Desktop && static_cast<NCConsole*>(Desktop)->NewWndF8)
    {
        MacroWnd(this)->AddMacro(Macro);
        return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x1005f7f0, NConsoleWnd, AddMacro)

/*-----------------------------------------------------------------------------
    The player's class.
-----------------------------------------------------------------------------*/

// 0x1006a740 - how many tattoo slots the player has: by the profession tier of its class (the 58 classes: the
// base class 1, the first profession 2, the second 3); -1 without a player or for an id past the table
INT NConsoleWnd::GetHennaSlotCount()
{
    User* Me = GetMyUser();
    // C3: the stock reads the class id at User +0x200 (C2: +0x54); include/ue2's User puts PKCount there and ClassID at
    // +0x54 (the coordinator's header)
    INT ClassID = Me ? nw::Field<INT>(Me, 0x200) : 0;
    if (Me && (UINT)ClassID <= 0x39)
    {
        switch (ClassID)
        {
        case 0: case 10: case 18: case 25: case 31: case 38: case 44: case 49: case 53:
            return 1;
        case 1: case 4: case 7: case 11: case 15: case 19: case 22: case 26: case 29: case 32:
        case 35: case 39: case 42: case 45: case 47: case 50: case 54: case 56:
            return 2;
        case 2: case 3: case 5: case 6: case 8: case 9: case 12: case 13: case 14: case 16: case 17:
        case 20: case 21: case 23: case 24: case 27: case 28: case 30: case 33: case 34: case 36:
        case 37: case 40: case 41: case 43: case 46: case 48: case 51: case 52: case 55: case 57:
            return 3;
        }
    }
    return -1;
}
NW_HOOK_METHOD(0x1006a740, NConsoleWnd, GetHennaSlotCount)

/*-----------------------------------------------------------------------------
    Items, the party and the window manager.
-----------------------------------------------------------------------------*/

// 0x10060900 - the item has an additional name (the data holds "NAME_None" when it has not)
UBOOL NConsoleWnd::HasItemAdditionalName(INT ItemID)
{
    TCHAR* Name = GL2GameData.GetItemAdditionalName(ItemID);
    if (Name && appStrcmp(Name, TEXT("NAME_None")))
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10060900, NConsoleWnd, HasItemAdditionalName)

// 0x10060a90 - the item's additional name. The stock compares it with "NAME_None" and drops the result: it returns the
// name either way
const TCHAR* NConsoleWnd::GetItemAdditionalName(INT ItemID)
{
    TCHAR* Name = GL2GameData.GetItemAdditionalName(ItemID);
    if (Name)
        appStrcmp(Name, TEXT("NAME_None"));
    return Name;
}
NW_HOOK_METHOD(0x10060a90, NConsoleWnd, GetItemAdditionalName)

// 0x1005b450 - the name of the Index-th member of the party window (the %party1.. words of a /command, ExpandToken);
// NULL out of range or without the window
TCHAR* NConsoleWnd::GetPartyMemberName(INT Index)
{
    if (Desktop)
    {
        NCPartyWnd* Party = static_cast<NCConsole*>(Desktop)->PartyWnd;
        if (Party && Party->Members)
        {
            if (Index < nw::Field<INT>(Party, 0xD4) && Index >= 0)   // C3: NCPartyWnd.h has NumMembers at 0xD8, the stock reads +0xD4
                return Party->Members[Index]->Name;
        }
    }
    return NULL;
}
NW_HOOK_METHOD(0x1005b450, NConsoleWnd, GetPartyMemberName)

// 0x1005f0b0 - the mouse position of the game window, from the window manager (a tail call into
// NCVirtualWndMain::GetMousePos, 0x1002e390; NCWnd::Snap asks it)
UBOOL NConsoleWnd::GetMousePos(INT* X, INT* Y)
{
    return WndMain->GetMousePos(X, Y);
}
NW_HOOK_METHOD(0x1005f0b0, NConsoleWnd, GetMousePos)

// 0x1005ea70 - the GM's snoop window was closed: NetHandler slot 251 (a tail call)
void NConsoleWnd::RequestGMSnoopEnd(INT SnoopID)
{
    Engine->NetHandler->GMSnoopEnd(SnoopID);
}
NW_HOOK_METHOD(0x1005ea70, NConsoleWnd, RequestGMSnoopEnd)

// 0x1005f070 - a line into the desktop's chat window, with sub type 5 and the given color (the /setcl help)
void NConsoleWnd::ShowChatText(const TCHAR* Text, DWORD Color)
{
    if (Desktop)
        static_cast<NCConsole*>(Desktop)->AddSystemText(Text, 5, Color);
}
NW_HOOK_METHOD(0x1005f070, NConsoleWnd, ShowChatText)

// 0x1005ec80 - the text of system message MsgID with one number as its parameter, written to Out instead of shown
// (the security card prompt of the login box): the number is added as a parameter of type 1, then the message is
// ended with the text asked for
void NConsoleWnd::GetNumberMessage(INT MsgID, INT Number, TCHAR* Out)
{
    L2ParamStack Params(10);
    Params.PushBack((void*)1);
    Params.PushBack((void*)(INT_PTR)Number);
    AddSystemMessageParam(Params);
    L2ParamStack Message(10);
    Message.PushBack((void*)(INT_PTR)MsgID);
    EndSystemMessageParam(Message, 1, (INT)(INT_PTR)Out);
}
NW_HOOK_METHOD(0x1005ec80, NConsoleWnd, GetNumberMessage)

// 0x1005aa10 (guard name) - the card key for the login (NetHandler slot 68, RequestSCCheck)
UBOOL NConsoleWnd::RequestAuthCardKeyLogin(INT CardID, INT Key)
{
    guard(NConsoleWnd::RequestAuthCardKeyLogin);
    return Engine->NetHandler->RequestSCCheck(CardID, Key) != 0;
    unguard;
}
NW_HOOK_METHOD(0x1005aa10, NConsoleWnd, RequestAuthCardKeyLogin)

// 0x1006ffc0 - the "/useshortcut" kind of command 0x50: the item of the shortcut slot, when it is a weapon or armor that
// the player does not wear yet, is put on (RequestUseItem). The slot Index counts the pages
void NConsoleWnd::UseShortCutEquip(INT Index)
{
    NShortCutItem* Item = static_cast<NCConsole*>(Desktop)->ShortCutWnd->ItemAt(Index);
    if (Item && Item->Type == SHORTCUT_Item)
    {
        NCEquipItemWnd* Equip = static_cast<NCConsole*>(Desktop)->InventoryWnd->EquipWnd;
        for (INT i = 0; i < Equip->Items.Num(); i++)
            if (Equip->Items(i) && Equip->Items(i)->ID == Item->ID)
                return;
        NViewItemData* Data = NItemTable::Find(&Item->ID);
        if (Data && (Data->Class == ITEMCLASS_Weapon || Data->Class == ITEMCLASS_Armor))
            RequestUseItem(Item->ObjectID);
    }
}
NW_HOOK_METHOD(0x1006ffc0, NConsoleWnd, UseShortCutEquip)

// 0x10053f90 - Cast<ALineagePlayerController>, out of line
ALineagePlayerController* CastToLineagePlayerController(UObject* Object)
{
    if (Object && Object->IsA(ALineagePlayerController::StaticClass()))
        return (ALineagePlayerController*)Object;
    return NULL;
}
NW_HOOK_FUNC(0x10053f90, CastToLineagePlayerController)

// 0x100574e0 (name ours; the stock's GIsL2NetLogin branch of NConsoleWnd::Initialize, which the C2 binary patch jumps
// over, so it does not run in the patched client) - the game's side of the nProtect GameGuard start-up. The library
// (NGameGuard.h: statically linked into the stock image, which hook mode calls in place; the standalone build has none) is
// asked to start with NPGameLibApi::InitNPGameMon (0x100DFF20); its answer 0x755 (the library's success code, kept at
// 0x10060620) or 0x73 hands the game window to SetHwndToGameMon (0x100DFF40); any other answer is shown in the warning
// dialog as "<text> (ErrorCode=<answer>)", the text a system message chosen by the answer (765 .. 771; 1077 for the
// answers not listed)
void NConsoleWnd::StartGameGuard(INT hWnd)
{
    TCHAR Message[512];
    INT Code = NGameGuard::Init();
    INT MsgID;
    if (Code == GNPGameMonSuccess)
    {
        NGameGuard::SetHwnd(hWnd);
        return;
    }
    switch (Code)
    {
    case 0x73:
        NGameGuard::SetHwnd(hWnd);
        return;
    case 0x6E: MsgID = 765; break;
    case 0x72: MsgID = 766; break;
    case 0x78: case 0x7C: case 0x96: case 0x99: case 0xAA:
        MsgID = 767;
        break;
    case 0x9B: MsgID = 768; break;
    case 0xC8: MsgID = 769; break;
    case 0x154: case 0x15E: case 0x17C:
        MsgID = 770;
        break;
    case 0x168: MsgID = 771; break;
    default: MsgID = 1077; break;
    }
    appSprintf(Message, TEXT("%s (ErrorCode=%d)"), *GL2GameData.SystemMsg[MsgID].Message, Code);
    ShowWarningDialog((INT)(INT_PTR)Message, 1);
}
NW_HOOK_METHOD(0x10060620, NConsoleWnd, StartGameGuard)

// 0x10029fa0 - the allocator's Free (slot 302, a virtual of the console that does nothing else; the stock linker folded the
// global operator delete into it, so NString and the other stock code call it as a function): GMalloc->Free(Ptr)
void NConsoleWnd::Free(void* Ptr)
{
    appFree(Ptr);
}
NW_HOOK_METHOD(0x10029fa0, NConsoleWnd, Free)

/*-----------------------------------------------------------------------------
    The virtuals only C3 has that forward to a window of the desktop (names ours; each takes the
    stock's arguments and returns what it returns). The functions called by address are the ones
    this port has no class for.
-----------------------------------------------------------------------------*/

// 0x1005D350 (C3 virtual 166) - the buff window's 0x10102BA0 (its extra icon removed)
void NConsoleWnd::C3Virtual166()
{
    ((NCAbnormalStatusWndExt*)GetDesktop()->AbnormalStatusWnd)->ClearExtraStatus();
}
NW_HOOK_METHOD(0x1005D350, NConsoleWnd, C3Virtual166)

// 0x1005D370 (C3 virtual 168) - the buff window's 0x10102B30 (its second list emptied)
void NConsoleWnd::C3Virtual168()
{
    ((NCAbnormalStatusWndExt*)GetDesktop()->AbnormalStatusWnd)->ClearStatus2();
}
NW_HOOK_METHOD(0x1005D370, NConsoleWnd, C3Virtual168)

// 0x1005B950 (C3 virtual 202) - the NCGodWnd's restart menu hidden
void NConsoleWnd::C3Virtual202()
{
    if (GodWnd)
        static_cast<NCGodWnd*>(GodWnd)->HideRestartMenu();   // 0x100858E0
}
NW_HOOK_METHOD(0x1005B950, NConsoleWnd, C3Virtual202)

// 0x1005B960 (C3 virtual 203) - the NCGodWnd's restart menu shown
void NConsoleWnd::C3Virtual203()
{
    if (GodWnd)
        static_cast<NCGodWnd*>(GodWnd)->ShowRestartMenu();   // 0x10085960
}
NW_HOOK_METHOD(0x1005B960, NConsoleWnd, C3Virtual203)

// 0x1005B980 (C3 virtual 206) - the loading screen shown
void NConsoleWnd::C3Virtual206()
{
    LoadingWnd->ShowWindow();
}
NW_HOOK_METHOD(0x1005B980, NConsoleWnd, C3Virtual206)

// 0x1005E590 (C3 virtual 270) - the desktop's ToggleMinimap
void NConsoleWnd::C3Virtual270(INT Mode)
{
    GetDesktop()->ToggleMinimap(Mode);
}
NW_HOOK_METHOD(0x1005E590, NConsoleWnd, C3Virtual270)

// 0x10061140 (C3 virtual 291) - the messenger window's 0x10139670 (its main panel's CloseAll)
void NConsoleWnd::C3Virtual291()
{
    ((NMsnShellWnd*)GetDesktop()->MessengerWnd)->PanelCall139370();
}
NW_HOOK_METHOD(0x10061140, NConsoleWnd, C3Virtual291)

// 0x10061160 (C3 virtual 293) - the messenger window's 0x10131530 (its main panel's ReloadFriends)
void NConsoleWnd::C3Virtual293()
{
    ((NMsnShellWnd*)GetDesktop()->MessengerWnd)->ReloadFriends();
}
NW_HOOK_METHOD(0x10061160, NConsoleWnd, C3Virtual293)

// 0x10060BC0 (C3 virtual 346) - the desktop hidden
void NConsoleWnd::C3Virtual346()
{
    if (Desktop)
        Desktop->HideWindow();
}
NW_HOOK_METHOD(0x10060BC0, NConsoleWnd, C3Virtual346)

// 0x10060BE0 (C3 virtual 347) - the desktop shown
void NConsoleWnd::C3Virtual347()
{
    if (Desktop)
        Desktop->ShowWindow();
}
NW_HOOK_METHOD(0x10060BE0, NConsoleWnd, C3Virtual347)

// 0x1006B6A0 (C3 virtual 349) - the petition window's +0xDC child enabled
void NConsoleWnd::C3Virtual349()
{
    nw::Field<NCWnd*>(PetitionWnd, 0xDC)->EnableWindow(1);
}
NW_HOOK_METHOD(0x1006B6A0, NConsoleWnd, C3Virtual349)

// 0x1005BC90 (C3 virtual 350) - the zone title window's 0x1010B320 with the three values
void NConsoleWnd::C3Virtual350(INT A, INT B, INT C)
{
    if (Desktop && GetDesktop()->ZoneTitleWnd)
        ((NCZoneTitleWndExt*)GetDesktop()->ZoneTitleWnd)->SetTexts((const TCHAR*)A, (const TCHAR*)B, (const TCHAR*)C);   // 0x1010B320
}
NW_HOOK_METHOD(0x1005BC90, NConsoleWnd, C3Virtual350)

// 0x10077C30 (C3 virtual 351) - a flag of the character window's quest page (+0xDC)
UBOOL NConsoleWnd::C3Virtual351()
{
    if (Desktop && GetDesktop()->MainWnd && GetDesktop()->MainWnd->QuestWnd)
        return nw::Field<BYTE>(GetDesktop()->MainWnd->QuestWnd, 0xDC) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10077C30, NConsoleWnd, C3Virtual351)

// 0x10077C60 (C3 virtual 352) - the NCGodWnd's 0x10085C50 as a flag
UBOOL NConsoleWnd::C3Virtual352()
{
    if (GodWnd)
        return ((NCGodWndExt*)GodWnd)->GetFlag() != 0;   // 0x10085C50
    return 0;
}
NW_HOOK_METHOD(0x10077C60, NConsoleWnd, C3Virtual352)

// 0x10077C80 (C3 virtual 353) - a flag of the character window's quest page (+0xDD)
UBOOL NConsoleWnd::C3Virtual353()
{
    if (Desktop && GetDesktop()->MainWnd && GetDesktop()->MainWnd->QuestWnd)
        return nw::Field<BYTE>(GetDesktop()->MainWnd->QuestWnd, 0xDD) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10077C80, NConsoleWnd, C3Virtual353)

// 0x10077CB0 (C3 virtual 354) - sets the quest page's flag at +0xDD
void NConsoleWnd::C3Virtual354(INT bValue)
{
    if (Desktop && GetDesktop()->MainWnd && GetDesktop()->MainWnd->QuestWnd)
        nw::Field<BYTE>(GetDesktop()->MainWnd->QuestWnd, 0xDD) = (bValue != 0);
}
NW_HOOK_METHOD(0x10077CB0, NConsoleWnd, C3Virtual354)

// 0x10077E00 (C3 virtual 357) - the quest page's 0x100DA3A0 with three values
void NConsoleWnd::C3Virtual357(INT A, INT B, INT C)
{
    if (Desktop && GetDesktop()->MainWnd && GetDesktop()->MainWnd->QuestWnd)
        ((NCQuestGaraTreeWndExt*)GetDesktop()->MainWnd->QuestWnd)->SetTargetPos(A, B, C);   // 0x100DA3A0
}
NW_HOOK_METHOD(0x10077E00, NConsoleWnd, C3Virtual357)

// 0x10061DA0 (C3 virtual 374) - the party matching window's 0x100A5B10
void NConsoleWnd::C3Virtual374(L2ParamStack& Params)
{
    GetDesktop()->MatchWnd->SetList(Params);   // 0x100A5B10
}
NW_HOOK_METHOD(0x10061DA0, NConsoleWnd, C3Virtual374)

// 0x10061DC0 (C3 virtual 375) - the party matching window shown, focused and refreshed (0x100A4DE0)
void NConsoleWnd::C3Virtual375()
{
    GetDesktop()->MatchWnd->ShowWindow();
    GetDesktop()->MatchWnd->SetFocus();
    GetDesktop()->MatchWnd->ClearList();   // 0x100A4DE0
}
NW_HOOK_METHOD(0x10061DC0, NConsoleWnd, C3Virtual375)

// 0x10061E00 (C3 virtual 376) - the party room window's 0x100A5450
void NConsoleWnd::C3Virtual376(L2ParamStack& Params)
{
    static_cast<NCPartyRoomWnd*>(GetDesktop()->PartyRoomWnd)->SetRoomInfo(Params);   // 0x100A5450
}
NW_HOOK_METHOD(0x10061E00, NConsoleWnd, C3Virtual376)

// 0x10061E20 (C3 virtual 377) - the party room window's 0x100A6EA0
void NConsoleWnd::C3Virtual377(L2ParamStack& Params)
{
    static_cast<NCPartyRoomWnd*>(GetDesktop()->PartyRoomWnd)->SetMembers(Params);   // 0x100A6EA0
}
NW_HOOK_METHOD(0x10061E20, NConsoleWnd, C3Virtual377)

// 0x10061E80 (C3 virtual 379) - the party room window's 0x100A75B0
void NConsoleWnd::C3Virtual379(L2ParamStack& Params)
{
    static_cast<NCPartyRoomWnd*>(GetDesktop()->PartyRoomWnd)->UpdateMember(Params);   // 0x100A75B0
}
NW_HOOK_METHOD(0x10061E80, NConsoleWnd, C3Virtual379)

// 0x10062300 (C3 virtual 382) - the broadcast GM window shown and given the value (0x1010DD50)
void NConsoleWnd::C3Virtual382(INT A)
{
    GetDesktop()->BroadcastGMWnd->ShowWindow();
    static_cast<NCBroadcastGMWnd*>(GetDesktop()->BroadcastGMWnd)->NotifyGameInfo((L2ParamStack*)A);   // 0x1010DD50 (the value is the stack's address)
}
NW_HOOK_METHOD(0x10062300, NConsoleWnd, C3Virtual382)

// 0x10062340 (C3 virtual 383) - the broadcast GM window's 0x1010E110
void NConsoleWnd::C3Virtual383(L2ParamStack& Params)
{
    static_cast<NCBroadcastGMWnd*>(GetDesktop()->BroadcastGMWnd)->NotifyTeamMember(&Params);   // 0x1010E110
}
NW_HOOK_METHOD(0x10062340, NConsoleWnd, C3Virtual383)

// 0x1005BA00 (C3 virtual 385) - the desktop and the window at +0x39E0 hidden
void NConsoleWnd::C3Virtual385()
{
    Desktop->HideWindow();
    nw::Field<NCWnd*>(this, 0x39E0)->HideWindow();
}
NW_HOOK_METHOD(0x1005BA00, NConsoleWnd, C3Virtual385)

// 0x1005BA20 (C3 virtual 386) - the window at +0x39E0 shown
void NConsoleWnd::C3Virtual386()
{
    nw::Field<NCWnd*>(this, 0x39E0)->ShowWindow();
}
NW_HOOK_METHOD(0x1005BA20, NConsoleWnd, C3Virtual386)

// 0x10077C20 (C3 virtual 389) - the loading picture's id
INT NConsoleWnd::C3Virtual389()
{
    return LoadingScreenID;
}
NW_HOOK_METHOD(0x10077C20, NConsoleWnd, C3Virtual389)

/*-----------------------------------------------------------------------------
    Wave 3: the C3-only virtuals and helpers that act on a window or the network handler (names ours unless a guard
    string gave them). The windows the console's own header has no class for are reached by their offsets.
-----------------------------------------------------------------------------*/

// 0x1005c600 (guard name) - the try-on shop's list sent to the server: NetHandler slot 96 (the stock returns its answer)
INT NConsoleWnd::RequestPreviewItem(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestPreviewItem);
    return Engine->NetHandler->RequestPreviewItem(Params);
    unguard;
}
NW_HOOK_METHOD(0x1005c600, NConsoleWnd, RequestPreviewItem)

// 0x10060b40 (guard name) - the answer of a confirmation dialog the server asked for: NetHandler slot 260
void NConsoleWnd::SendConfirmDlg(L2ParamStack& Params)
{
    guard(NConsoleWnd::SendConfirmDlg);
    Engine->NetHandler->ConfirmDlg(Params);
    unguard;
}
NW_HOOK_METHOD(0x10060b40, NConsoleWnd, SendConfirmDlg)

// 0x1005eeb0 (guard name; C3 virtual 192) - a builder command (a GM's "//" line) sent as a one-text parameter stack through
// the console's BuilderCommand (slot 193, called on the global console as the stock does). The text arrives by value
void NConsoleWnd::SendBypassBuildCmd(FString Command)
{
    guard(NConsoleWnd::SendBypassBuildCmd);
    L2ParamStack Params(10);
    Params.PushBack((void*)*Command);
    GConsole->BuilderCommand(Params);
    unguard;
}
typedef void (NConsoleWnd::*SigSendBypassBuildCmd)(FString);
// installed as the member itself (no forwarder): the by-value FString a forwarder would copy again is the stock's own
NW_HOOK_VARARGS(0x1005eeb0, NConsoleWnd, SendBypassBuildCmd, SigSendBypassBuildCmd)


// 0x1008e4d0 (name ours) - NCLoadingWnd: the picture by the name of its texture; an empty name clears it
void NCLoadingWndExt::SetTextureName(FString Name)
{
    NCLoadingWnd* This = (NCLoadingWnd*)this;
    TCHAR Buffer[256];
    if (Name == TEXT(""))
        This->LoadingTex = NULL;
    else
    {
        appStrcpy(Buffer, *Name);
        This->LoadingTex = This->FindTexture(Buffer);
    }
}
typedef void (NCLoadingWndExt::*SigSetTextureName)(FString);
NW_HOOK_VARARGS(0x1008E4D0, NCLoadingWndExt, SetTextureName, SigSetTextureName)

// 0x1005b990 (C3 virtual 207) - the loading screen's picture by texture name (the by-value text goes on to the window)
void NConsoleWnd::SetLoadingTextureName(FString Name)
{
    ((NCLoadingWndExt*)LoadingWnd)->SetTextureName(Name);
}
typedef void (NConsoleWnd::*SigSetLoadingTextureName)(FString);
NW_HOOK_VARARGS(0x1005b990, NConsoleWnd, SetLoadingTextureName, SigSetLoadingTextureName)

// 0x100db630 (name ours) - the quest page of the character window: the tree's first item of the quest the list has
// selected (+0x11C of the child at +0xCC) is marked, and so is its last child (flags at +0x20A8 / +0x20AC: the quest is
// tracked on the map); the raw offsets are the quest tree classes' (NCQuestGaraTreeWnd, not placed in a header yet)
void NCQuestGaraTreeWndExt::MarkSelectedQuest()
{
    BYTE* Ctrl = nw::Field<BYTE*>(this, 0xCC);
    BYTE* Tree = nw::Field<BYTE*>(nw::Field<BYTE*>(Ctrl, 0x114), 0x1D4);
    if (!Tree)
        return;
    for (INT i = 0; i < nw::Field<INT>(Tree, 0x208C); i++)
    {
        BYTE* Item = nw::Field<BYTE**>(Tree, 0x2088)[i];
        if (nw::Field<INT>(nw::Field<BYTE*>(this, 0xCC), 0x11C) == nw::Field<INT>(Item, 0x1A08))
        {
            nw::Field<INT>(Item, 0x20A8) = 1;
            nw::Field<INT>(Item, 0x20AC) = 1;
            BYTE* Last = nw::Field<BYTE**>(Item, 0x2088)[nw::Field<INT>(Item, 0x208C) - 1];
            nw::Field<INT>(Last, 0x20A8) = 1;
            nw::Field<INT>(Last, 0x20AC) = 1;
        }
    }
}
NW_HOOK_METHOD(0x100DB630, NCQuestGaraTreeWndExt, MarkSelectedQuest)

// 0x1005cb10 (guard name; C3 virtual 171) - the quest list is complete: the quest page marks the selected quest
void NConsoleWnd::EndQuestList()
{
    guard(NConsoleWnd::EndQuestList);
    ((NCQuestGaraTreeWndExt*)GetDesktop()->MainWnd->QuestWnd)->MarkSelectedQuest();
    unguard;
}
NW_HOOK_METHOD(0x1005cb10, NConsoleWnd, EndQuestList)

// 0x10101c60 (name ours) - NCPartyWnd: the buffs of the member with that object id are forgotten
void NCPartyWndExt::ClearMemberBuffs(INT MemberID)
{
    NCPartyWnd* This = (NCPartyWnd*)this;
    for (INT i = 0; i < This->NumMembers; i++)
    {
        if (This->Members[i]->MemberID == MemberID)
        {
            This->Members[i]->ClearBuffs();
            return;
        }
    }
}
NW_HOOK_METHOD(0x10101C60, NCPartyWndExt, ClearMemberBuffs)

// 0x10101c10 (name ours) - NCPartyWnd: a buff (a skill id) of the member with that object id
void NCPartyWndExt::AddMemberBuff(INT MemberID, INT SkillID)
{
    NCPartyWnd* This = (NCPartyWnd*)this;
    for (INT i = 0; i < This->NumMembers; i++)
    {
        if (This->Members[i]->MemberID == MemberID)
        {
            This->Members[i]->AddBuff(SkillID);
            return;
        }
    }
}
NW_HOOK_METHOD(0x10101C10, NCPartyWndExt, AddMemberBuff)

// 0x1005db00 (C3 virtual 114) - the buffs of one of the party's windows follow: the kind (0 a party member by its object id,
// 1 the pet, 2 the summoned creature), the member's id, the count and one skill id each. The member's buffs are cleared
// first. The pet's window is called with NCPartyStatusWnd's functions (0x10100CA0, 0x10100CE0: the same arrays at
// +0x338 / +0x344)
void NConsoleWnd::ReceiveBuffList(L2ParamStack& Params)
{
    INT Kind = (INT)Params.Top();
    INT MemberID = (INT)Params.Top();
    INT Count = (INT)Params.Top();
    if (Kind == 0)
    {
        ((NCPartyWndExt*)GetDesktop()->PartyWnd)->ClearMemberBuffs(MemberID);
        for (INT i = 0; i < Count; i++)
        {
            INT SkillID = (INT)Params.Top();
            ((NCPartyWndExt*)GetDesktop()->PartyWnd)->AddMemberBuff(MemberID, SkillID);
        }
    }
    else if (Kind == 1)
    {
        reinterpret_cast<NCPartyStatusWnd*>(GetDesktop()->PetStatusWnd)->ClearBuffs();   // NCPartyStatusWnd's 0x10100CA0
        for (INT i = 0; i < Count; i++)
        {
            INT SkillID = (INT)Params.Top();
            reinterpret_cast<NCPartyStatusWnd*>(GetDesktop()->PetStatusWnd)->AddBuff(SkillID);   // NCPartyStatusWnd's 0x10100CE0
        }
    }
    else if (Kind == 2)
    {
        GetDesktop()->SummonedStatusWnd->ClearBuffs();
        for (INT i = 0; i < Count; i++)
        {
            INT SkillID = (INT)Params.Top();
            GetDesktop()->SummonedStatusWnd->AddBuff(SkillID);
        }
    }
}
NW_HOOK_METHOD(0x1005db00, NConsoleWnd, ReceiveBuffList)

// 0x1005e7c0 (C3 virtual 272) - the map cover's minimap (+0xC8 of the window at desktop +0x14C) shows a place, while the
// cover is shown (the last argument, 0: no follow)
void NConsoleWnd::ShowMapPosition(INT X, INT Y, INT Z)
{
    NCWnd* Cover = Desktop ? GetDesktop()->MapCoverWnd : NULL;
    if (Cover && nw::Field<NCMinimapWnd*>(Cover, 0xC8) && Cover->IsShowWindow())
        nw::Field<NCMinimapWnd*>(GetDesktop()->MapCoverWnd, 0xC8)->ShowPosition(X, Y, Z, 0);
}
NW_HOOK_METHOD(0x1005e7c0, NConsoleWnd, ShowMapPosition)

// 0x100662f0 (C3 virtual 271) - the same, on the place of the player's pawn (the stock truncates each coordinate with
// _ftol); (0, 0, 0) without a controller or a pawn
void NConsoleWnd::ShowMyMapPosition()
{
    FVector Loc(0.f, 0.f, 0.f);
    APlayerController* Controller = Engine->Client->Viewports(0)->Actor;
    if (Controller && Controller->Pawn)
        Loc = Controller->Pawn->Location;
    INT X = (INT)Loc.X;
    INT Y = (INT)Loc.Y;
    NCWnd* Cover = Desktop ? GetDesktop()->MapCoverWnd : NULL;
    if (Cover && nw::Field<NCMinimapWnd*>(Cover, 0xC8) && Cover->IsShowWindow())
        nw::Field<NCMinimapWnd*>(GetDesktop()->MapCoverWnd, 0xC8)->ShowPosition(X, Y, (INT)Loc.Z, 0);
}
NW_HOOK_METHOD(0x100662f0, NConsoleWnd, ShowMyMapPosition)

/*-----------------------------------------------------------------------------
    Wave 3 (2): the match broadcast of the GM's and the observer's windows (the desktop's +0x1AC .. +0x1BC: all of their
    own functions stay stock and are called by address), the buff lists of the C3 buff window, the Seven Signs board, the
    shortcut bar's auto shots, the quest page's target, and the small forwarders of the C3-only slots. Names ours; the
    guard strings are the stock's (copied where they are another function's or have a typo).
-----------------------------------------------------------------------------*/

// NCBroadcastObserverWnd's one small function this port owns (the other functions of the class stay stock)
struct NCBroadcastObserverWndExt
{
    __declspec(noinline) void SetTeamState(INT TeamID, INT State, INT Unused);   // 0x10117D80
};

// 0x10117d80 (name ours) - the observer's window: when the team is the one it follows (+0xE0), State 1 clears its flag
// (+0xE4), 2 sets it (the third value is not read)
void NCBroadcastObserverWndExt::SetTeamState(INT TeamID, INT State, INT Unused)
{
    if (nw::Field<INT>(this, 0xE0) == TeamID)
    {
        BYTE Mode = (BYTE)State;
        if (Mode == 1)
            nw::Field<INT>(this, 0xE4) = 0;
        else if (Mode == 2)
            nw::Field<INT>(this, 0xE4) = 1;
    }
}
NW_HOOK_METHOD(0x10117D80, NCBroadcastObserverWndExt, SetTeamState)

// 0x100611e0 (C3 virtual 360; guard name) - a message of the match: kind 0 is the GM's text for the observer's message
// window, kinds 1..8 select the line of the special message window (the kind goes through the setter 0x10116B20 the
// private stores use too); the window is shown. The kind is a byte in a dword
void NConsoleWnd::OnEventMatchMessage(INT Kind, FString Message)
{
    guard(NConsoleWnd::OnEventMatchMessage);
    if (Desktop)
    {
        INT K = (signed char)Kind;
        if (K == 0)
        {
            NCWnd* Wnd = GetDesktop()->BroadcastObserverGMMsgWnd;
            if (Wnd)
            {
                static_cast<NCBroadcastObserverGMMsgWnd*>(Wnd)->SetMsg(Message);   // 0x10113760 (the copy is the callee's)
                Wnd->ShowWindow();
            }
        }
        else if (K > 0 && K <= 8)
        {
            NCWnd* Wnd = GetDesktop()->BroadcastObserverSpecialMsgWnd;
            if (Wnd)
            {
                reinterpret_cast<NCStoreWnd*>(Wnd)->SetAdena(K);   // the shared setter 0x10116B20 (NCStoreWnd::SetAdena)
                Wnd->ShowWindow();
            }
        }
    }
    unguard;
}
typedef void (NConsoleWnd::*SigOnEventMatchMessage)(INT, FString);
NW_HOOK_VARARGS(0x100611E0, NConsoleWnd, OnEventMatchMessage, SigOnEventMatchMessage)

// 0x100612f0 (C3 virtual 361; guard name) - the score to the observer's window (NCBroadcastObserverWnd::NotifyScore)
void NConsoleWnd::OnEventMatchScore(INT A, INT B, INT C)
{
    guard(NConsoleWnd::OnEventMatchScore);
    if (Desktop && GetDesktop()->BroadcastObserverWnd)
        static_cast<NCBroadcastObserverWnd*>(GetDesktop()->BroadcastObserverWnd)->NotifyScore(A, B, C);
    unguard;
}
NW_HOOK_METHOD(0x100612F0, NConsoleWnd, OnEventMatchScore)

// 0x10061390 (C3 virtual 362; the guard string is "NConsoelWnd::..." in the stock) - the teams' list begins: to the GM's
// window while it is shown (NotifyTeamInfoStart 0x1010D980) and to the observer's window when the console is in the
// observer's UI mode (UIMode 1)
void NConsoleWnd::OnEventMatchTeamInfoStart(INT A, INT B, INT C)
{
    guard(NConsoelWnd::OnEventMatchTeamInfoStart);
    if (Desktop)
    {
        NCWnd* GM = GetDesktop()->BroadcastGMWnd;
        if (GM && GM->IsShowWindow())
            static_cast<NCBroadcastGMWnd*>(GM)->NotifyTeamInfoStart(A, B, C);
        NCWnd* Observer = GetDesktop()->BroadcastObserverWnd;
        if (Observer && UIMode == 1)
            ((NCBroadcastObserverWndExt*)Observer)->SetTeamState(A, B, C);
    }
    unguard;
}
NW_HOOK_METHOD(0x10061390, NConsoleWnd, OnEventMatchTeamInfoStart)

// 0x10061470 (C3 virtual 363; guard name) - a team of the list (its two numbers, its name and ten more numbers), to the
// GM's window (0x1010DA90) and the observer's (0x10117DC0), each while it is shown
void NConsoleWnd::OnEventMatchTeamInfo(INT A, INT B, FString Name, INT P1, INT P2, INT P3, INT P4, INT P5, INT P6,
                                       INT P7, INT P8, INT P9, INT P10)
{
    guard(NConsoleWnd::OnEventMatchTeamInfo);
    if (Desktop)
    {
        NCWnd* GM = GetDesktop()->BroadcastGMWnd;
        if (GM && GM->IsShowWindow())
            static_cast<NCBroadcastGMWnd*>(GM)->NotifyTeamInfo(A, B, Name, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10);
        NCWnd* Observer = GetDesktop()->BroadcastObserverWnd;
        if (Observer && Observer->IsShowWindow())
            static_cast<NCBroadcastObserverWnd*>(Observer)->NotifyTeamInfo(A, B, Name, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10);
    }
    unguard;
}
typedef void (NConsoleWnd::*SigOnEventMatchTeamInfo)(INT, INT, FString, INT, INT, INT, INT, INT, INT, INT, INT, INT, INT);
NW_HOOK_VARARGS(0x10061470, NConsoleWnd, OnEventMatchTeamInfo, SigOnEventMatchTeamInfo)

// 0x100615e0 (C3 virtual 364; "NConsoelWnd" in the stock's guard) - the teams' list ends (0x1010DB90, 0x10117EC0)
void NConsoleWnd::OnEventMatchTeamInfoEnd()
{
    guard(NConsoelWnd::OnEventMatchTeamInfoEnd);
    if (Desktop)
    {
        NCWnd* GM = GetDesktop()->BroadcastGMWnd;
        if (GM && GM->IsShowWindow())
            static_cast<NCBroadcastGMWnd*>(GM)->NotifyTeamInfoEnd();
        NCWnd* Observer = GetDesktop()->BroadcastObserverWnd;
        if (Observer && Observer->IsShowWindow())
            static_cast<NCBroadcastObserverWnd*>(Observer)->NotifyTeamInfoEnd();
    }
    unguard;
}
NW_HOOK_METHOD(0x100615E0, NConsoleWnd, OnEventMatchTeamInfoEnd)

// 0x100616b0 (C3 virtual 365; guard name) - a game was created: its id as text to the GM's window (0x1010CF00)
void NConsoleWnd::OnEventMatchCreate(INT GameID)
{
    guard(NConsoleWnd::OnEventMatchCreate);
    if (Desktop && GetDesktop()->BroadcastGMWnd)
        static_cast<NCBroadcastGMWnd*>(GetDesktop()->BroadcastGMWnd)->NotifyCreateBroadcastGame(FString::Printf(TEXT("%d"), GameID));
    unguard;
}
NW_HOOK_METHOD(0x100616B0, NConsoleWnd, OnEventMatchCreate)

// 0x10061760 (C3 virtual 366; guard "OnEventMatchUserInfo") - a player of the match (a number, a name, eight numbers) to
// the observer's window (NotifyUserInfo 0x10118010)
void NConsoleWnd::OnEventMatchUserInfo(INT A, FString Name, INT P1, INT P2, INT P3, INT P4, INT P5, INT P6, INT P7, INT P8)
{
    guard(NConsoleWnd::OnEventMatchUserInfo);
    if (Desktop && GetDesktop()->BroadcastObserverWnd)
        static_cast<NCBroadcastObserverWnd*>(GetDesktop()->BroadcastObserverWnd)->NotifyUserInfo(A, Name, P1, P2, P3, P4, P5, P6, P7, P8);
    unguard;
}
typedef void (NConsoleWnd::*SigOnEventMatchUserInfo)(INT, FString, INT, INT, INT, INT, INT, INT, INT, INT);
NW_HOOK_VARARGS(0x10061760, NConsoleWnd, OnEventMatchUserInfo, SigOnEventMatchUserInfo)

// 0x10061840 (C3 virtual 367; the stock's guard string is the previous function's) - the spells' list of a player begins
// (NotifySpellInfoStart 0x101192E0)
void NConsoleWnd::OnEventMatchSpellInfoStart(INT A, INT B, INT C)
{
    guard(NConsoleWnd::OnEventMatchUserInfo);
    if (Desktop && GetDesktop()->BroadcastObserverWnd)
        static_cast<NCBroadcastObserverWnd*>(GetDesktop()->BroadcastObserverWnd)->NotifySpellInfoStart(A, B, C);
    unguard;
}
NW_HOOK_METHOD(0x10061840, NConsoleWnd, OnEventMatchSpellInfoStart)

// 0x100618e0 (C3 virtual 368; the same guard string) - a spell of the list (NotifySpellInfo 0x10119400)
void NConsoleWnd::OnEventMatchSpellInfo(INT A, INT B, INT C, INT D)
{
    guard(NConsoleWnd::OnEventMatchUserInfo);
    if (Desktop && GetDesktop()->BroadcastObserverWnd)
        static_cast<NCBroadcastObserverWnd*>(GetDesktop()->BroadcastObserverWnd)->NotifySpellInfo(A, B, C, D);
    unguard;
}
NW_HOOK_METHOD(0x100618E0, NConsoleWnd, OnEventMatchSpellInfo)

// 0x10061980 (C3 virtual 369; the same guard string) - the spells' list ends (NotifySpellInfoEnd 0x10118140)
void NConsoleWnd::OnEventMatchSpellInfoEnd()
{
    guard(NConsoleWnd::OnEventMatchUserInfo);
    if (Desktop && GetDesktop()->BroadcastObserverWnd)
        static_cast<NCBroadcastObserverWnd*>(GetDesktop()->BroadcastObserverWnd)->NotifySpellInfoEnd();
    unguard;
}
NW_HOOK_METHOD(0x10061980, NConsoleWnd, OnEventMatchSpellInfoEnd)

// 0x10061a10 (C3 virtual 370; guard name) - a team unlocked (NotifyTeamUnlocked 0x101181E0)
void NConsoleWnd::OnEventMatchTeamUnlocked(INT A, INT B)
{
    guard(NConsoleWnd::OnEventMatchTeamUnlocked);
    if (Desktop && GetDesktop()->BroadcastObserverWnd)
        static_cast<NCBroadcastObserverWnd*>(GetDesktop()->BroadcastObserverWnd)->NotifyTeamUnlocked(A, B);
    unguard;
}
NW_HOOK_METHOD(0x10061A10, NConsoleWnd, OnEventMatchTeamUnlocked)

// 0x10061aa0 (C3 virtual 371; guard name) - the GM's broadcast window shown
void NConsoleWnd::OnEventMatchGMTest()
{
    guard(NConsoleWnd::OnEventMatchGMTest);
    if (Desktop && GetDesktop()->BroadcastGMWnd)
        GetDesktop()->BroadcastGMWnd->ShowWindow();
    unguard;
}
NW_HOOK_METHOD(0x10061AA0, NConsoleWnd, OnEventMatchGMTest)

// 0x10061b30 (C3 virtual 372; guard name) - a skill cast (the casters' and the targets' object ids at +0x18 of their
// records, the third value as it came) to the observer's window (NotifyMagicSkillUse 0x10118BC0)
void NConsoleWnd::OnReceiveMagicSkillUse(User* Caster, User* Target, INT Param)
{
    guard(NConsoleWnd::OnReceiveMagicSkillUse);
    if (Desktop && GetDesktop()->BroadcastObserverWnd)
        static_cast<NCBroadcastObserverWnd*>(GetDesktop()->BroadcastObserverWnd)->NotifyMagicSkillUse(Caster->ObjectID, Target->ObjectID, Param);
    unguard;
}
NW_HOOK_METHOD(0x10061B30, NConsoleWnd, OnReceiveMagicSkillUse)

// 0x10061bd0 (C3 virtual 373; guard name) - a character's info, only while the console is in the observer's UI mode
// (UIMode 1), to the observer's window (NotifyCharInfo 0x10118AE0)
void NConsoleWnd::OnCharInfo(INT Info)
{
    guard(NConsoleWnd::OnCharInfo);
    if (UIMode == 1 && Desktop && GetDesktop()->BroadcastObserverWnd)
        static_cast<NCBroadcastObserverWnd*>(GetDesktop()->BroadcastObserverWnd)->NotifyCharInfo((User*)Info);
    unguard;
}
NW_HOOK_METHOD(0x10061BD0, NConsoleWnd, OnCharInfo)

// 0x10061c70 (C3 virtual 384; guard name) - a command line as if it was typed in the chat (NCChatWnd::ProcessChatLine)
void NConsoleWnd::ExecuteCommand(TCHAR* Line)
{
    guard(NConsoleWnd::ExecuteCommand);
    if (Desktop && GetDesktop()->ChatWnd)
        GetDesktop()->ChatWnd->ProcessChatLine(Line);
    unguard;
}
NW_HOOK_METHOD(0x10061C70, NConsoleWnd, ExecuteCommand)

/*-----------------------------------------------------------------------------
    The C3 buff window's second list and its extra icon (the desktop's +0x108: the console's slots 166 .. 169).
-----------------------------------------------------------------------------*/

// NCAbnormalStatusWnd's own functions this port reaches (NCAbnormalStatusWndExt, declared in NCConsole.h)

// a record of the buff window from the status the console built (the icon by its texture's name, the black texture when
// there is none, the level, the time and the skill id, the name and the description from the skill tables, the skill's
// tool tip): AddStatus' own body, for both new places
static FORCEINLINE NAbnormalEntry* MakeAbnormalEntry(NCAbnormalStatusWnd* Wnd, FAbnormalStatus* Status)
{
    NAbnormalEntry* Entry = new NAbnormalEntry;
    if (Status->IconName)
        Entry->Icon = Wnd->FindTexture(Status->IconName);
    if (!Entry->Icon)
        Entry->Icon = Wnd->FindTexture(TEXT("NWindow.BlackTexture"));
    Entry->UnknownC18 = Status->Level;
    Entry->RemainTime = Status->RemainTime;
    Entry->ID = Status->SkillID;
    if (GConsole->GetSkillName(Status->SkillID, Status->Level))
        appStrcpy(Entry->Name, GConsole->GetSkillName(Status->SkillID, Status->Level));
    if (GConsole->GetSkillDesc(Status->SkillID, Status->Level))
        appStrcpy(Entry->Description, GConsole->GetSkillDesc(Status->SkillID, Status->Level));
    Entry->FillToolTip(NTT_SkillName, 0);
    return Entry;
}

// 0x10101f50 (name ours) - a buff into the second list; 0 when it holds ten already. The window is opened when hidden
INT NCAbnormalStatusWndExt::AddStatus2(FAbnormalStatus* Status)
{
    NCAbnormalStatusWnd* This = (NCAbnormalStatusWnd*)this;
    if (This->Statuses2.Num() > 9)
        return 0;
    if (!This->IsShowWindow())
        This->ShowWindow();
    NAbnormalEntry* Entry = MakeAbnormalEntry(This, Status);
    INT i = This->Statuses2.Add();
    This->Statuses2(i) = Entry;
    This->UpdateLayout();
    return 1;
}
NW_HOOK_METHOD(0x10101F50, NCAbnormalStatusWndExt, AddStatus2)

// 0x10103a50 (name ours) - the extra icon: set from the status, or removed when there is none or its name is empty. The
// previous icon is not freed by a new one (as in the stock)
INT NCAbnormalStatusWndExt::SetExtraStatus(FAbnormalStatus* Status)
{
    NCAbnormalStatusWnd* This = (NCAbnormalStatusWnd*)this;
    if (!This->IsShowWindow())
        This->ShowWindow();
    if (Status && Status->Name[0])
    {
        NAbnormalEntry* Entry = MakeAbnormalEntry(This, Status);
        This->ExtraStatus = Entry;
        This->UpdateLayout();
    }
    else
        ((NCAbnormalStatusWndExt*)This)->ClearExtraStatus();
    return 1;
}
NW_HOOK_METHOD(0x10103A50, NCAbnormalStatusWndExt, SetExtraStatus)

// 0x10102b30 (name ours) - the second list's records go and the layout is redone (vslot 68)
void NCAbnormalStatusWndExt::ClearStatus2()
{
    NCAbnormalStatusWnd* This = (NCAbnormalStatusWnd*)this;
    for (INT i = 0; i < This->Statuses2.Num(); i++)
    {
        if (This->Statuses2(i))
            delete This->Statuses2(i);
    }
    This->Statuses2.Empty();
    This->UpdateLayout();
}
NW_HOOK_METHOD(0x10102B30, NCAbnormalStatusWndExt, ClearStatus2)

// 0x10102ba0 (name ours) - the extra icon goes and the layout is redone (nothing to do without one)
void NCAbnormalStatusWndExt::ClearExtraStatus()
{
    NCAbnormalStatusWnd* This = (NCAbnormalStatusWnd*)this;
    if (This->ExtraStatus)
    {
        delete This->ExtraStatus;
        This->ExtraStatus = NULL;
        This->UpdateLayout();
    }
}
NW_HOOK_METHOD(0x10102BA0, NCAbnormalStatusWndExt, ClearExtraStatus)

// 0x10102c50 (name ours) - the buff window forgets everything (NCConsole::Reset): the first list, the extra icon, the
// second list (the last one as a tail jump)
void NCAbnormalStatusWndExt::ClearAll()
{
    ((NCAbnormalStatusWnd*)this)->ClearStatus();
    ClearExtraStatus();
    ClearStatus2();
}
NW_HOOK_METHOD(0x10102C50, NCAbnormalStatusWndExt, ClearAll)

// 0x10064bd0 (C3 virtual 169) - a buff of the second list: the skill's id and level (the time is not sent)
INT NConsoleWnd::AddAbnormalStatus2(L2ParamStack& Params)
{
    FAbnormalStatus Status;
    Status.SkillID = (INT)Params.Top();
    Status.Level = (INT)Params.Top();
    Status.RemainTime = 0;
    if (GetSkillIconName(Status.SkillID, Status.Level))
        appStrcpy(Status.IconName, GetSkillIconName(Status.SkillID, Status.Level));
    if (GetSkillName(Status.SkillID, Status.Level))
        appStrcpy(Status.Name, GetSkillName(Status.SkillID, Status.Level));
    if (GetSkillDesc(Status.SkillID, Status.Level))
        appStrcpy(Status.Desc, GetSkillDesc(Status.SkillID, Status.Level));
    return ((NCAbnormalStatusWndExt*)GetDesktop()->AbnormalStatusWnd)->AddStatus2(&Status);
}
NW_HOOK_METHOD(0x10064BD0, NConsoleWnd, AddAbnormalStatus2)

// 0x10064d20 (C3 virtual 167) - the extra icon: the skill's id, its level and the seconds left
INT NConsoleWnd::SetExtraAbnormalStatus(L2ParamStack& Params)
{
    FAbnormalStatus Status;
    Status.SkillID = (INT)Params.Top();
    Status.Level = (INT)Params.Top();
    Status.RemainTime = (INT)Params.Top();
    if (GetSkillIconName(Status.SkillID, Status.Level))
        appStrcpy(Status.IconName, GetSkillIconName(Status.SkillID, Status.Level));
    if (GetSkillName(Status.SkillID, Status.Level))
        appStrcpy(Status.Name, GetSkillName(Status.SkillID, Status.Level));
    if (GetSkillDesc(Status.SkillID, Status.Level))
        appStrcpy(Status.Desc, GetSkillDesc(Status.SkillID, Status.Level));
    return ((NCAbnormalStatusWndExt*)GetDesktop()->AbnormalStatusWnd)->SetExtraStatus(&Status);
}
NW_HOOK_METHOD(0x10064D20, NConsoleWnd, SetExtraAbnormalStatus)

/*-----------------------------------------------------------------------------
    The quest page's target (the quest the player follows: its name at +0xE4 and a place at +0xF0 of the quest page), the
    HP bar's animation, the shortcut bar's auto shots and a few dialogs.
-----------------------------------------------------------------------------*/

// 0x100da340 / 0x100da370 / 0x100da3a0 (names ours) - NCQuestGaraTreeWnd's target: the name (a copy of the FString at
// +0xE4) and the three words at +0xF0 (a place), read and written
FString NCQuestGaraTreeWndExt::GetTargetName()
{
    return nw::Field<FString>(this, 0xE4);
}
NW_HOOK_METHOD(0x100DA340, NCQuestGaraTreeWndExt, GetTargetName)

FVector NCQuestGaraTreeWndExt::GetTargetPos()
{
    return nw::Field<FVector>(this, 0xF0);
}
NW_HOOK_METHOD(0x100DA370, NCQuestGaraTreeWndExt, GetTargetPos)

void NCQuestGaraTreeWndExt::SetTargetPos(INT X, INT Y, INT Z)
{
    nw::Field<INT>(this, 0xF0) = X;
    nw::Field<INT>(this, 0xF4) = Y;
    nw::Field<INT>(this, 0xF8) = Z;
}
NW_HOOK_METHOD(0x100DA3A0, NCQuestGaraTreeWndExt, SetTargetPos)

// 0x10060c90 (C3 virtual 345) - the followed quest's target: its name and its place into the caller's records
void NConsoleWnd::GetQuestTarget(FString* Name, FVector* Pos)
{
    NCQuestGaraTreeWndExt* Quest = (NCQuestGaraTreeWndExt*)GetDesktop()->MainWnd->QuestWnd;
    *Name = Quest->GetTargetName();
    *Pos = Quest->GetTargetPos();
}
NW_HOOK_METHOD(0x10060C90, NConsoleWnd, GetQuestTarget)

// 0x10077ce0 (C3 virtual 355) - the followed quest's target name (empty without the quest page)
FString NConsoleWnd::GetQuestTargetName()
{
    FString Name;
    if (Desktop && GetDesktop()->MainWnd && GetDesktop()->MainWnd->QuestWnd)
        Name = ((NCQuestGaraTreeWndExt*)GetDesktop()->MainWnd->QuestWnd)->GetTargetName();
    return Name;
}
NW_HOOK_METHOD(0x10077CE0, NConsoleWnd, GetQuestTargetName)

// 0x10077db0 (C3 virtual 356) - the followed quest's target place ((0, 0, 0) without the quest page)
FVector NConsoleWnd::GetQuestTargetPos()
{
    if (Desktop && GetDesktop()->MainWnd && GetDesktop()->MainWnd->QuestWnd)
        return ((NCQuestGaraTreeWndExt*)GetDesktop()->MainWnd->QuestWnd)->GetTargetPos();
    FVector Zero;
    nw::Field<INT>(&Zero, 0) = 0;
    nw::Field<INT>(&Zero, 4) = 0;
    nw::Field<INT>(&Zero, 8) = 0;
    return Zero;
}
NW_HOOK_METHOD(0x10077DB0, NConsoleWnd, GetQuestTargetPos)

// UE2's appSeconds (UnVcWin32.h): the time stamp counter in seconds, plus 2^24 (the stock expands it inline)
static FORCEINLINE DOUBLE NwAppSeconds()
{
    unsigned __int64 Cycles = __rdtsc();
    DWORD L = (DWORD)Cycles;
    DWORD H = (DWORD)(Cycles >> 32);
    return ((DOUBLE)L + 4294967296.0 * (DOUBLE)H) * GSecondsPerCycle + 16777216.0;
}

// NCStatusBarCtrl's animated change (the player's HP bar, NCPlayerStatusWnd +0x334)
struct NCStatusBarCtrlExt
{
    __declspec(noinline) void StartAnim(INT Value, INT Max, DOUBLE Time);   // 0x100FB700
};

// 0x100fb700 (name ours) - the bar moves to Value of Max over Time seconds: it starts from a point below the value (by
// 0.666 of half the maximum), the speed is that distance over the bar's length times the time, and the start is stamped
void NCStatusBarCtrlExt::StartAnim(INT Value, INT Max, DOUBLE Time)
{
    nw::Field<DOUBLE>(this, 0x1F8) = (DOUBLE)Value - (DOUBLE)(Max / 2) * 0.666;
    nw::Field<DOUBLE>(this, 0x20C) = nw::Field<DOUBLE>(this, 0x1F8) / ((DOUBLE)Max * 0.666) * Time;
    nw::Field<DOUBLE>(this, 0x204) = Time;
    nw::Field<INT>(this, 0x200) = Max;
    nw::Field<DOUBLE>(this, 0x218) = NwAppSeconds();
}
NW_HOOK_METHOD(0x100FB700, NCStatusBarCtrlExt, StartAnim)

// 0x100611b0 (C3 virtual 358) - the HP bar of the player's status window animates (the first value is not read)
void NConsoleWnd::StartHPBarAnim(INT Unused, INT Value, INT Max, DOUBLE Time)
{
    ((NCStatusBarCtrlExt*)GetDesktop()->PlayerStatusWnd->HPBar)->StartAnim(Value, Max, Time);
}
NW_HOOK_METHOD(0x100611B0, NConsoleWnd, StartHPBarAnim)

// 0x10062110 (C3 virtual 380) - the auto use of a shot (its id, on / off) is set on every shortcut slot that holds it (the
// stock walks the 120 slots of the bar: 12 pages of 10)
void NConsoleWnd::SetShotAutoUse(L2ParamStack& Params)
{
    INT ItemID = (INT)Params.Top();
    INT bOn = (INT)Params.Top();
    for (INT i = 0; i < 120; i++)
    {
        NShortCutItem& Slot = GetDesktop()->ShortCutWnd->Items[i];
        if (Slot.ID == ItemID)
            Slot.bAutoShot = (bOn != 0);
    }
}
NW_HOOK_METHOD(0x10062110, NConsoleWnd, SetShotAutoUse)

// 0x100626a0 (name ours) - TArray<INT>::FindItem (the stock's instance for the shortcut code): the index of the first
// element equal to Item in Index (the count when none), 1 when found
struct NIntArrayExt
{
    __declspec(noinline) UBOOL FindItem(const INT* Item, INT* Index);   // 0x100626A0
};
UBOOL NIntArrayExt::FindItem(const INT* Item, INT* Index)
{
    TArray<INT>* This = (TArray<INT>*)this;
    *Index = 0;
    for (; *Index < This->Num(); (*Index)++)
    {
        if ((*This)((*Index)) == *Item)
            return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x100626A0, NIntArrayExt, FindItem)

// 0x10075140 (C3 virtual 381) - the shots of the auto list are used for the weapon the player wears (the first record of the
// paperdoll with type 0; its crystal grade at +0x195C: 0 none .. 5 S). With bFlag 0 every listed shot whose grade is the
// weapon's is used (D: 0x5B7, C: 0x5B8, B: 0x5B9, A: 0x5BA, S: 0x5BB; no grade: 0x72B and 0x169D); with bFlag set the first
// of the grade's list that the auto list holds is used (no grade: 0x9CD, 0xF6B, 0x169E; D: 0x9CE, 0xF6C; C: 0x9CF, 0xF6D;
// B: 0x9D0, 0xF6E; A: 0x9D1, 0xF6F; S: 0x9D2, 0xF70). The shot is used by its object id
void NConsoleWnd::UseAutoShots(INT bFlag)
{
    if (!Desktop)
        return;
    TArray<NItemEntry*>& Worn = GetDesktop()->InventoryWnd->EquipWnd->Items;
    INT n = Worn.Num();
    if (n <= 0)
        return;
    INT w = 0;
    NItemEntry* Weapon = NULL;
    for (; w < n; w++)
    {
        Weapon = Worn(w);
        if (Weapon && Weapon->Type == 0)
            break;
    }
    if (!Weapon)
        return;
    TArray<INT> Ids;
    TArray<INT> Objects;
    for (INT s = 0; s < 120; s++)
    {
        NShortCutItem* Slot = &GetDesktop()->ShortCutWnd->Items[s];
        if (Slot && Slot->bAutoShot)
        {
            Ids.AddItem(Slot->ID);
            Objects.AddItem(Slot->ObjectID);
        }
    }
    if (bFlag == 0)
    {
        for (INT i = 0; i < Ids.Num(); i++)
        {
            INT Id = Ids(i);
            UBOOL bUse = 0;
            if (Id > 0x72B)
                bUse = (Id == 0x169D) && Weapon->CrystalGrade == 0;
            else if (Id == 0x72B)
                bUse = Weapon->CrystalGrade == 0;
            else if ((UINT)(Id - 0x5B7) <= 4)
                bUse = Weapon->CrystalGrade == Id - 0x5B7 + 1;
            if (bUse)
                RequestUseItem(Objects(i));
        }
    }
    else
    {
        static const INT Lists[6][3] =
        {
            { 0x9CD, 0xF6B, 0x169E },
            { 0x9CE, 0xF6C, 0 },
            { 0x9CF, 0xF6D, 0 },
            { 0x9D0, 0xF6E, 0 },
            { 0x9D1, 0xF6F, 0 },
            { 0x9D2, 0xF70, 0 }
        };
        INT Index = -1;
        INT Grade = Weapon->CrystalGrade;
        if ((UINT)Grade <= 5)
        {
            for (INT k = 0; k < 3; k++)
            {
                INT Id = Lists[Grade][k];
                if (!Id)
                    break;
                if (((NIntArrayExt*)&Ids)->FindItem(&Id, &Index))
                {
                    if (Index >= 0)
                        RequestUseItem(Objects(Index));
                    break;
                }
            }
        }
    }
}
NW_HOOK_METHOD(0x10075140, NConsoleWnd, UseAutoShots)

// 0x10061e40 (C3 virtual 378) - the party room window closes (the child at +0xCC is told through its slot 71); the
// parameter stack is not read
void NConsoleWnd::ClosePartyRoom(L2ParamStack& Params)
{
    GetDesktop()->PartyRoomWnd->HideWindow();
    nw::VirtualCall<void>(nw::Field<NCWnd*>(GetDesktop()->PartyRoomWnd, 0xCC), 71);
}
NW_HOOK_METHOD(0x10061E40, NConsoleWnd, ClosePartyRoom)

// 0x10062b90 (C3 virtual 306) - a message of the messenger library for its window (the grandchild of the desktop's
// messenger window, +0xC8 twice): 0x506 and 0x507 go to its slots 0 and 1 with the two words
void NConsoleWnd::DispatchMessengerMsg(INT hWnd, UINT Msg, INT wParam, INT lParam)
{
    if (!Desktop || !GetDesktop()->MessengerWnd)
        return;
    NCWnd* Child = nw::Field<NCWnd*>(GetDesktop()->MessengerWnd, 0xC8);
    if (!Child)
        return;
    NCWnd* Target = nw::Field<NCWnd*>(Child, 0xC8);
    if (!Target)
        return;
    if (Msg == 0x506)
        nw::VirtualCall<void>(Target, 0, wParam, lParam);
    else if (Msg == 0x507)
        nw::VirtualCall<void>(Target, 1, wParam, lParam);
}
NW_HOOK_METHOD(0x10062B90, NConsoleWnd, DispatchMessengerMsg)

// 0x1006b0c0 (C3 virtual 344) - a system message the server sends as a dialog box of the kind 0x46: the message's id from
// the stack, passed as the box's parameter too
void NConsoleWnd::ShowMessageDialog46(L2ParamStack& Params)
{
    BYTE InfoMem[sizeof(NDialogInfo)];
    NDialogInfo* Info = ::new ((void*)InfoMem) NDialogInfo;
    INT MsgID = (INT)Params.Top();
    Info->ID = 0x46;
    Info->bMainBox = 1;
    Info->bInput = 0;
    Info->SetParams(MsgID, -1);
    Info->SetSystemMessage(MsgID);
    GConsole->ShowDialog(Info);
    Info->~NDialogInfo();   // 0x10006E20
}
NW_HOOK_METHOD(0x1006B0C0, NConsoleWnd, ShowMessageDialog46)

// the windows that an end dialog hides (the stock's seven: the buffs, the chat, the shortcuts, the player's status, the
// menu bar, the radar and the observer's window; the desktop itself is shown first)
static void HideGameWindows(NConsoleWnd* Console, UBOOL bWithObserver)
{
    NCConsole* Desk = Console->GetDesktop();
    Desk->AbnormalStatusWnd->HideWindow();
    Desk->ChatWnd->HideWindow();
    Desk->ShortCutWnd->HideWindow();
    Desk->PlayerStatusWnd->HideWindow();
    Desk->MenuWnd->HideWindow();
    Desk->RadarWnd->HideWindow();
    if (bWithObserver)
        Desk->NewWnd198->HideWindow();
}

// 0x1006b700 (C3 virtual 387) - the end of a match: the desktop is shown, the game's windows hidden, and a dialog of the
// kind 0x1C (the system menu window answers: its entry is the owner) asks with the system message 1337
void NConsoleWnd::ShowMatchEndDialog()
{
    GetDesktop()->ShowWindow();
    HideGameWindows(this, 1);
    BYTE InfoMem[sizeof(NDialogInfo)];
    NDialogInfo* Info = ::new ((void*)InfoMem) NDialogInfo;
    Info->bInput = 0;
    Info->ID = 0x1C;
    Info->bMainBox = 1;
    Info->Owner = GetDesktop()->SystemMenuWnd->Entry;
    Info->SetSystemMessage(1337);
    GConsole->ShowDialog(Info);
    Info->~NDialogInfo();   // 0x10006E20
}
NW_HOOK_METHOD(0x1006B700, NConsoleWnd, ShowMatchEndDialog)

// 0x1006b840 (C3 virtual 388) - a text in a dialog of the kind 0x50 (owned by the system menu window); with bHide set the
// desktop is shown and the game's windows are hidden first (all but the observer's window)
void NConsoleWnd::ShowMatchTextDialog(const TCHAR* Text, INT bHide)
{
    if (bHide)
    {
        GetDesktop()->ShowWindow();
        HideGameWindows(this, 0);
    }
    BYTE InfoMem[sizeof(NDialogInfo)];
    NDialogInfo* Info = ::new ((void*)InfoMem) NDialogInfo;
    Info->ID = 0x50;
    Info->bMainBox = 1;
    Info->bInput = 0;
    Info->Owner = GetDesktop()->SystemMenuWnd->Entry;
    Info->SetText(Text);
    GConsole->ShowDialog(Info);
    Info->~NDialogInfo();   // 0x10006E20
}
NW_HOOK_METHOD(0x1006B840, NConsoleWnd, ShowMatchTextDialog)

/*-----------------------------------------------------------------------------
    The Seven Signs board (NCSSQMainBoard, the desktop's +0x1F4): its status page (+0xD0), event page (+0xD4) and list
    page (+0xD8) are fed by the console's slots 286 .. 290. The pages' tree builders (0x100EB1D0, 0x100ECCA0 and the root
    makers 0x100EB150, 0x100ECC20) stay the stock's, called by address. Names ours.
-----------------------------------------------------------------------------*/

struct NCSSQStatusPageExt
{
    __declspec(noinline) void SetStatus(const INT* Record);          // 0x100E8E60 the sixteen words of a status packet
    __declspec(noinline) void ClearLists();                          // 0x100E98E0 the three lists at +0x104, +0x110, +0x11C of +0x108
    __declspec(noinline) void ReadLists(L2ParamStack& Params);       // 0x100E9800 the lists from the packet
};
struct NCSSQEventPageExt
{
    __declspec(noinline) INT AddEvent(const void* Record);           // 0x100EB570 the tree builder of the child at +0xCC
    __declspec(noinline) void Reset();                               // 0x100EB580 the page's slot 72, then a new root
};
struct NCSSQListPageExt
{
    __declspec(noinline) INT AddEntry(const void* Record);           // 0x100ECFC0 the tree builder of the child at +0xC8
    __declspec(noinline) void Reset();                               // 0x100ECFD0 the child's slot 72 (0x100EBB50), then a new root
};

// 0x100e8e60 (name ours) - the page keeps the packet's words (the record's order is the packet's: a few of them sit
// out of line)
void NCSSQStatusPageExt::SetStatus(const INT* R)
{
    nw::Field<INT>(this, 0xC8) = R[0x00 / 4];
    nw::Field<INT>(this, 0xF8) = R[0x30 / 4];
    nw::Field<INT>(this, 0xCC) = R[0x04 / 4];
    nw::Field<INT>(this, 0xD0) = R[0x08 / 4];
    nw::Field<INT>(this, 0xD4) = R[0x0C / 4];
    nw::Field<INT>(this, 0x104) = R[0x3C / 4];
    nw::Field<INT>(this, 0xD8) = R[0x10 / 4];
    nw::Field<INT>(this, 0xDC) = R[0x14 / 4];
    nw::Field<INT>(this, 0xE0) = R[0x18 / 4];
    nw::Field<INT>(this, 0xE4) = R[0x1C / 4];
    nw::Field<INT>(this, 0xE8) = R[0x20 / 4];
    nw::Field<INT>(this, 0xEC) = R[0x24 / 4];
    nw::Field<INT>(this, 0xF0) = R[0x28 / 4];
    nw::Field<INT>(this, 0xF4) = R[0x2C / 4];
    nw::Field<INT>(this, 0xFC) = R[0x34 / 4];
    nw::Field<INT>(this, 0x100) = R[0x38 / 4];
}
NW_HOOK_METHOD(0x100E8E60, NCSSQStatusPageExt, SetStatus)

// 0x100e98e0 (name ours) - the page forgets its numbers and the three lists of the object at +0x108
void NCSSQStatusPageExt::ClearLists()
{
    nw::Field<INT>(this, 0x10C) = 0;
    nw::Field<INT>(this, 0x110) = 0;
    BYTE* Lists = nw::Field<BYTE*>(this, 0x108);
    nw::Field<TArray<INT> >(Lists, 0x104).Empty();
    nw::Field<TArray<INT> >(Lists, 0x110).Empty();
    nw::Field<TArray<INT> >(Lists, 0x11C).Empty();
}
NW_HOOK_METHOD(0x100E98E0, NCSSQStatusPageExt, ClearLists)

// 0x100e9800 (name ours) - a packet of the lists: a value that is dropped, the one the page keeps at +0x10C, the count
// (+0x110) and three values each, each into its list
void NCSSQStatusPageExt::ReadLists(L2ParamStack& Params)
{
    Params.Top();
    nw::Field<INT>(this, 0x10C) = (INT)Params.Top();
    INT Count = (INT)Params.Top();
    nw::Field<INT>(this, 0x110) = Count;
    for (INT i = 0; i < Count; i++)
    {
        BYTE* Lists = nw::Field<BYTE*>(this, 0x108);
        INT A = (INT)Params.Top();
        nw::Field<TArray<INT> >(Lists, 0x104).AddItem(A);
        Lists = nw::Field<BYTE*>(this, 0x108);
        INT B = (INT)Params.Top();
        nw::Field<TArray<INT> >(Lists, 0x110).AddItem(B);
        Lists = nw::Field<BYTE*>(this, 0x108);
        INT C = (INT)Params.Top();
        nw::Field<TArray<INT> >(Lists, 0x11C).AddItem(C);
    }
}
NW_HOOK_METHOD(0x100E9800, NCSSQStatusPageExt, ReadLists)

// 0x100eb570 / 0x100eb580 / 0x100ecfc0 / 0x100ecfd0 (names ours) - the event and the list pages hand their work to a child
INT NCSSQEventPageExt::AddEvent(const void* Record)
{
    return (INT)nw::Field<NCSSQMainEventGaraTreeCtrl*>(this, 0xCC)->AddEvent((const NSSQEventRecord*)Record);   // 0x100EB1D0
}
NW_HOOK_METHOD(0x100EB570, NCSSQEventPageExt, AddEvent)

void NCSSQEventPageExt::Reset()
{
    ((NCSSQMainEventWnd*)this)->ClearList();   // 0x100EA340 (its tree control's slot 72)
    nw::Field<NCSSQEventTreeWndExt*>(this, 0xCC)->MakeRoot();   // 0x100EB150
}
NW_HOOK_METHOD(0x100EB580, NCSSQEventPageExt, Reset)

INT NCSSQListPageExt::AddEntry(const void* Record)
{
    return (INT)nw::Field<NCSSQSealStatusGaraTreeCtrl*>(this, 0xC8)->AddEntry((const NSSQSealRecord*)Record);   // 0x100ECCA0
}
NW_HOOK_METHOD(0x100ECFC0, NCSSQListPageExt, AddEntry)

void NCSSQListPageExt::Reset()
{
    ((NCSSQSealStatusListWnd*)this)->ClearList();   // 0x100EBB50
    nw::Field<NCSSQListTreeWndExt*>(this, 0xC8)->MakeRoot();   // 0x100ECC20
}
NW_HOOK_METHOD(0x100ECFD0, NCSSQListPageExt, Reset)

// 0x10060f10 (C3 virtual 286) - the board's status: sixteen words (the record is filled in the packet's order, the page
// keeps them by their places), the board is shown with the focus
void NConsoleWnd::ReceiveSSQStatus(L2ParamStack& Params)
{
    INT R[16];
    R[0x00 / 4] = (INT)Params.Top();
    R[0x30 / 4] = (INT)Params.Top();
    R[0x34 / 4] = (INT)Params.Top();
    R[0x38 / 4] = (INT)Params.Top();
    R[0x04 / 4] = (INT)Params.Top();
    R[0x08 / 4] = (INT)Params.Top();
    R[0x0C / 4] = (INT)Params.Top();
    R[0x3C / 4] = (INT)Params.Top();
    R[0x10 / 4] = (INT)Params.Top();
    R[0x18 / 4] = (INT)Params.Top();
    R[0x14 / 4] = (INT)Params.Top();
    R[0x1C / 4] = (INT)Params.Top();
    R[0x20 / 4] = (INT)Params.Top();
    R[0x24 / 4] = (INT)Params.Top();
    R[0x28 / 4] = (INT)Params.Top();
    R[0x2C / 4] = (INT)Params.Top();
    GetDesktop()->SSQMainBoard->HideWindow();
    static_cast<NCSSQMainBoard*>(GetDesktop()->SSQMainBoard)->StatusWnd->ResetStatus();   // 0x100E8F00
    ((NCSSQStatusPageExt*)nw::Field<void*>(GetDesktop()->SSQMainBoard, 0xD0))->SetStatus(R);
    GetDesktop()->SSQMainBoard->ShowWindow();
    GetDesktop()->SSQMainBoard->SetFocus();
}
NW_HOOK_METHOD(0x10060F10, NConsoleWnd, ReceiveSSQStatus)

// 0x10060ed0 (C3 virtual 287) - the status page's lists: emptied, then read from the packet
void NConsoleWnd::ReceiveSSQLists(L2ParamStack& Params)
{
    NCSSQStatusPageExt* Page = (NCSSQStatusPageExt*)nw::Field<void*>(GetDesktop()->SSQMainBoard, 0xD0);
    Page->ClearLists();
    ((NCSSQStatusPageExt*)nw::Field<void*>(GetDesktop()->SSQMainBoard, 0xD0))->ReadLists(Params);
}
NW_HOOK_METHOD(0x10060ED0, NConsoleWnd, ReceiveSSQLists)

// 0x10061020 (C3 virtual 288) - the event page starts over: its slot 72 is told (0x100EA340), then it asks for a new
// root (Reset also tells the slot again, as the stock does)
void NConsoleWnd::ResetSSQEvents()
{
    static_cast<NCSSQMainBoard*>(GetDesktop()->SSQMainBoard)->EventWnd->ClearList();   // 0x100EA340
    ((NCSSQEventPageExt*)nw::Field<void*>(GetDesktop()->SSQMainBoard, 0xD4))->Reset();
}
NW_HOOK_METHOD(0x10061020, NConsoleWnd, ResetSSQEvents)

// a period of the event page (name ours; 0xF0 bytes: six words and two arrays of nine texts). The packet reader's view of the
// record NCSSQMainEventGaraTreeCtrl.h declares as NSSQEventRecord (the tree builder's names of the same words)
struct NSSQEventPacket
{
    INT Id;                 // +0x00 the packet's first value (restored after each entry)
    INT Period;             // +0x04 the packet's third (restored after each entry)
    INT Value8;             // +0x08
    INT ValueC;             // +0x0C
    INT Value10;            // +0x10
    INT Value14;            // +0x14
    FString Texts1[9];      // +0x18 filled from the second list of the entry
    FString Texts2[9];      // +0x84 filled from the first list of the entry

    __declspec(noinline) NSSQEventPacket();          // 0x10061060 the two arrays of FStrings (the numbers are left)
    __declspec(noinline) ~NSSQEventPacket();         // 0x100610d0 the second array, then the first
};
NW_ASSERT_SIZE(NSSQEventPacket, 0xF0);

NSSQEventPacket::NSSQEventPacket()
{
}
NW_HOOK_CTOR(0x10061060, NSSQEventPacket)

NSSQEventPacket::~NSSQEventPacket()
{
}
NW_HOOK_DTOR(0x100610d0, NSSQEventPacket)

// 0x1006b170 (C3 virtual 289) - the event page's entries: an id, a count of periods, and for each a number, a count of
// entries and for each three numbers and two lists of texts (up to nine kept: the stock does not check the counts),
// handed one by one to the page's builder; the texts are emptied after each
void NConsoleWnd::ReceiveSSQEvents(L2ParamStack& Params)
{
    NSSQEventPacket Record;
    INT Id = (INT)Params.Top();
    Record.Id = Id;
    INT Periods = (INT)Params.Top();
    for (INT p = 0; p < Periods; p++)
    {
        INT Period = (INT)Params.Top();
        Record.Period = Period;
        INT Entries = (INT)Params.Top();
        for (INT e = 0; e < Entries; e++)
        {
            Record.Value8 = (INT)Params.Top();
            Record.ValueC = (INT)Params.Top();
            Record.Value14 = (INT)Params.Top();
            INT Count2 = (INT)Params.Top();
            for (INT i = 0; i < Count2; i++)
            {
                TCHAR* Text = (TCHAR*)Params.Top();
                if (Text && i < 9)
                    Record.Texts2[i] = Text;
            }
            Record.Value10 = (INT)Params.Top();
            INT Count1 = (INT)Params.Top();
            for (INT i = 0; i < Count1; i++)
            {
                TCHAR* Text = (TCHAR*)Params.Top();
                if (Text && i < 9)
                    Record.Texts1[i] = Text;
            }
            ((NCSSQEventPageExt*)nw::Field<void*>(GetDesktop()->SSQMainBoard, 0xD4))->AddEvent(&Record);
            for (INT i = 0; i < 9; i++)
            {
                Record.Texts1[i].Empty();
                Record.Texts2[i].Empty();
            }
            Record.Id = Id;
            Record.Period = Period;
        }
    }
}
NW_HOOK_METHOD(0x1006B170, NConsoleWnd, ReceiveSSQEvents)

// 0x1006b330 (C3 virtual 290) - the list page starts over (its child's slot 72, then the page's own: Reset), then its
// entries: three numbers, a count and four numbers each, handed one by one to the page's builder
void NConsoleWnd::ReceiveSSQList(L2ParamStack& Params)
{
    static_cast<NCSSQMainBoard*>(GetDesktop()->SSQMainBoard)->SealWnd->ClearList();   // 0x100EBB50
    ((NCSSQListPageExt*)nw::Field<void*>(GetDesktop()->SSQMainBoard, 0xD8))->Reset();
    INT R[7];
    R[0] = 0;
    R[1] = 0;
    R[2] = 0;
    R[3] = 0;
    R[4] = 0;
    R[5] = 0;
    R[6] = 0;
    R[0] = (INT)Params.Top();
    R[3] = (INT)Params.Top();
    R[4] = (INT)Params.Top();
    INT Count = (INT)Params.Top();
    for (INT i = 0; i < Count; i++)
    {
        R[1] = (INT)Params.Top();
        R[2] = (INT)Params.Top();
        R[6] = (INT)Params.Top();
        R[5] = (INT)Params.Top();
        ((NCSSQListPageExt*)nw::Field<void*>(GetDesktop()->SSQMainBoard, 0xD8))->AddEntry(R);
    }
}
NW_HOOK_METHOD(0x1006B330, NConsoleWnd, ReceiveSSQList)

/*-----------------------------------------------------------------------------
    Wave 3 (3): the small anonymous methods that the console's functions and the shortcut keys call. Each is a member of a
    window class that is another lane's (or has no header): a small view struct carries it, hooked at the stock address.
-----------------------------------------------------------------------------*/


// 0x1000f7b0 (name ours) - NControl, the one whose mouse press the console follows (the object at the console's +0x39E4,
// asked on every tick): when it still holds the drag (+0x234) the position of the mouse relative to its window (and its
// press offset at +0x228 / +0x22C) goes to its parent as a message 3 and the drag ends
void NControlExt::EndMouseDrag()
{
    if (nw::Field<INT>(this, 0x234))
    {
        INT X, Y;
        GConsole->GetMousePos(&X, &Y);
        WORD DY = (WORD)Y;
        DY = (WORD)(DY - nw::Field<WORD>(this, 0x22C));
        WORD DX = (WORD)X;
        DX = (WORD)(DX - nw::Field<WORD>(this, 0x228));
        DY = (WORD)(DY - nw::Field<WORD>(this, 0x3C));
        DX = (WORD)(DX - nw::Field<WORD>(this, 0x38));
        ((NControl*)this)->SendToParent(3, 0, (INT)(((DWORD)DY << 16) | DX));   // 0x10005D20
        nw::Field<INT>(this, 0x234) = 0;
    }
}
NW_HOOK_METHOD(0x1000F7B0, NControlExt, EndMouseDrag)

// 0x100475a0 (name ours) - the pledge's info record (0x638 bytes: the name, the master's name, the crest, the ally's and the
// numbers) is cleared field by field; it returns itself
NPledgeInfoExt* NPledgeInfoExt::Clear()
{
    nw::Field<WORD>(this, 0x000) = 0;
    nw::Field<WORD>(this, 0x208) = 0;
    nw::Field<WORD>(this, 0x41C) = 0;
    nw::Field<INT>(this, 0x200) = 0;
    nw::Field<INT>(this, 0x204) = 0;
    nw::Field<INT>(this, 0x624) = 0;
    nw::Field<INT>(this, 0x408) = 0;
    nw::Field<INT>(this, 0x40C) = 0;
    nw::Field<INT>(this, 0x410) = 0;
    nw::Field<INT>(this, 0x414) = 0;
    nw::Field<INT>(this, 0x418) = 0;
    nw::Field<INT>(this, 0x61C) = 0;
    nw::Field<INT>(this, 0x620) = 0;
    nw::Field<INT>(this, 0x62C) = 0;
    nw::Field<INT>(this, 0x630) = 0;
    nw::Field<INT>(this, 0x634) = 0;
    return this;
}
NW_HOOK_METHOD(0x100475A0, NPledgeInfoExt, Clear)

// 0x10053780 (name ours) - the desktop tells the messenger window (+0x18C) that the player leaves (0x1013BB30, the
// messenger's own: it stays stock)
INT NCConsoleExt::LogoutMessenger()
{
    return static_cast<NMsnMessengerView*>(((NCConsole*)this)->MessengerWnd)->SignOut();
}
NW_HOOK_METHOD(0x10053780, NCConsoleExt, LogoutMessenger)

// 0x10053790 (name ours) - the chat window is hidden when shown, else shown and focused (the shortcut key's)
void NCConsoleExt::ToggleChatWnd()
{
    NCConsole* Desk = (NCConsole*)this;
    if (Desk->ChatWnd->IsShowWindow())
        Desk->ChatWnd->HideWindow();
    else
    {
        Desk->ChatWnd->ShowWindow();
        Desk->ChatWnd->SetFocus();
    }
}
NW_HOOK_METHOD(0x10053790, NCConsoleExt, ToggleChatWnd)

// 0x10060c30 (name ours; a shortcut key's) - the buff icons of the party's window (+0xDC), the pet's (+0x428) and the
// summoned creature's (+0x360) are switched on / off together
void NConsoleWnd::ToggleBuffIcons()
{
    NCConsole* Desk = GetDesktop();
    nw::Field<INT>(Desk->PartyWnd, 0xDC) = nw::Field<INT>(Desk->PartyWnd, 0xDC) == 0;
    nw::Field<INT>(Desk->PetStatusWnd, 0x428) = nw::Field<INT>(Desk->PetStatusWnd, 0x428) == 0;
    nw::Field<INT>(Desk->SummonedStatusWnd, 0x360) = nw::Field<INT>(Desk->SummonedStatusWnd, 0x360) == 0;
}
NW_HOOK_METHOD(0x10060C30, NConsoleWnd, ToggleBuffIcons)

// 0x100b9430 (name ours; a shortcut key's) - NCVideoOptionWnd: the "lower detail" check box (+0xF8, its state at +0x1F0)
// is flipped with a system message (0x503 on, 0x504 off, colored 0xFFB09B79), the window's slot 70 and Apply follow
void NCVideoOptionWndExt::ToggleLowerDetail()
{
    BYTE* Box = nw::Field<BYTE*>(this, 0xF8);
    INT MsgID;
    if (nw::Field<INT>(Box, 0x1F0))
    {
        nw::Field<INT>(Box, 0x1F0) = 0;
        MsgID = 0x504;
    }
    else
    {
        nw::Field<INT>(Box, 0x1F0) = 1;
        MsgID = 0x503;
    }
    GConsole->ShowSystemMessage(MsgID, 0xFFB09B79);
    nw::VirtualCall<void>(this, 70);
    ((NCVideoOptionWnd*)this)->Apply();   // 0x100B8550
}
NW_HOOK_METHOD(0x100B9430, NCVideoOptionWndExt, ToggleLowerDetail)

// 0x10118510 (name ours; a shortcut key's) - a flag of the observer's window (+0xF0) flipped
INT NCBroadcastObserverWndExt2::ToggleFlag()
{
    INT New = nw::Field<INT>(this, 0xF0) == 0;
    nw::Field<INT>(this, 0xF0) = New;
    return New;
}
NW_HOOK_METHOD(0x10118510, NCBroadcastObserverWndExt2, ToggleFlag)

// 0x1007aa40 (name ours) - NCDeliverItemWnd: an item of the package window's grid; 0 when it is full. The icon by its
// texture's name (the black texture when missing), the entry made inline from the record with its count as the trade
// count (a signed 64-bit), its tool tip (type 2) and the scroll range
UBOOL NCDeliverItemWndExt::AddItem(NItemInfo* Info)
{
    NCDeliverItemWnd* This = (NCDeliverItemWnd*)this;
    if (This->Items.Num() >= This->MaxItems)
        return 0;
    if (Info->IconName)
        Info->Icon = This->FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = This->FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Entry = NewItemEntry(Info);
    Entry->TradeCount = (SQWORD)Info->Count;
    Entry->FillToolTip(2, 0);
    This->Items.AddItem(Entry);
    This->UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x1007AA40, NCDeliverItemWndExt, AddItem)

// 0x1007ad80 (name ours) - NCDeliverWnd: an item of the player's inventory (the grid on top, +0xC8)
UBOOL NCDeliverWndExt::AddInventoryItem(NItemInfo* Info)
{
    return ((NCDeliverItemWndExt*)((NCDeliverWnd*)this)->InventoryItems)->AddItem(Info);
}
NW_HOOK_METHOD(0x1007AD80, NCDeliverWndExt, AddInventoryItem)

// 0x10082840 (name ours) - the GM's quest window keeps an item's id and count (a list of pairs at +0x180)
void NCGMQuestWndExt::AddQuestItem(INT ItemID, INT Count)
{
    TArray<NIntPair>& Pairs = nw::Field<TArray<NIntPair> >(this, 0x180);
    INT i = Pairs.Add();
    Pairs(i).A = ItemID;
    Pairs(i).B = Count;
}
NW_HOOK_METHOD(0x10082840, NCGMQuestWndExt, AddQuestItem)

// 0x10085c50 (name ours) - NCGodWnd: a flag three windows down (+0xD4, +0xD8, +0x104: the options window's check box
// at +0x1F0), 0 when one of them is missing
INT NCGodWndExt::GetFlag()
{
    BYTE* A = nw::Field<BYTE*>(this, 0xD4);
    if (!A)
        return 0;
    BYTE* B = nw::Field<BYTE*>(A, 0xD8);
    if (!B)
        return 0;
    BYTE* C = nw::Field<BYTE*>(B, 0x104);
    if (!C)
        return 0;
    return nw::Field<INT>(C, 0x1F0);
}
NW_HOOK_METHOD(0x10085C50, NCGodWndExt, GetFlag)

// 0x1008f490 (name ours) - NCLobbyWnd: the first word of each of the character slots (0x230 bytes each from +0x230, as
// many as +0x1030 says) is cleared
void NCLobbyWndExt::ClearSlots()
{
    BYTE* Slot = (BYTE*)this + 0x230;
    for (INT i = 0; i < nw::Field<INT>(this, 0x1030); i++)
    {
        nw::Field<INT>(Slot, 0) = 0;
        Slot += 0x230;
    }
}
NW_HOOK_METHOD(0x1008F490, NCLobbyWndExt, ClearSlots)

// 0x100970e0 (name ours) - NCVIPShopWnd: a product of the shop (a copy of the console's entry with the icon found by
// its name, the crystal grade of its item kept at +0x51C) into the window's records (+0xF0)
void NCVIPShopWndExt::AddRecord(const NVIPShopEntry* Src)
{
    NCVIPShopWnd* This = (NCVIPShopWnd*)this;
    NVIPShopRecord* Rec = (NVIPShopRecord*)appMalloc(sizeof(NVIPShopRecord), TEXT("new"));
    if (Rec)
        Rec = new ((void*)Rec) NVIPShopRecord;
    Rec->ProductID = Src->ProductID;
    Rec->Type = Src->Type;
    Rec->ItemID = Src->ItemID;
    appStrcpy(Rec->Name, Src->Name);
    Rec->Icon = This->FindTexture(Src->IconName);
    appStrcpy(Rec->Desc, Src->Desc);
    Rec->Count = Src->Count;
    Rec->BodyPart = Src->BodyPart;
    nw::Field<INT>(Rec, 0x518) = nw::Field<INT>(Src, 0x518);
    NViewItemData* Data = NItemTable::Find(&Rec->ItemID);
    if (Data)
    {
        INT Class = Data->Class;
        if (Class == 0)
            nw::Field<INT>(Rec, 0x51C) = nw::Field<INT>(Data, 0xBC);
        else if (Class == 1)
            nw::Field<INT>(Rec, 0x51C) = nw::Field<INT>(Data, 0x214);
        else if (Class == 2)
            nw::Field<INT>(Rec, 0x51C) = nw::Field<INT>(Data, 0x8C);
    }
    else
        nw::Field<INT>(Rec, 0x51C) = 0;
    This->Records.AddItem(Rec);
}
NW_HOOK_METHOD(0x100970E0, NCVIPShopWndExt, AddRecord)

// 0x100de7c0 (name ours) - NCRadarWnd: the target at that place is removed (the stock goes on to the element after the one
// that took its place)
void NCRadarWndExt::RemoveTarget(INT X, INT Y, INT Z)
{
    NCRadarWnd* This = (NCRadarWnd*)this;
    for (INT i = 0; i < This->Targets.Num(); i++)
    {
        FMinimapTarget& T = This->Targets(i);
        if (T.X == X && T.Y == Y && T.Z == Z)
            This->Targets.Remove(i, 1);
    }
}
NW_HOOK_METHOD(0x100DE7C0, NCRadarWndExt, RemoveTarget)

// 0x100e24b0 (name ours) - NCRecipeBookWnd: the dword at +0x16C (the book's id of the list that follows)
void NCRecipeBookWndExt::SetField16C(INT Value)
{
    nw::Field<INT>(this, 0x16C) = Value;
}
NW_HOOK_METHOD(0x100E24B0, NCRecipeBookWndExt, SetField16C)

// 0x100edab0 (name ours) - the server list's child: a 16-byte key (a session key of the login) at +0xFC, cleared when none
void NCLoginServerWndExt::SetKey(const void* Key)
{
    if (Key)
        appMemcpy((BYTE*)this + 0xFC, Key, 0x10);
    else
        appMemset((BYTE*)this + 0xFC, 0, 0x10);
}
NW_HOOK_METHOD(0x100EDAB0, NCLoginServerWndExt, SetKey)

// 0x1010b320 (name ours) - NCZoneTitleWnd: its three texts (+0xC8, +0xD4, +0xE0) are emptied, then set from the ones given
void NCZoneTitleWndExt::SetTexts(const TCHAR* Text1, const TCHAR* Text2, const TCHAR* Text3)
{
    FString& T1 = nw::Field<FString>(this, 0xC8);
    FString& T2 = nw::Field<FString>(this, 0xD4);
    FString& T3 = nw::Field<FString>(this, 0xE0);
    T1.Empty();
    T2.Empty();
    T3.Empty();
    if (Text1)
        T1 = Text1;
    if (Text2)
        T2 = Text2;
    if (Text3)
        T3 = Text3;
}
NW_HOOK_METHOD(0x1010B320, NCZoneTitleWndExt, SetTexts)

// 0x1010b470 (cdecl; the floats on the stack, the result in st0) - a number kept between a minimum and a maximum
static FLOAT __cdecl NZoneTitleClamp(FLOAT Value, FLOAT Min, FLOAT Max)
{
    if (Value < Min)
        return Min;
    if (Value >= Max)
        return Max;
    return Value;
}
NW_HOOK_FUNC(0x1010B470, NZoneTitleClamp)

// (NCZoneTitleWndTick, the zone title's stand-in for the method the stock keeps at 0x1010b4a0, is declared in NConsoleWndExt.h)

// 0x1010b4a0 - while the first line has text: a hidden window is placed (centered by the width of the longest line, a fifth of
// the screen down, as high as the first line) and shown, the time is added; past 8 seconds the lines are emptied, the time is
// reset and the window hidden. The cream text fades in over the first 2 seconds (alpha 127.5 a second) and out after the 4th
// (by 127.5 a second); the shadow in by 85 and out by 170
void NCZoneTitleWndTick::Tick(FLOAT DeltaTime)
{
    if (Line1.Len() <= 0)
        return;
    if (!IsShowWindow())
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, *Line1, 0, 0, 0, 0, 0, (L2FontType)4);
        SetPosition(GSEKScreenX / 2 - GetTitleWidth() / 2, GSEKScreenY / 5, GetTitleWidth(), Size.cy);
        ShowWindow();
    }
    DeltaTime = DeltaTime + Elapsed;
    Elapsed = DeltaTime;
    if (!(DeltaTime > 8.0f))
    {
        if (DeltaTime < 2.0f)
        {
            FLOAT Value = DeltaTime * 127.5f;
            if (Value < 0.0f)
                TextAlpha = (BYTE)(INT)0.0f;
            else if (Value < 255.0f)
                TextAlpha = (BYTE)(INT)Value;
            else
                TextAlpha = (BYTE)(INT)255.0f;
        }
        else if (DeltaTime > 4.0f)
            TextAlpha = (BYTE)(INT)NZoneTitleClamp(255.0f - (DeltaTime - 4.0f) * 127.5f, 0.0f, 255.0f);
        if (DeltaTime < 2.0f)
        {
            FLOAT Value = DeltaTime * 85.0f;
            if (Value < 0.0f)
                ShadowAlpha = (BYTE)(INT)0.0f;
            else if (Value < 255.0f)
                ShadowAlpha = (BYTE)(INT)Value;
            else
                ShadowAlpha = (BYTE)(INT)255.0f;
        }
        else if (DeltaTime > 4.0f)
            ShadowAlpha = (BYTE)(INT)NZoneTitleClamp(255.0f - (DeltaTime - 4.0f) * 170.0f, 0.0f, 255.0f);
        return;
    }
    Line1.Empty();
    Line2.Empty();
    Line3.Empty();
    Elapsed = 0.0f;
    if (IsShowWindow())
        HideWindow();
}
NW_HOOK_METHOD(0x1010B4A0, NCZoneTitleWndTick, Tick)

// the root item of a tree window (the quest page's tree and the Seven Signs board's two): a default tree item (0x1EB0
// bytes, the stock's constructor and destructor) with its second word cleared is added at the top of the tree (the
// control at +0x114, its slot 69), and the result kept at +0x118
static FORCEINLINE void MakeTreeRoot(BYTE* Wnd)
{
    BYTE Mem[sizeof(NGaraTVItemData)];
    NGaraTVItemData* Item = ::new ((void*)Mem) NGaraTVItemData;   // 0x10012890
    nw::Field<INT>(Item, 4) = 0;
    INT Root = nw::VirtualCall<INT>(nw::Field<void*>(Wnd, 0x114), 69, (void*)Item, (INT)0);
    nw::Field<INT>(Wnd, 0x118) = Root;
    Item->~NGaraTVItemData();   // 0x10012A00
}

void NCQuestTreeWndExt::MakeRoot()
{
    MakeTreeRoot((BYTE*)this);
}
NW_HOOK_METHOD(0x100DB380, NCQuestTreeWndExt, MakeRoot)

void NCSSQEventTreeWndExt::MakeRoot()
{
    MakeTreeRoot((BYTE*)this);
}
NW_HOOK_METHOD(0x100EB150, NCSSQEventTreeWndExt, MakeRoot)

void NCSSQListTreeWndExt::MakeRoot()
{
    MakeTreeRoot((BYTE*)this);
}
NW_HOOK_METHOD(0x100ECC20, NCSSQListTreeWndExt, MakeRoot)

// 0x100db610 (name ours) - the quest page: its slot 68 (the layout), then a new root in its tree window (+0xCC)
void NCQuestGaraTreeWndExt::ResetTree()
{
    nw::VirtualCall<void>(this, 68);
    ((NCQuestTreeWndExt*)nw::Field<void*>(this, 0xCC))->MakeRoot();
}
NW_HOOK_METHOD(0x100DB610, NCQuestGaraTreeWndExt, ResetTree)

/*-----------------------------------------------------------------------------
    The messenger's friend events (the console's slots 292, 294, 295, 296): a friend record (L2Friend: the name and three
    values, its small setters 0x101265A0 / 0x101265B0 / 0x101265C0 / SetName 0x101265D0 are the messenger's own and stay
    stock) is built and handed by value to the messenger window (the desktop's +0x18C).
-----------------------------------------------------------------------------*/

// L2Friend as the stock lays it out (name ours: 0x18 bytes)
struct NL2FriendRec
{
    FString Name;      // +0x00
    INT Unknown0C;     // +0x0C (set by 0x101265B0)
    INT Unknown10;     // +0x10 (set by 0x101265A0)
    INT Unknown14;     // +0x14 (set by 0x101265C0)

    // the three one-field setters (the stock ones are anonymous; the messenger's code calls them too)
    void SetUnknown10(INT Value);   // 0x101265A0 (ret 4)
    void SetUnknown0C(INT Value);   // 0x101265B0 (ret 4)
    void SetUnknown14(INT Value);   // 0x101265C0 (ret 4)
};
NW_ASSERT_SIZE(NL2FriendRec, 0x18);

// 0x101265a0 - the +0x10 value
void NL2FriendRec::SetUnknown10(INT Value)
{
    Unknown10 = Value;
}
NW_HOOK_METHOD(0x101265A0, NL2FriendRec, SetUnknown10)

// 0x101265b0 - the +0xC value
void NL2FriendRec::SetUnknown0C(INT Value)
{
    Unknown0C = Value;
}
NW_HOOK_METHOD(0x101265B0, NL2FriendRec, SetUnknown0C)

// 0x101265c0 - the +0x14 value
void NL2FriendRec::SetUnknown14(INT Value)
{
    Unknown14 = Value;
}
NW_HOOK_METHOD(0x101265C0, NL2FriendRec, SetUnknown14)

// the record goes to a function of the messenger window by value: L2Friend's copy constructor (0x10126500) builds the
// argument (which the callee destroys), the six words of it are the argument list
static FORCEINLINE void SendFriendToMessenger(NConsoleWnd* Console, void (NMsnShellWnd::*Function)(NL2FriendArg), NL2FriendRec& Rec)
{
    NL2FriendArg Copy;
    ::new ((void*)&Copy) L2Friend(*(const L2Friend*)&Rec);
    (((NMsnShellWnd*)Console->GetDesktop()->MessengerWnd)->*Function)(Copy);
}

// 0x1006b400 (C3 virtual 292) - a friend of the messenger (the +0x10 value, the name, the +0xC value, the +0x14 value) to the
// window's 0x10134610
void NConsoleWnd::OnMessengerFriendAdd(INT A, TCHAR* Name, INT B, INT C)
{
    NL2FriendRec Rec;
    Rec.SetUnknown10(A);
    ((L2Friend*)&Rec)->SetName(Name);
    Rec.SetUnknown0C(B);
    Rec.SetUnknown14(C);
    SendFriendToMessenger(this, &NMsnShellWnd::FriendAdd, Rec);      // 0x10134610
}
NW_HOOK_METHOD(0x1006B400, NConsoleWnd, OnMessengerFriendAdd)

// 0x1006b4b0 (C3 virtual 294) - the same with a kind first: 1 to the window's 0x10134610, 2 to 0x101346F0, 3 to 0x10134680
// (the other kinds only build the record)
void NConsoleWnd::OnMessengerFriendEvent(INT Kind, INT A, TCHAR* Name, INT B, INT C)
{
    NL2FriendRec Rec;
    Rec.SetUnknown10(A);
    ((L2Friend*)&Rec)->SetName(Name);
    Rec.SetUnknown0C(B);
    Rec.SetUnknown14(C);
    if (Kind == 1)
        SendFriendToMessenger(this, &NMsnShellWnd::FriendAdd, Rec);      // 0x10134610
    else if (Kind == 2)
        SendFriendToMessenger(this, &NMsnShellWnd::FriendUpdate, Rec);   // 0x101346F0
    else if (Kind == 3)
        SendFriendToMessenger(this, &NMsnShellWnd::FriendRemove, Rec);   // 0x10134680
}
NW_HOOK_METHOD(0x1006B4B0, NConsoleWnd, OnMessengerFriendEvent)

// 0x1006b5c0 (C3 virtual 295) - a friend's status (the +0xC value, the name, the +0x10 value) to the window's 0x101346F0
void NConsoleWnd::OnMessengerFriendStatus(INT B, TCHAR* Name, INT A)
{
    NL2FriendRec Rec;
    Rec.SetUnknown0C(B);
    ((L2Friend*)&Rec)->SetName(Name);
    Rec.SetUnknown10(A);
    SendFriendToMessenger(this, &NMsnShellWnd::FriendUpdate, Rec);   // 0x101346F0
}
NW_HOOK_METHOD(0x1006B5C0, NConsoleWnd, OnMessengerFriendStatus)

// 0x1006b660 (C3 virtual 296) - a message of a friend: the text is looked up for bad words (the stock does not read the
// answer), then both texts go to the messenger window's 0x10139680 when they are there
void NConsoleWnd::OnMessengerChat(INT A, TCHAR* Name, TCHAR* Text)
{
    HasBadWord(Text);   // 0x100665C0
    if (Name && Text)
        ((NMsnShellWnd*)GetDesktop()->MessengerWnd)->PanelChat(A, Name, Text);   // 0x10139680
}
NW_HOOK_METHOD(0x1006B660, NConsoleWnd, OnMessengerChat)

/*-----------------------------------------------------------------------------
    The messenger's friend list and the friends window's tree (the anonymous functions the events above end in). The classes
    are the messenger lanes'; what is declared here is what these functions read (names ours). The records are the L2Friend
    of NL2FriendRec.
-----------------------------------------------------------------------------*/


// Function holders that other lanes' files define: L2FriendWnd.cpp's friend book NL2Book (the friend list below is its view),
// ChatWnd.cpp's task list box and task window NChatTaskListBox / NChatL2TaskWnd: declared once, in MsnUiStock.h

static FORCEINLINE NL2FriendRec& NMsnFriendOf(NL2FriendArg& Arg)
{
    return *(NL2FriendRec*)&Arg;
}

// A string passed by value to a stock function that is only declared here (a naked stub cannot take an FString): its three words,
// built in raw memory so that nothing destroys it here - the callee does, as the stock caller expects
struct NStrArg
{
    DWORD W[3];
};
NW_ASSERT_SIZE(NStrArg, 0xC);

static FORCEINLINE NStrArg NMsnStrArg(const TCHAR* Text)
{
    BYTE Mem[sizeof(FString)];
    ::new ((void*)Mem) FString(Text);
    return *(NStrArg*)Mem;
}

// a string's copy for a callee that destroys its argument, made with FString's copy constructor (the words of NewChat's by-value
// strings)
static FORCEINLINE NStrArg NMsnStrCopy(const FString& Text)
{
    BYTE Mem[sizeof(FString)];
    ::new ((void*)Mem) FString(Text);
    return *(NStrArg*)Mem;
}

// the record's copy for a callee that destroys its argument: L2Friend's copy constructor (0x10126500), as the stock does
static FORCEINLINE void NMsnFriendCopy(NL2FriendArg& Copy, NL2FriendRec& Src)
{
    ::new ((void*)&Copy) L2Friend(*(const L2Friend*)&Src);
}

// The friend list: the records (+0x00), and a UE2 TMap (+0x0C) from the name to the record (TMapBase: the pairs of 0x14 bytes -
// the chain, the key, the value -, the hash and its size)
struct NMsnFriendPair
{
    INT HashNext;          // +0x00
    FString Name;          // +0x04
    NL2FriendRec* Rec;     // +0x10
};
NW_ASSERT_SIZE(NMsnFriendPair, 0x14);

struct NMsnFriendMap
{
    TArray<NMsnFriendPair> Pairs;   // +0x00
    INT* Hash;                      // +0x0C
    INT HashCount;                  // +0x10
};
NW_ASSERT_SIZE(NMsnFriendMap, 0x14);

// The friends are sorted with the online ones first (the +0xC value of the record, non zero), by name within each (appStrcmp)
// 0x1012c8b0 - the comparator appQsort gets (cdecl; the arguments point to the list's elements)
static INT __cdecl NMsnCompareFriends(const void* PA, const void* PB)
{
    NL2FriendRec* A = *(NL2FriendRec* const*)PA;
    NL2FriendRec* B = *(NL2FriendRec* const*)PB;
    if (A->Unknown0C != 0 && B->Unknown0C == 0)
        return -1;
    if (A->Unknown0C == 0 && B->Unknown0C != 0)
        return 1;
    return appStrcmp(*A->Name, *B->Name);
}
NW_HOOK_FUNC(0x1012C8B0, NMsnCompareFriends)

struct NMsnFriendList
{
    TArray<NL2FriendRec*> Friends;   // +0x00
    NMsnFriendMap Map;               // +0x0C

    __declspec(noinline) void Sort();     // 0x1012C9F0
    void Update(NL2FriendArg Arg);        // 0x1012CC30 (ret 0x18)
    void AddOrUpdate(NL2FriendArg Arg);   // 0x1012D1F0 (ret 0x18)
    void Remove(NL2FriendArg Arg);        // 0x1012D5A0 (ret 0x18)
    void Clear();                         // 0x1012D660
};
NW_ASSERT_SIZE(NMsnFriendList, 0x20);

// 0x1012c9f0 - the records sorted
void NMsnFriendList::Sort()
{
    appQsort(Friends.GetData(), Friends.Num(), sizeof(NL2FriendRec*), NMsnCompareFriends);
}
NW_HOOK_METHOD(0x1012C9F0, NMsnFriendList, Sort)

// 0x1012cc30 - the record of the same name (the list's Find, 0x1012CBD0, takes the name by value) takes the three values
void NMsnFriendList::Update(NL2FriendArg Arg)
{
    NL2FriendRec& Rec = NMsnFriendOf(Arg);
    NL2FriendRec* Found = (NL2FriendRec*)((NL2Book*)this)->Find(MsnStringCopy(Rec.Name));   // a copy, which Find destroys
    if (Found)
    {
        Found->SetUnknown0C(Rec.Unknown0C);
        Found->SetUnknown10(Rec.Unknown10);
        Found->SetUnknown14(Rec.Unknown14);
    }
    Rec.Name.~FString();
}
NW_HOOK_METHOD(0x1012CC30, NMsnFriendList, Update)

// 0x1012d1f0 - the same, and a name the list does not know is added: a new record made by L2Friend's copy constructor
// (0x10126500), the map told (0x101361B0: the name and the record's address) and the record appended
void NMsnFriendList::AddOrUpdate(NL2FriendArg Arg)
{
    NL2FriendRec& Rec = NMsnFriendOf(Arg);
    NL2FriendRec* Found = (NL2FriendRec*)((NMsnWndStrMap*)&Map)->FindWnd(&Rec.Name);   // 0x101347D0
    if (Found)
    {
        Found->SetUnknown0C(Rec.Unknown0C);
        Found->SetUnknown10(Rec.Unknown10);
        Found->SetUnknown14(Rec.Unknown14);
    }
    else
    {
        NL2FriendRec* New = (NL2FriendRec*)appMalloc(sizeof(NL2FriendRec), TEXT("new"));
        if (New)
            ::new ((void*)New) L2Friend(*(const L2Friend*)&Rec);
        ((NMsnWndStrMap*)&Map)->SetWnd(*Rec.Name, (NCWnd**)&New);   // 0x101361B0
        INT Index = Friends.Add();
        ((NL2FriendRec**)Friends.GetData())[Index] = New;
    }
    Rec.Name.~FString();
}
NW_HOOK_METHOD(0x1012D1F0, NMsnFriendList, AddOrUpdate)

// 0x1012d5a0 - the record of the same name (compared without case) goes (its name, then its memory), then the map forgets
// the name (0x101374E0)
void NMsnFriendList::Remove(NL2FriendArg Arg)
{
    NL2FriendRec& Rec = NMsnFriendOf(Arg);
    for (INT i = 0; i < Friends.Num(); i++)
    {
        NL2FriendRec* Other = Friends(i);
        if (Other && Other->Name == Rec.Name)
        {
            Other->Name.~FString();
            appFree(Other);
            Friends.Remove(i, 1);   // TPtrArray::Remove (0x100370D0, the template's out-of-line copy)
            break;
        }
    }
    ((NMsnStrMap*)&Map)->Remove(*Rec.Name);   // 0x101374E0
    Rec.Name.~FString();
}
NW_HOOK_METHOD(0x1012D5A0, NMsnFriendList, Remove)

// 0x1012d660 - every record and its memory go, the array emptied; the map's pairs lose their names, the pairs are emptied
// (of 0x14 bytes), the hash goes back to 8 entries and is rebuilt (0x1012CA90)
void NMsnFriendList::Clear()
{
    for (INT i = 0; i < Friends.Num(); i++)
    {
        NL2FriendRec* Other = Friends(i);
        if (Other)
        {
            Other->Name.~FString();
            appFree(Other);
        }
    }
    Friends.Empty();
    for (INT i = 0; i < Map.Pairs.Num(); i++)
        Map.Pairs(i).Name.~FString();
    Map.Pairs.FArray::Empty(sizeof(NMsnFriendPair));
    Map.HashCount = 8;
    ((NMsnStrMap*)&Map)->Rehash();   // 0x1012CA90
}
NW_HOOK_METHOD(0x1012D660, NMsnFriendList, Clear)

// Two stock tree walks (stdcall, one argument): the first child of an entry, else the entry that follows it among its
// parent's children (0x1002E600); the entry that follows it among its parent's children, NULL for the last (0x1002E5A0)
static NCTVItem* __stdcall NMsnFirstChildOrNext(NCTVItem* Item)
{
    return (NCTVItem*)((NMsnTreeNode*)Item)->GetNextNode((NMsnTreeNode*)Item);   // 0x1002E600 (the function ignores `this`)
}
static NCTVItem* __stdcall NMsnNextSibling(NCTVItem* Item)
{
    return (NCTVItem*)((NMsnTreeNode*)Item)->GetNextSibling((NMsnTreeNode*)Item);   // 0x1002E5A0 (the function ignores `this`)
}

// A copy of the friend list's array, made by the stock instance of the pointer array's copy constructor (0x10142780, ret 4)
// and unmade by its destructor (0x1012CD40, hooked as TPtrArray's)
struct NMsnRecordArray
{
    NL2FriendRec** Data;
    INT Num;
    INT Max;

    void CopyOf(const NMsnFriendList* List);   // 0x10142780
    void Destroy();                            // 0x1012CD40
};
// the pointer arrays' copy constructor (0x10142780: room for the other's count, then each pointer added at the end) and destructor
// (0x1012CD40, the instance of TArray<void*>): the template's own
void NMsnRecordArray::CopyOf(const NMsnFriendList* List)
{
    ::new ((void*)this) TArray<void*>(*(const TArray<void*>*)&List->Friends);
}
void NMsnRecordArray::Destroy()
{
    ((TArray<void*>*)this)->~TArray<void*>();
}

// The friends window of the messenger (the main panel's +0xF4): the friend list it shows (+0x114) and its tree view (+0x118;
// the root has two entries, the groups of the online (1) and the offline (0) friends, with the friends under them)
struct NMsnFriendsWnd : public NCWnd
{
    __declspec(noinline) void Rebuild();                         // 0x101437E0
    __declspec(noinline) void AddGroups();                       // 0x10143080
    __declspec(noinline) void AddFriendNode(DWORD W0, DWORD W1, DWORD W2, DWORD W3, DWORD W4, DWORD W5);   // 0x101431F0 (ret 0x18: the record
                                                                 //   by value, as six words, so that the hook entry is the function itself)
    __declspec(noinline) void Refresh();                         // 0x101436F0
    __declspec(noinline) NCTVItem* FindGroup(INT GroupID);       // 0x10142700 (ret 4)
    __declspec(noinline) void ResetRoot();                       // 0x10142740

    NMsnFriendList* List() { return nw::Field<NMsnFriendList*>(this, 0x114); }
    NCTree* Tree() { return nw::Field<NCTree*>(this, 0x118); }
};

// 0x10142740 - the tree starts again from an empty root
void NMsnFriendsWnd::ResetRoot()
{
    NTVItemData Data;
    Tree()->AddItem(&Data, NULL);
}
NW_HOOK_METHOD(0x10142740, NMsnFriendsWnd, ResetRoot)

// 0x10142700 - the group entry of the number (the +4 of its record), NULL when there is none
NCTVItem* NMsnFriendsWnd::FindGroup(INT GroupID)
{
    NCTVItem* Item = NMsnFirstChildOrNext(Tree()->Root);
    if (!Item)
        return NULL;
    while (Item->Data.Unknown04 != GroupID)
    {
        Item = NMsnNextSibling(Item);
        if (!Item)
            return NULL;
    }
    return Item;
}
NW_HOOK_METHOD(0x10142700, NMsnFriendsWnd, FindGroup)

// 0x10143080 - the two groups under the root (when the tree has one): the online ones (sysstring 1006, group 1) and the offline
// ones (1007, group 0), each with the plus and minus pictures of the quest tree and the black one as its second picture
void NMsnFriendsWnd::AddGroups()
{
    NTVItemData Data;
    UTexture* Plus = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndPlusBtn"));
    UTexture* Minus = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndMinusBtn"));
    Data.ExpandedTex = Minus;
    Data.CollapsedTex = Plus;
    Data.Icon2Tex = FindTexture(TEXT("NWindow.BlackTexture"));
    appStrcpy(Data.Text, *GL2GameData.SysString[1006]);
    Data.Unknown04 = 1;
    Data.Unknown2848 = 2;
    Data.bCenterOnIcon2 = 0;
    Data.Icon2Box.X = 0;
    Data.Icon2Box.Y = 0;
    Data.Icon2Box.W = 0.0f;
    Data.Icon2Box.H = 15.0f;
    Data.IconBox.X = 0;
    Data.IconBox.Y = 0;
    Data.IconBox.W = 15.0f;
    Data.IconBox.H = 15.0f;
    Data.TextBox.X = 0;
    Data.TextBox.Y = 0;
    Data.TextBox.W = 220.0f;
    Data.TextBox.H = 14.0f;
    if (Tree()->Root)
        Tree()->AddItem(&Data, Tree()->Root);
    appStrcpy(Data.Text, *GL2GameData.SysString[1007]);
    Data.Unknown04 = 0;
    if (Tree()->Root)
        Tree()->AddItem(&Data, Tree()->Root);
}
NW_HOOK_METHOD(0x10143080, NMsnFriendsWnd, AddGroups)

// 0x101431f0 (ret 0x18) - a friend under the group of its +0xC value (online or not): the name as the text, the +0x10 and +0xC
// values in the entry's +4 and +8, the text centered on the second picture (3 in the +0x2848 the tree uses)
void NMsnFriendsWnd::AddFriendNode(DWORD W0, DWORD W1, DWORD W2, DWORD W3, DWORD W4, DWORD W5)
{
    NL2FriendArg Arg;
    Arg.W[0] = W0;
    Arg.W[1] = W1;
    Arg.W[2] = W2;
    Arg.W[3] = W3;
    Arg.W[4] = W4;
    Arg.W[5] = W5;
    NL2FriendRec& Rec = NMsnFriendOf(Arg);
    NTVItemData Data;
    appStrcpy(Data.Text, *Rec.Name);
    Data.Unknown04 = Rec.Unknown10;
    Data.Unknown08 = Rec.Unknown0C;
    Data.Unknown2848 = 3;
    Data.bCenterOnIcon2 = 1;
    Data.Icon2Box.X = 0;
    Data.Icon2Box.Y = 0;
    Data.Icon2Box.W = 0.0f;
    Data.Icon2Box.H = 15.0f;
    Data.TextBox.X = 0;
    Data.TextBox.Y = 0;
    Data.TextBox.W = 220.0f;
    Data.TextBox.H = 14.0f;
    NCTVItem* Group = FindGroup(Rec.Unknown0C);
    if (Group)
        Tree()->AddItem(&Data, Group);
    Rec.Name.~FString();
}
NW_HOOK_METHOD(0x101431F0, NMsnFriendsWnd, AddFriendNode)

// 0x101436f0 - the friends of every group made again from the list: the group's entries deleted, then each record of a copy
// of the list that belongs to the group (by its +0xC value) added under it
void NMsnFriendsWnd::Refresh()
{
    NCTVItem* Group = NMsnFirstChildOrNext(Tree()->Root);
    while (Group)
    {
        Group->DeleteAllChildren();
        NMsnRecordArray Copy;
        Copy.CopyOf(List());
        for (INT i = 0; i < Copy.Num; i++)
        {
            NL2FriendRec* Rec = Copy.Data[i];
            if (Rec && Group == FindGroup(Rec->Unknown0C))
            {
                NL2FriendArg Arg;
                NMsnFriendCopy(Arg, *Rec);
                AddFriendNode(Arg.W[0], Arg.W[1], Arg.W[2], Arg.W[3], Arg.W[4], Arg.W[5]);
            }
        }
        Group = NMsnNextSibling(Group);
        Copy.Destroy();
    }
}
NW_HOOK_METHOD(0x101436F0, NMsnFriendsWnd, Refresh)

// 0x101437e0 - the tree built again: the window's slot 72, a new root, the two groups, every friend of a copy of the list under
// its group, and the root opened
void NMsnFriendsWnd::Rebuild()
{
    ::nw::VirtualCall<void>(this, 72);
    ResetRoot();
    AddGroups();
    NMsnRecordArray Copy;
    Copy.CopyOf(List());
    for (INT i = 0; i < Copy.Num; i++)
    {
        NL2FriendRec* Rec = Copy.Data[i];
        if (Rec)
        {
            NL2FriendArg Arg;
            NMsnFriendCopy(Arg, *Rec);
            AddFriendNode(Arg.W[0], Arg.W[1], Arg.W[2], Arg.W[3], Arg.W[4], Arg.W[5]);
        }
    }
    if (Tree()->Root)
        Tree()->Root->Expand();
    Copy.Destroy();
}
NW_HOOK_METHOD(0x101437E0, NMsnFriendsWnd, Rebuild)

// The windows of the chats. A chat window is kept by the friend's name in a map of pairs of 0x14 bytes (the chain, the name, the
// window); the windows of the chats that were closed are kept in a set (a map of 0xC byte pairs, the window as key and as value)
struct NMsnChatPair
{
    INT HashNext;      // +0x00
    FString Name;      // +0x04
    NCWnd* Wnd;        // +0x10
};
NW_ASSERT_SIZE(NMsnChatPair, 0x14);

struct NMsnChatMap
{
    TArray<NMsnChatPair> Pairs;   // +0x00
    INT* Hash;                    // +0x0C
    INT HashCount;                // +0x10
};
NW_ASSERT_SIZE(NMsnChatMap, 0x14);

struct NMsnWndPair
{
    INT HashNext;      // +0x00
    INT Key;           // +0x04 the window's address
    NCWnd* Wnd;        // +0x08
};
NW_ASSERT_SIZE(NMsnWndPair, 0xC);

struct NMsnWndSet
{
    TArray<NMsnWndPair> Pairs;    // +0x00
    INT* Hash;                    // +0x0C
    INT HashCount;                // +0x10

    NCWnd** Set(const INT* Key, NCWnd* const* Value);   // 0x10136230 (ret 8) the value under the key, made when there is none
};
NW_ASSERT_SIZE(NMsnWndSet, 0x14);
NCWnd** NMsnWndSet::Set(const INT* Key, NCWnd* const* Value)
{
    return ((NMsnWndIntMap*)this)->SetWnd(Key, (NCWnd**)Value);   // 0x10136230
}

// the chat window's text view (its +0xE4): a line added (0x101250C0, ret 0xC; not ported here)
struct NMsnChatView
{
    void AddLine(const TCHAR* Who, const TCHAR* Text, DWORD Color);
};
void NMsnChatView::AddLine(const TCHAR* Who, const TCHAR* Text, DWORD Color)
{
    ((L2ChatBox*)this)->AddMessage(Who, Text, Color);   // 0x101250C0
}

// A chat window: a reset (0x101242D0: its +0xD8 cleared, two of its parts told through their slots 71 and 70) and the lines
// the system code of a message adds to the view at +0xE4
struct NMsnChatWnd : public NCWnd
{
    void Reset();   // 0x101242D0
    __declspec(noinline) void OnCode(INT Code, DWORD A0, DWORD A1, DWORD A2, DWORD B0, DWORD B1, DWORD B2);   // 0x10125230 (ret 0x1C)
    void OnCodeStr(INT Code, DWORD A0, DWORD A1, DWORD A2, DWORD B0, DWORD B1, DWORD B2);   // the same entry, called with the texts as strings (their words)
};
void NMsnChatWnd::Reset()
{
    ((L2ChatWnd*)this)->ClearAll();   // 0x101242D0
}
void NMsnChatWnd::OnCodeStr(INT Code, DWORD A0, DWORD A1, DWORD A2, DWORD B0, DWORD B1, DWORD B2)
{
    OnCode(Code, A0, A1, A2, B0, B1, B2);   // 0x10125230: the same entry, the words are the strings (which OnCode destroys)
}

// 0x10125230 (ret 0x1C; the two texts by value, as six words, which the function destroys) - code 0 puts the two texts in
// the view (a line of the friend) in the normal color; 0x91, 0x93, 0x26C and 0x437 put a text of the game data
// (GL2GameData +0x4F47C, +0x4F41C, +0x4F4AC, +0x4F44C) in red (0xFFE37171) with no name; any other code adds nothing
void NMsnChatWnd::OnCode(INT Code, DWORD A0, DWORD A1, DWORD A2, DWORD B0, DWORD B1, DWORD B2)
{
    DWORD Words[6] = { A0, A1, A2, B0, B1, B2 };
    FString& TextA = *(FString*)&Words[0];
    FString& TextB = *(FString*)&Words[3];
    NMsnChatView* View = nw::Field<NMsnChatView*>(this, 0xE4);
    switch (Code)
    {
    case 0:
        View->AddLine(*TextA, *TextB, 0xFFDCDCDC);
        break;
    case 0x91:
        View->AddLine(NULL, *nw::Field<FString>(&GL2GameData, 0x4F47C), 0xFFE37171);
        break;
    case 0x93:
        View->AddLine(NULL, *nw::Field<FString>(&GL2GameData, 0x4F41C), 0xFFE37171);
        break;
    case 0x26C:
        View->AddLine(NULL, *nw::Field<FString>(&GL2GameData, 0x4F4AC), 0xFFE37171);
        break;
    case 0x437:
        View->AddLine(NULL, *nw::Field<FString>(&GL2GameData, 0x4F44C), 0xFFE37171);
        break;
    }
    TextA.~FString();
    TextB.~FString();
}
NW_HOOK_METHOD(0x10125230, NMsnChatWnd, OnCode)

// the layout object of the chat list (its +0xDC): a chat item taken out of it
struct NMsnChatLayout
{
    void Remove(NCWnd* Item);   // 0x10124FC0 (ret 4)
};
void NMsnChatLayout::Remove(NCWnd* Item)
{
    ((NChatTaskListBox*)this)->RemoveTask(Item);   // 0x10124FC0
}

// The chat list (the main panel's +0xF8): its items (a map of 0xC byte pairs at +0xC8, the item at +8 of each) and their layout
struct NMsnChatListWnd : public NCWnd
{
    __declspec(noinline) void DestroyItems();   // 0x101256A0
    void* FindItem(DWORD N0, DWORD N1, DWORD N2);   // 0x101255D0 (ret 0xC, not ported here) the item of a friend
};
void* NMsnChatListWnd::FindItem(DWORD N0, DWORD N1, DWORD N2)
{
    DWORD Words[3] = { N0, N1, N2 };
    FString& Name = *(FString*)Words;   // the caller's copy, which this function destroys (as the stock callee did)
    void* Found = ((L2TaskWnd*)this)->FindTask(Name);   // 0x101255D0: FindTask takes its own copy
    Name.~FString();
    return Found;
}

// 0x101256a0 - every item taken out of the layout, removed from the window and deleted (its scalar deleting destructor), the map
// emptied and its hash rebuilt (0x10124A60) with 8 entries
void NMsnChatListWnd::DestroyItems()
{
    NMsnWndSet& Map = *(NMsnWndSet*)((BYTE*)this + 0xC8);
    for (INT i = 0; i < Map.Pairs.Num(); i++)
    {
        NCWnd* Item = Map.Pairs(i).Wnd;
        if (Item)
        {
            nw::Field<NMsnChatLayout*>(this, 0xDC)->Remove(Item);
            RemoveChildWindow(Item->Entry);
            ::nw::VirtualCall<void>(Item, 3, 1);
        }
    }
    Map.Pairs.FArray::Empty(sizeof(NMsnWndPair));
    Map.HashCount = 8;
    ((NTaskMap*)&Map)->Rehash();   // 0x10124A60
}
NW_HOOK_METHOD(0x101256A0, NMsnChatListWnd, DestroyItems)

// The messenger's main panel (the window's +0xCC): the friend list (+0xC8), the chat windows by name (+0xCC) and the closed
// ones (+0xE0), the friends window (+0xF4) and the chat list (+0xF8)
struct NMsnMainPanel : public NCWnd
{
    void AddFriend(NL2FriendArg Arg);      // 0x10133E70 (ret 0x18)
    void RemoveFriend(NL2FriendArg Arg);   // 0x10133EF0 (ret 0x18)
    void UpdateFriend(NL2FriendArg Arg);   // 0x10133F70 (ret 0x18)
    __declspec(noinline) void ReloadFriends();   // 0x10131420
    __declspec(noinline) void CloseAll();        // 0x10139370
    __declspec(noinline) void HideAll();         // 0x10138B50 (name ours)
    void OnChat(INT A, TCHAR* Name, TCHAR* Text);      // 0x10139230 (ret 0xC)
    void NewChat(INT A, DWORD N0, DWORD N1, DWORD N2, DWORD T0, DWORD T1, DWORD T2);   // 0x10138F50 (ret 0x1C; the two texts by value, as words)

    NMsnFriendList* List() { return nw::Field<NMsnFriendList*>(this, 0xC8); }
    NMsnChatMap& Chats() { return *(NMsnChatMap*)((BYTE*)this + 0xCC); }
    NMsnWndSet& Parked() { return *(NMsnWndSet*)((BYTE*)this + 0xE0); }
    NMsnFriendsWnd* FriendsWnd() { return nw::Field<NMsnFriendsWnd*>(this, 0xF4); }
    NMsnChatListWnd* ChatList() { return nw::Field<NMsnChatListWnd*>(this, 0xF8); }
};

// every chat window reset and hidden and put in the closed set, the map of the chats emptied (the names destroyed, the hash
// rebuilt with 8 entries: 0x1012CA90): the first part of CloseAll (the stock has it inline in the panel's hide-all function too)
static FORCEINLINE void NMsnParkChats(NMsnMainPanel* Panel)
{
    NMsnChatMap& Chats = Panel->Chats();
    for (INT i = 0; i < Chats.Pairs.Num(); i++)
    {
        NCWnd* Wnd = Chats.Pairs(i).Wnd;
        if (Wnd)
        {
            NCWnd* Key = Wnd;
            NCWnd* Value = Wnd;
            ((NMsnChatWnd*)Wnd)->Reset();
            Panel->Parked().Set((const INT*)&Key, &Value);
            Wnd->HideWindow();
        }
    }
    for (INT i = 0; i < Chats.Pairs.Num(); i++)
        Chats.Pairs(i).Name.~FString();
    Chats.Pairs.FArray::Empty(sizeof(NMsnChatPair));
    Chats.HashCount = 8;
    ((NMsnStrMap*)&Chats)->Rehash();   // 0x1012CA90
}

// 0x10139370 - the chats parked and hidden, the chat list's items deleted and the list hidden, the friends window's slot 72
// and the friend list emptied
void NMsnMainPanel::CloseAll()
{
    NMsnParkChats(this);
    if (ChatList())
    {
        ChatList()->DestroyItems();
        ChatList()->HideWindow();
    }
    ::nw::VirtualCall<void>(FriendsWnd(), 72);
    List()->Clear();
}
NW_HOOK_METHOD(0x10139370, NMsnMainPanel, CloseAll)

// 0x10138b50 (name ours) - the messenger window's hide: the chats parked and hidden as CloseAll starts, every window of the closed
// set hidden (the ones parked before too), the chat list's items deleted and the list hidden. The friends window and the friend list
// stay as they are
void NMsnMainPanel::HideAll()
{
    NMsnParkChats(this);
    NMsnWndSet& Closed = Parked();
    for (INT i = 0; i < Closed.Pairs.Num(); i++)
    {
        NCWnd* Wnd = Closed.Pairs(i).Wnd;
        if (Wnd)
            Wnd->HideWindow();
    }
    if (ChatList())
    {
        ChatList()->DestroyItems();
        ChatList()->HideWindow();
    }
}
NW_HOOK_METHOD(0x10138b50, NMsnMainPanel, HideAll)

// The messenger window's first page, the MsnWnd (include/nwindow/MsnWnd.h is another lane's; its children by their offsets): the
// open chat windows by account (+0xCC, pairs of 0x14 bytes with the window at +0x10), the windows by a number (+0xF4, pairs of 0xC
// bytes with the window at +0x8), the Lineage II friends window (+0x160, may be none) and the twelve windows from +0x164 on
// (option, alarm, status, the dialogs: always there)
struct NMsnPageView : public NCWnd
{
    __declspec(noinline) void HideAll();   // 0x10136CA0 (name ours)
};

// 0x10136ca0 (name ours) - every child window of the messenger page hidden
void NMsnPageView::HideAll()
{
    NMsnChatMap& Chats = *(NMsnChatMap*)((BYTE*)this + 0xCC);
    for (INT i = 0; i < Chats.Pairs.Num(); i++)
    {
        NCWnd* Wnd = Chats.Pairs(i).Wnd;
        if (Wnd)
            Wnd->HideWindow();
    }
    NMsnWndSet& Numbered = *(NMsnWndSet*)((BYTE*)this + 0xF4);
    for (INT i = 0; i < Numbered.Pairs.Num(); i++)
    {
        NCWnd* Wnd = Numbered.Pairs(i).Wnd;
        if (Wnd)
            Wnd->HideWindow();
    }
    NCWnd* L2Friends = nw::Field<NCWnd*>(this, 0x160);
    if (L2Friends)
        L2Friends->HideWindow();
    for (DWORD Offset = 0x164; Offset <= 0x190; Offset += 4)
        nw::Field<NCWnd*>(this, Offset)->HideWindow();
}
NW_HOOK_METHOD(0x10136ca0, NMsnPageView, HideAll)

// The messenger window (CMessengerWnd, 0xD8 bytes): NMsnMessengerView, declared in NCConsole.h (the desktop calls HideAll)

// 0x10139650 (name ours; NCConsole::ShowWindow's) - both pages' children hidden
void NMsnMessengerView::HideAll()
{
    nw::Field<NMsnPageView*>(this, 0xC8)->HideAll();
    nw::Field<NMsnMainPanel*>(this, 0xCC)->HideAll();
}
NW_HOOK_METHOD(0x10139650, NMsnMessengerView, HideAll)

// 0x1013bb30 (name ours; NCConsole's +0x18C, the desktop's logout) - the sign-out button of the MSN page (MsnWnd::OnSignOutButton,
// 0x1013AA20), whose answer comes back
INT NMsnMessengerView::SignOut()
{
    return nw::Field<MsnWnd*>(this, 0xC8)->OnSignOutButton();   // 0x1013AA20
}
NW_HOOK_METHOD(0x1013bb30, NMsnMessengerView, SignOut)

// 0x10139230 (ret 0xC) - a message of a friend (A, the friend's name, the text): the chat window kept under the name (the map's
// Find, 0x101347D0) shows it through its OnCode (the code A with the name and the text); when that window is hidden and the
// chat list has the friend's item, the item is marked (+0xD8 = 1). A friend without a chat window gets one (0x10138F50, with the
// same three values)
void NMsnMainPanel::OnChat(INT A, TCHAR* Name, TCHAR* Text)
{
    NMsnChatWnd* Chat;
    {
        FString Key(Name);
        Chat = (NMsnChatWnd*)((NMsnWndStrMap*)&Chats())->FindWnd(&Key);   // 0x101347D0
    }
    if (Chat)
    {
        NStrArg TextArg = NMsnStrArg(Text);
        NStrArg NameArg = NMsnStrArg(Name);
        Chat->OnCodeStr(A, NameArg.W[0], NameArg.W[1], NameArg.W[2], TextArg.W[0], TextArg.W[1], TextArg.W[2]);
        if (!Chat->IsShowWindow() && ChatList())
        {
            NStrArg ItemName = NMsnStrArg(Name);
            void* Item = ChatList()->FindItem(ItemName.W[0], ItemName.W[1], ItemName.W[2]);
            if (Item)
                nw::Field<INT>(Item, 0xD8) = 1;
        }
    }
    else
    {
        NStrArg TextArg = NMsnStrArg(Text);
        NStrArg NameArg = NMsnStrArg(Name);
        NewChat(A, NameArg.W[0], NameArg.W[1], NameArg.W[2], TextArg.W[0], TextArg.W[1], TextArg.W[2]);
    }
}
NW_HOOK_METHOD(0x10139230, NMsnMainPanel, OnChat)

// 0x10138f50 (ret 0x1C; the name and the text by value, as six words, which the function destroys) - a message (code A) of a friend
// whose chat window is not entered by his name yet. The task window is made and shown first. A friend without a window gets one:
// a free one of the number table, or a new L2ChatWnd (256 x 401 at a random place, a child of the desktop); it is entered by the name,
// titled, given the name, marked (+0xD8 = 1) and left HIDDEN, and the task window lists it. A friend with a window has it titled
// and given the name again, marked, and when it is not shown the task window's entry of the friend is marked too. Then the
// window's OnCode shows the message
void NMsnMainPanel::NewChat(INT A, DWORD N0, DWORD N1, DWORD N2, DWORD T0, DWORD T1, DWORD T2)
{
    DWORD Words[6] = { N0, N1, N2, T0, T1, T2 };
    FString& Name = *(FString*)&Words[0];
    FString& Text = *(FString*)&Words[3];
    ((L2FriendWnd*)this)->ShowTaskWnd();   // 0x101360C0
    NCWnd* Chat;
    {
        FString Key(*Name);
        Chat = ((NMsnWndStrMap*)&Chats())->FindWnd(&Key);   // the table of names (0x101347D0)
    }
    if (!Chat)
    {
        Chat = ((L2FriendWnd*)this)->FindFreeChatWnd();   // 0x101373F0
        if (!Chat)
        {
            Chat = new L2ChatWnd(this, Name);   // class 0x102C1318, constructor 0x10125B20 (the name by value: a copy)
            INT X = appRand() % 200 + 200;
            INT Y = appRand() % 150 + 150;
            Chat->SetPosition(X, Y, 0x100, 0x191);
            GConsole->Desktop->CreateChild(Chat, 0x2002);
        }
        ((NMsnWndStrMap*)&Chats())->SetWnd(*Name, &Chat);   // entered by the name (0x101361B0)
        Chat->SetTitle(*nw::Field<FString>(&GL2GameData, 0x68948));
        ((L2ChatWnd*)Chat)->SetFriendName(Name);   // the chat window keeps the name (0x10131650, a copy)
        nw::Field<INT>(Chat, 0xD8) = 1;
        Chat->HideWindow();
        if (ChatList())
        {
            ((NChatL2TaskWnd*)ChatList())->AddTask(Chat, Name, 1);   // the task window lists it (0x10126240, a copy of the name)
        }
    }
    else
    {
        Chat->SetTitle(*nw::Field<FString>(&GL2GameData, 0x68948));
        ((L2ChatWnd*)Chat)->SetFriendName(Name);   // 0x10131650 (a copy)
        nw::Field<INT>(Chat, 0xD8) = 1;
        if (!Chat->IsShowWindow() && ChatList())
        {
            NStrArg Listed = NMsnStrCopy(Name);
            void* Item = ChatList()->FindItem(Listed.W[0], Listed.W[1], Listed.W[2]);
            if (Item)
                nw::Field<INT>(Item, 0xD8) = 1;
        }
    }
    NStrArg NameArg = NMsnStrCopy(Name);
    NStrArg TextArg = NMsnStrCopy(Text);
    ((NMsnChatWnd*)Chat)->OnCodeStr(A, NameArg.W[0], NameArg.W[1], NameArg.W[2], TextArg.W[0], TextArg.W[1], TextArg.W[2]);
    Name.~FString();
    Text.~FString();
}
NW_HOOK_METHOD(0x10138f50, NMsnMainPanel, NewChat)


// 0x10133e70 - the friend (a copy, which the list destroys) into the list, the list sorted, the friends window's tree made
// again; 0x10133ef0 the same with the friend taken out of the list; 0x10133f70 with its values updated
void NMsnMainPanel::AddFriend(NL2FriendArg Arg)
{
    NL2FriendArg Copy;
    NMsnFriendCopy(Copy, NMsnFriendOf(Arg));
    List()->AddOrUpdate(Copy);
    List()->Sort();
    FriendsWnd()->Refresh();
    NMsnFriendOf(Arg).Name.~FString();
}
NW_HOOK_METHOD(0x10133E70, NMsnMainPanel, AddFriend)

void NMsnMainPanel::RemoveFriend(NL2FriendArg Arg)
{
    NL2FriendArg Copy;
    NMsnFriendCopy(Copy, NMsnFriendOf(Arg));
    List()->Remove(Copy);
    List()->Sort();
    FriendsWnd()->Refresh();
    NMsnFriendOf(Arg).Name.~FString();
}
NW_HOOK_METHOD(0x10133EF0, NMsnMainPanel, RemoveFriend)

void NMsnMainPanel::UpdateFriend(NL2FriendArg Arg)
{
    NL2FriendArg Copy;
    NMsnFriendCopy(Copy, NMsnFriendOf(Arg));
    List()->Update(Copy);
    List()->Sort();
    FriendsWnd()->Refresh();
    NMsnFriendOf(Arg).Name.~FString();
}
NW_HOOK_METHOD(0x10133F70, NMsnMainPanel, UpdateFriend)

// 0x10131420 - the friends window shows the panel's list, sorted, and builds its tree (a tail jump into Rebuild)
void NMsnMainPanel::ReloadFriends()
{
    nw::Field<NMsnFriendList*>(FriendsWnd(), 0x114) = List();
    List()->Sort();
    FriendsWnd()->Rebuild();
}
NW_HOOK_METHOD(0x10131420, NMsnMainPanel, ReloadFriends)

// the object at +0xC8 of the shell's child at +0xC8, that holds a name: its text is given through 0x1012A700 (Msn::GetStatus: ret 4,
// the string by the hidden pointer)
struct NMsnNameHolder
{
    FString* GetText(FString* Result);
};
FString* NMsnNameHolder::GetText(FString* Result)
{
    ::new ((void*)Result) FString(((Msn*)this)->GetStatus());   // 0x1012A700 (ret 4, the string by the hidden pointer)
    return Result;
}


// 0x10134610 / 0x10134680 / 0x101346f0 - the friend (a copy, which the panel destroys) to the main panel
void NMsnShellWnd::FriendAdd(NL2FriendArg Arg)
{
    NL2FriendArg Copy;
    NMsnFriendCopy(Copy, NMsnFriendOf(Arg));
    MainPanel()->AddFriend(Copy);
    NMsnFriendOf(Arg).Name.~FString();
}
NW_HOOK_METHOD(0x10134610, NMsnShellWnd, FriendAdd)

void NMsnShellWnd::FriendRemove(NL2FriendArg Arg)
{
    NL2FriendArg Copy;
    NMsnFriendCopy(Copy, NMsnFriendOf(Arg));
    MainPanel()->RemoveFriend(Copy);
    NMsnFriendOf(Arg).Name.~FString();
}
NW_HOOK_METHOD(0x10134680, NMsnShellWnd, FriendRemove)

void NMsnShellWnd::FriendUpdate(NL2FriendArg Arg)
{
    NL2FriendArg Copy;
    NMsnFriendCopy(Copy, NMsnFriendOf(Arg));
    MainPanel()->UpdateFriend(Copy);
    NMsnFriendOf(Arg).Name.~FString();
}
NW_HOOK_METHOD(0x101346F0, NMsnShellWnd, FriendUpdate)

// 0x101345d0 (ret 4; the string through the hidden pointer, which it returns) - the name the shell's first child holds: its
// holder's text, empty when there is no holder (NCChatWnd::OnPaint draws it)
FString* NMsnShellWnd::GetPeerName(FString* Result)
{
    NMsnNameHolder* Holder = nw::Field<NMsnNameHolder*>(nw::Field<void*>(this, 0xC8), 0xC8);
    if (Holder)
    {
        Holder->GetText(Result);
        return Result;
    }
    ::new ((void*)Result) FString(TEXT(""));
    return Result;
}
NW_HOOK_METHOD(0x101345D0, NMsnShellWnd, GetPeerName)

// 0x10131530 - the main panel's ReloadFriends (a tail jump); 0x10139670 its 0x10139370; 0x10139680 its 0x10139230 with the
// three arguments
void NMsnShellWnd::ReloadFriends()
{
    MainPanel()->ReloadFriends();
}
NW_HOOK_METHOD(0x10131530, NMsnShellWnd, ReloadFriends)

void NMsnShellWnd::PanelCall139370()
{
    MainPanel()->CloseAll();
}
NW_HOOK_METHOD(0x10139670, NMsnShellWnd, PanelCall139370)


void NMsnShellWnd::PanelChat(INT A, TCHAR* Name, TCHAR* Text)
{
    MainPanel()->OnChat(A, Name, Text);
}
NW_HOOK_METHOD(0x10139680, NMsnShellWnd, PanelChat)
