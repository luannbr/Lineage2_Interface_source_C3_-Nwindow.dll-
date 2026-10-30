/*=============================================================================
    NConsoleWndNet.cpp - the console's requests to the server, and what they act on (the
    player, its target, what the network handler knows). Almost all of them only pass their
    arguments on to the engine's network handler (Engine->NetHandler, a UL2NetHandler whose 228
    virtuals are pure in Engine.dll and have no names yet: the slot is given at each call).
    Within a family the slots follow the protocol's packet order (e.g. 195..201 are the private
    store packets 0x73..0x79), which names the requests no chat command names.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleWndExt.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCLoadingWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCRestartMenuWnd.h"
#include "nwindow/NCItemWnd.h"

// interface sounds (FL2NetNotify::OnInterfacePlaySound; names ours)
enum { SOUND_UseShortcut = 1, SOUND_PickUp = 3 };

// the pawn the player's controller drives, and the actor under the mouse
static FORCEINLINE APawn* PlayerPawn(UEngine* Engine)
{
    return Engine->Client->Viewports(0)->Actor->Pawn;
}
static FORCEINLINE AActor* HoveredActor(UEngine* Engine)
{
    return static_cast<UGameEngine*>(Engine)->MouseHitActor;
}

// (GetMyLocation, 0x10057dd0 in C2, is in NConsoleWndMisc.cpp)

// 0x1005a890 - a chat line (NetHandler slot 146: type, text, whisper target); with
// GIsOnlyL2Console (no server) it is only shown in the chat window. Returns 0.
UBOOL NConsoleWnd::SendChatMsgToServer(INT Type, TCHAR* Text, TCHAR* Target)
{
    guard(NConsoleWnd::SendChatMsgToServer);
    UL2NetHandler* NetHandler = Engine->NetHandler;
    if (GIsOnlyL2Console)
        static_cast<NCConsole*>(Desktop)->ChatWnd->AddChatMessage(Text, 0xFFDCDCDC, 0, 0);
    else
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Type);
        Params.PushBack(Text);
        Params.PushBack(Target);
        NetHandler->Say2(Params);
    }
    return 0;
    unguard;
}
NW_HOOK_METHOD(0x1005a890, NConsoleWnd, SendChatMsgToServer)

// 0x1005a980 - NetHandler slot 68
UBOOL NConsoleWnd::RequestAuthLogin(const TCHAR* Account, const TCHAR* Password)
{
    guard(NConsoleWnd::RequestAuthLogin);
    return Engine->NetHandler->RequestAuthLogin((TCHAR*)Account, (TCHAR*)Password) != 0;
    unguard;
}
NW_HOOK_METHOD(0x1005a980, NConsoleWnd, RequestAuthLogin)

// 0x1005aaa0 - NetHandler slot 147
void NConsoleWnd::RequestLogout()
{
    ::nw::TraceCall("NConsoleWnd::RequestLogout", this);
    Engine->NetHandler->SendLogOutPacket();
}
NW_HOOK_METHOD(0x1005aaa0, NConsoleWnd, RequestLogout)

// 0x1005aab0 - NetHandler +0x134 (RequestGameStart); when sent, the loading screen replaces the character selection
// until the server answers (AcceptGameStart). C3: the console's message window group is the one at +0x3950 (an
// NCGodWnd: its +0xD0 is the message line, +0xCC the restart menu) and the loading texture id is at +0x39C4
UBOOL NConsoleWnd::RequestGameStart(INT Index)
{
    guard(NConsoleWnd::RequestGameStart);
    if (!Engine->NetHandler->RequestGameStart(Index))
        return 0;
    LobbyWnd->HideWindow();
    nw::Field<NCWnd*>(nw::Field<NCWnd*>(this, 0x3950), 0xD0)->HideWindow();
    LoadingWnd->SetLoadingTexture(nw::Field<INT>(this, 0x39C4));
    LoadingWnd->ShowWindow();
    DialogBox->HideWindow();
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1005aab0, NConsoleWnd, RequestGameStart)

// 0x1005abb0 - NetHandler slot 72
UBOOL NConsoleWnd::RequestCharacterDelete(INT Index)
{
    guard(NConsoleWnd::RequestCharacterDelete);
    if (!Engine->NetHandler->RequestCharacterDelete(Index))
        return 0;
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1005abb0, NConsoleWnd, RequestCharacterDelete)

// 0x1005ac60 - NetHandler slot 73
UBOOL NConsoleWnd::RequestCharacterRestore(INT Index)
{
    guard(NConsoleWnd::RequestCharacterRestore);
    if (!Engine->NetHandler->RequestCharacterRestore(Index))
        return 0;
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1005ac60, NConsoleWnd, RequestCharacterRestore)

// 0x1005ad10 - NetHandler slot 74
void NConsoleWnd::RequestNewCharacter()
{
    guard(NConsoleWnd::RequestNewCharacter);
    Engine->NetHandler->RequestNewCharacter();
    unguard;
}
NW_HOOK_METHOD(0x1005ad10, NConsoleWnd, RequestNewCharacter)

// 0x1005ad90 - NetHandler slot 75 (the creation's choices, by value: C2's record is 0x60 bytes, ret 0x60)
UBOOL NConsoleWnd::RequestCharacterCreate(CharacterInfo Info)
{
    guard(NConsoleWnd::RequestCharacterCreate);
    if (!Engine->NetHandler->RequestCharacterCreate(Info))
        return 0;
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1005ad90, NConsoleWnd, RequestCharacterCreate)

// 0x1005ae50 - NetHandler slot 159
void NConsoleWnd::RequestRestart(L2ParamStack& Params)
{
    ::nw::TraceCall("NConsoleWnd::RequestRestart", this, (int)&Params);
    Engine->NetHandler->RequestRestart(Params);
}
NW_HOOK_METHOD(0x1005ae50, NConsoleWnd, RequestRestart)

// 0x1005ae60 - NetHandler slot 160
void NConsoleWnd::RequestRestartPoint(L2ParamStack& Params)
{
    Engine->NetHandler->RequestRestartPoint(Params);
}
NW_HOOK_METHOD(0x1005ae60, NConsoleWnd, RequestRestartPoint)

// 0x1005afc0 - NetHandler slot 70
void NConsoleWnd::RequestServerLogin(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestServerLogin);
    Engine->NetHandler->RequestServerLogin(Params);
    unguard;
}
NW_HOOK_METHOD(0x1005afc0, NConsoleWnd, RequestServerLogin)

// 0x1005bb40 - NetHandler slot 77
void NConsoleWnd::RequestItemList()
{
    guard(NConsoleWnd::RequestItemList);
    Engine->NetHandler->RequestItemList();
    unguard;
}
NW_HOOK_METHOD(0x1005bb40, NConsoleWnd, RequestItemList)

// 0x1005bbc0 - NetHandler +0x170 (RequestUseItem(L2ParamStack&)): the object id and whether Ctrl is held (the
// virtual 297 of this client); what it returns is returned
INT NConsoleWnd::RequestUseItem(INT ObjectID)
{
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)ObjectID);
    Params.PushBack((void*)(INT_PTR)nw::VirtualCall<UBOOL>(this, 297));
    return Engine->NetHandler->RequestUseItem(Params);
}
NW_HOOK_METHOD(0x1005bbc0, NConsoleWnd, RequestUseItem)

// 0x1005bc80 - NetHandler slot 153
void NConsoleWnd::RequestUnEquipItem(L2ParamStack& Params)
{
    Engine->NetHandler->RequestUnEquipItem(Params);
}
NW_HOOK_METHOD(0x1005bc80, NConsoleWnd, RequestUnEquipItem)

// 0x1005bfc0 - NetHandler slot 82
void NConsoleWnd::RequestTrade(INT CreatureID)
{
    guard(NConsoleWnd::RequestTrade);
    Engine->NetHandler->RequestTrade(CreatureID);
    unguard;
}
NW_HOOK_METHOD(0x1005bfc0, NConsoleWnd, RequestTrade)

// 0x1005c040 - NetHandler slot 83 (by value)
void NConsoleWnd::RequestAddTradeItem(NItemRequest Request)
{
    guard(NConsoleWnd::RequestAddTradeItem);
    Engine->NetHandler->RequestAddTradeItem(Request);
    unguard;
}
NW_HOOK_METHOD(0x1005c040, NConsoleWnd, RequestAddTradeItem)

// 0x1005c0d0 - NetHandler slot 84
void NConsoleWnd::RequestTradeDone(INT bConfirm)
{
    guard(NConsoleWnd::RequestTradeDone);
    Engine->NetHandler->RequestTradeDone(bConfirm);
    unguard;
}
NW_HOOK_METHOD(0x1005c0d0, NConsoleWnd, RequestTradeDone)

// 0x1005c200 - NetHandler slot 90 (C2 only: the manor's seed purchase; the fmap paired the C1 RequestBuyItem with it)
void NConsoleWnd::RequestBuySeed(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestBuySeed);
    Engine->NetHandler->RequestBuySeed(Params);
    unguard;
}
NW_HOOK_METHOD(0x1005c200, NConsoleWnd, RequestBuySeed)

// 0x1005c280 - NetHandler slot 91 (C2 only: the manor's crop procurement)
void NConsoleWnd::RequestProcureCrop(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestProcureCrop);
    Engine->NetHandler->RequestProcureCrop(Params);
    unguard;
}
NW_HOOK_METHOD(0x1005c280, NConsoleWnd, RequestProcureCrop)

// 0x1005c450 - NetHandler slot 89
void NConsoleWnd::RequestBuyItem(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestBuyItem);
    Engine->NetHandler->RequestBuyItem(Params);
    unguard;
}
NW_HOOK_METHOD(0x1005c450, NConsoleWnd, RequestBuyItem)

// 0x1005c4d0 - NetHandler slot 88
void NConsoleWnd::RequestSellItem(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestSellItem);
    Engine->NetHandler->RequestSellItem(Params);
    unguard;
}
NW_HOOK_METHOD(0x1005c4d0, NConsoleWnd, RequestSellItem)

// 0x1005c810 - NetHandler slot 138: the warehouse window's list in its deposit mode
void NConsoleWnd::RequestWarehouseDeposit(L2ParamStack& Params)
{
    Engine->NetHandler->SendWareHouseDepositList(Params);
}
NW_HOOK_METHOD(0x1005c810, NConsoleWnd, RequestWarehouseDeposit)

// 0x1005c820 - NetHandler slot 139: the same in its withdraw mode
void NConsoleWnd::RequestWarehouseWithdraw(L2ParamStack& Params)
{
    Engine->NetHandler->SendWareHouseWithdrawList(Params);
}
NW_HOOK_METHOD(0x1005c820, NConsoleWnd, RequestWarehouseWithdraw)

// 0x1005d010 - NetHandler slot 145 (C2 only: macros; names ours, after the slot's)
void NConsoleWnd::RequestMakeMacro(L2ParamStack& Params)
{
    Engine->NetHandler->RequestMakeMacro(Params);
}
NW_HOOK_METHOD(0x1005d010, NConsoleWnd, RequestMakeMacro)

// 0x1005d020 - NetHandler +0x25C (C2 only: macros). The C3 engine's slots moved (the vtable names of the header do not
// follow the order of this client's network handler): called by offset, as the stock tail jump does
void NConsoleWnd::RequestDeleteMacro(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x25C / 4, &Params);
}
NW_HOOK_METHOD(0x1005d020, NConsoleWnd, RequestDeleteMacro)

// 0x1005d030 - NetHandler +0x258
void NConsoleWnd::RequestMagicSkillUse(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x258 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d030, NConsoleWnd, RequestMagicSkillUse)

// 0x1005d040 - NetHandler +0x268
void NConsoleWnd::RequestSkillList(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x268 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d040, NConsoleWnd, RequestSkillList)

// 0x1005d090 - NetHandler slot 199 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeBookOpen()
{
    Engine->NetHandler->RequestRecipeBookOpen();
}
NW_HOOK_METHOD(0x1005d090, NConsoleWnd, RequestRecipeBookOpen)

// 0x1005d0a0 - NetHandler slot 200 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeItemDelete(INT P1)
{
    Engine->NetHandler->RequestRecipeItemDelete(P1);
}
NW_HOOK_METHOD(0x1005d0a0, NConsoleWnd, RequestRecipeItemDelete)

// 0x1005d0b0 - NetHandler slot 201 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeItemMakeInfo(INT P1)
{
    Engine->NetHandler->RequestRecipeItemMakeInfo(P1);
}
NW_HOOK_METHOD(0x1005d0b0, NConsoleWnd, RequestRecipeItemMakeInfo)

// 0x1005d0c0 - NetHandler slot 202 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeItemMakeSelf(INT P1)
{
    Engine->NetHandler->RequestRecipeItemMakeSelf(P1);
}
NW_HOOK_METHOD(0x1005d0c0, NConsoleWnd, RequestRecipeItemMakeSelf)

// 0x1005d0d0 - NetHandler slot 203 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeShopManageList()
{
    Engine->NetHandler->RequestRecipeShopManageList();
}
NW_HOOK_METHOD(0x1005d0d0, NConsoleWnd, RequestRecipeShopManageList)

// 0x1005d0e0 - NetHandler slot 204 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeShopMessageSet(TCHAR* P1)
{
    Engine->NetHandler->RequestRecipeShopMessageSet(P1);
}
NW_HOOK_METHOD(0x1005d0e0, NConsoleWnd, RequestRecipeShopMessageSet)

// 0x1005d0f0 - NetHandler slot 205 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeShopListSet(L2ParamStack& P1)
{
    Engine->NetHandler->RequestRecipeShopListSet(P1);
}
NW_HOOK_METHOD(0x1005d0f0, NConsoleWnd, RequestRecipeShopListSet)

// 0x1005d100 - NetHandler slot 206 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeShopManageQuit()
{
    Engine->NetHandler->RequestRecipeShopManageQuit();
}
NW_HOOK_METHOD(0x1005d100, NConsoleWnd, RequestRecipeShopManageQuit)

// 0x1005d110 - NetHandler slot 207 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeShopManageCancel()
{
    Engine->NetHandler->RequestRecipeShopManageCancel();
}
NW_HOOK_METHOD(0x1005d110, NConsoleWnd, RequestRecipeShopManageCancel)

// 0x1005d120 - NetHandler slot 208 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeShopSellList(INT P1)
{
    Engine->NetHandler->RequestRecipeShopSellList(P1);
}
NW_HOOK_METHOD(0x1005d120, NConsoleWnd, RequestRecipeShopSellList)

// 0x1005d130 - NetHandler slot 209 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeShopMakeInfo(INT P1, INT P2)
{
    Engine->NetHandler->RequestRecipeShopMakeInfo(P1, P2);
}
NW_HOOK_METHOD(0x1005d130, NConsoleWnd, RequestRecipeShopMakeInfo)

// 0x1005d140 - NetHandler +0x35C (C2 only; name ours, after the slot's). C3: three values (ret 0xC)
void NConsoleWnd::RequestRecipeShopMakeDo(INT P1, INT P2, INT P3)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x35C / 4, P1, P2, P3);
}
NW_HOOK_METHOD(0x1005d140, NConsoleWnd, RequestRecipeShopMakeDo)

// 0x1005d190 - NetHandler slot 211 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestHennaItemInfo(INT P1)
{
    Engine->NetHandler->RequestHennaItemInfo(P1);
}
NW_HOOK_METHOD(0x1005d190, NConsoleWnd, RequestHennaItemInfo)

// 0x1005d1a0 - NetHandler slot 212 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestHennaEquip(INT P1)
{
    Engine->NetHandler->RequestHennaEquip(P1);
}
NW_HOOK_METHOD(0x1005d1a0, NConsoleWnd, RequestHennaEquip)

// 0x1005d1b0 - NetHandler slot 213 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestHennaUnequipInfo(INT P1)
{
    Engine->NetHandler->RequestHennaUnequipInfo(P1);
}
NW_HOOK_METHOD(0x1005d1b0, NConsoleWnd, RequestHennaUnequipInfo)

// 0x1005d1c0 - NetHandler slot 214 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestHennaUnequip(INT P1)
{
    Engine->NetHandler->RequestHennaUnequip(P1);
}
NW_HOOK_METHOD(0x1005d1c0, NConsoleWnd, RequestHennaUnequip)

// 0x10070070 - NetHandler slot 142, then the pickup sound (the icon was dropped on the bar)
void NConsoleWnd::RequestShortCutReg(L2ParamStack& Params)
{
    Engine->NetHandler->RequestShortCutReg(Params);
    PlayInterfaceSound(SOUND_PickUp);
}
NW_HOOK_METHOD(0x10070070, NConsoleWnd, RequestShortCutReg)

// C1 only (the request of a used shortcut): the C3 image has UseShortCut(Slot, ...) (0x10076020) that sends it itself; an
// inert body
void NConsoleWnd::UseShortcut(L2ParamStack& Params)
{
}

// 0x1005d280 - NetHandler slot 142
void NConsoleWnd::RequestShortCutDel(L2ParamStack& Params)
{
    Engine->NetHandler->RequestShortCutDel(Params);
}
NW_HOOK_METHOD(0x1005d280, NConsoleWnd, RequestShortCutDel)

// 0x1005d390 - NetHandler slot 143
void NConsoleWnd::RequestTargetCancel(L2ParamStack& Params)
{
    ::nw::TraceCall("NConsoleWnd::RequestTargetCancel", this, (int)&Params);
    Engine->NetHandler->RequestTargetCancel(Params);
}
NW_HOOK_METHOD(0x1005d390, NConsoleWnd, RequestTargetCancel)

// 0x1005d3a0 - NetHandler slot 157
void NConsoleWnd::RequestActionUse(L2ParamStack& Params)
{
    ::nw::TraceCall("NConsoleWnd::RequestActionUse", this, (int)&Params);
    Engine->NetHandler->RequestActionUse(Params);
}
NW_HOOK_METHOD(0x1005d3a0, NConsoleWnd, RequestActionUse)

// 0x1005d3b0 - NetHandler slot 52 (target, origin, stand still): 0
void NConsoleWnd::RequestAttackForce(INT ObjectID, FVector Origin)
{
    Engine->NetHandler->Attack(ObjectID, Origin, 0);
}
NW_HOOK_METHOD(0x1005d3b0, NConsoleWnd, RequestAttackForce)

// RequestAttackStand: a C2 function (0x10054710 there) with no counterpart in this client; the map paired it with
// RequestAttack's address, whose arguments are not these (24 bytes popped): never called, never hooked
// (standalone: no body, nothing calls it)
#if NW_HOOK_MODE
NW_NAKED void NConsoleWnd::RequestAttackStand(INT ObjectID, FVector Origin) NW_STOCK(0x10065250)
#endif

// 0x10065250 - attack with Ctrl held (NetHandler +0xDC), else select (+0xE0). Two more words than C1: the callers push
// them after the position; either is replaced by the key's state when Ctrl / Shift is down. C3: nothing happens while
// the engine's +0x17C flag is set (RequestTargetNext sets it), and Ctrl is the virtual 297 of this client
void NConsoleWnd::RequestAttack(INT ObjectID, FVector Origin, INT bForceAttack, INT bShift)
{
    if (!nw::Field<INT>(Engine, 0x17C))
    {
        if (nw::VirtualCall<UBOOL>(this, 297))
            bForceAttack = nw::VirtualCall<UBOOL>(this, 297);
        if (Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift))
            bShift = Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift);
        if (bForceAttack)
            Engine->NetHandler->Attack(ObjectID, Origin, bShift);
        else
            Engine->NetHandler->Action(ObjectID, Origin, bShift & 0xFF);
    }
}
NW_HOOK_METHOD(0x10065250, NConsoleWnd, RequestAttack)

// 0x1005d3f0 - NetHandler slot 57
void NConsoleWnd::RequestSocialAction(INT Action)
{
    Engine->NetHandler->SocialAction(Action);
}
NW_HOOK_METHOD(0x1005d3f0, NConsoleWnd, RequestSocialAction)

// 0x1005d400 - NetHandler slot 59
void NConsoleWnd::RequestChangeWaitType(UBOOL bStand)
{
    ::nw::TraceCall("NConsoleWnd::RequestChangeWaitType", this, bStand);
    Engine->NetHandler->ChangeWaitType(bStand);
}
NW_HOOK_METHOD(0x1005d400, NConsoleWnd, RequestChangeWaitType)

// 0x1005d410 - NetHandler slot 58
void NConsoleWnd::RequestChangeMoveType(UBOOL bRun)
{
    Engine->NetHandler->ChangeMoveType(bRun);
}
NW_HOOK_METHOD(0x1005d410, NConsoleWnd, RequestChangeMoveType)

// 0x1005d4b0 - NetHandler +0x1C0 (the clan requests: the C3 engine's slots of this family are not in the header's order). A tail
// jump, so the arguments are the target's: Engine.dll's UNetworkHandler slot 112 is RequestCreatePledge(L2ParamStack&) (the name of
// this wrapper is C2's; NCGaraDialogBox's dialog 0xB calls it with the clan's name)
void NConsoleWnd::RequestPledgeMemberList(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1C0 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d4b0, NConsoleWnd, RequestPledgeMemberList)

// 0x1005d4c0 - NetHandler +0x1C4 (dialog 0xB opens it: the clan's creation)
void NConsoleWnd::RequestNet103(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1C4 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d4c0, NConsoleWnd, RequestNet103)

// 0x1005d4d0 - NetHandler +0x1C8
void NConsoleWnd::RequestAnswerJoinPledge(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1C8 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d4d0, NConsoleWnd, RequestAnswerJoinPledge)

// 0x1005d4e0 - NetHandler +0x1CC: a tail jump into Engine.dll's slot 115, RequestDismissPledge() (no argument; NCGaraDialogBox's
// dialog 0xC calls it)
void NConsoleWnd::RequestOustPledgeMember()
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1CC / 4);
}
NW_HOOK_METHOD(0x1005d4e0, NConsoleWnd, RequestOustPledgeMember)

// 0x1005d4f0 - NetHandler +0x1D8: a tail jump into Engine.dll's slot 118, RequestStartPledgeWar(L2ParamStack&) (NCGaraDialogBox's
// dialog 0x12 calls it with the clan's name)
void NConsoleWnd::RequestNet106(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1D8 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d4f0, NConsoleWnd, RequestNet106)

// 0x1005d500 - NetHandler +0x1DC
void NConsoleWnd::RequestStartPledgeWar(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1DC / 4, &Params);
}
NW_HOOK_METHOD(0x1005d500, NConsoleWnd, RequestStartPledgeWar)

// 0x1005d510 - NetHandler +0x1E0
void NConsoleWnd::RequestReplyStartPledgeWar(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1E0 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d510, NConsoleWnd, RequestReplyStartPledgeWar)

// 0x1005d520 - NetHandler +0x1E4
void NConsoleWnd::RequestStopPledgeWar(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1E4 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d520, NConsoleWnd, RequestStopPledgeWar)

// 0x1005d530 - NetHandler +0x1E8
void NConsoleWnd::RequestReplyStopPledgeWar(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1E8 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d530, NConsoleWnd, RequestReplyStopPledgeWar)

// 0x1005d540 - NetHandler +0x1F0
void NConsoleWnd::RequestSurrenderPledgeWar(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1F0 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d540, NConsoleWnd, RequestSurrenderPledgeWar)

// 0x1005d550 - NetHandler +0x1F4
void NConsoleWnd::RequestSurrenderPersonally(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1F4 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d550, NConsoleWnd, RequestSurrenderPersonally)

// 0x1005d560 - NetHandler +0x1D0 (the member's title)
void NConsoleWnd::RequestGiveNickName(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1D0 / 4, &Params);
}
NW_HOOK_METHOD(0x1005d560, NConsoleWnd, RequestGiveNickName)

// 0x1005d570 - NetHandler +0x1D4: a tail jump into Engine.dll's slot 117, RequestWithDrawalPledge() (no argument; NCGaraDialogBox's
// dialog 0x1A, leaving the clan, calls it)
void NConsoleWnd::RequestJoinPledge()
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1D4 / 4);
}
NW_HOOK_METHOD(0x1005d570, NConsoleWnd, RequestJoinPledge)

// C1 only: no function of this name in the C3 image (the pledge window sends it through its own handler); an inert
// body
void NConsoleWnd::RequestWithdrawalPledge()
{
}

// 0x1005d750 - NetHandler slot 119
void NConsoleWnd::RequestAllyInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestJoinAlly(Params);
}
NW_HOOK_METHOD(0x1005d750, NConsoleWnd, RequestAllyInvite)

// 0x1005d760 - NetHandler slot 120
void NConsoleWnd::RequestAnswerJoinAlly(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAnswerJoinAlly(Params);
}
NW_HOOK_METHOD(0x1005d760, NConsoleWnd, RequestAnswerJoinAlly)

// 0x1005d770 - NetHandler slot 121
void NConsoleWnd::RequestOustAlly(L2ParamStack& Params)
{
    Engine->NetHandler->RequestOustAlly(Params);
}
NW_HOOK_METHOD(0x1005d770, NConsoleWnd, RequestOustAlly)

// 0x1005d780 - NetHandler slot 124
void NConsoleWnd::RequestAllyInfo()
{
    Engine->NetHandler->RequestAllyInfo();
}
NW_HOOK_METHOD(0x1005d780, NConsoleWnd, RequestAllyInfo)

// 0x1005d790 - NetHandler slot 122
void NConsoleWnd::RequestWithdrawAlly()
{
    Engine->NetHandler->RequestWithdrawAlly();
}
NW_HOOK_METHOD(0x1005d790, NConsoleWnd, RequestWithdrawAlly)

// 0x1005d7a0 - NetHandler slot 123
void NConsoleWnd::RequestDismissAlly()
{
    Engine->NetHandler->RequestDismissAlly();
}
NW_HOOK_METHOD(0x1005d7a0, NConsoleWnd, RequestDismissAlly)

// 0x1005d950 - NetHandler slot 125
void NConsoleWnd::RequestFriendInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestFriendInvite(Params);
}
NW_HOOK_METHOD(0x1005d950, NConsoleWnd, RequestFriendInvite)

// 0x1005d960 - NetHandler slot 126
void NConsoleWnd::RequestFriendDel(L2ParamStack& Params)
{
    Engine->NetHandler->RequestFriendDel(Params);
}
NW_HOOK_METHOD(0x1005d960, NConsoleWnd, RequestFriendDel)

// 0x1005d970 - NetHandler slot 128
void NConsoleWnd::RequestAnswerFriendInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestFriendAddReply(Params);
}
NW_HOOK_METHOD(0x1005d970, NConsoleWnd, RequestAnswerFriendInvite)

// 0x1005d980 - NetHandler slot 127
void NConsoleWnd::RequestFriendList()
{
    Engine->NetHandler->RequestFriendList();
}
NW_HOOK_METHOD(0x1005d980, NConsoleWnd, RequestFriendList)

// 0x1005d990 - NetHandler slot 129
void NConsoleWnd::RequestBlock(L2ParamStack& Params)
{
    Engine->NetHandler->RequestBlock(Params);
}
NW_HOOK_METHOD(0x1005d990, NConsoleWnd, RequestBlock)

// 0x1005da40 - NetHandler slot 130
void NConsoleWnd::RequestPartyInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestJoinParty(Params);
}
NW_HOOK_METHOD(0x1005da40, NConsoleWnd, RequestPartyInvite)

// 0x1005da50 - NetHandler slot 131
void NConsoleWnd::RequestAnswerJoinParty(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAnswerJoinParty(Params);
}
NW_HOOK_METHOD(0x1005da50, NConsoleWnd, RequestAnswerJoinParty)

// 0x1005da60 - NetHandler slot 132
void NConsoleWnd::RequestPartyLeave(L2ParamStack& Params)
{
    Engine->NetHandler->RequestWithDrawalParty(Params);
}
NW_HOOK_METHOD(0x1005da60, NConsoleWnd, RequestPartyLeave)

// 0x1005da70 - NetHandler slot 133
void NConsoleWnd::RequestPartyDismiss(L2ParamStack& Params)
{
    Engine->NetHandler->RequestOustPartyMember(Params);
}
NW_HOOK_METHOD(0x1005da70, NConsoleWnd, RequestPartyDismiss)

// The three C1 party matching requests (the configuration, the list, a detail): no such functions in the C3 image (the
// matching windows were redone); inert bodies
void NConsoleWnd::RequestPartyMatchConfig(L2ParamStack& Params)
{
}

void NConsoleWnd::RequestPartyMatchList(INT Status)
{
}

void NConsoleWnd::RequestPartyMatchDetail(INT ObjectID)
{
}

// 0x1005ddb0 - NetHandler slot 91
void NConsoleWnd::RequestLinkHtml(L2ParamStack& Params)
{
    Engine->NetHandler->RequestLinkHtml(Params);
}
NW_HOOK_METHOD(0x1005ddb0, NConsoleWnd, RequestLinkHtml)

// 0x1005ddc0 - NetHandler slot 93
void NConsoleWnd::RequestBypassToServer(L2ParamStack& Params)
{
    Engine->NetHandler->RequestBypassToServer(Params);
}
NW_HOOK_METHOD(0x1005ddc0, NConsoleWnd, RequestBypassToServer)

// 0x1005ddd0 - NetHandler slot 92
void NConsoleWnd::RequestBBSWrite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestBBSWrite(Params);
}
NW_HOOK_METHOD(0x1005ddd0, NConsoleWnd, RequestBBSWrite)

// 0x1005dde0 - NetHandler slot 163
void NConsoleWnd::RequestShowBoard(L2ParamStack& Params)
{
    Engine->NetHandler->RequestShowboard(Params);
}
NW_HOOK_METHOD(0x1005dde0, NConsoleWnd, RequestShowBoard)

// 0x1005df10 - NetHandler slot 94
void NConsoleWnd::RequestTutorialLinkHtml(L2ParamStack& Params)
{
    Engine->NetHandler->RequestTutorialLinkHtml(Params);
}
NW_HOOK_METHOD(0x1005df10, NConsoleWnd, RequestTutorialLinkHtml)

// 0x1005df20 - NetHandler slot 95
void NConsoleWnd::RequestTutorialPassCmdToServer(L2ParamStack& Params)
{
    Engine->NetHandler->RequestTutorialPassCmdToServer(Params);
}
NW_HOOK_METHOD(0x1005df20, NConsoleWnd, RequestTutorialPassCmdToServer)

// 0x1005df30 - NetHandler slot 96
void NConsoleWnd::RequestTutorialQuestionMark(INT ID)
{
    Engine->NetHandler->RequestTutorialQuestionMarkPressed(ID);
}
NW_HOOK_METHOD(0x1005df30, NConsoleWnd, RequestTutorialQuestionMark)

// 0x1005dfa0 - the item nearest to the pet within Range (NetHandler slot 210, a float) is
// picked up by it (slot 189 with the item's object id, its first field). The stock also tests
// the address of the item's field +0x10 for NULL, which it never is.
void NConsoleWnd::PetPickUp(INT Range, INT Param)
{
    Item* Found = Engine->NetHandler->GetPetNearestItem((FLOAT)Range, Param);
    if (Found)
        Engine->NetHandler->RequestPetGetItem(Found->ObjectID);
}
NW_HOOK_METHOD(0x1005dfa0, NConsoleWnd, PetPickUp)

// 0x1005dfe0 - NetHandler slot 190
void NConsoleWnd::RequestPetUseItem(INT ObjectID)
{
    Engine->NetHandler->RequestPetUseItem(ObjectID);
}
NW_HOOK_METHOD(0x1005dfe0, NConsoleWnd, RequestPetUseItem)

// 0x1005dff0 - NetHandler slot 191
void NConsoleWnd::RequestGiveItemToPet(L2ParamStack& Params)
{
    Engine->NetHandler->RequestGiveItemToPet(Params);
}
NW_HOOK_METHOD(0x1005dff0, NConsoleWnd, RequestGiveItemToPet)

// 0x1005e000 - NetHandler slot 192
void NConsoleWnd::RequestGetItemFromPet(L2ParamStack& Params)
{
    Engine->NetHandler->RequestGetItemFromPet(Params);
}
NW_HOOK_METHOD(0x1005e000, NConsoleWnd, RequestGetItemFromPet)

// 0x1005e010 - NetHandler slot 194
void NConsoleWnd::RequestChangePetName(TCHAR* Name)
{
    Engine->NetHandler->RequestChangePetName(Name);
}
NW_HOOK_METHOD(0x1005e010, NConsoleWnd, RequestChangePetName)

// 0x1005e130 - NetHandler slot 78 (C2: a second argument, passed on)
void NConsoleWnd::RequestPetition(TCHAR* Text, INT Type)
{
    Engine->NetHandler->RequestPetition(Text, Type);
}
NW_HOOK_METHOD(0x1005e130, NConsoleWnd, RequestPetition)

// 0x1005e140 - NetHandler slot 79
void NConsoleWnd::RequestPetitionCancel()
{
    Engine->NetHandler->RequestPetitionCancel();
}
NW_HOOK_METHOD(0x1005e140, NConsoleWnd, RequestPetitionCancel)

// 0x1005e150 - NetHandler slot 80
void NConsoleWnd::RequestGMList()
{
    Engine->NetHandler->RequestGMList();
}
NW_HOOK_METHOD(0x1005e150, NConsoleWnd, RequestGMList)

// The GM window's queries about a character: NetHandler slot 81 (name, what).

// 0x1005e160
void NConsoleWnd::RequestGMInventory(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 5);
}
NW_HOOK_METHOD(0x1005e160, NConsoleWnd, RequestGMInventory)

// 0x1005e180
void NConsoleWnd::RequestGMSkills(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 3);
}
NW_HOOK_METHOD(0x1005e180, NConsoleWnd, RequestGMSkills)

// 0x1005e1a0
void NConsoleWnd::RequestGMStatus(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 1);
}
NW_HOOK_METHOD(0x1005e1a0, NConsoleWnd, RequestGMStatus)

// 0x1005e1c0
void NConsoleWnd::RequestGMQuests(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 4);
}
NW_HOOK_METHOD(0x1005e1c0, NConsoleWnd, RequestGMQuests)

// 0x1005e1e0
void NConsoleWnd::RequestGMWarehouse(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 6);
}
NW_HOOK_METHOD(0x1005e1e0, NConsoleWnd, RequestGMWarehouse)

// 0x1005e200
void NConsoleWnd::RequestGMPledge(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 2);
}
NW_HOOK_METHOD(0x1005e200, NConsoleWnd, RequestGMPledge)

// The private stores: selling (NetHandler slots 195..201, packets 0x73..0x79) and buying
// (202..208, packets 0x90..0x96).

// 0x1005e880 - "/vendor": manage the selling store
void NConsoleWnd::RequestVendor()
{
    Engine->NetHandler->RequestPrivateStoreManage();
}
NW_HOOK_METHOD(0x1005e880, NConsoleWnd, RequestVendor)

// 0x1005e890 - the managed selling list (item, amount, price...)
void NConsoleWnd::RequestSetPrivateStoreListSell(L2ParamStack& Params)
{
    Engine->NetHandler->SetPrivateStoreList(Params);
}
NW_HOOK_METHOD(0x1005e890, NConsoleWnd, RequestSetPrivateStoreListSell)

// 0x1005e8a0 - the managing window closed without opening the store
void NConsoleWnd::RequestPrivateStoreManageCancel()
{
    Engine->NetHandler->RequestPrivateStoreManageCancel();
}
NW_HOOK_METHOD(0x1005e8a0, NConsoleWnd, RequestPrivateStoreManageCancel)

// 0x1005e8b0
void NConsoleWnd::RequestPrivateStoreQuitSell()
{
    Engine->NetHandler->RequestPrivateStoreQuit();
}
NW_HOOK_METHOD(0x1005e8b0, NConsoleWnd, RequestPrivateStoreQuitSell)

// 0x1005e8c0
void NConsoleWnd::SetPrivateStoreMsg(TCHAR* Text)
{
    Engine->NetHandler->SetPrivateStoreMsg(Text);
}
NW_HOOK_METHOD(0x1005e8c0, NConsoleWnd, SetPrivateStoreMsg)

// 0x1005e8d0 - buying from another player's store
void NConsoleWnd::RequestPrivateStoreBuy(L2ParamStack& Params)
{
    Engine->NetHandler->SendPrivateStoreBuyList(Params);
}
NW_HOOK_METHOD(0x1005e8d0, NConsoleWnd, RequestPrivateStoreBuy)

// 0x1005e910 - "/buy": manage the buying store
void NConsoleWnd::RequestBuy()
{
    Engine->NetHandler->RequestPrivateStoreBuyManage();
}
NW_HOOK_METHOD(0x1005e910, NConsoleWnd, RequestBuy)

// 0x1005e920
void NConsoleWnd::RequestSetPrivateStoreListBuy(L2ParamStack& Params)
{
    Engine->NetHandler->SetPrivateStoreBuyList(Params);
}
NW_HOOK_METHOD(0x1005e920, NConsoleWnd, RequestSetPrivateStoreListBuy)

// 0x1005e930
void NConsoleWnd::RequestPrivateStoreBuyManageCancel()
{
    Engine->NetHandler->RequestPrivateStoreBuyManageCancel();
}
NW_HOOK_METHOD(0x1005e930, NConsoleWnd, RequestPrivateStoreBuyManageCancel)

// 0x1005e940
void NConsoleWnd::RequestPrivateStoreQuitBuy()
{
    Engine->NetHandler->RequestPrivateStoreBuyQuit();
}
NW_HOOK_METHOD(0x1005e940, NConsoleWnd, RequestPrivateStoreQuitBuy)

// 0x1005e950
void NConsoleWnd::SetPrivateStoreBuyMsg(TCHAR* Text)
{
    Engine->NetHandler->SetPrivateStoreBuyMsg(Text);
}
NW_HOOK_METHOD(0x1005e950, NConsoleWnd, SetPrivateStoreBuyMsg)

// 0x1005e960 - selling to another player's buying store
void NConsoleWnd::RequestPrivateStoreSell(L2ParamStack& Params)
{
    Engine->NetHandler->SendPrivateStoreBuyBuyList(Params);
}
NW_HOOK_METHOD(0x1005e960, NConsoleWnd, RequestPrivateStoreSell)

// 0x1005ee80 - NetHandler slot 248 (C2 only; name ours, after the slot's)
void NConsoleWnd::BypassUserCmd(L2ParamStack& Params)
{
    Engine->NetHandler->BypassUserCmd(Params);
}
NW_HOOK_METHOD(0x1005ee80, NConsoleWnd, BypassUserCmd)

// 0x1005ee90 - NetHandler slot 247
void NConsoleWnd::RequestDwarvenManufacture()
{
    Engine->NetHandler->RequestRemainTime();
}
NW_HOOK_METHOD(0x1005ee90, NConsoleWnd, RequestDwarvenManufacture)

// 0x1005ef80 - "Previous" on the character selection: back to the login screen (NetHandler +0x270, the logout, and
// +0x2C4). C3: the desktop does its 0x10053780 first and the login stages' dialog box is hidden
void NConsoleWnd::ReturnToLogin()
{
    ((NCConsoleExt*)Desktop)->LogoutMessenger();
    LobbyWnd->ClearCharacters();
    LobbyWnd->HideWindow();
    Engine->NetHandler->SendLogOutPacket();
    Engine->NetHandler->AuthReconnect();
    LoginWnd->ResetWindows();
    LoginWnd->ShowWindow();
    DialogBox->HideWindow();
    nw::Field<NCMessageWnd*>(nw::Field<NCWnd*>(this, 0x3950), 0xD0)->SetText(TEXT("Copyright 2002 NCsoft Corporation. All Rights Reserved."));
    MoveCameraByState(L2CS_None, L2CS_Login);
    SetState(L2CS_Login);
}
NW_HOOK_METHOD(0x1005ef80, NConsoleWnd, ReturnToLogin)

// 0x1005f470 - the item lying on the ground with that object id, by the network handler's +0x9C (the C3 engine's
// slots before the users' moved). Named GetServerID before: it takes the object id (UpdateCursor pushes it)
Item* NConsoleWnd::FindItemByID(INT ObjectID)
{
    return nw::VirtualCall<Item*>(Engine->NetHandler, 0x9C / 4, ObjectID);
}
NW_HOOK_METHOD(0x1005f470, NConsoleWnd, FindItemByID)

// 0x1005f480 - NetHandler slot 31
PetInfo NConsoleWnd::GetPetInfo()
{
    return Engine->NetHandler->GetPetInfo();
}
NW_HOOK_METHOD(0x1005f480, NConsoleWnd, GetPetInfo)

// 0x1005f4a0 - NetHandler slot 32
User* NConsoleWnd::FindUserByName(TCHAR* Name)
{
    return Engine->NetHandler->GetUser(Name);
}
NW_HOOK_METHOD(0x1005f4a0, NConsoleWnd, FindUserByName)

// 0x1005f4b0 - NetHandler slot 33 (not inlined: the stock calls it)
__declspec(noinline) User* NConsoleWnd::FindUserByID(INT ObjectID)
{
    return Engine->NetHandler->GetUser(ObjectID);
}
NW_HOOK_METHOD(0x1005f4b0, NConsoleWnd, FindUserByID)

// 0x1005f4c0 - NetHandler slot 34
FServerStaticObject* NConsoleWnd::FindStaticObjectInfo(INT CreatureID)
{
    return Engine->NetHandler->GetStaticObject(CreatureID);
}
NW_HOOK_METHOD(0x1005f4c0, NConsoleWnd, FindStaticObjectInfo)

// 0x1005f4d0 - the pawn under the mouse; with bCheck, only a player's (its User record,
// NetHandler slot 33, without Unknown8)
AActor* NConsoleWnd::GetTargetedActor(UBOOL bCheck)
{
    guard(NConsoleWnd::GetTargetedActor);
    if (!Engine || !HoveredActor(Engine) || !HoveredActor(Engine)->IsA(APawn::StaticClass()))
        return NULL;
    AActor* Actor = HoveredActor(Engine);
    if (bCheck)
    {
        User* Info = Engine->NetHandler->GetUser(Actor->CreatureID);
        if (!Info || Info->Unknown8)
            return NULL;
    }
    return Actor;
    unguard;
}
NW_HOOK_METHOD(0x1005f4d0, NConsoleWnd, GetTargetedActor)

// 0x1005f600 - NetHandler slot 38
PledgeInfo* NConsoleWnd::GetPledgeInfo(INT PledgeID)
{
    return Engine->NetHandler->GetPledgeInfo(PledgeID);
}
NW_HOOK_METHOD(0x1005f600, NConsoleWnd, GetPledgeInfo)

// 0x1005f610 - NetHandler slot 39
UTexture* NConsoleWnd::GetPledgeCrestTexture(INT PledgeID)
{
    return Engine->NetHandler->GetPledgeCrestTex(PledgeID);
}
NW_HOOK_METHOD(0x1005f610, NConsoleWnd, GetPledgeCrestTexture)

// 0x1005f620 - NetHandler slot 40
UTexture* NConsoleWnd::GetAllyCrestTexture(INT AllyID)
{
    return Engine->NetHandler->GetAllyCrestTex(AllyID);
}
NW_HOOK_METHOD(0x1005f620, NConsoleWnd, GetAllyCrestTexture)

// 0x10063ba0 - NetHandler slot 87 (by value)
void NConsoleWnd::RequestDropItem(NItemRequest Request, FVector Location)
{
    guard(NConsoleWnd::RequestDropItem);
    Engine->NetHandler->RequestDropItem(Request, Location);
    unguard;
}
NW_HOOK_METHOD(0x10063ba0, NConsoleWnd, RequestDropItem)

// The actions on a target: NetHandler slot 53 (object id, where the player stands, Shift held)
// selects it, slot 52 (same) attacks it.

// 0x10064e70 - a click on the player's status window
void NConsoleWnd::TargetMyself()
{
    APawn* Pawn = PlayerPawn(Engine);
    if (!Pawn)
        return;
    FVector Origin = Pawn->Location;
    Engine->NetHandler->Action(PlayerPawn(Engine)->CreatureID, Origin,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10064e70, NConsoleWnd, TargetMyself)

// 0x10064f00
void NConsoleWnd::RequestTarget(INT ObjectID)
{
    APawn* Pawn = PlayerPawn(Engine);
    if (!Pawn)
        return;
    FVector Origin = Pawn->Location;
    Engine->NetHandler->Action(ObjectID, Origin,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10064f00, NConsoleWnd, RequestTarget)

// 0x10064f80 - the item nearest to the player within Range (NetHandler slot 211, a float); the
// stock tests the address of its field +0x10 too, never NULL
void NConsoleWnd::RequestPickUp(INT Range, INT ObjectID)
{
    Item* Found = Engine->NetHandler->GetNearestItem((FLOAT)Range, ObjectID);
    if (!Found)
        return;
    Engine->NetHandler->Action(Found->ObjectID, PlayerPawn(Engine)->Location,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10064f80, NConsoleWnd, RequestPickUp)

// 0x10065010 - the next creature within Range after CurrentID (NetHandler +0x3C4); C3 sets the engine's +0x17C flag
// (read by RequestAttack: it ignores the attack while it is set) before the action
void NConsoleWnd::RequestTargetNext(INT Range, INT CurrentID)
{
    User* Next = Engine->NetHandler->GetNextEnemy((FLOAT)Range, CurrentID);
    if (!Next || !Next->Pawn)
        return;
    nw::Field<INT>(Engine, 0x17C) = 1;
    Engine->NetHandler->Action(Next->ObjectID, PlayerPawn(Engine)->Location,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10065010, NConsoleWnd, RequestTargetNext)

// 0x100650b0 - what the party member targets (C2: the member's target may be a static object, found by the network
// handler's slot 35; the action starts from the player's own place: the Origin argument is not read)
void NConsoleWnd::AssistMember(INT ObjectID, FVector Origin)
{
    User* Member = FindUserByID(ObjectID);
    if (!Member || !Member->Pawn)
        return;
    AController* Controller = Member->Pawn->Controller;
    if (!Controller || !Controller->Target)
        return;
    User* Target = FindUserByID(Controller->Target->CreatureID);
    INT TargetObjectID;
    if (Target)
        TargetObjectID = Target->ObjectID;
    else
    {
        FServerStaticObject* StaticObject = Engine->NetHandler->GetStaticObject(Member->Pawn->Controller->Target->CreatureID);
        if (!StaticObject)
            return;
        TargetObjectID = StaticObject->Unknown8;
    }
    Engine->NetHandler->Action(TargetObjectID, PlayerPawn(Engine)->Location,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x100650b0, NConsoleWnd, AssistMember)

// 0x100651f0 - a click on the pet / summon status window
void NConsoleWnd::RequestAction(INT ObjectID, FVector Origin)
{
    Engine->NetHandler->Action(ObjectID, Origin,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x100651f0, NConsoleWnd, RequestAction)

// 0x1006a260 - the player's User record (not inlined: the stock calls it)
__declspec(noinline) User* NConsoleWnd::GetMyUser()
{
    APawn* Pawn = PlayerPawn(Engine);
    if (!Pawn)
        return NULL;
    return FindUserByID(Pawn->CreatureID);
}
NW_HOOK_METHOD(0x1006a260, NConsoleWnd, GetMyUser)

// 0x1006a2b0 - the User record of the player's target (not inlined: the stock calls it)
__declspec(noinline) User* NConsoleWnd::GetSelectedActor()
{
    guard(NConsoleWnd::GetSelectedActor);
    APawn* Pawn = PlayerPawn(Engine);
    if (Engine && Pawn && Pawn->Controller && Pawn->Controller->Target)
        return FindUserByID(Pawn->Controller->Target->CreatureID);
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x1006a2b0, NConsoleWnd, GetSelectedActor)

// 0x1006a360 - the player's target as an actor (doors and the other static objects have no
// User record)
AActor* NConsoleWnd::GetSelectedStaticActor()
{
    APawn* Pawn = PlayerPawn(Engine);
    if (Engine && Pawn && Pawn->Controller && Pawn->Controller->Target)
        return Pawn->Controller->Target;
    return NULL;
}
NW_HOOK_METHOD(0x1006a360, NConsoleWnd, GetSelectedStaticActor)

// 0x1006a3a0 - the player has a target (the shortcut keys)
UBOOL NConsoleWnd::HasSelectedTarget()
{
    APawn* Pawn = PlayerPawn(Engine);
    if (Engine && Pawn && Pawn->Controller && Pawn->Controller->Target)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x1006a3a0, NConsoleWnd, HasSelectedTarget)

// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)
// 0x1006aa00 - move there (NetHandler slot 90: the player's id, 3 and the point in whole units,
// or 2 and no height)
UBOOL NConsoleWnd::MoveToLocation(FVector Loc, UBOOL bNoZ)
{
    APawn* Pawn = PlayerPawn(Engine);
    if (bNoZ)
        Engine->NetHandler->RequestTeleport(Pawn->CreatureID, 2, (INT)Loc.X, (INT)Loc.Y, 0);
    else
        Engine->NetHandler->RequestTeleport(Pawn->CreatureID, 3, (INT)Loc.X, (INT)Loc.Y,
                                            (INT)Loc.Z);
    return 1;
}
NW_HOOK_METHOD(0x1006aa00, NConsoleWnd, MoveToLocation)

// 0x10070420 - what the target targets (C2: a static object's too: slot 35 of the network handler)
void NConsoleWnd::RequestAssist()
{
    User* Target = GetSelectedActor();
    if (!Target || !Target->Pawn)
        return;
    AController* Controller = Target->Pawn->Controller;
    if (!Controller || !Controller->Target)
        return;
    User* AssistTarget = FindUserByID(Controller->Target->CreatureID);
    INT ObjectID;
    if (AssistTarget)
        ObjectID = AssistTarget->ObjectID;
    else
    {
        FServerStaticObject* StaticObject = Engine->NetHandler->GetStaticObject(Target->Pawn->Controller->Target->CreatureID);
        if (!StaticObject)
            return;
        ObjectID = StaticObject->Unknown8;
    }
    Engine->NetHandler->Action(ObjectID, PlayerPawn(Engine)->Location,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10070420, NConsoleWnd, RequestAssist)


/*-----------------------------------------------------------------------------
    The virtual requests: UL2ConsoleWnd slots the engine leaves pure and the console's own
    (272..284), which the windows call through the vtable.
-----------------------------------------------------------------------------*/

// 0x1005af40 - NetHandler slot 69
void NConsoleWnd::RequestServerList()
{
    guard(NConsoleWnd::RequestServerList);
    Engine->NetHandler->RequestServerList();
    unguard;
}
NW_HOOK_METHOD(0x1005af40, NConsoleWnd, RequestServerList)

// 0x1005b8a0 - the answer to a resurrection offer (NetHandler +0x148: the answer and the offer's two values); refused,
// the restart menu comes back (C3: it is the +0xCC child of the group at +0x3950)
void NConsoleWnd::AnswerReviveRequest(UBOOL bYes, INT Param1, INT Param2)
{
    if (!bYes)
        nw::Field<NCWnd*>(nw::Field<NCWnd*>(this, 0x3950), 0xCC)->ShowWindow();
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)bYes);
    Params.PushBack((void*)(INT_PTR)Param1);
    Params.PushBack((void*)(INT_PTR)Param2);
    Engine->NetHandler->ReviveReply(Params);
}
NW_HOOK_METHOD(0x1005b8a0, NConsoleWnd, AnswerReviveRequest)

// 0x1005be30 - NetHandler slot 171: the item the enchant scroll goes on
void NConsoleWnd::RequestEnchantItem(INT ObjectID)
{
    guard(NConsoleWnd::RequestEnchantItem);
    Engine->NetHandler->RequestEnchantItem(ObjectID);
    unguard;
}
NW_HOOK_METHOD(0x1005be30, NConsoleWnd, RequestEnchantItem)

// 0x1005bf20 - NetHandler slot 165
void NConsoleWnd::RequestDestroyItem(NItemRequest Request)
{
    guard(NConsoleWnd::RequestDestroyItem);
    Engine->NetHandler->RequestDestroyItem(Request);
    unguard;
}
NW_HOOK_METHOD(0x1005bf20, NConsoleWnd, RequestDestroyItem)

// 0x1005bfb0 - NetHandler slot 166
void NConsoleWnd::RequestCrystallizeItem(INT ObjectID, INT Count)
{
    Engine->NetHandler->RequestCrystallizeItem(ObjectID, Count);
}
NW_HOOK_METHOD(0x1005bfb0, NConsoleWnd, RequestCrystallizeItem)

// 0x1005c150 - NetHandler slot 156: the answer to a trade offer
void NConsoleWnd::AnswerTradeRequest(L2ParamStack& Params)
{
    Engine->NetHandler->AnswerTradeRequest(Params);
}
NW_HOOK_METHOD(0x1005c150, NConsoleWnd, AnswerTradeRequest)

// 0x1005c960 - NetHandler slot 173 (C2: not in the Thai HTML mode either)
UBOOL NConsoleWnd::RequestQuestList()
{
    guard(NConsoleWnd::RequestQuestList);
    if (GIsOnlyL2Console || GL2ThaiHtmlMode)
        return 0;
    return Engine->NetHandler->RequestQuestList();
    unguard;
}
NW_HOOK_METHOD(0x1005c960, NConsoleWnd, RequestQuestList)

// 0x1005ca00 - NetHandler slot 170
void NConsoleWnd::RequestDestroyQuest(INT QuestID)
{
    guard(NConsoleWnd::RequestDestroyQuest);
    Engine->NetHandler->RequestDestroyQuest(QuestID);
    unguard;
}
NW_HOOK_METHOD(0x1005ca00, NConsoleWnd, RequestDestroyQuest)

// 0x1005cfd0 - NetHandler slot 172: what a skill of the trainer needs
void NConsoleWnd::RequestAcquireSkillInfo(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAcquireSkillInfo(Params);
}
NW_HOOK_METHOD(0x1005cfd0, NConsoleWnd, RequestAcquireSkillInfo)

// 0x1005cfe0 - NetHandler slot 173
void NConsoleWnd::RequestAcquireSkill(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAcquireSkill(Params);
}
NW_HOOK_METHOD(0x1005cfe0, NConsoleWnd, RequestAcquireSkill)

// 0x1005d870 - NetHandler slot 175
void NConsoleWnd::RequestStartAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestStartAllianceWar(Params);
}
NW_HOOK_METHOD(0x1005d870, NConsoleWnd, RequestStartAllianceWar)

// 0x1005d880 - NetHandler slot 176
void NConsoleWnd::RequestReplyStartAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->ReplyStartAllianceWar(Params);
}
NW_HOOK_METHOD(0x1005d880, NConsoleWnd, RequestReplyStartAllianceWar)

// 0x1005d890 - NetHandler slot 177
void NConsoleWnd::RequestStopAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestStopAllianceWar(Params);
}
NW_HOOK_METHOD(0x1005d890, NConsoleWnd, RequestStopAllianceWar)

// 0x1005d8a0 - NetHandler slot 178
void NConsoleWnd::RequestReplyStopAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->ReplyStopAllianceWar(Params);
}
NW_HOOK_METHOD(0x1005d8a0, NConsoleWnd, RequestReplyStopAllianceWar)

// 0x1005d8b0 - NetHandler slot 179
void NConsoleWnd::RequestSurrenderAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestSurrenderAllianceWar(Params);
}
NW_HOOK_METHOD(0x1005d8b0, NConsoleWnd, RequestSurrenderAllianceWar)

// 0x1005d8c0 - NetHandler slot 187: the freight
void NConsoleWnd::RequestPackageSend(L2ParamStack& Params)
{
    Engine->NetHandler->RequestPackageSend(Params);
}
NW_HOOK_METHOD(0x1005d8c0, NConsoleWnd, RequestPackageSend)

// 0x1005d8d0 - NetHandler slot 188
void NConsoleWnd::RequestPackageSendableItemList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestPackageSendableItemList(Params);
}
NW_HOOK_METHOD(0x1005d8d0, NConsoleWnd, RequestPackageSendableItemList)

// 0x1005d9a0 - NetHandler slot 180 (no caller in NWindow)
void NConsoleWnd::vfn184(L2ParamStack& Params)
{
    Engine->NetHandler->RequestCastleSiegeInfo(Params);
}
NW_HOOK_METHOD(0x1005d9a0, NConsoleWnd, vfn184)

// 0x1005d9b0 - NetHandler slot 181
void NConsoleWnd::RequestSiegeAttackerList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestCastleSiegeAttackerList(Params);
}
NW_HOOK_METHOD(0x1005d9b0, NConsoleWnd, RequestSiegeAttackerList)

// 0x1005d9c0 - NetHandler slot 182
void NConsoleWnd::RequestSiegeDefenderList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestCastleSiegeDefenderList(Params);
}
NW_HOOK_METHOD(0x1005d9c0, NConsoleWnd, RequestSiegeDefenderList)

// 0x1005d9d0 - NetHandler slot 183
void NConsoleWnd::RequestJoinSiege(L2ParamStack& Params)
{
    Engine->NetHandler->RequestJoinCastleSiege(Params);
}
NW_HOOK_METHOD(0x1005d9d0, NConsoleWnd, RequestJoinSiege)

// 0x1005d9e0 - NetHandler slot 184
void NConsoleWnd::RequestConfirmSiegeWaitingList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestConfirmCastleSiegeWaitingList(Params);
}
NW_HOOK_METHOD(0x1005d9e0, NConsoleWnd, RequestConfirmSiegeWaitingList)

// 0x1005d9f0 - NetHandler slot 185: the siege date
void NConsoleWnd::RequestSetSiegeTime(L2ParamStack& Params)
{
    Engine->NetHandler->RequestSetCastleSiegeTime(Params);
}
NW_HOOK_METHOD(0x1005d9f0, NConsoleWnd, RequestSetSiegeTime)

// 0x1005df40 - the tutorial's client events: each pending one (TutorialEnableClientEvent) is sent once (NetHandler
// +0x1A8). C3: the pending bits are at +0x39BC (the header's TutorialEvents is at +0x39C0)
void NConsoleWnd::SendTutorialEvent(INT Event)
{
    if (nw::Field<INT>(this, 0x39BC) & Event)
    {
        Engine->NetHandler->RequestTutorialClientEvent(Event);
        nw::Field<INT>(this, 0x39BC) ^= Event;
    }
}
NW_HOOK_METHOD(0x1005df40, NConsoleWnd, SendTutorialEvent)

// 0x1005ea00 - NetHandler slot 186
void NConsoleWnd::RequestBuyVIPItem(L2ParamStack& Params)
{
    Engine->NetHandler->RequestMultiSellChoose(Params);
}
NW_HOOK_METHOD(0x1005ea00, NConsoleWnd, RequestBuyVIPItem)

// 0x1005eea0 - NetHandler slot 164: a GM command
void NConsoleWnd::BuilderCommand(L2ParamStack& Params)
{
    Engine->NetHandler->SendBypassBuildCmd(Params);
}
NW_HOOK_METHOD(0x1005eea0, NConsoleWnd, BuilderCommand)

// 0x10060140 (vslot 226) - NetHandler +0x2DC (no caller in NWindow). The stock tail-jumps into the slot with Params
// still on the stack: the headers it was built with gave the slot one argument, but the client's RequestSkillCoolTime
// takes none (and pops none). Called through the slot to keep that, not through the declaration.
void NConsoleWnd::vfn176(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x2DC / 4, &Params);
}
NW_HOOK_METHOD(0x10060140, NConsoleWnd, vfn176)
