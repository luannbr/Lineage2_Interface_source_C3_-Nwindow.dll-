/*=============================================================================
    NCGameOptionWnd.cpp - the game page of the options. Stock: 0x1007dd70 .. 0x1007de64,
    0x10080970 .. 0x10080da6, 0x10080de0 .. 0x100817b1, 0x10082440 .. 0x100825a4,
    0x10082f10 .. 0x100837e9.
=============================================================================*/
#include "nwindow/NCGameOptionWnd.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NKeyCrypt.h"

// 0x100bd520
NCGameOptionWnd::NCGameOptionWnd()
{
    Unused9C[0] = NULL;
    Unused9C[1] = NULL;
    Unused9C[2] = NULL;
    GamePadBox = NULL;
}
NW_HOOK_CTOR(0x100bd520, NCGameOptionWnd)

// 0x100bd580
NW_HOOK_DTOR(0x100bd580, NCGameOptionWnd)

// 0x100b71a0 (vslot 25)
INT NCGameOptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == (WPARAM)NID_GAMEOPT_OTHERPC)
            return OnOtherPCClicked() != 0;
        if (wParam == (WPARAM)NID_GAMEOPT_RESET)
            return OnReset() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100b71a0, NCGameOptionWnd, WindowProc)

// 0x100bc810 (vslot 37) - C3: the labels belong to the check boxes now (the page draws only five headings); two more
// boxes (transparency and arrow mode) and the screen shot quality box; the reset button of the new skin. A check box is
// made with the constructor 0x100049E0 (label, checked, label left, 0)
static FORCEINLINE NCCheckBox* NewOptionBox(NCWnd* Parent, INT LabelID, INT ID, INT X, INT Y)
{
    NCCheckBox* Box = new NCCheckBox(*NCObject::SysString(LabelID), 1, 0, 0);
    Box->ID = ID;
    Box->SetPosition(X, Y, 100, 12);
    Parent->CreateChild(Box, NWS_VISIBLE);
    return Box;
}

INT NCGameOptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    LanguageCombo = new NCComboBox;
    LanguageCombo->ID = NID_GAMEOPT_LANGUAGE;
    LanguageCombo->SetPosition(87, 50, 145, 17);
    CreateChild(LanguageCombo, NWS_VISIBLE);
    NameBoxes[NAMEBOX_Player] = NewOptionBox(this, 436, NID_GAMEOPT_PLAYER, 84, 85);
    NameBoxes[NAMEBOX_Monster] = NewOptionBox(this, 437, NID_GAMEOPT_MONSTER, 84, 102);
    NameBoxes[NAMEBOX_OtherPC] = NewOptionBox(this, 438, NID_GAMEOPT_OTHERPC, 84, 119);
    NameBoxes[NAMEBOX_Clan] = NewOptionBox(this, 439, NID_GAMEOPT_CLAN, 95, 136);
    NameBoxes[NAMEBOX_Party] = NewOptionBox(this, 440, NID_GAMEOPT_PARTY, 95, 153);
    NameBoxes[NAMEBOX_General] = NewOptionBox(this, 441, NID_GAMEOPT_GENERAL, 95, 170);
    TrackingBox = NewOptionBox(this, 463, NID_GAMEOPT_TRACKING, 135, 199);
    EnterChatBox = NewOptionBox(this, 442, NID_GAMEOPT_ENTERCHAT, 25, 199);
    OldChatBox = NewOptionBox(this, 566, NID_GAMEOPT_OLDCHAT, 25, 216);

    ResetButton = new NCButton(TEXT("L2UI_ch3.button.smallbutton2"), TEXT("L2UI_ch3.button.smallbutton2_down"), 0);
    ResetButton->ID = NID_GAMEOPT_RESET;
    ResetButton->SetPosition(88, 14, 66, 21);
    CreateChild(ResetButton, NWS_VISIBLE);
    ResetButton->SetTextID(0x1DF);   // "Initialize"

    TransparencyBox = NewOptionBox(this, 927, NID_GAMEOPT_TRANSPARENCY, 164, 18);
    ArrowBox = NewOptionBox(this, 928, NID_GAMEOPT_ARROW, 25, 233);

    PartyLootCombo = new NCComboBox;
    PartyLootCombo->ID = NID_GAMEOPT_PARTYLOOT;
    PartyLootCombo->SetPosition(87, 295, 145, 17);
    CreateChild(PartyLootCombo, NWS_VISIBLE | 0x10);   // 0x10: meaning unknown
    if (GL2CanUseJoystick)
        GamePadBox = NewOptionBox(this, 606, NID_GAMEOPT_GAMEPAD, 135, 233);
    KeyCryptBox = NewOptionBox(this, 926, NID_GAMEOPT_KEYCRYPT, 135, 216);
    ScreenShotCombo = new NCComboBox;
    ScreenShotCombo->ID = NID_GAMEOPT_SHOTQUALITY;
    ScreenShotCombo->SetPosition(87, 250, 145, 17);
    CreateChild(ScreenShotCombo, NWS_VISIBLE | 0x10);
    InitItems();
    return 1;
}
NW_HOOK_METHOD(0x100bc810, NCGameOptionWnd, OnCreate)

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

// 0x100ba200 (vslot 38) - guarded; the children in the stock's order
INT NCGameOptionWnd::OnDestroy()
{
    guard(NCGameOptionWnd::OnDestroy);
    for (INT i = 0; i < NAMEBOX_MAX; i++)
        DestroyChild(this, NameBoxes[i]);
    DestroyChild(this, LanguageCombo);
    DestroyChild(this, ResetButton);
    DestroyChild(this, Unused9C[0]);
    DestroyChild(this, Unused9C[1]);
    DestroyChild(this, Unused9C[2]);
    DestroyChild(this, EnterChatBox);
    DestroyChild(this, OldChatBox);
    DestroyChild(this, TrackingBox);
    DestroyChild(this, PartyLootCombo);
    DestroyChild(this, ScreenShotCombo);
    if (GL2CanUseJoystick && GamePadBox)
        DestroyChild(this, GamePadBox);
    DestroyChild(this, TransparencyBox);
    DestroyChild(this, ArrowBox);
    DestroyChild(this, KeyCryptBox);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100ba200, NCGameOptionWnd, OnDestroy)

// 0x100b7240 (vslot 44) - the party loot rule cannot be changed by one who is not the leader (User +0x190); C3: the
// options window's second texture (its +0xCC) is set on each show
void NCGameOptionWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    User* Me = GConsole->GetMyUser();
    if (!Me || Me->Unknown188 == 0)
        PartyLootCombo->EnableWindow(1);
    else
        PartyLootCombo->EnableWindow(0);
    ::nw::Field<UTexture*>(Entry->Parent->Wnd, 0xCC) = FindTexture(TEXT("L2ui_ch3.option3_back"));
}
NW_HOOK_METHOD(0x100b7240, NCGameOptionWnd, ShowWindow)

// a label right-aligned on x Right (repeated inline in the stock OnPaint)
static FORCEINLINE void DrawOptionLabel(NCGDevice* Device, INT TextID, INT Right, INT Y)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *NCObject::SysString(TextID), 0, 0, 0, 0, 0);
    Device->DrawText(Right - Size.cx, Y, 0xFFDCDCDC, *NCObject::SysString(TextID), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
}

// 0x100b9f20 (vslot 56) - C3: the combo is disabled while the player is in a party or the desktop's window at +0x148
// shows (the stock reads +0xF0 +0xD4 and +0x148 of the desktop); five headings right-aligned, in 0xFFDCDCDC
INT NCGameOptionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCOptionWnd::OnPaint);
    INT Members = ::nw::Field<INT>(::nw::Field<void*>(GDesktop, 0xF0), 0xD4);
    NCWnd* Other = ::nw::Field<NCWnd*>(GDesktop, 0x148);
    if (Members < 1 && !Other->IsShowWindow())
        PartyLootCombo->EnableWindow(1);
    else
        PartyLootCombo->EnableWindow(0);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    DrawOptionLabel(Device, 478, 82, 18);
    DrawOptionLabel(Device, 329, 79, 53);    // Language
    DrawOptionLabel(Device, 435, 76, 85);    // Display
    DrawOptionLabel(Device, 486, 79, 298);   // Party Loot
    DrawOptionLabel(Device, 979, 87, 252);   // Screen shot quality
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b9f20, NCGameOptionWnd, OnPaint)

// 0x100bb020 (vslot 68) - "Other PCs" switches its three sub-boxes
INT NCGameOptionWnd::OnOtherPCClicked()
{
    if (NameBoxes[NAMEBOX_OtherPC]->bChecked)
    {
        NameBoxes[NAMEBOX_Clan]->bDisabled = 0;
        NameBoxes[NAMEBOX_Party]->bDisabled = 0;
        NameBoxes[NAMEBOX_General]->bDisabled = 0;
    }
    else
    {
        NameBoxes[NAMEBOX_Clan]->bDisabled = 1;
        NameBoxes[NAMEBOX_Party]->bDisabled = 1;
        NameBoxes[NAMEBOX_General]->bDisabled = 1;
    }
    return 1;
}
NW_HOOK_METHOD(0x100bb020, NCGameOptionWnd, OnOtherPCClicked)

// 0x100b7220 (vslot 69)
INT NCGameOptionWnd::OnReset()
{
    GConsole->ResetWindowPositions();
    return 1;
}
NW_HOOK_METHOD(0x100b7220, NCGameOptionWnd, OnReset)

// The nProtect key encryption driver's helper (NKeyCrypt, NConsoleWnd +0x359C): the stock calls the functions that
// never read the object with it in ecx, and the two npkcrypt.dll ones through its own linker thunks (GetLoadStartup
// 0x1003ed30, LoadAtStartup 0x1003eb80, one argument); 0x1003ec20 tells whether the process may install the driver
// (a member of the Administrators group)
static INT KeyCryptGetLoadStartup() { return NKeyCrypt::GetLoadStartup(); }
static void KeyCryptLoadAtStartup(INT bLoad) { NKeyCrypt::LoadAtStartup(bLoad); }
static INT KeyCryptIsAdmin() { return NKeyCrypt::IsAdministrator(); }

// 0x100ba610 - C3: the screen shot quality, the transparency and arrow modes are read as well
void NCGameOptionWnd::LoadSettings()
{
    UBOOL bNative = 1;
    UBOOL bOtherPC = 0;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("IsNative"), bNative, TEXT("Option.ini")))
        bNative = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("MyName"), GL2ShowMyName, TEXT("Option.ini")))
        GL2ShowMyName = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("NPCName"), GL2ShowNPCName, TEXT("Option.ini")))
        GL2ShowNPCName = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("GroupName"), bOtherPC, TEXT("Option.ini")))
        bOtherPC = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("PledgeMemberName"), GL2ShowPledgeMemberName,
                          TEXT("Option.ini")))
        GL2ShowPledgeMemberName = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("PartyMemberName"), GL2ShowPartyMemberName,
                          TEXT("Option.ini")))
        GL2ShowPartyMemberName = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("OtherPCName"), GL2ShowOtherPCName,
                          TEXT("Option.ini")))
        GL2ShowOtherPCName = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("EnterChatting"), GIsEnterChatting,
                          TEXT("Option.ini")))
        GIsEnterChatting = 0;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("OldChatting"), GIsOldChat, TEXT("Option.ini")))
        GIsOldChat = 1;
    if (!GConfig->GetInt(TEXT("Game"), TEXT("PartyLooting"), GL2PartyLooting, TEXT("Option.ini")))
        GL2PartyLooting = 0;
    if (!GConfig->GetInt(TEXT("Game"), TEXT("ScreenShotQuality"), GL2ScreenShotQuality, TEXT("Option.ini")))
        GL2ScreenShotQuality = 0;
    NameBoxes[NAMEBOX_OtherPC]->bChecked = bOtherPC;
    NameBoxes[NAMEBOX_Player]->bChecked = GL2ShowMyName;
    NameBoxes[NAMEBOX_Monster]->bChecked = GL2ShowNPCName;
    NameBoxes[NAMEBOX_Clan]->bChecked = GL2ShowPledgeMemberName;
    NameBoxes[NAMEBOX_Party]->bChecked = GL2ShowPartyMemberName;
    NameBoxes[NAMEBOX_General]->bChecked = GL2ShowOtherPCName;
    OnOtherPCClicked();
    if (!bOtherPC)
    {
        GL2ShowOtherPCName = 0;
        GL2ShowPartyMemberName = 0;
        GL2ShowPledgeMemberName = 0;
    }
    TrackingBox->bChecked = GL2UseAutoTrackingPawn;
    LanguageCombo->SetSelected(bNative ? 0 : 1);
    if (!GConfig->GetBool(TEXT("Game"), TEXT("UseJoystick"), GL2UsingJoystick, TEXT("Option.ini")))
        GL2UsingJoystick = 0;
    if (!GL2CanUseJoystick)
    {
        if (GamePadBox)
            GamePadBox->bChecked = 0;
        if (GamePadBox)
            GamePadBox->bDisabled = 1;
    }
    if (!GL2UsingJoystick)
    {
        if (GamePadBox)
            GamePadBox->bChecked = 0;
        GDesktop->ShortCutWnd->PadButton->HideWindow();
    }
    else
    {
        if (GamePadBox)
            GamePadBox->bChecked = 1;
        GDesktop->ShortCutWnd->PadButton->ShowWindow();
    }
    if (!GIsEnterChatting)
        EnterChatBox->bChecked = 0;
    else
        EnterChatBox->bChecked = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("TransparencyMode"), GIsTransparencyMode, TEXT("Option.ini")))
        GIsTransparencyMode = 1;
    TransparencyBox->bChecked = GIsTransparencyMode;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("ArrowMode"), GIsArrowMode, TEXT("Option.ini")))
        GIsArrowMode = 1;
    ArrowBox->bChecked = GIsArrowMode;
    if (GL2UseKeyCrypt && KeyCryptGetLoadStartup())
        KeyCryptBox->bChecked = 1;
    else
        KeyCryptBox->bChecked = 0;
    if (GL2UseKeyCrypt && KeyCryptIsAdmin())
        KeyCryptBox->bDisabled = 0;
    else
        KeyCryptBox->bDisabled = 1;
}
NW_HOOK_METHOD(0x100ba610, NCGameOptionWnd, LoadSettings)

// 0x100baab0 - C3: GameData.Load takes an argument (0); the transparency and arrow modes, the party loot rule only
// while the combo is enabled (else it goes back to the console's rule, +0x39C8) and the screen shot quality are saved
void NCGameOptionWnd::Apply()
{
    INT Language = LanguageCombo->Selected;
    if (Language == GIsNative)
    {
        GIsNative = !GIsNative;
        GL2GameData.Load(0);
        GConsole->ReloadGameTexts();
    }
    UBOOL bSavedNative = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("IsNative"), bSavedNative, TEXT("Option.ini")))
        bSavedNative = 1;
    if (Language == 0)
        GIsNative = 1;
    else if (Language == 1)
        GIsNative = 0;
    if (bSavedNative != GIsNative)
    {
        GL2GameData.Load(0);
        GConsole->ReloadGameTexts();
    }
    GL2ShowMyName = NameBoxes[NAMEBOX_Player]->bChecked;
    GL2ShowNPCName = NameBoxes[NAMEBOX_Monster]->bChecked;
    if (NameBoxes[NAMEBOX_OtherPC]->bChecked)
    {
        GL2ShowPledgeMemberName = NameBoxes[NAMEBOX_Clan]->bChecked;
        GL2ShowPartyMemberName = NameBoxes[NAMEBOX_Party]->bChecked;
        GL2ShowOtherPCName = NameBoxes[NAMEBOX_General]->bChecked;
    }
    else
    {
        GL2ShowOtherPCName = 0;
        GL2ShowPartyMemberName = 0;
        GL2ShowPledgeMemberName = 0;
    }
    GIsTransparencyMode = TransparencyBox->bChecked;
    GIsArrowMode = ArrowBox->bChecked;
    GL2UseAutoTrackingPawn = TrackingBox->bChecked;
    GIsEnterChatting = EnterChatBox->bChecked;
    GIsOldChat = OldChatBox->bChecked;
    GConfig->SetBool(TEXT("Game"), TEXT("IsNative"), GIsNative, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("MyName"), GL2ShowMyName, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("NPCName"), GL2ShowNPCName, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("GroupName"), NameBoxes[NAMEBOX_OtherPC]->bChecked,
                     TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("PledgeMemberName"), NameBoxes[NAMEBOX_Clan]->bChecked,
                     TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("PartyMemberName"), NameBoxes[NAMEBOX_Party]->bChecked,
                     TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("OtherPCName"), NameBoxes[NAMEBOX_General]->bChecked,
                     TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("AutoTrackingPawn"), GL2UseAutoTrackingPawn,
                     TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("EnterChatting"), GIsEnterChatting, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("OldChatting"), GIsOldChat, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("TransparencyMode"), GIsTransparencyMode, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("ArrowMode"), GIsArrowMode, TEXT("Option.ini"));
    // the console's party item distribution (its +0x39C8 in this client; the header's field is 4 bytes above)
    INT& PartyItemRule = ::nw::Field<INT>(GConsole, 0x39C8);
    if (PartyLootCombo->IsEnabled())
    {
        PartyItemRule = PartyLootCombo->Selected;
        GL2PartyLooting = PartyLootCombo->Selected;
        GConfig->SetInt(TEXT("Game"), TEXT("PartyLooting"), GL2PartyLooting, TEXT("Option.ini"));
    }
    else
        PartyLootCombo->SetSelected(PartyItemRule);
    GL2ScreenShotQuality = ScreenShotCombo->Selected;
    GConfig->SetInt(TEXT("Game"), TEXT("ScreenShotQuality"), GL2ScreenShotQuality, TEXT("Option.ini"));
    if (GL2CanUseJoystick)
    {
        if (!GamePadBox->bChecked)
        {
            GL2UsingJoystick = 0;
            GDesktop->ShortCutWnd->PadButton->HideWindow();
            NCShortCutWnd* Bar = GDesktop->ShortCutWnd;
            if (Bar->bPadMode == 1)
            {
                Bar->bPadMode = 0;
                GDesktop->ShortCutWnd->RotateButton->ShowWindow();
                GDesktop->ShortCutWnd->UpdateLayout();
            }
        }
        else
        {
            GL2UsingJoystick = 1;
            GDesktop->ShortCutWnd->PadButton->ShowWindow();
        }
        GConfig->SetBool(TEXT("Game"), TEXT("UseJoystick"), GL2UsingJoystick, TEXT("Option.ini"));
    }
    if (GL2UseKeyCrypt && KeyCryptBox->bChecked)
        KeyCryptLoadAtStartup(1);   // C2: the key encryption driver at startup
    else
        KeyCryptLoadAtStartup(0);
}
NW_HOOK_METHOD(0x100baab0, NCGameOptionWnd, Apply)

// 0x100ba530 - C3: the new boxes too
void NCGameOptionWnd::ClearItems()
{
    for (INT i = 0; i < NAMEBOX_MAX; i++)
        NameBoxes[i]->bChecked = 0;
    LanguageCombo->ClearItems();
    EnterChatBox->bChecked = 0;
    OldChatBox->bChecked = 0;
    TrackingBox->bChecked = 0;
    PartyLootCombo->ClearItems();
    TransparencyBox->bChecked = 0;
    ArrowBox->bChecked = 0;
    ScreenShotCombo->ClearItems();
    if (GL2CanUseJoystick)
        GamePadBox->bChecked = 0;
    KeyCryptBox->bChecked = 0;
}
NW_HOOK_METHOD(0x100ba530, NCGameOptionWnd, ClearItems)

// 0x100bba80 - C3: the language is read from localization.ini (FontInfo.ini in C2), the party loot box has five rules
// (sysstrings 487, 488, 798 .. 800) and the screen shot box four (980 .. 983)
void NCGameOptionWnd::InitItems()
{
    INT ClientLanguage = 0;
    if (!GConfig->GetInt(TEXT("LanguageSet"), TEXT("Language"), ClientLanguage,
                         TEXT("localization.ini")))
    {
        ClientLanguage = 0;
        LanguageCombo->AddItem(TEXT("Korean"));
        LanguageCombo->AddItem(TEXT("English"));
        if (!GEnableEngSelection)
            LanguageCombo->EnableWindow(0);
    }
    else if (ClientLanguage == 0)
    {
        LanguageCombo->AddItem(TEXT("Korean"));
        LanguageCombo->AddItem(TEXT("English"));
        if (!GEnableEngSelection)
            LanguageCombo->EnableWindow(0);
    }
    else if (ClientLanguage == 1)
        LanguageCombo->AddItem(TEXT("English"));
    else
    {
        const TCHAR* Own = NULL;
        if (ClientLanguage == 2)
            Own = TEXT("Japanese");
        else if (ClientLanguage == 3)
            Own = TEXT("Chinese(Taiwan)");
        else if (ClientLanguage == 4)
            Own = TEXT("China");
        else if (ClientLanguage == 5)
            Own = TEXT("Thai");
        if (Own)
        {
            LanguageCombo->AddItem(Own);
            LanguageCombo->AddItem(TEXT("English"));
            if (!GEnableEngSelection)
                LanguageCombo->EnableWindow(0);
        }
    }
    PartyLootCombo->AddItem(*SysString(487));   // Finders Keepers
    PartyLootCombo->AddItem(*SysString(488));   // Random
    PartyLootCombo->AddItem(*SysString(798));
    PartyLootCombo->AddItem(*SysString(799));
    PartyLootCombo->AddItem(*SysString(800));
    ScreenShotCombo->AddItem(*SysString(980));
    ScreenShotCombo->AddItem(*SysString(981));
    ScreenShotCombo->AddItem(*SysString(982));
    ScreenShotCombo->AddItem(*SysString(983));
    LoadSettings();
    if (EnterChatBox)
        EnterChatBox->bChecked = GIsEnterChatting != 0;
    if (OldChatBox)
        OldChatBox->bChecked = GIsOldChat != 0;
    PartyLootCombo->SetSelected(GL2PartyLooting);
    ScreenShotCombo->SetSelected(GL2ScreenShotQuality);
    ::nw::Field<INT>(GConsole, 0x39C8) = PartyLootCombo->Selected;   // the console's party item distribution
}
NW_HOOK_METHOD(0x100bba80, NCGameOptionWnd, InitItems)

NW_CHECK_VTABLE(NCGameOptionWnd, 0x10193248, 70)
