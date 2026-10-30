#pragma once
// NConsoleWnd - the game console (UClass "ConsoleWnd", CLASS_Config, sizeof 0x35B8): implements
// UL2ConsoleWnd for the engine, owns the window manager and every window, runs the timers.
// The whole vtable is declared in the order of the C2 one (335 slots, 0x10115438): the 275 slots the
// engine leaves pure (all named: the 39 that only C2 has from the UGameEngine handlers that call them and
// from what they do, NConsoleWndC2.cpp), the engine's own, and 15 of the console's (slots 320..334); docs/notas/NConsoleWnd.md
// maps the C1 ones. Every virtual below sits at its C2 slot (the number in its comment).

#include "nwindow/NCObject.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCWnd;
class NCConsole;
class NCVirtualWndMain;
class NCDebugWnd;
struct NDialogInfo;
struct CharacterInfo;
class NCHtmlViewer;
class NCRestartMenuWnd;
class NCOptionWnd;
class NCLoginWnd;
class NCLobbyWnd;
class NCPawnCreateWnd;
class NCLoadingWnd;
class NCMessageWnd;
class NCDialogBox;
class NKeyCrypt;
struct NItemRequest;
struct NItemEntry;
struct NMacroEntry;

// A list of windows that can shrink to its count (UE2's TArray::Shrink, which the stock build expands inline): the
// windows of one UI mode (name ours)
class NWndPtrArray : public TArray<NCWnd*>
{
public:
    void Shrink()
    {
        if (ArrayMax != ArrayNum)
        {
            ArrayMax = ArrayNum;
            Realloc(sizeof(NCWnd*));
        }
    }
};
NW_ASSERT_SIZE(NWndPtrArray, 0xC);

// one SetTimer record (NConsoleWnd::NSetTimer)
struct NTimer
{
    UINT ID;              // +0x00
    UINT Elapse;          // +0x04 milliseconds
    UBOOL bKilled;        // +0x08 set by NKillTimer, dropped by the next Tick
    FLOAT Elapsed;        // +0x0C seconds since the last WM_TIMER
    NCObject* Owner;      // +0x10 gets WindowProc(WM_TIMER, ID, 0)
};
NW_ASSERT_SIZE(NTimer, 0x14);

// a parameter of the system message being received (names ours; AddSystemMessageParam): its
// text, made from a string or a number
struct NSysMsgParam
{
    TCHAR* Text;          // +0x00 GMalloc'd copy
    INT Len;              // +0x04 appStrlen(Text)

    void Set(const TCHAR* InText);    // 0x10047380 a copy of InText; nothing for NULL
    void SetNumber(INT Value);        // 0x100473d0 "%d"
};
NW_ASSERT_SIZE(NSysMsgParam, 0x8);

// The Korean particles of the system messages ("%" and a particle after a "$sN"): whether
// a Hangul syllable ends without a final consonant (any other character counts so; stock
// 0x100427d0, which takes the character in AX: not hooked, its own calling convention),
// and the particle's form after it: i / ga, eul / reul, eun / neun, gwa / wa (0 for
// another character: nothing is put). MakeSystemMessage and EndSystemMessageParam.
FORCEINLINE UBOOL HasNoFinalConsonant(TCHAR C)
{
    if (C < 0xAC00 || C > 0xD7A3)
        return 1;
    return (C - 0xAC00) % (21 * 28) % 28 == 0;
}
FORCEINLINE TCHAR ParticleForm(TCHAR Particle, UBOOL bNoFinal)
{
    // C3 (FormatSystemMessage 0x1005f130, MakeSystemMessage 0x10077330): three pairs, as single-byte codes
    switch (Particle)
    {
    case 0xC0: return bNoFinal ? 0xCC : 0xC0;
    case 0xB0: return bNoFinal ? 0xA1 : 0xB0;
    case 0xB8: return bNoFinal ? 0xB8 : 0xA6;
    }
    return 0;
}

// One line of questname-*.dat in the C2 game data (name ours): 1000 of them at FL2GameData +0x4CF4
// (0x24 bytes each: C1's FL2QuestNameData had 0x14, the two texts are FStrings now). The Engine
// header does not place the table yet; NConsoleWnd reads it through GQuestNames.
struct NQuestNameLine
{
    INT bValid;               // +0x00 0: no quest here
    INT QuestID;              // +0x04
    INT Step;                 // +0x08
    FString Name;             // +0x0C the quest's (AddQuestID)
    FString Text;             // +0x18 the step's (GetQuestText)
};
NW_ASSERT_SIZE(NQuestNameLine, 0x24);
#define GQuestNames ((NQuestNameLine*)((BYTE*)&GL2GameData + 0x4CF4))

// A dye of the henna lists (names ours; 0x44 bytes in C2: UGameEngine::ReceiveHennaInfo and its kind pass it by value
// to the console, NConsoleWndC2.cpp reads these fields: the henna's id for the henna data, the dye's item id for the
// item data)
struct HennaInfo
{
    INT HennaID;               // +0x00 FL2GameData::GetHennaData's key
    INT ItemID;                // +0x04 the dye's item (FL2GameData::GetItemData)
    INT Param08;               // +0x08
    INT Param0C;               // +0x0C
    INT Param10;               // +0x10
    BYTE _Unknown14[0x44 - 0x14]; // +0x14 not read by NWindow
};
NW_ASSERT_SIZE(HennaInfo, 0x44);

// A macro of the list the server sends (UGameEngine::ReceiveMacroListInfo; 0x3FC bytes in C2: the console copies it
// into the macro window's records and its macro object; the fields are the macro window's business)
struct MacroInfo
{
    BYTE Data[0x3FC];
};
NW_ASSERT_SIZE(MacroInfo, 0x3FC);

// a window's place as NConsoleWnd::ScaleRect takes it (names ours): the screen place and size
struct NResizeRect
{
    INT X;
    INT Y;
    INT W;
    INT H;
};
NW_ASSERT_SIZE(NResizeRect, 0x10);

class NConsoleWnd : public UL2ConsoleWnd
{
    NW_DECLARE_CLASS(NConsoleWnd, UL2ConsoleWnd, CLASS_Config, 0x1027B548)

    UBOOL bInitialized;               // +0x392C (name ours) set at the end of Initialize (C3 evidence: the constructor 0x10077ab0 clears it,
                                      //   ConsolePostRender tests it; the derived +0x3948 was wrong)
    INT UIMode;                       // +0x3930 (new in C3, name ours) 0 the game's windows, 1 the observer's (SwitchUIMode 0x10074890 keeps the
                                      //   visible windows of each mode in the two arrays below); the shortcut keys and the target window read it
    NWndPtrArray ModeWnds[2];         // +0x3934 (name ours) the windows of each UI mode: the ones of the desktop that were shown when the mode was left
                                      //   (SwitchUIMode), at +0x3934 for mode 0 and +0x3940 for mode 1
    NCVirtualWndMain* WndMain;        // +0x394C (NConsoleWnd::SetCursor)
    // the windows of the login and character stages (Initialize; names ours)
    NCWnd* GodWnd;                    // +0x3950 (C3: NCGodWnd, 0xDC bytes; name ours, was RestartMenuWnd) the screen-sized top window made last by
                                      //   Initialize (style 0xA, ctor 0x100861C0): it owns the restart menu (+0xCC) and the options window (+0xD4)
                                      //   and answers ShowMessage / ShowOptionWnd / ReceiveServerList / RestartBegin; hidden for the test consoles
    // C2: not placed (no evidence for where it went): NCOptionWnd* OptionWnd;           // +0x3524
    NCLoginWnd* LoginWnd;             // +0x3954
    NCWnd* CreditWnd;                 // +0x3958 (name ours; new in C2) NCCreditWnd, a top window made by Initialize   // C2: offset derived, no evidence
    NCLobbyWnd* LobbyWnd;             // +0x395C the character selection
    NCPawnCreateWnd* PawnCreateWnd;   // +0x3960 the character creation
    NCLoadingWnd* LoadingWnd;         // +0x3964
    // C2: not placed (no evidence for where it went): NCMessageWnd* MessageWnd;         // +0x353C the line under the login screens
    NCWnd* Desktop;                   // +0x3968 (name ours) the screen-sized NCConsole window,
                                      //   root of the in-game UI (Initialize)
    NCDebugWnd* DebugWnd;             // +0x396C (NCObject::Debugf)
    NCWnd* UnusedWnd;                 // +0x3970 (name ours) a top window never created   // C2: offset derived, no evidence
                                      //   (Finalize destroys it)
    NCDialogBox* DialogBox;           // +0x3974 (name ours) the login stages' dialog box   // C2: offset derived, no evidence
    NCWnd* PetitionWnd;               // +0x3978 (name ours) top-level NCPetitionWnd, the GM   // C2: offset derived, no evidence
                                      //   chat window (Initialize)
    NCWnd* WarningDialogBox;          // +0x397C (C3: NCPetitionFeedBackWnd, class 0x1029E728, ctor 0x10077A10, 0xE8 bytes, SysString 848 as its title;
                                      //   the name is the C2 one, kept for NCPetitionWnd.cpp) a top window made by Initialize at the screen's lower right
    TArray<NSysMsgParam> SysMsgParams;// +0x3980 (name ours) of the system message being received (the constructor
                                      //   clears +0x3558, +0x355C, +0x3560; Add/EndSystemMessageParam, MakeSystemMessage)
    NCWnd* WordWrapperWnd;            // +0x398C (name ours; new in C2) L2WordWrapperWnd, a top window made by Initialize   // C2: offset derived, no evidence
    TArray<NTimer> Timers;            // +0x3990
    INT MouseWheelDelta;              // +0x399C (name ours) of the last wheel event
    FLOAT DeltaTime;                  // +0x39A0 seconds of the last Tick   // C2: offset derived, no evidence
    INT Unknown357C;                  // +0x39A4 (new in C2; the constructor sets 1)   // C2: offset derived, no evidence
    INT Unknown3580;                  // +0x39A8 (new in C2; the constructor clears it)   // C2: offset derived, no evidence
    UBOOL bCtrlDown;                  // +0x39AC (name ours) PressCtrl / ReleaseCtrl   // C2: offset derived, no evidence
    // C3: bCtrlDownAlt is gone (PressCtrl / ReleaseCtrl are two plain stores, IsCtrlDown is no function). From here on
    // every field is 4 bytes below the derived offsets this header had (evidence: the constructor's stores, PressCtrl,
    // ConsolePostRender 0x10075c80 (bSuspended), MasterConsoleEventProcess 0x10075d50 (bWheelHandled), SendTutorialEvent
    // 0x1005df40 (TutorialEvents), UseAction 0x10074ab0 (PartyItemDistribution), Initialize 0x10077f20 (KeyCrypt))
    UBOOL bSuspended;                 // +0x39B0 (name ours) no painting, no input; never set
    UBOOL bWheelHandled;              // +0x39B4 (name ours) the last wheel step went to a window:
                                      //   its release is eaten (MasterConsoleEventProcess)
    INT Unknown3720;                  // +0x39B8 (AcceptGameStart passes it to SetRes as the fullscreen flag)
    INT TutorialEvents;               // +0x39BC (name ours) the pending tutorial event bits
    NKeyCrypt* KeyCrypt;              // +0x39C0 (new in C2; the constructor clears it, Initialize sets it) the keyboard protection helper
                                      //   (NKeyCrypt.h), made by Initialize when GL2UseKeyCrypt is set
    INT LoadingScreenID;              // +0x39C4 (name ours) shown when entering the game (10001)
    INT PartyItemDistribution;        // +0x39C8 (name ours) sent with party invitations (NCGameOptionWnd::Apply writes it,
                                      //   NCMatchWnd::OnInvite and UseAction read it)
    UBOOL bNoChatOnEnter;             // +0x39CC (name ours) Enter leaves the chat line alone (the constructor clears it,
                                      //   HookShortCutKey reads it); never set
    INT Unknown39D0;                  // +0x39D0 (new in C3; ReceiveVersionCheck stores it, 0x1008cb80 reads it)
    NCWnd* PawnViewerWnd[3];          // +0x39D4 (name ours) the pawn viewer's windows
    NCWnd* ReplayWnd;                 // +0x39E0 (new in C3, name ours) the replay mode's screen-sized top window (NCReplayWnd, made by
                                      //   InitializeReplayMode 0x100744a0; C3Virtual385 / 386 hide and show it)
    INT Unknown39E4;                  // +0x39E4 (new in C3, up to sizeof; the constructor clears it)

    NConsoleWnd();                                                   // 0x10077ab0 (standalone: the real constructor, NConsoleWnd.cpp)
    // Hook build: the stock constructor's and destructor's bodies as plain methods (names ours; hooked at 0x10077AB0 / 0x10077E80).
    // The objects are made by the stock InternalConstructor and must keep the STOCK vtables (0x10185F80 and, at +0x28, 0x10185F7C):
    // our compiled vtable still has placeholders (NConsoleWnd.cpp, VT DIFF), so a C++ constructor / destructor hook, which stores
    // ours, is no option (SwitchboardSession and MSNSocket write the stock vtable by hand the same way). Not defined in the
    // standalone (the C++ constructor and NW_DECLARE_CLASS's destructor are those bodies there)
    NConsoleWnd* ConstructConsole();                                 // 0x10077ab0 (returns this)
    void DestroyConsole();                                           // 0x10077e80
    NCConsole* GetDesktop() const;                                   // Desktop as what it is
                                                                     //   (defined in NCConsole.h)

    // Decompiled in NConsoleWndNet.cpp / NConsoleWndData.cpp, not in the groups of the windows
    // below (names ours: the requests after the protocol's packets)
    void RequestBypassToServer(L2ParamStack& Params);                // 0x1005ddc0 HTML "bypass"
    void RequestTutorialPassCmdToServer(L2ParamStack& Params);       // 0x1005df20 the tutorial's
    UTexture* GetAllyCrestTexture(INT AllyID);                       // 0x1005f620 8x12 crest
    void RequestAddTradeItem(NItemRequest Request);                  // 0x1005c040 (guard name)
    void RequestDropItem(NItemRequest Request, FVector Location);    // 0x10063ba0 (guard name)
    const TCHAR* GetSkillIconName(INT SkillID, INT Level);           // 0x1005f630
    const TCHAR* GetActionIconName(INT ActionID);                    // 0x1006a3e0 (C2: heap entries, FString at +0x28)
    const TCHAR* GetCastleName(INT CastleID);                        // 0x1005fcb0 castlename-*.dat
    const TCHAR* GetCastleLocation(INT CastleID);                    // 0x1005fd00 its 2nd text
    const TCHAR* GetCastleDescription(INT CastleID);                 // 0x1005fd50 its 3rd text
    INT GetWeaponEnchantSteps(INT Enchant);                          // 0x10060120 past +3: x2
    INT GetArmorEnchantSteps(INT Enchant);                           // 0x10060130 past +3: x3
    const TCHAR* GetIconName(INT ItemID);                            // 0x1006aa90 (C1 only: no C2 match) (guard name)
    UBOOL HasSelectedTarget();                                       // 0x1006a3a0 (shortcut keys)
    void FormatDateTime(INT Time, TCHAR* Out);                       // 0x10078ea0 "yyyy/mm/dd hh:mm"
                                                                     //   (the siege windows)
    void MakeSystemMessage(L2ParamStack& Params, TCHAR* Out);        // 0x10077330 (NConsoleWndText.cpp)
                                                                     //   systemmsg id, count, then
                                                                     //   (type, value) each

    // Declared by the windows that use them. Decompiled: the requests to the server
    // (NConsoleWndNet.cpp) and the data getters (NConsoleWndData.cpp); the rest in the other
    // NConsoleWnd*.cpp files. Names ours unless a guard string gave them; the
    // Request* ones pass the call on to the engine's network handler (UL2NetHandler at Engine
    // +0x5C) and are named after the chat command that uses them.
    INT GetIMEType();                 // 0x1005a4b0 kind of the keyboard layout's IME: 1 closed
                                      //   or none, 2..12 by layout (9, 10: Chinese), -1 other
    UBOOL SendChatMsgToServer(INT Type, TCHAR* Text, TCHAR* Target); // 0x1005a890 (guard name) 0
    void RequestTrade(INT CreatureID);                               // 0x1005bfc0 (guard name)
    User* GetSelectedActor();                                        // 0x1006a2b0 (guard name) target
    User* GetMyUser();                                               // 0x1006a260 the player
    User* FindUserByName(TCHAR* Name);                               // 0x1005f4a0
    FVector GetMyLocation();                                         // 0x10062940 (C2: anonymous, the C1 twin was 0x10038540) where the player's pawn stands
    // the player's pawn and the macros (C2 only; names ours): the macro list is the macro window's (NCConsole +0xF8)
    APawn* GetMyPawn();                                              // 0x1006a290 the first viewport's controller's pawn (no checks)
    __declspec(noinline) INT GetMacroCount();                                             // 0x1005f830 0 without the macro window
    __declspec(noinline) NMacroEntry* GetMacro(INT Index);                                // 0x1006a480 NULL out of 0..GetMacroCount
    UBOOL IsAnyMacroRunning();                                       // 0x1006a4b0 a macro with bRunning set
    TCHAR* GetMacroName(INT MacroID);                                // 0x1006a510 the name of the macro with that id, NULL when none
    INT GetMacroIconType(INT MacroID);                               // 0x1006a570 its icon number, -1 when none
    void StartMacro(INT MacroID);                                    // 0x1006b040 sets bRunning of the macro with that id
    UBOOL AddMacro(NMacroEntry* Macro);                              // 0x1005f7f0 into the macro window's list; 0 when there is none
    INT GetHennaSlotCount();                                         // 0x1006a740 1..3 by the profession tier of the class (-1: no player)
    UBOOL HasItemAdditionalName(INT ItemID);                         // 0x10060900 the item's additional name is not "NAME_None"
    const TCHAR* GetItemAdditionalName(INT ItemID);                  // 0x10060a90
    TCHAR* GetPartyMemberName(INT Index);                            // 0x1005b450 the party window's member, NULL out of range
    UBOOL GetMousePos(INT* X, INT* Y);                               // 0x1005f0b0 the window manager's (tail call)
    void RequestGMSnoopEnd(INT SnoopID);                             // 0x1005ea70 NetHandler slot 251 (a tail call)
    void StartGameGuard(INT hWnd);                                   // 0x10060620 (name ours) the game's side of the nProtect GameGuard start-up
    void ScaleRect(INT W0, INT H0, INT W1, INT H1, NResizeRect* Rect); // (C2 only: no C3 function, nobody calls it) the place a window keeps in proportion when the desktop goes from W0 x H0 to W1 x H1
    void ShowChatText(const TCHAR* Text, DWORD Color);               // 0x1005f070 the desktop's AddSystemText with sub type 5
    void GetNumberMessage(INT MsgID, INT Number, TCHAR* Out);        // 0x1005ec80 the text of a system message with one number
    __declspec(noinline) void ShowSystemMessage(INT ID, DWORD Color); // 0x10073770 systemmsg line (Color is not read in C2)
    __declspec(noinline) void PlaySoundName(const TCHAR* Name);                           // 0x10060ac0 (name ours) the interface sound of a name
    void PlayInterfaceSound(INT SoundID);                            // 0x1006b0a0 (name ours) the sound of a number: the table
                                                                     //   of names at 0x1016a0a0 (0 click_1 ... 10 petition)
    void ShowDialog(NDialogInfo* Info);                              // 0x100664a0
    void ExecCommand(TCHAR* Command);                                // 0x10066470 viewport Exec
    void RequestPetition(TCHAR* Text, INT Type);                     // 0x1005e130 (C2: Type is new, passed on: name ours)
    void RequestPetitionCancel();                                    // 0x1005e140
    void RequestGMList();                                            // 0x1005e150
    void RequestSocialAction(INT Action);                            // 0x1005d3f0
    void RequestChangeMoveType(UBOOL bRun);                          // 0x1005d410
    void RequestChangeWaitType(UBOOL bStand);                        // 0x1005d400
    void RequestAttack(INT ObjectID, FVector Origin, INT bForceAttack = 0, INT bShift = 0); // 0x10065250 (C2: two more words, ret 0x18)
    void RequestAttackForce(INT ObjectID, FVector Origin);           // 0x1005d3b0
    void RequestAttackStand(INT ObjectID, FVector Origin);           // 0x10054710 (C1 only: no C2 match)
    void RequestTarget(INT ObjectID);                                // 0x10064f00
    void RequestTargetNext(INT Range, INT CurrentID);                // 0x10065010
    void RequestPickUp(INT Range, INT ObjectID);                     // 0x10064f80
    void RequestAssist();                                            // 0x10070420
    void RequestPartyInvite(L2ParamStack& Params);                   // 0x1005da40
    void RequestPartyLeave(L2ParamStack& Params);                    // 0x1005da60
    void RequestPartyDismiss(L2ParamStack& Params);                  // 0x1005da70
    void RequestVendor();                                            // 0x1005e880
    void RequestMagicSkillUse(L2ParamStack& Params);                 // 0x1005d030
    void RequestMakeMacro(L2ParamStack& Params);                     // 0x1005d010 (C2 only; name ours) NetHandler slot 145
    void RequestDeleteMacro(L2ParamStack& Params);                   // 0x1005d020 (C2 only; name ours) NetHandler slot 146
    void BypassUserCmd(L2ParamStack& Params);                        // 0x1005ee80 (C2 only; name ours) NetHandler slot 248
    void RequestRecipeBookOpen();                                     // 0x1005d090 (C2 only; name ours) NetHandler slot 199
    void RequestRecipeItemDelete(INT P1);                             // 0x1005d0a0 (C2 only; name ours) NetHandler slot 200
    void RequestRecipeItemMakeInfo(INT P1);                           // 0x1005d0b0 (C2 only; name ours) NetHandler slot 201
    void RequestRecipeItemMakeSelf(INT P1);                           // 0x1005d0c0 (C2 only; name ours) NetHandler slot 202
    void RequestRecipeShopManageList();                               // 0x1005d0d0 (C2 only; name ours) NetHandler slot 203
    void RequestRecipeShopMessageSet(TCHAR* P1);                      // 0x1005d0e0 (C2 only; name ours) NetHandler slot 204
    void RequestRecipeShopListSet(L2ParamStack& P1);                  // 0x1005d0f0 (C2 only; name ours) NetHandler slot 205
    void RequestRecipeShopManageQuit();                               // 0x1005d100 (C2 only; name ours) NetHandler slot 206
    void RequestRecipeShopManageCancel();                             // 0x1005d110 (C2 only; name ours) NetHandler slot 207
    void RequestRecipeShopSellList(INT P1);                           // 0x1005d120 (C2 only; name ours) NetHandler slot 208
    void RequestRecipeShopMakeInfo(INT P1, INT P2);                   // 0x1005d130 (C2 only; name ours) NetHandler slot 209
    void RequestRecipeShopMakeDo(INT P1, INT P2, INT P3);             // 0x1005d140 (C2 only; name ours) NetHandler +0x35C (C3: three values, ret 0xC)
    void RequestHennaItemInfo(INT P1);                                // 0x1005d190 (C2 only; name ours) NetHandler slot 211
    void RequestHennaEquip(INT P1);                                   // 0x1005d1a0 (C2 only; name ours) NetHandler slot 212
    void RequestHennaUnequipInfo(INT P1);                             // 0x1005d1b0 (C2 only; name ours) NetHandler slot 213
    void RequestHennaUnequip(INT P1);                                 // 0x1005d1c0 (C2 only; name ours) NetHandler slot 214
    void UseShortcut(L2ParamStack& Params);                          // 0x10044860 (C1 only: no C2 match) (C1 only: no C2 match)
    void UseShortCutEquip(INT Index);                                // 0x1006ffc0 (C2 only; name ours) the item of a shortcut slot, a weapon or armor not worn yet: put on
    void RequestAllyInvite(L2ParamStack& Params);                    // 0x1005d750
    void RequestAllyInfo();                                          // 0x1005d780
    void RequestFriendInvite(L2ParamStack& Params);                  // 0x1005d950
    void RequestFriendDel(L2ParamStack& Params);                     // 0x1005d960
    void RequestFriendList();                                        // 0x1005d980
    void RequestBlock(L2ParamStack& Params);                         // 0x1005d990
    void RequestBuy();                                               // 0x1005e910
    void RequestDwarvenManufacture();                                // 0x1005ee90
    // used by the dialog box (NCDialogBox::OnOK / OnCancel)
    void ConvertNewLines(TCHAR* Text, FString& Result);              // 0x1005fda0 escaped newlines
    UBOOL HasInvalidChars(TCHAR* Text);                              // 0x1005f2b0 empty, or a
                                                                     //   character the font lacks
    UBOOL HasBadWord(TCHAR* Text);                                   // 0x1004a190 (C1 only: no C2 match) (C1 only: no C2 match) word filter
    UBOOL IsValidName(TCHAR* Text);                                  // 0x10077260
    void RegisterPledgeCrest(L2ParamStack& Params);                  // 0x10076300 bmp file path
    void RegisterAllyCrest(L2ParamStack& Params);                    // 0x100765c0
    void SetPrivateStoreMsg(TCHAR* Text);                            // 0x1005e8c0
    void SetPrivateStoreBuyMsg(TCHAR* Text);                         // 0x1005e950
    void RequestChangePetName(TCHAR* Name);                          // 0x1005e010
    void RequestAnswerJoinParty(L2ParamStack& Params);               // 0x1005da50
    void RequestAnswerJoinPledge(L2ParamStack& Params);              // 0x1005d4d0
    void RequestAnswerJoinAlly(L2ParamStack& Params);                // 0x1005d760
    void RequestAnswerFriendInvite(L2ParamStack& Params);            // 0x1005d970
    void RequestGiveNickName(L2ParamStack& Params);                  // 0x1005d560 member title
    void RequestOustPledgeMember();                                  // 0x1005d4e0 (NetHandler +0x1CC: Engine RequestDismissPledge(), no argument)
    void RequestWithdrawalPledge();                                  // 0x100548b0 (C1 only: no C2 match)
    void RequestStartPledgeWar(L2ParamStack& Params);                // 0x1005d500
    void RequestReplyStartPledgeWar(L2ParamStack& Params);           // 0x1005d510
    void RequestStopPledgeWar(L2ParamStack& Params);                 // 0x1005d520
    void RequestReplyStopPledgeWar(L2ParamStack& Params);            // 0x1005d530
    void RequestSurrenderPledgeWar(L2ParamStack& Params);            // 0x1005d540
    void RequestSurrenderPersonally(L2ParamStack& Params);           // 0x1005d550
    void RequestOustAlly(L2ParamStack& Params);                      // 0x1005d770
    void RequestWithdrawAlly();                                      // 0x1005d790
    void RequestDismissAlly();                                       // 0x1005d7a0
    void RequestActionUse(L2ParamStack& Params);                     // 0x1005d3a0
    void RequestNet103(L2ParamStack& Params);                        // 0x1005d4c0 (NetHandler +0x1C4: Engine RequestAnswerJoinPledge(L2ParamStack&))
    void RequestNet106(L2ParamStack& Params);                        // 0x1005d4f0 (NetHandler +0x1D8: Engine RequestStartPledgeWar(L2ParamStack&))
    // used by the login screens (NCAuthWnd, NCLoginWnd, NCServerSelectWnd)
    UBOOL RequestAuthLogin(const TCHAR* Account, const TCHAR* Password); // 0x1005a980 (guard name)
    UBOOL RequestAuthCardKeyLogin(INT CardID, INT Key);              // 0x1005aa10 (guard name; C2 only) NetHandler slot 68 (RequestSCCheck)
    void RequestServerLogin(L2ParamStack& Params);                   // 0x1005afc0 (guard name)
    // ShowMessageWnd (the message window) is a virtual in C2: slot 317
    void ToggleOptionWnd();                                          // 0x100460c0 (C1 only: no C2 match) (C1 only: no C2 match) +0x36C4
    // used by the clan and party matching windows
    UTexture* GetPledgeCrestTexture(INT PledgeID);                   // 0x1005f610 16x12 crest
    void RequestJoinPledge();                                        // 0x1005d570 (NetHandler +0x1D4: Engine RequestWithDrawalPledge(), no argument)
    void RequestPartyMatchConfig(L2ParamStack& Params);              // 0x10054db0 (C1 only: no C2 match)
    void RequestPartyMatchDetail(INT ObjectID);                      // 0x10054dd0 (C1 only: no C2 match)
    const TCHAR* GetClassNameString(INT ClassID);                    // 0x1005fec0 SysString[247 + ID]
    // used by the menus and the option windows
    void RequestRestartPoint(L2ParamStack& Params);                  // 0x1005ae60 restart menu
                                                                     //   choice (0 village .. 4 fixed)
    void RequestRestart(L2ParamStack& Params);                       // 0x1005ae50 back to the
                                                                     //   character selection
    // RequestLogout (quit the game) is a virtual in C2: slot 315
    // C1 virtuals (slots 269 and 270) with no function in C2 (the pad of the shortcut bar is gone)
    void SetPadShift1(BYTE bShift);                                  // 0x10047b50 (C1 only: no C2 match) (C1 only: no C2 match) ShortCutWnd->bPadShift1
    void SetPadShift2(BYTE bShift);                                  // 0x1004e940 (C1 only: no C2 match) ShortCutWnd->bPadShift2
    void ResetWindowPositions();                                     // 0x100694D0 every window back
                                                                     //   to its default place
    void ReloadGameTexts();                                          // 0x1005f3e0 after the item /
                                                                     //   skill texts were reloaded
    // used by the status windows (player, target, party, buffs)
    void TargetMyself();                                             // 0x10064e70 click on the
                                                                     //   player window
    AActor* GetSelectedStaticActor();                                // 0x1006a360 door or other
                                                                     //   static target
    FServerStaticObject* FindStaticObjectInfo(INT CreatureID);       // 0x1005f4c0
    PetInfo GetPetInfo();                                            // 0x1005f480
    BYTE IsSiegeNpc(INT NpcID);                                      // 0x10057320 (C1 only: no C2 match) castle guards,
                                                                     //   teleporters, mercenaries
    PledgeInfo* GetPledgeInfo(INT PledgeID);                         // 0x1005f600
    void RequestTargetCancel(L2ParamStack& Params);                  // 0x1005d390
    void AssistMember(INT ObjectID, FVector Origin);                 // 0x100650b0 targets what
                                                                     //   the member targets
    INT GetSkillRange(INT SkillID, INT Level);                       // 0x1005f7c0 skill data +0x2C
    TCHAR* GetSkillName(INT SkillID, INT Level);                     // 0x1005f660
    TCHAR* GetSkillDesc(INT SkillID, INT Level);                     // 0x1005f690
    Item* FindItemByID(INT ObjectID);                                // 0x1005f470 (C3; the C2 name) NetHandler +0x9C: the item on the ground
                                                                     //   30 (GM status: crest names)
    // used by the pet, summon and minimap windows
    void RequestAction(INT ObjectID, FVector Origin);                // 0x100651f0 net handler
                                                                     //   slot 53 (click on it)
    void LockCursor();                                               // 0x1005f0c0
    void UnlockCursor();                                             // 0x1005f0d0
    // used by the action window
    void RequestPledgeMemberList(L2ParamStack& Params);              // 0x1005d4b0 (NetHandler +0x1C0: Engine RequestCreatePledge(L2ParamStack&))
    void RequestShortCutReg(L2ParamStack& Params);                   // 0x10044830 (C1 only: no C2 match) (C1 only: no C2 match) (type, slot,
                                                                     //   id...) + the drop sound
    void PetPickUp(INT Range, INT Param);                            // 0x1005dfa0 nearest item
                                                                     //   (slot 210) to slot 189
    // used by the character selection and creation (NCLobbyWnd, NCPawnSelectWnd,
    // NCPawnCreateWnd) and the announcements (NCNoticeWnd)
    UBOOL RequestGameStart(INT Index);                               // 0x1005aab0 (guard name)
    UBOOL RequestCharacterDelete(INT Index);                         // 0x1005abb0 (guard name)
    UBOOL RequestCharacterRestore(INT Index);                        // 0x1005ac60 (guard name)
    void RequestNewCharacter();                                      // 0x1005ad10 (guard name)
    void ReturnToLogin();                                            // 0x1005ef80 "Previous" on
                                                                     //   the character selection
    void ReturnToCharacterSelect();                                  // 0x1005f020 creation cancelled
    void ShowWarningWnd();                                           // 0x1005ae70 after the
                                                                     //   announcements, when any
    void FormatSystemMessage(TCHAR* Out, const TCHAR* Format, const TCHAR* Param1,
                             const TCHAR* Param2);                   // 0x10046120 $s1 / $s2
    UBOOL RequestCharacterCreate(CharacterInfo Info);                // 0x1005ad90 (guard name)
    // used by the quest and item windows
    const TCHAR* GetQuestText(INT QuestID, INT Step);                // 0x1005cd20 (name ours; it
                                                                     //   uses AddQuestID's guard)
    void GetItemSlotText(INT Type, INT BodyPart, FString& Result, INT SubType); // 0x1005f940 the
                                                                     //   slot line of a tool tip
    // used by the shortcut bar and the skill windows
    INT RequestUseItem(INT ObjectID);                                // 0x1005bbc0 (guard name; C3 returns the network handler's answer)
    void RequestPetUseItem(INT ObjectID);                            // 0x1005dfe0 NetHandler slot 190
    void RequestShortCutDel(L2ParamStack& Params);                   // 0x1005d280 (slot)
    void RequestSkillList(L2ParamStack& Params);                     // 0x1005d040 NetHandler slot 158
    const TCHAR* GetItemName(INT ItemID);                            // 0x100739c0 (guard name)
    const TCHAR* GetActionName(INT ActionID);                        // 0x1006a430 (C2: FString at +0x1C) actionname-*.dat
    UBOOL IsToggleSkill(INT SkillID, INT Level);                     // 0x1005f6f0 skill data +0x14 == 3
    UBOOL HasAbnormal(INT SkillID);                                  // 0x1006aa90 the skill's effect
                                                                     //   is on the player (buffs)
    UBOOL IsShotItem(INT ItemID);                                    // 0x10062370 soul / spirit shots (the ones whose auto use can be turned on)
    INT GetSkillMpCost(INT SkillID, INT Level);                      // 0x1005f790 skill data +0x1C
    const TCHAR* GetSkillTypeText(INT SkillID, INT Level);           // 0x1005f720 "Active Skill",
                                                                     //   "Passive Skill" or "Magic"
    // used by the HTML viewer
    UBOOL HasNoAmpersand(TCHAR* Text);                               // 0x1005f310 0 when Text
                                                                     //   holds a '&'
    void RequestTutorialQuestionMark(INT ID);                        // 0x1005df30 the tutorial
                                                                     //   question mark clicked
    // used by the desktop (NCConsole)
    UBOOL MoveToLocation(FVector Loc, UBOOL bNoZ);                   // 0x1006aa00 NetHandler slot 90
    User* FindUserByID(INT ObjectID);                                // 0x1005f4b0 NetHandler slot 33
    void RequestItemList();                                          // 0x1005bb40 (guard name)
    void RequestPartyMatchList(INT Status);                          // 0x10054dc0 (C1 only: no C2 match) 3: leaves the room
    void RequestLinkHtml(L2ParamStack& Params);                      // 0x1005ddb0 HTML "Link"
    void RequestBBSWrite(L2ParamStack& Params);                      // 0x1005ddd0 HTML "Write"
    void RequestShowBoard(L2ParamStack& Params);                     // 0x1005dde0 community board
    void RequestTutorialLinkHtml(L2ParamStack& Params);              // 0x1005df10
    void RequestPrivateStoreQuitSell();                              // 0x1005e8b0
    void RequestPrivateStoreQuitBuy();                               // 0x1005e940
    void RequestGMInventory(TCHAR* Name);                            // 0x1005e160 GM command 5
    void RequestGMSkills(TCHAR* Name);                               // 0x1005e180 GM command 3
    void RequestGMStatus(TCHAR* Name);                               // 0x1005e1a0 GM command 1
    void RequestGMQuests(TCHAR* Name);                               // 0x1005e1c0 GM command 4
    void RequestGMWarehouse(TCHAR* Name);                            // 0x1005e1e0 GM command 6
    void RequestGMPledge(TCHAR* Name);                               // 0x1005e200 GM command 2
    // used by the trade window and the pet's inventory
    void RequestTradeDone(INT bConfirm);                             // 0x1005c0d0 (guard name) OK 1,
                                                                     //   cancel 0
    void RequestGetItemFromPet(L2ParamStack& Params);                // 0x1005e000 NetHandler slot
                                                                     //   192 (object id, amount, 0)
    // used by the inventory (NCInventoryWnd, NCInvenItemWnd, NCEquipItemWnd)
    void RequestUnEquipItem(L2ParamStack& Params);                   // 0x1005bc80 body part bits
    void RequestGiveItemToPet(L2ParamStack& Params);                 // 0x1005dff0 object id, amount
    AActor* GetTargetedActor(UBOOL bCheck);                          // 0x1005f4d0 (guard name) the
                                                                     //   pawn under the mouse
    INT GetEquipSlot(INT BodyPart, INT ObjectID);                    // 0x1006a7c0 paperdoll slot,
                                                                     //   14 / 15: two-slot items
    const TCHAR* GetGradeText(INT Grade);                            // 0x1005fee0 crystal grade
    UBOOL IsPetCollar(INT ItemID);                                   // 0x10046880 (C1 only: no C2 match) (C1 only: no C2 match) 2375, 3500..3502
    UBOOL IsMagicArmor(INT ItemID);                                  // 0x10073a70 armor type 3 (MP)
    const TCHAR* GetWeaponTypeName(INT Type);                        // 0x1005f850 sysstring 43..504
    const TCHAR* GetAttackSpeedText(INT Speed);                      // 0x1005fc30 sysstring 306..310
    INT GetEnchantedDefense(INT Grade, INT Enchant, INT Base);       // 0x10060e60 (C1 only: no C2 match) base + bonus
    INT GetEnchantedAttack(INT Grade, INT Enchant, INT Base);        // 0x1006a710 (weapons)
    // used by the shop and warehouse windows (NCShopWnd, NCStoreWnd): (id, count) lists
    void RequestBuyItem(L2ParamStack& Params);                       // 0x1005c450 (guard name)
    void RequestBuySeed(L2ParamStack& Params);                       // 0x1005c200 (guard name; C2 only) NetHandler slot 90
    void RequestProcureCrop(L2ParamStack& Params);                   // 0x1005c280 (guard name; C2 only) NetHandler slot 91
    void RequestSellItem(L2ParamStack& Params);                      // 0x1005c4d0 (guard name)
    void RequestWarehouseDeposit(L2ParamStack& Params);              // 0x1005c810 NetHandler slot 138
    void RequestWarehouseWithdraw(L2ParamStack& Params);             // 0x1005c820 NetHandler slot 139
    // used by the private stores (names ours, after the packets; the quit requests, NetHandler
    // slots 198 and 205, are with the desktop's above)
    void RequestSetPrivateStoreListSell(L2ParamStack& Params);       // 0x1005e890 NetHandler slot
                                                                     //   196: count, then object id,
                                                                     //   count and price of each
    void RequestPrivateStoreManageCancel();                          // 0x1005e8a0 NetHandler slot 197
    void RequestPrivateStoreBuy(L2ParamStack& Params);               // 0x1005e8d0 NetHandler slot
                                                                     //   201: the owner's object id,
                                                                     //   then as above
    void RequestSetPrivateStoreListBuy(L2ParamStack& Params);        // 0x1005e920 NetHandler slot
                                                                     //   203: count, then item id,
                                                                     //   enchant, count and price
    void RequestPrivateStoreBuyManageCancel();                       // 0x1005e930 NetHandler slot 204
    void RequestPrivateStoreSell(L2ParamStack& Params);              // 0x1005e960 NetHandler slot
                                                                     //   208: the owner's object id,
                                                                     //   count, then object id and
                                                                     //   the above for each
    // the console's own window management (names ours unless a guard string gave them)
    UBOOL Initialize(HWND hWnd);                                     // 0x10061e10 (C1 only: no C2 match) every window
    void LoadWindowPositions();                                      // 0x10067a70 WindowsInfo.ini
    void SaveWindowPositions();                                      // 0x10066940 WindowsInfo.ini
    void Finalize();                                                 // 0x1006ba90
    UBOOL InitializePawnViewer(HWND hWnd);                           // 0x100613a0 (C1 only: no C2 match)
    // the replay mode (C3 only; names from the stock guard strings). InitReplayMode is the C3 virtual 325, not declared
    // virtual (the placeholder NewVirtual325 below jumps to the stock address, where the hook is)
    UBOOL InitializeReplayMode(HWND hWnd);                           // 0x100744a0 (ret 4) Initialize for the replay
    UBOOL InitReplayMode(UEngine* InEngine);                         // 0x10077820 (ret 4) the same for the replay: the engine kept, the state InGame
    void SwitchUIMode(INT NewMode);                                  // 0x10074890 (ret 4) 0 the game's windows, 1 the observer's
    // C3 virtual 359 (not declared virtual: FL2NetNotify's OnEventMatchObserver, ret 0x30): mode 0 / 1 of a match observed
    void OnEventMatchObserver(INT A, ANSICHAR Mode, ANSICHAR C, FString D, FString E, INT F, INT G, INT H); // 0x10075030
    INT NSetTimer(UINT ID, UINT Elapse, NCObject* Owner);            // 0x1006be40 (NCObject::SetTimer)
    UBOOL NKillTimer(UINT ID, NCObject* Owner);                      // 0x10062bf0
    void SetCursor(INT Type, UTexture* Texture);                     // 0x1005f090 unless locked
    void UpdateCursor();                                             // 0x10073bc0 every frame
    void UpdateTargetWindow();                                       // 0x1006a9d0 (C1 only: no C2 match) every tick
    INT DispatchWndMsgX(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam); // 0x10075640
    UBOOL HookShortCutKey(UINT Msg, WPARAM wParam, LPARAM lParam);   // 0x1004f050 (C1 only: no C2 match) (C1 only: no C2 match)
    // the console's own text services and actions (names ours)
    void RemoveInvalidChars(TCHAR* Text);                            // 0x1005f350 the characters
                                                                     //   the font lacks
    AActor* GetPickupUnderMouse();                                   // 0x1005f5c0 an AL2Pickup
    void UseAction(INT ActionID, INT bForce = 0, INT bShift = 0);    // 0x10074ab0 an action slot (C2: ret 0xC: Ctrl and Shift)
    void UseShortCut(INT Slot, INT bForce = 0, INT bShift = 0);      // 0x10076020 (C2: ret 0xC) a shortcut slot
                                                                     //   (the keys, slot 263)

    // UObject's and FExec's
    virtual void Destroy();                                             // 10 0x10074870
    virtual UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar) { return 0; } // FExec 0x100016f0

    // UL2ConsoleWnd's, implemented here (called by the engine), in the order of the C2 vtable: each slot has the
    // name of include/ue2/gen/UL2ConsoleWnd.vt.inc (tools/ue2_data/l2consolewnd_slots_c2.csv says how it was
    // found). The UGameEngine::On* handler of FL2NetNotify that calls a slot names it (without "On") unless a
    // guard string does; names ours otherwise (vfnNNN: not known). The slots only C2 has are in NConsoleWndC2.cpp
    // (the manor's in NConsoleWndItems.cpp, 314 in NConsoleWndResize.cpp).
    virtual void Tick(FLOAT DeltaTime);                                 // 25 0x10075a10
    // WndMain->IsOverWindow
    virtual UBOOL IsOverWindow(INT X, INT Y);                           // 27 0x1005a6a0
    virtual void ShowWarningDialog(INT P1, INT P2);                                         // 28 0x10060870 (name ours)
    virtual void GameGuardQuery(INT P1, INT P2, INT P3, INT P4, INT P5); // 29 0x10061d00 (ret 0x14: five values)
    void GameGuardQueryX(INT P1, INT P2, INT P3, INT P4, INT P5);       // 0x10061d00 the body (C3: ret 0x14; the base's slot 29 has four)
    virtual void AuthLoginSuccess() {}                                  // 30 0x10001b00
    virtual void SCCheckStart(INT P1, INT P2);                                         // 31 0x10063350 (name ours)
    virtual void AuthLoginFail(INT Reason);                             // 32 0x10062fd0
    // the announcements (NCNoticeWnd)
    virtual void StartNotice(L2ParamStack& Params);                     // 33 0x10063370
    virtual void CharacterCreateFail(INT Reason);                       // 34 0x1005b260
    virtual void ReceiveCharacterInfo();                                // 35 0x100635e0
    void ReceiveCharacterInfoX(INT bNoVoice);                           // 0x100635e0 the body (C3: ret 4; the base's slot 35 has no argument)
    virtual void AddCharacterInfo(User Info);                           // 36 0x1005b360
    virtual void AcceptGameStart(INT Result);                           // 37 0x100636f0
    virtual void AcceptCharacterDelete(INT Result);                     // 38 0x1005b4a0
    virtual void AcceptNewCharacter(INT Result);                        // 39 0x100638a0
    virtual void AcceptCharacterCreate(INT Result);                     // 40 0x100639a0
    // the same body as QuitGame (321)
    virtual void AcceptLogOut();                                        // 41 0x1006c9f0
    virtual void ServerClose(INT Reason);                               // 42 0x1006ca00
    virtual void ReviveRequest(INT Param1, INT Param2);                 // 43 0x1006cb80
    // OnStartServerList
    virtual void ReceiveServerList(INT LastServerID);                   // 44 0x1005b040
    virtual void AddServerList(L2ParamStack& Params);                   // 45 0x1005b140
    virtual void EndServerList();                                       // 46 0x1005b1d0

    // the target, the inventory, the trade, the NPC shops and the warehouse
    virtual void MyTargetSelected(INT LevelDiff);                       // 47 0x10065320
    virtual void ReceiveItemList(INT bShow);                            // 48 0x1005bcb0
    virtual void ReceiveUpdateItemList();                               // 49 0x1005bd60 no handler
    virtual void ReceiveTradeStart(INT ObjectID);                       // 50 0x1006d3e0
    // the engine's ItemInfo, by value
    virtual void ReceiveAddTradeInventoryItem(NItemRequest Item);       // 51 0x1006d530
    virtual void ReceiveAddTradeOwnItem(NItemRequest Item);             // 52 0x1006d830
    virtual void ReceiveAddTradeOtherItem(NItemRequest Item);           // 53 0x1006db30
    virtual void ReceiveTradeUpdate(INT Change, ItemInfo& Item);        // 54 0x1006de30
    virtual void ReceiveTradeDoneMsg(INT Result);                       // 55 0x1005c160
    virtual void AddRecommandedData(RecommandedDataStr Data);           // 56 0x1005ba30
    virtual void EndRecommandedData();                                  // 57 0x1005bac0
    virtual void AddInventoryItem(ItemInfo& Item);                      // 58 0x1006cc50
    // OnReceiveUpdateItemList
    virtual void UpdateInventoryItem(INT Change, ItemInfo& Item);       // 59 0x1006cf80
    virtual void CheckGeo();                                            // 60 0x1005bde0 (no argument: the stock pops nothing)
    virtual void NewVirtual61();  // 61 0x1005c680 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual62();  // 62 0x1006ea20 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void StartBuyList(INT Adena, INT ListID);                   // 63 0x1005c550 NPC shop
    virtual void AddBuyList(ItemInfo& Item);                            // 64 0x1006e710
    virtual void StartSellList(INT Adena, INT ListID);                  // 65 0x1005c740
    virtual void AddSellList(ItemInfo& Item);                           // 66 0x1006ecf0
    virtual void StartManorBuyList(INT Adena, INT ListID);              // 67 0x1005c300
    virtual void AddManorBuyList(ItemInfo& Item);                       // 68 0x1006e130
    virtual void StartManorSellList(INT Adena, INT ListID);             // 69 0x1005c390
    virtual void AddManorSellList(ItemInfo& Item);                      // 70 0x1006e420
    virtual void StartDepositList(INT Adena);                           // 71 0x1005c830 warehouse
    virtual void AddDepositList(ItemInfo& Item);                        // 72 0x1006efe0
    virtual void StartWithdrawList(INT Adena);                          // 73 0x1005c8e0
    virtual void AddWithdrawList(ItemInfo& Item);                       // 74 0x1006f2d0
    // OnReceiveWareHouseDonePacket
    virtual void ReceiveWareHouseDone(L2ParamStack& Params);            // 75 0x10075ff0
    virtual void UpdatePlayerInfo(L2ParamStack& Params);                // 76 0x1005ee20

    // HTML pages, the tutorial, the radar
    virtual void NpcHtmlMessage(L2ParamStack& Params);                  // 77 0x10076c20
    virtual void TutorialShowHtml(TCHAR* Html);                         // 78 0x1005ddf0
    virtual void TutorialShowQuestionMark(INT ID);                      // 79 0x10065dd0
    virtual void TutorialEnableClientEvent(INT Events);                 // 80 0x1005df70
    virtual void TutorialCloseHtml();                                   // 81 0x1005df80
    // CheckTutorialClientEvent; once per pending bit (+0x3724)
    virtual void SendTutorialEvent(INT Event);                          // 82 0x1005df40
    virtual void ShowRadar(INT Param1, INT Param2, INT X, INT Y, INT Z); // 83 0x1005e5a0
    virtual void DeleteRadar(INT X, INT Y, INT Z);                      // 84 0x1005e770

    // the clan, the alliance, the friends, the party
    // empty, folded
    virtual void ReceivePledgeExtendedInfo(PledgeInfo& Info);           // 85 0x1012f080 (empty; traced in the standalone)
    virtual void ReceivePledgeInfo(PledgeInfo& Info);                   // 86 0x100653e0
    virtual void ReceivePledgeInfoUpdate(PledgeInfo& Info);             // 87 0x10065530
    virtual void ReceivePledgeMemberList();                             // 88 0x1005d580
    virtual void AddPledgeMember(L2ParamStack& Params);                 // 89 0x10065640
    virtual void UpdatePledgeMember(L2ParamStack& Params);              // 90 0x10065730
    virtual void DeletePledgeMember(TCHAR* Name);                       // 91 0x1005d610
    virtual void DeleteAllPledgeMember();                               // 92 0x1005d6a0
    virtual void ReceiveCreatePledge(L2ParamStack& Params);             // 93 0x100768c0
    virtual void ReceiveAskJoinPledge(L2ParamStack& Params);            // 94 0x10070570
    virtual void ReceiveOustPledgeMember(L2ParamStack& Params);         // 95 0x10076900
    virtual void ReceiveDismissPledge(L2ParamStack& Params);            // 96 0x10076940
    virtual void ReceiveJoinPledge(L2ParamStack& Params);               // 97 0x10076890
    // OnReceiveWidthdrawalPledge
    virtual void ReceiveWithdrawalPledge(L2ParamStack& Params);         // 98 0x10076980
    virtual void ReceiveOustPledgeMessage(L2ParamStack& Params);        // 99 0x100769c0
    virtual void ReceiveDismissPledgeMessage(L2ParamStack& Params);     // 100 0x100769e0
    virtual void ReceiveStartPledgeWar(L2ParamStack& Params);           // 101 0x10070680
    virtual void ReceiveReplyStartPledgeWar(L2ParamStack& Params);      // 102 0x10076a20
    virtual void ReceiveStopPledgeWar(L2ParamStack& Params);            // 103 0x100707a0
    virtual void ReceiveReplyStopPledgeWar(L2ParamStack& Params);       // 104 0x10076a60
    virtual void ReceiveSurrenderPledgeWar(L2ParamStack& Params);       // 105 0x10076aa0
    virtual void ReceiveSurrenderPersonally(L2ParamStack& Params);      // 106 0x10076ad0
    virtual void ReceiveAskJoinParty(L2ParamStack& Params);             // 107 0x10071030
    virtual void ReceiveJoinParty(L2ParamStack& Params);                // 108 0x10076b80
    virtual void ReceiveWithdrawalParty(L2ParamStack& Params);          // 109 0x1005da80
    virtual void ReceiveOustPartyMember(L2ParamStack& Params);          // 110 0x10076bb0
    virtual void ReceiveOustPartyMessage(L2ParamStack& Params);         // 111 0x10076be0
    virtual void ReceiveDismissParty(L2ParamStack& Params);             // 112 0x1005dac0
    // OnRecevieDismissPartyMessage
    virtual void ReceiveDismissPartyMessage(L2ParamStack& Params);      // 113 0x10076c00
    virtual void NewVirtual114();  // 114 0x1005db00 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void ReceiveAskJoinAlly(L2ParamStack& Params);              // 115 0x100708c0
    virtual void ReceiveDismissAlly(L2ParamStack& Params);              // 116 0x1005d830
    virtual void ReceiveJoinAlly(L2ParamStack& Params);                 // 117 0x10076b00
    virtual void ReceiveOustAlly(L2ParamStack& Params);                 // 118 0x1005d7f0
    virtual void ReceiveWithdrawAlly(L2ParamStack& Params);             // 119 0x1005d7b0
    virtual void ReceiveFriendAddRequestResult(L2ParamStack& Params);   // 120 0x10076b40
    virtual void ReceiveFriendAddRequest(L2ParamStack& Params);         // 121 0x10070f70
    // empty, folded
    virtual void ReceiveFriendAdd(L2ParamStack& Params) {}              // 122 0x1012f080
    // empty, folded
    virtual void ReceiveFriendRemove(L2ParamStack& Params) {}           // 123 0x1012f080
    // empty, folded
    virtual void ReceiveFriendList(L2ParamStack& Params) {}             // 124 0x1012f080
    // empty, folded
    virtual void ReceiveFriendStatus(L2ParamStack& Params) {}           // 125 0x1012f080

    // new in C2 (recipes, hennas, the minimap, ...): nobody has named them yet, except the two UL2NetHandler forwarders
    virtual void ReceiveRecipeBookItemList(INT P1);                                         // 126 0x10063f60 (name ours)
    virtual void AddRecipeBookItem(INT RecipeID);                                         // 127 0x1006f860 (name ours)
    virtual void ReceiveRecipeItemMakeInfo(INT P1, INT P2, INT P3, INT P4);                                         // 128 0x10064020 (name ours)
    virtual void ReceiveRecipeShopManageList(INT P1, INT P2);                                         // 129 0x10064140 (name ours)
    virtual void AddRecipeShopManageItem(INT RecipeID, INT P2);                                         // 130 0x1006f990 (name ours)
    virtual void AddRecipeShopRegisterdItem(INT RecipeID, INT P2, INT P3);                                         // 131 0x1006fad0 (name ours)
    virtual void ReceiveRecipeShopSellList(INT P1, INT P2, INT P3, INT P4);                                         // 132 0x100641a0 (name ours)
    virtual void AddRecipeShopSellItem(INT RecipeID, INT P2, INT P3);                                         // 133 0x1006fc20 (name ours)
    virtual void ReceiveRecipeShopItemInfo(INT P1, INT P2, INT P3, INT P4, INT P5);                                         // 134 0x10064290 (name ours)
    void ReceiveRecipeShopItemInfoX(INT P1, INT P2, INT P3, INT P4, INT P5, INT P6);   // 0x10064290 the body (C3: ret 0x18; the base's slot 134 has five)
    virtual void ReceiveMacroListInfo(char P1, char P2, MacroInfo Macro, char P4, INT P5);                                         // 135 0x1006fd60 (name ours)
    virtual void ReceiveHennaEquipList(INT P1, INT P2);                                         // 136 0x100643e0 (name ours)
    virtual void AddHennaEquipInfo(HennaInfo Info);                                         // 137 0x100644c0 (name ours)
    virtual void ReceiveHennaItemInfo(HennaInfo Info, INT P2);                                         // 138 0x100645e0 (name ours)
    virtual void ReceiveHennaInfo(HennaInfo Info, INT P2);                                         // 139 0x100646f0 (name ours)
    virtual void AddHennaInfo(INT P1, INT P2);                                         // 140 0x1005d150 (name ours)
    virtual void ReceiveHennaUnequipList(INT P1, INT P2);                                         // 141 0x10064730 (name ours)
    virtual void AddHennaUnequipInfo(HennaInfo Info);                                         // 142 0x10064810 (name ours)
    virtual void ReceiveHennaUnequipItemInfo(HennaInfo Info, INT P2);                                         // 143 0x10064970 (name ours)
    virtual void RequestHennaItemList();                                // 144 0x1005d1d0
    virtual void RequestHennaUnequipList();                             // 145 0x1005d1e0
    virtual void GMAddHennaInfo(INT P1, INT P2);                                         // 146 0x1005d170 (name ours)
    virtual void GMReceiveHennaInfo(HennaInfo Info, INT P2);                                         // 147 0x10064930 (name ours)
    virtual void ReceiveBlockList(L2ParamStack& Params) {}              // 148 0x1012f080

    // the inventory and character toggles, the shortcut keys and the Ctrl cursor (C1 slots 261-270 moved here)
    virtual void ToggleInventory();                                     // 149 0x1005d1f0
    // no handler; Desktop->ToggleCharacter(0)
    virtual void ShowCharacterStatus();                                 // 150 0x1005d230
    virtual void UseShortCutSlot(INT Index);                            // 151 0x10078e70
    virtual void ChangeShortCutPage(INT bNext);                         // 152 0x1005d240
    // no handler; the pawn after (or before) CreatureID among the pawns in an area (ULevel slot 57 in C2)
    virtual User* FindNextPawnInArea(FLOAT Radius, INT CreatureID, INT bNext, INT P4, INT P5, INT P6, INT P7); // 153 0x1006adb0
    // the desktop's view rectangle (NCConsole +0x1C0..0x1CC in C2: left, top, width, height)
    virtual void SetViewRect(INT Left, INT Top, INT Right, INT Bottom); // 154 0x1006af50
    // bCtrlDown = 1
    virtual void PressCtrl();                                           // 155 0x1005f0e0
    // bCtrlDown = 0
    virtual void ReleaseCtrl();                                         // 156 0x1005f0f0

    // shortcuts, buffs, quests, chat, party, skills, system messages
    virtual void ReceiveShortCutList();                                 // 157 0x1005d290
    virtual void AddShortCutItem(L2ParamStack& Params);                 // 158 0x100700a0
    virtual void UpdateShortCutItem(INT ObjectID);                      // 159 0x1005d2d0
    virtual void UpdatePetShortCutItem(INT ObjectID);                   // 160 0x1005d2f0
    virtual void ReceiveShortCutDelete(INT Slot);                       // 161 0x1005d310
    virtual void UpdateMacroShortCut(INT P1, INT P2);                                         // 162 0x1005d2b0 (name ours)
    virtual void ReceiveMagicSkillList(L2ParamStack& Params);           // 163 0x1005d060
    virtual void ReceiveAbnormalStatus();                               // 164 0x1005d330
    virtual void AddAbnormalStatus(L2ParamStack& Params);               // 165 0x10064a80
    virtual void NewVirtual167();  // 167 0x10064d20 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual169();  // 169 0x10064bd0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void ReceiveQuestList();                                    // 170 0x1005ca80
    virtual void NewVirtual171();  // 171 0x1005cb10 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void AddQuestID(INT QuestID, INT Step);                     // 172 0x1005cba0
    virtual void AddQuestItem(ItemInfo& Item);                          // 173 0x1012f080
    // a chat line
    virtual UBOOL Say2(L2ParamStack& Params);                           // 174 0x10073630
    // no handler; the debug window
    virtual void VARARGS DebugLogf(INT Category, const TCHAR* Fmt, ...); // 175 0x1005ec20
    virtual void ReceiveVersionCheck(L2ParamStack& Params);             // 176 0x1005b5f0
    virtual void UpdateEquipItem(L2ParamStack& Params);                 // 177 0x1005bf00
    // OnPartyAddMemberPacket
    virtual void PartyAddMember(L2ParamStack& Params);                  // 178 0x1005dc00
    virtual void PartyUpdateMember(L2ParamStack& Params);               // 179 0x1005dcc0
    virtual void PartyDeleteMember(L2ParamStack& Params);               // 180 0x1005dd40
    virtual void PartyDeleteAll(L2ParamStack& Params);                  // 181 0x1005dd80
    void ReceivePartyMatchList();                               // C3: slot gone (was 174), no longer virtual
    void AddPartyMatchMember(L2ParamStack& Params);             // C3: slot gone (was 175), no longer virtual
    void ReceivePartyMatchDetail(L2ParamStack& Params);         // C3: slot gone (was 176), no longer virtual
    // empty, folded
    virtual void ShowMagicWnd() {}                                      // 182 0x10001b00
    virtual void ShowSkillWnd();                                        // 183 0x1005d070
    // OnSkillListPacket
    virtual void ReceiveSkillList(L2ParamStack& Params);                // 184 0x10063e20
    // OnMagicListPacket; empty, folded
    virtual void ReceiveMagicList(L2ParamStack& Params) {}              // 185 0x1012f080
    virtual void ReceiveTradeDemand(L2ParamStack& Params);              // 186 0x1006d2d0
    // OnRestartResponse, first
    virtual void RestartResetWindows();                                 // 187 0x1005b6a0
    // OnRestartResponse, then
    virtual void RestartToLobby();                                      // 188 0x1005b700
    virtual void AddSystemMessageParam(L2ParamStack& Params);           // 189 0x10076cf0
    virtual void EndSystemMessageParam(L2ParamStack& Params, INT P2, INT P3); // 190 0x10076de0

    // board, enchant, skill trainer, restart, VIP shop, chat channels, pets
    // OnShowboardPacket
    virtual void ShowBoard(L2ParamStack& Params);                       // 191 0x1005de40
    // NetHandler 164
    virtual void NewVirtual192();  // 192 0x1005eeb0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void BuilderCommand(L2ParamStack& Params);                  // 193 0x1005eea0
    virtual void SelectItemToEnchant();                                 // 194 0x1005be00
    virtual void ReceiveEnchantResult(INT Result);                      // 195 0x1005beb0
    virtual void ReceiveAction(INT Action);                             // 196 0x10070500
    virtual void ShowTrainListWnd();                                    // 197 0x1005cf80
    virtual void ShowTrainInfoWnd(L2ParamStack& Params);                // 198 0x10078bf0
    virtual void AddAcquireSkillInfo(L2ParamStack& Params);             // 199 0x10063c50
    virtual void AddAcquireSkillExtendedInfo(L2ParamStack& Params);     // 200 0x1006f5c0
    // no handler; the restart points after dying
    virtual void ShowRestartMenu(L2ParamStack& Params);                 // 201 0x10063a60
    virtual void TeleportToLocation();                                  // 205 0x1005b970
    void HideTrainWnd();                                        // 204 0x1005cff0 (C2: slot 197) - virtual: the base declares it at 204 (s5-prep: the NewVirtual204 placeholder that also pointed here was a second slot, appended at 390)
    virtual void NewVirtual207();  // 207 0x1005b990 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void SetVIPShopID(INT ShopID);                              // 208 0x1005e9a0
    virtual void AddVIPItemList(INT ProductID, INT ItemID, INT BodyPart, INT Type, INT Count, INT Param6); // 209 0x10073350 (C3: ret 0x18)
    virtual void AddVIPNeedItemList(INT ProductID, INT ItemID, INT Count, INT Param4); // 210 0x10073550 (C3: ret 0x10)
    virtual void VIPItemListEnd();                                      // 211 0x1005e9c0
    // NetHandler 186
    virtual void RequestBuyVIPItem(L2ParamStack& Params);               // 212 0x1005ea00
    virtual void EnablePartyChat(bool bEnable);                         // 213 0x1005ea10
    virtual void EnablePledgeChat(bool bEnable);                        // 214 0x1005ea30
    virtual void EnableAllyChat(bool bEnable);                          // 215 0x1005ea50
    virtual void ShowPetStatusWnd(INT Type);                            // 216 0x100711e0
    virtual void ShowPetPartyWnd(INT Type);                             // 217 0x1005e020
    virtual void HideSummonWnd();                                       // 218 0x10071250 no handler
    virtual void HidePetWnd();                                          // 219 0x100712e0
    virtual void ShowPetNameBtn(INT bShow);                             // 220 0x1005e0d0
    virtual void ReceivePetStatusInfo(INT Type, L2ParamStack& Params);  // 221 0x1005e0f0
    virtual void ReceivePetItemList();                                  // 222 0x1005e110
    virtual void AddPetInventoryItem(ItemInfo& Item);                   // 223 0x10071370
    virtual void UpdatePetInventoryItem(INT Change, ItemInfo& Item);    // 224 0x10071680
    // OnSkillCoolTimePacket
    virtual void SkillCoolTime(INT Param1, INT Param2, INT Param3, INT Param4); // 225 0x1006aae0

    // alliance wars and sieges (the Request* pass the call on to the network handler)
    // NetHandler 174; no caller in NWindow
    virtual void vfn176(L2ParamStack& Params);                          // 226 0x10060140
    virtual void MacroSkillUse(INT P1, INT P2);                                         // 227 0x1005d420 (name ours)
    virtual void MacroTargetSelected();                                         // 228 0x1005d460 (name ours)
    virtual void MacroItemPickedUp();                                         // 229 0x1005d440 (name ours)
    virtual void RequestStartAllianceWar(L2ParamStack& Params);         // 230 0x1005d870
    // NetHandler 176
    virtual void RequestReplyStartAllianceWar(L2ParamStack& Params);    // 231 0x1005d880
    // NetHandler 177
    virtual void RequestStopAllianceWar(L2ParamStack& Params);          // 232 0x1005d890
    // NetHandler 178
    virtual void RequestReplyStopAllianceWar(L2ParamStack& Params);     // 233 0x1005d8a0
    // NetHandler 179
    virtual void RequestSurrenderAllianceWar(L2ParamStack& Params);     // 234 0x1005d8b0
    virtual void StartAllianceWar(TCHAR* AllianceName, TCHAR* Name);    // 235 0x100709e0
    virtual void StopAllianceWar(TCHAR* AllianceName, TCHAR* Name);     // 236 0x10070b00
    // NetHandler 180; no caller in NWindow
    virtual void vfn184(L2ParamStack& Params);                          // 237 0x1005d9a0
    // NetHandler 181
    virtual void RequestSiegeAttackerList(L2ParamStack& Params);        // 238 0x1005d9b0
    // NetHandler 182
    virtual void RequestSiegeDefenderList(L2ParamStack& Params);        // 239 0x1005d9c0
    // NetHandler 183
    virtual void RequestJoinSiege(L2ParamStack& Params);                // 240 0x1005d9d0
    // NetHandler 184
    virtual void RequestConfirmSiegeWaitingList(L2ParamStack& Params);  // 241 0x1005d9e0
    // NetHandler 185
    virtual void RequestSetSiegeTime(L2ParamStack& Params);             // 242 0x1005d9f0
    virtual void ReceiveSiegeInfo(L2ParamStack& Params);                // 243 0x10065820
    virtual void ReceiveSiegeAttackerList(L2ParamStack& Params);        // 244 0x10065940
    virtual void ReceiveSiegeDefenderList(L2ParamStack& Params);        // 245 0x10065a30
    virtual void AddSelectableTime(INT Time);                           // 246 0x10065b30
    virtual void EndSiegeAttackerList();                                // 247 0x10065b50
    virtual void EndSiegeDefenderList();                                // 248 0x10065b70
    virtual void ClearAttackerList();                                   // 249 0x1005da00
    virtual void ClearDefenderList();                                   // 250 0x1005da20

    // packages, GM views, minimap, private stores
    // NetHandler 187
    virtual void RequestPackageSend(L2ParamStack& Params);              // 251 0x1005d8c0
    // NetHandler 188
    virtual void RequestPackageSendableItemList(L2ParamStack& Params);  // 252 0x1005d8d0
    virtual void AddPackageToList(L2ParamStack& Params);                // 253 0x1005d8e0
    virtual void AddSendableItemList(INT Index, INT TargetID, INT Adena, ItemInfo& Item); // 254 0x10070c20
    virtual void GMViewCharacterInfo(User& Info);                       // 255 0x1005e220
    virtual void GMViewReceivePledgeInfo(PledgeInfo& Info);             // 256 0x10065e50
    // guard ReceivePledgeMemberList
    virtual void GMViewReceivePledgeMemberList();                       // 257 0x1005e250
    // guard AddPledgeMember
    virtual void GMViewAddPledgeMember(L2ParamStack& Params);           // 258 0x10065f90
    virtual void GMViewShowSkillWnd();                                  // 259 0x1005e2e0
    // OnGMViewSkillListPacket
    virtual void GMViewSkillList(L2ParamStack& Params);                 // 260 0x10066080
    // empty, folded
    virtual void GMViewShowMagicWnd() {}                                // 261 0x10001b00
    // OnGMViewMagicListPacket
    virtual void GMViewMagicList(L2ParamStack& Params);                 // 262 0x100661c0
    // guard ReceiveQuestList
    virtual void GMViewReceiveQuestList();                              // 263 0x1005e300
    // guard AddQuestID
    virtual void GMViewAddQuestID(INT QuestID, INT Step);               // 264 0x1005e380
    // guard AddQuestItem
    virtual void GMViewAddQuestItem(ItemInfo& Item);                    // 265 0x1005e440
    // guard ReceiveItemList
    virtual void GMViewReceiveItemList(INT bShow);                      // 266 0x1005e470
    // guard AddInventoryItem
    virtual void GMViewAddInventoryItem(ItemInfo& Item);                // 267 0x100719a0
    virtual void GMViewStartWithdrawList(INT Adena);                    // 268 0x1005e520
    virtual void GMViewAddWithdrawList(ItemInfo& Item);                 // 269 0x10071cd0
    // OnShowMinimap
    void ShowMapItem(INT ItemID);                               // C3: slot gone (was 260), no longer virtual
    virtual void NewVirtual271();  // 271 0x100662f0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual272();  // 272 0x1005e7c0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void ShowCalc(INT Code);                                         // 273 0x1005e820 (name ours)
    // the same body as StartPrivateStoreList (263)
    virtual void StartPrivateStoreManageList();                         // 274 0x1005e8e0
    virtual void StartPrivateStoreList();                               // 275 0x1005e8e0
    virtual void PrivateStoreManageList(ItemInfo& InventoryItem, ItemInfo& StoreItem, INT OwnerID, INT Adena); // 276 0x10071fa0
    virtual void PrivateStoreList(ItemInfo& Item, INT OwnerID, INT Adena); // 277 0x10072620
    // also OnPrivateStoreBuyMsg; empty
#if NW_HOOK_MODE
    virtual void PrivateStoreMsg(TCHAR* Text, INT Param) {}             // 278 0x10003400
#else
    virtual void PrivateStoreMsg(TCHAR* Text, INT Param);               // 278: NW_SLOT_IS of EmptyTwoArgs 0x10003400 (NLeftHelpers.cpp)
#endif
    // the same body as StartPrivateStoreBuyList (268)
    virtual void StartPrivateStoreBuyManageList();                      // 279 0x1005e970
    virtual void StartPrivateStoreBuyList();                            // 280 0x1005e970
    virtual void PrivateStoreBuyManageList(ItemInfo& InventoryItem, ItemInfo& StoreItem, INT OwnerID, INT Adena); // 281 0x100729a0
    virtual void PrivateStoreBuyList(ItemInfo& Item, INT OwnerID, INT Adena); // 282 0x10072fc0

    // the rest of the interface
    virtual void BlockedAccountWithMsg(TCHAR* Msg);                     // 283 0x10062c40
    virtual void BlockedAccount(INT Reason);                            // 284 0x10062db0
    virtual void SetSummonRemainTime(INT Remaining, INT Total);         // 285 0x10065e20
    // bCtrlDown || bCtrlDownAlt
    UBOOL IsCtrlDown();                                         // C3: slot gone (was 274), no longer virtual
    virtual void NewVirtual286();  // 286 0x10060f10 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual287();  // 287 0x10060ed0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual288();  // 288 0x10061020 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual289();  // 289 0x1006b170 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual290();  // 290 0x1006b330 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual292();  // 292 0x1006b400 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual294();  // 294 0x1006b4b0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual295();  // 295 0x1006b5c0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual296();  // 296 0x1006b660 (only this client has it; defined in NNewVirtuals.gen.cpp)
    // slot 297 0x10066410 (C3 only; name ours): not declared virtual (a virtual of ours the base lacks lands after the base's
    // slots, not at 297): the callers use nw::VirtualCall<UBOOL>(this, 297)
    UBOOL IsAttackKeyDown();
    // The C3-only virtuals ported below (names ours): not declared virtual (a virtual of ours that the base lacks lands after
    // the base's slots, not at its C3 number); hooked by their addresses, called by slot (nw::VirtualCall) when needed
    void C3Virtual166();   // 0x1005D350 (C3-only virtual 166, not declared virtual: see above; name ours) the buff window's 0x10102BA0 (name unknown)
    void C3Virtual168();   // 0x1005D370 (C3-only virtual 168, not declared virtual: see above; name ours) the buff window's 0x10102B30 (name unknown)
    void C3Virtual202();   // 0x1005B950 (C3-only virtual 202, not declared virtual: see above; name ours) the NCGodWnd's restart menu hidden
    void C3Virtual203();   // 0x1005B960 (C3-only virtual 203, not declared virtual: see above; name ours) the NCGodWnd's restart menu shown
    void C3Virtual206();   // 0x1005B980 (C3-only virtual 206, not declared virtual: see above; name ours) the loading screen shown
    void C3Virtual270(INT Mode);   // 0x1005E590 (C3-only virtual 270, not declared virtual: see above; name ours) the desktop's ToggleMinimap
    void C3Virtual291();   // 0x10061140 (C3-only virtual 291, not declared virtual: see above; name ours) the messenger window's 0x10139670 (name unknown)
    void C3Virtual293();   // 0x10061160 (C3-only virtual 293, not declared virtual: see above; name ours) the messenger window's 0x10131530 (name unknown)
    void C3Virtual346();   // 0x10060BC0 (C3-only virtual 346, not declared virtual: see above; name ours) the desktop hidden
    void C3Virtual347();   // 0x10060BE0 (C3-only virtual 347, not declared virtual: see above; name ours) the desktop shown
    void C3Virtual349();   // 0x1006B6A0 (C3-only virtual 349, not declared virtual: see above; name ours) the petition window's +0xDC child enabled
    void C3Virtual350(INT A, INT B, INT C);   // 0x1005BC90 (C3-only virtual 350, not declared virtual: see above; name ours) the zone title window's 0x1010B320 with the three values
    UBOOL C3Virtual351();   // 0x10077C30 (C3-only virtual 351, not declared virtual: see above; name ours) a flag of the character window's quest page (+0xDC)
    UBOOL C3Virtual352();   // 0x10077C60 (C3-only virtual 352, not declared virtual: see above; name ours) the NCGodWnd's 0x10085C50 as a flag
    UBOOL C3Virtual353();   // 0x10077C80 (C3-only virtual 353, not declared virtual: see above; name ours) a flag of the character window's quest page (+0xDD)
    void C3Virtual354(INT bValue);   // 0x10077CB0 (C3-only virtual 354, not declared virtual: see above; name ours) sets the quest page's flag at +0xDD
    void C3Virtual357(INT A, INT B, INT C);   // 0x10077E00 (C3-only virtual 357, not declared virtual: see above; name ours) the quest page's 0x100DA3A0 with three values
    void C3Virtual374(L2ParamStack& Params);   // 0x10061DA0 (C3-only virtual 374, not declared virtual: see above; name ours) the party matching window's 0x100A5B10
    void C3Virtual375();   // 0x10061DC0 (C3-only virtual 375, not declared virtual: see above; name ours) the party matching window shown, focused and refreshed (0x100A4DE0)
    void C3Virtual376(L2ParamStack& Params);   // 0x10061E00 (C3-only virtual 376, not declared virtual: see above; name ours) the party room window's 0x100A5450
    void C3Virtual377(L2ParamStack& Params);   // 0x10061E20 (C3-only virtual 377, not declared virtual: see above; name ours) the party room window's 0x100A6EA0
    void C3Virtual379(L2ParamStack& Params);   // 0x10061E80 (C3-only virtual 379, not declared virtual: see above; name ours) the party room window's 0x100A75B0
    void C3Virtual382(INT A);   // 0x10062300 (C3-only virtual 382, not declared virtual: see above; name ours) the broadcast GM window shown and given the value (0x1010DD50)
    void C3Virtual383(L2ParamStack& Params);   // 0x10062340 (C3-only virtual 383, not declared virtual: see above; name ours) the broadcast GM window's 0x1010E110
    void C3Virtual385();   // 0x1005BA00 (C3-only virtual 385, not declared virtual: see above; name ours) the desktop and the window at +0x39E0 hidden
    void C3Virtual386();   // 0x1005BA20 (C3-only virtual 386, not declared virtual: see above; name ours) the window at +0x39E0 shown
    INT C3Virtual389();   // 0x10077C20 (C3-only virtual 389, not declared virtual: see above; name ours) the loading picture's id
    // the engine's pure slots this console implements with a non-virtual function (the C3Virtual<N> above, IsAttackKeyDown): each is
    // a placeholder (NNewVirtuals.gen.cpp) that jumps to the stock function in the hook build and is an alias of that very function
    // in the standalone (NW_SLOT_IS); without them the standalone NConsoleWnd is abstract
    virtual void NewVirtual166();  // 166 0x1005d350 (the engine class declares it unnamed; the stock slot is C3Virtual166; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual168();  // 168 0x1005d370 (the engine class declares it unnamed; the stock slot is C3Virtual168; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual202();  // 202 0x1005b950 (the engine class declares it unnamed; the stock slot is C3Virtual202; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual203();  // 203 0x1005b960 (the engine class declares it unnamed; the stock slot is C3Virtual203; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual206();  // 206 0x1005b980 (the engine class declares it unnamed; the stock slot is C3Virtual206; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual270();  // 270 0x1005e590 (the engine class declares it unnamed; the stock slot is C3Virtual270; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual291();  // 291 0x10061140 (the engine class declares it unnamed; the stock slot is C3Virtual291; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual293();  // 293 0x10061160 (the engine class declares it unnamed; the stock slot is C3Virtual293; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual297();  // 297 0x10066410 (the engine class declares it unnamed; the stock slot is IsAttackKeyDown; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual346();  // 346 0x10060bc0 (the engine class declares it unnamed; the stock slot is C3Virtual346; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual347();  // 347 0x10060be0 (the engine class declares it unnamed; the stock slot is C3Virtual347; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual349();  // 349 0x1006b6a0 (the engine class declares it unnamed; the stock slot is C3Virtual349; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual350();  // 350 0x1005bc90 (the engine class declares it unnamed; the stock slot is C3Virtual350; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual351();  // 351 0x10077c30 (the engine class declares it unnamed; the stock slot is C3Virtual351; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual352();  // 352 0x10077c60 (the engine class declares it unnamed; the stock slot is C3Virtual352; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual353();  // 353 0x10077c80 (the engine class declares it unnamed; the stock slot is C3Virtual353; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual354();  // 354 0x10077cb0 (the engine class declares it unnamed; the stock slot is C3Virtual354; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual357();  // 357 0x10077e00 (the engine class declares it unnamed; the stock slot is C3Virtual357; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual374();  // 374 0x10061da0 (the engine class declares it unnamed; the stock slot is C3Virtual374; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual375();  // 375 0x10061dc0 (the engine class declares it unnamed; the stock slot is C3Virtual375; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual376();  // 376 0x10061e00 (the engine class declares it unnamed; the stock slot is C3Virtual376; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual377();  // 377 0x10061e20 (the engine class declares it unnamed; the stock slot is C3Virtual377; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual379();  // 379 0x10061e80 (the engine class declares it unnamed; the stock slot is C3Virtual379; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual382();  // 382 0x10062300 (the engine class declares it unnamed; the stock slot is C3Virtual382; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual383();  // 383 0x10062340 (the engine class declares it unnamed; the stock slot is C3Virtual383; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual385();  // 385 0x1005ba00 (the engine class declares it unnamed; the stock slot is C3Virtual385; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual386();  // 386 0x1005ba20 (the engine class declares it unnamed; the stock slot is C3Virtual386; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual389();  // 389 0x10077c20 (the engine class declares it unnamed; the stock slot is C3Virtual389; defined in NNewVirtuals.gen.cpp)
    virtual void VARARGS MessageBoxf(const TCHAR* Caption, const TCHAR* Fmt, ...); // 298 0x1005ebd0 no handler
    virtual UBOOL Init(UEngine* InEngine);                              // 303 0x10078ab0
    virtual INT ConsolePostRender(UCanvas* Canvas);                     // 304 0x10075c80
    virtual INT DispatchWndMsg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam); // 305 0x10078b60
    virtual void NewVirtual306();  // 306 0x10062b90 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual UBOOL NpcSay(L2ParamStack& Params);                         // 307 0x1005a6c0
    virtual UBOOL MasterConsoleEventProcess(INT Key, INT Action, FLOAT Delta); // 308 0x10075d50
    // "L2Debug", "L2DebugWindow"
    virtual UBOOL DebugCommand(const TCHAR* Cmd);                       // 309 0x10060360
    virtual UBOOL InitForPawnViewer(UEngine* InEngine);                 // 321 0x10077770
    virtual void PawnViewerFunction(INT Function);                      // 322 0x10060030
    // GMalloc'd array of the item ids
    virtual void NewVirtual325();  // 325 0x10077820 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual INT* GetInventoryItemIDs(INT& Count);                       // 326 0x1006a940

    // the last pure slots: C1's Free and SetServerPing, the rest C2 additions (RequestLogout and ShowMessageWnd, non-virtual in C1, are here)
    // folded with operator delete
    virtual void Free(void* Ptr);                                       // 327 0x10029fa0
    virtual INT GetPartyMemberCount();                                         // 328 0x1005b420 (name ours)
    virtual void SetServerPing(INT ServerID, DWORD Ping);               // 330 0x10060600
    virtual void Snoop(L2ParamStack& Params);                                         // 331 0x1005ea80 (name ours)
    virtual void SetPadHoldL(BYTE bHold);                                         // 332 0x10060940 (name ours)
    virtual void SetPadHoldR(BYTE bHold);                                         // 333 0x10060960 (name ours)
    virtual void vfn271(INT Param) {}                                   // 334 0x1012f080
    virtual void ShowTownMap(L2ParamStack& Params);                                         // 335 0x10060980 (name ours)
    virtual void ObserverStart();                                         // 336 0x100609f0 (name ours)
    virtual void ObserverEnd();                                         // 337 0x10060a20 (name ours)
    virtual void PledgePower(INT P1, INT P2, BYTE* P3);                                         // 338 0x10060a70 (name ours)
    virtual void ResizeWindows();                                         // 339 0x10069150 (name ours)
    virtual void RequestLogout();                                       // 340 0x1005aaa0
    virtual void ShowQuitDialog();                                         // 341 0x1006afa0 (name ours)
    virtual void ShowMessageWnd(const TCHAR* Text);                     // 342 0x1005f100
    virtual void vfn318(INT P1) {}                                         // 343 0x1012f080 (name ours)
    virtual void NewVirtual344();  // 344 0x1006b0c0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual345();  // 345 0x10060c90 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void ShowXMasSeal(INT P1);                                         // 348 0x10060c00 (name ours)

    // NConsoleWnd's own
    virtual void NewVirtual355();  // 355 0x10077ce0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual356();  // 356 0x10077db0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual358();  // 358 0x100611b0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual359();  // 359 0x10075030 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual360();  // 360 0x100611e0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual361();  // 361 0x100612f0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual362();  // 362 0x10061390 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual363();  // 363 0x10061470 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual364();  // 364 0x100615e0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual365();  // 365 0x100616b0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual366();  // 366 0x10061760 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual367();  // 367 0x10061840 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual368();  // 368 0x100618e0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual369();  // 369 0x10061980 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual370();  // 370 0x10061a10 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual371();  // 371 0x10061aa0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual372();  // 372 0x10061b30 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual373();  // 373 0x10061bd0 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual378();  // 378 0x10061e40 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual380();  // 380 0x10062110 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual381();  // 381 0x10075140 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual384();  // 384 0x10061c70 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual387();  // 387 0x1006b700 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void NewVirtual388();  // 388 0x1006b840 (only this client has it; defined in NNewVirtuals.gen.cpp)
    virtual void RequestServerList();                                   // 390 0x1005af40
    // the same body as AcceptLogOut (41)
    virtual void QuitGame();                                            // 391 0x1006c9f0
    virtual void ReceiveServerClose();                                  // 392 0x1005b560
    // NetHandler 76
    virtual void AnswerReviveRequest(UBOOL bYes, INT Param1, INT Param2); // 393 0x1005b8a0
    // NetHandler 171
    virtual void RequestEnchantItem(INT ObjectID);                      // 394 0x1005be30
    // NetHandler 165
    virtual void RequestDestroyItem(NItemRequest Request);              // 395 0x1005bf20
    // NetHandler 166
    virtual void RequestCrystallizeItem(INT ObjectID, INT Count);       // 396 0x1005bfb0
    // NetHandler 156
    virtual void AnswerTradeRequest(L2ParamStack& Params);              // 397 0x1005c150
    // NetHandler 169; 0 without a server
    virtual UBOOL RequestQuestList();                                   // 398 0x1005c960
    // NetHandler 170
    virtual void RequestDestroyQuest(INT QuestID);                      // 399 0x1005ca00
    // NetHandler 172
    virtual void RequestAcquireSkillInfo(L2ParamStack& Params);         // 400 0x1005cfd0
    // NetHandler 173
    virtual void RequestAcquireSkill(L2ParamStack& Params);             // 401 0x1005cfe0
    // "bypass [-h] ..."; 2: close the page
    virtual INT RunHtmlCommand(TCHAR* Command, NCHtmlViewer* Viewer);   // 402 0x10065b90
    virtual void RequestObserverEndPacket(L2ParamStack& Params);        // 403 0x10060a40 (C2 only: a tail call of UL2NetHandler's slot of the same name)
    virtual void RequestPledgePower(INT P1, INT P2, BYTE* P3);          // 404 0x10060a50 (C2 only: a tail call of UL2NetHandler's slot of the same name)

    // ---- console-core ----
    __declspec(noinline) void PlayVoiceName(const TCHAR* Name);         // 0x10060b00 (name ours; new in C3) the voice of a system message

    // ---- wave 3 (nconsole lane) ----
    // The C3-only virtuals ported here are NOT declared virtual (the NewVirtualN placeholders above stay for the vtable's
    // sake and jump to the stock address, which the hook replaces): hooked by address, called by slot when needed.
    // Names ours unless a guard string gave them.
    INT RequestPreviewItem(L2ParamStack& Params);                    // 0x1005c600 (guard name) NetHandler slot 96: the try-on list
    void StartPreviewList(INT OwnerID, INT Adena, INT ListID);       // 0x1005c680 (C3 virtual 61) the try-on shop opened by an NPC
    void AddPreviewItem(ItemInfo& Item);                             // 0x1006ea20 (C3 virtual 62) a good of the try-on shop
    void EndQuestList();                                             // 0x1005cb10 (guard name; C3 virtual 171)
    TCHAR* GetQuestName(INT QuestID);                                // 0x1005cc60 (guard name) questname-*.dat
    TCHAR* GetCurrentZoneName();                                     // 0x1005cdf0 (guard name)
    INT GetCurrentZoneID();                                          // 0x1005ce70 (guard name)
    INT GetZoneID(FLOAT X, FLOAT Y, FLOAT Z);                        // 0x1005cef0 (guard name)
    void ReceiveBuffList(L2ParamStack& Params);                      // 0x1005db00 (C3 virtual 114) the buffs of a party member, the pet or the summon
    void ShowMapPosition(INT X, INT Y, INT Z);                       // 0x1005e7c0 (C3 virtual 272) the minimap on that place
    void ShowMyMapPosition();                                        // 0x100662f0 (C3 virtual 271) the minimap on the player
    void SendBypassBuildCmd(FString Command);                        // 0x1005eeb0 (guard name; C3 virtual 192) a builder command as a parameter stack
    void GetSkillCoolTime(INT SkillID, DOUBLE* Total, DOUBLE* Remaining); // 0x10060150 (name ours) the skill window's cool time of a skill
    UBOOL IsQuickUseItem(INT ItemID);                                // 0x10060190 (name ours) potions and shots: the key repeats
    void RequestAutoShot(L2ParamStack& Params);                      // 0x10062360 (name ours) NetHandler slot 275, a tail call
    void SendConfirmDlg(L2ParamStack& Params);                       // 0x10060b40 (guard name) NetHandler slot 260
    void SetLoadingTextureName(FString Name);                        // 0x1005b990 (C3 virtual 207) the loading picture by its texture's name
    void MakeItemInfo(NItemEntry* Entry);                            // 0x10073aa0 (guard name) the texts of an entry from the item table
    void OnEventMatchMessage(INT Kind, FString Message);              // 0x100611e0 (guard name; C3 virtual 360) the match's text to the observer
    void OnEventMatchScore(INT A, INT B, INT C);                     // 0x100612f0 (guard name; C3 virtual 361)
    void OnEventMatchTeamInfoStart(INT A, INT B, INT C);             // 0x10061390 (C3 virtual 362; the stock's guard says "NConsoelWnd")
    void OnEventMatchTeamInfo(INT A, INT B, FString Name, INT P1, INT P2, INT P3, INT P4, INT P5, INT P6, INT P7, INT P8, INT P9, INT P10); // 0x10061470 (C3 virtual 363)
    void OnEventMatchTeamInfoEnd();                                  // 0x100615e0 (C3 virtual 364; "NConsoelWnd")
    void OnEventMatchCreate(INT GameID);                             // 0x100616b0 (guard name; C3 virtual 365)
    void OnEventMatchUserInfo(INT A, FString Name, INT P1, INT P2, INT P3, INT P4, INT P5, INT P6, INT P7, INT P8); // 0x10061760 (C3 virtual 366)
    void OnEventMatchSpellInfoStart(INT A, INT B, INT C);            // 0x10061840 (C3 virtual 367; guard says OnEventMatchUserInfo)
    void OnEventMatchSpellInfo(INT A, INT B, INT C, INT D);          // 0x100618e0 (C3 virtual 368; the same guard)
    void OnEventMatchSpellInfoEnd();                                 // 0x10061980 (C3 virtual 369; the same guard)
    void OnEventMatchTeamUnlocked(INT A, INT B);                     // 0x10061a10 (guard name; C3 virtual 370)
    void OnEventMatchGMTest();                                       // 0x10061aa0 (guard name; C3 virtual 371)
    void OnReceiveMagicSkillUse(User* Caster, User* Target, INT Param); // 0x10061b30 (guard name; C3 virtual 372)
    void OnCharInfo(INT Info);                                       // 0x10061bd0 (guard name; C3 virtual 373)
    void ExecuteCommand(TCHAR* Line);                                // 0x10061c70 (guard name; C3 virtual 384)
    INT AddAbnormalStatus2(L2ParamStack& Params);                    // 0x10064bd0 (C3 virtual 169) a buff of the buff window's second list
    INT SetExtraAbnormalStatus(L2ParamStack& Params);                // 0x10064d20 (C3 virtual 167) the buff window's extra icon
    void GetQuestTarget(FString* Name, FVector* Pos);                // 0x10060c90 (C3 virtual 345) the followed quest's target
    FString GetQuestTargetName();                                    // 0x10077ce0 (C3 virtual 355)
    FVector GetQuestTargetPos();                                     // 0x10077db0 (C3 virtual 356)
    void StartHPBarAnim(INT Unused, INT Value, INT Max, DOUBLE Time); // 0x100611b0 (C3 virtual 358) the player's HP bar animates
    void SetShotAutoUse(L2ParamStack& Params);                       // 0x10062110 (C3 virtual 380) a shot's auto use on / off on the shortcut slots
    void UseAutoShots(INT bFlag);                                    // 0x10075140 (C3 virtual 381) the auto list's shots used for the weapon's grade
    void ClosePartyRoom(L2ParamStack& Params);                       // 0x10061e40 (C3 virtual 378)
    void DispatchMessengerMsg(INT hWnd, UINT Msg, INT wParam, INT lParam); // 0x10062b90 (C3 virtual 306) the messenger library's messages
    void ShowMessageDialog46(L2ParamStack& Params);                  // 0x1006b0c0 (C3 virtual 344)
    void ShowMatchEndDialog();                                       // 0x1006b700 (C3 virtual 387)
    void ShowMatchTextDialog(const TCHAR* Text, INT bHide);          // 0x1006b840 (C3 virtual 388)
    void ReceiveSSQStatus(L2ParamStack& Params);                     // 0x10060f10 (C3 virtual 286) the Seven Signs board's status
    void ReceiveSSQLists(L2ParamStack& Params);                      // 0x10060ed0 (C3 virtual 287)
    void ResetSSQEvents();                                           // 0x10061020 (C3 virtual 288)
    void ReceiveSSQEvents(L2ParamStack& Params);                     // 0x1006b170 (C3 virtual 289)
    void ReceiveSSQList(L2ParamStack& Params);                       // 0x1006b330 (C3 virtual 290)
    void ToggleBuffIcons();                                          // 0x10060c30 (name ours; a shortcut key) the buff icons of the party, pet and summon windows on / off
    void OnMessengerFriendAdd(INT A, TCHAR* Name, INT B, INT C);     // 0x1006b400 (C3 virtual 292) a friend of the messenger to its window
    void OnMessengerFriendEvent(INT Kind, INT A, TCHAR* Name, INT B, INT C); // 0x1006b4b0 (C3 virtual 294) the same by kind (1, 2, 3)
    void OnMessengerFriendStatus(INT B, TCHAR* Name, INT A);         // 0x1006b5c0 (C3 virtual 295)
    void OnMessengerChat(INT A, TCHAR* Name, TCHAR* Text);           // 0x1006b660 (C3 virtual 296)
};
NW_ASSERT_OFFSET(NConsoleWnd, WndMain, 0x394C);
NW_ASSERT_OFFSET(NConsoleWnd, Desktop, 0x3968);
NW_ASSERT_OFFSET(NConsoleWnd, DebugWnd, 0x396C);
NW_ASSERT_OFFSET(NConsoleWnd, PetitionWnd, 0x3978);
NW_ASSERT_OFFSET(NConsoleWnd, Timers, 0x3990);
NW_ASSERT_OFFSET(NConsoleWnd, DeltaTime, 0x39A0);
NW_ASSERT_OFFSET(NConsoleWnd, PartyItemDistribution, 0x39C8);
NW_ASSERT_OFFSET(NConsoleWnd, GodWnd, 0x3950);
// NW_ASSERT_OFFSET(NConsoleWnd, MessageWnd, 0x353C);   // C2: field not mapped
NW_ASSERT_OFFSET(NConsoleWnd, LoadingScreenID, 0x39C4);
NW_ASSERT_OFFSET(NConsoleWnd, bInitialized, 0x392C);
NW_ASSERT_OFFSET(NConsoleWnd, CreditWnd, 0x3958);
NW_ASSERT_OFFSET(NConsoleWnd, WarningDialogBox, 0x397C);
NW_ASSERT_OFFSET(NConsoleWnd, SysMsgParams, 0x3980);
NW_ASSERT_OFFSET(NConsoleWnd, WordWrapperWnd, 0x398C);
NW_ASSERT_OFFSET(NConsoleWnd, TutorialEvents, 0x39BC);
NW_ASSERT_OFFSET(NConsoleWnd, PawnViewerWnd, 0x39D4);
NW_ASSERT_SIZE(NConsoleWnd, 0x39E8);

// The children of the login screen in C2, by offset (the stock code reads them at these places; NCLoginWnd.h, the
// pawn group's, still has C1's order): +0x98 AuthWnd, +0x9C NoticeWnd, +0xA0 WarningWnd, +0xA4 and +0xA8 two
// windows C2 added (names ours: LOGIN_Wnd_A4, LOGIN_Wnd_A8), +0xAC the server list
class NCServerSelectWnd;
#define NLOGIN_SERVERLIST(Login) (nw::Field<NCServerSelectWnd*>((Login), 0xAC))
#define NLOGIN_WND_A4(Login) (nw::Field<NCWnd*>((Login), 0xA4))
#define NLOGIN_WND_A8(Login) (nw::Field<NCWnd*>((Login), 0xA8))

// Cast<APawn> out of line (0x10047230, name ours): NULL unless Object is a pawn
APawn* CastToPawn(UObject* Object);

// Cast<ALineagePlayerController> out of line (0x1004c5c0, name ours): NULL unless Object is the L2 player's controller
ALineagePlayerController* CastToLineagePlayerController(UObject* Object);

#pragma pack(pop)