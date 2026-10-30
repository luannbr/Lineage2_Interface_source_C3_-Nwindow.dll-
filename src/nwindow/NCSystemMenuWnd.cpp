/*=============================================================================
    NCSystemMenuWnd.cpp - the system menu. Stock: 0x10096da0 .. 0x10096fe5, 0x10097790 ..
    0x100980b6 (OnDestroy 0x10097dd0 is its own), plus ShowWindow 0x1008c9c0 (shared by six
    windows) and OnOptions 0x10082c50 (NCLoginWnd's slot 68 too).
=============================================================================*/
#include "nwindow/NCSystemMenuWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCPetitionBeginWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/MsnWnd.h"

// 0x100ab860 - Buttons[SYSMENU_Unused], ButtonB and ButtonC are left as allocated
NCSystemMenuWnd::NCSystemMenuWnd()
{
    BackTex = NULL;
    Buttons[SYSMENU_Help] = NULL;
    Buttons[SYSMENU_Options] = NULL;
    Buttons[SYSMENU_Restart] = NULL;
    Buttons[SYSMENU_Exit] = NULL;
    ButtonA = NULL;
}
NW_HOOK_CTOR(0x100ab860, NCSystemMenuWnd)


// 0x100a9c00 (vslot 25) - C3: a seventh button (NID_SYSMENU_BUTTON_C: slot 74, NewVirtual74 here, whose body is
// OnBoard), and the dialog result is the last test
INT NCSystemMenuWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_SYSMENU_HELP)
            return OnHelp() != 0;
        if (wParam == NID_SYSMENU_OPTIONS)
            return OnOptions() != 0;
        if (wParam == NID_SYSMENU_RESTART)
            return OnRestart() != 0;
        if (wParam == NID_SYSMENU_EXIT)
            return OnExit() != 0;
        if (wParam == NID_SYSMENU_BUTTON_A)
            return OnHelpWindow() != 0;
        if (wParam == NID_SYSMENU_BUTTON_B)
            return OnHelpPage() != 0;
        if (wParam == NID_SYSMENU_BUTTON_C)
        {
            NewVirtual74();
            return 1;
        }
    }
    else if (Msg == (UINT)NWM_DIALOG_RESULT)
        return OnDialogResult(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100a9c00, NCSystemMenuWnd, WindowProc)

// 0x100aae90 (vslot 37) - C3: the icons of the CH3 skin (36 x 36 at x 13, one per row of text, no label on them), the
// title bar of 20 px from the frame constructor 0x10010450 (Decorated 1) and the new seventh button first. The order
// of creation: C, A, Help, B, Options, Restart, Exit
// (one button of the menu: repeated inline in the stock OnCreate)
static FORCEINLINE NCButton* CreateMenuButton(NCWnd* Parent, const TCHAR* NormalTexName, const TCHAR* PushedTexName, INT ID, INT Y)
{
    NCButton* Button = new NCButton(NormalTexName, PushedTexName, 0);
    Button->ID = ID;
    Button->SetPosition(13, Y, 36, 36);
    Parent->CreateChild(Button, NWS_VISIBLE);
    return Button;
}

INT NCSystemMenuWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.SystemMenuWnd.systemmenu_back"));
    FrameCtrl = new NCFrameCtrl(1, 0, 0);   // 0x10010450 (ret 0xc)
    FrameID = NID_SYSMENU_FRAME;
    FrameCtrl->ID = NID_SYSMENU_FRAME;
    FrameCtrl->SetPosition(0, 0, (DWORD)(INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    ButtonC = CreateMenuButton(this, TEXT("L2UI_CH3.SystemMenuWnd.systemicon1"),
                               TEXT("L2UI_CH3.SystemMenuWnd.systemicon1_down"), NID_SYSMENU_BUTTON_C, 29);
    ButtonA = CreateMenuButton(this, TEXT("L2UI_CH3.SystemMenuWnd.systemicon2"),
                               TEXT("L2UI_CH3.SystemMenuWnd.systemicon2_down"), NID_SYSMENU_BUTTON_A, 66);
    Buttons[SYSMENU_Help] = CreateMenuButton(this, TEXT("L2UI_CH3.SystemMenuWnd.systemicon3"),
                                             TEXT("L2UI_CH3.SystemMenuWnd.systemicon3_down"), NID_SYSMENU_HELP, 103);
    ButtonB = CreateMenuButton(this, TEXT("L2UI_CH3.SystemMenuWnd.systemicon4"),
                               TEXT("L2UI_CH3.SystemMenuWnd.systemicon4_down"), NID_SYSMENU_BUTTON_B, 140);
    Buttons[SYSMENU_Options] = CreateMenuButton(this, TEXT("L2UI_CH3.SystemMenuWnd.systemicon5"),
                                                TEXT("L2UI_CH3.SystemMenuWnd.systemicon5_down"), NID_SYSMENU_OPTIONS, 177);
    Buttons[SYSMENU_Restart] = CreateMenuButton(this, TEXT("L2UI_CH3.SystemMenuWnd.systemicon6"),
                                                TEXT("L2UI_CH3.SystemMenuWnd.systemicon6_down"), NID_SYSMENU_RESTART, 214);
    Buttons[SYSMENU_Exit] = CreateMenuButton(this, TEXT("L2UI_CH3.SystemMenuWnd.systemicon7"),
                                             TEXT("L2UI_CH3.SystemMenuWnd.systemicon7_down"), NID_SYSMENU_EXIT, 251);
    return 1;
}
NW_HOOK_METHOD(0x100aae90, NCSystemMenuWnd, OnCreate)

// 0x100aa540 (vslot 38) - guarded; the five buttons of the array (the fifth never made) and A, B, C go, then NCWnd's
// (the title bar goes with it)
template <class T> static FORCEINLINE void DestroyMenuChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

INT NCSystemMenuWnd::OnDestroy()
{
    guard(NCSystemMenuWnd::OnDestroy);
    for (INT i = 0; i < SYSMENU_MAX; i++)
        DestroyMenuChild(this, Buttons[i]);
    DestroyMenuChild(this, ButtonA);
    DestroyMenuChild(this, ButtonB);
    DestroyMenuChild(this, ButtonC);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100aa540, NCSystemMenuWnd, OnDestroy)

// 0x100aa860 (vslot 44) - one body for several windows: the icon of a window that was minimized is replaced by the
// window at the icon's place and goes away
void NCSystemMenuWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Icon)
    {
        SetPosition(Icon->ScreenX, Icon->ScreenY, (DWORD)(INT)Width, (DWORD)(INT)Height);
        Icon->SetPosition(0, 0, 0, 0);
        Icon->HideWindow();
    }
}
NW_HOOK_METHOD(0x100aa860, NCSystemMenuWnd, ShowWindow)

// 0x100aa310 (vslot 56) - C3: the back from y 20 (the window's height less the 20 of the title bar) and the seven texts at x 51 in 0xFFDCDCDC (sysstrings 387 (Alt+B), 711
// (Alt+R), then 145, 470, 146, 147, 148: the buttons carry none), 37 (C2: label on the buttons)
INT NCSystemMenuWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 20, (INT)Width, (INT)(Height - 20.0f), 0, 0, (INT)Width, (INT)(Height - 20.0f), BackTex);
    TCHAR Buffer[20];
    appSprintf(Buffer, TEXT("%s %s"), *SysString(387), TEXT("(Alt+B)"));
    Device->DrawText(51, 43, 0xFFDCDCDC, Buffer, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    appSprintf(Buffer, TEXT("%s %s"), *SysString(711), TEXT("(Alt+R)"));
    Device->DrawText(51, 80, 0xFFDCDCDC, Buffer, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(51, 117, 0xFFDCDCDC, *SysString(145), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(51, 154, 0xFFDCDCDC, *SysString(470), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(51, 191, 0xFFDCDCDC, *SysString(146), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(51, 228, 0xFFDCDCDC, *SysString(147), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(51, 265, 0xFFDCDCDC, *SysString(148), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100aa310, NCSystemMenuWnd, OnPaint)

// 0x10096ec0 (vslot 67) - the Help button: the desktop's help window with argument 0 (NCConsole
// 0x100a9d40, which shows the macro help page; its C1 equivalent took no argument)
INT NCSystemMenuWnd::OnHelp()
{
    GDesktop->ToggleHelp(0);   // 0x1004BD80
    return 1;
}
NW_HOOK_METHOD(0x100a9d40, NCSystemMenuWnd, OnHelp)

// 0x100a9d80 (vslot 71) - the C1 OnHelp slot: in C2 the desktop's 0x1004C100 toggles the macro window (NCConsole::ToggleMacroList;
// NCConsole::ToggleHelp, 0x1004BD80, is the other help window's toggle)
INT NCSystemMenuWnd::OnHelpWindow()
{
    static_cast<NCConsole*>(GConsole->Desktop)->ToggleMacroList();
    return 1;
}
NW_HOOK_METHOD(0x100a9d80, NCSystemMenuWnd, OnHelpWindow)

// 0x100a9da0 (vslot 74, NewVirtual74 in the header) - the first icon of C3: the board (community) window toggles
// (NCConsole::ToggleBoard, 0x100538b0)
INT NCSystemMenuWnd::OnBoard()
{
    GDesktop->ToggleBoard();
    return 1;
}
NW_HOOK_METHOD(0x100a9da0, NCSystemMenuWnd, OnBoard)

// 0x100a9f10 (vslot 73) - C3: the petition form (the desktop's +0xEC) toggles: shown, it goes away; else it starts
// over and shows with the focus
INT NCSystemMenuWnd::OnHelpPage()
{
    if (GDesktop->PetitionBeginWnd->IsShowWindow())
    {
        GDesktop->PetitionBeginWnd->HideWindow();
        return 1;
    }
    ((NCPetitionBeginWnd*)GDesktop->PetitionBeginWnd)->ResetForm();   // 0x100d0750
    GDesktop->PetitionBeginWnd->ShowWindow();
    GDesktop->PetitionBeginWnd->SetFocus();
    return 1;
}
NW_HOOK_METHOD(0x100a9f10, NCSystemMenuWnd, OnHelpPage)

// 0x100a9d60 (vslot 69) - "Options" opens or closes the option window (the body is shared with NCLoginWnd's)
INT NCSystemMenuWnd::OnOptions()
{
    GConsole->ToggleOptionWnd();
    return 1;
}
NW_HOOK_METHOD(0x100a9d60, NCSystemMenuWnd, OnOptions)

// the confirmation of Restart / Exit (repeated in both slots)
static FORCEINLINE void AskSystemMenu(NCWnd* Menu, INT DialogID, INT MsgID)
{
    NDialogInfo Info;
    Info.ID = DialogID;
    Info.bMainBox = 1;
    Info.bInput = 0;
    Info.Owner = Menu->Entry;
    Info.SetSystemMessage(MsgID);
    GConsole->ShowDialog(&Info);
}

// 0x100aa720 (vslot 70)
INT NCSystemMenuWnd::OnRestart()
{
    AskSystemMenu(this, DIALOG_SYSMENU_RESTART, 126);
    return 1;
}
NW_HOOK_METHOD(0x100aa720, NCSystemMenuWnd, OnRestart)

// 0x100aa7c0 (vslot 72)
INT NCSystemMenuWnd::OnExit()
{
    AskSystemMenu(this, DIALOG_SYSMENU_EXIT, 125);
    return 1;
}
NW_HOOK_METHOD(0x100aa7c0, NCSystemMenuWnd, OnExit)

// 0x100a9dc0 (vslot 75) - the answer of the confirmation. C3: Restart first has the desktop save its state (0x10053780)
// and hide its chat window (+0x18C); Exit Game quits at once when the client runs only the L2 console or replays
// (GIsOnlyL2Console, GL2ReplayMode), else it asks the console to log out
INT NCSystemMenuWnd::OnDialogResult(WPARAM DialogID, LPARAM Unused)
{
    if (DialogID == DIALOG_SYSMENU_RESTART)
    {
        NCConsole* Desk = GDesktop;
        if (Desk)
        {
#if NW_HOOK_MODE
            ::nw::StockCallAt<void>(0x10053780, Desk);
#else
            // 0x10053780 is NCConsoleExt::LogoutMessenger (NConsoleWndMisc.cpp, a stand-in no header declares): the messenger
            // window's (+0x18C) sign-out, 0x1013BB30, which is its MSN page's (+0xC8) sign-out button, MsnWnd::OnSignOutButton
            ::nw::Field<MsnWnd*>(Desk->MessengerWnd, 0xC8)->OnSignOutButton();
#endif
            NCWnd* Chat = ::nw::Field<NCWnd*>(Desk, 0x18C);
            if (Chat)
                Chat->HideWindow();
        }
        L2ParamStack Params(10);
        GConsole->RequestRestart(Params);
        HideWindow();
    }
    else if (DialogID == DIALOG_SYSMENU_EXIT)
    {
        if (!GIsOnlyL2Console && !GL2ReplayMode)
            GConsole->RequestLogout();
        else
            appRequestExit(0);
    }
    return 1;
}
NW_HOOK_METHOD(0x100a9dc0, NCSystemMenuWnd, OnDialogResult)

// 0x100ab8d0 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x100ab8d0, NCSystemMenuWnd)

// 0x100a9eb0 (vslot 49) - C3: the icon has its own picture and tool tip (sysstring 193) set first,
// then NCWnd::Minimize without the sound
void NCSystemMenuWnd::Minimize()
{
    if (Icon)
        Icon->SetIcon(TEXT("L2UI_CH3.MenuIcon.Menuicon4"), 0xC1, 1);
    if (ScreenX < 0 && Icon)
        Icon->OffsetWindow(-ScreenX, 0);
    HideWindow();
    if (Icon)
    {
        Icon->ShowWindow();
        Icon->SetFocus();
    }
}
NW_HOOK_METHOD(0x100a9eb0, NCSystemMenuWnd, Minimize)

NW_CHECK_VTABLE(NCSystemMenuWnd, 0x10190210, 76)
