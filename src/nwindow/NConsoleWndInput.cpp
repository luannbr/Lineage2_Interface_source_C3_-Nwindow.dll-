/*=============================================================================
    NConsoleWndInput.cpp - the console's part of each frame and of the input: the timers and
    the tick, the cursor, the painting of the windows, the Windows messages and the engine's
    mouse events handed to the window manager, the shortcut keys, and the shortcut bar and
    targeting slots the engine calls.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleWndExt.h"
#include "nwindow/NCBroadcastObserverWnd.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCCreditWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCPawnSelectWnd.h"
#include "nwindow/NCPetitionWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCVirtualWndMain.h"
#include "nwindow/NMacroEntry.h"
#include "nwindow/NViewerData.h"

#if NW_FEATURES
// phase 6: the LineageWindow features called from here (docs/notas/Fase6.md)
#include "features/MinimapHotkey.h"
#include "features/WindowFix.h"
#endif

#include <imm.h>

// set while a window or an item is dragged: SetCursor leaves the cursor alone (name ours)
#if NW_HOOK_MODE
#define GCursorLocked NW_GLOBAL(UBOOL, 0x1027B53C)
#else
extern UBOOL GCursorLocked;
#endif

// SetCursor's types (the window manager's cursors; names ours)
enum { CURSOR_Normal, CURSOR_Attack, CURSOR_Talk, CURSOR_PickUp };

// the engine's input (UE2 EInputKey / EInputAction values the console tests)
enum
{
    IK_LeftMouse = 0x01,
    IK_RightMouse = 0x02,
    IK_MouseW = 0xE7,
    IK_MouseWheelUp = 0xEC,
    IK_MouseWheelDown = 0xED,
};
enum { IST_Press = 1, IST_Release = 3 };

// the key the shortcut bar leaves to the console on its release (VK_OEM_3, '`')
enum { VK_BACKQUOTE = 0xC0 };

// the actor under the mouse
static FORCEINLINE AActor* HoveredActor(UEngine* Engine)
{
    return static_cast<UGameEngine*>(Engine)->MouseHitActor;
}

// whether the window with the keyboard focus takes the keys itself (NCWnd slot 33: the edit
// controls); C2: and is shown
static FORCEINLINE UBOOL FocusTakesKeys()
{
    return GFocusEntry && GFocusEntry->Wnd->vfn032() && GFocusEntry->Wnd->IsShowWindow();
}

/*-----------------------------------------------------------------------------
    The cursor.
-----------------------------------------------------------------------------*/

// 0x1005f090 - the window manager's cursor, unless a drag locked it
void NConsoleWnd::SetCursor(INT Type, UTexture* Texture)
{
    if (!GCursorLocked)
        WndMain->SetCursorType(Type, Texture);
}
NW_HOOK_METHOD(0x1005f090, NConsoleWnd, SetCursor)

// 0x1005f0c0
void NConsoleWnd::LockCursor()
{
    GCursorLocked = 1;
}
NW_HOOK_METHOD(0x1005f0c0, NConsoleWnd, LockCursor)

// 0x1005f0d0
void NConsoleWnd::UnlockCursor()
{
    GCursorLocked = 0;
}
NW_HOOK_METHOD(0x1005f0d0, NConsoleWnd, UnlockCursor)

// 0x1005f0e0 (vslot 155) - Ctrl pressed (the engine's key). C3: no attack cursor any more, a plain store (C2 also
// locked the cursor)
void NConsoleWnd::PressCtrl()
{
    bCtrlDown = 1;
}
NW_HOOK_METHOD(0x1005f0e0, NConsoleWnd, PressCtrl)

// 0x1005f0f0 (vslot 156)
void NConsoleWnd::ReleaseCtrl()
{
    bCtrlDown = 0;
}
NW_HOOK_METHOD(0x1005f0f0, NConsoleWnd, ReleaseCtrl)

// C3: IsCtrlDown is no function any more (no virtual, no stock body: the callers read the flag): kept as a plain
// member of ours (C2: bCtrlDown || bCtrlDownAlt; the second flag is gone)
UBOOL NConsoleWnd::IsCtrlDown()
{
    return bCtrlDown;
}

// 0x10066410 (vslot 297; C3 only, name ours) - whether a click is an attack: never in language 0 while the loading
// picture is 10002, else when the engine's input has Ctrl down or the console's flag (PressCtrl) says so
UBOOL NConsoleWnd::IsAttackKeyDown()
{
    if (GLanguageType == 0 && LoadingScreenID == 0x2712)
        return 0;
    if (Engine && Engine->Client && Engine->Client->Viewports(0) && Engine->Client->Viewports(0)->Input)
    {
        if (Engine->Client->Viewports(0)->Input->KeyDown(VK_CONTROL))
            return 1;
    }
    return bCtrlDown != 0;
}
NW_HOOK_METHOD(0x10066410, NConsoleWnd, IsAttackKeyDown)

// 0x10073bc0 - once a frame (ConsolePostRender): nothing while the mouse is out of the viewport (UViewport +0x3C /
// +0x40 are its mouse position, floats). With color cursors, the cursor for what is under the mouse - the target: a
// member of a clan at war with the player's (the network handler's IsEnemyPledge) gets attack, or the normal cursor
// when either side has surrendered (IsSurrender), else attack / talk as the actor allows or talk for a private store
// (selling, buying or the recipe store); an item on the ground: its name drawn above it and the pick up cursor.
// C3: the normal cursor while a window took the mouse move (WndMain's bMouseOnWindow), the attack key (slot 297)
// turns an unknown or unselected hover into the attack cursor, and a hovered user without a color-cursor option
// leaves the cursor alone
void NConsoleWnd::UpdateCursor()
{
    UViewport* Viewport = Engine->Client->Viewports(0);
    FLOAT MouseX = nw::Field<FLOAT>(Viewport, 0x3C);
    FLOAT MouseY = nw::Field<FLOAT>(Viewport, 0x40);
    if (MouseX < 0.0f || (FLOAT)Viewport->SizeX < MouseX)
        return;
    if (MouseY < 0.0f || (FLOAT)nw::Field<INT>(Viewport, 0x8C) < MouseY)
        return;
    AActor* Hovered = GetTargetedActor(0);
    User* Target = GetSelectedActor();
    INT Cursor = CURSOR_Normal;
    if (!WndMain->bMouseOnWindow)
    {
        if (Hovered)
        {
            User* HoverUser = FindUserByID(Hovered->CreatureID);
            if (HoverUser)
            {
                if (!GL2UseColorCursor)
                    return;
                if (HoverUser == Target)
                {
                    User* Me = GetMyUser();
                    INT MyPledge = 0;
                    INT MyAlly = 0;
                    if (Me)
                    {
                        MyPledge = Me->PledgeID;
                        MyAlly = Me->Unknown1E8;
                    }
                    INT HisAlly = HoverUser->Unknown1E8;
                    INT HisPledge = HoverUser->PledgeID;
                    if (Engine->NetHandler->IsEnemyPledge(MyPledge, HisPledge))
                    {
                        if (!Engine->NetHandler->IsSurrender(MyPledge, MyAlly) &&
                            !Engine->NetHandler->IsSurrender(HisPledge, HisAlly))
                            Cursor = CURSOR_Attack;
                    }
                    else if (::nw::VirtualCall<UBOOL>(this, 297) || Hovered->CanBeAttacked())
                        Cursor = CURSOR_Attack;
                    else if (Hovered->CanBeTold())
                        Cursor = CURSOR_Talk;
                    else
                    {
                        SWORD StoreType = HoverUser->PrivateStoreType;
                        if (StoreType == 1 || StoreType == 3 || StoreType == 5)
                            Cursor = CURSOR_Talk;
                    }
                }
                else if (::nw::VirtualCall<UBOOL>(this, 297))
                    Cursor = CURSOR_Attack;
            }
        }
        else
        {
            AActor* Pickup = GetPickupUnderMouse();
            if (Pickup)
            {
                Item* Info = FindItemByID(Pickup->CreatureID);
                if (Info && Pickup->Mesh)
                {
                    FBox Box = Pickup->Mesh->GetRenderBoundingBox(Pickup);
                    FVector Location = Pickup->Location;
                    Location.Z = Box.Max.Z * 0.5f + Location.Z;
                    const TCHAR* Name = GetItemName(Info->ItemID);
                    if (Name && *Name)
                    {
                        TCHAR Text[256];
                        if (Info->bStackable)
                            appSprintf(Text, TEXT("%s(%d)"), Name, Info->Count);
                        else
                            appSprintf(Text, TEXT("%s"), Name);
                        UCanvas* Canvas = Engine->Client->Viewports(0)->Canvas;
                        Canvas->Draw3DCoordText(Location, 0xFFDCDCDC, Text, NULL, 2, 0, L2FONT_Normal, 0);
                    }
                }
                Cursor = CURSOR_PickUp;
            }
            else if (::nw::VirtualCall<UBOOL>(this, 297))
                Cursor = CURSOR_Attack;
        }
    }
    if (GL2UseColorCursor)
        SetCursor(Cursor, NULL);
}
NW_HOOK_METHOD(0x10073bc0, NConsoleWnd, UpdateCursor)

/*-----------------------------------------------------------------------------
    The timers and the tick.
-----------------------------------------------------------------------------*/

// 0x1006be40 - NCObject::SetTimer: the owner gets WindowProc(WM_TIMER, ID) each Elapse ms
INT NConsoleWnd::NSetTimer(UINT ID, UINT Elapse, NCObject* Owner)
{
    guard(NConsoleWnd::NSetTimer);
    NTimer Timer;
    Timer.ID = ID;
    Timer.Elapse = Elapse;
    Timer.Elapsed = 0;
    Timer.bKilled = 0;
    Timer.Owner = Owner;
    Timers.AddItem(Timer);
    return ID;
    unguard;
}
NW_HOOK_METHOD(0x1006be40, NConsoleWnd, NSetTimer)

// 0x10062bf0 - NCObject::KillTimer: the owner's first live timer of that id is marked, the
// next Tick drops it
UBOOL NConsoleWnd::NKillTimer(UINT ID, NCObject* Owner)
{
    for (INT i = 0; i < Timers.Num(); i++)
    {
        NTimer& Timer = Timers(i);
        if (Timer.ID == ID && Timer.Owner == Owner && !Timer.bKilled)
        {
            Timer.bKilled = 1;
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x10062bf0, NConsoleWnd, NKillTimer)

// 0x100738a0 (guard name IsSelectedTarget; the C2 name was UpdateTargetWindow) - once a tick, unless the flag at
// +0x3930 is set: the target status window (+0x100) shows (and takes the focus) while the player has a target, a
// door or other static one, and hides when it has none
void NConsoleWnd::UpdateTargetWindow()
{
    guard(NConsoleWnd::IsSelectedTarget);
    if (!nw::Field<INT>(this, 0x3930))
    {
        User* Target = GetSelectedActor();
        AActor* StaticActor = GetSelectedStaticActor();
        FServerStaticObject* StaticInfo = NULL;
        if (StaticActor)
            StaticInfo = FindStaticObjectInfo(nw::Field<INT>(StaticActor, 0x54));
        if (Desktop)
        {
            NCWnd* TargetWnd = static_cast<NCConsole*>(Desktop)->TargetStatusWnd;
            if (!Target && !StaticInfo)
            {
                if (TargetWnd->IsShowWindow())
                    TargetWnd->HideWindow();
            }
            else if (!TargetWnd->IsShowWindow())
            {
                TargetWnd->ShowWindow();
                TargetWnd->SetFocus();
            }
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x100738a0, NConsoleWnd, UpdateTargetWindow)

// the seconds since the game guard's last check (name ours): the stock Tick adds the frame time to it
// and, when it is past 10 seconds, would clear it and ask nProtect's NPGameMon (CheckNPGameMon,
// 0x100E0110); the packager's binary patch turned that conditional jump (0x1006BDD9, 75 4D) into an
// unconditional one, so the check never runs and the value only grows. Kept as it runs.
#if NW_HOOK_MODE
#define GGameGuardTimer NW_GLOBAL(FLOAT, 0x1027BA78)
#else
extern FLOAT GGameGuardTimer;
#endif

// The macros of the desktop's macro window (NewWndF8, NCMacroListWnd) are run from the tick: GetMacroCount
// (0x10056A20) is its macro count (0 without the window), GetMacro (0x10060CE0) the macro of an index (NULL out
// of range), whose bRunning (+0x9D4) is non-zero while one is loaded, and NMacroEntry::Tick (0x1008EBE0) runs it
// for the frame time.
static FORCEINLINE INT MacroCount(NConsoleWnd* Console)
{
    return Console->GetMacroCount();
}
static FORCEINLINE NMacroEntry* MacroOf(NConsoleWnd* Console, INT Index)
{
    return Console->GetMacro(Index);
}

// 0x10075a10 (vslot 25) - the engine's tick, then the timers: the killed ones are dropped (the one after each is
// skipped this tick), the others count the time and send WM_TIMER to their owner once their interval has passed,
// keeping the time over; then (C3) the object at +0x39E4, the shortcut bar's cool times, the credit window and the
// macros, the target window, the chat's text capture, the game guard's timer and the zone title window
void NConsoleWnd::Tick(FLOAT InDeltaTime)
{
    guard(NConsoleWnd::Tick);
    UL2ConsoleWnd::Tick(InDeltaTime);
    DeltaTime = InDeltaTime;
    if (nw::Field<void*>(this, 0x39E4))
        nw::Field<NControlExt*>(this, 0x39E4)->EndMouseDrag();
    for (INT i = 0; i < Timers.Num(); i++)
    {
        NTimer& Timer = Timers(i);
        if (Timer.bKilled)
        {
            Timers.Remove(i, 1);
            continue;
        }
        DOUBLE Elapsed = (DOUBLE)InDeltaTime + Timer.Elapsed;
        Timer.Elapsed = (FLOAT)Elapsed;
        DOUBLE Interval = (DOUBLE)Timer.Elapse * 0.001f;
        if (Elapsed > Interval)
        {
            Timer.Elapsed = (FLOAT)(Elapsed - Interval);
            if (Timer.Owner)   // a window's (NCObject::SetTimer)
                static_cast<NCWnd*>(Timer.Owner)->WindowProc(WM_TIMER, Timer.ID, 0);
        }
    }
    if (Desktop && GetDesktop()->ShortCutWnd)
        GetDesktop()->ShortCutWnd->Tick(InDeltaTime);
    if (CreditWnd && CreditWnd->IsShowWindow())
        static_cast<NCCreditWnd*>(CreditWnd)->Tick(InDeltaTime);   // NCCreditWnd's scroll
    for (INT i = 0; i < MacroCount(this); i++)
    {
        if (MacroOf(this, i) && MacroOf(this, i)->bRunning)
            MacroOf(this, i)->Tick(InDeltaTime);
    }
    UpdateTargetWindow();
    if ((GL2TextCapture == 1 || GL2TextCapture == 3) && Desktop && GetDesktop()->ChatWnd)
        GetDesktop()->ChatWnd->UpdateTextCapture();
    GGameGuardTimer += InDeltaTime;
    if (Desktop && GetDesktop()->ZoneTitleWnd)
        static_cast<NCZoneTitleWndTick*>(GetDesktop()->ZoneTitleWnd)->Tick(InDeltaTime);   // NCZoneTitleWnd::Tick (0x1010B4A0)
    unguard;
}
NW_HOOK_METHOD(0x10075a10, NConsoleWnd, Tick)

/*-----------------------------------------------------------------------------
    Painting and the messages.
-----------------------------------------------------------------------------*/

// 0x10075c80 (vslot 281) - once a frame after the scene: the cursor (UpdateCursor), then
// every window painted on the canvas; what the paint returned, else the canvas pointer
INT NConsoleWnd::ConsolePostRender(UCanvas* Canvas)
{
    guard(NConsoleWnd::ConsolePostRender);
    if (Desktop)
        UpdateCursor();
    if (!WndMain || !bInitialized || bSuspended)
        return (INT)(INT_PTR)Canvas;
    WndMain->SetGDevice(Canvas);
    INT Result = WndMain->DispatchWndMsg(WM_PAINT, 0, 0);
    WndMain->ReleaseGDevice();
    return Result;
    unguard;
}
NW_HOOK_METHOD(0x10075c80, NConsoleWnd, ConsolePostRender)

// 0x10078b60 (vslot 238) - a message of the game window
INT NConsoleWnd::DispatchWndMsg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    guard(NConsoleWnd::DispatchWndMsg);
    return DispatchWndMsgX(hWnd, Msg, wParam, lParam);
    unguard;
}
NW_HOOK_METHOD(0x10078b60, NConsoleWnd, DispatchWndMsg)

// 0x10075640 - a message for the windows (hWnd is not read): none before Initialize; some keys never go to the
// windows (unless a modal window is up), the arrows only to a focused control that takes keys. In the game (or the
// test console) the shortcut keys go first (not while a modal window is up). Closing messages finalize the console.
// On the character selection a click that no window took picks the character under the mouse, or plays its pose
// again when it is the chosen one; the Enter key starts the game with the chosen one (C3). C3 against C2: the wait
// for a character-stage request no longer swallows the messages, Shift is dropped only when the focused window
// takes no text, the chat line's right click and button up checks are gone
INT NConsoleWnd::DispatchWndMsgX(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    guard(NConsoleWnd::DispatchWndMsgX);
    if (!bInitialized || bSuspended)
        return 0;
    if (!WndMain->NumFocusWindows() && (Msg == WM_KEYDOWN || Msg == WM_KEYUP))
    {
        if (wParam == VK_PRIOR || wParam == VK_NEXT || wParam == VK_INSERT || wParam == VK_SNAPSHOT)
            return 0;
        if (wParam == VK_SHIFT &&
            !(GFocusEntry && GFocusEntry->Wnd && GFocusEntry->Wnd->vfn032()))
            return 0;
    }
    if (!WndMain->NumFocusWindows() && (Msg == WM_KEYDOWN || Msg == WM_KEYUP))
    {
        if ((wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN ||
             (wParam >= 0xC8 && wParam <= 0xD7)) &&
            !FocusTakesKeys())
            return 0;
    }
    if (GIsOnlyL2Console || (State == L2CS_InGame && !WndMain->NumFocusWindows()))
    {
        if (HookShortCutKey(Msg, wParam, lParam))
            return 1;
    }
    switch (Msg)
    {
    case WM_DESTROY:
    case WM_CLOSE:
    case WM_QUIT:
        Finalize();
        return 0;
    case WM_PAINT:
        return 0;
    }
    INT Result = WndMain->DispatchWndMsg(Msg, wParam, lParam);
    if (State != L2CS_Lobby)
        return Result;
    if (Result)
        return Result;
    if (Msg == WM_LBUTTONDOWN)
    {
        if (!Engine || !HoveredActor(Engine))
            return 1;
        APawn* Pawn = CastToPawn(HoveredActor(Engine));
        if (!Pawn)
            return 1;
        nw::Field<NCMessageWnd*>(GodWnd, 0xD0)->ClearText();   // C3: the message window is the NCGodWnd's
        LobbyWnd->DialogBox->HideWindow();
        // a character of the selection: chosen, or its third social action when it was already
        if (Pawn->SelectFlags & 1)
        {
            INT Chosen = -1;
            if (LobbyWnd && LobbyWnd->PawnSelectWnd)
            {
                Chosen = LobbyWnd->PawnSelectWnd->GetSelected();
                LobbyWnd->PawnSelectWnd->SelectCharacter(Pawn->SelectIndex);
            }
            if (Chosen != Pawn->SelectIndex)
                SelectedCharacterNum(Pawn->SelectIndex);
            else
                Pawn->PlayAnim(0, Pawn->SocialAnimName[3], Pawn->SpeedRate, 0.3f, 0, 1);
        }
        return 1;
    }
    if (Msg == WM_KEYDOWN)
    {
        if (wParam != VK_RETURN)
            return 0;
        if (!LobbyWnd)
            return 1;
        LobbyWnd->OnStart();
        return 1;
    }
    if (Msg != WM_RBUTTONDOWN)
        return 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10075640, NConsoleWnd, DispatchWndMsgX)

// 0x10075d50 (vslot 240) - the engine's mouse: the buttons as the Windows messages at the
// cursor, the wheel as WM_MOUSEWHEEL with the last delta (IK_MouseW). A wheel step no
// window takes is sent a second time; one a window took eats the release that follows.
UBOOL NConsoleWnd::MasterConsoleEventProcess(INT Key, INT Action, FLOAT Delta)
{
    guard(NConsoleWnd::MasterConsoleEventProcess);
    UINT Msg;
    WPARAM wParam = 0;
    POINT Mouse;
    LPARAM lParam;
    switch (Key)
    {
    case IK_LeftMouse:
        Msg = (Action == IST_Press) ? WM_LBUTTONDOWN : WM_LBUTTONUP;
#if NW_FEATURES
        // phase 6: WindowFix (NWindow_C1 src/inject/WindowFix.cpp): the click at the cursor
        // in the game window's client space
        nwf::WindowFix_GetCursorPos(&Mouse);
#else
        GetCursorPos(&Mouse);
#endif
        lParam = (Mouse.y << 16) | Mouse.x;
        break;
    case IK_RightMouse:
        Msg = (Action == IST_Press) ? WM_RBUTTONDOWN : WM_RBUTTONUP;
#if NW_FEATURES
        // phase 6: WindowFix (NWindow_C1 src/inject/WindowFix.cpp): the same
        nwf::WindowFix_GetCursorPos(&Mouse);
#else
        GetCursorPos(&Mouse);
#endif
        lParam = (Mouse.y << 16) | Mouse.x;
        break;
    case IK_MouseW:
        MouseWheelDelta = (INT)Delta;
        return 0;
    case IK_MouseWheelUp:
    case IK_MouseWheelDown:
        if (Action == IST_Press)
        {
            Msg = WM_MOUSEWHEEL;
            wParam = MouseWheelDelta;
#if NW_FEATURES
            // phase 6: WindowFix (NWindow_C1 src/inject/WindowFix.cpp): the same
            nwf::WindowFix_GetCursorPos(&Mouse);
#else
            GetCursorPos(&Mouse);
#endif
            lParam = (Mouse.y << 16) | Mouse.x;
            if (DispatchWndMsgX(GViewportHWnd, Msg, wParam, lParam))
            {
                bWheelHandled = 1;
                return 1;
            }
            break;
        }
        if (Action == IST_Release && bWheelHandled)
        {
            bWheelHandled = 0;
            return 1;
        }
        return 0;
    default:
        return 0;
    }
    return DispatchWndMsgX(GViewportHWnd, Msg, wParam, lParam);
    unguard;
}
NW_HOOK_METHOD(0x10075d50, NConsoleWnd, MasterConsoleEventProcess)

/*-----------------------------------------------------------------------------
    The shortcut keys.
-----------------------------------------------------------------------------*/

// a key the windows left puts the focus in the chat line, but over an edit control, or for
// Tab, Ctrl, Alt or Esc
static FORCEINLINE void FocusChatLine(NConsoleWnd* Console, WPARAM Key)
{
    if (FocusTakesKeys())
        return;
    NCEditBox* ChatLine = Console->GetDesktop()->ChatWnd->EditBox;
    if (ChatLine->IsFocused())
        return;
    if (Key == VK_TAB || Key == VK_CONTROL || Key == VK_ESCAPE || Key == VK_MENU)   // C3: and Alt
        return;
    ChatLine->SetFocus();
}

// whether the player has GM rights (the GM windows' keys)
static FORCEINLINE UBOOL IsBuilder(NConsoleWnd* Console)
{
    return Console->GetMyUser() && Console->GetMyUser()->BuilderLevel > 0 &&
           Console->GetMyUser()->BuilderLevel < 7;
}

// A key or click stops the running macros (the stock repeats this loop in every path; 0x1009AFF0
// clears the macro's state)
static FORCEINLINE void StopMacros(NConsoleWnd* Console)
{
    for (INT i = 0; i < MacroCount(Console); i++)
    {
        if (MacroOf(Console, i) && MacroOf(Console, i)->bRunning)
            MacroOf(Console, i)->Reset();   // 0x1009AFF0
    }
}

// read only by HookShortCutKey (name ours, nothing in NWindow writes it): while it is set the Y key toggles the chat
#if NW_HOOK_MODE
#define GShortCutChatKey NW_GLOBAL(INT, 0x10205580)
#else
extern INT GShortCutChatKey;
#endif

// 0x1006bf00 - the keys before the windows get them (1: taken). F10 comes as a system key: taken as a plain one
// when pressed (HIWORD(lParam) 0x44 / 0x4044) or released (0xC044). The shortcut bar's keys first
// (OnShortCutKeyDown / Up); Esc drops the target (not while a bit of the player controller's flags +0xA4C is up),
// Enter puts the focus in the chat line when the chat needs Enter, Tab the inventory in the game unless the focused
// window has a Tab chain. Letters open windows while the chat needs Enter and no window takes keys; otherwise any key
// or character (below 0x80) puts the focus in the chat line. Alt + a key opens windows, Alt + F1 .. F10 (and 1 .. 0
// when the chat needs Enter) turns the shortcut bar's page. The IME's start switches the IME to alphanumeric when
// the chat needs Enter, else focuses the chat line. A key (not a handled letter), the left button and the IME's
// messages stop the running macros. C3: UIMode 1 (the observer's) has its own letters (the broadcast observer
// window's) and only Alt + J. The functions called by address are the ones this port has no class for.
UBOOL NConsoleWnd::HookShortCutKey(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    guard(NConsoleWnd::HookShortCutKey);
    UBOOL bHandled = 0;
    L2ParamStack Params(10);
    if ((Msg == WM_SYSKEYDOWN || Msg == WM_SYSKEYUP) && wParam == VK_F10)
    {
        WORD Flags = HIWORD(lParam);
        if (Flags == 0x44 || Flags == 0x4044)
            Msg = WM_KEYDOWN;
        else if (Flags == 0xC044)
            Msg = WM_KEYUP;
    }
    switch (Msg)
    {
    case WM_KEYUP:
        if (GetDesktop()->ShortCutWnd->OnShortCutKeyUp(wParam, lParam))
            return 1;
        if (wParam == VK_BACKQUOTE)
            bHandled = 1;
        return bHandled;

    case WM_KEYDOWN:
    {
        UBOOL bMacroKeys = IsAnyMacroRunning();
        if (GetDesktop()->ShortCutWnd->OnShortCutKeyDown(wParam, lParam))
            return 1;
        if (wParam == VK_ESCAPE)
        {
            if (nw::Field<BYTE>(Engine->Client->Viewports(0)->Actor, 0xA4C) & 1)
                return bHandled;
            if (HasSelectedTarget())
            {
                L2ParamStack CancelParams(10);
                RequestTargetCancel(CancelParams);
            }
            bHandled = 1;
        }
        else if (wParam == VK_RETURN)
        {
            if (GIsEnterChatting && !GetDesktop()->ChatWnd->EditBox->IsFocused() && !bNoChatOnEnter &&
                !FocusTakesKeys())
            {
                GetDesktop()->ChatWnd->EditBox->SetFocus();
                bHandled = 1;
            }
        }
        else if (wParam == VK_TAB && State == L2CS_InGame)
        {
            // not while the focused window has a Tab chain (its edit lines): Tab moves the focus there
            if (GFocusEntry && GFocusEntry->Wnd && (GFocusEntry->Wnd->NextTab || GFocusEntry->Wnd->PrevTab))
                return 0;
            GetDesktop()->ToggleInventory();
            bMacroKeys = 0;
            bHandled = 1;
        }
        if (UIMode == 0)
        {
            if (GIsEnterChatting && !FocusTakesKeys())
            {
                switch (wParam)
                {
                case 'B': GetDesktop()->ToggleBoard(); bHandled = 1; break;
                case 'C': GetDesktop()->ToggleCharacter(2); bHandled = 1; break;
                case 'F': ToggleBuffIcons(); bHandled = 1; break;   // the party, pet and summon windows' toggles
                case 'I': GetDesktop()->ToggleCharacter(-1); bHandled = 1; break;
                case 'K': GetDesktop()->ToggleCharacter(1); bHandled = 1; break;
                case 'M': RequestDeleteMacro(Params); bHandled = 1; break;
                case 'N': GetDesktop()->ToggleCharacter(3); bHandled = 1; break;
                case 'P': ((NCVideoOptionWndExt*)nw::Field<NCWnd*>(nw::Field<NCWnd*>(GodWnd, 0xD4), 0xD0))->ToggleLowerDetail(); bHandled = 1; break;
                case 'R': GetDesktop()->ToggleMacroList(); bHandled = 1; break;
                case 'T': GetDesktop()->ToggleCharacter(0); bHandled = 1; break;
                case 'U': GetDesktop()->ToggleCharacter(4); bHandled = 1; break;
                case 'V': GetDesktop()->ToggleInventory(); bHandled = 1; break;
                case 'X': GetDesktop()->ToggleSystemMenu(); bHandled = 1; break;
                case 'Y':
                    if (GShortCutChatKey)
                        GetDesktop()->ToggleChat();
                    bHandled = 1;
                    break;
                default:
                    if (bMacroKeys)
                        StopMacros(this);
                    break;
                }
            }
            else if (bMacroKeys)
                StopMacros(this);
        }
        else if (UIMode == 1)
        {
            NCBroadcastObserverWnd* Observer = static_cast<NCBroadcastObserverWnd*>(GetDesktop()->BroadcastObserverWnd);
            switch (wParam)
            {
            case 'G': Observer->SwitchMsgMode(); break;
            case 'I': Observer->InitPosition(); break;
            case 'L': Observer->LockPosition(); break;
            case 'M': ::nw::VirtualCall<void>(Engine->Client->Viewports(0)->Canvas, 76); break;   // UCanvas slot 76
            case 'N': ((NCBroadcastObserverWndExt2*)Observer)->ToggleFlag(); break;   // a flag toggled (+0xF0)
            case 'P': Observer->ShowPartyWnd(); break;
            }
        }
        if (GIsEnterChatting)
            return bHandled;
        if (wParam <= 0x7F)
            FocusChatLine(this, wParam);
        return bHandled;
    }

    case WM_CHAR:
        if (!GIsEnterChatting && wParam <= 0x7F)
            FocusChatLine(this, wParam);
        return bHandled;

    case WM_SYSKEYDOWN:
        if (UIMode != 0)
        {
            if (UIMode == 1 && wParam == 'J')
                ((NCConsoleExt*)GetDesktop())->ToggleChatWnd();   // the chat window shown (and focused) or hidden
        }
        else
        {
            switch (wParam)
            {
            case VK_PRIOR: GetDesktop()->ChatWnd->CycleTab(0); break;
            case VK_NEXT: GetDesktop()->ChatWnd->CycleTab(1); break;
            case 'B': GetDesktop()->ToggleBoard(); break;
            case 'C': GetDesktop()->MainWnd->ShowTab(2); break;
            case 'F': ToggleBuffIcons(); break;
            case 'G':
                if (IsBuilder(this))
                    GetDesktop()->ToggleGMWnd();
                break;
            case 'H': GetDesktop()->ToggleInterface(); break;
            case 'I': GetDesktop()->ToggleCharacter(-1); break;
            case 'J': ((NCConsoleExt*)GetDesktop())->ToggleChatWnd(); break;
            case 'K': GetDesktop()->MainWnd->ShowTab(1); break;
            case 'M': GConsole->RequestDeleteMacro(Params); break;
            case 'N': GetDesktop()->MainWnd->ShowTab(3); break;
            case 'O':
                if (IsBuilder(this))
                {
                    GetDesktop()->GMPetitionWnd->ShowWindow();
                    GetDesktop()->GMPetitionWnd->SetFocus();
                }
                break;
            case 'P': ((NCVideoOptionWndExt*)nw::Field<NCWnd*>(nw::Field<NCWnd*>(GodWnd, 0xD4), 0xD0))->ToggleLowerDetail(); break;
            case 'R': GetDesktop()->ToggleMacroList(); break;
            case 'T': GetDesktop()->MainWnd->ShowTab(0); break;
            case 'U': GetDesktop()->MainWnd->ShowTab(4); break;
            case 'V': GetDesktop()->ToggleInventory(); break;
            case 'W': GetDesktop()->CloseWindows(); break;
            case 'X': GetDesktop()->ToggleSystemMenu(); break;
            case 'Y':
                if (GShortCutChatKey)
                    GetDesktop()->ToggleChat();
                break;
            case VK_F1: case VK_F2: case VK_F3: case VK_F4: case VK_F5:
            case VK_F6: case VK_F7: case VK_F8: case VK_F9: case VK_F10:
                GetDesktop()->ShortCutWnd->SetPage(wParam - VK_F1);
                break;
            }
        }
        if (GIsEnterChatting && wParam >= '0' && wParam <= '9')
        {
            INT Page = wParam - '1';
            if (Page == -1)
                Page = 9;
            GetDesktop()->ShortCutWnd->SetPage(Page);
        }
        return 1;

    case WM_LBUTTONDOWN:
        StopMacros(this);
        return bHandled;

    case WM_IME_STARTCOMPOSITION:
    case WM_IME_NOTIFY:
        StopMacros(this);
        if (GIsEnterChatting && GLanguageType == 0)
        {
            if (!FocusTakesKeys())
                ImmSetConversionStatus(ImmGetContext(GViewportHWnd), 0, 0);
        }
        else if (!FocusTakesKeys() && !GetDesktop()->ChatWnd->EditBox->IsFocused())
            GetDesktop()->ChatWnd->EditBox->SetFocus();
        return bHandled;
    }
    return bHandled;
    unguard;
}
NW_HOOK_METHOD(0x1006bf00, NConsoleWnd, HookShortCutKey)

/*-----------------------------------------------------------------------------
    Slots of the shortcut bar and the targeting the engine calls.
-----------------------------------------------------------------------------*/

// 0x1005a6a0 (vslot 27)
UBOOL NConsoleWnd::IsOverWindow(INT X, INT Y)
{
    if (!WndMain)
        return 0;
    return WndMain->IsOverWindow(X, Y);
}
NW_HOOK_METHOD(0x1005a6a0, NConsoleWnd, IsOverWindow)

// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)
// 0x10078e70 (vslot 151) - the slot of the current page
void NConsoleWnd::UseShortCutSlot(INT Index)
{
    UseShortCut(GetDesktop()->ShortCutWnd->PageSlot(Index));
}
NW_HOOK_METHOD(0x10078e70, NConsoleWnd, UseShortCutSlot)

// 0x1005d240 (vslot 152) - 0: the page before, 1: the next one
void NConsoleWnd::ChangeShortCutPage(INT bNext)
{
    if (bNext == 0)
        GetDesktop()->ShortCutWnd->PrevPage();
    else if (bNext == 1)
        GetDesktop()->ShortCutWnd->NextPage();
}
NW_HOOK_METHOD(0x1005d240, NConsoleWnd, ChangeShortCutPage)

// The C1 virtuals 269 and 270 (the shortcut bar's pad halves): not in the C2 or C3 vtable any more (the C3 ones are
// SetPadHoldL / SetPadHoldR of the bar and the console's C2 slots 332 / 333); inert bodies
void NConsoleWnd::SetPadShift1(BYTE bShift)
{
}

void NConsoleWnd::SetPadShift2(BYTE bShift)
{
}

// 0x1006adb0 (vslot 150) - no handler in NWindow: among the pawns the level lists around the
// player (ULevel::GetPawnListExtent on GMem, given P4 .. P7 and the radius), the user of the
// one after CreatureID (bNext 1) or before it (0; before the first: the last); the first
// one's when CreatureID is not above 0 or not in the list, NULL for no list (the same body as C1's)
User* NConsoleWnd::FindNextPawnInArea(FLOAT Radius, INT CreatureID, INT bNext, INT P4, INT P5,
                                      INT P6, INT P7)
{
    FMemMark Mark(GMem);
    User* Result = NULL;
    ULevel* Level = NULL;
    UViewport* Viewport = GConsole->Engine->Client->Viewports(0);
    if (Viewport)
        Level = Viewport->Actor->XLevel;
    if (Viewport->Actor->Pawn && Level)
    {
        FCheckResult* List = Level->GetPawnListExtent(GMem, P4, P5, P6, P7, (INT)Radius);
        if (List)
        {
            Result = FindUserByID(List->Actor->CreatureID);
            if (CreatureID > 0)
            {
                INT Count = 0;
                for (FCheckResult* Hit = List; Hit; Hit = Hit->Next)
                    Count++;
                FCheckResult* Last = &List[Count - 1];   // the list is contiguous on GMem
                FCheckResult* Prev = NULL;
                FCheckResult* Hit = List;
                for (INT i = 0; i < Count; i++)
                {
                    if (Hit->Actor->CreatureID == CreatureID)
                    {
                        if (bNext == 1)
                        {
                            if (Hit->Next)
                            {
                                Result = FindUserByID(Hit->Next->Actor->CreatureID);
                                break;
                            }
                        }
                        else if (bNext == 0)
                        {
                            if (Prev)
                            {
                                Result = FindUserByID(Prev->Actor->CreatureID);
                                break;
                            }
                            if (Last)
                            {
                                Result = FindUserByID(Last->Actor->CreatureID);
                                break;
                            }
                        }
                    }
                    Prev = Hit;
                    if (Hit)
                        Hit = Hit->Next;
                }
            }
        }
    }
    Mark.Pop();
    return Result;
}
NW_HOOK_METHOD(0x1006adb0, NConsoleWnd, FindNextPawnInArea)
