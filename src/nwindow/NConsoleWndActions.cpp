/*=============================================================================
    NConsoleWndActions.cpp - what the console does for the keyboard and the pages: the
    shortcut slots and the actions of the action window used from the keys, the console
    commands of the chat, the HTML pages' "bypass" commands, and the actors under the mouse
    (an item on the ground, a pawn).
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleWndExt.h"
#include "nwindow/NCActionItemWnd.h"
#include "nwindow/NCChatListBox.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCMatchWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCShortCutWnd.h"

#include <wchar.h>

// the dialog boxes a shortcut item closes (NDialogInfo::ID, the inventory's)
enum { DIALOG_DROP_ITEM = 4, DIALOG_DESTROY_ITEM = 5, DIALOG_CRYSTALLIZE_ITEM = 6 };

// the actor under the mouse
static FORCEINLINE AActor* HoveredActor(UEngine* Engine)
{
    return static_cast<UGameEngine*>(Engine)->MouseHitActor;
}

// a system message line in the chat window's first list
static FORCEINLINE void ChatSystemMessage(NConsoleWnd* Console, INT MsgID)
{
    Console->GetDesktop()->ChatWnd->Lists[CHATLIST_All]->AddString(
        *GL2GameData.SystemMsg[MsgID].Message, 0xFFB09B79, -1);
}

// 0x10066470 - a console command (the chat's "/" commands the client does not know) run by
// the first viewport, its output to the log
void NConsoleWnd::ExecCommand(TCHAR* Command)
{
    UClient* Client = Engine->Client;
    if (Client && Client->Viewports.Num())
        static_cast<FExec*>(Client->Viewports(0))->Exec(Command, *GLog);
}
NW_HOOK_METHOD(0x10066470, NConsoleWnd, ExecCommand)

// 0x1005f5c0 - the item on the ground under the mouse, NULL for anything else
AActor* NConsoleWnd::GetPickupUnderMouse()
{
    if (!Engine)
        return NULL;
    AActor* Actor = HoveredActor(Engine);
    if (Actor && Actor->IsA(AL2Pickup::StaticClass()))
        return Actor;
    return NULL;
}
NW_HOOK_METHOD(0x1005f5c0, NConsoleWnd, GetPickupUnderMouse)

// 0x10062770
APawn* CastToPawn(UObject* Object)
{
    if (Object && Object->IsA(APawn::StaticClass()))
        return (APawn*)Object;
    return NULL;
}
NW_HOOK_FUNC(0x10062770, CastToPawn)

// the dialog boxes of the shortcut items (NDialogInfo::ID; names ours): an item with a warning (it asks first)
// and an item with a message of its own (NItemEntry::GetUnknown19AC)
enum { DIALOG_CONFIRM_ITEM = 0x21, DIALOG_ITEM_MESSAGE = 0x45 };

// 0x10074ab0 - an action used from a shortcut key (UseShortCut) or the action window: the requests of the
// action, the social motions by the action table's parameter; the others go to the server with Ctrl (force) and
// Shift (don't move). C3: the invitation sends the player's NAME (not the creature id), the party matching (11)
// is a request of its own and two combo boxes of the window set to the first line, and 50 is new (a request of the
// selected player's name: sub_10061EA0, with message 1400 when there is none)
void NConsoleWnd::UseAction(INT ActionID, INT bForce, INT bShift)
{
    ::nw::TraceCall("NConsoleWnd::UseAction", this, ActionID, bForce, bShift);
    switch (ActionID)
    {
    case ACTION_Attack:
    {
        User* Target = GetSelectedActor();
        User* Me = GetMyUser();
        if (Me && Target)
            RequestAttack(Target->ObjectID, Me->Pawn->Location, bForce, bShift);
        break;
    }
    case ACTION_Exchange:
    {
        User* Target = GetSelectedActor();
        if (Target && Target->Pawn)
            RequestTrade(Target->Pawn->CreatureID);
        break;
    }
    case ACTION_NextTarget:
    {
        User* Target = GetSelectedActor();
        if (!Target || !Target->Pawn)
            RequestTargetNext(200, -1);
        else
            RequestTargetNext(200, Target->Pawn->CreatureID);
        break;
    }
    case ACTION_PickUp:
        RequestPickUp(200, -1);
        break;
    case ACTION_PetPickUp:
        PetPickUp(200, -1);
        break;
    case ACTION_Assist:
        RequestAssist();
        break;
    case ACTION_Invite:
    {
        User* Target = GetSelectedActor();
        if (!Target || Target->Unknown8)
            ChatSystemMessage(this, 185);   // "Select user to invite to your party."
        else
        {
            L2ParamStack Params(10);
            Params.PushBack((BYTE*)Target + 0x1C);                          // the name
            Params.PushBack((void*)(INT_PTR)nw::Field<INT>(this, 0x39C8));  // the party item distribution
            RequestPartyInvite(Params);
        }
        break;
    }
    case ACTION_LeaveParty:
    {
        L2ParamStack Params(10);
        RequestPartyLeave(Params);
        break;
    }
    case ACTION_Dismiss:
    {
        User* Target = GetSelectedActor();
        if (!Target || Target->Unknown8)
            ChatSystemMessage(this, 187);   // "Select user to expel."
        else
        {
            L2ParamStack Params(10);
            Params.PushBack(Target->GetName());
            RequestPartyDismiss(Params);
        }
        break;
    }
    case ACTION_PrivateStoreSell:
        RequestVendor();
        break;
    case ACTION_PrivateStoreBuy:
        RequestBuy();
        break;
    case ACTION_PartyMatching:
    {
        // the list of the party matching: everyone (1), any level (-1), any place (1); then the window's two
        // selection boxes go to the first line
        L2ParamStack Params(10);
        Params.PushBack((void*)1);
        Params.PushBack((void*)-1);
        Params.PushBack((void*)1);
        static_cast<NConsoleForwardFns*>(this)->RequestListPartyWating(Params);
        NCMatchWnd* Match = GConsole->GetDesktop()->MatchWnd;
        nw::Field<NCComboBox*>(Match, 0xCC)->SetSelected(1);
        Match = GConsole->GetDesktop()->MatchWnd;
        nw::Field<NCComboBox*>(Match, 0xD0)->SetSelected(1);
        break;
    }
    case 0x25:
        RequestRecipeShopManageList();
        break;
    case ACTION_Greeting:
    case ACTION_Victory:
    case ACTION_Advance:
    case ACTION_SocialYes:
    case ACTION_SocialNo:
    case ACTION_SocialBow:
    case ACTION_SocialUnaware:
    case ACTION_SocialWaiting:
    case ACTION_SocialLaugh:
    case 0x21:
    case 0x22:
    case 0x23:
        if (ActionID > 0 && ActionID < GL2GameData.ActionTable.Num())
        {
            INT Social = *(INT*)((BYTE*)GL2GameData.ActionTable(ActionID) + 8);   // the social action of the entry
            if (Social > 0)
                RequestSocialAction(Social);
        }
        break;
    case 0x28:
    {
        // the recommendation of a player (UL2NetHandler vslot 259, +0x40C); then, like any other, the action itself
        User* Target = GetSelectedActor();
        if (!Target)
            ShowSystemMessage(0xF2, 0xFFB09B79);
        else if (Target->Unknown8)
            ShowSystemMessage(0x90, 0xFFB09B79);
        else if (Engine && Engine->NetHandler)
            nw::VirtualCall<void>(Engine->NetHandler, 0x40C / 4, Target->Pawn->CreatureID);
        goto Request;
    }
    case 0x32:
    {
        User* Target = GetSelectedActor();
        if (!Target || Target->Unknown8)
            ChatSystemMessage(this, 1400);
        else
        {
            L2ParamStack Params(10);
            Params.PushBack((BYTE*)Target + 0x1C);                          // the name
            static_cast<NConsoleForwardFns*>(this)->RequestHandOverPartyMaster(Params);
        }
        break;
    }
    default:
    Request:
    {
        if (nw::VirtualCall<UBOOL>(this, 297))                              // Ctrl (IsCtrlDown)
            bForce = nw::VirtualCall<UBOOL>(this, 297);
        if (Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift))
            bShift = Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift);
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)ActionID);
        Params.PushBack((void*)(INT_PTR)bForce);
        Params.PushBack((void*)(INT_PTR)bShift);
        RequestActionUse(Params);
        break;
    }
    }
}
NW_HOOK_METHOD(0x10074ab0, NConsoleWnd, UseAction)

// 0x10076020 - a slot of the shortcut bar used from a key (HookShortCutKey, UseShortCutSlot): a skill with Ctrl
// (force) and Shift (don't move), an action, a macro (0x1006b040) or an item of the player's inventory or the pet's
// (an item with a warning, or with a message of its own, asks first). C3: the slot is an NItemEntry whose Type is at
// +0x1910 and the warning flag at +0x199C (the C2 +0x190C, +0x1998); the warning only counts for the player's
// dwarven classes (sub_1006B6C0); no map items; the key no longer waits again after a use. The skill request is the
// function at 0x1005D040 (RequestSkillList by the C2 map's names)
void NConsoleWnd::UseShortCut(INT Slot, INT bForce, INT bShift)
{
    NShortCutItem* Item = GetDesktop()->ShortCutWnd->ItemAt(Slot);
    if (!Item)
        return;
    if (nw::VirtualCall<UBOOL>(this, 297))                          // Ctrl (IsCtrlDown)
        bForce = nw::VirtualCall<UBOOL>(this, 297);
    if (Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift))
        bShift = Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift);
    INT Type = nw::Field<INT>(Item, 0x1910);
    if (Type == SHORTCUT_Item)
    {
        if (nw::Field<INT>(Item, 0x199C) == 0 || !static_cast<NConsoleLeftFns*>(this)->IsDwarf())
        {
            if (Item->GetUnknown19AC() > 0)
            {
                NDialogInfo Info;
                Info.ID = DIALOG_ITEM_MESSAGE;
                Info.Owner = GetDesktop()->ShortCutWnd->Entry;
                Info.bMainBox = 1;
                Info.bInput = 0;
                Info.SetSystemMessage(Item->GetUnknown19AC());
                Info.bLargeAmount = 0;
                Info.SetParams(Item->ObjectID, -1);
                GConsole->ShowDialog(&Info);
            }
            else if (Item->Owner == 1)
                RequestUseItem(Item->ObjectID);
            else if (Item->Owner == 2)
                RequestPetUseItem(Item->ObjectID);
        }
        else
        {
            TCHAR Name[64];
            appSprintf(Name, TEXT("%s"), Item->NItemEntry::GetName());   // the entry's own, not NShortCutItem's C1 one
            NDialogInfo Info;
            Info.ID = DIALOG_CONFIRM_ITEM;
            Info.Owner = GetDesktop()->ShortCutWnd->Entry;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.SetSystemMessage(0x31E);
            Info.SetButtons(Name, NULL);
            Info.bLargeAmount = 0;
            Info.SetParams(Item->ObjectID, -1);
            GConsole->ShowDialog(&Info);
        }
    }
    else if (Type == SHORTCUT_Skill)
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Item->ID);
        Params.PushBack((void*)(INT_PTR)bForce);
        Params.PushBack((void*)(INT_PTR)bShift);
        RequestSkillList(Params);
    }
    else if (Type == SHORTCUT_Action)
        UseAction(Item->ID, bForce, bShift);
    else if (Type == SHORTCUT_Macro)
        StartMacro(Item->ID);
}
NW_HOOK_METHOD(0x10076020, NConsoleWnd, UseShortCut)

// 0x10065b90 (vslot 402) - an HTML page's command: "bypass [-h] ..." sends its tokens
// ("$name": the value of the page's control of that name) joined by spaces to the server
// (the tutorial's pages their own way); the result is 2 when an "-h" came first (the page
// closes), else 1, and 0 for a bypass without tokens (nothing sent). Any other command is
// the desktop's.
INT NConsoleWnd::RunHtmlCommand(TCHAR* Command, NCHtmlViewer* Viewer)
{
    INT Result = 0;
    const TCHAR* Str = Command;
    if (!ParseCommand(&Str, TEXT("BYPASS")))
        return GetDesktop()->OnHtmlCommand(Command, Viewer);
    TCHAR Bypass[1024] = {0};
    L2ParamStack Params(10);
    TCHAR Token[256];
    if (ParseToken(Str, Token, 256, 1))
    {
        do
        {
            if (Result != 2 && !wcscmp(Token, TEXT("-h")))
                Result = 2;
            else if (Token[0] == '$')
            {
                if (Viewer)
                {
                    // virtual 72 of the viewer (+0x120): GetControlValue by slot, as the stock does
                    TCHAR* Value = nw::VirtualCall<TCHAR*>(Viewer, 0x120 / 4, &Token[1]);
                    if (Value)
                    {
                        if (!Bypass[0])
                            appSprintf(Bypass, TEXT("%s"), Value);
                        else
                            appSprintf(Bypass, TEXT("%s %s"), Bypass, Value);
                    }
                }
            }
            else if (!Bypass[0])
                appSprintf(Bypass, TEXT("%s"), Token);
            else
                appSprintf(Bypass, TEXT("%s %s"), Bypass, Token);
        } while (ParseToken(Str, Token, 256, 1));
        Params.PushBack(Bypass);
        if (Viewer->IsA(NClass::NCTutorialHtmlViewer()))
            RequestTutorialPassCmdToServer(Params);
        else
            RequestBypassToServer(Params);
        if (!Result)
            Result = 1;
    }
    return Result;
}
NW_HOOK_METHOD(0x10065b90, NConsoleWnd, RunHtmlCommand)
