/*=============================================================================
    NCLobbyWnd.cpp - the character selection screen. Stock (C3): 0x1008f600 .. 0x1008f95b,
    0x10090650 .. 0x10090b19 (NCPawnSelectWnd's functions sit in between),
    0x10090d60 .. 0x1009116e.
=============================================================================*/
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCPawnSelectWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGaraDialogBox.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// NWM_COMMAND id handled by WindowProc (the buttons are in the two windows at the bottom now)
enum
{
    NID_LOBBY_10001 = 0x10001,
};

// systemmsg lines (names ours)
enum
{
    MSG_TooManyCharacters = 77,    // "You cannot create another character. ..."
    MSG_DeleteCharacter = 78,      // "Do you want to delete $s1?"
    MSG_BadCharacterName = 205,    // "Incorrect character name. Please ask the GM."
    MSG_DaysLeft = 1185,           // the unit written after the days left until the deletion
};

// dialog id of the delete question: only its OK comes back (NWM_DIALOG_RESULT)
enum { DIALOG_DeleteCharacter = 0 };

// the characters of the stage as the console keeps them (UL2ConsoleWnd +0x27AC in C3, eight User
// records)
static User* ConsoleCharacters() { return GConsole->SelectCharacterInfo; }

// 0x10090d60 - the chat texture is looked up here; the panel, the boxes and the windows are left as allocated
NCLobbyWnd::NCLobbyWnd()
{
    NumCharacters = 0;
    ChatTex = FindTexture(TEXT("NWindow.Chat"));
}
NW_HOOK_CTOR(0x10090d60, NCLobbyWnd)

// 0x10090df0
NW_HOOK_DTOR(0x10090df0, NCLobbyWnd)

// 0x1008f600 (vslot 25) - the delete question's answer arrives as NWM_DIALOG_RESULT; of the commands only 0x10001
// comes here (the buttons are in the two windows below, which call the virtuals 70 .. 73)
INT NCLobbyWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_LOBBY_10001)
            return OnCommand10001() != 0;
    }
    else if (Msg == NWM_DIALOG_RESULT)
        return OnDialogResult((INT)wParam, (INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1008f600, NCLobbyWnd, WindowProc)

// 0x10090e40 (vslot 37) - the panel in the upper left corner, the question box centered, the "Start" window and the
// function window (the buttons) at the bottom (C3: NCLobbyStartWnd and NCLobbyFunctionWnd, classes of their own)
INT NCLobbyWnd::OnCreate()
{
    NCWnd::OnCreate();

    PawnSelectWnd = new NCPawnSelectWnd;
    PawnSelectWnd->SetPosition(24, 27, 256, 108);
    CreateChild(PawnSelectWnd, 0x10000 | NWS_NOCLOSE | NWS_VISIBLE);
    PawnSelectWnd->SetTitle(*GL2GameData.SysString[157]);      // "Select Character"
    PawnSelectWnd->SetAnchor(1, 24, 27);

    DialogBox = new NCGaraDialogBox;         // the question box
    DialogBox->SetPosition((INT)(Width * 0.5f - 101.0f), (INT)(Height * 0.5f - 44.0f), 203, 87);
    CreateChild(DialogBox, 0x20 | NWS_VISIBLE);
    DialogBox->SetButtonTexts(184, 185);                       // "Yes", "No"
    DialogBox->HideWindow();

    StartWnd = new NCLobbyStartWnd;
    StartWnd->SetPosition(GSEKScreenX / 2 - 71, GSEKScreenY - 111, 140, 49);
    CreateChild(StartWnd, NWS_VISIBLE);
    StartWnd->SetAnchor(8, 0, -10);

    FunctionWnd = new NCLobbyFunctionWnd;
    FunctionWnd->SetPosition(GSEKScreenX - 154, GSEKScreenY - 200, 120, 138);
    CreateChild(FunctionWnd, NWS_VISIBLE);
    FunctionWnd->SetAnchor(9, -34, -10);
    return 1;
}
NW_HOOK_METHOD(0x10090e40, NCLobbyWnd, OnCreate)

// 0x10090910 (vslot 38) - the two windows at the bottom are left alone
INT NCLobbyWnd::OnDestroy()
{
    if (PawnSelectWnd)
    {
        RemoveChildWindow(PawnSelectWnd->Entry);
        delete PawnSelectWnd;
        PawnSelectWnd = NULL;
    }
    if (DialogBox)
    {
        RemoveChildWindow(DialogBox->Entry);
        delete DialogBox;
        DialogBox = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10090910, NCLobbyWnd, OnDestroy)

// 0x1008f900 (vslot 44) - nothing chosen yet: the selection cleared and the buttons enabled
void NCLobbyWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    GConsole->SelectedCharacterNum(-1);
    PawnSelectWnd->NameCombo->SetSelected(-1);
    PawnSelectWnd->Selected = -1;
    FunctionWnd->CreateButton->EnableWindow(1);
    FunctionWnd->DeleteButton->EnableWindow(1);
    StartWnd->StartButton->EnableWindow(1);
    DialogBox->HideWindow();
}
NW_HOOK_METHOD(0x1008f900, NCLobbyWnd, ShowWindow)

// 0x100906a0 (vslot 56) - each character's name over its head: half its collision height above the location, plus 7
// (4 more for mesh type 0), white, reddened by its karma; a character marked for deletion (and not locked) also gets
// its days left under the name. Every record is copied whole first, as in the stock code. The clip goes on before
// BeginPaint.
INT NCLobbyWnd::OnPaint(NCGDevice* Device)
{
    guard(NCLobbyWnd::OnPaint);
    TCHAR Days[512];
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    for (INT i = 0; i < 8; i++)
    {
        User Info = ConsoleCharacters()[i];
        if (Info.Pawn)
        {
            FVector Loc = Info.Pawn->Location;
            FLOAT Z = Info.Pawn->CollisionHeight * 0.5f + Loc.Z + 7.f;
            if (Info.GetMeshType() == 0)
                Z += 4.f;
            INT Red = 0;
            if (Info.Karma > 255)
                Red = 255;
            if (Info.Karma > 0)
            {
                Red = Info.Karma / 16 + 100;
                if (Info.Karma >= Red)
                {
                    Red = Info.Karma;
                    if (Info.Karma >= 255)
                        Red = 255;
                }
            }
            DWORD Color = ((~Red) << 8 | (255 - Red)) | 0xFFFF0000;
            Device->Draw3DCoordText(FVector(Loc.X, Loc.Y, Z), Color, Info.GetName(), NULL, 2, 0,
                                    (L2FontType)1);
            INT DaysLeft = Info.DeleteDays & 0xFFFF;
            if (DaysLeft > 0 && !(Info.DeleteDays & CHAR_LOCKED_FLAG))
            {
                Z += 9.0f;
                appSprintf(Days, TEXT("%d%s"), DaysLeft, *GL2GameData.SystemMsg[MSG_DaysLeft].Message);
                // NCGDevice::DrawChatting (0x10010ef0: the same arguments as the text's, the color first)
                Device->DrawChatting(FVector(Loc.X, Loc.Y, Z), (DWORD)0xFFDCDCDC, Days, 2, 0, (L2FontType)0, 0, 1, 0);
            }
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100906a0, NCLobbyWnd, OnPaint)

// 0x1008f990 (vslot 59) - Enter is a click of "Start"; the base class handles the key as well
INT NCLobbyWnd::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN)
        OnStartClick();
    NCWnd::OnKeyDown(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x1008f990, NCLobbyWnd, OnKeyDown)

// 0x1008f6a0 - enter the world with the chosen character, unless it is marked for deletion or locked, or its name
// has characters the font cannot draw
void NCLobbyWnd::OnStart()
{
    INT Index = PawnSelectWnd->GetSelected();
    if (Index >= 0)
    {
        DWORD DeleteDays = PawnSelectWnd->Characters[Index].DeleteDays;
        if ((INT)(DeleteDays & 0xFFFF) <= 0 && (INT)(DeleteDays & CHAR_LOCKED_FLAG) <= 0 && NumCharacters > 0 &&
            Index < NumCharacters)
        {
            const TCHAR* Name = PawnSelectWnd->NameCombo->GetItemText(Index);
            if (!Name || !*Name || !GConsole->HasInvalidChars((TCHAR*)Name))
            {
                GConsole->RequestGameStart(Index);
                GConsole->StopIntroMusic();
            }
            else
                GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_BadCharacterName].Message);
        }
    }
}
NW_HOOK_METHOD(0x1008f6a0, NCLobbyWnd, OnStart)

// 0x1008f840 (vslot 69) - the delete question's answer (NWM_DIALOG_RESULT): 0 cancelled, the selection is cleared and
// the buttons enabled; else the chosen character is deleted
INT NCLobbyWnd::OnDialogResult(INT bYes, INT Unused)
{
    if (bYes == 0)
    {
        GConsole->SelectedCharacterNum(-1);
        PawnSelectWnd->NameCombo->SetSelected(-1);
        PawnSelectWnd->Selected = -1;
        FunctionWnd->CreateButton->EnableWindow(1);
        FunctionWnd->DeleteButton->EnableWindow(1);
        StartWnd->StartButton->EnableWindow(1);
    }
    else
    {
        INT Index = PawnSelectWnd->GetSelected();
        if (Index >= 0)
            GConsole->RequestCharacterDelete(Index);
    }
    return 1;
}
NW_HOOK_METHOD(0x1008f840, NCLobbyWnd, OnDialogResult)

// 0x1008f770 (vslot 70) - the "Start" button: the voices are stopped (fade 1.5), then the character starts
INT NCLobbyWnd::OnStartClick()
{
    if (GConsole && GConsole->Engine && GConsole->Engine->Audio)
        GConsole->Engine->Audio->StopAllVoice(1.5f, 0);
    OnStart();
    return 1;
}
NW_HOOK_METHOD(0x1008f770, NCLobbyWnd, OnStartClick)

// 0x1008f7b0 (vslot 71) - the "Previous" button: the voices are stopped, then back to the login
INT NCLobbyWnd::OnPreviousClick()
{
    if (GConsole && GConsole->Engine && GConsole->Engine->Audio)
        GConsole->Engine->Audio->StopAllVoice(1.5f, 0);
    GConsole->ReturnToLogin();
    return 1;
}
NW_HOOK_METHOD(0x1008f7b0, NCLobbyWnd, OnPreviousClick)

// 0x1008f7f0 (vslot 72) - seven characters at most
INT NCLobbyWnd::OnCreateCharacter()
{
    if (NumCharacters == 7)
    {
        GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_TooManyCharacters].Message);
        return 1;
    }
    if (NumCharacters < 7)
        GConsole->RequestNewCharacter();
    return 1;
}
NW_HOOK_METHOD(0x1008f7f0, NCLobbyWnd, OnCreateCharacter)

// 0x100909e0 (vslot 73) - "Do you want to delete ...?" (a locked character gets no question)
INT NCLobbyWnd::OnDelete()
{
    TCHAR Name[32];
    TCHAR Text[128];
    INT Index = PawnSelectWnd->GetSelected();
    if (Index >= 0 && !(PawnSelectWnd->Characters[Index].DeleteDays & CHAR_LOCKED_FLAG))
    {
        appSprintf(Name, TEXT("%s"), PawnSelectWnd->GetCharacterName(PawnSelectWnd->GetSelected()));
        GConsole->FormatSystemMessage(Text, *GL2GameData.SystemMsg[MSG_DeleteCharacter].Message, Name, NULL);
        NDialogInfo Info;
        Info.ID = DIALOG_DeleteCharacter;
        Info.bMainBox = 0;
        Info.bInput = 0;
        Info.Owner = Entry;
        Info.SetText(Text);
        DialogBox->SetInfo(&Info);
        DialogBox->ShowWindow();
    }
    return 1;
}
NW_HOOK_METHOD(0x100909e0, NCLobbyWnd, OnDelete)

// 0x10090650 - one character of the server's list (NConsoleWnd)
INT NCLobbyWnd::AddCharacter(User Info)
{
    if (NumCharacters >= 7)
        return 0;
    PawnSelectWnd->AddCharacter(Info);
    NumCharacters++;
    return 1;
}
NW_HOOK_METHOD(0x10090650, NCLobbyWnd, AddCharacter)

// 0x1008f680 - NConsoleWnd::ReturnToLogin and before a new list
void NCLobbyWnd::ClearCharacters()
{
    PawnSelectWnd->ClearCharacters();
    NumCharacters = 0;
}
NW_HOOK_METHOD(0x1008f680, NCLobbyWnd, ClearCharacters)

NW_CHECK_VTABLE(NCLobbyWnd, 0x1018b110, 74)

/*-----------------------------------------------------------------------------
    NCLobbyStartWnd (C3).
-----------------------------------------------------------------------------*/

// NWM_COMMAND ids of the two windows' buttons (names ours)
enum
{
    NID_LOBBY_START = 0x1009,          // NCLobbyStartWnd's button
    NID_LOBBY_PREVIOUS = 0x100A,       // NCLobbyFunctionWnd's
    NID_LOBBY_CREATE = 0x100B,
    NID_LOBBY_DELETE = 0x100C,
};

// 0x1008f2a0
NCLobbyStartWnd::NCLobbyStartWnd()
{
}
NW_HOOK_CTOR(0x1008f2a0, NCLobbyStartWnd)

// 0x1008f2f0
NW_HOOK_DTOR(0x1008f2f0, NCLobbyStartWnd)

// 0x1008eed0 (vslot 25)
INT NCLobbyStartWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_LOBBY_START)
        return OnGameStartBtnClick() != 0;
    return 0;
}
NW_HOOK_METHOD(0x1008eed0, NCLobbyStartWnd, WindowProc)

// 0x1008f150 (vslot 37) - the button (disabled until a character is chosen) and its backdrop; NCWnd::OnCreate is not
// called
INT NCLobbyStartWnd::OnCreate()
{
    guard(NCLobbyStartWnd::OnCreate);
    BackTex = FindTexture(TEXT("L2UI_CH3.LobbyWnd.lobby_startbtnback"));

    StartButton = new NCButton(TEXT("L2UI_CH3.Button.BigButton2"),
                                     TEXT("L2UI_CH3.Button.BigButton2_down"), 0);
    StartButton->ID = NID_LOBBY_START;
    StartButton->SetPosition(13, 10, 116, 31);
    CreateChild(StartButton, NWS_VISIBLE);
    StartButton->SetTextID(160);                                // "Start"
    StartButton->EnableWindow(0);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008f150, NCLobbyStartWnd, OnCreate)

// 0x1008ef30 (vslot 38) - the button goes; NCWnd::OnDestroy is not called (as in the stock code)
INT NCLobbyStartWnd::OnDestroy()
{
    guard(NCLobbyStartWnd::OnDestroy);
    if (StartButton)
    {
        RemoveChildWindow(StartButton->Entry);
        delete StartButton;
        StartButton = NULL;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008ef30, NCLobbyStartWnd, OnDestroy)

// 0x1008efd0 (vslot 56) - the backdrop (a 140 x 49 piece) stretched over the window
INT NCLobbyStartWnd::OnPaint(NCGDevice* Device)
{
    guard(NCLobbyStartWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, 140, 49, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008efd0, NCLobbyStartWnd, OnPaint)

// 0x1008f0c0 (vslot 68) - the lobby's slot 70 (OnStartClick); the cast is checked
INT NCLobbyStartWnd::OnGameStartBtnClick()
{
    guard(NCLobbyStartWnd::OnGameStartBtnClick);
    CastChecked<NCLobbyWnd>(Entry->Parent->Wnd)->OnStartClick();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008f0c0, NCLobbyStartWnd, OnGameStartBtnClick)

NW_CHECK_VTABLE(NCLobbyStartWnd, 0x1018ab78, 69)

/*-----------------------------------------------------------------------------
    NCLobbyFunctionWnd (C3).
-----------------------------------------------------------------------------*/

// 0x1008ee20
NCLobbyFunctionWnd::NCLobbyFunctionWnd()
{
}
NW_HOOK_CTOR(0x1008ee20, NCLobbyFunctionWnd)

// 0x1008ee70
NW_HOOK_DTOR(0x1008ee70, NCLobbyFunctionWnd)

// 0x1008e6d0 (vslot 25)
INT NCLobbyFunctionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_LOBBY_PREVIOUS)
            return OnGameCancelBtnClick() != 0;
        if (wParam == NID_LOBBY_CREATE)
            return OnNewCharacterBtnClick() != 0;
        if (wParam == NID_LOBBY_DELETE)
            return OnDelCharacterBtnClick() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1008e6d0, NCLobbyFunctionWnd, WindowProc)

// 0x1008eb70 (vslot 37) - the backdrop and the buttons "Create" and "Delete" (disabled until a character is chosen)
// above the one of sysstring 884; NCWnd::OnCreate is called
INT NCLobbyFunctionWnd::OnCreate()
{
    guard(NCLobbyFunctionWnd::OnCreate);
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.LobbyWnd.lobby_menuback"));

    CreateButton = new NCButton(TEXT("L2UI_CH3.Button.BigButton"),
                                      TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    CreateButton->ID = NID_LOBBY_CREATE;
    CreateButton->SetPosition(13, 20, 96, 23);
    CreateChild(CreateButton, NWS_VISIBLE);
    CreateButton->SetTextID(158);                               // "Create"

    DeleteButton = new NCButton(TEXT("L2UI_CH3.Button.BigButton"),
                                      TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    DeleteButton->ID = NID_LOBBY_DELETE;
    DeleteButton->SetPosition(13, 45, 96, 23);
    CreateChild(DeleteButton, NWS_VISIBLE);
    DeleteButton->SetTextID(159);                               // "Delete"
    DeleteButton->EnableWindow(0);

    PreviousButton = new NCButton(TEXT("L2UI_CH3.Button.BigButton"),
                                        TEXT("L2UI_CH3.Button.BigButton_down"), 0);
    PreviousButton->ID = NID_LOBBY_PREVIOUS;
    PreviousButton->SetPosition(13, 95, 96, 23);
    CreateChild(PreviousButton, NWS_VISIBLE);
    PreviousButton->SetTextID(884);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008eb70, NCLobbyFunctionWnd, OnCreate)

// 0x1008e770 (vslot 38) - the three buttons go; NCWnd::OnDestroy is not called (as in the stock code)
INT NCLobbyFunctionWnd::OnDestroy()
{
    guard(NCLobbyFunctionWnd::OnDestroy);
    if (PreviousButton)
    {
        RemoveChildWindow(PreviousButton->Entry);
        delete PreviousButton;
        PreviousButton = NULL;
    }
    if (CreateButton)
    {
        RemoveChildWindow(CreateButton->Entry);
        delete CreateButton;
        CreateButton = NULL;
    }
    if (DeleteButton)
    {
        RemoveChildWindow(DeleteButton->Entry);
        delete DeleteButton;
        DeleteButton = NULL;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008e770, NCLobbyFunctionWnd, OnDestroy)

// 0x1008e870 (vslot 56) - the backdrop (a 120 x 138 piece) stretched over the window
INT NCLobbyFunctionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCLobbyFunctionWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, 120, 138, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008e870, NCLobbyFunctionWnd, OnPaint)

// 0x1008e9c0 (vslot 68) - the lobby's slot 71 (OnPreviousClick); the cast is checked
INT NCLobbyFunctionWnd::OnGameCancelBtnClick()
{
    guard(NCLobbyFunctionWnd::OnGameCancelBtnClick);
    CastChecked<NCLobbyWnd>(Entry->Parent->Wnd)->OnPreviousClick();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008e9c0, NCLobbyFunctionWnd, OnGameCancelBtnClick)

// 0x1008ea50 (vslot 69) - the lobby's slot 72 (OnCreateCharacter)
INT NCLobbyFunctionWnd::OnNewCharacterBtnClick()
{
    guard(NCLobbyFunctionWnd::OnNewCharacterBtnClick);
    CastChecked<NCLobbyWnd>(Entry->Parent->Wnd)->OnCreateCharacter();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008ea50, NCLobbyFunctionWnd, OnNewCharacterBtnClick)

// 0x1008eae0 (vslot 70) - the lobby's slot 73 (OnDelete)
INT NCLobbyFunctionWnd::OnDelCharacterBtnClick()
{
    guard(NCLobbyFunctionWnd::OnDelCharacterBtnClick);
    CastChecked<NCLobbyWnd>(Entry->Parent->Wnd)->OnDelete();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008eae0, NCLobbyFunctionWnd, OnDelCharacterBtnClick)

// 0x1008e960 - CastChecked<NCLobbyWnd>, the stock's out-of-line instance (the function and start windows' buttons)
static NCLobbyWnd* CastCheckedNCLobbyWnd(UObject* Object)
{
    return CastChecked<NCLobbyWnd>(Object);
}
NW_HOOK_FUNC(0x1008E960, CastCheckedNCLobbyWnd)

NW_CHECK_VTABLE(NCLobbyFunctionWnd, 0x1018a880, 71)
