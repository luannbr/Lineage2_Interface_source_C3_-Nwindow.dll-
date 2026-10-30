/*=============================================================================
    NCConsole.cpp - the desktop. Stock: 0x1003d740 .. 0x1003d7d1, 0x1003d7e0 .. 0x1003daa0,
    0x1003dac0 .. 0x1003e716, 0x1003e950 .. 0x10040f1e, 0x10041800 .. 0x10041b19,
    0x10041bc0 .. 0x100424d1, 0x10042530 .. 0x100427ba (HideWindow is NCChatWnd's body; the
    TArray<NChatBalloon> code at 0x10041b20 and 0x100424e0 is the template's).
=============================================================================*/
#include "nwindow/NCConsole.h"
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCActionWnd.h"
#include "nwindow/NCBoardWnd.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCDeliverWnd.h"
#include "nwindow/NCDetailStatusWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCGMWnd.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCMainWnd.h"
// C3: class gone, no counterpart: #include "NCMatchOptionWnd.h"
#include "nwindow/NCMatchWnd.h"
#include "nwindow/NCMenuWnd.h"
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCNPCHtmlViewer.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCPetitionWnd.h"
#include "nwindow/NCPetWnd.h"
#include "nwindow/NCPrivateBuyWnd.h"
#include "nwindow/NCPrivateShopWnd.h"
#include "nwindow/NCRadarWnd.h"
#include "nwindow/NCShopWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCSiegeInfoMainWnd.h"
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NCSummonedWnd.h"
#include "nwindow/NCSystemMenuWnd.h"
#include "nwindow/NCTestHtmlViewer.h"
#include "nwindow/NCTradeWnd.h"
#include "nwindow/NCTrainWnd.h"
#include "nwindow/NCTutorialViewerWnd.h"
#include "nwindow/NCVIPShopWnd.h"
#include "nwindow/NCCalcWnd.h"
#include "nwindow/NCEmpowerWnd.h"
#include "nwindow/NCGMSnoopManager.h"
#include "nwindow/NCGaraDialogBox.h"
#include "nwindow/NCHennaInfoWnd.h"
#include "nwindow/NCHennaListWnd.h"
#include "nwindow/NCMacroEditWnd.h"
#include "nwindow/NCMacroInfoWnd.h"
#include "nwindow/NCMacroListWnd.h"
#include "nwindow/NCManorShopWnd.h"
#include "nwindow/NCObserverWnd.h"
#include "nwindow/NCPetitionBeginWnd.h"
#include "nwindow/NCRecipeBookWnd.h"
#include "nwindow/NCRecipeBuyListWnd.h"
#include "nwindow/NCRecipeBuyManufactureWnd.h"
#include "nwindow/NCRecipeManufactureWnd.h"
#include "nwindow/NCRecipeShopWnd.h"
// C3: class gone, no counterpart: #include "NCRecipeTreeWnd.h"
#include "nwindow/NCXMasSealWnd.h"
#include "nwindow/NTownMapWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/MessengerWnd.h"
#include "nwindow/NCBroadcastGMFenceWnd.h"
#include "nwindow/NCBroadcastGMWnd.h"
#include "nwindow/NCBroadcastObserverGMMsgWnd.h"
#include "nwindow/NCBroadcastObserverSpecialMsgWnd.h"
#include "nwindow/NCBroadcastObserverWnd.h"
#include "nwindow/NCMapCoverWnd.h"
#include "nwindow/NCPartyRoomMakingWnd.h"
#include "nwindow/NCPartyRoomWnd.h"
#include "nwindow/NCPledgeWnd.h"
#include "nwindow/NCQuestGaraTreeWnd.h"
#include "nwindow/NCSSQMainBoard.h"
#include "nwindow/NCShopPreviewWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NCZoneTitleWnd.h"
#include "nwindow/NClasses.gen.h"

#include <stdio.h>
#include <wchar.h>
#include <imm.h>

// OnCreate numbers the windows it makes, in order (NCWnd +0x78; name ours)
#if NW_HOOK_MODE
#define GNextWndNumber NW_GLOBAL(INT, 0x10271080)   // C3 (name ours): the next window number, NCWnd::OnCreate's too (GNextFrameID is 0x101F3134)
#else
extern INT GNextWndNumber;
#endif

// interface sounds of the toggles (names ours)
enum { SOUND_WindowOpen = 5, SOUND_WindowClose = 6, SOUND_Minimap = 8, SOUND_Petition = 10 };
// C3 added eight (the table of NConsoleWnd::PlayInterfaceSound has 19 names now)
enum
{
    SOUND_CharOpen = 11, SOUND_CharClose = 12, SOUND_InventoryOpen = 13, SOUND_InventoryClose = 14,
    SOUND_MapOpen = 15, SOUND_MapClose = 16, SOUND_SystemOpen = 17, SOUND_SystemClose = 18
};

// the player's petition window (NConsoleWnd::PetitionWnd is declared as an NCWnd)
static FORCEINLINE NCPetitionWnd* PlayerPetitionWnd()
{
    return static_cast<NCPetitionWnd*>(GConsole->PetitionWnd);
}

// the console's options window
#if 0  // C2: does not compile against the C2 headers (db/demote.csv)
static FORCEINLINE NCWnd* OptionWnd()
{
    return GConsole->OptionWnd;
}
#endif

// NCStoreWnd::GetMode; the stock code calls it on the NPC shop too, whose mode is at the same
// place (+0xBC)
static FORCEINLINE INT StoreWndMode(NCWnd* Store)
{
    return static_cast<NCStoreWnd*>(Store)->GetMode();
}

/*-----------------------------------------------------------------------------
    Construction.
-----------------------------------------------------------------------------*/

// 0x10056920 - the default constructor (the class's InternalConstructor): the windows OnCreate makes are cleared
// (the stock stores them one by one; the object comes zeroed all the same), two fields are 1 and the balloon list
// is empty. Which windows are cleared differs from the second constructor's: +0x178, +0x17C, +0x1E4, +0x1E8 and
// +0xE0 here, +0x1D0 and +0x1EC there
NCConsole::NCConsole()
{
    ChatWnd = NULL;
    NpcHtmlWnd = NULL;
    HelpWnd = NULL;
    Owner = NULL;
    InventoryWnd = NULL;
    TargetStatusWnd = NULL;
    PlayerStatusWnd = NULL;
    AbnormalStatusWnd = NULL;
    NewWndE4 = NULL;
    MenuWnd = NULL;
    SystemMenuWnd = NULL;
    NewWndF8 = NULL;
    NewWndFC = NULL;
    NewWnd100 = NULL;
    ShortCutWnd = NULL;
    TrainWnd = NULL;
    MatchWnd = NULL;
    MapCoverWnd = NULL;
    BoardWnd = NULL;
    PetWnd = NULL;
    SummonedWnd = NULL;
    NewWnd134 = NULL;
    RecipeGaraTreeWnd = NULL;
    NewWnd13C = NULL;
    RecipeShopWnd = NULL;
    NewWnd144 = NULL;
    NewWnd14C = NULL;
    NewWnd150 = NULL;
    MessengerWnd = NULL;
    GMWnd = NULL;
    GMInventoryWnd = NULL;
    GMMagicSkillWnd = NULL;
    GMDetailStatusWnd = NULL;
    GMQuestWnd = NULL;
    GMStoreWnd = NULL;
    BroadcastGMWnd = NULL;
    BroadcastGMFenceWnd = NULL;
    BroadcastObserverWnd = NULL;
    NewWnd194 = NULL;
    NewWnd198 = NULL;
    BlackTex = NULL;
    BalloonTex = NULL;
    UnknownA8 = 1;
    Unknown150 = 1;
    BalloonTimer = 0;
    Ticks = 0;
    Unknown170[0] = 0;
    Unknown170[1] = 0;
    Unknown170[2] = 0;
    Unknown170[3] = 0;
}
NW_HOOK_CTOR(0x10056920, NCConsole)

// 0x10056b10 - the constructor Initialize uses: the same with the console as owner (+0xE0)
NCConsole::NCConsole(NConsoleWnd* InOwner)
{
    ChatWnd = NULL;
    NpcHtmlWnd = NULL;
    HelpWnd = NULL;
    InventoryWnd = NULL;
    TargetStatusWnd = NULL;
    PlayerStatusWnd = NULL;
    AbnormalStatusWnd = NULL;
    NewWndE4 = NULL;
    MenuWnd = NULL;
    SystemMenuWnd = NULL;
    NewWndF8 = NULL;
    NewWndFC = NULL;
    NewWnd100 = NULL;
    ShortCutWnd = NULL;
    TrainWnd = NULL;
    MatchWnd = NULL;
    MapCoverWnd = NULL;
    BoardWnd = NULL;
    PetWnd = NULL;
    SummonedWnd = NULL;
    NewWnd134 = NULL;
    RecipeGaraTreeWnd = NULL;
    NewWnd13C = NULL;
    NewWnd14C = NULL;
    NewWnd150 = NULL;
    MessengerWnd = NULL;
    GMWnd = NULL;
    GMInventoryWnd = NULL;
    GMMagicSkillWnd = NULL;
    GMDetailStatusWnd = NULL;
    GMQuestWnd = NULL;
    GMStoreWnd = NULL;
    BroadcastGMWnd = NULL;
    BroadcastGMFenceWnd = NULL;
    BroadcastObserverWnd = NULL;
    TestNpcHtmlWnd = NULL;
    NewWnd19C = NULL;
    BlackTex = NULL;
    BalloonTex = NULL;
    UnknownA8 = 1;
    Unknown150 = 1;
    BalloonTimer = 0;
    Ticks = 0;
    Unknown170[0] = 0;
    Unknown170[1] = 0;
    Unknown170[2] = 0;
    Unknown170[3] = 0;
    Owner = InOwner;
}
NW_HOOK_CTOR_ARGS(0x10056b10, NCConsole, (NConsoleWnd* InOwner), (InOwner))


/*-----------------------------------------------------------------------------
    Messages.
-----------------------------------------------------------------------------*/

// 0x10092210 - the stock body is shared by every Exit slot (folded identical code, `appRequestExit(0); return 1`):
// NCAuthWnd::OnCancel hooks it, once; this is the same body as plain code (it was a NW_STOCK jump into that hook), so it
// carries no hook of its own
INT NCConsole::OnQuit()
{
    appRequestExit(0);
    return 1;
}

// 0x10053fd0 (vslot 25) - NID_CONSOLE_QUIT quits, NWM_MOVETO walks (C2: no IsEnabled test first, as C1 had)
INT NCConsole::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_CONSOLE_QUIT)
            return OnQuit() != 0;
    }
    else if (Msg == (UINT)NWM_MOVETO)
        return OnMoveTo(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10053fd0, NCConsole, WindowProc)

// 0x10053180 - the player walks to the FVector wParam points at
INT NCConsole::OnMoveTo(WPARAM wParam, LPARAM lParam)
{
    Owner->MoveToLocation(*(FVector*)wParam, 0);
    return 1;
}
NW_HOOK_METHOD(0x10053180, NCConsole, OnMoveTo)

// 0x100564c0 (vslot 35) - each second the balloons older than 5 seconds go (and off their
// pawns)
INT NCConsole::OnTimer(INT TimerID)
{
    if (TimerID == NTIMER_BALLOONS)
    {
        Ticks++;
        for (INT i = 0; i < Balloons.Num();)
        {
            NChatBalloon& Balloon = Balloons(i);
            if (Balloon.Tick + 5 < Ticks)
            {
                User* Speaker = GConsole->FindUserByID(Balloon.ObjectID);
                if (Speaker && Speaker->Pawn)
                {
                    Speaker->Pawn->ChatBalloon = NULL;
                    Speaker->Pawn->Unknown5C++;
                }
                if (Balloon.Text)
                {
                    delete[] Balloon.Text;
                    Balloon.Text = NULL;
                }
                Balloons.Remove(i, 1);
            }
            else
                i++;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100564c0, NCConsole, OnTimer)

// 0x100530e0 (vslot 56) - draws nothing: the clip of the desktop's rectangle (C3: floats, truncated) is set and
// released
INT NCConsole::OnPaint(NCGDevice* Device)
{
    guard(NCConsole::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100530e0, NCConsole, OnPaint)

/*-----------------------------------------------------------------------------
    Creation.
-----------------------------------------------------------------------------*/

// A child hidden before its creation message (inlined in the stock OnCreate for eight
// windows: the shortcut bar, the trainer, the party matching and its options, the minimap, the
// GM window and the two test pages; name ours): registered, hidden, then attached (entry,
// screen position, WM_CREATE)
static FORCEINLINE void CreateHiddenChild(NCWnd* Parent, NCWnd* Child, DWORD ChildStyle)
{
    NWndEntry* E = Parent->AddChildWindow(Child, ChildStyle);
    Child->HideWindow();
    Child->AttachEntry(E);
}

// A child hidden before its creation message, the attach inline (the shortcut bar, the trainer, the matching
// windows, the minimap, the town map and the observer window: name ours): registered, hidden, then attached
static FORCEINLINE void CreateChildHidden(NCWnd* Parent, NCWnd* Child, DWORD ChildStyle)
{
    NWndEntry* E = Parent->AddChildWindow(Child, ChildStyle);
    Child->HideWindow();
    Child->Entry = E;
    if (E->Parent)
    {
        NCWnd* Owner = E->Parent->Wnd;
        Child->ScreenX += Owner->ScreenX;
        Child->ScreenY += Owner->ScreenY;
    }
    Child->WindowProc(WM_CREATE, 0, 0);
}

// A child attached by the out-of-line AttachEntry (where the stock stopped inlining it: the manor shop, the GM
// windows and the last ones; name ours)
static FORCEINLINE void CreateChildCall(NCWnd* Parent, NCWnd* Child, DWORD ChildStyle)
{
    NWndEntry* E = Parent->AddChildWindow(Child, ChildStyle);
    Child->AttachEntry(E);
}

// The inline half of CreateChild after the entry came back (stock-inlined in OnCreate for most windows): the entry,
// the place made absolute and WM_CREATE
static FORCEINLINE void AttachInline(NCWnd* Parent, NCWnd* Child, NWndEntry* E)
{
    Child->Entry = E;
    if (E->Parent)
    {
        NCWnd* Owner = E->Parent->Wnd;
        Child->ScreenX += Owner->ScreenX;
        Child->ScreenY += Owner->ScreenY;
    }
    Child->WindowProc(WM_CREATE, 0, 0);
}

// 0x10056c70 (vslot 37) - every in-game window, in the order of the stock (C3: the generated text of the stock's
// straight-line code, one block per window: `new` of the class (the stock's StaticAllocateObject of the class's
// UClass with the default arguments, then its constructor), keep the pointer at the desktop's field, place it (the
// positions are floats of the desktop's own rectangle, truncated), register it as a child and attach it, then what
// that window gets: hidden, numbered for WindowsInfo.ini, titled). The fields are the offsets of NCConsole.h
INT NCConsole::OnCreate()
{
    NCWnd* W = NULL;
    NWndEntry* E = NULL;
    NRectF Local;
    Local.X = 0;
    Local.Y = 0;
    Local.W = 0;
    Local.H = 0;
    GetLocalRect(&Local);

    // class 0x102b7370
    W = new NCZoneTitleWnd;
    nw::Field<NCWnd*>(this, 0x1F0) = W;   // ZoneTitleWnd
    W->SetPosition(0, 0, 0, 0);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1F0), 10);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)

    // NCMainWnd
    W = new NCMainWnd;
    nw::Field<NCWnd*>(this, 0xC8) = W;   // MainWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xC8), 0x3002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0xC8)->Unknown78 = ++GNextWndNumber;

    // class 0x102a9b48
    W = new NCSSQMainBoard;
    nw::Field<NCWnd*>(this, 0x1F4) = W;   // SSQMainBoard
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1F4), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x1F4)->HideWindow();
    // SetTitle
    nw::Field<NCWnd*>(this, 0x1F4)->SetTitle(*GL2GameData.SysString[882]);

    // NCCalcWnd
    W = new NCCalcWnd;
    nw::Field<NCWnd*>(this, 0xCC) = W;   // CalcWnd
    W->SetPosition((INT)(((Local.W - 256.0f) * 0.5f)), (INT)(((Local.H - 211.0f) * 0.5f)), 256, 211);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xCC), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    // SetTitle
    nw::Field<NCWnd*>(this, 0xCC)->SetTitle(*GL2GameData.SysString[669]);

    // NCGMSnoopManager
    W = new NCGMSnoopManager;
    nw::Field<NCWnd*>(this, 0x1E0) = W;   // GMSnoopManager
    E = AddChildWindow(W, 6);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)

    // NCGMPetitionWnd
    W = new NCGMPetitionWnd;
    nw::Field<NCWnd*>(this, 0xE8) = W;   // GMPetitionWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 310, 301);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xE8), 2);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    // SetTitle
    nw::Field<NCWnd*>(this, 0xE8)->SetTitle(*GL2GameData.SysString[472]);
    nw::Field<NCWnd*>(this, 0xE8)->Unknown78 = ++GNextWndNumber;

    // NCPetitionBeginWnd
    W = new NCPetitionBeginWnd;
    nw::Field<NCWnd*>(this, 0xEC) = W;   // PetitionBeginWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xEC), 2);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    // SetTitle
    nw::Field<NCWnd*>(this, 0xEC)->SetTitle(*GL2GameData.SysString[470]);

    // NCEmpowerWnd
    W = new NCEmpowerWnd;
    nw::Field<NCWnd*>(this, 0xD0) = W;   // EmpowerWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xD0), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    // SetTitle
    nw::Field<NCWnd*>(this, 0xD0)->SetTitle(*GL2GameData.SysString[668]);

    // NCNPCHtmlViewer
    W = new NCNPCHtmlViewer;
    nw::Field<NCWnd*>(this, 0xD8) = W;   // NpcHtmlWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 310, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xD8), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0xD8)->HideWindow();
    nw::Field<NCWnd*>(this, 0xD8)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0xD8)->SetTitle(*GL2GameData.SysString[444]);
    nw::Field<UTexture*>(nw::Field<NCWnd*>(this, 0xD8), 0xCC) = FindTexture(TEXT("L2UI_ch3.NpcWnd.Npc1_back"));

    // NCBoardWnd
    W = new NCBoardWnd;
    nw::Field<NCWnd*>(this, 0x150) = W;   // BoardWnd
    W->SetPosition((INT)(((Width * 0.5f) - 323.0f)), (INT)(((Height * 0.5f) - 265.0f)), 646, 530);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x150), 0x3002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x150)->HideWindow();
    nw::Field<NCWnd*>(this, 0x150)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x150)->SetTitle(*GL2GameData.SysString[387]);

    // NCInventoryWnd
    W = new NCInventoryWnd;
    nw::Field<NCWnd*>(this, 0xFC) = W;   // InventoryWnd
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xFC), 0x3002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0xFC)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0xFC)->SetTitle(*GL2GameData.SysString[138]);

    // NCPlayerStatusWnd
    W = new NCPlayerStatusWnd(100);
    nw::Field<NCWnd*>(this, 0x104) = W;   // PlayerStatusWnd
    W->SetPosition(0, 0, 172, 76);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x104), 0x85002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x104)->Unknown78 = ++GNextWndNumber;

    // NCTargetStatusWnd
    W = new NCTargetStatusWnd(150);
    nw::Field<NCWnd*>(this, 0x100) = W;   // TargetStatusWnd
    W->SetPosition((INT)(((Local.W * 0.5f) - 86.0f)), 0, 172, 46);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x100), 0x81022);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x100)->Unknown78 = ++GNextWndNumber;

    // NCAbnormalStatusWnd
    W = new NCAbnormalStatusWnd(24);
    nw::Field<NCWnd*>(this, 0x108) = W;   // AbnormalStatusWnd
    W->SetPosition(172, 0, 252, 50);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x108), 0x85022);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x108)->Unknown78 = ++GNextWndNumber;

    // NCPartyWnd
    W = new NCPartyWnd;
    nw::Field<NCWnd*>(this, 0xF0) = W;   // PartyWnd
    W->SetPosition(0, 152, 172, 46);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xF0), 0x81002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0xF0)->Unknown78 = ++GNextWndNumber;

    // NCPetStatusWnd
    W = new NCPetStatusWnd(100);
    nw::Field<NCWnd*>(this, 0xF4) = W;   // PetStatusWnd
    W->SetPosition(0, 76, 172, 46);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xF4), 0x5002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0xF4)->Unknown78 = ++GNextWndNumber;

    // NCSummonedStatusWnd
    W = new NCSummonedStatusWnd(100);
    nw::Field<NCWnd*>(this, 0xF8) = W;   // SummonedStatusWnd
    W->SetPosition(0, 76, 172, 46);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xF8), 0x5002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0xF8)->Unknown78 = ++GNextWndNumber;

    // NCStoreWnd
    W = new NCStoreWnd;
    nw::Field<NCWnd*>(this, 0x10C) = W;   // StoreWnd
    W->SetPosition(80, 156, 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x10C), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x10C)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x10C)->SetTitle(*GL2GameData.SysString[131]);

    // NCDeliverWnd
    W = new NCDeliverWnd;
    nw::Field<NCWnd*>(this, 0x1C8) = W;   // DeliverWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1C8), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    // SetTitle
    nw::Field<NCWnd*>(this, 0x1C8)->SetTitle(*GL2GameData.SysString[557]);

    // NCSelectDeliverWnd
    W = new NCSelectDeliverWnd;
    nw::Field<NCWnd*>(this, 0x1CC) = W;   // SelectDeliverWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 201);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1CC), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    // SetTitle
    nw::Field<NCWnd*>(this, 0x1CC)->SetTitle(*GL2GameData.SysString[558]);

    // NCShopWnd
    W = new NCShopWnd;
    nw::Field<NCWnd*>(this, 0x110) = W;   // ShopWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x110), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x110)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x110)->SetTitle(*GL2GameData.SysString[136]);

    // class 0x102acec0
    W = new NCShopPreviewWnd;
    nw::Field<NCWnd*>(this, 0x114) = W;   // ShopPreviewWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x114), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x114)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x114)->SetTitle(*GL2GameData.SysString[847]);

    // NCVIPShopWnd
    W = new NCVIPShopWnd;
    nw::Field<NCWnd*>(this, 0x11C) = W;   // VIPShopWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x11C), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    // SetTitle
    nw::Field<NCWnd*>(this, 0x11C)->SetTitle(*GL2GameData.SysString[136]);

    // NCTradeWnd
    W = new NCTradeWnd;
    nw::Field<NCWnd*>(this, 0x128) = W;   // TradeWnd
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x128), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x128)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x128)->SetTitle(*GL2GameData.SysString[445]);

    // NCMenuWnd
    W = new NCMenuWnd;
    nw::Field<NCWnd*>(this, 0x120) = W;   // MenuWnd
    W->SetPosition((INT)((Width - 173.0f)), (INT)((Height - 46.0f)), 173, 46);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x120), 0x85002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x120)->Unknown78 = ++GNextWndNumber;

    // NCSystemMenuWnd
    W = new NCSystemMenuWnd;
    nw::Field<NCWnd*>(this, 0x124) = W;   // SystemMenuWnd
    W->SetPosition((INT)((Width - 172.0f)), (INT)(((Height - 295.0f) - 46.0f)), 172, 295);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x124), 0x3002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    // SetTitle
    nw::Field<NCWnd*>(this, 0x124)->SetTitle(*GL2GameData.SysString[446]);

    // NCNPCHtmlViewer
    W = new NCNPCHtmlViewer;
    nw::Field<NCWnd*>(this, 0xDC) = W;   // HelpWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 310, 311);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xDC), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0xDC)->HideWindow();
    nw::Field<NCWnd*>(this, 0xDC)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0xDC)->SetTitle(*GL2GameData.SysString[145]);
    nw::Field<UTexture*>(nw::Field<NCWnd*>(this, 0xDC), 0xCC) = FindTexture(TEXT("L2UI_ch3.NpcWnd.Npc2_back"));

    // NCGaraDialogBox
    W = new NCGaraDialogBox;
    nw::Field<NCWnd*>(this, 0x1D8) = W;   // MainDialogBox
    W->SetPosition((INT)(((Width * 0.5f) - 128.0f)), (INT)(((Height * 0.5f) - 54.0f)), 256, 108);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1D8), 34);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    static_cast<NCGaraDialogBox*>(nw::Field<NCWnd*>(this, 0x1D8))->SetButtonTexts(140, 141);   // NCGaraDialogBox::SetButtonTexts (OK, Cancel)

    // NCGaraDialogBox
    W = new NCGaraDialogBox;
    nw::Field<NCWnd*>(this, 0x1DC) = W;   // SecondDialogBox
    W->SetPosition(347, (INT)(((Height - 108.0f) - 2.0f)), 256, 108);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1DC), 34);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    static_cast<NCGaraDialogBox*>(nw::Field<NCWnd*>(this, 0x1DC))->SetButtonTexts(184, 185);   // NCGaraDialogBox::SetButtonTexts (the second box's)

    // NCMacroListWnd
    W = new NCMacroListWnd;
    nw::Field<NCWnd*>(this, 0x12C) = W;   // NewWndF8
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x12C), 0x3022);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x12C)->HideWindow();
    nw::Field<NCWnd*>(this, 0x12C)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x12C)->SetTitle(*GL2GameData.SysString[711]);

    // NCMacroInfoWnd
    W = new NCMacroInfoWnd;
    nw::Field<NCWnd*>(this, 0x134) = W;   // NewWnd100
    W->SetPosition(20, (INT)((((Height * 0.5f) - 252.0f) + 80.0f)), 256, 161);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x134), 2);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x134)->HideWindow();
    nw::Field<NCWnd*>(this, 0x134)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x134)->SetTitle(*GL2GameData.SysString[657]);

    // NCMacroEditWnd
    W = new NCMacroEditWnd;
    nw::Field<NCWnd*>(this, 0x130) = W;   // NewWndFC
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x130), 0x3002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x130)->HideWindow();
    nw::Field<NCWnd*>(this, 0x130)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x130)->SetTitle(*GL2GameData.SysString[711]);

    // NCShortCutWnd
    W = new NCShortCutWnd(12, 10, 32);
    nw::Field<NCWnd*>(this, 0x138) = W;   // ShortCutWnd
    W->SetPosition((INT)((Width - 46.0f)), (INT)(((Height * 0.5f) - 252.0f)), 46, 504);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x138), 0x85022);
    nw::Field<NCWnd*>(this, 0x138)->HideWindow();
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x138)->Unknown78 = ++GNextWndNumber;

    // NCTrainWnd
    W = new NCTrainWnd;
    nw::Field<NCWnd*>(this, 0x13C) = W;   // TrainWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x13C), 0x1002);
    nw::Field<NCWnd*>(this, 0x13C)->HideWindow();
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x13C)->Unknown78 = ++GNextWndNumber;

    // NCMatchWnd
    W = new NCMatchWnd;
    nw::Field<NCWnd*>(this, 0x140) = W;   // MatchWnd
    W->SetPosition(0, 76, 600, 431);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x140), 0x3002);
    nw::Field<NCWnd*>(this, 0x140)->HideWindow();
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x140)->Unknown78 = ++GNextWndNumber;

    // class 0x1028e010
    W = new NCPartyRoomMakingWnd;
    nw::Field<NCWnd*>(this, 0x144) = W;   // PartyRoomMakingWnd
    W->SetPosition((INT)(((Width * 0.5f) - 178.0f)), (INT)(((Height * 0.5f) - 70.0f)), 356, 141);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x144), 0x10002);
    nw::Field<NCWnd*>(this, 0x144)->HideWindow();
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)

    // class 0x1028ef78
    W = new NCPartyRoomWnd;
    nw::Field<NCWnd*>(this, 0x148) = W;   // PartyRoomWnd
    W->SetPosition(0, 76, 600, 431);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x148), 0x3002);
    nw::Field<NCWnd*>(this, 0x148)->HideWindow();
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x148)->Unknown78 = ++GNextWndNumber;

    // class 0x10290e78
    W = new NCMapCoverWnd;
    nw::Field<NCWnd*>(this, 0x14C) = W;   // MapCoverWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 334, 383);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x14C), 0x3002);
    nw::Field<NCWnd*>(this, 0x14C)->HideWindow();
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x14C)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x14C)->SetTitle(*GL2GameData.SysString[447]);

    // NTownMapWnd
    W = new NTownMapWnd;
    nw::Field<NCWnd*>(this, 0x1E4) = W;   // NewWnd194
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 334, 354);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1E4), 0x1002);
    nw::Field<NCWnd*>(this, 0x1E4)->HideWindow();
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x1E4)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x1E4)->SetTitle(*GL2GameData.SysString[447]);

    // NCObserverWnd
    W = new NCObserverWnd;
    nw::Field<NCWnd*>(this, 0x1E8) = W;   // NewWnd198
    W->SetPosition((INT)(((Local.W * 0.5f) - 51.0f)), 0, 102, 32);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1E8), 0x5002);
    nw::Field<NCWnd*>(this, 0x1E8)->HideWindow();
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x1E8)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x1E8)->SetTitle(*GL2GameData.SysString[664]);

    // NCPrivateShopWnd
    W = new NCPrivateShopWnd;
    nw::Field<NCWnd*>(this, 0x1C0) = W;   // PrivateShopWnd
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1C0), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x1C0)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1C0)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x1C0)->SetTitle(*GL2GameData.SysString[596]);

    // NCPrivateBuyWnd
    W = new NCPrivateBuyWnd;
    nw::Field<NCWnd*>(this, 0x1C4) = W;   // PrivateBuyWnd
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1C4), 0x1002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x1C4)->HideWindow();
    // SetTitle
    nw::Field<NCWnd*>(this, 0x1C4)->SetTitle(*GL2GameData.SysString[597]);

    // NCTutorialViewerWnd
    W = new NCTutorialViewerWnd;
    nw::Field<NCWnd*>(this, 0x154) = W;   // TutorialWnd
    W->SetPosition((INT)(((Local.W - 310.0f) - 20.0f)), 20, 310, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x154), 0x5002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x154)->HideWindow();
    // SetTitle
    nw::Field<NCWnd*>(this, 0x154)->SetTitle(*GL2GameData.SysString[448]);

    // NCTutorialQuestionWnd
    W = new NCTutorialQuestionWnd;
    nw::Field<NCWnd*>(this, 0x158) = W;   // TutorialQuestionWnd
    W->SetPosition(5, (INT)((((Local.H - 187.0f) - 32.0f) - 5.0f)), 32, 32);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x158), 0x81002);
    AttachInline(this, W, E);   // the inline attach: entry, absolute place, WindowProc(WM_CREATE, 0, 0)
    nw::Field<NCWnd*>(this, 0x158)->HideWindow();

    // NCRadarWnd
    W = new NCRadarWnd;
    nw::Field<NCWnd*>(this, 0x164) = W;   // RadarWnd
    W->SetPosition((INT)((Local.W - 102.0f)), 0, 102, 102);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x164), 0x8000A);
    W->AttachEntry(E);

    // NCPetWnd
    W = new NCPetWnd;
    nw::Field<NCWnd*>(this, 0x15C) = W;   // PetWnd
    W->SetPosition(172, 76, 256, 229);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x15C), 0x3002);
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x15C)->HideWindow();
    // SetTitle
    nw::Field<NCWnd*>(this, 0x15C)->SetTitle(*GL2GameData.SysString[494]);

    // NCSummonedWnd
    W = new NCSummonedWnd;
    nw::Field<NCWnd*>(this, 0x160) = W;   // SummonedWnd
    W->SetPosition(172, 76, 256, 201);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x160), 0x3002);
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x160)->HideWindow();
    // SetTitle
    nw::Field<NCWnd*>(this, 0x160)->SetTitle(*GL2GameData.SysString[495]);

    // NCChatWnd
    W = new NCChatWnd;
    nw::Field<NCWnd*>(this, 0xD4) = W;   // ChatWnd
    W->SetPosition(0, (INT)((Height - 187.0f)), 348, 187);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xD4), 0x8100A);
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0xD4)->Unknown78 = ++GNextWndNumber;

    // NCRecipeBookWnd
    W = new NCRecipeBookWnd;
    nw::Field<NCWnd*>(this, 0x16C) = W;   // NewWnd134
    W->SetPosition((INT)((((Local.W - 256.0f) - 46.0f) - 256.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x16C), 0x1022);
    W->AttachEntry(E);
    // SetTitle
    nw::Field<NCWnd*>(this, 0x16C)->SetTitle(*GL2GameData.SysString[663]);

    // class 0x102a5870
    W = new NCRecipeGaraTreeWnd;
    nw::Field<NCWnd*>(this, 0x170) = W;   // RecipeGaraTreeWnd
    W->SetPosition((INT)((((Local.W - 256.0f) - 46.0f) - 256.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x170), 0x1002);
    W->AttachEntry(E);
    // SetTitle
    nw::Field<NCWnd*>(this, 0x170)->SetTitle(*GL2GameData.SysString[662]);

    // NCRecipeManufactureWnd
    W = new NCRecipeManufactureWnd;
    nw::Field<NCWnd*>(this, 0x174) = W;   // NewWnd13C
    W->SetPosition((INT)((((Local.W - 256.0f) - 46.0f) - 256.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x174), 0x1002);
    W->AttachEntry(E);
    // SetTitle
    nw::Field<NCWnd*>(this, 0x174)->SetTitle(*GL2GameData.SysString[663]);

    // NCRecipeShopWnd
    W = new NCRecipeShopWnd;
    nw::Field<NCWnd*>(this, 0x178) = W;   // RecipeShopWnd
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x178), 0x1002);
    W->AttachEntry(E);
    // SetTitle
    nw::Field<NCWnd*>(this, 0x178)->SetTitle(*GL2GameData.SysString[663]);

    // NCRecipeBuyListWnd
    W = new NCRecipeBuyListWnd;
    nw::Field<NCWnd*>(this, 0x17C) = W;   // NewWnd144
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x17C), 0x1002);
    W->AttachEntry(E);
    // SetTitle
    nw::Field<NCWnd*>(this, 0x17C)->SetTitle(*GL2GameData.SysString[663]);

    // NCRecipeBuyManufactureWnd
    W = new NCRecipeBuyManufactureWnd;
    nw::Field<NCWnd*>(this, 0x180) = W;   // RecipeBuyManufactureWnd
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x180), 0x1002);
    W->AttachEntry(E);
    // SetTitle
    nw::Field<NCWnd*>(this, 0x180)->SetTitle(*GL2GameData.SysString[663]);

    // NCHennaListWnd
    W = new NCHennaListWnd;
    nw::Field<NCWnd*>(this, 0x184) = W;   // NewWnd14C
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x184), 0x1002);
    W->AttachEntry(E);

    // NCHennaInfoWnd
    W = new NCHennaInfoWnd;
    nw::Field<NCWnd*>(this, 0x188) = W;   // NewWnd150
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x188), 0x1002);
    W->AttachEntry(E);

    // class 0x102c3728
    W = new ::MessengerWnd;
    nw::Field<NCWnd*>(this, 0x18C) = W;   // MessengerWnd
    W->SetPosition(384, 180, 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x18C), 0x3002);
    W->AttachEntry(E);
    // SetTitle
    nw::Field<NCWnd*>(this, 0x18C)->SetTitle(*GL2GameData.SysString[999]);
    nw::Field<NCWnd*>(this, 0x18C)->Unknown78 = ++GNextWndNumber;

    // NCManorShopWnd
    W = new NCManorShopWnd;
    nw::Field<NCWnd*>(this, 0x118) = W;   // NewWndE4
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x118), 0x1002);
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x118)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x118)->SetTitle(*GL2GameData.SysString[738]);

    // NCGMWnd
    W = new NCGMWnd;
    nw::Field<NCWnd*>(this, 0x190) = W;   // GMWnd
    W->SetPosition((INT)(((Width - 208.0f) - 1.0f)), (INT)((((Height - 26.0f) - 1.0f) - 300.0f)), 208, 229);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x190), 2);
    nw::Field<NCWnd*>(this, 0x190)->HideWindow();
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x190)->Unknown78 = ++GNextWndNumber;

    // NCGMInventoryWnd
    W = new NCGMInventoryWnd;
    nw::Field<NCWnd*>(this, 0x194) = W;   // GMInventoryWnd
    W->SetPosition((INT)(((Local.W - 200.0f) - 1.0f)), 100, 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x194), 2);
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x194)->Unknown78 = ++GNextWndNumber;

    // NCGMMagicSkillWnd
    W = new NCGMMagicSkillWnd;
    nw::Field<NCWnd*>(this, 0x198) = W;   // GMMagicSkillWnd
    W->SetPosition((INT)(((Local.W - 208.0f) - 1.0f)), 100, 256, 355);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x198), 2);
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x198)->Unknown78 = ++GNextWndNumber;

    // NCGMDetailStatusWnd
    W = new NCGMDetailStatusWnd;
    nw::Field<NCWnd*>(this, 0x19C) = W;   // GMDetailStatusWnd
    W->SetPosition(0, 100, 256, 355);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x19C), 2);
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x19C)->Unknown78 = ++GNextWndNumber;

    // NCGMQuestWnd
    W = new NCGMQuestWnd;
    nw::Field<NCWnd*>(this, 0x1A0) = W;   // GMQuestWnd
    W->SetPosition((INT)(((Local.W - 208.0f) - 1.0f)), 100, 256, 355);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1A0), 34);
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1A0)->Unknown78 = ++GNextWndNumber;

    // NCGMStoreWnd
    W = new NCGMStoreWnd;
    nw::Field<NCWnd*>(this, 0x1A4) = W;   // GMStoreWnd
    W->SetPosition((INT)(((Local.W - 208.0f) - 1.0f)), 100, 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1A4), 34);
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1A4)->Unknown78 = ++GNextWndNumber;

    // NCGMActionWnd
    W = new NCGMActionWnd;
    nw::Field<NCWnd*>(this, 0x1A8) = W;   // GMActionWnd
    W->SetPosition((INT)(((Local.W - 208.0f) - 1.0f)), 100, 256, 355);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1A8), 2);
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1A8)->Unknown78 = ++GNextWndNumber;

    // NCSiegeInfoMainWnd
    W = new NCSiegeInfoMainWnd;
    nw::Field<NCWnd*>(this, 0x168) = W;   // SiegeInfoWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 256, 401);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x168), 0x1002);
    W->AttachEntry(E);
    // SetTitle
    nw::Field<NCWnd*>(this, 0x168)->SetTitle(*GL2GameData.SysString[593]);

    // NCXMasSealWnd
    W = new NCXMasSealWnd;
    nw::Field<NCWnd*>(this, 0x1EC) = W;   // NewWnd19C
    W->SetPosition((INT)(((Local.W * 0.5f) - 167.0f)), (INT)(((Local.H * 0.5f) - 178.0f)), 334, 354);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1EC), 0x1002);
    nw::Field<NCWnd*>(this, 0x1EC)->HideWindow();
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1EC)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x1EC)->SetTitle(*GL2GameData.SysString[2]);

    // class 0x102b7dc0
    W = new NCBroadcastGMWnd;
    nw::Field<NCWnd*>(this, 0x1AC) = W;   // BroadcastGMWnd
    W->SetPosition((INT)(((Width * 0.5f) - 350.0f)), (INT)(((Height * 0.5f) - 250.0f)), 700, 500);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1AC), 0x2002);
    nw::Field<NCWnd*>(this, 0x1AC)->HideWindow();
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1AC)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x1AC)->SetTitle(*GL2GameData.SysString[1104]);

    // class 0x102b7898
    W = new NCBroadcastGMFenceWnd;
    nw::Field<NCWnd*>(this, 0x1B0) = W;   // BroadcastGMFenceWnd
    W->SetPosition((INT)(((Width * 0.5f) - 250.0f)), (INT)(((Height * 0.5f) - 150.0f)), 500, 170);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1B0), 2);
    nw::Field<NCWnd*>(this, 0x1B0)->HideWindow();
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1B0)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x1B0)->SetTitle(*GL2GameData.SysString[1105]);

    // class 0x102bac20
    W = new NCBroadcastObserverWnd;
    nw::Field<NCWnd*>(this, 0x1B4) = W;   // BroadcastObserverWnd
    W->SetPosition(0, 0, (INT)(Width), (INT)(Height));
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1B4), 10);
    nw::Field<NCWnd*>(this, 0x1B4)->HideWindow();
    W->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1B4)->Unknown78 = ++GNextWndNumber;
    // SetTitle
    nw::Field<NCWnd*>(this, 0x1B4)->SetTitle(*nw::Field<FString>(&GL2GameData, 0x4C59C));

    // class 0x102b8810
    W = new NCBroadcastObserverGMMsgWnd;
    nw::Field<NCWnd*>(this, 0x1B8) = W;   // BroadcastObserverGMMsgWnd
    // the stock picks the place by the flag at +0x3930 of the console: 0x342 when it is 1
    W->SetPosition((INT)(Width * 0.5f - 360.0f),
                   (nw::Field<INT>(GConsole, 0x3930) == 1) ? 834 : (INT)(Height * 0.5f + 100.0f), 720, 88);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1B8), 10);
    W->AttachEntry(E);

    // class 0x102ba1d8
    W = new NCBroadcastObserverSpecialMsgWnd;
    nw::Field<NCWnd*>(this, 0x1BC) = W;   // BroadcastObserverSpecialMsgWnd
    W->SetPosition((INT)(((Width * 0.5f) - 360.0f)), 390, 720, 320);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1BC), 10);
    W->AttachEntry(E);


    if (GIsOnlyL2Console)
    {
        // NCNPCHtmlViewer: the test page ("test.html")
        W = new NCNPCHtmlViewer;
        nw::Field<NCWnd*>(this, 0x1D0) = W;   // TestNpcHtmlWnd
        W->SetPosition(60, 20, 646, 483);
        E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1D0), 0x1022);
        nw::Field<NCWnd*>(this, 0x1D0)->HideWindow();
        nw::Field<NCWnd*>(this, 0x1D0)->AttachEntry(E);
        // NCHtmlViewer::LoadHtmlFile on the viewer's html view (+0xC8: the stock loads [W + 0xC8] into ecx)
        static_cast<NCNPCHtmlViewer*>(nw::Field<NCWnd*>(this, 0x1D0))->HtmlViewer->LoadHtmlFile((TCHAR*)TEXT("test.html"));
    }
    if (GIsTestHtml)
    {
        // NCTestHtmlViewer
        W = new NCTestHtmlViewer;
        nw::Field<NCWnd*>(this, 0x1D4) = W;   // TestHtmlWnd
        W->SetPosition((INT)(Local.W * 0.5f), (INT)(Local.H * 0.5f), 208, 412);
        E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1D4), 34);
        nw::Field<NCWnd*>(this, 0x1D4)->HideWindow();
        nw::Field<NCWnd*>(this, 0x1D4)->AttachEntry(E);
    }
    BlackTex = FindTexture(TEXT("NWindow.BlackTexture"));
    BalloonTex = FindTexture(TEXT("NWindow.chat"));
    BalloonTimer = SetTimer(NTIMER_BALLOONS, 1000, NULL);
    return 1;
}
NW_HOOK_METHOD(0x10056c70, NCConsole, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x10055780 (vslot 38) - the balloons and the windows the desktop made, then the timer. C3: the order of the stock
// (by offset below: chat, NPC html, help, GM petition, board, the macro windows, the shortcut bar ...), the pet and
// summoned status windows are destroyed too, and of the broadcast windows only the GM's (+0x1AC) and the observer's
// (+0x1B4) are. Names of the offsets: NCConsole.h
#define DESTROY_AT(Off) DestroyChild(this, nw::Field<NCWnd*>(this, Off))
INT NCConsole::OnDestroy()
{
    for (INT i = 0; i < Balloons.Num(); i++)
        if (Balloons(i).Text)
            delete[] Balloons(i).Text;
    Balloons.Empty();
    DESTROY_AT(0xd4); DESTROY_AT(0xd8); DESTROY_AT(0xdc); DESTROY_AT(0xe8); DESTROY_AT(0x150); DESTROY_AT(0x12c);
    DESTROY_AT(0x134); DESTROY_AT(0x130); DESTROY_AT(0x138); DESTROY_AT(0xfc); DESTROY_AT(0x100); DESTROY_AT(0x104);
    DESTROY_AT(0x108); DESTROY_AT(0x10c); DESTROY_AT(0x110); DESTROY_AT(0x114); DESTROY_AT(0x11c); DESTROY_AT(0x120);
    DESTROY_AT(0x124); DESTROY_AT(0x128); DESTROY_AT(0x1d8); DESTROY_AT(0x1dc); DESTROY_AT(0x13c); DESTROY_AT(0x140);
    DESTROY_AT(0x144); DESTROY_AT(0x148); DESTROY_AT(0x14c); DESTROY_AT(0x1e4); DESTROY_AT(0x15c); DESTROY_AT(0x160);
    DESTROY_AT(0x16c); DESTROY_AT(0x170); DESTROY_AT(0x174); DESTROY_AT(0x178); DESTROY_AT(0x17c); DESTROY_AT(0x180);
    DESTROY_AT(0x184); DESTROY_AT(0x188); DESTROY_AT(0x18c); DESTROY_AT(0x190); DESTROY_AT(0x194); DESTROY_AT(0x198);
    DESTROY_AT(0x19c); DESTROY_AT(0x1a0); DESTROY_AT(0x1a4); DESTROY_AT(0x1a8); DESTROY_AT(0x1c0); DESTROY_AT(0x1c4);
    DESTROY_AT(0xc8); DESTROY_AT(0x1f4); DESTROY_AT(0xf0); DESTROY_AT(0x154); DESTROY_AT(0x158); DESTROY_AT(0x164);
    DESTROY_AT(0x1c8); DESTROY_AT(0x1cc); DESTROY_AT(0x168); DESTROY_AT(0x1e0); DESTROY_AT(0xec); DESTROY_AT(0xcc);
    DESTROY_AT(0xd0); DESTROY_AT(0x118); DESTROY_AT(0x1f0); DESTROY_AT(0x1ec); DESTROY_AT(0x1e8); DESTROY_AT(0xf8);
    DESTROY_AT(0xf4); DESTROY_AT(0x1ac); DESTROY_AT(0x1b4);
    if (GIsOnlyL2Console)
        DestroyChild(this, TestNpcHtmlWnd);
    if (GIsTestHtml)
        DestroyChild(this, TestHtmlWnd);
    if (BalloonTimer)
    {
        KillTimer(BalloonTimer);
        BalloonTimer = 0;
    }
    NCWnd::OnDestroy();
    return 1;
}
#undef DESTROY_AT
NW_HOOK_METHOD(0x10055780, NCConsole, OnDestroy)

// 0x10054050 (vslot 44) - entering the game: the bars, the chat, the radar and the status windows show, the rest hides.
// C3: with GIsOnlyMSN (the messenger-only start) every child of the desktop hides and only the messenger window
// shows; the options window is the one of the window at +0x3950; the buffs window shows only while it has something
// to show; the party window hides without members; the observer window goes unless the player's controller has the
// flag at +0xA04 set; with GLanguageType 0 the IME goes to the alphanumeric mode
void NCConsole::ShowWindow()
{
    NCWnd::ShowWindow();
    if (GIsOnlyMSN)
    {
        for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
            Node->Entry->Wnd->HideWindow();
        MessengerWnd->ShowWindow();
        return;
    }
    SiegeInfoWnd->HideWindow();
    DeliverWnd->HideWindow();
    SelectDeliverWnd->HideWindow();
    GMPetitionWnd->HideWindow();
    if (GConsole->PetitionWnd)
        GConsole->PetitionWnd->HideWindow();
    MainWnd->HideWindow();
    SSQMainBoard->HideWindow();
    BoardWnd->HideWindow();
    NewWndF8->HideWindow();
    NewWnd100->HideWindow();
    NewWndFC->HideWindow();
    HelpWnd->HideWindow();
    ShortCutWnd->ShowWindow();
    ChatWnd->ShowWindow();
    InventoryWnd->HideWindow();
    TargetStatusWnd->HideWindow();
    PlayerStatusWnd->ShowWindow();
    MainDialogBox->HideWindow();
    SecondDialogBox->HideWindow();
    MenuWnd->ShowWindow();
    SystemMenuWnd->HideWindow();
    StoreWnd->HideWindow();
    ShopWnd->HideWindow();
    ShopPreviewWnd->HideWindow();
    VIPShopWnd->HideWindow();
    TradeWnd->HideWindow();
    NpcHtmlWnd->HideWindow();
    TrainWnd->HideWindow();
    MatchWnd->HideWindow();
    PartyRoomMakingWnd->HideWindow();
    PartyRoomWnd->HideWindow();
    MapCoverWnd->HideWindow();
    TutorialWnd->HideWindow();
    TutorialQuestionWnd->HideWindow();
    RadarWnd->ShowWindow();
    PetWnd->HideWindow();
    SummonedWnd->HideWindow();
    NewWnd134->HideWindow();
    RecipeGaraTreeWnd->HideWindow();
    NewWnd13C->HideWindow();
    RecipeShopWnd->HideWindow();
    NewWnd144->HideWindow();
    RecipeBuyManufactureWnd->HideWindow();
    NewWnd14C->HideWindow();
    NewWnd150->HideWindow();
    MessengerWnd->HideWindow();
    static_cast<NMsnMessengerView*>(MessengerWnd)->HideAll();   // 0x10139650: its two pages' children hidden
    GMWnd->HideWindow();
    GMInventoryWnd->HideWindow();
    GMMagicSkillWnd->HideWindow();
    GMDetailStatusWnd->HideWindow();
    GMQuestWnd->HideWindow();
    GMStoreWnd->HideWindow();
    GMActionWnd->HideWindow();
    PrivateShopWnd->HideWindow();
    PrivateBuyWnd->HideWindow();
    nw::Field<NCWnd*>(GConsole->GodWnd, 0xD4)->HideWindow();
    PetitionBeginWnd->HideWindow();
    CalcWnd->HideWindow();
    EmpowerWnd->HideWindow();
    NewWnd194->HideWindow();
    NewWndE4->HideWindow();
    ZoneTitleWnd->HideWindow();
    NewWnd19C->HideWindow();
    BroadcastGMWnd->HideWindow();
    BroadcastGMFenceWnd->HideWindow();
    BroadcastObserverWnd->HideWindow();
    BroadcastObserverGMMsgWnd->HideWindow();
    BroadcastObserverSpecialMsgWnd->HideWindow();
    NCWnd* Buffs = AbnormalStatusWnd;
    if (nw::Field<INT>(Buffs, 0xCC) > 0 || nw::Field<INT>(Buffs, 0xE0) != 0 || nw::Field<INT>(Buffs, 0xD8) > 0)
        Buffs->ShowWindow();
    else
        Buffs->HideWindow();
    if (PartyWnd && !nw::Field<INT>(PartyWnd, 0xD4))
        PartyWnd->HideWindow();
    if (NewWnd198)
    {
        APlayerController* Actor = GConsole->Engine->Client->Viewports(0)->Actor;
        ALineagePlayerController* Controller = CastToLineagePlayerController(Actor);
        if (Controller && !(nw::Field<BYTE>(Controller, 0xA04) & 1))
            NewWnd198->HideWindow();
    }
    if (GLanguageType == 0)
        ImmSetConversionStatus(ImmGetContext(GViewportHWnd), 1, 0);
    if (GIsTestHtml)
        TestHtmlWnd->ShowWindow();
}
NW_HOOK_METHOD(0x10054050, NCConsole, ShowWindow)

// stock body shared with NCChatWnd's (0x1003dab0)
void NCConsole::HideWindow()
{
    NCWnd::HideWindow();
}

/*-----------------------------------------------------------------------------
    Toggles and resets (the menus, the shortcut keys, the console).
-----------------------------------------------------------------------------*/

// (repeated inline in the stock CloseWindows)
static FORCEINLINE void HideIfShown(NCWnd* Wnd)
{
    if (Wnd->IsShowWindow())
        Wnd->HideWindow();
}

// 0x100531b0 - the private stores are left (and closed on the server when set up), the recipe shop and the trade
// cancelled, the party room left, the options and petition windows hidden, then the whole interface shown or hidden
// (C3: nothing while a replay plays; the party matching window is the party room's (+0x148) with its own body)
void NCConsole::ToggleInterface()
{
    if (GL2ReplayMode)
        return;
    if (PrivateShopWnd->IsShowWindow())
    {
        if (PrivateShopWnd->Mode == 1)
            GConsole->RequestPrivateStoreQuitSell();
        PrivateShopWnd->Reset();   // NCPrivateShopWnd::Reset (0x100D3E40)
    }
    if (PrivateBuyWnd->IsShowWindow())
    {
        if (PrivateBuyWnd->Mode == 1)
            GConsole->RequestPrivateStoreQuitBuy();
        PrivateBuyWnd->Reset();    // the sell window's body for both (0x100D3E40; ours is the same code, same fields)
    }
    if (RecipeShopWnd->IsShowWindow())
    {
        GConsole->RequestRecipeShopManageQuit();
        static_cast<NCRecipeShopWnd*>(RecipeShopWnd)->ClearLists();
    }
    if (TradeWnd->IsShowWindow())
        GConsole->RequestTradeDone(0);
    if (PartyRoomWnd->IsShowWindow())
        static_cast<NCPartyRoomWnd*>(PartyRoomWnd)->RequestExit();
    NCWnd* Options = nw::Field<NCWnd*>(GConsole->GodWnd, 0xD4);   // C3: the window at +0x3950 holds the options window
    if (Options)
        Options->HideWindow();
    if (GConsole->PetitionWnd)
        GConsole->PetitionWnd->HideWindow();
    if (IsShowWindow())
        HideWindow();
    else
        ShowWindow();
}
NW_HOOK_METHOD(0x100531b0, NCConsole, ToggleInterface)

// 0x100532f0 - the dialogs, shops, stores, menus and the radar close (C3: the shop preview, the board of the SSQ and
// the main window too; the petition window only when there is one)
void NCConsole::CloseWindows()
{
    HideIfShown(InventoryWnd);
    HideIfShown(ShopWnd);
    HideIfShown(ShopPreviewWnd);
    HideIfShown(VIPShopWnd);
    HideIfShown(StoreWnd);
    HideIfShown(TradeWnd);
    HideIfShown(SystemMenuWnd);
    HideIfShown(HelpWnd);
    HideIfShown(MainDialogBox);
    HideIfShown(SecondDialogBox);
    HideIfShown(RadarWnd);
    HideIfShown(PrivateShopWnd);
    HideIfShown(PrivateBuyWnd);
    HideIfShown(MainWnd);
    HideIfShown(SSQMainBoard);
    if (GConsole->PetitionWnd)
        HideIfShown(GConsole->PetitionWnd);
    HideIfShown(GMPetitionWnd);
    HideIfShown(DeliverWnd);
    HideIfShown(SelectDeliverWnd);
    HideIfShown(SiegeInfoWnd);
}
NW_HOOK_METHOD(0x100532f0, NCConsole, CloseWindows)

// 0x10053590 - leaving the game: the dialogs, the chat, the shortcuts, the buffs, the party room, the clan page, the radar
// and both petition windows empty (C3: also the character window's last page). The calls on the windows whose classes
// are not declared are the stock's own bodies
void NCConsole::Reset()
{
    MainDialogBox->Reset();
    SecondDialogBox->Reset();
    ChatWnd->ClearAll();
    ShortCutWnd->ClearShortCuts();
    ((NCAbnormalStatusWndExt*)AbnormalStatusWnd)->ClearAll();               // 0x10102C50
    MatchWnd->ClearList();                                                  // 0x100A4DE0
    MainWnd->ActionWnd->ResetPledgeWnd();                                   // 0x1004B030
    static_cast<NCPledgeWnd*>(MainWnd->PledgeWnd)->Reset();                 // NCPledgeWnd::Reset 0x10048300
    RadarWnd->ClearTargets();                                               // 0x100DE820
    GMPetitionWnd->Clear();                                                 // NCPetitionWnd::Clear 0x100D04B0 (the GM's copy)
    PlayerPetitionWnd()->Clear();
    static_cast<NCQuestGaraTreeWnd*>(MainWnd->QuestWnd)->ClearMark();      // 0x100DB5D0
}
NW_HOOK_METHOD(0x10053590, NCConsole, Reset)

// C1 only: the C3 image has no function of this name (the petition windows are toggled inline by their callers); an
// inert body, so that a stray call does nothing instead of stopping the game
void NCConsole::TogglePetition()
{
}

// 0x10053630
void NCConsole::ToggleSystemMenu()
{
    if (SystemMenuWnd->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(SOUND_SystemClose);    // C3: its own sounds (C2: window close / open)
        SystemMenuWnd->HideWindow();
    }
    else
    {
        GConsole->PlayInterfaceSound(SOUND_SystemOpen);
        SystemMenuWnd->ShowWindow();
        SystemMenuWnd->SetFocus();
    }
}
NW_HOOK_METHOD(0x10053630, NCConsole, ToggleSystemMenu)

// 0x10053690 - not while a shop (mode 1), a warehouse (mode 0), a trade or the freight is
// open; opening asks the server for the item list (its answer shows the window)
void NCConsole::ToggleInventory()
{
    if (ShopWnd->IsShowWindow() && StoreWndMode(ShopWnd) == 1)
        return;
    if (StoreWnd->IsShowWindow() && !StoreWndMode(StoreWnd))
        return;
    if (TradeWnd->IsShowWindow() || DeliverWnd->IsShowWindow())
        return;
    if (InventoryWnd->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(SOUND_InventoryClose);    // C3: its own sounds (C2: window close / open)
        InventoryWnd->HideWindow();
    }
    else
    {
        GConsole->RequestItemList();
        GConsole->PlayInterfaceSound(SOUND_InventoryOpen);
    }
}
NW_HOOK_METHOD(0x10053690, NCConsole, ToggleInventory)

// 0x10053740 - the messenger window (+0x18C, new in C3): hidden when shown, else shown and focused (tail calls)
void NCConsole::ToggleChat()
{
    if (MessengerWnd->IsShowWindow())
        MessengerWnd->HideWindow();
    else
    {
        MessengerWnd->ShowWindow();
        MessengerWnd->SetFocus();
    }
}
NW_HOOK_METHOD(0x10053740, NCConsole, ToggleChat)

// 0x100537d0 - the help page; bMacro (C2, name ours) opens the macro help instead
void NCConsole::ToggleHelp(UBOOL bMacro)
{
    if (HelpWnd->IsShowWindow())
    {
        HelpWnd->HideWindow();
        return;
    }
    HelpWnd->LoadFile((TCHAR*)(bMacro ? TEXT("..\\L2text\\help_macro.htm") : TEXT("..\\L2text\\help.htm")));
    HelpWnd->ShowWindow();
    HelpWnd->SetFocus();
}
NW_HOOK_METHOD(0x100537d0, NCConsole, ToggleHelp)

// C1 only: no function of this name in the C3 image (the map paired it with 0x10004EA0, a one-argument setter folded
// with NCComboBox's: not NCConsole's); nobody calls it, an inert body so that a stray call cannot run that setter
void NCConsole::TogglePartyMatch()
{
}

// 0x10053830 - C3: the map cover window (+0x14C) takes a mode number into its child's +0x16C, then is hidden when
// shown (the map close sound), else shown and focused (the same sound)
void NCConsole::ToggleMinimap(INT Mode)
{
    if (!MapCoverWnd)
        return;
    nw::Field<INT>(nw::Field<BYTE*>(MapCoverWnd, 0xC8), 0x16C) = Mode;
    if (MapCoverWnd->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(SOUND_MapClose);
        MapCoverWnd->HideWindow();
        return;
    }
    MapCoverWnd->ShowWindow();
    MapCoverWnd->SetFocus();
    GConsole->PlayInterfaceSound(SOUND_MapClose);
}
NW_HOOK_METHOD(0x10053830, NCConsole, ToggleMinimap)

// C1 only: no function of this name in the C3 image (the matching windows were redone); an inert body
void NCConsole::ToggleMatchOption()
{
}

// 0x100538b0 - opening asks the server for the board unless a command is pending (the flag of the board's html page,
// +0x154); the tab control (+0xD8) goes back to its first tab (C3 SetTab takes two arguments)
void NCConsole::ToggleBoard()
{
    if (BoardWnd->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(SOUND_WindowClose);
        BoardWnd->HideWindow();
        return;
    }
    if (nw::Field<INT>(nw::Field<BYTE*>(BoardWnd, 0xC8), 0x154))
        return;
    L2ParamStack Params(10);
    GConsole->RequestShowBoard(Params);
    static_cast<NCBoardWnd*>(BoardWnd)->TabCtrl->SetTab(0, 0);   // NCTabControl::SetTab (0x1002A350)
    BoardWnd->HideWindow();
    GConsole->PlayInterfaceSound(SOUND_WindowOpen);
    nw::Field<INT>(nw::Field<BYTE*>(BoardWnd, 0xC8), 0x154) = 1;
}
NW_HOOK_METHOD(0x100538b0, NCConsole, ToggleBoard)

// 0x100539b0 - the macro window (+0xF8): shown and focused with the sound of opening a window (5), or hidden with the
// sound of closing it (6)
void NCConsole::ToggleMacroList()
{
    if (NewWndF8->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(6);
        NewWndF8->HideWindow();
        return;
    }
    NewWndF8->ShowWindow();
    NewWndF8->SetFocus();
    GConsole->PlayInterfaceSound(5);
}
NW_HOOK_METHOD(0x100539b0, NCConsole, ToggleMacroList)

// 0x10053a10 - the macro window the editor key opens, the same way (C3: the window made second after the list, at +0x134;
// C2 toggled the one at +0xFC)
void NCConsole::ToggleMacroEdit()
{
    if (NewWnd100->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(6);
        NewWnd100->HideWindow();
        return;
    }
    NewWnd100->ShowWindow();
    NewWnd100->SetFocus();
    GConsole->PlayInterfaceSound(5);
}
NW_HOOK_METHOD(0x10053a10, NCConsole, ToggleMacroEdit)

// 0x10053d50 - the Christmas seal window (+0x1EC): hidden when shown, else shown and focused (tail calls)
void NCConsole::ToggleXMasSeal()
{
    if (NewWnd19C->IsShowWindow())
        NewWnd19C->HideWindow();
    else
    {
        NewWnd19C->ShowWindow();
        NewWnd19C->SetFocus();
    }
}
NW_HOOK_METHOD(0x10053d50, NCConsole, ToggleXMasSeal)

// 0x10053ad0 - the GM window's name box takes the focus
void NCConsole::ToggleGMWnd()
{
    if (GMWnd->IsShowWindow())
    {
        GMWnd->HideWindow();
        return;
    }
    GMWnd->ShowWindow();
    GMWnd->NameEdit->SetFocus();
}
NW_HOOK_METHOD(0x10053ad0, NCConsole, ToggleGMWnd)

// the GM view windows: hidden, or the server asked about the player Name and the window
// shown (repeated in the six stock toggles)
#define NCCONSOLE_GM_TOGGLE(Method, Wnd, Request)                                             \
    void NCConsole::Method(TCHAR* Name)                                                       \
    {                                                                                         \
        if (Wnd->IsShowWindow())                                                              \
        {                                                                                     \
            Wnd->HideWindow();                                                                \
            return;                                                                           \
        }                                                                                     \
        GConsole->Request(Name);                                                              \
        Wnd->ShowWindow();                                                                    \
        Wnd->SetFocus();                                                                      \
    }

// 0x10053b10
NCCONSOLE_GM_TOGGLE(ToggleGMInventory, GMInventoryWnd, RequestGMInventory)
NW_HOOK_METHOD(0x10053b10, NCConsole, ToggleGMInventory)
// 0x10053b70
NCCONSOLE_GM_TOGGLE(ToggleGMMagicSkill, GMMagicSkillWnd, RequestGMSkills)
NW_HOOK_METHOD(0x10053b70, NCConsole, ToggleGMMagicSkill)
// 0x10053bd0
NCCONSOLE_GM_TOGGLE(ToggleGMDetailStatus, GMDetailStatusWnd, RequestGMStatus)
NW_HOOK_METHOD(0x10053bd0, NCConsole, ToggleGMDetailStatus)
// 0x10053c30
NCCONSOLE_GM_TOGGLE(ToggleGMQuest, GMQuestWnd, RequestGMQuests)
NW_HOOK_METHOD(0x10053c30, NCConsole, ToggleGMQuest)
// 0x10053c90
NCCONSOLE_GM_TOGGLE(ToggleGMStore, GMStoreWnd, RequestGMWarehouse)
NW_HOOK_METHOD(0x10053c90, NCConsole, ToggleGMStore)
// 0x10053cf0
NCCONSOLE_GM_TOGGLE(ToggleGMAction, GMActionWnd, RequestGMPledge)
NW_HOOK_METHOD(0x10053cf0, NCConsole, ToggleGMAction)

// 0x10054df0 - a tab of the character window (NCMainWnd::ShowTab), or with -1 the window shown (and focused) or hidden
void NCConsole::ToggleCharacter(INT Tab)
{
    if (Tab != -1)
    {
        MainWnd->ShowTab(Tab);
        return;
    }
    if (MainWnd->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(SOUND_CharClose);
        MainWnd->HideWindow();
    }
    else
    {
        MainWnd->ShowWindow();
        MainWnd->SetFocus();
        GConsole->PlayInterfaceSound(SOUND_CharOpen);
    }
}
NW_HOOK_METHOD(0x10054df0, NCConsole, ToggleCharacter)

/*-----------------------------------------------------------------------------
    Chat.
-----------------------------------------------------------------------------*/

// 0x10053d90 - a system message (type 5) into the chat window in the color given, when its line has a text
void NCConsole::AddSystemMessage(INT MsgID, DWORD Color)
{
    FL2SystemMsgData& Msg = GL2GameData.SystemMsg[MsgID];
    if (*Msg.Message)
        ChatWnd->AddChatMessage(*Msg.Message, Color, 5, 0);
}
NW_HOOK_METHOD(0x10053d90, NCConsole, AddSystemMessage)

// 0x10053df0 - a system line (type 5), when not empty
void NCConsole::AddSystemText(const TCHAR* Text, INT SubType, DWORD Color)
{
    if (Text && *Text)
        ChatWnd->AddChatMessage(Text, Color, 5, SubType);
}
NW_HOOK_METHOD(0x10053df0, NCConsole, AddSystemText)

// 0x10054e70 - "Name: Text" of the chat types but the petition ones (6, 7: into the GM petition window, or the
// player's, shown and focused, when the GM one is hidden). C3: type 14 goes to the party room window (+0x148, its
// chat list). Then the speaker's balloon: its first 19 characters ("..." after a longer text) for 5 seconds, a
// newer one replacing it - not when the speaker has a private store (User +0x1F0)
void NCConsole::Say(INT ObjectID, const TCHAR* Name, const TCHAR* Text, DWORD Color, INT Type)
{
    TCHAR Line[1024];
    _swprintf(Line, TEXT("%s: %s"), Name, Text);
    if (Type == 6 || Type == 7)
    {
        GMPetitionWnd->AddMessage(Line, Color, Type, 0);
        if (!GMPetitionWnd->IsShowWindow())
        {
            GConsole->PetitionWnd->ShowWindow();
            GConsole->PetitionWnd->SetFocus();
            PlayerPetitionWnd()->AddMessage(Line, Color, Type, 0);
        }
        return;
    }
    if (Type == 0xE)
        static_cast<NCPartyRoomWnd*>(PartyRoomWnd)->AddChatLine(Line, Color);   // the room's chat list takes the line
    else
        ChatWnd->AddChatMessage(Line, Color, Type, 0);
    User* Speaker = GConsole->FindUserByID(ObjectID);
    if (Speaker && Speaker->PrivateStoreType)
        return;
    NChatBalloon Balloon;
    TCHAR* Short = new TCHAR[23];
    if (wcslen(Text) < 19)
        appStrcpy(Short, Text);
    else
    {
        wcsncpy(Short, Text, 19);
        Short[19] = '.';
        Short[20] = '.';
        Short[21] = '.';
        Short[22] = 0;
    }
    Balloon.Tick = Ticks;
    Balloon.ObjectID = ObjectID;
    Balloon.Text = Short;
    INT i;
    for (i = 0; i < Balloons.Num(); i++)
        if (Balloons(i).ObjectID == ObjectID)
            break;
    if (i < Balloons.Num())
    {
        if (Balloons(i).Text)
            delete[] Balloons(i).Text;
        Balloons(i) = Balloon;
    }
    else
        Balloons(Balloons.Add()) = Balloon;
    Speaker = GConsole->FindUserByID(ObjectID);
    if (Speaker && Speaker->Pawn)
    {
        if (!Speaker->Pawn->ChatBalloon)
            Speaker->Pawn->Unknown5C++;
        Speaker->Pawn->ChatBalloon = Short;
    }
}
NW_HOOK_METHOD(0x10054e70, NCConsole, Say)

/*-----------------------------------------------------------------------------
    HTML page commands.
-----------------------------------------------------------------------------*/

// 0x10054540 (vslot 68) - "Link <bypass>": to the server (the tutorial pages' links go their own way). "Write
// <board> <a> <b> <control> [<control> [<control>]]": a board post, the texts of the page's controls named; boards 4,
// 5 and 7 take "-" for the second text, 4 and 7 for the third. C3: a missing third text is no post at all (returns
// 0 without sending; C2 sent it)
INT NCConsole::OnHtmlCommand(const TCHAR* Command, NCHtmlViewer* Source)
{
    const TCHAR* Str = Command;
    if (ParseCommand(&Str, TEXT("Link")))
    {
        TCHAR Bypass[256] = { 0 };
        if (ParseToken(Str, Bypass, 256, 1))
        {
            L2ParamStack Params(10);
            Params.PushBack(Bypass);
            if (Source->IsA(NClass::NCTutorialHtmlViewer()))
                GConsole->RequestTutorialLinkHtml(Params);
            else
                GConsole->RequestLinkHtml(Params);
        }
        return 1;
    }
    if (!ParseCommand(&Str, TEXT("Write")))
        return 0;
    L2ParamStack Params(10);
    TCHAR Board[256], Arg1[256], Arg2[256], Control1[256], Control2[256], Control3[256];
    if (!ParseToken(Str, Board, 256, 1))
        return 0;
    Params.PushBack(Board);
    if (!ParseToken(Str, Arg1, 256, 1))
        return 0;
    Params.PushBack(Arg1);
    if (!ParseToken(Str, Arg2, 256, 1))
        return 0;
    Params.PushBack(Arg2);
    if (!ParseToken(Str, Control1, 256, 1) || !Source)
        return 0;
    TCHAR* Text = Source->GetControlValue(Control1);
    if (!Text)
        return 0;
    Params.PushBack(Text);
    if (appStrcmp(Board, TEXT("4")) && appStrcmp(Board, TEXT("5")) && appStrcmp(Board, TEXT("7")))
    {
        if (!ParseToken(Str, Control2, 256, 1))
            return 0;
        Text = Source->GetControlValue(Control2);
        if (!Text)
            return 0;
        Params.PushBack(Text);
    }
    else
    {
        appStrcpy(Control2, TEXT("-"));
        Params.PushBack(Control2);
    }
    if (appStrcmp(Board, TEXT("4")) && appStrcmp(Board, TEXT("7")))
    {
        if (ParseToken(Str, Control3, 256, 1))
        {
            Text = Source->GetControlValue(Control3);
            if (!Text)
                return 0;
            Params.PushBack(Text);
        }
    }
    else
    {
        appStrcpy(Control3, TEXT("-"));
        Params.PushBack(Control3);
    }
    GConsole->RequestBBSWrite(Params);
    return 1;
}
NW_HOOK_METHOD(0x10054540, NCConsole, OnHtmlCommand)

// 0x10053fc0 - a balloon's destructor (the text is freed by hand; Say's unwind code)
NW_HOOK_DTOR(0x10053fc0, NChatBalloon)

// 0x10056ab0 - the chat balloons go, then NCWnd
NW_HOOK_DTOR(0x10056ab0, NCConsole)

// 0x10053e20 (vslot 43) - guarded: the desktop is resized, and the broadcast observer window with it
void NCConsole::SetWindowSize(FLOAT W, FLOAT H)
{
    guard(NCConsole::SetWindowSize);
    NCWnd::SetWindowSize(W, H);
    if (BroadcastObserverWnd)
        BroadcastObserverWnd->SetWindowSize(W, H);
    unguard;
}
NW_HOOK_METHOD(0x10053e20, NCConsole, SetWindowSize)

NW_CHECK_VTABLE(NCConsole, 0x101839d0, 69)
