/*=============================================================================
    NCGodWnd.cpp - the screen-sized top window of the game screen (C3 only; C3 addresses). Stock: 0x10085620 ..
    0x10085D8F (the small methods, OnDestroy, DefaultPosition, SetWindowSize, ShowWarningMessage), OnCreate 0x10085DD0
    .. 0x1008618B, the constructor 0x100861C0 and the destructor 0x10086250. The console calls the small methods by
    address (NConsoleWnd::ShowMessageWnd, ToggleOptionWnd, C3Virtual202 / 203, ...).
=============================================================================*/
#include "nwindow/NCGodWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCRestartMenuWnd.h"
#include "nwindow/NCWarningDialogBox.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NGameData.h"

// The sizes and places of the children (the stock code repeats them inline)
enum
{
    GOD_MessageHeight = 52,
    GOD_RestartSize = 128,
    GOD_OptionWidth = 256,
    GOD_OptionHeight = 401,
    GOD_WarningWidth = 256,
    GOD_WarningHeight = 108,
    SYSSTR_OptionTitle = 146,   // GL2GameData + 0x66164
};

// A child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x100861c0 - the five fields cleared, then the vtable
NCGodWnd::NCGodWnd()
    : UnknownC8(NULL), RestartMenu(NULL), MessageWnd(NULL), OptionWnd(NULL), WarningBox(NULL)
{
}
NW_HOOK_CTOR(0x100861c0, NCGodWnd)

// 0x10086250
NW_HOOK_DTOR(0x10086250, NCGodWnd)

// 0x10119f60 (vslot 25) - NControl's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCGodWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x10119f60)
#else
NW_SLOT_IS("?WindowProc@NCGodWnd@@UAEHIIJ@Z", "?WindowProc@NControl@@UAEHIIJ@Z")   // NCGodWnd::WindowProc = NControl::WindowProc (0x10119f60)
#endif

// 0x10085620 (guard: NCGodWnd::ReceiveServerList; ret 4, the argument is not used) - the message line emptied (the
// console shows its own text while the server list is awaited) and the options window hidden
void NCGodWnd::ReceiveServerList(INT Param)
{
    guard(NCGodWnd::ReceiveServerList);
    if (MessageWnd)
        MessageWnd->ClearText();
    if (OptionWnd)
        OptionWnd->HideWindow();
    unguard;
}
NW_HOOK_METHOD(0x10085620, NCGodWnd, ReceiveServerList)

// 0x100856b0 (guard: NCGodWnd::RestartBegin) - the options window and the restart menu hidden
void NCGodWnd::RestartBegin()
{
    guard(NCGodWnd::RestartBegin);
    if (OptionWnd)
        OptionWnd->HideWindow();
    if (RestartMenu)
        RestartMenu->HideWindow();
    unguard;
}
NW_HOOK_METHOD(0x100856b0, NCGodWnd, RestartBegin)

// 0x10085740 (guard: NCGodWnd::ShowOptionWnd) - the options window toggles: hidden with the close sound (6), shown
// with the open sound (5) and the focus
void NCGodWnd::ShowOptionWnd()
{
    guard(NCGodWnd::ShowOptionWnd);
    if (OptionWnd->IsShowWindow())
    {
        PlayInterfaceSound(6);
        OptionWnd->HideWindow();
    }
    else
    {
        PlayInterfaceSound(5);
        OptionWnd->ShowWindow();
        OptionWnd->SetFocus();
    }
    unguard;
}
NW_HOOK_METHOD(0x10085740, NCGodWnd, ShowOptionWnd)

// 0x10085810 (guard: NCGodWnd::ShowMessage; ret 4) - the line's text is replaced by the one given (nothing for an empty
// one), and the line is shown with the focus when it was not
void NCGodWnd::ShowMessage(const TCHAR* Text)
{
    guard(NCGodWnd::ShowMessage);
    if (MessageWnd)
    {
        MessageWnd->ClearText();
        if (Text && *Text)
            MessageWnd->SetText(Text);
        if (!MessageWnd->IsShowWindow())
        {
            MessageWnd->ShowWindow();
            MessageWnd->SetFocus();
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x10085810, NCGodWnd, ShowMessage)

// 0x100858e0 (guard: NCGodWnd::HideRestartMenu)
void NCGodWnd::HideRestartMenu()
{
    guard(NCGodWnd::HideRestartMenu);
    if (RestartMenu)
        RestartMenu->HideWindow();
    unguard;
}
NW_HOOK_METHOD(0x100858e0, NCGodWnd, HideRestartMenu)

// 0x10085960 (guard: NCGodWnd::ShowRestartMenu)
void NCGodWnd::ShowRestartMenu()
{
    guard(NCGodWnd::ShowRestartMenu);
    if (RestartMenu)
        RestartMenu->ShowWindow();
    unguard;
}
NW_HOOK_METHOD(0x10085960, NCGodWnd, ShowRestartMenu)

// 0x100859e0 (vslot 38, guard: NCGodWnd::OnDestroy) - the options window, the message line, the restart menu and the
// warning box go; NCWnd's OnDestroy is not called
INT NCGodWnd::OnDestroy()
{
    guard(NCGodWnd::OnDestroy);
    DestroyChild(this, OptionWnd);
    DestroyChild(this, MessageWnd);
    DestroyChild(this, RestartMenu);
    DestroyChild(this, WarningBox);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100859e0, NCGodWnd, OnDestroy)

// 0x10085b00 (guard: NCGodWnd::DefaultPosition) - the windows back in their places for the size the window has now:
// the options window centered (256 x 401: its place is set on the screen, from this window's origin), the message
// line along the bottom edge (52 high; its width follows the window's) and the restart menu centered (128 x 128)
void NCGodWnd::DefaultPosition()
{
    guard(NCGodWnd::DefaultPosition);
    POINT Origin;
    Origin.x = ScreenX;
    Origin.y = ScreenY;
    FLOAT W = Width;
    FLOAT H = Height;
    if (OptionWnd)
        OptionWnd->SetScreenPos((INT)(W * 0.5f - 128.0f), (INT)(H * 0.5f - 200.0f), &Origin);
    if (MessageWnd)
    {
        MessageWnd->SetWindowSize(W, (FLOAT)GOD_MessageHeight);
        MessageWnd->SetScreenPos(0, (INT)(H - 52.0f), &Origin);
    }
    if (RestartMenu)
        RestartMenu->SetPosition((INT)(W * 0.5f - 64.0f), (INT)(H * 0.5f - 64.0f), GOD_RestartSize, GOD_RestartSize);
    unguard;
}
NW_HOOK_METHOD(0x10085b00, NCGodWnd, DefaultPosition)

// 0x10085c80 (vslot 43, guard: NCGodWnd::SetWindowSize; ret 8) - NCWnd's, then the message line takes the width
void NCGodWnd::SetWindowSize(FLOAT W, FLOAT H)
{
    guard(NCGodWnd::SetWindowSize);
    NCWnd::SetWindowSize(W, H);
    FLOAT NewWidth = Width;
    if (MessageWnd)
        MessageWnd->SetWindowSize(NewWidth, (FLOAT)GOD_MessageHeight);
    unguard;
}
NW_HOOK_METHOD(0x10085c80, NCGodWnd, SetWindowSize)

// 0x10085d20 (guard: NCGodWnd::ShowWarningMessage; ret 8) - the warning box: marked as one that ends the client when
// asked to (bExit), and, with a text, raised modal (NCWarningDialogBox 0x10007520 sets the text, NCDialogBox 0x10007470
// shows it as the modal window with the focus)
void NCGodWnd::ShowWarningMessage(const TCHAR* Text, INT bExit)
{
    guard(NCGodWnd::ShowWarningMessage);
    if (bExit)
        WarningBox->bExit = 1;
    if (Text)
    {
        WarningBox->SetWarningMessage(Text);   // 0x10007520
        WarningBox->DoModal();                 // 0x10007470
    }
    unguard;
}
NW_HOOK_METHOD(0x10085d20, NCGodWnd, ShowWarningMessage)

// 0x10085dd0 (vslot 37, guard: NCGodWnd::OnCreate) - the restart menu (hidden; only for a client that is not the test
// console, and while +0xC8 is empty), the message line with its first text (shown, anchored at the bottom), the options
// window (hidden, with its title) and the warning box (hidden); NCWnd's OnCreate is not called
INT NCGodWnd::OnCreate()
{
    guard(NCGodWnd::OnCreate);
    FLOAT H = Height;
    FLOAT W = Width;
    if (!UnknownC8 && !GIsOnlyL2Console)
    {
        RestartMenu = new NCRestartMenuWnd;
        RestartMenu->SetPosition((INT)(W * 0.5f - 64.0f), (INT)(H * 0.5f - 64.0f), GOD_RestartSize, GOD_RestartSize);
        CreateChild(RestartMenu, NWS_VISIBLE);
        RestartMenu->HideWindow();
    }

    MessageWnd = new NCMessageWnd;
    MessageWnd->SetPosition(0, (INT)(H - 52.0f), (DWORD)(INT)W, GOD_MessageHeight);
    CreateChild(MessageWnd, NWS_VISIBLE);
    MessageWnd->SetText(*::nw::Field<FString>(&GL2GameData, 0x407DC));
    MessageWnd->ShowWindow();
    MessageWnd->SetAnchor(8, 0, 0);

    OptionWnd = new NCOptionWnd((INT)(INT_PTR)UnknownC8);   // the stock pushes the pointer (ret 4)
    OptionWnd->SetPosition((INT)(W * 0.5f - 128.0f), (INT)(H * 0.5f - 200.0f), GOD_OptionWidth, GOD_OptionHeight);
    CreateChild(OptionWnd, NWS_VISIBLE | NWS_TOPMOST | 0x10000);
    OptionWnd->HideWindow();
    OptionWnd->SetTitle(*GL2GameData.SysString[SYSSTR_OptionTitle]);

    WarningBox = new NCWarningDialogBox;
    WarningBox->SetPosition(GSEKScreenX / 2 - 128, GSEKScreenY / 2 - 54, GOD_WarningWidth, GOD_WarningHeight);
    CreateChild(WarningBox, NWS_VISIBLE);
    WarningBox->HideWindow();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10085dd0, NCGodWnd, OnCreate)

NW_CHECK_VTABLE(NCGodWnd, 0x101895b8, 68)
