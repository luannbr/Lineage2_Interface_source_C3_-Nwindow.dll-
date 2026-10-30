/*=============================================================================
    NCPledgeWnd.cpp - the clan page. Stock: 0x10033da0 .. 0x10034053, 0x10034060 .. 0x10034203,
    0x10034210 .. 0x100342b9, 0x10034f50 .. 0x1003504f, 0x10035050 .. 0x100360d9,
    0x100360e0 .. 0x100364b5, 0x10036dc0 .. 0x10036e14 (the rest of the range is
    NCActionWnd's and NCActionItemWnd's).
=============================================================================*/
#include "nwindow/NCPledgeWnd.h"
#include "nwindow/NCListCtrl.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleFns.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCEmpowerWnd.h"
#include "nwindow/NColumnInfoC3.h"

#include <wchar.h>

// systemmsg lines
enum
{
    MSG_TargetNotFound = 50,          // "Target can not be found."
    MSG_ConfirmWithdraw = 68,         // "Withdraw from the $s1 Clan. Do you want to continue?"
    MSG_ConfirmExpel = 69,            // "Expel $s1 from the clan. Do you want to continue?"
    MSG_SelectInvitee = 186,          // "Select user to invite to your clan."
    MSG_EnterWarTarget = 206,         // "Enter name of clan to declare war on."
    MSG_CrestFormat = 211,            // "Can only register 16*12 sized bmp files of 256 colors."
    MSG_LeaderCannotWithdraw = 239,   // "The clan leader cannot withdraw."
    MSG_EnterSurrenderTarget = 252,   // "Enter the name of clan to surrender to."
    MSG_EnterEndWarTarget = 253,      // "Enter the name of clan to request end of war."
    MSG_EnterName = 256,              // "Enter name."
    MSG_SelectMember = 259,           // "Select clan members from list."
    MSG_ConfirmRemoveTitle = 1181,    // (0x49D; text not read) the dialog 0x48 of OnRemoveTitle
    MSG_ConfirmRemoveCrest = 1182,    // (0x49E; text not read) the dialog 0x49 of OnRemoveCrest
    MSG_NoPowerTarget = 145,          // (0x91; text not read) OnNewButton2's dialog for a selected member without an id
};

enum { DIALOG_SystemMessage = 0x1F };   // NDialogInfo::ID: a message, OK only

// sysstring lines
enum
{
    STR_Name = 50,
    STR_Lv = 88,
    STR_None = 27,                    // the castle / hideout line of a clan with neither (name ours)
    STR_NoName = 431,                 // the clan's name line when it has none (name ours)
    STR_Cls = 391,
    STR_StatusC3A = 341,              // the status text when the clan's Status is 3 (name ours)
    STR_StatusC3B = 340,              // the status text otherwise, when Unknown704 is set (name ours)
    STR_StatusC3C = 894,              // the status text otherwise, when Unknown704 is clear (name ours)
    STR_Leader = 342,
    STR_Base = 343,
    STR_HideoutExists = 344,
    STR_CastleExists = 345,
    STR_Status = 346,
    STR_On = 347,
    STR_Out = 348,
};

// dialogs of the buttons (NCDialogBox::OnOK / OnCancel act on the ID)
enum
{
    DIALOG_GiveTitle = 0xF,
    DIALOG_RegisterCrest = 0x10,
    DIALOG_DeclareWar = 0x12,
    DIALOG_EndWar = 0x14,
    DIALOG_Surrender = 0x17,
    DIALOG_Expel = 0x18,
    DIALOG_SurrenderPersonally = 0x19,
    DIALOG_Withdraw = 0x1A,
    DIALOG_RemoveTitle = 0x48,
    DIALOG_RemoveCrest = 0x49,
};

enum { CLAN_TEXT_COLOR = 0xFFB09B79, CLAN_LABEL_COLOR = 0xFFA3A3A3 };

// the member list (the stock builds it with `new`: the object comes from StaticAllocateObject, then the constructor of
// five arguments, 0x10021080, ret 0x14)
static NCListCtrl* NewMemberList()
{
    return new NCListCtrl(1000, 7, 4, 12, 0);
}

// one column of the member list (repeated inline in the stock OnCreate, each with its own record)
static FORCEINLINE void AddMemberColumn(NCListCtrl* List, NColumnInfoC3& Info, INT Width, INT TitleID)
{
    Info.Width = Width;
    Info.Height = 19;
    Info.SetTitleID(TitleID);
    Info.SetNormalTex(TEXT("L2UI_CH3.ListCtrl.tab1"), 0);
    Info.SetPushedTex(TEXT("L2UI_CH3.ListCtrl.tab1_down"), 0);
    Info.SetNormalTex(TEXT("L2UI_CH3.ListCtrl.tab2"), 1);
    Info.SetPushedTex(TEXT("L2UI_CH3.ListCtrl.tab2_down"), 1);
    Info.SetNormalTex(TEXT("L2UI_CH3.ListCtrl.tab3"), 2);
    Info.SetPushedTex(TEXT("L2UI_CH3.ListCtrl.tab3_down"), 2);
    List->AddColumn(0, reinterpret_cast<NListColumnInfo*>(&Info));
}

// one button of the page (repeated inline in the stock OnCreate)
static FORCEINLINE NCButton* MakeButton(NCWnd* Parent, INT ID, INT X, INT Y, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    Button->ID = ID;
    Button->SetPosition(X, Y, 76, 23);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetTextID(TextID);
    return Button;
}

// one child of OnDestroy (repeated inline in the stock code)
template <class T> static void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x1004ae10 - the back and crest textures and the member list empty, the clan record built (the stock calls the
// record's out-of-line constructor, 0x100475a0, which the nconsole lane hooks)
NCPledgeWnd::NCPledgeWnd()
{
    BackTex = NULL;
    CrestTex = NULL;
    MemberList = NULL;
}
NW_HOOK_CTOR(0x1004AE10, NCPledgeWnd)


// 0x10046cb0 (vslot 25) - C3: two more buttons (0xF01A, 0xF01B: slots 80 and 81)
INT NCPledgeWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_PLEDGE_INVITE)
            return OnInvite() != 0;
        if ((INT)wParam == NID_PLEDGE_TITLE)
            return OnGiveTitle() != 0;
        if ((INT)wParam == NID_PLEDGE_CREST)
            return OnSetCrest() != 0;
        if ((INT)wParam == NID_PLEDGE_WAR)
            return OnDeclareWar() != 0;
        if ((INT)wParam == NID_PLEDGE_TRUCE)
            return OnEndWar() != 0;
        if ((INT)wParam == NID_PLEDGE_SURRENDER)
            return OnSurrender() != 0;
        if ((INT)wParam == NID_PLEDGE_EXPEL)
            return OnExpel() != 0;
        if ((INT)wParam == NID_PLEDGE_YIELD)
            return OnSurrenderPersonally() != 0;
        if ((INT)wParam == NID_PLEDGE_LEAVE)
            return OnWithdraw() != 0;
        if ((INT)wParam == NID_PLEDGE_NEW1)
            return OnNewButton1() != 0;
        if ((INT)wParam == NID_PLEDGE_NEW2)
            return OnNewButton2() != 0;
        if ((INT)wParam == NID_PLEDGE_NEW3)
            return OnRemoveTitle() != 0;
        if ((INT)wParam == NID_PLEDGE_NEW4)
            return OnRemoveCrest() != 0;
    }
    else if (Msg == (UINT)NWM_DIALOG_RESULT)
        return OnDialogResult(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10046cb0, NCPledgeWnd, WindowProc)

// 0x10049610 (vslot 37) - C3: the member list has four columns (name, level, class, online) and the page thirteen
// buttons in a 3-wide grid; the class icons are loaded here
INT NCPledgeWnd::OnCreate()
{
    NColumnInfoC3 NameColumn, LevelColumn, ClassColumn, StatusColumn;   // four records, as in the stock frame
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.BloodHoodWnd.BloodHood_Back"));

    MemberList = NewMemberList();
    MemberList->SetPosition(8, 62, 240, 137);
    CreateChild(MemberList, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    MemberList->SetTextOffset(0, 3);
    AddMemberColumn(MemberList, NameColumn, 124, STR_Name);
    AddMemberColumn(MemberList, LevelColumn, 32, STR_Lv);
    AddMemberColumn(MemberList, ClassColumn, 32, STR_Cls);
    AddMemberColumn(MemberList, StatusColumn, 52, STR_Status);

    InviteButton = MakeButton(this, NID_PLEDGE_INVITE, 11, 202, 330);        // "Invite"
    TitleButton = MakeButton(this, NID_PLEDGE_TITLE, 91, 202, 331);          // "Title"
    Button714 = MakeButton(this, NID_PLEDGE_NEW3, 171, 202, 842);            // new in C3
    ExpelButton = MakeButton(this, NID_PLEDGE_EXPEL, 11, 227, 338);          // "Dismiss"
    CrestButton = MakeButton(this, NID_PLEDGE_CREST, 91, 227, 332);          // "Set Crest"
    Button72C = MakeButton(this, NID_PLEDGE_NEW4, 171, 227, 843);            // new in C3
    NewButton6F4 = MakeButton(this, NID_PLEDGE_NEW1, 11, 252, 666);          // new in C2
    NewButton6F8 = MakeButton(this, NID_PLEDGE_NEW2, 91, 252, 667);          // new in C2
    LeaveButton = MakeButton(this, NID_PLEDGE_LEAVE, 171, 252, 337);         // "Leave"
    WarButton = MakeButton(this, NID_PLEDGE_WAR, 11, 281, 333);              // "Battle"
    TruceButton = MakeButton(this, NID_PLEDGE_TRUCE, 91, 281, 334);          // "Truce"
    SurrenderButton = MakeButton(this, NID_PLEDGE_SURRENDER, 11, 306, 335);  // "Surrender"
    YieldButton = MakeButton(this, NID_PLEDGE_YIELD, 91, 306, 336);          // "Yield"

    ClassIcons[0] = FindTexture(TEXT("L2UI_CH3.PartyWnd.party_styleicon1"));
    ClassIcons[1] = FindTexture(TEXT("L2UI_CH3.PartyWnd.party_styleicon2"));
    ClassIcons[2] = FindTexture(TEXT("L2UI_CH3.PartyWnd.party_styleicon3"));
    ClassIcons[3] = FindTexture(TEXT("L2UI_CH3.PartyWnd.party_styleicon4"));
    ClassIcons[4] = FindTexture(TEXT("L2UI_CH3.PartyWnd.party_styleicon5"));
    ClassIcons[5] = FindTexture(TEXT("L2UI_CH3.PartyWnd.party_styleicon6"));
    ClassIcons[6] = FindTexture(TEXT("L2UI_CH3.PartyWnd.party_styleicon7"));
    ClassIcons[7] = FindTexture(TEXT("L2UI_CH3.PartyWnd.party_styleicon1_1"));
    ClassIcons[8] = FindTexture(TEXT("L2UI_CH3.PartyWnd.party_styleicon1_2"));
    return 1;
}
NW_HOOK_METHOD(0x10049610, NCPledgeWnd, OnCreate)

// 0x10048020 (vslot 38) - C3 guards it; the thirteen buttons go in the order of the stock code
INT NCPledgeWnd::OnDestroy()
{
    guard(NCPledgeWnd::OnDestroy);
    DestroyChild(this, MemberList);
    DestroyChild(this, InviteButton);
    DestroyChild(this, TitleButton);
    DestroyChild(this, CrestButton);
    DestroyChild(this, WarButton);
    DestroyChild(this, TruceButton);
    DestroyChild(this, SurrenderButton);
    DestroyChild(this, ExpelButton);
    DestroyChild(this, YieldButton);
    DestroyChild(this, LeaveButton);
    DestroyChild(this, NewButton6F4);
    DestroyChild(this, NewButton6F8);
    DestroyChild(this, Button72C);
    DestroyChild(this, Button714);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10048020, NCPledgeWnd, OnDestroy)

// a clan privilege byte of the player (C2: three bytes at User +0x1EE, +0x1EF, +0x1F0, new; the
// engine header only has them in a filler): invite, title, crest
static FORCEINLINE UBOOL PledgeRight(User* Me, INT Offset)
{
    return *((const BYTE*)Me + Offset) != 0;
}

// 0x10046eb0 (vslot 44) - the list scrolls back in range; the leader gets everything but Leave and Yield (and the
// three new ones), the others only Leave; a member holding a right gets that button; without a clan nothing is on
void NCPledgeWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (MemberList)
        MemberList->UpdateScrollRange();
    User* Me = GConsole->GetMyUser();
    if (!Me)
        return;
    UBOOL bLeader = (Me->StatusFlags >> 6) & 1;
    if (!bLeader)
    {
        InviteButton->EnableWindow(0);
        TitleButton->EnableWindow(0);
        CrestButton->EnableWindow(0);
        WarButton->EnableWindow(0);
        TruceButton->EnableWindow(0);
        SurrenderButton->EnableWindow(0);
        ExpelButton->EnableWindow(0);
        LeaveButton->EnableWindow(1);
    }
    else
    {
        InviteButton->EnableWindow(1);
        TitleButton->EnableWindow(1);
        CrestButton->EnableWindow(1);
        WarButton->EnableWindow(1);
        TruceButton->EnableWindow(1);
        SurrenderButton->EnableWindow(1);
        ExpelButton->EnableWindow(1);
        LeaveButton->EnableWindow(0);
        YieldButton->EnableWindow(0);
        Button714->EnableWindow(1);
        Button72C->EnableWindow(1);
        NewButton6F4->EnableWindow(1);
        NewButton6F8->EnableWindow(1);
    }
    if (PledgeRight(Me, 0x218))
        InviteButton->EnableWindow(1);
    if (PledgeRight(Me, 0x219))
        TitleButton->EnableWindow(1);
    if (PledgeRight(Me, 0x21A))
        CrestButton->EnableWindow(1);
    if (Me->PledgeID <= 0)
    {
        InviteButton->EnableWindow(0);
        TitleButton->EnableWindow(0);
        CrestButton->EnableWindow(0);
        WarButton->EnableWindow(0);
        TruceButton->EnableWindow(0);
        SurrenderButton->EnableWindow(0);
        ExpelButton->EnableWindow(0);
        LeaveButton->EnableWindow(0);
        Button714->EnableWindow(0);
        YieldButton->EnableWindow(0);
        Button72C->EnableWindow(0);
        NewButton6F4->EnableWindow(0);
        NewButton6F8->EnableWindow(0);
    }
}
NW_HOOK_METHOD(0x10046eb0, NCPledgeWnd, ShowWindow)

// 0x10047bc0 (vslot 56) - C3: the page is 256 x 335 and drawn from the top (no clip above it); the clan's name has a
// fallback text, the leader's name is followed by a status text and the level is shown only when there is a name; the
// castle and the hideout are named by their ids (Unknown6FC, Unknown700)
INT NCPledgeWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPledgeWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 0, 256, 335, 0, 0, 256, 335, BackTex, 0xFF, 1);

    const TCHAR* ClanName = Info.Name;
    if (!ClanName || !*ClanName)
        ClanName = *SysString(STR_NoName);
    Device->DrawText(19, 10, CLAN_TEXT_COLOR, ClanName, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);

    if (*SysString(STR_Leader))
        Device->DrawText(19, 26, CLAN_LABEL_COLOR, *SysString(STR_Leader), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    const TCHAR* Leader = Info.LeaderName;
    if (Leader)
    {
        if (*Leader)
            Device->DrawText(61, 26, CLAN_TEXT_COLOR, Leader, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        if (*Leader)
        {
            const TCHAR* StatusText;
            if (Info.Status == 3)
                StatusText = *SysString(STR_StatusC3A);
            else if (Unknown704 == 0)
                StatusText = *SysString(STR_StatusC3C);
            else
                StatusText = *SysString(STR_StatusC3B);
            Device->DrawText(192, 10, CLAN_TEXT_COLOR, StatusText, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
    }

    if (*SysString(STR_Lv))
        Device->DrawText(192, 26, CLAN_LABEL_COLOR, *SysString(STR_Lv), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    const TCHAR* ClanNameAgain = Info.Name;
    if (ClanNameAgain && *ClanNameAgain)
    {
        TCHAR Number[10];
        appSprintf(Number, TEXT("%d"), Info.Level);
        Device->DrawText(208, 26, CLAN_TEXT_COLOR, Number, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }

    if (*SysString(STR_Base))
        Device->DrawText(19, 42, CLAN_LABEL_COLOR, *SysString(STR_Base), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (Leader && *Leader)
    {
        if (Info.bHasHideout)
        {
            Device->DrawText(61, 42, CLAN_TEXT_COLOR, GConsole->GetCastleName(Unknown700), 0, 0, 0, L2FONT_Normal,
                             0, 0, 0, 0, 0, 0);
            if (Info.bHasCastle)
            {
                SIZE Size;
                NCGDevice::GetTextExtent(&Size, GConsole->GetCastleName(Unknown700), 0, 0, 0, 0, 0);
                Device->DrawText(Size.cx + 61, 42, CLAN_TEXT_COLOR, TEXT(","), 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                                 0, 0, 0);
                Device->DrawText(Size.cx + 65, 42, CLAN_TEXT_COLOR, GConsole->GetCastleName(Unknown6FC), 0, 0, 0,
                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            }
        }
        else
        {
            const TCHAR* Base;
            if (Info.bHasCastle)
                Base = GConsole->GetCastleName(Unknown6FC);
            else
                Base = *SysString(STR_None);
            Device->DrawText(61, 42, CLAN_TEXT_COLOR, Base, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10047bc0, NCPledgeWnd, OnPaint)

// 0x10047350 (vslot 68) - the target, when it is a player
INT NCPledgeWnd::OnInvite()
{
    User* Target = GConsole->GetSelectedActor();
    if (Target && !Target->Unknown8)
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)Target->Pawn->CreatureID);
        // 0x1005D560 -> NetHandler +0x1D0 (Engine RequestJoinPledge); the wrapper named RequestJoinPledge is +0x1D4
        GConsole->RequestGiveNickName(Params);
    }
    else
        GConsole->ShowSystemMessage(MSG_SelectInvitee, CLAN_TEXT_COLOR);
    return 1;
}
NW_HOOK_METHOD(0x10047350, NCPledgeWnd, OnInvite)

// 0x100489b0 (vslot 69) - the file name is typed in the box
INT NCPledgeWnd::OnSetCrest()
{
    NDialogInfo Dialog;
    Dialog.ID = DIALOG_RegisterCrest;
    Dialog.bMainBox = 1;
    Dialog.bInput = 0;
    Dialog.Type = 0;
    Dialog.SetSystemMessage(MSG_CrestFormat);
    GConsole->ShowDialog(&Dialog);
    return 1;
}
NW_HOOK_METHOD(0x100489b0, NCPledgeWnd, OnSetCrest)

// 0x10048a50 (vslot 70)
INT NCPledgeWnd::OnDeclareWar()
{
    NDialogInfo Dialog;
    Dialog.ID = DIALOG_DeclareWar;
    Dialog.bMainBox = 1;
    Dialog.bInput = 1;
    Dialog.Type = 0;
    Dialog.SetSystemMessage(MSG_EnterWarTarget);
    GConsole->ShowDialog(&Dialog);
    return 1;
}
NW_HOOK_METHOD(0x10048a50, NCPledgeWnd, OnDeclareWar)

// 0x10048af0 (vslot 71)
INT NCPledgeWnd::OnEndWar()
{
    NDialogInfo Dialog;
    Dialog.ID = DIALOG_EndWar;
    Dialog.bMainBox = 1;
    Dialog.bInput = 1;
    Dialog.Type = 0;
    Dialog.SetSystemMessage(MSG_EnterEndWarTarget);
    GConsole->ShowDialog(&Dialog);
    return 1;
}
NW_HOOK_METHOD(0x10048af0, NCPledgeWnd, OnEndWar)

// 0x10048b90 (vslot 72)
INT NCPledgeWnd::OnSurrender()
{
    NDialogInfo Dialog;
    Dialog.ID = DIALOG_Surrender;
    Dialog.bMainBox = 1;
    Dialog.bInput = 1;
    Dialog.Type = 0;
    Dialog.SetSystemMessage(MSG_EnterSurrenderTarget);
    GConsole->ShowDialog(&Dialog);
    return 1;
}
NW_HOOK_METHOD(0x10048b90, NCPledgeWnd, OnSurrender)

// 0x10048c30 (vslot 73) - the selected member's name goes in the first button's text as well
// as in the parameter, as in the stock code
INT NCPledgeWnd::OnExpel()
{
    NListRow* Row = MemberList->GetSelectedRow();
    if (!Row)
    {
        GConsole->ShowSystemMessage(MSG_SelectMember, CLAN_TEXT_COLOR);
        return 1;
    }
    const TCHAR* Member = Row->Cells->Text;
    if (Member && *Member)
    {
        NDialogInfo Dialog;
        Dialog.ID = DIALOG_Expel;
        Dialog.bMainBox = 1;
        Dialog.bInput = 0;
        Dialog.SetSystemMessage(MSG_ConfirmExpel);
        Dialog.SetButtons(Row->Cells->Text, NULL);
        Dialog.SetParamText(Row->Cells->Text);
        GConsole->ShowDialog(&Dialog);
    }
    return 1;
}
NW_HOOK_METHOD(0x10048c30, NCPledgeWnd, OnExpel)

// 0x10048d20 (vslot 74) - the same message as OnSurrender
INT NCPledgeWnd::OnSurrenderPersonally()
{
    NDialogInfo Dialog;
    Dialog.ID = DIALOG_SurrenderPersonally;
    Dialog.bMainBox = 1;
    Dialog.bInput = 1;
    Dialog.Type = 0;
    Dialog.SetSystemMessage(MSG_EnterSurrenderTarget);
    GConsole->ShowDialog(&Dialog);
    return 1;
}
NW_HOOK_METHOD(0x10048d20, NCPledgeWnd, OnSurrenderPersonally)

// 0x10048dc0 (vslot 75) - the clan's name goes in the first button's text
INT NCPledgeWnd::OnWithdraw()
{
    User* Me = GConsole->GetMyUser();
    if (Me && !((Me->StatusFlags >> 6) & 1))
    {
        NDialogInfo Dialog;
        Dialog.ID = DIALOG_Withdraw;
        Dialog.bMainBox = 1;
        Dialog.bInput = 0;
        Dialog.SetSystemMessage(MSG_ConfirmWithdraw);
        Dialog.SetButtons(Info.Name, NULL);
        GConsole->ShowDialog(&Dialog);
        return 1;
    }
    GConsole->ShowSystemMessage(MSG_LeaderCannotWithdraw, CLAN_TEXT_COLOR);
    return 1;
}
NW_HOOK_METHOD(0x10048dc0, NCPledgeWnd, OnWithdraw)

// 0x10048790 (vslot 76) - the title of the target, when it is a player
INT NCPledgeWnd::OnGiveTitle()
{
    User* Target = GConsole->GetSelectedActor();
    if (Target && !Target->Unknown8)
    {
        NDialogInfo Dialog;
        Dialog.ID = DIALOG_GiveTitle;
        Dialog.bMainBox = 1;
        Dialog.bInput = 1;
        Dialog.Type = 0;
        Dialog.SetSystemMessage(MSG_EnterName);
        GConsole->ShowDialog(&Dialog);
        return 1;
    }
    GConsole->ShowSystemMessage(MSG_TargetNotFound, CLAN_TEXT_COLOR);
    return 1;
}
NW_HOOK_METHOD(0x10048790, NCPledgeWnd, OnGiveTitle)

// the record the console hands over is 0x638 bytes in this client (NConsoleWndEvents.cpp builds it); the
// header's NPledgeInfo stops at +0x62C, the three dwords after it are read here
#define PLEDGE_EXTRA(Info, Off) nw::Field<INT>((Info), (Off))

// 0x10047170 - the status text, UnknownE0 and UnknownE4 are left out; the names only when given
void NCPledgeWnd::SetPledgeInfo(NPledgeInfo* InInfo)
{
    Info.PledgeID = InInfo->PledgeID;
    if (*InInfo->Name)
        appStrcpy(Info.Name, InInfo->Name);
    const TCHAR* Leader = InInfo->LeaderName;
    if (Leader && *Leader)
        appStrcpy(Info.LeaderName, Leader);
    Info.CrestID = InInfo->CrestID;
    Info.Level = InInfo->Level;
    Info.bHasCastle = InInfo->bHasCastle;
    Info.bHasHideout = InInfo->bHasHideout;
    Info.Unknown94 = InInfo->Unknown94;
    Info.Unknown98 = InInfo->Unknown98;
    Info.Status = InInfo->Status;
    Info.UnknownE8 = InInfo->UnknownE8;
    Unknown6FC = PLEDGE_EXTRA(InInfo, 0x62C);
    Unknown700 = PLEDGE_EXTRA(InInfo, 0x630);
    Unknown704 = PLEDGE_EXTRA(InInfo, 0x634);
}
NW_HOOK_METHOD(0x10047170, NCPledgeWnd, SetPledgeInfo)

// 0x10047250 - the same without the ID and the names
void NCPledgeWnd::UpdatePledgeInfo(NPledgeInfo* InInfo)
{
    Info.CrestID = InInfo->CrestID;
    Info.Level = InInfo->Level;
    Info.bHasCastle = InInfo->bHasCastle;
    Info.bHasHideout = InInfo->bHasHideout;
    Info.Unknown94 = InInfo->Unknown94;
    Info.Unknown98 = InInfo->Unknown98;
    Info.Status = InInfo->Status;
    Info.UnknownE8 = InInfo->UnknownE8;
    Unknown6FC = PLEDGE_EXTRA(InInfo, 0x62C);
    Unknown700 = PLEDGE_EXTRA(InInfo, 0x630);
    Unknown704 = PLEDGE_EXTRA(InInfo, 0x634);
}
NW_HOOK_METHOD(0x10047250, NCPledgeWnd, UpdatePledgeInfo)

// 0x100472e0
void NCPledgeWnd::ClearMembers()
{
    MemberList->ClearRows();
}
NW_HOOK_METHOD(0x100472e0, NCPledgeWnd, ClearMembers)

// 0x100472f0 - UnknownE8 is kept
void NCPledgeWnd::ClearInfo()
{
    Info.Name[0] = 0;
    Info.LeaderName[0] = 0;
    Info.StatusText[0] = 0;
    Info.PledgeID = 0;
    Info.CrestID = 0;
    Info.UnknownE4 = 0;
    Info.Level = 0;
    Info.bHasCastle = 0;
    Info.bHasHideout = 0;
    Info.Unknown94 = 0;
    Info.Unknown98 = 0;
    Info.Status = 0;
    Info.UnknownE0 = 0;
}
NW_HOOK_METHOD(0x100472f0, NCPledgeWnd, ClearInfo)

// 0x10048300 (the stock caller keeps ecx across ClearInfo, so both are ours)
void NCPledgeWnd::Reset()
{
    ClearInfo();
    ClearMembers();
}
NW_HOOK_METHOD(0x10048300, NCPledgeWnd, Reset)

// the group (0 .. 8) of a class: the stock helper 0x10060400 (ret 4, on nothing of the console's), or -1
static FORCEINLINE INT ClassGroupOf(INT ClassID)
{
    return ((NConsoleLeftFns*)GConsole)->GetClassGroup(ClassID);
}

// the new dword at the end of a list cell (C3: the cell grew by 4 bytes; the page clears it before it sets the
// cell's icon; NListCell has it in a filler)
static FORCEINLINE void ClearCellTail(NListCell* Cell)
{
    ::nw::Field<INT>(Cell, 0x1820) = 0;
}

// 0x10048310 (name ours) - a member: the row with that name takes the level, the class and the online icon; a new row
// is made of the name, the level, those two icons and the params (bOnline, ClassID, -1); the tool tip is filled
// from the row. C3: four cells (C2: three), the class icon is the group of the class, the online text is "1" / "2"
// (the sort key of the column)
void NCPledgeWnd::AddMember(NPledgeMember* Member)
{
    TArray<NListRow*>* Rows = MemberList->GetRows();
    for (INT i = 0; i < Rows->Num(); i++)
    {
        const TCHAR* Name = Member->Name;
        const TCHAR* RowName = (*Rows)(i)->Cells->Text;
        if (Name && *Name && RowName && *RowName && !_wcsicmp(RowName, Name))
        {
            (*Rows)(i)->Cells[1].SetNumber(Member->Level);
            ClearCellTail(&(*Rows)(i)->Cells[2]);
            if (Member->ClassID > -1)
            {
                INT Group = ClassGroupOf(Member->ClassID);
                if (Group >= 0 && Group < 9)
                {
                    (*Rows)(i)->Cells[2].SetNumber(Group);
                    (*Rows)(i)->Cells[2].SetIcon(ClassIcons[Group], 11, 11);
                }
            }
            ClearCellTail(&(*Rows)(i)->Cells[3]);
            if (Member->bOnline > 0)
            {
                (*Rows)(i)->Cells[3].SetText(TEXT("1"));
                (*Rows)(i)->Cells[3].SetIcon(FindTexture(TEXT("L2UI_CH3.BloodHoodWnd.BloodHood_Logon")), 32, 11);
            }
            else
            {
                (*Rows)(i)->Cells[3].SetText(TEXT("2"));
                (*Rows)(i)->Cells[3].SetIcon(FindTexture(TEXT("L2UI_CH3.BloodHoodWnd.BloodHood_Logoff")), 32, 11);
            }
            return;
        }
    }
    NListRow* Row = new NListRow(4);
    Row->Cells[0].SetText(Member->Name);
    Row->Cells[1].SetNumber(Member->Level);
    ClearCellTail(&Row->Cells[2]);
    if (Member->ClassID > -1)
    {
        INT Group = ClassGroupOf(Member->ClassID);
        if (Group >= 0 && Group < 9)
        {
            Row->Cells[2].SetNumber(Group);
            Row->Cells[2].SetIcon(ClassIcons[Group], 11, 11);
        }
    }
    ClearCellTail(&Row->Cells[3]);
    if (Member->bOnline > 0)
    {
        Row->Cells[3].SetText(TEXT("1"));
        Row->Cells[3].SetIcon(FindTexture(TEXT("L2UI_CH3.BloodHoodWnd.BloodHood_Logon")), 32, 11);
    }
    else
    {
        Row->Cells[3].SetText(TEXT("2"));
        Row->Cells[3].SetIcon(FindTexture(TEXT("L2UI_CH3.BloodHoodWnd.BloodHood_Logoff")), 32, 11);
    }
    Row->SetParams(Member->bOnline, Member->ClassID, -1);
    MemberList->AddRow(Row);
    Row->ToolTip.FillMatchRow(Row, 0x8000);
}
NW_HOOK_METHOD(0x10048310, NCPledgeWnd, AddMember)

// 0x100485e0 - members not listed are ignored; an online member's row also gets its class icon, the params and the
// tool tip, an offline one's row only the offline icon (it keeps the old params)
void NCPledgeWnd::UpdateMember(NPledgeMember* Member)
{
    TArray<NListRow*>* Rows = MemberList->GetRows();
    for (INT i = 0; i < Rows->Num(); i++)
    {
        const TCHAR* Name = Member->Name;
        const TCHAR* RowName = (*Rows)(i)->Cells->Text;
        if (Name && *Name && RowName && *RowName && !_wcsicmp(RowName, Name))
        {
            (*Rows)(i)->Cells[1].SetNumber(Member->Level);
            if (Member->bOnline > 0)
            {
                if (Member->ClassID > -1)
                {
                    INT Group = ClassGroupOf(Member->ClassID);
                    if (Group >= 0 && Group < 9)
                    {
                        (*Rows)(i)->Cells[2].SetNumber(Group);
                        (*Rows)(i)->Cells[2].SetIcon(ClassIcons[Group], 11, 11);
                    }
                }
                (*Rows)(i)->Cells[3].SetText(TEXT("1"));
                (*Rows)(i)->Cells[3].SetIcon(FindTexture(TEXT("L2UI_CH3.BloodHoodWnd.BloodHood_Logon")), 32, 11);
                (*Rows)(i)->SetParams(Member->bOnline, Member->ClassID, -1);
                (*Rows)(i)->ToolTip.FillMatchRow((*Rows)(i), 0x8000);
            }
            else
            {
                (*Rows)(i)->Cells[3].SetText(TEXT("2"));
                (*Rows)(i)->Cells[3].SetIcon(FindTexture(TEXT("L2UI_CH3.BloodHoodWnd.BloodHood_Logoff")), 32, 11);
            }
            return;
        }
    }
}
NW_HOOK_METHOD(0x100485e0, NCPledgeWnd, UpdateMember)

// 0x1004a3e0 - the row leaves the list but is not deleted (the stock code does not free it either)
void NCPledgeWnd::DeleteMember(const TCHAR* Name)
{
    TArray<NListRow*>* Rows = MemberList->GetRows();
    for (INT i = 0; i < Rows->Num(); i++)
    {
        if (Name && !_wcsicmp((*Rows)(i)->Cells->Text, Name))
        {
            Rows->Remove(i, 1);
            return;
        }
    }
}
NW_HOOK_METHOD(0x1004a3e0, NCPledgeWnd, DeleteMember)

// 0x10048ea0 (vslot 77; name ours) - the button 0xF019 (text 666): the player's own privileges in the clan. The server
// is asked for them (RequestPledgePower, slot 404 of this client's console: the player's object id, mode 1 = view),
// the empower window (the desktop's +0xD0) is emptied, told whose they are (the player's id and name: SetMember
// inlined) and opened and focused
INT NCPledgeWnd::OnNewButton1()
{
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        ::nw::VirtualCall<void>(GConsole, 404, Me->ObjectID, 1, 0);
        NCEmpowerWnd* Empower = static_cast<NCEmpowerWnd*>(GDesktop->EmpowerWnd);
        Empower->Reset();
        Empower = static_cast<NCEmpowerWnd*>(GDesktop->EmpowerWnd);
        Empower->MemberID = Me->ObjectID;
        Empower->Mode = 1;
        const TCHAR* MyName = (const TCHAR*)((BYTE*)Me + 0x1C);   // User's name
        if (MyName)
            Empower->Name = MyName;
        GDesktop->EmpowerWnd->ShowWindow();
        GDesktop->EmpowerWnd->SetFocus();
    }
    return 1;
}
NW_HOOK_METHOD(0x10048ea0, NCPledgeWnd, OnNewButton1)

// 0x10048f60 (vslot 78; name ours) - the button 0xF018 (text 667): the privileges of the member selected in the list,
// for the clan leader only (StatusFlags bit 6): an online member's row (its first param, the member's id: 0 =
// offline) asks the server (mode 2 = edit), empties the empower window, tells it whose they are and opens and
// focuses it; with no id, an OK-only dialog box with the system message 0x91. Nothing when no row with a name is
// selected.
INT NCPledgeWnd::OnNewButton2()
{
    if ((GConsole->GetMyUser()->StatusFlags >> 6) & 1)
    {
        NListRow* Row = MemberList->GetSelectedRow();
        if (Row && Row->Cells && Row->Cells->Text[0])
        {
            if ((DWORD)Row->Param1 > 0)
            {
                ::nw::VirtualCall<void>(GConsole, 404, Row->Param1, 2, 0);
                static_cast<NCEmpowerWnd*>(GDesktop->EmpowerWnd)->Reset();
                static_cast<NCEmpowerWnd*>(GDesktop->EmpowerWnd)->SetMember(Row->Param1, Row->Cells->Text, 2);
                GDesktop->EmpowerWnd->ShowWindow();
                GDesktop->EmpowerWnd->SetFocus();
            }
            else
            {
                NDialogInfo Dialog;
                Dialog.ID = DIALOG_SystemMessage;
                Dialog.bMainBox = 1;
                Dialog.bInput = 0;
                Dialog.SetSystemMessage(MSG_NoPowerTarget);
                GConsole->ShowDialog(&Dialog);
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10048f60, NCPledgeWnd, OnNewButton2)

// 0x10048860 (vslot 80; new in C3, name ours) - take the title away: for a player targeted in the world (a record with
// no "not a player" mark), the confirmation 0x48 (system message 1181; its OK asks the server to give the target an
// empty title); else "Target can not be found."
INT NCPledgeWnd::OnRemoveTitle()
{
    User* Target = GConsole->GetSelectedActor();
    if (Target && Target->Unknown8 == 0)
    {
        NDialogInfo Dialog;
        Dialog.ID = DIALOG_RemoveTitle;
        Dialog.bMainBox = 1;
        Dialog.bInput = 0;
        Dialog.SetSystemMessage(MSG_ConfirmRemoveTitle);
        GConsole->ShowDialog(&Dialog);
    }
    else
        GConsole->ShowSystemMessage(MSG_TargetNotFound, CLAN_TEXT_COLOR);
    return 1;
}
NW_HOOK_METHOD(0x10048860, NCPledgeWnd, OnRemoveTitle)

// 0x10048920 (vslot 81; new in C3, name ours) - the confirmation 0x49 (system message 1182; its OK sends the request
// with no crest at all, UL2NetHandler's slot 108 with two zeros: the clan's crest goes)
INT NCPledgeWnd::OnRemoveCrest()
{
    NDialogInfo Dialog;
    Dialog.ID = DIALOG_RemoveCrest;
    Dialog.bMainBox = 1;
    Dialog.bInput = 0;
    Dialog.SetSystemMessage(MSG_ConfirmRemoveCrest);
    GConsole->ShowDialog(&Dialog);
    return 1;
}
NW_HOOK_METHOD(0x10048920, NCPledgeWnd, OnRemoveCrest)

// 0x1004ae80 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x1004ae80, NCPledgeWnd)

NW_CHECK_VTABLE(NCPledgeWnd, 0x10181980, 82)
