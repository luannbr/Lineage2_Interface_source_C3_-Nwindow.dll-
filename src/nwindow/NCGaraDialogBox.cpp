/*=============================================================================
    NCGaraDialogBox.cpp - the desktop's message box (C2 only). Stock: 0x100065f0 .. 0x100067cf,
    0x10006860 .. 0x10006bcf (Cancel), 0x10006c50 .. 0x10006d0d, 0x10006f90 .. 0x10007b1f,
    0x10007b20 .. 0x10008e77 (OK), 0x10008fa0 .. 0x10009091 and 0x10009500 (ShowWindow);
    NDialogInfo keeps 0x100064b0 .. 0x100065c0, 0x10006e60 and 0x100067d0.
=============================================================================*/
#include "nwindow/NCGaraDialogBox.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCProgressCtrl.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCrt.h"
#include "nwindow/MsnUiStock.h"

enum { SYSMSG_ItemFlying = 179 };           // "cannot be dropped here" (name ours): SystemMsg[179]
enum { COLOR_Warning = 0xFFB09B79 };        // the system message color
enum { SOUND_TrashBasket = 4 };             // the table's 4 (name ours)

/*-----------------------------------------------------------------------------
    Construction.
-----------------------------------------------------------------------------*/

// 0x10009f80 - NControl's fields inline (the stock calls NCWnd's constructor), the strings of C3's two sounds
NCGaraDialogBox::NCGaraDialogBox()
{
    BackTex = NULL;
    OKButton = NULL;
    CancelButton = NULL;
    Progress = NULL;
    Owner = NULL;
    bProgress = 0;
    bInput = 0;
    Param1 = 0;
    Param2 = 0;
    DialogID = -1;
}
NW_HOOK_CTOR(0x10009f80, NCGaraDialogBox)


/*-----------------------------------------------------------------------------
    The small virtuals.
-----------------------------------------------------------------------------*/

// 0x10006c40 (vslot 25)
INT NCGaraDialogBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_DIALOG_OK)
            return OnOK() != 0;
        if ((INT)wParam == NID_DIALOG_CANCEL)
            return OnCancel() != 0;
        if ((INT)wParam == NID_DIALOG_KEYPAD)
            return OnKeypad(lParam) != 0;
        if ((INT)wParam == NID_DIALOG_EDIT)
            return OnEditNotify(lParam) != 0;
        if ((INT)wParam == NID_DIALOG_PROGRESS)
            return OnProgressEnd() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10006c40, NCGaraDialogBox, WindowProc)

// 0x10006d20 (vslot 29)
void NCGaraDialogBox::SetFocus()
{
    NCWnd::SetFocus();
    if (bInput)
        EditBox->SetFocus();
}
NW_HOOK_METHOD(0x10006d20, NCGaraDialogBox, SetFocus)

// 0x10006d40 (vslot 59) - Enter answers yes only where something was typed or the box is a notice
INT NCGaraDialogBox::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN)
    {
        if (bInput == 0 && DialogID != 0x1D && DialogID != 0x1F && DialogID != 0x10)
        {
            OnCancel();
            return 1;
        }
        OnOK();
    }
    return 1;
}
NW_HOOK_METHOD(0x10006d40, NCGaraDialogBox, OnKeyDown)

// 0x10006de0 (vslot 45)
void NCGaraDialogBox::HideWindow()
{
    Reset();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x10006de0, NCGaraDialogBox, HideWindow)

// 0x10009d90 (vslot 44) - C3: the two sounds the info brought (names; "None" and empty ones are skipped by the console)
void NCGaraDialogBox::ShowWindow()
{
    NCWnd::ShowWindow();
    GConsole->PlaySoundName(*ShowSoundA);
    GConsole->PlayVoiceName(*ShowSoundB);   // NConsoleWnd::PlayVoiceName 0x10060b00, the second sound's twin
    UpdateLayout();
}
NW_HOOK_METHOD(0x10009d90, NCGaraDialogBox, ShowWindow)

// 0x10006df0 (vslot 71) - Enter in the edit line (1), or on an empty one (0)
INT NCGaraDialogBox::OnEditNotify(INT Code)
{
    if (Code == 1)
    {
        OnOK();
        return 1;
    }
    if (Code == 0)
        OnCancel();
    return 1;
}
NW_HOOK_METHOD(0x10006df0, NCGaraDialogBox, OnEditNotify)

// 0x100073c0 (vslot 73) - time is up
INT NCGaraDialogBox::OnProgressEnd()
{
    OnCancel();
    return 1;
}
NW_HOOK_METHOD(0x100073c0, NCGaraDialogBox, OnProgressEnd)

// 0x100095d0 (vslot 72) - a key of the keypad: the digits are typed into the edit line, key 10 is the
// backspace, key 11 types the number of Unknown238 (the line emptied first), key 12 a zero
INT NCGaraDialogBox::OnKeypad(INT Key)
{
    TCHAR Text[128];
    if (Key >= 0 && Key < 9)
    {
        appSprintf(Text, TEXT("%d"), Key + 1);
        EditBox->InsertText(Text, 1);
        return 1;
    }
    if (Key == 9)
    {
        appSprintf(Text, TEXT("%d"), 0);
        EditBox->InsertText(Text, 1);
        return 1;
    }
    if (Key == 10)
    {
        SendTo(EditBox->Entry, WM_KEYDOWN, VK_BACK, 0);
        return 1;
    }
    if (Key == 11)
    {
        appSprintf(Text, TEXT("%d"), Unknown238);
        EditBox->ClearText();
        EditBox->InsertText(Text, 1);
        return 1;
    }
    if (Key == 12)
    {
        EditBox->ClearText();
        EditBox->InsertText(TEXT("0"), 1);
    }
    return 1;
}
NW_HOOK_METHOD(0x100095d0, NCGaraDialogBox, OnKeypad)

/*-----------------------------------------------------------------------------
    Creation, destruction and paint.
-----------------------------------------------------------------------------*/

// 0x10009920 (vslot 37) - C3: the CH3 skin: the back, the keypad's panel (right of the box, 108 wide) and the warning
// icon; the edit line at (50, 48); the 13 keys of the keypad drawn at X + Width (25 px steps from 7, the three keys of
// the right column 47 high); the progress bar at (17, 61) 222 x 6; the buttons 76 x 23 at y 75
INT NCGaraDialogBox::OnCreate()
{
    BackTex = FindTexture(TEXT("L2UI_ch3.dialog.system_back"));
    KeypadBackTex = FindTexture(TEXT("L2UI_ch3.Calculate.Calculate1_back"));
    IconTex = FindTexture(TEXT("L2UI_ch3.dialog.warningicon"));

    EditBox = new NCEditBox(3, NULL, 1, 150, 0);
    EditBox->ID = NID_DIALOG_EDIT;
    EditBox->SetPosition(50, 48, 156, 17);
    CreateChild(EditBox, NWS_VISIBLE);

    INT X = 7;
    INT Y = 7;
    INT W = 23;
    INT H = 23;
    for (INT i = 0; i < 13; i++)
    {
        if (i >= 0 && i < 9)
        {
            if (i % 3 == 0)
            {
                X = 7;
                if (i != 0)
                    Y += 25;
            }
            else
                X += 25;
        }
        else if (i == 9)
        {
            X = 7;
            Y += 25;
            W = 23;
        }
        else if (i == 10)
        {
            Y = 7;
            X = 82;
            H = 47;
            W = 23;
        }
        else if (i == 11)
        {
            X = 32;
            Y = 82;
            W = 47;
            H = 23;
        }
        else if (i == 12)
        {
            Y = 57;
            X = 82;
            H = 47;
            W = 23;
        }
        Keypad[i] = new NCButton(GKeypadTextures[i][0], GKeypadTextures[i][1], 0);
        Keypad[i]->ID = NID_DIALOG_KEYPAD;
        Keypad[i]->NotifyID = i;
        Keypad[i]->SetPosition((INT)((FLOAT)X + Width), Y, W, H);
        CreateChild(Keypad[i], NWS_VISIBLE);
    }

    Progress = new NCProgressCtrl(TEXT("L2UI_ch3.dialog.minibar_back"), TEXT("L2UI_ch3.dialog.minibar_time1"), 1);
    Progress->ID = NID_DIALOG_PROGRESS;
    Progress->SetPosition(17, 61, 222, 6);
    CreateChild(Progress, NWS_VISIBLE);
    Progress->SetMax(10000);
    Progress->Reset();

    OKButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    OKButton->ID = NID_DIALOG_OK;
    OKButton->SetPosition(51, 75, 76, 23);
    CreateChild(OKButton, NWS_VISIBLE);

    CancelButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    CancelButton->ID = NID_DIALOG_CANCEL;
    CancelButton->SetPosition(131, 75, 76, 23);
    CreateChild(CancelButton, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10009920, NCGaraDialogBox, OnCreate)

// 0x100078b0 (vslot 38) - guarded in C2; NCWnd's OnDestroy at the end (NControl's is not called)
INT NCGaraDialogBox::OnDestroy()
{
    guard(NCGaraDialogBox::OnDestroy);
    for (INT i = 0; i < 13; i++)
    {
        if (Keypad[i])
        {
            RemoveChildWindow(Keypad[i]->Entry);
            delete Keypad[i];
            Keypad[i] = NULL;
        }
    }
    if (OKButton)
    {
        RemoveChildWindow(OKButton->Entry);
        delete OKButton;
        OKButton = NULL;
    }
    if (CancelButton)
    {
        RemoveChildWindow(CancelButton->Entry);
        delete CancelButton;
        CancelButton = NULL;
    }
    if (EditBox)
    {
        RemoveChildWindow(EditBox->Entry);
        delete EditBox;
        EditBox = NULL;
    }
    if (Progress)
    {
        RemoveChildWindow(Progress->Entry);
        delete Progress;
        Progress = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100078b0, NCGaraDialogBox, OnDestroy)

// 0x100076f0 (vslot 56) - C3: the box stretched over the window, the warning icon at (11, 15), with an edit line the
// keypad's panel at x 256 (108 wide), and the message at (41, 16) wrapped to 200 px in 0xFFDCDCDC, its escaped
// newlines made real; the clip goes on before the origin
INT NCGaraDialogBox::OnPaint(NCGDevice* Device)
{
    guard(NCGaraDialogBox::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, (INT)Width, (INT)Height, BackTex);
    if (IconTex)
        Device->DrawTexture(11, 15, 24, 24, 0, 0, 24, 24, IconTex);
    if (bInput != 0 && KeypadBackTex)
        Device->DrawTexture(256, 0, 108, (INT)Height, 0, 0, 108, (INT)Height, KeypadBackTex);
    FString Text;
    GConsole->ConvertNewLines((TCHAR*)*Message, Text);
    Device->DrawText(41, 16, 0xFFDCDCDC, *Text, 200, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100076f0, NCGaraDialogBox, OnPaint)

/*-----------------------------------------------------------------------------
    The state: Reset, SetInfo, the buttons' texts, the layout.
-----------------------------------------------------------------------------*/

// 0x10006d90 - ParamText and the params are kept. C3 clears the two new flags as well
void NCGaraDialogBox::Reset()
{
    DialogID = -1;
    bInput = 0;
    Owner = NULL;
    Message.Empty();
    Unknown238 = 0;
    bProgress = 0;
    Flag238 = 0;
    bLargeAmount = 0;
    EditBox->ClearText();
}
NW_HOOK_METHOD(0x10006d90, NCGaraDialogBox, Reset)

// 0x10007310
void NCGaraDialogBox::SetButtonTexts(INT OKTextID, INT CancelTextID)
{
    if (OKButton)
    {
        if (*SysString(OKTextID) != NULL)
            OKButton->SetTextID(OKTextID);
    }
    if (CancelButton)
    {
        if (*SysString(CancelTextID) != NULL)
            CancelButton->SetTextID(CancelTextID);
    }
}
NW_HOOK_METHOD(0x10007310, NCGaraDialogBox, SetButtonTexts)

// 0x100073b0
NWndEntry* NCGaraDialogBox::GetOwner()
{
    return Owner;
}
NW_HOOK_METHOD(0x100073b0, NCGaraDialogBox, GetOwner)

// 0x10007380 (ret 4)
void NCGaraDialogBox::SetEditText(const TCHAR* Text)
{
    if (bInput && Text)
        EditBox->InsertText(Text, 1);
}
NW_HOOK_METHOD(0x10007380, NCGaraDialogBox, SetEditText)

// 0x10007a20 - C3: 256 x 108 (87 was C2's: the buttons at y 75 + 23 ended below the box; the box with a digits line: 364
// wide, with the keypad); the buttons at (51, 75) and
// (131, 75), a notice's OK alone and centered (half the width less 38); the FLOAT sizes
void NCGaraDialogBox::UpdateLayout()
{
    SetWindowSize(256.0f, 108.0f);
    // the stock code fills { X, Y, W, H } with GetLocalRect; SetScreenPos reads x and y
    NRectF Local;
    Local.X = 0;
    Local.Y = 0;
    Local.W = 0;
    Local.H = 0;
    GetLocalRect(&Local);
    OKButton->SetScreenPos(51, 75, (const POINT*)&Local);
    CancelButton->SetScreenPos(131, 75, (const POINT*)&Local);
    if (bInput == 0)
    {
        SetWindowSize(256.0f, 108.0f);
        EditBox->HideWindow();
        for (INT i = 0; i < 13; i++)
            Keypad[i]->HideWindow();
    }
    else if (Type == 3)
        SetWindowSize(364.0f, 108.0f);
    else
    {
        SetWindowSize(256.0f, 108.0f);
        for (INT i = 0; i < 13; i++)
            Keypad[i]->HideWindow();
    }
    if (DialogID == 0x1D || DialogID == 0x1F || DialogID == 0x10 || DialogID == 0x50)
    {
        CancelButton->HideWindow();
        OKButton->SetScreenPos((INT)(Width * 0.5f - 38.0f), 75, (const POINT*)&Local);
    }
    if (bProgress == 0)
        Progress->HideWindow();
    else
    {
        Progress->ShowWindow();
        if (DialogID != 0x13 && DialogID != 0x34)
            Progress->SetMax(10000);
        else
            Progress->SetMax(900000);
    }
}
NW_HOOK_METHOD(0x10007a20, NCGaraDialogBox, UpdateLayout)

// 0x10007be0 - Text2 wins over Text; an empty ParamText or a zero param keeps the box's. C3: an open box is cancelled
// first, the progress flag moved to +0x1C of the info (+0x18 is a new flag, kept at +0x238), the place and the two
// sounds come along (+0x78 .. +0x94). The stock asks the box for its local rectangle in between and ignores it
void NCGaraDialogBox::SetInfo(NDialogInfo* Info)
{
    if (DialogID != -1)
        OnCancel();
    Reset();
    bLargeAmount = Info->bLargeAmount;
    DialogID = Info->ID;
    Type = Info->Type;
    EditBox->SetType(Info->Type);
    bInput = Info->bInput;
    Flag238 = Info->Flag18;
    Owner = Info->Owner;
    bProgress = Info->bProgress;
    if (bProgress == 0)
        Progress->HideWindow();
    else
    {
        Progress->ShowWindow();
        Progress->Reset();
        Progress->Start();
    }
    if (Info->Text2.Num() == 0)
    {
        if (Info->Text.Num() != 0)
            Message = Info->Text;
    }
    else
        Message = Info->Text2;
    if (Info->ParamText.Num() != 0)
        ParamText = Info->ParamText;
    if (Info->Param1)
        Param1 = Info->Param1;
    if (Info->Param2)
        Param2 = Info->Param2;
    NRectF Local;
    Local.X = 0;
    Local.Y = 0;
    Local.W = 0;
    Local.H = 0;
    GetLocalRect(&Local);
    Unknown238 = Info->Unknown74;
    Unknown22C[0] = ::nw::Field<INT>(Info, 0x7C);
    Unknown22C[1] = ::nw::Field<INT>(Info, 0x80);
    Unknown22C[2] = ::nw::Field<INT>(Info, 0x84);
    ShowSoundA = Info->Text88;
    ShowSoundB = Info->Text94;
    if (bInput && Info->Text60.Num() != 0)
        SetEditText(*Info->Text60);
}
NW_HOOK_METHOD(0x10007be0, NCGaraDialogBox, SetInfo)

/*-----------------------------------------------------------------------------
    The answers. OnOK: the amount typed is read first (0 .. 1,000,000, or 100,000,000 with
    bLargeAmount; 1 without an edit line or a text), then DialogID picks what the box does. The ids
    are C2's (C1's from 0x24 on moved up by four, with new ones in between).
-----------------------------------------------------------------------------*/

// a request with one number
#define VALUE_REQUEST(Request, Value)                                                          \
    {                                                                                          \
        L2ParamStack Params(10);                                                               \
        Params.PushBack((void*)(INT_PTR)(Value));                                              \
        GConsole->Request(Params);                                                             \
    }
// a request with ParamText and a number
#define NAMED_REQUEST(Request, Value)                                                          \
    {                                                                                          \
        L2ParamStack Params(10);                                                               \
        Params.PushBack((void*)*ParamText);                                                    \
        Params.PushBack((void*)(INT_PTR)(Value));                                              \
        GConsole->Request(Params);                                                             \
    }

// the name typed as the stock code measures it: bytes in the ANSI code page (C3: a 800 byte buffer, C2's was 512)
static FORCEINLINE INT AnsiLength(const TCHAR* Text)
{
    ANSICHAR Ansi[800];
    WideCharToMultiByte(CP_ACP, 0, Text, -1, Ansi, 800, NULL, NULL);
    return (INT)strlen(Ansi);
}

// __wtoi64 (0x1014ca7c): spaces, one sign, digits added up with no overflow check (the UCRT's would saturate)
static INT64 NWtoi64(const TCHAR* Str)
{
    while (NIswctype(*Str, _SPACE))
        Str++;
    TCHAR C = *Str++;
    const TCHAR Sign = C;
    if (C == '-' || C == '+')
        C = *Str++;
    unsigned __int64 Total = 0;
    for (INT Digit; (Digit = NWcharToDigit(C)) != -1; C = *Str++)
        Total = Total * 10 + (unsigned __int64)Digit;
    return (INT64)(Sign == '-' ? (unsigned __int64)0 - Total : Total);
}

// 0x1006b980 (name ours; NConsoleWnd's, ret 4) - a notice box (dialog 0x1F, the desktop's main box) with one system
// message: the answer of a refused amount, the owner not taking it. The stock ignores this and reads GConsole
static void __fastcall ShowNoticeDialog(NConsoleWnd*, void*, INT MessageID)
{
    NDialogInfo Info;
    Info.bInput = 0;
    Info.ID = 0x1F;
    Info.bMainBox = 1;
    Info.SetSystemMessage(MessageID);
    GConsole->ShowDialog(&Info);
}
NW_HOOK_FUNC(0x1006B980, ShowNoticeDialog)

// 0x10007d80 (vslot 69) - OK. The box closes afterwards unless noted ("return 1" before the end keeps it open or
// has closed it already). C3: the amount typed is read as a 64 bit number at the top and only once (it goes to
// the cases in Amount); an item that is not adena or a stackable one (the console's 0x100725f0 asks the item table:
// 3 / 2) has no limit but the box's own, a refused amount shows the notice (0x53A / 0x559) and the box stays closed;
// the answers that need the owner's yes show the notice 0x559 when it refuses; the confirmations of the war
// (0x17, 0x19) and the alliance (0x32) ask a second question (0x4E, 0x4F, 0x4D) that makes the request itself;
// the new ids 0x46 .. 0x51 (a confirmation, sendhome / recall, the alliance war surrender, exit, the VIP buy).
// The console's request thunks (0x1005d4b0 .. 0x1005d7a0): the header's names of them are C2's and one off (each is a tail jump into
// the network handler; the comment at each call names the engine's slot), and some take no argument
INT NCGaraDialogBox::OnOK()
{
    if (bProgress)
        Progress->Stop();
    INT Amount = 1;
    if (bInput && EditBox->CommitText())
    {
        INT64 Typed = NWtoi64(EditBox->CommitText());
        if (((NConsoleLeftFns*)GConsole)->GetEtcItemStackKind(Flag238) == 3 || bLargeAmount != 0)    // 0x100725F0
        {
            if (Typed >= 0)
            {
                if (Typed > 2000000000)
                {
                    Reset();
                    ShowNoticeDialog(GConsole, NULL, 0x559);
                    return 1;
                }
                Amount = (INT)Typed;
            }
            else
                Amount = 0;
        }
        else
        {
            if (((NConsoleLeftFns*)GConsole)->GetEtcItemStackKind(Flag238) == 2)                      // 0x100725F0
            {
                if (Typed < 0)
                    Amount = 0;
                else if (Typed > 1000000)
                {
                    Reset();
                    ShowNoticeDialog(GConsole, NULL, 0x53A);
                    return 1;
                }
                else
                    Amount = (INT)Typed;
            }
            else if (Typed < 0)
                Amount = 0;
            else
                Amount = (INT)Typed;
        }
    }

    switch (DialogID)
    {
    case 0: // yes to the owner
    case 1:
        if (Owner)
            SendTo(Owner, 0xF0000003, 1, 0);
        break;
    case 2: // amounts for the owner (items, adena...); without an edit line, yes; the id comes along
    case 3:
    case 8:
    case 0x23:
        if (Owner)
        {
            if (bInput == 0)
                SendTo(Owner, 0xF0000003, 1, DialogID);
            else if (EditBox->CommitText())
                SendTo(Owner, 0xF0000003, Amount, DialogID);
        }
        break;
    case 4: // drop an item where the pawn stands (unless that is forbidden)
    {
        NItemRequest Request;
        Request.ObjectID = Param1;
        Request.Count = Amount;
        if ((GConsole->GetMyUser()->Pawn->Unknown690 & 1) == 0)
            GConsole->RequestDropItem(Request, *(FVector*)Unknown22C);
        else
            GConsole->GetDesktop()->AddSystemText(*GL2GameData.SystemMsg[SYSMSG_ItemFlying].Message, 0, 0xFFDCDCDC);
        break;
    }
    case 5: // destroy an item
    {
        NItemRequest Request;
        Request.ObjectID = Param1;
        Request.Count = Amount;
        ::nw::VirtualCall<void>(GConsole, 395, Request);        // NConsoleWnd::RequestDestroyItem
        GConsole->PlayInterfaceSound(SOUND_TrashBasket);
        break;
    }
    case 6: // crystallize an item
        ::nw::VirtualCall<void>(GConsole, 396, Param1, Amount); // NConsoleWnd::RequestCrystallizeItem
        GConsole->PlayInterfaceSound(SOUND_TrashBasket);
        break;
    case 7: // trade request
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)1);
        ::nw::VirtualCall<void>(GConsole, 397, &Params);        // NConsoleWnd::AnswerTradeRequest
        break;
    }
    case 0xA: // party invitation
        VALUE_REQUEST(RequestAnswerJoinParty, 1)
        break;
    case 0xB:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)EditBox->CommitText());
        GConsole->RequestPledgeMemberList(Params);   // 0x1005D4B0: the network handler's +0x1C0, RequestCreatePledge(L2ParamStack&)
        break;
    }
    case 0xC:
        GConsole->RequestOustPledgeMember();          // 0x1005D4E0: the network handler's +0x1CC, RequestDismissPledge()
        break;
    case 0xE: // clan invitation
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)1);
        GConsole->RequestNet103(Params);                            // 0x1005D4C0 (the network handler's +0x1C4)
        break;
    }
    case 0xF: // title for the selected clan member
    {
        User* Target = GConsole->GetSelectedActor();
        if (!Target || Target->Unknown8)
            break;
        INT Len = AnsiLength(EditBox->CommitText());
        if (!GConsole->HasInvalidChars((TCHAR*)EditBox->CommitText()) &&
            GConsole->IsValidName((TCHAR*)EditBox->CommitText()))
        {
            if (Len > 16)
            {
                GConsole->ShowSystemMessage(80, COLOR_Warning);
                EditBox->SetFocus();
                return 1;
            }
            L2ParamStack Params(10);
            Params.PushBack(Target->GetName());
            Params.PushBack((void*)EditBox->CommitText());
            GConsole->RequestSurrenderPersonally(Params);              // 0x1005D550: the network handler's +0x1F4, RequestGiveNickName
            break;
        }
        GConsole->ShowSystemMessage(204, COLOR_Warning);
        EditBox->ClearText();
        EditBox->SetFocus();
        return 1;
    }
    case 0x10: // clan crest notice: then the file name (dialog 0x11)
    {
        NDialogInfo Info;
        Info.Owner = Entry;
        Info.ID = 0x11;
        Info.bMainBox = 1;
        Info.bInput = 1;
        Info.Type = 0;
        Info.SetSystemMessage(208);
        GConsole->ShowDialog(&Info);
        return 1;
    }
    case 0x11: // the crest's file (C3: a second value, 0)
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)EditBox->CommitText());
        Params.PushBack((void*)0);
        GConsole->RegisterPledgeCrest(Params);
        break;
    }
    case 0x12: // clan war: declare, answer, stop, answer
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)EditBox->CommitText());
        GConsole->RequestNet106(Params);              // 0x1005D4F0: the network handler's +0x1D8, RequestStartPledgeWar(L2ParamStack&)
        break;
    }
    case 0x13:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)*ParamText);
        Params.PushBack((void*)1);
        GConsole->RequestStartPledgeWar(Params);                    // 0x1005D500 (the network handler's +0x1DC)
        break;
    }
    case 0x14:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)EditBox->CommitText());
        GConsole->RequestReplyStartPledgeWar(Params);               // 0x1005D510 (the network handler's +0x1E0)
        break;
    }
    case 0x15:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)*ParamText);
        Params.PushBack((void*)1);
        GConsole->RequestStopPledgeWar(Params);                     // 0x1005D520 (the network handler's +0x1E4)
        break;
    }
    case 0x17: // surrender the war: the clan name typed, then the question (0x4E) with it
        if (EditBox->CommitText())
        {
            NDialogInfo Info;
            Info.ID = 0x4E;
            Info.Owner = Entry;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.SetParamText(EditBox->CommitText());
            Info.SetSystemMessage(0x4E4);
            GConsole->ShowDialog(&Info);
            return 1;
        }
        break;
    case 0x18: // expel a member
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)*ParamText);
        GConsole->RequestAnswerJoinPledge(Params);                  // 0x1005D4D0 (the network handler's +0x1C8)
        break;
    }
    case 0x19: // surrender personally: the question (0x4F) with the name typed
        if (EditBox->CommitText())
        {
            NDialogInfo Info;
            Info.ID = 0x4F;
            Info.Owner = Entry;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.SetParamText(EditBox->CommitText());
            Info.SetSystemMessage(0x4E5);
            GConsole->ShowDialog(&Info);
            return 1;
        }
        break;
    case 0x1A: // leave the clan
        GConsole->RequestJoinPledge();                // 0x1005D570: the network handler's +0x1D4, RequestWithDrawalPledge()
        break;
    case 0x1B: // restart / quit (the system menu)
        if (Owner)
            SendTo(Owner, 0xF0000003, 0x1B, 0);
        break;
    case 0x1C:
        if (Owner)
            SendTo(Owner, 0xF0000003, 0x1C, 0);
        break;
    case 0x1D: // disconnected: the game ends
        ::nw::VirtualCall<void>(GConsole, 391);                 // NConsoleWnd::QuitGame
        break;
    case 0x1E: // delete a quest, the desktop's quest window forgets the selection
        ::nw::VirtualCall<void>(GConsole, 399, Param1);         // NConsoleWnd::RequestDestroyQuest
        ((NConsoleLeftFns*)GConsole)->ShowMapMark(0);              // 0x100076C0
        break;
    case 0x20: // revival
        ::nw::VirtualCall<void>(GConsole, 393, 1, Param1, Param2);   // NConsoleWnd::AnswerReviveRequest
        break;
    case 0x21: // use an item
        GConsole->RequestUseItem(Param1);
        break;
    case 0x22: // delete a recipe
        GConsole->RequestRecipeItemDelete(Param1);
        break;
    case 0x24: // the recipe shop's message
        if (EditBox->CommitText())
        {
            const TCHAR* Text = EditBox->CommitText();
            if (Text && *Text)
                GConsole->RequestRecipeShopMessageSet((TCHAR*)Text);
        }
        break;
    case 0x25: // private store (sell) amounts; the box stays when the owner refuses
        if (Owner)
        {
            if (bInput == 0)
                SendTo(Owner, 0xF0000006, 1, 0);
            else if (EditBox->CommitText())
            {
                if (SendTo(Owner, 0xF0000006, Amount, 0))
                {
                    Reset();
                    HideWindow();
                    return 1;
                }
                if (DialogID != 0x25)
                    return 1;
                ShowNoticeDialog(GConsole, NULL, 0x559);
                return 1;
            }
        }
        break;
    case 0x26:
        if (Owner)
        {
            if (bInput != 0 && !EditBox->CommitText())
                break;
            if (!SendTo(Owner, 0xF0000007, bInput == 0 ? 1 : Amount, 0))
            {
                ShowNoticeDialog(GConsole, NULL, 0x559);
                return 1;
            }
        }
        break;
    case 0x27: // private store message
        if (EditBox->CommitText())
        {
            const TCHAR* Text = EditBox->CommitText();
            if (*Text)
                GConsole->SetPrivateStoreMsg((TCHAR*)Text);
        }
        break;
    case 0x28:
        if (Owner)
        {
            if (SendTo(Owner, 0xF0000008, 0, 0))
            {
                Reset();
                HideWindow();
                return 1;
            }
            if (DialogID != 0x28)
                return 1;
            ShowNoticeDialog(GConsole, NULL, 0x559);
            return 1;
        }
        break;
    case 0x29:
        if (Owner)
        {
            if (!SendTo(Owner, 0xF000000A, 0, 0))
            {
                ShowNoticeDialog(GConsole, NULL, 0x559);
                return 1;
            }
        }
        break;
    case 0x2A: // private store (buy) amounts
        if (Owner)
        {
            if (bInput == 0)
                SendTo(Owner, 0xF000000B, 1, 0);
            else if (EditBox->CommitText())
            {
                if (!SendTo(Owner, 0xF000000B, Amount, 0))
                    return 1;
                Reset();
                HideWindow();
                return 1;
            }
        }
        break;
    case 0x2B:
        if (Owner)
        {
            if (bInput == 0)
                SendTo(Owner, 0xF000000C, 1, 0);
            else if (EditBox->CommitText())
            {
                if (!SendTo(Owner, 0xF000000C, Amount, 0))
                {
                    ShowNoticeDialog(GConsole, NULL, 0x559);
                    return 1;
                }
            }
        }
        break;
    case 0x2C: // private store (buy) message
        if (EditBox->CommitText())
        {
            const TCHAR* Text = EditBox->CommitText();
            if (*Text)
                GConsole->SetPrivateStoreBuyMsg((TCHAR*)Text);
        }
        break;
    case 0x2D:
        if (Owner)
        {
            if (!SendTo(Owner, 0xF000000D, 0, 0))
                return 1;
            Reset();
            HideWindow();
            return 1;
        }
        break;
    case 0x2E:
        if (Owner)
        {
            if (!SendTo(Owner, 0xF000000F, 0, 0))
            {
                ShowNoticeDialog(GConsole, NULL, 0x559);
                return 1;
            }
        }
        break;
    case 0x2F: // alliance: crest (C3: a second value, 0)
        if (EditBox->CommitText())
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)EditBox->CommitText());
            Params.PushBack((void*)0);
            GConsole->RegisterAllyCrest(Params);
        }
        break;
    case 0x30: // dismiss
        GConsole->RequestDismissAlly();                             // 0x1005D7A0
        break;
    case 0x31: // leave
        GConsole->RequestWithdrawAlly();                            // 0x1005D790
        break;
    case 0x32: // expel a clan: the question (0x4D) with the name typed
        if (EditBox->CommitText())
        {
            NDialogInfo Info;
            Info.ID = 0x4D;
            Info.Owner = Entry;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.SetParamText(EditBox->CommitText());
            Info.SetSystemMessage(0x4E3);
            GConsole->ShowDialog(&Info);
            return 1;
        }
        break;
    case 0x33: // alliance invitation
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)1);
        GConsole->RequestAnswerJoinAlly(Params);                    // 0x1005D760
        break;
    }
    case 0x34: // alliance war: start / stop asked by the other side
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)*ParamText);
        Params.PushBack((void*)1);
        ::nw::VirtualCall<void>(GConsole, 231, &Params);        // NConsoleWnd::RequestReplyStartAllianceWar
        break;
    }
    case 0x35:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)*ParamText);
        Params.PushBack((void*)1);
        ::nw::VirtualCall<void>(GConsole, 233, &Params);        // NConsoleWnd::RequestReplyStopAllianceWar
        break;
    }
    case 0x36: // give an item to the pet / take one back
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Param1);
        Params.PushBack((void*)(INT_PTR)Amount);
        GConsole->RequestGiveItemToPet(Params);                     // 0x1005DFF0
        break;
    }
    case 0x37:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Param1);
        Params.PushBack((void*)(INT_PTR)Amount);
        Params.PushBack((void*)0);
        GConsole->RequestGetItemFromPet(Params);                    // 0x1005E000
        break;
    }
    case 0x38: // drop an item where the cursor points (unless that is forbidden)
    {
        NItemRequest Request;
        Request.ObjectID = Param1;
        Request.Count = Amount;
        if ((GConsole->GetMyUser()->Pawn->Unknown690 & 1) == 0)
            GConsole->RequestDropItem(Request, ((UGameEngine*)GConsole->Engine)->MouseHitLocation);
        else
            GConsole->GetDesktop()->AddSystemText(*GL2GameData.SystemMsg[SYSMSG_ItemFlying].Message, 0, 0xFFDCDCDC);
        break;
    }
    case 0x39: // pet name
    {
        INT Len = AnsiLength(EditBox->CommitText());
        if (!GConsole->HasInvalidChars((TCHAR*)EditBox->CommitText()) &&
            GConsole->IsValidName((TCHAR*)EditBox->CommitText()))
        {
            if (Len > 16)
            {
                GConsole->ShowSystemMessage(80, COLOR_Warning);
                EditBox->SetFocus();
                return 1;
            }
            GConsole->RequestChangePetName((TCHAR*)EditBox->CommitText());
            break;
        }
        GConsole->ShowSystemMessage(591, COLOR_Warning);
        EditBox->ClearText();
        EditBox->SetFocus();
        return 1;
    }
    case 0x3A: // action 20, with the modifier keys held (C3: vslot 297, the attack key, in place of Ctrl)
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)20);
        Params.PushBack((void*)(INT_PTR)::nw::VirtualCall<UBOOL>(GConsole, 297));
        Params.PushBack((void*)(INT_PTR)GConsole->Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
        GConsole->RequestActionUse(Params);
        break;
    }
    case 0x3B: // friend request
        VALUE_REQUEST(RequestAnswerFriendInvite, 1)
        break;
    case 0x3C:
        if (Owner)
            SendTo(Owner, 0xF0000010, 0, 0);
        break;
    case 0x3D:
        if (Owner)
            SendTo(Owner, 0xF0000011, 0, 0);
        break;
    case 0x3E: // siege: register / cancel as attacker / defender (castle in Param1)
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Param1);
        Params.PushBack((void*)1);
        Params.PushBack((void*)1);
        ::nw::VirtualCall<void>(GConsole, 240, &Params);        // NConsoleWnd::RequestJoinSiege
        break;
    }
    case 0x3F:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Param1);
        Params.PushBack((void*)1);
        Params.PushBack((void*)0);
        ::nw::VirtualCall<void>(GConsole, 240, &Params);
        break;
    }
    case 0x40:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Param1);
        Params.PushBack((void*)0);
        Params.PushBack((void*)1);
        ::nw::VirtualCall<void>(GConsole, 240, &Params);
        break;
    }
    case 0x41:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Param1);
        Params.PushBack((void*)0);
        Params.PushBack((void*)0);
        ::nw::VirtualCall<void>(GConsole, 240, &Params);
        break;
    }
    case 0x42: // siege: approve / refuse a defending clan
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Param1);
        Params.PushBack((void*)(INT_PTR)Param2);
        Params.PushBack((void*)1);
        ::nw::VirtualCall<void>(GConsole, 241, &Params);        // NConsoleWnd::RequestConfirmSiegeWaitingList
        break;
    }
    case 0x43:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Param1);
        Params.PushBack((void*)(INT_PTR)Param2);
        Params.PushBack((void*)0);
        ::nw::VirtualCall<void>(GConsole, 241, &Params);
        break;
    }
    case 0x44:
    case 0x47:
        if (Owner)
            SendTo(Owner, 0xF0000003, 0, 0);
        break;
    case 0x45:
        GConsole->RequestUseItem(Param1);
        break;
    case 0x46: // a confirmation: Param1 and 1
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Param1);
        Params.PushBack((void*)1);
        GConsole->SendConfirmDlg(Params);                         // NConsoleWnd::SendConfirmDlg 0x10060B40
        break;
    }
    case 0x48: // take the title of the selected clan member away
    {
        User* Target = GConsole->GetSelectedActor();
        if (!Target || Target->Unknown8)
            break;
        L2ParamStack Params(10);
        Params.PushBack(Target->GetName());
        TCHAR Empty[2];
        Empty[0] = 0;
        Params.PushBack(Empty);
        GConsole->RequestSurrenderPersonally(Params);            // 0x1005D550: the network handler's +0x1F4, RequestGiveNickName
        break;
    }
    case 0x49:
        ((NConsoleLeftFns*)GConsole)->RemovePledgeCrest();          // 0x1005D490
        break;
    case 0x4A: // GM: send a player home
    {
        FString Command(TEXT("sendhome "));
        Command += ParamText;
        L2ParamStack Params(10);
        Params.PushBack((void*)*Command);
        ::nw::VirtualCall<void>(GConsole, 193, &Params);        // NConsoleWnd::BuilderCommand
        break;
    }
    case 0x4B: // GM: recall a player
    {
        FString Command(TEXT("recall "));
        Command += ParamText;
        L2ParamStack Params(10);
        Params.PushBack((void*)*Command);
        ::nw::VirtualCall<void>(GConsole, 193, &Params);
        break;
    }
    case 0x4C:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)*ParamText);
        ::nw::VirtualCall<void>(GConsole, 234, &Params);        // NConsoleWnd::RequestSurrenderAllianceWar
        break;
    }
    case 0x4D:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)*ParamText);
        GConsole->RequestOustAlly(Params);                       // NConsoleWnd::RequestOustAlly 0x1005D770
        break;
    }
    case 0x4E:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)*ParamText);
        GConsole->RequestReplyStopPledgeWar(Params);             // 0x1005D530: the network handler's +0x1E8, RequestSurrenderPledgeWar
        break;
    }
    case 0x4F:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)*ParamText);
        GConsole->RequestSurrenderPledgeWar(Params);             // 0x1005D540: the network handler's +0x1F0, RequestSurrenderPersonally
        break;
    }
    case 0x50:
        appRequestExit(0);
        break;
    case 0x51:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Param1);
        Params.PushBack((void*)(INT_PTR)Param2);
        ::nw::VirtualCall<void>(GConsole, 212, &Params);        // NConsoleWnd::RequestBuyVIPItem
        break;
    }
    }
    Reset();
    HideWindow();
    if (EditBox)
        EditBox->KillFocus();
    KillFocus();
    return 1;
}
NW_HOOK_METHOD(0x10007D80, NCGaraDialogBox, OnOK)

// a request to a console function, by the method that holds the address the C3 code calls (the console header's names of 0x1005d4c0 ..
// 0x1005d530 are shifted by one against the C2 source's: the addresses are the evidence, they are named beside each call)
#define VALUE_REQUEST_AT(Method, Value)                                                        \
    {                                                                                          \
        L2ParamStack Params(10);                                                               \
        Params.PushBack((void*)(INT_PTR)(Value));                                              \
        GConsole->Method(Params);                                                              \
    }
#define NAMED_REQUEST_AT(Method, Value)                                                        \
    {                                                                                          \
        L2ParamStack Params(10);                                                               \
        Params.PushBack((void*)*ParamText);                                                    \
        Params.PushBack((void*)(INT_PTR)(Value));                                              \
        GConsole->Method(Params);                                                              \
    }

// 0x10006ed0 (vslot 70) - Cancel: the refusals; the box closes. C3: dialog 0 answers the owner like 1 and 0x46 is new
// (a confirmation sent with Param1 and 0)
INT NCGaraDialogBox::OnCancel()
{
    if (bProgress)
        Progress->Stop();
    if (EditBox)
        EditBox->KillFocus();
    switch (DialogID)
    {
    case 0:
    case 1:
        if (Owner)
            SendTo(Owner, NWM_DIALOG_RESULT, 0, 0);
        break;
    case 7:
        // NConsoleWnd::AnswerTradeRequest, slot 397 (vcall +0x634: the header's declaration compiles to +0x764)
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)0);
            ::nw::VirtualCall<void>(GConsole, 397, &Params);
        }
        break;
    case 0xA:
        VALUE_REQUEST(RequestAnswerJoinParty, 0)
        break;
    case 0xE:
        VALUE_REQUEST_AT(RequestNet103, 0)                   // 0x1005D4C0 (the network handler's +0x1C4)
        break;
    case 0x13:
        NAMED_REQUEST_AT(RequestStartPledgeWar, 0)           // 0x1005D500 (+0x1DC)
        break;
    case 0x15:
        VALUE_REQUEST_AT(RequestStopPledgeWar, 0)            // 0x1005D520 (+0x1E4); no clan name, unlike OK
        break;
    case 0x20:
        ::nw::VirtualCall<void>(GConsole, 393, 0, Param1, Param2);   // NConsoleWnd::AnswerReviveRequest (vcall +0x624)
        break;
    case 0x29:
        if (Owner)
            Owner->Wnd->HideWindow();
        break;
    case 0x33:
        VALUE_REQUEST(RequestAnswerJoinAlly, 0)
        break;
    case 0x34:
        NAMED_REQUEST(RequestReplyStartAllianceWar, 0)
        break;
    case 0x35:
        NAMED_REQUEST(RequestReplyStopAllianceWar, 0)
        break;
    case 0x3B:
        VALUE_REQUEST(RequestAnswerFriendInvite, 0)
        // falls through: the owner is told too
    case 0x28:
        if (Owner)
            SendTo(Owner, 0xF0000009, 0, 0);
        break;
    case 0x46:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Param1);
        Params.PushBack((void*)0);
        GConsole->SendConfirmDlg(Params);                         // NConsoleWnd::SendConfirmDlg 0x10060B40
        break;
    }
    }
    Reset();
    HideWindow();
    KillFocus();
    return 1;
}
NW_HOOK_METHOD(0x10006ed0, NCGaraDialogBox, OnCancel)

#undef GARA_AMOUNT
#undef TEXT_REQUEST
#undef VALUE_REQUEST
#undef NAMED_REQUEST
#undef SIEGE_REQUEST

// 0x1000a090 - the four strings (the two sounds, the param text, the message) go, then NControl
NW_HOOK_DTOR(0x1000a090, NCGaraDialogBox)

NW_CHECK_VTABLE(NCGaraDialogBox, 0x101788b8, 74)
