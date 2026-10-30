#pragma once
// NCConsole - the desktop (UClass "CConsole", 0x180; NConsoleWnd +0x36DC, built by the
// console's Initialize): the screen-sized parent of every in-game window. OnCreate makes them
// all, numbered in order (NCWnd +0x78); ShowWindow (entering the game) hides the dialogs and
// shows the bars, the chat and the status windows. It owns the toggles the menus and the
// shortcut keys use, the HTML page commands ("Link", "Write") and the chat balloons: the
// first 19 characters a character says are drawn over it (its pawn's +0x142C) for 5 seconds.

#include "nwindow/NCWnd.h"
#include "nwindow/NConsoleWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NConsoleWnd;
class NCMainWnd;
class NCChatWnd;
class NCNPCHtmlViewer;
class NCPartyWnd;
class NCPetStatusWnd;
class NCSummonedStatusWnd;
class NCTargetStatusWnd;
class NCPlayerStatusWnd;
class NCAbnormalStatusWnd;
class NCMenuWnd;
class NCSystemMenuWnd;
class NCShortCutWnd;
class NCTrainWnd;
class NCMatchWnd;
class NCMatchOptionWnd;
class NCMinimapWnd;
class NCTutorialViewerWnd;
class NCTutorialQuestionWnd;
class NCPetWnd;
class NCSummonedWnd;
class NCRadarWnd;
class NCGMMagicSkillWnd;
class NCGMDetailStatusWnd;
class NCDialogBox;
class NCHtmlViewer;
class NCGMPetitionWnd;
class NCInventoryWnd;
class NCTradeWnd;
class NCGMWnd;
class NCGMInventoryWnd;
class NCGMQuestWnd;
class NCGMActionWnd;
class NCVIPShopWnd;
class NCStoreWnd;
class NCShopWnd;
class NCGMStoreWnd;
class NCDeliverWnd;
class NCSelectDeliverWnd;
class NCPrivateShopWnd;
class NCPrivateBuyWnd;

// NWM_COMMAND id the desktop answers: quit the game (name ours)
enum { NID_CONSOLE_QUIT = 0x1002 };

// the desktop's own messages (names ours): wParam an FVector, the player moves there
enum { NWM_MOVETO = (INT)0xF0000001 };

// the balloon timer (SetTimer id, once a second)
enum { NTIMER_BALLOONS = 0xFA0D05 };

// A chat balloon (name ours; 12 bytes, NCConsole::Balloons).
struct NChatBalloon
{
    INT ObjectID;              // +0x0 who said it
    TCHAR* Text;               // +0x4 new TCHAR[23]: at most 19 characters, then "..."; the
                               //   pawn's balloon (APawn +0x142C) points at it
    INT Tick;                  // +0x8 NCConsole::Ticks when said

    NChatBalloon() : ObjectID(0), Text(NULL), Tick(0) {}
    ~NChatBalloon() { Text = NULL; }   // the text is freed by hand
};
NW_ASSERT_SIZE(NChatBalloon, 0xC);

class NCConsole : public NCWnd
{
    NW_DECLARE_CLASS(NCConsole, NCWnd, 0, 0x1027B018)

    NCMainWnd* MainWnd;                    // +0xC8 character window
    NCWnd* CalcWnd;                        // +0xCC NCCalcWnd (new in C2; name ours), made by OnCreate
    NCWnd* EmpowerWnd;                     // +0xD0 NCEmpowerWnd (new in C2; name ours)
    NCChatWnd* ChatWnd;                    // +0xD4
    NCNPCHtmlViewer* NpcHtmlWnd;           // +0xD8 NPC dialogs
    NCNPCHtmlViewer* HelpWnd;              // +0xDC "..\L2text\help.htm"
    NConsoleWnd* Owner;                    // +0xE0 the console (the second constructor)
    INT UnknownA8;                         // +0xE4 (1)
    NCGMPetitionWnd* GMPetitionWnd;        // +0xE8
    NCWnd* PetitionBeginWnd;               // +0xEC NCPetitionBeginWnd (new in C2; name ours)
    NCPartyWnd* PartyWnd;                  // +0xF0
    NCPetStatusWnd* PetStatusWnd;          // +0xF4
    NCSummonedStatusWnd* SummonedStatusWnd;// +0xF8
    NCInventoryWnd* InventoryWnd;          // +0xFC
    NCTargetStatusWnd* TargetStatusWnd;    // +0x100
    NCPlayerStatusWnd* PlayerStatusWnd;    // +0x104
    NCAbnormalStatusWnd* AbnormalStatusWnd;// +0x108 buffs
    NCStoreWnd* StoreWnd;                  // +0x10C warehouse
    NCShopWnd* ShopWnd;                    // +0x110 NPC shop
    NCWnd* ShopPreviewWnd;                 // +0x114 NCShopPreviewWnd (new in C3; evidence: OnCreate, CloseWindows)
    NCWnd* NewWndE4;                       // +0x118 (name ours; new in C2: zeroed by both constructors) NCManorShopWnd (OnCreate; text 738)
    NCVIPShopWnd* VIPShopWnd;              // +0x11C
    NCMenuWnd* MenuWnd;                    // +0x120 the menu bar
    NCSystemMenuWnd* SystemMenuWnd;        // +0x124
    NCTradeWnd* TradeWnd;                  // +0x128
    NCWnd* NewWndF8;                       // +0x12C (name ours; new in C2: zeroed by both constructors) NCMacroListWnd (OnCreate)
    NCWnd* NewWndFC;                       // +0x130 NCMacroEditWnd (its +0x9C holds the edit lines)
    NCWnd* NewWnd100;                      // +0x134 NCMacroInfoWnd
    NCShortCutWnd* ShortCutWnd;            // +0x138
    NCTrainWnd* TrainWnd;                  // +0x13C skill trainer
    NCMatchWnd* MatchWnd;                  // +0x140 party matching
    // C2: not placed (no evidence for where it went): NCMatchOptionWnd* MatchOptionWnd;      // +0x110
    // C2: not placed (no evidence for where it went): NCMinimapWnd* MinimapWnd;              // +0x114
    NCWnd* PartyRoomMakingWnd;             // +0x144 NCPartyRoomMakingWnd (new in C3; evidence: OnCreate)
    NCWnd* PartyRoomWnd;                   // +0x148 NCPartyRoomWnd (new in C3; ToggleInterface leaves it)
    NCWnd* MapCoverWnd;                    // +0x14C NCMapCoverWnd (new in C3; ToggleMinimap)
    NCWnd* BoardWnd;                       // +0x150 NCBoardWnd (community)
    NCTutorialViewerWnd* TutorialWnd;      // +0x154
    NCTutorialQuestionWnd* TutorialQuestionWnd; // +0x158 the question mark
    NCPetWnd* PetWnd;                      // +0x15C
    NCSummonedWnd* SummonedWnd;            // +0x160
    NCRadarWnd* RadarWnd;                  // +0x164
    NCWnd* SiegeInfoWnd;                   // +0x168 NCSiegeInfoMainWnd
    NCWnd* NewWnd134;                      // +0x16C (name ours; new in C2, zeroed by both constructors) NCRecipeBookWnd
    // C2: not placed (no evidence for where it went): NCWnd* NewWnd138;                      // +0x138 NCRecipeTreeWnd
    NCWnd* RecipeGaraTreeWnd;              // +0x170 NCRecipeGaraTreeWnd (new in C3; evidence: OnCreate)
    NCWnd* NewWnd13C;                      // +0x174 NCRecipeManufactureWnd
    NCWnd* RecipeShopWnd;                  // +0x178 (name ours; new in C2) NCRecipeShopWnd; ToggleInterface: the recipe shop manage window
    NCWnd* NewWnd144;                      // +0x17C (name ours; new in C2: only the first constructor zeroes it) NCRecipeBuyListWnd
    NCWnd* RecipeBuyManufactureWnd;        // +0x180 NCRecipeBuyManufactureWnd (new in C2; name ours)
    NCWnd* NewWnd14C;                      // +0x184 (name ours; new in C2, zeroed by both constructors) NCHennaListWnd
    NCWnd* NewWnd150;                      // +0x188 NCHennaInfoWnd
    NCWnd* MessengerWnd;                   // +0x18C MessengerWnd (new in C3; ToggleChat)
    NCGMWnd* GMWnd;                        // +0x190
    NCGMInventoryWnd* GMInventoryWnd;      // +0x194
    NCGMMagicSkillWnd* GMMagicSkillWnd;    // +0x198
    NCGMDetailStatusWnd* GMDetailStatusWnd;// +0x19C
    NCGMQuestWnd* GMQuestWnd;              // +0x1A0
    NCGMStoreWnd* GMStoreWnd;              // +0x1A4
    NCGMActionWnd* GMActionWnd;            // +0x1A8
    NCWnd* BroadcastGMWnd;                 // +0x1AC NCBroadcastGMWnd (new in C3; evidence: OnCreate)
    NCWnd* BroadcastGMFenceWnd;            // +0x1B0 NCBroadcastGMFenceWnd
    NCWnd* BroadcastObserverWnd;           // +0x1B4 NCBroadcastObserverWnd
    NCWnd* BroadcastObserverGMMsgWnd;      // +0x1B8 NCBroadcastObserverGMMsgWnd
    NCWnd* BroadcastObserverSpecialMsgWnd; // +0x1BC NCBroadcastObserverSpecialMsgWnd
    NCPrivateShopWnd* PrivateShopWnd;      // +0x1C0 selling
    NCPrivateBuyWnd* PrivateBuyWnd;        // +0x1C4 buying
    NCDeliverWnd* DeliverWnd;              // +0x1C8 freight
    NCSelectDeliverWnd* SelectDeliverWnd;  // +0x1CC
    NCNPCHtmlViewer* TestNpcHtmlWnd;       // +0x1D0 "test.html" (GIsOnlyL2Console only)
    NCWnd* TestHtmlWnd;                    // +0x1D4 NCTestHtmlViewer (GIsTestHtml only)
    NCDialogBox* MainDialogBox;            // +0x1D8 NDialogInfo::bMainBox
    NCDialogBox* SecondDialogBox;          // +0x1DC
    NCWnd* GMSnoopManager;                 // +0x1E0 NCGMSnoopManager (new in C2; name ours): a child with no rectangle
    NCWnd* NewWnd194;                      // +0x1E4 (name ours; new in C2: only the first constructor zeroes it) NTownMapWnd
    NCWnd* NewWnd198;                      // +0x1E8 NCObserverWnd
    NCWnd* NewWnd19C;                      // +0x1EC (zeroed by both constructors) NCXMasSealWnd
    NCWnd* ZoneTitleWnd;                   // +0x1F0 NCZoneTitleWnd (new in C3; evidence: OnCreate)
    NCWnd* SSQMainBoard;                   // +0x1F4 NCSSQMainBoard (new in C3; CloseWindows hides it)
    INT Unknown150;                        // +0x1F8 (1)
    UTexture* BlackTex;                    // +0x1FC "NWindow.BlackTexture"
    UTexture* BalloonTex;                  // +0x200 "NWindow.chat"
    TArray<NChatBalloon> Balloons;         // +0x204
    INT BalloonTimer;                      // +0x210 SetTimer result
    INT Ticks;                             // +0x214 seconds (the balloon timer)
    INT Unknown170[4];                     // +0x218

    NCConsole();                                                     // 0x10056920
    NCConsole(NConsoleWnd* InOwner);                                 // 0x10056b10 (Initialize)
    // ~NCConsole                                                    // 0x10056ab0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10053fd0
    virtual INT OnTimer(INT TimerID);                                // 36 0x100564c0
    virtual INT OnCreate();                                          // 37 0x10056c70
    virtual void SetWindowSize(FLOAT W, FLOAT H);                    // 43 0x10053e20 (C3: the observer window follows)
    virtual INT OnDestroy();                                         // 38 0x10055780
    virtual void ShowWindow();                                       // 44 0x10054050
    virtual void HideWindow();                                       // 45 0x100412d0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100530e0
    // name ours: an HTML page's "Link <bypass>" or "Write <board> ..." command
    virtual INT OnHtmlCommand(const TCHAR* Command, NCHtmlViewer* Source); // 68 0x10054540

    // names ours
    __declspec(noinline) INT OnMoveTo(WPARAM wParam, LPARAM lParam);                      // 0x10053180 NWM_MOVETO
    __declspec(noinline) INT OnQuit();                                                    // 0x10092210 (folded with NCAuthWnd's virtual 69) NID_CONSOLE_QUIT: appRequestExit(0)
    void ToggleInterface();                                          // 0x100531b0 (a shortcut key)
    void CloseWindows();                                             // 0x100532f0 (a shortcut key)
    void Reset();                                                    // 0x10053590 (the console)
    void TogglePetition();                                           // 0x1003de90 (C1 only: no C2 match) (C1 only: no C2 match) (the chat line)
    void ToggleSystemMenu();                                         // 0x10053630
    void ToggleInventory();                                          // 0x10053690
    void ToggleChat();                                               // 0x10053740
    void ToggleHelp(UBOOL bMacro);                                   // 0x100537d0 (C2: bMacro, name ours)
    void TogglePartyMatch();                                         // 0x1004bde0 (C1 only: no C2 match)
    void ToggleMinimap(INT Mode = 0);                                // 0x10053830 (C3: one argument, the mode of the map cover's child +0x16C)
    void ToggleMatchOption();                                        // 0x1004c020 (C1 only: no C2 match)
    void ToggleBoard();                                              // 0x100538b0
    __declspec(noinline) void ToggleMacroList();                                          // 0x100539b0 (C2 only; name ours) the macro window, with its sound
    __declspec(noinline) void ToggleMacroEdit();                                          // 0x10053a10 (C2 only; name ours) the macro editor, the same way
    __declspec(noinline) void ToggleXMasSeal();                                           // 0x10053d50 (C2 only; name ours) the Christmas seal window
    void ToggleGMWnd();                                              // 0x10053ad0
    void ToggleGMInventory(TCHAR* Name);                             // 0x10053b10 (the GM window)
    void ToggleGMMagicSkill(TCHAR* Name);                            // 0x10053b70
    void ToggleGMDetailStatus(TCHAR* Name);                          // 0x10053bd0
    void ToggleGMQuest(TCHAR* Name);                                 // 0x10053c30
    void ToggleGMStore(TCHAR* Name);                                 // 0x10053c90
    void ToggleGMAction(TCHAR* Name);                                // 0x10053cf0
    void AddSystemMessage(INT MsgID, DWORD Color);                   // 0x10053d90 into the chat
    void AddSystemText(const TCHAR* Text, INT SubType, DWORD Color); // 0x10053df0 into the chat
    void ToggleCharacter(INT Tab);                                   // 0x10054df0 -1: show / hide
    void Say(INT ObjectID, const TCHAR* Name, const TCHAR* Text, DWORD Color,
             INT Type);                                              // 0x10054e70 the console
};
NW_ASSERT_OFFSET(NCConsole, MainWnd, 0xC8);
NW_ASSERT_OFFSET(NCConsole, CalcWnd, 0xCC);
NW_ASSERT_OFFSET(NCConsole, EmpowerWnd, 0xD0);
NW_ASSERT_OFFSET(NCConsole, Owner, 0xE0);
NW_ASSERT_OFFSET(NCConsole, PetitionBeginWnd, 0xEC);
NW_ASSERT_OFFSET(NCConsole, RecipeBuyManufactureWnd, 0x180);
NW_ASSERT_OFFSET(NCConsole, GMSnoopManager, 0x1E0);
NW_ASSERT_OFFSET(NCConsole, InventoryWnd, 0xFC);
NW_ASSERT_OFFSET(NCConsole, ShortCutWnd, 0x138);
NW_ASSERT_OFFSET(NCConsole, RadarWnd, 0x164);
NW_ASSERT_OFFSET(NCConsole, RecipeShopWnd, 0x178);
NW_ASSERT_OFFSET(NCConsole, MainDialogBox, 0x1D8);
NW_ASSERT_OFFSET(NCConsole, Balloons, 0x204);
NW_ASSERT_OFFSET(NCConsole, Ticks, 0x214);
NW_ASSERT_SIZE(NCConsole, 0x228);

// the desktop (NConsoleWnd::Desktop is declared as an NCWnd)
#define GDesktop (static_cast<NCConsole*>(GConsole->Desktop))
inline NCConsole* NConsoleWnd::GetDesktop() const
{
    return static_cast<NCConsole*>(Desktop);
}

/*-----------------------------------------------------------------------------
    Function holders of NConsoleWndMisc.cpp that the desktop calls too (stage 5: a typed call replaces the stock call by
    address). Not classes: a window pointer is cast to one for a call; the members are hooked in NConsoleWndMisc.cpp.
-----------------------------------------------------------------------------*/

struct FAbnormalStatus;

// NCAbnormalStatusWnd's own functions this port reaches: the stock's small ones that no header names
struct NCAbnormalStatusWndExt
{
    __declspec(noinline) INT AddStatus2(FAbnormalStatus* Status);        // 0x10101F50 a buff of the second list (10 at most)
    __declspec(noinline) INT SetExtraStatus(FAbnormalStatus* Status);    // 0x10103A50 the extra icon (+0xE0), cleared by an empty name
    __declspec(noinline) void ClearStatus2();                            // 0x10102B30 the second list emptied
    __declspec(noinline) void ClearExtraStatus();                        // 0x10102BA0 the extra icon removed
    __declspec(noinline) void ClearAll();                                // 0x10102C50 both lists and the extra icon
};

// The messenger window (CMessengerWnd, 0xD8 bytes; include/nwindow/MessengerWnd.h is another lane's): the MSN page (+0xC8) and the
// Lineage II friends page (+0xCC)
struct NMsnMessengerView : public NCWnd
{
    __declspec(noinline) void HideAll();   // 0x10139650 (name ours)
    __declspec(noinline) INT SignOut();    // 0x1013BB30 (name ours)
};

// An L2Friend passed by value, as the stock callee gets it: six words that the callee owns (it destroys the name). Plain
// words, so that a hook does not copy the string; NConsoleWndMisc.cpp's NMsnFriendOf reads them as the record
struct NL2FriendArg
{
    DWORD W[6];
};
NW_ASSERT_SIZE(NL2FriendArg, 0x18);

struct NMsnMainPanel;   // NConsoleWndMisc.cpp's view of the messenger's main panel

// The messenger window (the desktop's +0x18C) as the console's code sees it: its main panel is the child at +0xCC (the members
// are NConsoleWndMisc.cpp's; NCChatWnd.cpp calls GetPeerName)
struct NMsnShellWnd : public NCWnd
{
    void FriendAdd(NL2FriendArg Arg);      // 0x10134610 (ret 0x18)
    void FriendRemove(NL2FriendArg Arg);   // 0x10134680 (ret 0x18)
    void FriendUpdate(NL2FriendArg Arg);   // 0x101346F0 (ret 0x18)
    __declspec(noinline) void ReloadFriends();     // 0x10131530 (out of line, as the stock: the console's slot 293 calls it)
    __declspec(noinline) void PanelCall139370();   // 0x10139670: the panel's 0x10139370 (out of line, as the stock)
    FString* GetPeerName(FString* Result);   // 0x101345D0 (ret 4: the hidden result)
    __declspec(noinline) void PanelChat(INT A, TCHAR* Name, TCHAR* Text);   // 0x10139680 (ret 0xC): the panel's 0x10139230 (out of line)

    NMsnMainPanel* MainPanel() { return nw::Field<NMsnMainPanel*>(this, 0xCC); }
};

#pragma pack(pop)