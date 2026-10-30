/*=============================================================================
    NCPetitionWnd.cpp - the petition windows, the player's and the GM's. Stock: 0x100472e0 ..
    0x1004737f, 0x10094380 .. 0x100947d3, 0x100949c0 .. 0x10094bcd, 0x10094f10 ..
    0x10095587 and NCGMPetitionWnd's 0x1003e810 .. 0x1003e8af (ShowWindow is
    NCSystemMenuWnd's body, HideWindow NCChatWnd's).
=============================================================================*/
#include "nwindow/NCPetitionWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCPetitionListBox.h"
#include "nwindow/NCommandParser.h"
#include "nwindow/NConsoleWnd.h"

// chat types of the petition conversation (names ours)
enum { CHAT_Petition = 6, CHAT_GMPetition = 7 };

// a child window goes (repeated inline in the stock OnDestroys)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// the conversation list of both windows (repeated in the stock OnCreates): C3: 11 lines seen,
// 200 kept, a bare list (NWS_NOHITTEST, not NWS_VISIBLE) that follows the last line
static FORCEINLINE NCPetitionListBox* CreatePetitionList(NCWnd* Parent, INT ID)
{
    NCPetitionListBox* List = new NCPetitionListBox(200, 11, 15);
    List->ID = ID;
    List->SetPosition(8, 37, 294, 200);
    Parent->CreateChild(List, NWS_NOHITTEST);
    List->bAutoScroll = 1;
    List->LineGap = 0;
    return List;
}

// the line to write in (repeated in the stock OnCreates)
static FORCEINLINE NCEditBox* CreatePetitionEdit(NCWnd* Parent, INT ID)
{
    NCEditBox* Edit = new NCEditBox(1, NULL, 1, 2600, 0);
    Edit->ID = ID;
    Edit->SetPosition(7, 246, 296, 17);
    Parent->CreateChild(Edit, NWS_VISIBLE);
    return Edit;
}

// a button under the list (repeated in the stock OnCreates): 76 x 23
static FORCEINLINE NCButton* CreatePetitionButton(NCWnd* Parent, INT ID, INT X, const TCHAR* Normal, const TCHAR* Pushed)
{
    NCButton* Button = new NCButton(Normal, Pushed, 0);
    Button->ID = ID;
    Button->SetPosition(X, 272, 76, 23);
    Parent->CreateChild(Button, NWS_VISIBLE);
    return Button;
}

// NCFrameCtrl(Decorated, Vertical, Small): 0x10010450 (ret 0xc, NCFrameCtrl.cpp), run on the object `new` allocates
// (StaticAllocateObject of the class in the transient package)
static NCFrameCtrl* NewFrameCtrl(INT A, INT B, INT C)
{
    return new NCFrameCtrl(A, B, C);
}

// the title bar of both windows (repeated in the stock OnCreates): as wide as the window, 20 high
static FORCEINLINE void CreatePetitionFrame(NCWnd* Wnd, INT ID)
{
    Wnd->FrameCtrl = NewFrameCtrl(1, 0, 0);
    Wnd->FrameCtrl->ID = ID;
    Wnd->FrameCtrl->SetPosition(0, 0, (DWORD)(INT)Wnd->Width, 20);
    Wnd->CreateChild(Wnd->FrameCtrl, NWS_VISIBLE);
}

// the background under the title bar (one stock body for both windows, 0x100d0c20); the texture is not
// checked. C3: the size is the window's (the title bar's 20 pixels off the height)
static FORCEINLINE INT PaintPetitionWnd(NCWnd* Wnd, UTexture* BackTex, NCGDevice* Device)
{
    Device->BeginPaint(Wnd->ScreenX, Wnd->ScreenY);
    Device->SetClip(Wnd->ScreenX, Wnd->ScreenY, (INT)Wnd->Width, (INT)Wnd->Height);
    INT H = (INT)(Wnd->Height - 20.0f);
    INT W = (INT)Wnd->Width;
    Device->DrawTexture(0, 20, W, H, 0, 0, W, H, BackTex);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}

// a line of the conversation: the player's own petition lines (type 6) in white
static FORCEINLINE void AddPetitionMessage(NCPetitionListBox* List, TCHAR* Text, DWORD Color,
                                           INT Type)
{
    if (!List)
        return;
    if (Type == CHAT_Petition)
        List->AddString(Text, 0xFFDCDCDC, -1);
    else
        List->AddString(Text, Color, -1);
}

/*-----------------------------------------------------------------------------
    NCPetitionWnd
-----------------------------------------------------------------------------*/

// 0x10077970 - NCWnd's constructor and the vtable
NCPetitionWnd::NCPetitionWnd()
{
}
NW_HOOK_CTOR(0x10077970, NCPetitionWnd)

// 0x100779c0
NW_HOOK_DTOR(0x100779c0, NCPetitionWnd)

// 0x100d0180 (vslot 25) - C3: the buttons are Send (69), Cancel (70), the new one (71) and the line (68)
INT NCPetitionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_PETITION_SEND)
            return OnSend() != 0;
        if (wParam == NID_PETITION_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_PETITION_NEWBUTTON)
            return OnNewButton() != 0;
        if (wParam == NID_PETITION_EDIT)
            return OnEdit((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100d0180, NCPetitionWnd, WindowProc)

// 0x100d2cd0 (vslot 37) - C3: another layout (the list 294 x 200 at (8, 37), the line at (7, 246)) and the
// buttons "Cancel petition" (473) and a new one (text 849, disabled)
INT NCPetitionWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_ch3.PetitionWnd.petition2_back"));
    CreatePetitionFrame(this, NID_PETITION_FRAME);
    ListBox = CreatePetitionList(this, NID_PETITION_LIST);
    EditBox = CreatePetitionEdit(this, NID_PETITION_EDIT);
    CancelButton = CreatePetitionButton(this, NID_PETITION_CANCEL, 80, TEXT("L2ui_ch3.button.btn1_normal"),
                                        TEXT("L2ui_ch3.button.btn1_normalon"));
    CancelButton->SetText(*GL2GameData.SysString[473]);
    NewButton = CreatePetitionButton(this, NID_PETITION_NEWBUTTON, 160, TEXT("L2ui_ch3.button.btn1_normal"),
                                     TEXT("L2ui_ch3.button.btn1_normalon"));
    NewButton->SetTextID(849);
    NewButton->EnableWindow(0);
    return 1;
}
NW_HOOK_METHOD(0x100d2cd0, NCPetitionWnd, OnCreate)

// 0x100d0af0 (vslot 38) - the list, the line, the two buttons; the title bar is left alone
INT NCPetitionWnd::OnDestroy()
{
    guard(NCPetitionWnd::OnDestroy);
    DestroyChild(this, ListBox);
    DestroyChild(this, EditBox);
    DestroyChild(this, CancelButton);
    DestroyChild(this, NewButton);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100d0af0, NCPetitionWnd, OnDestroy)

// 0x100aa860 (vslot 44) - NCSystemMenuWnd's body, shared (restored from its icon, the window comes back where the icon was)
#if NW_HOOK_MODE
NW_NAKED void NCPetitionWnd::ShowWindow() NW_STOCK(0x100aa860)
#else
NW_SLOT_IS("?ShowWindow@NCPetitionWnd@@UAEXXZ", "?ShowWindow@NCSystemMenuWnd@@UAEXXZ")   // NCPetitionWnd::ShowWindow = NCSystemMenuWnd::ShowWindow (0x100aa860)
#endif

// 0x100d0d50 (vslot 45) - C3: the new button is disabled again
void NCPetitionWnd::HideWindow()
{
    NewButton->EnableWindow(0);
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x100d0d50, NCPetitionWnd, HideWindow)

// 0x100d0c20 (vslot 56) - NCGMPetitionWnd's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCPetitionWnd::OnPaint(NCGDevice* Device) NW_STOCK(0x100d0c20)
#else
NW_SLOT_IS("?OnPaint@NCPetitionWnd@@UAEHPAVNCGDevice@@@Z", "?OnPaint@NCGMPetitionWnd@@UAEHPAVNCGDevice@@@Z")   // NCPetitionWnd::OnPaint = NCGMPetitionWnd::OnPaint (0x100d0c20)
#endif

// 0x100d0cc0 (vslot 68) - Enter: the line to the GM (chat type 6), then emptied; a line of 255 characters or more
// is refused with a message in the console's petition window (sysmsg 971; C3: in the color 0xFFDCDCDC)
INT NCPetitionWnd::OnEdit(LPARAM lParam)
{
    if (lParam == 1)
    {
        const TCHAR* Text = EditBox->CommitText();
        if (Text)
        {
            if (appStrlen(Text) < 255)
                GConsole->SendChatMsgToServer(CHAT_Petition, (TCHAR*)Text, NULL);
            else
                static_cast<NCPetitionWnd*>(GConsole->PetitionWnd)->AddMessage(
                    (TCHAR*)*GL2GameData.SystemMsg[971].Message, 0xFFDCDCDC, 0, 0);
        }
        EditBox->ClearText();
    }
    return 1;
}
NW_HOOK_METHOD(0x100d0cc0, NCPetitionWnd, OnEdit)

// 0x100d0d70 (vslot 70) - the new button goes disabled, the server is asked to cancel the petition
INT NCPetitionWnd::OnCancel()
{
    NewButton->EnableWindow(0);
    GConsole->RequestPetitionCancel();
    return 1;
}
NW_HOOK_METHOD(0x100d0d70, NCPetitionWnd, OnCancel)

// 0x100d0240 (vslot 71, name ours) - the console's warning dialog box is shown
INT NCPetitionWnd::OnNewButton()
{
    GConsole->WarningDialogBox->ShowWindow();
    return 1;
}
NW_HOOK_METHOD(0x100d0240, NCPetitionWnd, OnNewButton)

// 0x100d0470 - Param is not used; also the desktop's calls on NCGMPetitionWnd. Not inlined: the stock OnEdits call it
__declspec(noinline) void NCPetitionWnd::AddMessage(TCHAR* Text, DWORD Color, INT Type, INT Param)
{
    AddPetitionMessage(ListBox, Text, Color, Type);
}
NW_HOOK_METHOD(0x100d0470, NCPetitionWnd, AddMessage)

// 0x100d04b0 - the line and the conversation empty; also NCGMPetitionWnd's
void NCPetitionWnd::Clear()
{
    EditBox->ClearText();
    ListBox->Clear();
}
NW_HOOK_METHOD(0x100d04b0, NCPetitionWnd, Clear)

NW_CHECK_VTABLE(NCPetitionWnd, 0x10185d00, 72)

/*-----------------------------------------------------------------------------
    NCGMPetitionWnd
-----------------------------------------------------------------------------*/

// 0x10056680 - NCWnd's constructor and the vtable
NCGMPetitionWnd::NCGMPetitionWnd()
{
}
NW_HOOK_CTOR(0x10056680, NCGMPetitionWnd)

// 0x100566d0
NW_HOOK_DTOR(0x100566d0, NCGMPetitionWnd)

// 0x100d0260 (vslot 25) - C2 no longer refuses the messages of a disabled window here
INT NCGMPetitionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_GMPETITION_START)
            return OnStart() != 0;
        if (wParam == NID_GMPETITION_END)
            return OnEnd() != 0;
        if (wParam == NID_GMPETITION_EDIT)
            return OnEdit((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100d0260, NCGMPetitionWnd, WindowProc)

// 0x100d3080 (vslot 37) - the same layout as the player's, with the buttons "Start" (474) and "End" (475)
INT NCGMPetitionWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_ch3.PetitionWnd.petition2_back"));
    CreatePetitionFrame(this, NID_GMPETITION_FRAME);
    ListBox = CreatePetitionList(this, NID_GMPETITION_LIST);
    EditBox = CreatePetitionEdit(this, NID_GMPETITION_EDIT);
    StartButton = CreatePetitionButton(this, NID_GMPETITION_START, 80, TEXT("L2UI_ch3.button.btn1_normal"),
                                       TEXT("L2UI_ch3.button.btn1_normalon"));
    StartButton->SetText(*GL2GameData.SysString[474]);
    EndButton = CreatePetitionButton(this, NID_GMPETITION_END, 160, TEXT("L2UI_ch3.button.btn1_normal"),
                                     TEXT("L2UI_ch3.button.btn1_normalon"));
    EndButton->SetText(*GL2GameData.SysString[475]);
    return 1;
}
NW_HOOK_METHOD(0x100d3080, NCGMPetitionWnd, OnCreate)

// 0x100d0da0 (vslot 38) - the list, the line and the two buttons go; the title bar is left alone
INT NCGMPetitionWnd::OnDestroy()
{
    guard(NCGMPetitionWnd::OnDestroy);
    DestroyChild(this, ListBox);
    DestroyChild(this, EditBox);
    DestroyChild(this, StartButton);
    DestroyChild(this, EndButton);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100d0da0, NCGMPetitionWnd, OnDestroy)

// 0x100aa860 (vslot 44) - NCSystemMenuWnd's body, shared
#if NW_HOOK_MODE
NW_NAKED void NCGMPetitionWnd::ShowWindow() NW_STOCK(0x100aa860)
#else
NW_SLOT_IS("?ShowWindow@NCGMPetitionWnd@@UAEXXZ", "?ShowWindow@NCSystemMenuWnd@@UAEXXZ")   // NCGMPetitionWnd::ShowWindow = NCSystemMenuWnd::ShowWindow (0x100aa860)
#endif

// 0x100412d0 (vslot 45) - NCChatWnd's body, shared: NCWnd::HideWindow
#if NW_HOOK_MODE
NW_NAKED void NCGMPetitionWnd::HideWindow() NW_STOCK(0x100412d0)
#else
NW_SLOT_IS("?HideWindow@NCGMPetitionWnd@@UAEXXZ", "?HideWindow@NCChatWnd@@UAEXXZ")   // NCGMPetitionWnd::HideWindow = NCChatWnd::HideWindow (0x100412d0)
#endif

// 0x100d0c20 (vslot 56) - the body of both windows
INT NCGMPetitionWnd::OnPaint(NCGDevice* Device)
{
    return PaintPetitionWnd(this, BackTex, Device);
}
NW_HOOK_METHOD(0x100d0c20, NCGMPetitionWnd, OnPaint)

// 0x100d0300 (vslot 68) - Enter: a "//" line goes as a builder command, a plain one as the GM's petition chat
// (type 7), another /command nowhere; the line is emptied unless it was empty. A line of 255 characters or more is
// refused with a message in the console's petition window (sysmsg 971), leaving the line as it is
INT NCGMPetitionWnd::OnEdit(LPARAM lParam)
{
    if (lParam != 1)
        return 1;
    const TCHAR* Text = EditBox->CommitText();
    if (!Text)
        return 1;
    if (appStrlen(Text) >= 255)
    {
        static_cast<NCPetitionWnd*>(GConsole->PetitionWnd)->AddMessage(
            (TCHAR*)*GL2GameData.SystemMsg[971].Message, 0xFFDCDCDC, 0, 0);
        return 1;
    }
    TCHAR Line[1024];
    appStrcpy(Line, Text);
    NCommandParser Parser;
    INT Command = Parser.GetCommand(Line);
    if (Command == NCMD_Builder)
    {
        L2ParamStack Params(10);
        Params.PushBack(Line + 2);
        GConsole->BuilderCommand(Params);
    }
    else if (Command == NCMD_Chat)
        GConsole->SendChatMsgToServer(CHAT_GMPetition, (TCHAR*)Text, NULL);
    EditBox->ClearText();
    return 1;
}
NW_HOOK_METHOD(0x100d0300, NCGMPetitionWnd, OnEdit)

// 0x100d04d0 (vslot 69)
INT NCGMPetitionWnd::OnStart()
{
    L2ParamStack Params(10);
    Params.PushBack((void*)TEXT("startpet"));
    GConsole->BuilderCommand(Params);
    return 1;
}
NW_HOOK_METHOD(0x100d04d0, NCGMPetitionWnd, OnStart)

// 0x100d0550 (vslot 70)
INT NCGMPetitionWnd::OnEnd()
{
    L2ParamStack Params(10);
    Params.PushBack((void*)TEXT("endpet"));
    GConsole->BuilderCommand(Params);
    return 1;
}
NW_HOOK_METHOD(0x100d0550, NCGMPetitionWnd, OnEnd)

// stock body shared with NCPetitionWnd's (0x10094480)
void NCGMPetitionWnd::AddMessage(TCHAR* Text, DWORD Color, INT Type, INT Param)
{
    AddPetitionMessage(ListBox, Text, Color, Type);
}

// stock body shared with NCPetitionWnd's (0x100944c0)
void NCGMPetitionWnd::Clear()
{
    EditBox->ClearText();
    ListBox->Clear();
}

NW_CHECK_VTABLE(NCGMPetitionWnd, 0x10183568, 71)
