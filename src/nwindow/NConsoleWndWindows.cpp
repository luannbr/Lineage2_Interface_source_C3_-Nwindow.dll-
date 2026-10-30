/*=============================================================================
    NConsoleWndWindows.cpp - the console's windows: their creation for the game and for the
    pawn viewer mode, their destruction, their positions (WindowsInfo.ini and the defaults),
    the dialog boxes, the message and warning lines of the login screens, the option window,
    the way back from the character creation, the texts after a language change, and the
    debug window's commands.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCGodWnd.h"
#include "nwindow/NCPetitionFeedBackWnd.h"
#include "nwindow/NCReplayWnd.h"
#include "nwindow/NCBroadcastObserverWnd.h"
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCActionWnd.h"
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDebugWnd.h"
#include "nwindow/NCDetailStatusWnd.h"
#include "nwindow/NCCreditWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGaraDialogBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCGMWnd.h"
#include "nwindow/NCIcon.h"
#include "nwindow/L2WordWrapperWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCLoadingWnd.h"
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCMatchWnd.h"
#include "nwindow/NCMenuWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCMSViewrWnd.h"
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NCNpcViewrWnd.h"
#include "nwindow/NCNPCHtmlViewer.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCPawnCreateWnd.h"
#include "nwindow/NCPawnSelectWnd.h"
#include "nwindow/NCPawnViewrWnd.h"
#include "nwindow/NCPetitionWnd.h"
#include "nwindow/NCPetWnd.h"
#include "nwindow/NCPrivateBuyWnd.h"
#include "nwindow/NCPrivateShopWnd.h"
#include "nwindow/NCRestartMenuWnd.h"
#include "nwindow/NCServerSelectWnd.h"
#include "nwindow/NCShopWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NCSummonedWnd.h"
#include "nwindow/NCSystemMenuWnd.h"
#include "nwindow/NCTradeWnd.h"
#include "nwindow/NCTrainWnd.h"
#include "nwindow/NCTutorialViewerWnd.h"
#include "nwindow/NCVirtualWndMain.h"
#include "nwindow/NCWarningDialogBox.h"
#include "nwindow/NCWarningWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NKeyCrypt.h"

#if NW_FEATURES
// phase 6: WindowFix (the mode the game starts in)
#include "features/WindowFix.h"
#endif

#include <stdarg.h>
#include <stdio.h>

// interface sounds (FL2NetNotify::OnInterfacePlaySound; names ours)
enum { SOUND_WindowOpen = 5, SOUND_WindowClose = 6 };

// sysstring lines
enum { STR_OK = 140, STR_Cancel = 141, STR_Options = 146, STR_Petition = 470, STR_PetitionFeedBack = 848 };

// the copyright line under the login screens (a systemmsg line)
enum { SYSMSG_Copyright = 94 };

// package globals (names ours): the debug log switch ("L2Debug", the debug window's lines)
// and the auto login of AutoLog.txt
#if NW_HOOK_MODE
#define GL2DebugLog     NW_GLOBAL(UBOOL, 0x1027B538)
#define GIsL2AutoLogOn  NW_GLOBAL(UBOOL, 0x1027B540)
#define GGameGuardConsole NW_GLOBAL(NConsoleWnd*, 0x1027BA6C)   // (name ours) the console, set at the end of Initialize
#else
extern UBOOL GL2DebugLog;
extern UBOOL GIsL2AutoLogOn;
extern NConsoleWnd* GGameGuardConsole;
#endif

// the viewport's window (UViewport::GetWindow, pure in Engine.dll: WinDrv.dll's)
// C3 (console-core): re-enabled
static FORCEINLINE HWND ViewportWindow(UViewport* Viewport) { return (HWND)Viewport->GetWindow(); }

// The window files: WindowsInfo.ini keeps each desktop window's place in the section of its
// number (NCWnd +0x78, given by the desktop's OnCreate).
static const TCHAR GWindowsInfoIni[] = TEXT("WindowsInfo.ini");

// A virtual variadic method: NW_HOOK_VARARGS cannot take its address (MSVC gives a vcall
// thunk), but our vtable's slot holds the method itself.
extern "C" const void* const nwvt_NConsoleWnd[];
#define NW_HOOK_VARARGS_SLOT(va, Class, Method, Slot)                                            \
    namespace {                                                                                  \
    ::nw::HookReg NW_CAT(nwHook_, __LINE__)(va, (void*)nwvt_##Class[Slot], #Class "::" #Method,  \
                                            __FILE__);                                           \
    }

/*-----------------------------------------------------------------------------
    Creation and destruction.
-----------------------------------------------------------------------------*/

// a top window registered with the window manager and attached to its entry (NCWnd's
// AttachEntry, a call here)
static FORCEINLINE void AttachTopWindow(NConsoleWnd* Console, NCWnd* Wnd, DWORD Style)
{
    Wnd->AttachEntry(Console->WndMain->RegisterNWindow(Wnd, Style, NULL));
}

// the login screen's windows by their C3 offsets (names ours; NCLoginWnd.h still has the C2 order)
static FORCEINLINE NCWnd* LoginChild(NConsoleWnd* Console, DWORD Offset)
{
    return nw::Field<NCWnd*>(Console->LoginWnd, Offset);
}

// the same with AttachEntry inline (as the stock code has it for some windows)
static FORCEINLINE void CreateTopWnd(NConsoleWnd* Console, NCWnd* Wnd, DWORD Style)
{
    NWndEntry* E = Console->WndMain->RegisterNWindow(Wnd, Style, NULL);
    Wnd->Entry = E;
    if (E->Parent)
    {
        NCWnd* Parent = E->Parent->Wnd;
        Wnd->ScreenX += Parent->ScreenX;
        Wnd->ScreenY += Parent->ScreenY;
    }
    Wnd->WindowProc(WM_CREATE, 0, 0);
}

// the debug window, as both initializations make it
static FORCEINLINE void SetupDebugWnd(NCDebugWnd* DebugWnd)
{
    DebugWnd->SetBackTexture(TEXT("NWindow.ChatBack"));
    DebugWnd->LineGap = 0;
    DebugWnd->TextX = 30;
    DebugWnd->TextY = 3;
    DebugWnd->LoadCategories();
    DebugWnd->bAutoScroll = 1;
    DebugWnd->HideWindow();
}

// A top window the stock makes out of line (NCGodWnd, NCPetitionFeedBackWnd, NCReplayWnd; C3): its object allocated as the
// stock does (StaticAllocateObject of the class in the transient package; nothing without it), then the class's
// constructor (0x100861C0, 0x10077A10, 0x100E7720: our C++ constructors, called, not inlined) run on it
template <class T> static FORCEINLINE NCWnd* NewTopWnd()
{
    UObject* Object = UObject::StaticAllocateObject(T::StaticClass());
    if (!Object)
        return NULL;
    return new ((EInternal*)Object) T;
}

// 0x10077f20 - once: the globals (the game window, the console, the canvas), the key protection object
// (GL2UseKeyCrypt), the debug switch, and every top window - the window manager, (but for the test consoles) the
// login screen, the character selection and creation, the loading screen, the credit window, the debug window and
// the login stages' dialog box, then the petition window, the petition feedback window, the desktop, the NCGodWnd
// (the restart menu and the options window live in it) and the Thai word wrapper (GL2ThaiHtmlMode). The auto login
// of AutoLog.txt fills the login in (its texts are sprintf formats) and disables the buttons that would leave it.
// The saved window positions last. C3 against C2: no video mode here (SetRes is gone), no message line under the
// login screen (its height is still taken off), no restart menu or warning box of their own, the credit window has
// style 0x200002, the NCGodWnd is made after the desktop and only the test consoles hide it (and show the desktop);
// the stock's call after the positions (with GIsL2NetLogin, a packager's patch made the jump unconditional) never runs
UBOOL NConsoleWnd::Initialize(HWND hWnd)
{
    guard(NConsoleWnd::Initialize);
    if (bInitialized)
        return 1;
    GViewportHWnd = hWnd;
    GConsole = this;
    GCanvas = Engine->Client->Viewports(0)->Canvas;
    if (GL2UseKeyCrypt)
    {
        void* Memory = GMalloc->Malloc(sizeof(NKeyCrypt), TEXT("new"));
        KeyCrypt = Memory ? ::new (Memory) NKeyCrypt : NULL;
    }
    INT ShortCutDelay = 500;
    if (!GConfig->GetInt(TEXT("URL"), TEXT("ShortCutDelay"), ShortCutDelay))
        ShortCutDelay = 500;
    if (!GConfig->GetBool(TEXT("URL"), TEXT("L2Debug"), GL2DebugLog))
        GL2DebugLog = 0;

    WndMain = new NCVirtualWndMain;
    WndMain->LoadCursors();
    INT ScreenX = GSEKScreenX;
    INT ScreenY = GSEKScreenY;
    if (!GIsOnlyL2Console && !GIsTestHtml)
    {
        LoginWnd = new NCLoginWnd;
        LoginWnd->SetPosition(0, 0, ScreenX, ScreenY - 52);
        AttachTopWindow(this, LoginWnd, NWS_VISIBLE);
        LoginWnd->ShowWindow();

        LobbyWnd = new NCLobbyWnd;
        LobbyWnd->SetPosition(0, 0, ScreenX, ScreenY - 52);
        AttachTopWindow(this, LobbyWnd, NWS_NOHITTEST);
        LobbyWnd->HideWindow();

        PawnCreateWnd = new NCPawnCreateWnd;
        PawnCreateWnd->SetPosition(0, 0, ScreenX, ScreenY - 52);
        AttachTopWindow(this, PawnCreateWnd, NWS_VISIBLE);
        PawnCreateWnd->HideWindow();

        LoadingWnd = new NCLoadingWnd;
        LoadingWnd->SetPosition(0, 0, ScreenX, ScreenY);
        AttachTopWindow(this, LoadingWnd, NWS_VISIBLE);
        LoadingWnd->HideWindow();

        CreditWnd = new NCCreditWnd;
        CreditWnd->SetPosition(0, 0, ScreenX, ScreenY);
        AttachTopWindow(this, CreditWnd, NWS_VISIBLE | NWS_NOFOCUSSEARCH);
        CreditWnd->HideWindow();

        DebugWnd = new NCDebugWnd(150, 20);
        DebugWnd->SetPosition(0, 0, GSEKScreenX, 246);
        AttachTopWindow(this, DebugWnd, NWS_VISIBLE);
        SetupDebugWnd(DebugWnd);

        DialogBox = (NCDialogBox*)new NCGaraDialogBox;
        DialogBox->SetPosition(GSEKScreenX / 2 - 128, GSEKScreenY / 2 - 54, 256, 108);
        AttachTopWindow(this, DialogBox, NWS_VISIBLE);
        DialogBox->HideWindow();
        DialogBox->SetButtonTexts(STR_OK, STR_Cancel);
    }

    // the petition windows: the stock inlines the constructor of the first (an NCWnd constructor and the class's
    // vtable), the second one's is out of line
    PetitionWnd = new NCPetitionWnd;
    PetitionWnd->SetPosition(106, GSEKScreenY / 2 - 192, 310, 301);
    CreateTopWnd(this, PetitionWnd, NWS_VISIBLE);
    PetitionWnd->SetTitle(*GL2GameData.SysString[STR_Petition]);
    PetitionWnd->HideWindow();

    WarningDialogBox = NewTopWnd<NCPetitionFeedBackWnd>();
    WarningDialogBox->SetPosition(GSEKScreenX - 256, GSEKScreenY - 401, 256, 401);
    CreateTopWnd(this, WarningDialogBox, NWS_VISIBLE);
    WarningDialogBox->SetTitle(*GL2GameData.SysString[STR_PetitionFeedBack]);
    WarningDialogBox->HideWindow();

    Desktop = new NCConsole(this);
    Desktop->SetPosition(0, 0, GSEKScreenX, GSEKScreenY);
    CreateTopWnd(this, Desktop, NWS_VISIBLE | NWS_NOHITTEST);
    Desktop->HideWindow();
    // the shortcut bar's key delay: the stock calls an empty `ret 4` (0x1012F080, folded with the empty
    // ReceiveMagicList) when the desktop's shortcut window exists. Standalone: nothing (the call does nothing)
#if NW_HOOK_MODE
    if (Desktop && GetDesktop()->ShortCutWnd)
        ::nw::StockCallAt<void>(0x1012F080, GetDesktop()->ShortCutWnd, ShortCutDelay);
#endif

    GodWnd = NewTopWnd<NCGodWnd>();
    GodWnd->SetPosition(0, 0, ScreenX, ScreenY);
    CreateTopWnd(this, GodWnd, NWS_VISIBLE | NWS_NOHITTEST);

    if (GL2ThaiHtmlMode)
    {
        WordWrapperWnd = new L2WordWrapperWnd;
        WordWrapperWnd->SetPosition(GSEKScreenX / 2, GSEKScreenY / 2, 300, 300);
        CreateTopWnd(this, WordWrapperWnd, NWS_VISIBLE);
        LoginWnd->HideWindow();
        GetDesktop()->ToggleInterface();
        GetDesktop()->ChatWnd->HideWindow();
        GetDesktop()->PlayerStatusWnd->HideWindow();
    }
    if (GIsOnlyL2Console || GIsTestHtml)
    {
        GodWnd->HideWindow();
        Desktop->ShowWindow();
    }

    if (!GConfig->GetBool(TEXT("AutoLogOn"), TEXT("IsL2AutoLogOn"), GIsL2AutoLogOn, TEXT("AutoLog.txt")))
        GIsL2AutoLogOn = 0;
    else if (GIsL2AutoLogOn)
    {
        TCHAR Account[64];
        TCHAR Password[64];
        appSprintf(Account, GConfig->GetStr(TEXT("AutoLogOn"), TEXT("L2ID"), TEXT("AutoLog.txt")));
        appSprintf(Password, GConfig->GetStr(TEXT("AutoLogOn"), TEXT("L2Passwd"), TEXT("AutoLog.txt")));
        LoginWnd->AuthWnd->AccountEdit->SetText(Account);
        LoginWnd->AuthWnd->PasswordEdit->SetText(Password);
        LoginWnd->AuthWnd->OKButton->SetFocus();
        nw::Field<NCWnd*>(LobbyWnd->FunctionWnd, 0xCC)->EnableWindow(0);
        nw::Field<NCWnd*>(LobbyWnd->FunctionWnd, 0xD0)->EnableWindow(0);
        nw::Field<NCWnd*>(LobbyWnd->FunctionWnd, 0xC8)->EnableWindow(0);
        nw::Field<NCWnd*>(GetDesktop()->SystemMenuWnd, 0xD4)->EnableWindow(0);   // the system menu's restart button
    }
    if (!GIsOnlyL2Console)
        LoadWindowPositions();
    GGameGuardConsole = this;
    bInitialized = 1;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10077f20, NConsoleWnd, Initialize)

// 0x10073ec0 - the pawn viewer mode (the engine's InitForPawnViewer): the window manager, the debug window, the
// desktop (shown for the test consoles), the three viewers at the right edge of the game window - the pawn viewer
// (style 0x10002), the NPC viewer and the skill viewer - all hidden, and the loading window (the picture 0). C3:
// no video mode here either, the viewers' names are their titles
UBOOL NConsoleWnd::InitializePawnViewer(HWND hWnd)
{
    guard(NConsoleWnd::InitializePawnViewer);
    if (!bInitialized)
    {
        GViewportHWnd = hWnd;
        GCanvas = Engine->Client->Viewports(0)->Canvas;
        INT ScreenX = GSEKScreenX;
        INT ScreenY = GSEKScreenY;
        GConsole = this;
        WndMain = new NCVirtualWndMain;
        WndMain->LoadCursors();

        DebugWnd = new NCDebugWnd(150, 20);
        DebugWnd->SetPosition(0, 0, ScreenX, 246);
        CreateTopWnd(this, DebugWnd, NWS_VISIBLE);
        SetupDebugWnd(DebugWnd);

        Desktop = new NCConsole(this);
        Desktop->SetPosition(0, 0, GSEKScreenX, GSEKScreenY);
        CreateTopWnd(this, Desktop, NWS_VISIBLE | NWS_NOHITTEST);
        Desktop->HideWindow();

        RECT Client;
        GetClientRect(hWnd, &Client);

        PawnViewerWnd[0] = new NCPawnViewrWnd;
        PawnViewerWnd[0]->SetPosition(Client.right - 241, 100, 240, 599);
        CreateTopWnd(this, PawnViewerWnd[0], NWS_VISIBLE | 0x10000);
        PawnViewerWnd[0]->SetTitle(TEXT("PAWNVIEWER"));
        PawnViewerWnd[0]->HideWindow();

        PawnViewerWnd[1] = new NCNpcViewrWnd;
        PawnViewerWnd[1]->SetPosition(Client.right - 241, 100, 240, 275);
        CreateTopWnd(this, PawnViewerWnd[1], NWS_VISIBLE);
        PawnViewerWnd[1]->SetTitle(TEXT("NPCVIEWER"));
        PawnViewerWnd[1]->HideWindow();

        PawnViewerWnd[2] = new NCMSViewrWnd;
        PawnViewerWnd[2]->SetPosition(Client.right - 241, 100, 240, 400);
        CreateTopWnd(this, PawnViewerWnd[2], NWS_VISIBLE);
        PawnViewerWnd[2]->SetTitle(TEXT("SKILLVIEWER"));
        PawnViewerWnd[2]->HideWindow();

        if (GIsOnlyL2Console || GIsTestHtml)
            Desktop->ShowWindow();

        LoadingWnd = new NCLoadingWnd;
        LoadingWnd->SetPosition(0, 0, ScreenX, ScreenY);
        CreateTopWnd(this, LoadingWnd, NWS_VISIBLE);
        LoadingWnd->HideWindow();
        LoadingWnd->SetLoadingTexture(0);
        bInitialized = 1;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10073ec0, NConsoleWnd, InitializePawnViewer)

// 0x100744a0 (ret 4) - Initialize for the replay mode (C3): the globals and the window manager, then four top windows,
// each hidden once it is made - the replay window (NCReplayWnd, kept at +0x39E0), the loading window (picture 0), the
// desktop and the NCGodWnd (its +0xC8 set, which keeps its OnCreate from making the restart menu) - and the saved
// window positions unless the console is the test one. None of the login stages' windows is made
UBOOL NConsoleWnd::InitializeReplayMode(HWND hWnd)
{
    guard(NConsoleWnd::InitializeReplayMode);
    if (bInitialized)
        return 1;
    GViewportHWnd = hWnd;
    GConsole = this;
    GCanvas = Engine->Client->Viewports(0)->Canvas;
    WndMain = new NCVirtualWndMain;
    WndMain->LoadCursors();
    INT ScreenX = GSEKScreenX;
    INT ScreenY = GSEKScreenY;

    ReplayWnd = NewTopWnd<NCReplayWnd>();
    ReplayWnd->SetPosition(0, 0, ScreenX, ScreenY);
    CreateTopWnd(this, ReplayWnd, NWS_VISIBLE | NWS_NOHITTEST);
    ReplayWnd->HideWindow();

    LoadingWnd = new NCLoadingWnd;
    LoadingWnd->SetPosition(0, 0, ScreenX, ScreenY);
    CreateTopWnd(this, LoadingWnd, NWS_VISIBLE);
    LoadingWnd->HideWindow();
    LoadingWnd->SetLoadingTexture(0);

    Desktop = new NCConsole(this);
    Desktop->SetPosition(0, 0, GSEKScreenX, GSEKScreenY);
    CreateTopWnd(this, Desktop, NWS_VISIBLE | NWS_NOHITTEST);
    Desktop->HideWindow();

    GodWnd = NewTopWnd<NCGodWnd>();
    nw::Field<INT>(GodWnd, 0xC8) = 1;               // NCGodWnd::UnknownC8
    GodWnd->SetPosition(0, 0, ScreenX, ScreenY);
    CreateTopWnd(this, GodWnd, NWS_VISIBLE | NWS_NOHITTEST);
    GodWnd->HideWindow();

    if (!GIsOnlyL2Console)
        LoadWindowPositions();
    bInitialized = 1;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100744a0, NConsoleWnd, InitializeReplayMode)

// 0x10074890 (ret 4) - the UI mode (0 the game's windows, 1 the observer's, a broadcast or a match watched): the windows
// of the desktop that are shown are remembered in the list of the mode being left and hidden, the ones of the new mode
// (remembered the same way when it was left) are shown. The engine's canvas is told (slot 75 finishes, 74 starts its
// broadcast observer mode); in mode 1 the desktop's observer window is shown and given the focus. Nothing happens for
// the mode it is in, for a mode other than 0 and 1, or without a desktop. (The stock runs each child through
// CastCheckedNCWnd, 0x10030BA0: a type check only)
void NConsoleWnd::SwitchUIMode(INT NewMode)
{
    guard(NConsoleWnd::SwitchUIMode);
    if (UIMode != NewMode && NewMode >= 0 && NewMode < 2 && Desktop)
    {
        NWndPtrArray& Old = ModeWnds[UIMode];
        Old.Empty();
        for (NWndListNode* Node = Desktop->ChildHead; Node; Node = Node->Next)
        {
            NCWnd* Child = Node->Entry->Wnd;
            if (Child->IsShowWindow())
            {
                INT Index = Old.Add();
                Old(Index) = Child;
            }
        }
        Old.Shrink();
        for (INT i = 0; i < ModeWnds[UIMode].Num(); i++)
        {
            NCWnd* Wnd = ModeWnds[UIMode](i);
            if (Wnd)
                Wnd->HideWindow();
        }
        for (INT i = 0; i < ModeWnds[NewMode].Num(); i++)
        {
            NCWnd* Wnd = ModeWnds[NewMode](i);
            if (Wnd)
                Wnd->ShowWindow();
        }
        UIMode = NewMode;
        if (NewMode == 0)
            ::nw::VirtualCall<void>(Engine->Client->Viewports(0)->Canvas, 75);   // UCanvas::FinishBroadcastObserverMode
        else
        {
            ::nw::VirtualCall<void>(Engine->Client->Viewports(0)->Canvas, 74);   // UCanvas::StartBroadcastObserverMode
            NCWnd* Observer = GetDesktop()->BroadcastObserverWnd;
            if (Observer)
            {
                Observer->ShowWindow();
                Observer->SetFocus();
            }
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x10074890, NConsoleWnd, SwitchUIMode)

// 0x10075030 (C3 virtual 359, ret 0x30) - FL2NetNotify's OnEventMatchObserver: the mode 0 or 1 of the match watched (any other
// leaves the windows as they are) switches the UI mode; for 1 the desktop's observer window (NCBroadcastObserverWnd,
// +0x1B4) is told with the first number and the two texts (their copies are the stock's arguments of its
// NotifyStartObserver, 0x10117BE0, ret 0x1c). The other numbers are not read
void NConsoleWnd::OnEventMatchObserver(INT A, ANSICHAR Mode, ANSICHAR C, FString D, FString E, INT F, INT G, INT H)
{
    guard(NConsoleWnd::OnEventMatchObserver);
    if (Mode == 0)
        SwitchUIMode(0);
    else if (Mode == 1)
    {
        SwitchUIMode(1);
        NCConsole* Console = static_cast<NCConsole*>(Desktop);
        if (Console && Console->BroadcastObserverWnd)
            static_cast<NCBroadcastObserverWnd*>(Console->BroadcastObserverWnd)->NotifyStartObserver(A, D, E);
    }
    unguard;
}
NW_HOOK_METHOD(0x10075030, NConsoleWnd, OnEventMatchObserver)

// a top window unregistered and deleted
template <class T> static FORCEINLINE void DestroyTopWnd(NConsoleWnd* Console, T*& Wnd)
{
    if (Wnd)
    {
        Console->WndMain->UnRegisterNWindow(Wnd->Entry);
        delete Wnd;
        Wnd = NULL;
    }
}

// 0x1006ba90 - the top windows unregistered and deleted, in the order of the stock. C3: the window at +0x3950 (the
// options and the message window) is not one of them, the key protection object is at +0x39C0 (made by Initialize
// when GL2UseKeyCrypt is set: the stock closes its driver, then runs its destructor and frees it) and the three pawn
// viewer windows at +0x39D4 .. +0x39DC. The empty loop over the timers is the stock's (their destructors)
void NConsoleWnd::Finalize()
{
    guard(NConsoleWnd::Finalize);
    if (!bInitialized)
        return;
    for (INT i = 0; i < Timers.Num(); i++)
    {
    }
    Timers.Empty();
    GViewportHWnd = NULL;
    DestroyTopWnd(this, PetitionWnd);
    DestroyTopWnd(this, WarningDialogBox);
    DestroyTopWnd(this, LoginWnd);
    DestroyTopWnd(this, LobbyWnd);
    DestroyTopWnd(this, PawnCreateWnd);
    DestroyTopWnd(this, LoadingWnd);
    DestroyTopWnd(this, CreditWnd);
    DestroyTopWnd(this, Desktop);
    DestroyTopWnd(this, DebugWnd);
    DestroyTopWnd(this, UnusedWnd);
    DestroyTopWnd(this, DialogBox);
    DestroyTopWnd(this, PawnViewerWnd[0]);
    DestroyTopWnd(this, PawnViewerWnd[1]);
    DestroyTopWnd(this, PawnViewerWnd[2]);
    if (WndMain)
    {
        WndMain->DestroyCursors();
        WndMain->DeleteGDevice();
        delete WndMain;
        WndMain = NULL;
    }
    if (GL2UseKeyCrypt && KeyCrypt)
    {
        KeyCrypt->Close();   // closes the driver (NPKCloseDriver)
        if (KeyCrypt)        // (the stock reads the field again: the C++ delete of a pointer)
        {
            KeyCrypt->Close();        // its destructor, a jump to Close (NKeyCrypt.cpp has no body for ~NKeyCrypt yet)
            GMalloc->Free(KeyCrypt);
        }
    }
    bInitialized = 0;
    unguard;
}
NW_HOOK_METHOD(0x1006ba90, NConsoleWnd, Finalize)

// 0x10078ab0 (vslot 280) - the engine hands the console over, with its viewport's window
UBOOL NConsoleWnd::Init(UEngine* InEngine)
{
    guard(NConsoleWnd::Init);
    if (!InEngine)
        return 0;
    Engine = InEngine;
    Initialize(ViewportWindow(InEngine->Client->Viewports(0)));
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10078ab0, NConsoleWnd, Init)

// 0x10077770 (vslot 297) - the same for the pawn viewer mode
UBOOL NConsoleWnd::InitForPawnViewer(UEngine* InEngine)
{
    guard(NConsoleWnd::InitForPawnViewer);
    if (!InEngine)
        return 0;
    Engine = InEngine;
    InitializePawnViewer(ViewportWindow(InEngine->Client->Viewports(0)));
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10077770, NConsoleWnd, InitForPawnViewer)

// 0x10077820 (C3 virtual 325, ret 4) - the same for the replay mode; the state goes to "in the world"
UBOOL NConsoleWnd::InitReplayMode(UEngine* InEngine)
{
    guard(NConsoleWnd::InitReplayMode);
    if (!InEngine)
        return 0;
    Engine = InEngine;
    InitializeReplayMode(ViewportWindow(InEngine->Client->Viewports(0)));
    SetState(L2CS_InGame);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10077820, NConsoleWnd, InitReplayMode)

// 0x10074870 (vslot 10)
void NConsoleWnd::Destroy()
{
    Finalize();
    UObject::Destroy();
}
NW_HOOK_METHOD(0x10074870, NConsoleWnd, Destroy)

/*-----------------------------------------------------------------------------
    The window positions.
-----------------------------------------------------------------------------*/

// a desktop window's saved place, when both keys are there
static FORCEINLINE void LoadWindowPos(NCWnd* Wnd, INT& X, INT& Y)
{
    if (GConfig->GetInt(appItoa(Wnd->Unknown78), TEXT("posX"), X, GWindowsInfoIni) &&
        GConfig->GetInt(appItoa(Wnd->Unknown78), TEXT("posY"), Y, GWindowsInfoIni))
        Wnd->SetPosition(X, Y, 0, 0);
}

// 0x10067a70 - the desktop windows where WindowsInfo.ini left them (Initialize): the chat's height (from its default,
// 174), the player window's width (from 172), the shortcut bar's direction. C3: nothing without a desktop; the
// list follows the stock's order, the party matching window is the party room (+0x148) and the map cover
// (+0x14C), the messenger window (+0x18C) and the recipe tree (+0x170) are new
void NConsoleWnd::LoadWindowPositions()
{
    if (!Desktop)
        return;
    INT X, Y;
    if (GConfig->GetInt(appItoa(GetDesktop()->ChatWnd->Unknown78), TEXT("Height"), X, GWindowsInfoIni))
        GetDesktop()->ChatWnd->ResizeBy(174 - X);
    LoadWindowPos(GetDesktop()->InventoryWnd, X, Y);
    LoadWindowPos(GetDesktop()->NpcHtmlWnd, X, Y);
    LoadWindowPos(GetDesktop()->PlayerStatusWnd, X, Y);
    if (GConfig->GetInt(appItoa(GetDesktop()->PlayerStatusWnd->Unknown78), TEXT("width"), X,
                        GWindowsInfoIni))
        GetDesktop()->PlayerStatusWnd->Resize(X - 172);
    LoadWindowPos(GetDesktop()->BoardWnd, X, Y);
    UBOOL bHorizontal = 0;
    if (GConfig->GetBool(appItoa(GetDesktop()->ShortCutWnd->Unknown78), TEXT("rotate"), bHorizontal,
                         GWindowsInfoIni))
    {
        nw::Field<INT>(GetDesktop()->ShortCutWnd, 0x240) = bHorizontal;   // bHorizontal
        ::nw::VirtualCall<void>(GetDesktop()->ShortCutWnd, 69);          // UpdateLayout (vslot 69)
    }
    LoadWindowPos(GetDesktop()->ShortCutWnd, X, Y);
    LoadWindowPos(GetDesktop()->MainWnd, X, Y);
    LoadWindowPos(GetDesktop()->TargetStatusWnd, X, Y);
    LoadWindowPos(GetDesktop()->TradeWnd, X, Y);
    LoadWindowPos(GetDesktop()->HelpWnd, X, Y);
    LoadWindowPos(GetDesktop()->MatchWnd, X, Y);
    LoadWindowPos(GetDesktop()->PartyRoomWnd, X, Y);
    LoadWindowPos(GetDesktop()->MapCoverWnd, X, Y);
    LoadWindowPos(GetDesktop()->PrivateShopWnd, X, Y);
    LoadWindowPos(GetDesktop()->PartyWnd, X, Y);
    LoadWindowPos(GetDesktop()->PetStatusWnd, X, Y);
    LoadWindowPos(GetDesktop()->SummonedStatusWnd, X, Y);
    LoadWindowPos(GetDesktop()->StoreWnd, X, Y);
    LoadWindowPos(GetDesktop()->ShopWnd, X, Y);
    LoadWindowPos(GetDesktop()->TrainWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMPetitionWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMActionWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMStoreWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMQuestWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMDetailStatusWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMMagicSkillWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMInventoryWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMWnd, X, Y);
    LoadWindowPos(GetDesktop()->AbnormalStatusWnd, X, Y);
    LoadWindowPos(GetDesktop()->MenuWnd, X, Y);
    LoadWindowPos(GetDesktop()->NewWndFC, X, Y);
    LoadWindowPos(GetDesktop()->NewWndF8, X, Y);
    LoadWindowPos(GetDesktop()->NewWnd100, X, Y);
    LoadWindowPos(GetDesktop()->NewWnd134, X, Y);
    LoadWindowPos(GetDesktop()->NewWnd144, X, Y);
    LoadWindowPos(GetDesktop()->RecipeBuyManufactureWnd, X, Y);
    LoadWindowPos(GetDesktop()->NewWnd13C, X, Y);
    LoadWindowPos(GetDesktop()->RecipeShopWnd, X, Y);
    LoadWindowPos(GetDesktop()->RecipeGaraTreeWnd, X, Y);
    LoadWindowPos(GetDesktop()->NewWndE4, X, Y);
    LoadWindowPos(GetDesktop()->MessengerWnd, X, Y);
}
NW_HOOK_METHOD(0x10067a70, NConsoleWnd, LoadWindowPositions)

// a desktop window's place saved
static FORCEINLINE void SaveWindowPos(NCWnd* Wnd)
{
    INT Number = Wnd->Unknown78;
    INT X = Wnd->ScreenX;   // C2 and C3: the absolute place (C1 saved LocalX / LocalY)
    INT Y = Wnd->ScreenY;
    GConfig->SetInt(appItoa(Number), TEXT("posX"), X, GWindowsInfoIni);
    GConfig->SetInt(appItoa(Number), TEXT("posY"), Y, GWindowsInfoIni);
}

// 0x10066940 - the desktop windows' places into WindowsInfo.ini (the logout, AcceptLogOut), with the shortcut bar's
// direction, the player window's width and the chat's and the recipe book's heights (the floats truncated). C3: the
// list follows the stock's order; the party matching window is the party room (+0x148) and the map cover (+0x14C),
// the messenger window (+0x18C) and the recipe tree (+0x170) are new; the windows' height is a float
void NConsoleWnd::SaveWindowPositions()
{
    SaveWindowPos(GetDesktop()->InventoryWnd);
    SaveWindowPos(GetDesktop()->NpcHtmlWnd);
    SaveWindowPos(GetDesktop()->HelpWnd);
    SaveWindowPos(GetDesktop()->ShortCutWnd);
    GConfig->SetBool(appItoa(GetDesktop()->ShortCutWnd->Unknown78), TEXT("rotate"),
                     nw::Field<INT>(GetDesktop()->ShortCutWnd, 0x240), GWindowsInfoIni);
    SaveWindowPos(GetDesktop()->TargetStatusWnd);
    SaveWindowPos(GetDesktop()->BoardWnd);
    SaveWindowPos(GetDesktop()->TradeWnd);
    SaveWindowPos(GetDesktop()->MatchWnd);
    SaveWindowPos(GetDesktop()->PartyRoomWnd);
    SaveWindowPos(GetDesktop()->MapCoverWnd);
    SaveWindowPos(GetDesktop()->PrivateShopWnd);
    SaveWindowPos(GetDesktop()->MainWnd);
    {
        NCWnd* Player = GetDesktop()->PlayerStatusWnd;
        INT Number = Player->Unknown78;
        INT X = Player->ScreenX;
        INT Y = Player->ScreenY;
        GConfig->SetInt(appItoa(Number), TEXT("posX"), X, GWindowsInfoIni);
        GConfig->SetInt(appItoa(Number), TEXT("posY"), Y, GWindowsInfoIni);
        GConfig->SetInt(appItoa(Number), TEXT("width"), (INT)Player->Width, GWindowsInfoIni);
    }
    SaveWindowPos(GetDesktop()->PartyWnd);
    SaveWindowPos(GetDesktop()->PetStatusWnd);
    SaveWindowPos(GetDesktop()->SummonedStatusWnd);
    SaveWindowPos(GetDesktop()->StoreWnd);
    SaveWindowPos(GetDesktop()->ShopWnd);
    SaveWindowPos(GetDesktop()->TrainWnd);
    SaveWindowPos(GetDesktop()->GMPetitionWnd);
    SaveWindowPos(GetDesktop()->GMActionWnd);
    SaveWindowPos(GetDesktop()->GMStoreWnd);
    SaveWindowPos(GetDesktop()->GMQuestWnd);
    SaveWindowPos(GetDesktop()->GMDetailStatusWnd);
    SaveWindowPos(GetDesktop()->GMMagicSkillWnd);
    SaveWindowPos(GetDesktop()->GMInventoryWnd);
    SaveWindowPos(GetDesktop()->GMWnd);
    SaveWindowPos(GetDesktop()->AbnormalStatusWnd);
    SaveWindowPos(GetDesktop()->MenuWnd);
    GConfig->SetInt(appItoa(GetDesktop()->ChatWnd->Unknown78), TEXT("Height"), (INT)GetDesktop()->ChatWnd->Height,
                    GWindowsInfoIni);
    GConfig->SetInt(appItoa(GetDesktop()->NewWnd134->Unknown78), TEXT("Height"),
                    (INT)GetDesktop()->NewWnd134->Height, GWindowsInfoIni);
    SaveWindowPos(GetDesktop()->NewWndF8);
    SaveWindowPos(GetDesktop()->NewWndFC);
    SaveWindowPos(GetDesktop()->NewWnd100);
    SaveWindowPos(GetDesktop()->NewWnd134);
    SaveWindowPos(GetDesktop()->NewWnd144);
    SaveWindowPos(GetDesktop()->NewWnd13C);
    SaveWindowPos(GetDesktop()->RecipeShopWnd);
    SaveWindowPos(GetDesktop()->RecipeGaraTreeWnd);
    SaveWindowPos(GetDesktop()->NewWndE4);
    SaveWindowPos(GetDesktop()->MessengerWnd);
}
NW_HOOK_METHOD(0x10066940, NConsoleWnd, SaveWindowPositions)

// a window's local rectangle (a copy of LocalX .. LocalH): SetScreenPos takes its corner as
// the origin
#if 0  // C3: does not compile against the new headers (db/demote.csv)
struct NWndRect
{
    INT X;
    INT Y;
    INT W;
    INT H;

    NWndRect() : X(0), Y(0), W(0), H(0) {}
    void Set(NCWnd* Wnd)
    {
        X = Wnd->LocalX;
        Y = Wnd->LocalY;
        H = Wnd->LocalH;
        W = Wnd->LocalW;
    }
    const POINT* Origin() const { return reinterpret_cast<const POINT*>(this); }
};
#endif

// a minimized window back to its size
static FORCEINLINE void RestoreIfMinimized(NCWnd* Wnd)
{
    if (Wnd->Icon->IsShowWindow())
        Wnd->Restore();
}

// 0x100694d0 - every window back to its default place (the options window's reset button): the minimized ones
// restored, the desktop to the screen's size, then each window at its own offset from the desktop's corner (SetScreenPos
// from the corner `Origin`, the desktop's local rectangle). The offsets are the stock's constants: half the screen's
// height less 252 (a window's middle), the right edge less its width, and so on. Windows are reached by the desktop's
// offsets (their names after NCConsole.h).
void NConsoleWnd::ResetWindowPositions()
{
    NCConsole* Desk = GetDesktop();
    if (!Desk)
        return;
#define DESK_WND(Offset) ::nw::Field<NCWnd*>(Desk, Offset)
    RestoreIfMinimized(DESK_WND(0xC8));      // MainWnd
    RestoreIfMinimized(DESK_WND(0x140));     // MatchWnd
    RestoreIfMinimized(DESK_WND(0x148));     // PartyRoomWnd
    RestoreIfMinimized(DESK_WND(0x124));     // SystemMenuWnd
    RestoreIfMinimized(DESK_WND(0x150));     // BoardWnd
    RestoreIfMinimized(DESK_WND(0x14C));     // MapCoverWnd
    RestoreIfMinimized(DESK_WND(0x15C));     // PetWnd
    RestoreIfMinimized(DESK_WND(0x160));     // SummonedWnd
    RestoreIfMinimized(DESK_WND(0xFC));      // InventoryWnd

    NRectF Origin;       // the desktop's own rectangle: the corner every window below is placed from
    NRectF DialogRect;   // read for the two dialog boxes and not used
    Origin.X = 0;
    Origin.Y = 0;
    Origin.W = 0.0f;
    Origin.H = 0.0f;
    DialogRect.X = 0;
    DialogRect.Y = 0;
    DialogRect.W = 0.0f;
    DialogRect.H = 0.0f;
    Desk->GetLocalRect(&Origin);
    if (GodWnd)
        static_cast<NCGodWnd*>(GodWnd)->DefaultPosition();
    Desk->SetWindowSize((FLOAT)GSEKScreenX, (FLOAT)GSEKScreenY);
    Desk->GetLocalRect(&Origin);

    // the chat window sits on the bottom edge and keeps its height; its resize is told its height less 187
    NRectF ChatRect;
    ChatRect.X = 0;
    ChatRect.Y = 0;
    ChatRect.W = 0.0f;
    ChatRect.H = 0.0f;
    Desk->ChatWnd->GetLocalRect(&ChatRect);
    Desk->ChatWnd->SetScreenPos(0, (INT)((FLOAT)GSEKScreenY - ChatRect.H), (const POINT*)&Origin);
    Desk->ChatWnd->ResizeBy((INT)(ChatRect.H - 187.0f));

    DESK_WND(0xD8)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);      // NpcHtmlWnd
    DESK_WND(0xDC)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);      // HelpWnd
    // the short cut bar goes vertical again
    Desk->ShortCutWnd->bHorizontal = 0;
    Desk->ShortCutWnd->UpdateLayout();
    Desk->ShortCutWnd->SetScreenPos(GSEKScreenX - 0x2E, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);
    DESK_WND(0xC8)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);      // MainWnd

    NRectF DeskRect;
    DeskRect.X = 0;
    DeskRect.Y = 0;
    DeskRect.W = 0.0f;
    DeskRect.H = 0.0f;
    Desk->GetLocalRect(&DeskRect);
    Desk->PlayerStatusWnd->SetScreenPos(0, 0, (const POINT*)&Origin);
    Desk->PlayerStatusWnd->Resize((INT)(172.0f - DeskRect.W));
    DESK_WND(0x100)->SetScreenPos(GSEKScreenX / 2 - 0x56, 0, (const POINT*)&Origin);    // TargetStatusWnd
    DESK_WND(0x108)->SetScreenPos(0xAC, 0, (const POINT*)&Origin);                      // AbnormalStatusWnd
    ::nw::VirtualCall<void>(DESK_WND(0x108), 68);                                       // its NewVirtual68
    DESK_WND(0x14C)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // MapCoverWnd
    DESK_WND(0x154)->SetScreenPos(GSEKScreenX - 0x14A, 0x14, (const POINT*)&Origin);    // TutorialWnd
    DESK_WND(0x150)->SetScreenPos(GSEKScreenX / 2 - 0x143, GSEKScreenY / 2 - 0x109, (const POINT*)&Origin);   // BoardWnd
    DESK_WND(0x140)->SetScreenPos(0, 0x4C, (const POINT*)&Origin);                      // MatchWnd
    DESK_WND(0x144)->SetScreenPos(GSEKScreenX / 2 - 0xB2, GSEKScreenY / 2 - 0x46, (const POINT*)&Origin);   // PartyRoomMakingWnd
    DESK_WND(0x148)->SetScreenPos(0, 0x4C, (const POINT*)&Origin);                      // PartyRoomWnd
    DESK_WND(0x10C)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // StoreWnd
    DESK_WND(0x110)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // ShopWnd
    DESK_WND(0x120)->SetScreenPos(GSEKScreenX - 0xAD, GSEKScreenY - 0x2E, (const POINT*)&Origin);   // MenuWnd
    DESK_WND(0x124)->SetScreenPos(GSEKScreenX - 0xAC, GSEKScreenY - 0x155, (const POINT*)&Origin);  // SystemMenuWnd
    DESK_WND(0x1D8)->GetLocalRect(&DialogRect);                                         // MainDialogBox
    DESK_WND(0x1D8)->SetScreenPos(GSEKScreenX / 2 - 0x80, GSEKScreenY / 2 - 0x36, (const POINT*)&Origin);
    DESK_WND(0x1DC)->GetLocalRect(&DialogRect);                                         // SecondDialogBox
    DESK_WND(0x1DC)->SetScreenPos(0x15B, GSEKScreenY - 0x6E, (const POINT*)&Origin);
    DESK_WND(0xF0)->SetScreenPos(0, 0x98, (const POINT*)&Origin);                       // PartyWnd
    DESK_WND(0xF4)->SetScreenPos(0, 0x4C, (const POINT*)&Origin);                       // PetStatusWnd
    DESK_WND(0xF8)->SetScreenPos(0, 0x4C, (const POINT*)&Origin);                       // SummonedStatusWnd
    // the tutorial's question mark: above the chat window
    DESK_WND(0x158)->SetScreenPos(5, (INT)((FLOAT)GSEKScreenY - ChatRect.H - 32.0f - 5.0f), (const POINT*)&Origin);
    DESK_WND(0x13C)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // TrainWnd
    PetitionWnd->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);
    DESK_WND(0x15C)->SetScreenPos(0xAC, 0x4C, (const POINT*)&Origin);                   // PetWnd
    DESK_WND(0x160)->SetScreenPos(0xAC, 0x4C, (const POINT*)&Origin);                   // SummonedWnd
    DESK_WND(0x16C)->SetScreenPos(GSEKScreenX - 0x22E, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);  // RecipeBookWnd
    DESK_WND(0x174)->SetScreenPos(GSEKScreenX - 0x22E, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);  // RecipeManufactureWnd
    DESK_WND(0x170)->SetScreenPos(GSEKScreenX - 0x22E, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);  // RecipeGaraTreeWnd
    DESK_WND(0x118)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // ManorShopWnd
    DESK_WND(0x190)->SetScreenPos(GSEKScreenX - 0xD1, GSEKScreenY - 0x147, (const POINT*)&Origin);   // GMWnd
    DESK_WND(0x194)->SetScreenPos(GSEKScreenX - 0xD1, 100, (const POINT*)&Origin);      // GMInventoryWnd
    DESK_WND(0x198)->SetScreenPos(GSEKScreenX - 0xD1, 100, (const POINT*)&Origin);      // GMMagicSkillWnd
    DESK_WND(0x19C)->SetScreenPos(0, 100, (const POINT*)&Origin);                       // GMDetailStatusWnd
    DESK_WND(0x1A0)->SetScreenPos(GSEKScreenX - 0xD1, 100, (const POINT*)&Origin);      // GMQuestWnd
    DESK_WND(0x1A4)->SetScreenPos(GSEKScreenX - 0xD1, 100, (const POINT*)&Origin);      // GMStoreWnd
    DESK_WND(0x1A8)->SetScreenPos(GSEKScreenX - 0xD1, 100, (const POINT*)&Origin);      // GMActionWnd
    DESK_WND(0xE8)->SetScreenPos(0x6A, GSEKScreenY / 2 - 0xC0, (const POINT*)&Origin);  // GMPetitionWnd
    DESK_WND(0xCC)->SetScreenPos((GSEKScreenX - 0x100) / 2, (GSEKScreenY - 0xD3) / 2, (const POINT*)&Origin);   // CalcWnd
    DESK_WND(0xD0)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);     // EmpowerWnd
    DESK_WND(0xEC)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);     // PetitionBeginWnd
    DESK_WND(0x1E8)->SetScreenPos(GSEKScreenX / 2 - 0x33, 0, (const POINT*)&Origin);    // ObserverWnd
    DESK_WND(0x130)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // MacroEditWnd
    DESK_WND(0x12C)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // MacroListWnd
    DESK_WND(0x134)->SetScreenPos(0x14, GSEKScreenY / 2 - 0xAC, (const POINT*)&Origin); // MacroInfoWnd
    DESK_WND(0x168)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // SiegeInfoWnd
    DESK_WND(0x11C)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // VIPShopWnd
    DESK_WND(0x184)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // HennaListWnd
    DESK_WND(0x188)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // HennaInfoWnd
    DESK_WND(0x1CC)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // SelectDeliverWnd
    DESK_WND(0x1C8)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // DeliverWnd
    DESK_WND(0x1F4)->SetScreenPos(0, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);    // SSQMainBoard
    DESK_WND(0xFC)->SetScreenPos(GSEKScreenX - 0x12E, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);   // InventoryWnd
    DESK_WND(0x128)->SetScreenPos(GSEKScreenX - 0x12E, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);  // TradeWnd
    DESK_WND(0x17C)->SetScreenPos(GSEKScreenX - 0x12E, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);  // RecipeBuyListWnd
    DESK_WND(0x180)->SetScreenPos(GSEKScreenX - 0x12E, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);  // RecipeBuyManufactureWnd
    DESK_WND(0x178)->SetScreenPos(GSEKScreenX - 0x12E, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);  // RecipeShopWnd
    DESK_WND(0x1C0)->SetScreenPos(GSEKScreenX - 0x12E, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);  // PrivateShopWnd
    DESK_WND(0x1C4)->SetScreenPos(GSEKScreenX - 0x12E, GSEKScreenY / 2 - 0xFC, (const POINT*)&Origin);  // PrivateBuyWnd
    DESK_WND(0x18C)->SetScreenPos(0x180, 0xB4, (const POINT*)&Origin);                  // MessengerWnd
#undef DESK_WND
}
NW_HOOK_METHOD(0x100694D0, NConsoleWnd, ResetWindowPositions)

/*-----------------------------------------------------------------------------
    Dialogs and the login screens.
-----------------------------------------------------------------------------*/

// a string the dialog fills in is there and not empty
static FORCEINLINE UBOOL HasText(const FString& Text)
{
    return Text.Num() && *Text.GetCharData();
}

// 0x100664a0 - a dialog box: its text with the parameters put in (FormatSystemMessage, into
// Text2) in the desktop's main box or its second one; each takes the focus (C3; C2: the main box either way).
// C2: nothing is shown when the box has neither Text2 nor Text
void NConsoleWnd::ShowDialog(NDialogInfo* Info)
{
    TCHAR Buffer[0x1000];
    FormatSystemMessage(Buffer, *Info->Text, *Info->Button1, *Info->Button2);
    Info->Text2 = Buffer;
    if (Info->bMainBox)
    {
        if (HasText(Info->Text2) || HasText(Info->Text))
        {
            GetDesktop()->MainDialogBox->SetInfo(Info);
            GetDesktop()->MainDialogBox->ShowWindow();
            GetDesktop()->MainDialogBox->SetFocus();
        }
    }
    else if (HasText(Info->Text2) || HasText(Info->Text))
    {
        GetDesktop()->SecondDialogBox->SetInfo(Info);
        GetDesktop()->SecondDialogBox->ShowWindow();
        GetDesktop()->SecondDialogBox->SetFocus();      // C3: the second box takes the focus (C2: the main one)
    }
}
NW_HOOK_METHOD(0x100664a0, NConsoleWnd, ShowDialog)

// 0x1005f100 (vslot 342) - a line on the message window, which C3 keeps in the window at +0x3950 (NCGodWnd::ShowMessage,
// a tail call); nothing without that window
void NConsoleWnd::ShowMessageWnd(const TCHAR* Text)
{
    if (GodWnd)
        static_cast<NCGodWnd*>(GodWnd)->ShowMessage(Text);
}
NW_HOOK_METHOD(0x1005f100, NConsoleWnd, ShowMessageWnd)

// 0x1005ae70 - after the announcements, when any: the warning text (GL2GameData +0x77D38) goes to the login screen's
// warning window (+0xCC), which is shown and focused, over the login box and the other windows, hidden
void NConsoleWnd::ShowWarningWnd()
{
    FString& WarningText = nw::Field<FString>(&GL2GameData, 0x77D38);
    if (WarningText.Num() && WarningText.Num() - 1 > 0)
    {
        LoginChild(this, 0xC8)->HideWindow();
        LoginChild(this, 0xD8)->HideWindow();
        LoginChild(this, 0xD0)->HideWindow();
        LoginChild(this, 0xD4)->HideWindow();
        LoginChild(this, 0xCC)->ShowWindow();
        LoginChild(this, 0xCC)->SetFocus();
        static_cast<NCWarningWnd*>(LoginChild(this, 0xCC))->ClearText();
        static_cast<NCWarningWnd*>(LoginChild(this, 0xCC))->SetText(*WarningText);
    }
}
NW_HOOK_METHOD(0x1005ae70, NConsoleWnd, ShowWarningWnd)

// 0x1005f120 - the options window, which C3 keeps in the window at +0x3950 (NCGodWnd::ShowOptionWnd, a tail call)
void NConsoleWnd::ToggleOptionWnd()
{
    if (GodWnd)
        static_cast<NCGodWnd*>(GodWnd)->ShowOptionWnd();
}
NW_HOOK_METHOD(0x1005f120, NConsoleWnd, ToggleOptionWnd)

// 0x1005f020 - the creation was cancelled: its screen gives way to the character selection
void NConsoleWnd::ReturnToCharacterSelect()
{
    PawnCreateWnd->HideWindow();
    LobbyWnd->ShowWindow();
    LobbyWnd->SetFocus();
    MoveCameraByState(L2CS_None, L2CS_Lobby);
    SetState(L2CS_Lobby);
}
NW_HOOK_METHOD(0x1005f020, NConsoleWnd, ReturnToCharacterSelect)

// 0x1005f3e0 - the names of the items, skills and actions again after the data tables were
// reloaded (a language change): inventory, shop, warehouse, trade, shortcut bar, skills,
// actions
void NConsoleWnd::ReloadGameTexts()
{
    GetDesktop()->InventoryWnd->ReloadTexts();
    GetDesktop()->ShopWnd->ReloadTexts();
    GetDesktop()->StoreWnd->ReloadTexts();
    GetDesktop()->TradeWnd->ReloadTexts();
    GetDesktop()->ShortCutWnd->ReloadNames();
    static_cast<NCMagicSkillWnd*>(GetDesktop()->MainWnd->SkillWnd)->ReloadSkillTexts();
    GetDesktop()->MainWnd->ActionWnd->ReloadActions();
}
NW_HOOK_METHOD(0x1005f3e0, NConsoleWnd, ReloadGameTexts)

/*-----------------------------------------------------------------------------
    The rest of the windows' slots.
-----------------------------------------------------------------------------*/

// 0x1005d230 (vslot 150) - no handler: the character window
void NConsoleWnd::ShowCharacterStatus()
{
    GetDesktop()->ToggleCharacter(0);
}
NW_HOOK_METHOD(0x1005d230, NConsoleWnd, ShowCharacterStatus)

// 0x1006af50 (vslot 154) - the desktop's view rectangle: left and top, then its width and height as floats (C3; the
// desktop has no local rectangle any more)
void NConsoleWnd::SetViewRect(INT Left, INT Top, INT Right, INT Bottom)
{
    if (Desktop)
    {
        nw::Field<FLOAT>(Desktop, 0x220) = (FLOAT)(Right - Left);
        nw::Field<INT>(Desktop, 0x218) = Left;
        nw::Field<FLOAT>(Desktop, 0x224) = (FLOAT)(Bottom - Top);
        nw::Field<INT>(Desktop, 0x21C) = Top;
    }
}
NW_HOOK_METHOD(0x1006af50, NConsoleWnd, SetViewRect)

// 0x10060030 (vslot 322) - the pawn viewer mode's windows: 0 / 1 show / hide the pawn viewer,
// 3 / 4 the NPC viewer, 5 / 6 the skill viewer
void NConsoleWnd::PawnViewerFunction(INT Function)
{
    guard(NConsoleWnd::PawnViewerFunction);
    switch (Function)
    {
    case 0:
        if (PawnViewerWnd[0])
            PawnViewerWnd[0]->ShowWindow();
        break;
    case 1:
        if (PawnViewerWnd[0])
            PawnViewerWnd[0]->HideWindow();
        break;
    case 3:
        if (PawnViewerWnd[1])
            PawnViewerWnd[1]->ShowWindow();
        break;
    case 4:
        if (PawnViewerWnd[1])
            PawnViewerWnd[1]->HideWindow();
        break;
    case 5:
        if (PawnViewerWnd[2])
            PawnViewerWnd[2]->ShowWindow();
        break;
    case 6:
        if (PawnViewerWnd[2])
            PawnViewerWnd[2]->HideWindow();
        break;
    }
    unguard;
}
NW_HOOK_METHOD(0x10060030, NConsoleWnd, PawnViewerFunction)

// 0x10060360 (vslot 285) - "L2Debug" turns the debug log on or off, "L2DebugWindow" shows or
// hides the debug window
UBOOL NConsoleWnd::DebugCommand(const TCHAR* Cmd)
{
    if (ParseCommand(&Cmd, TEXT("L2Debug")))
    {
        GL2DebugLog = !GL2DebugLog;
        return 1;
    }
    if (ParseCommand(&Cmd, TEXT("L2DebugWindow")))
    {
        if (DebugWnd)
        {
            if (DebugWnd->IsShowWindow())
                DebugWnd->HideWindow();
            else
                DebugWnd->ShowWindow();
        }
        return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x10060360, NConsoleWnd, DebugCommand)

// 0x1005ec20 (vslot 175) - no handler: a line of the category in the debug window, while the debug log is on. The
// stock buffer has 256 characters and the formatting has no limit (a longer line overruns the stack); here it has
// 1024, so the /GS check does not end the game (same output when it fits)
void VARARGS NConsoleWnd::DebugLogf(INT Category, const TCHAR* Fmt, ...)
{
    TCHAR Line[1024];
    if (!GL2DebugLog || !Fmt)
        return;
    va_list ArgPtr;
    va_start(ArgPtr, Fmt);
    _vswprintf(Line, Fmt, ArgPtr);
    va_end(ArgPtr);
    DebugWnd->AddLog(Category, Line, 0xFFDCDCDC);
}
NW_HOOK_VARARGS_SLOT(0x1005ec20, NConsoleWnd, DebugLogf, 175)

// 0x1005ebd0 (vslot 298) - no handler: a message box. The stock buffer has 256 characters and the formatting has no
// limit; here it has 1024, as in DebugLogf (same box when it fits)
void VARARGS NConsoleWnd::MessageBoxf(const TCHAR* Caption, const TCHAR* Fmt, ...)
{
    TCHAR Text[1024];
    if (!Caption || !Fmt)
        return;
    va_list ArgPtr;
    va_start(ArgPtr, Fmt);
    _vswprintf(Text, Fmt, ArgPtr);
    va_end(ArgPtr);
    MessageBoxW(NULL, Text, Caption, MB_OK);
}
NW_HOOK_VARARGS_SLOT(0x1005ebd0, NConsoleWnd, MessageBoxf, 298)

// 0x1006a940 (vslot 301) - the ids (NItemEntry::ID) of the items on the paperdoll, then on the
// inventory's first page, in an array the engine frees (Free, slot 258); NULL for none
INT* NConsoleWnd::GetInventoryItemIDs(INT& Count)
{
    Count = 0;
    NCEquipItemWnd* Equip = GetDesktop()->InventoryWnd->EquipWnd;
    NCInvenItemWnd* Items = GetDesktop()->InventoryWnd->ItemWnd[0];
    INT NumEquip = Equip ? Equip->Items.Num() : 0;
    INT NumItems = Items ? Items->Items.Num() : 0;
    Count = NumEquip + NumItems;
    if (Count <= 0)
        return NULL;
    INT* IDs = new INT[Count];
    INT i;
    for (i = 0; i < NumEquip; i++)
        IDs[i] = Equip->Items(i)->ID;
    for (; i < Count; i++)
        IDs[i] = Items->Items(i - NumEquip)->ID;
    return IDs;
}
NW_HOOK_METHOD(0x1006a940, NConsoleWnd, GetInventoryItemIDs)
