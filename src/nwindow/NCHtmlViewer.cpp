/*=============================================================================
    NCHtmlViewer.cpp - the HTML viewer and NCTutorialHtmlViewer. Stock: 0x10025f50 ..
    0x1002be63, interleaved with the page tree (NHtmlLayout.cpp) and the board viewers.
=============================================================================*/
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NHtmlLayout.h"
#include "nwindow/NCActionLinker.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCCreditWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCDynamicLinker.h"
#include "nwindow/NCFileLinker.h"
#include "nwindow/NCGaraDialogBox.h"
#include "nwindow/NCHtmlButton.h"
#include "nwindow/NCHtmlComboBox.h"
#include "nwindow/NCHtmlEdit.h"
#include "nwindow/NCMultiEdit.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCTutorialViewerWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCrt.h"

#include <new>
#include <string.h>
#include <wchar.h>

// NCHtmlViewer::FileLinkID .. ComboBoxID
enum { NID_HTML_FIRST = (INT)0xFF00FF00 };

// the code a click in an edit box sends: MAKELONG(100, its y) (NCHtmlEdit::OnLButtonDown)
enum { HTML_SCROLLTO = 100 };

// style bit 0x200 of the page controls (meaning unknown); C2's link controls also set 0x4 (name ours)
enum { NWS_HTMLCONTROL = 0x200, NWS_LINK = 0x4 };

// dialog of NCActionLinker's question (NCGaraDialogBox::DialogID)
enum { DIALOG_ActionLink = 0x3C };   // C2 (C1: 0x38)

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_Click = 0 };

// systemmsg lines (names ours)
enum
{
    MSG_IncorrectName = 204,           // "Incorrect name. Please try again. "
    MSG_TooLong3000 = 329,             // "Please input contents using less than 3000 characters."
};

// the console's "bypass" command (NConsoleWnd::RunHtmlCommand, vslot 402: 2 closes the page); called by the slot
// number: the console's header declares it further down than the C3 vtable has it
static FORCEINLINE INT RunHtmlCommand(TCHAR* Command, NCHtmlViewer* Viewer)
{
    return ::nw::VirtualCall<INT>(GConsole, 402, Command, Viewer);
}

// <html imgsrc> absent: no background. The stock code tests this pointer, not the text.
static const TCHAR HtmlTransparent[] = TEXT("Transparent");

// a new page control is shown only when its top is on the page's visible part (repeated in
// every Add* of the stock code). C3: the top is read through GetLocalPos (the local rectangle is gone) and the
// viewer's height is a FLOAT
static FORCEINLINE void ShowIfVisible(NCHtmlViewer* Viewer, NControl* Control)
{
    INT Pos[2];
    Control->GetLocalPos(Pos);
    if (Pos[1] <= (INT)Viewer->Height && Pos[1] >= 0)
        Control->ShowWindow();
    else
        Control->HideWindow();
}

/*-----------------------------------------------------------------------------
    NCHtmlViewer.
-----------------------------------------------------------------------------*/

// 0x10039260 - the alignment stacks of the parser get their bottom 0
NCHtmlViewer::NCHtmlViewer()
{
    BackTex = NULL;
    Unknown100 = 0;
    UnknownFC = 0;
    CreateWidth = 0;
    CreateHeight = 0;
    if (GHtmlAlignStack.Num() <= 0)
        GHtmlAlignStack.AddItem(0);
    if (GHtmlVAlignStack.Num() <= 0)
        GHtmlVAlignStack.AddItem(0);
    bLoaded = 0;
    Unknown100 = 0;
    UnknownFC = 0;
    Owner = NULL;
    Doc = NULL;
    FontType = 0;
    FileLinkID = NID_HTML_FIRST;
    ActionLinkID = NID_HTML_FIRST + 1;
    ButtonID = NID_HTML_FIRST + 2;
    EditID = NID_HTML_FIRST + 3;
    MultiEditID = NID_HTML_FIRST + 4;
    ComboBoxID = NID_HTML_FIRST + 5;
    Unknown0C8 = 16;
}
NW_HOOK_CTOR(0x10039260, NCHtmlViewer)

// 0x100390d0 - Controls is TArray<NControl*>'s (0x10037ec0)
NW_HOOK_DTOR(0x100390d0, NCHtmlViewer)

// 0x10039390 - the same with the font the page is drawn in (the credit window's viewer)
NCHtmlViewer::NCHtmlViewer(INT InFontType)
{
    BackTex = NULL;
    Unknown100 = 0;
    UnknownFC = 0;
    CreateWidth = 0;
    CreateHeight = 0;
    if (GHtmlAlignStack.Num() <= 0)
        GHtmlAlignStack.AddItem(0);
    if (GHtmlVAlignStack.Num() <= 0)
        GHtmlVAlignStack.AddItem(0);
    bLoaded = 0;
    Unknown100 = 0;
    UnknownFC = 0;
    Owner = NULL;
    Doc = NULL;
    FontType = InFontType;
    FileLinkID = NID_HTML_FIRST;
    ActionLinkID = NID_HTML_FIRST + 1;
    ButtonID = NID_HTML_FIRST + 2;
    EditID = NID_HTML_FIRST + 3;
    MultiEditID = NID_HTML_FIRST + 4;
    ComboBoxID = NID_HTML_FIRST + 5;
    Unknown0C8 = 16;
}
NW_HOOK_CTOR_ARGS(0x10039390, NCHtmlViewer, (INT A), (A))

// 0x10035260 (vslot 25) - each kind of page control has its id
INT NCHtmlViewer::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCScrollWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (FileLinkID == (INT)wParam)
            return OnFileLink((TCHAR*)lParam) != 0;
        if (ActionLinkID == (INT)wParam)
            return OnActionLink((TCHAR*)lParam) != 0;
        if (ButtonID == (INT)wParam)
            return OnButton((TCHAR*)lParam) != 0;
        if (EditID == (INT)wParam)
            return OnEdit(lParam) != 0;
        if (MultiEditID == (INT)wParam)
            return OnMultiEdit(lParam) != 0;
        if (ComboBoxID == (INT)wParam)
            return OnComboBox(lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10035260, NCHtmlViewer, WindowProc)

// 0x10035360 (vslot 36)
INT NCHtmlViewer::OnCreate()
{
    NCScrollWnd::OnCreate();
    CreateHeight = Height;
    CreateWidth = Width;
    // C3: one line of the page is a line of text in the font: the scroll step
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, TEXT("A"), 0, 0, 0, 0, 0, L2FONT_Normal);
    LineStep = Size.cy;
    return 1;
}
NW_HOOK_METHOD(0x10035360, NCHtmlViewer, OnCreate)

// 0x100394c0 (vslot 37) - the page and its controls go
INT NCHtmlViewer::OnDestroy()
{
    guard(NCHtmlViewer::OnDestroy);
    if (bLoaded && Doc)
    {
        Doc->Release();
        if (Doc)
            delete Doc;
        Doc = NULL;
    }
    INT N = Controls.Num();
    for (INT i = 0; i < N; i++)
    {
        NControl* Control = Controls(i);
        if (Control->Entry && Control)
        {
            RemoveChildWindow(Control->Entry);
            delete Control;
        }
    }
    Controls.Empty();
    NCScrollWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100394c0, NCHtmlViewer, OnDestroy)

// 0x10036840 (vslot 44) - the controls below the visible part stay hidden
void NCHtmlViewer::ShowWindow()
{
    NCWnd::ShowWindow();
    INT N = Controls.Num();
    NRectF R;
    R.X = 0;
    R.Y = 0;
    R.W = 0.0f;
    R.H = 0.0f;
    for (INT i = 0; i < N; i++)
    {
        NControl* Control = Controls(i);
        INT Pos[2];
        Control->GetLocalPos(Pos);
        Control->GetLocalRect(&R);
        if ((INT)((FLOAT)ScrollY + Height) < Pos[1])
            Control->HideWindow();
        else
            Control->ShowWindow();
    }
}
NW_HOOK_METHOD(0x10036840, NCHtmlViewer, ShowWindow)

// 0x100376f0 (vslot 56) - background (the scroll bar's width of the texture left out while the
// bar shows), the page shifted by the scroll position, whose height sets the scroll range. The
// clip goes on before BeginPaint.
INT NCHtmlViewer::OnPaint(NCGDevice* Device)
{
    guard(NCHtmlViewer::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
    {
        INT SourceW;
        if (ScrollUpButton->IsShowWindow())
            SourceW = (INT)(Width - (FLOAT)Unknown0C8);
        else
            SourceW = (INT)Width;
        Device->DrawTexture(0, 0, (INT)Width, (INT)Height, 0, 0, SourceW, (INT)Height, BackTex, 0xFF, 1);
    }
    Device->PushOrigin(ScrollX, ScrollY);
    if (bLoaded)
    {
        if (Doc)
            Doc->Draw(Device);
        if (bLoaded && Doc)
            SetContentHeight(Doc->GetContentHeight());
    }
    Device->PopOrigin();
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100376f0, NCHtmlViewer, OnPaint)

// 0x100356e0 (vslot 62) - a click on the credits page reaches the credit window (its handler is
// the NewUI group's: stock call)
INT NCHtmlViewer::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    NCWnd* Credit = GConsole->CreditWnd;
    if (Entry->Parent->Wnd == Credit)
        ((NCCreditWnd*)Credit)->NextPage();   // 0x1006E360
    return 1;
}
NW_HOOK_METHOD(0x100356e0, NCHtmlViewer, OnLButtonDown)

// 0x10035710 (vslot 59) - Escape on the credits page closes the credit window and goes back to the login screen:
// the login window and the control of the console's message window show, the credit window hides and loses its
// focus, the audio subsystem is told (C3: its slot 52 with 0, 0, the console's slot 323, the audio's slot 36 with a
// number of the credit window), the credit window cleans up (the NewUI group's). Every key then goes to NCWnd's
// handler (not NCScrollWnd's, so the page does not scroll from the keyboard).
INT NCHtmlViewer::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    NCWnd* Credit = GConsole->CreditWnd;
    if (Entry->Parent->Wnd == Credit && wParam == VK_ESCAPE)
    {
        // the window at +0x3950 of the console (C2's MessageWnd, the line under the login screens) holds a control at +0xD0
        if (GConsole->LoginWnd && ::nw::Field<NCWnd*>(::nw::Field<NCWnd*>(GConsole, 0x3950), 0xD0) != NULL && Credit)
        {
            ((NCWnd*)GConsole->LoginWnd)->ShowWindow();
            ::nw::Field<NCWnd*>(::nw::Field<NCWnd*>(GConsole, 0x3950), 0xD0)->ShowWindow();
            GConsole->CreditWnd->HideWindow();
            GConsole->CreditWnd->KillFocus();
            ::nw::Field<INT>(GConsole->CreditWnd, 0x1E0) = 0;
            ::nw::Field<INT>(GConsole->CreditWnd, 0x1DC) = 0;
            ((NCCreditWnd*)GConsole->CreditWnd)->FreeFont();   // 0x100791C0
        }
        if (GConsole->Engine && GConsole->Engine->Audio)
        {
            ::nw::VirtualCall<void>(GConsole->Engine->Audio, 52, 0, 0);
            ::nw::VirtualCall<void>(GConsole, 323);
            ::nw::VirtualCall<void>(GConsole->Engine->Audio, 36, ::nw::Field<INT>(GConsole->CreditWnd, 0xBDC));
        }
    }
    NCWnd::OnKeyDown(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x10035710, NCHtmlViewer, OnKeyDown)

// 0x10036660 (vslot 67) - the page's controls follow every scroll
INT NCHtmlViewer::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    NCScrollWnd::OnMouseWheel(wParam, lParam);
    ScrollControls(0, DeltaY);
    return 1;
}
NW_HOOK_METHOD(0x10036660, NCHtmlViewer, OnMouseWheel)

// 0x100365f0 (vslot 69)
INT NCHtmlViewer::ScrollUp()
{
    NCScrollWnd::ScrollUp();
    ScrollControls(0, DeltaY);
    return 1;
}
NW_HOOK_METHOD(0x100365f0, NCHtmlViewer, ScrollUp)

// 0x10036610 (vslot 70)
INT NCHtmlViewer::ScrollDown()
{
    NCScrollWnd::ScrollDown();
    ScrollControls(0, DeltaY);
    return 1;
}
NW_HOOK_METHOD(0x10036610, NCHtmlViewer, ScrollDown)

// 0x10036630 (vslot 69)
INT NCHtmlViewer::OnScrollBar(INT Value)
{
    NCScrollWnd::OnScrollBar(Value);
    ScrollControls(0, DeltaY);
    return 1;
}
NW_HOOK_METHOD(0x10036630, NCHtmlViewer, OnScrollBar)

// 0x10036690 (vslot 70) - the value of the control whose caption (variable) is Name, for a
// $Name in a command. A one-line box must hold a name: 1 to 16 bytes in the ANSI code page,
// characters the font has and no '&'; a multi-line box 2 to 3000 bytes. Anything else
// shows a system message and gives NULL.
TCHAR* NCHtmlViewer::GetControlValue(const TCHAR* Name)
{
    ANSICHAR Ansi[800];   // C3: 0x320 bytes (C2 512)
    if (!Name)
        return NULL;
    INT N = Controls.Num();
    for (INT i = 0; i < N; i++)
    {
        NControl* Control = Controls(i);
        const TCHAR* Caption = Control->GetCaption();
        if (!Caption || wcscmp(Caption, Name))
            continue;

        if (!Control->GetValue())
            return NULL;
        if (!Controls(i)->IsA(NClass::NCHtmlMultiEdit()))
        {
            if (GConsole->HasInvalidChars((TCHAR*)Control->GetValue()) ||
                !GConsole->HasNoAmpersand((TCHAR*)Control->GetValue()))
            {
                GConsole->ShowSystemMessage(MSG_IncorrectName, 0xFFB09B79);
                return NULL;
            }
        }
        WideCharToMultiByte(CP_ACP, 0, Control->GetValue(), -1, Ansi, 800, NULL, NULL);
        INT Length = (INT)strlen(Ansi);
        if (Controls(i)->IsA(NClass::NCHtmlMultiEdit()))
        {
            if (Length <= 1 || Length > 3000)
            {
                GConsole->ShowSystemMessage(MSG_TooLong3000, 0xFFB09B79);
                return NULL;
            }
        }
        if (!Controls(i)->IsA(NClass::NCHtmlMultiEdit()) && (Length <= 0 || Length > 16))
        {
            GConsole->ShowSystemMessage(MSG_IncorrectName, 0xFFB09B79);
            return NULL;
        }
        return (TCHAR*)Control->GetValue();
    }
    return NULL;
}
NW_HOOK_METHOD(0x10036690, NCHtmlViewer, GetControlValue)

// 0x1003be30 (vslot 73) - <a link>: another page from a file
INT NCHtmlViewer::OnFileLink(TCHAR* File)
{
    wcscpy(GHtmlLinkTarget, File);
    if (GHtmlLinkTarget[0])
        LoadHtmlFile(GHtmlLinkTarget);
    return 1;
}
NW_HOOK_METHOD(0x1003be30, NCHtmlViewer, OnFileLink)

// 0x10035420 (vslot 72) - <a action>: the command, a click sound
INT NCHtmlViewer::OnActionLink(TCHAR* Action)
{
    wcscpy(GHtmlLinkTarget, Action);
    if (GHtmlLinkTarget[0])
    {
        if (RunHtmlCommand(GHtmlLinkTarget, this) == 2)
        {
            if (Owner)
                Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
            else
                HideWindow();
        }
        PlayInterfaceSound(SOUND_Click);
    }
    return 1;
}
NW_HOOK_METHOD(0x10035420, NCHtmlViewer, OnActionLink)

// 0x100354a0 (vslot 73) - <button action> (no emptiness test, no sound)
INT NCHtmlViewer::OnButton(TCHAR* Action)
{
    wcscpy(GHtmlLinkTarget, Action);
    if (RunHtmlCommand(GHtmlLinkTarget, this) == 2)
    {
        if (Owner)
        {
            Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
            return 1;
        }
        HideWindow();
    }
    return 1;
}
NW_HOOK_METHOD(0x100354a0, NCHtmlViewer, OnButton)

// 0x10035510 (vslot 74) - from an edit box: a click scrolls it into view when it is above
// the visible part; Enter runs its action
INT NCHtmlViewer::OnEdit(LPARAM Param)
{
    if ((SWORD)LOWORD(Param) == HTML_SCROLLTO)
    {
        INT EditY = (SWORD)HIWORD(Param);
        if (EditY < 0)
        {
            OnScrollBar(EditY - ScrollY);
            ScrollBar->SetScrollPos(-ScrollY);
        }
        return 1;
    }
    TCHAR* Action = (TCHAR*)Param;
    if (Action && *Action)
    {
        wcscpy(GHtmlLinkTarget, Action);
        if (RunHtmlCommand(GHtmlLinkTarget, this) == 2)
        {
            if (Owner)
            {
                Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
                return 1;
            }
            HideWindow();
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10035510, NCHtmlViewer, OnEdit)

// 0x100355d0 (vslot 75) - the same for a multi-line box (an empty action runs too)
INT NCHtmlViewer::OnMultiEdit(LPARAM Param)
{
    if ((SWORD)LOWORD(Param) == HTML_SCROLLTO)
    {
        INT EditY = (SWORD)HIWORD(Param);
        if (EditY < 0)
        {
            OnScrollBar(EditY - ScrollY);
            ScrollBar->SetScrollPos(-ScrollY);
        }
        return 1;
    }
    TCHAR* Action = (TCHAR*)Param;
    if (Action)
    {
        wcscpy(GHtmlLinkTarget, Action);
        if (RunHtmlCommand(GHtmlLinkTarget, this) == 2)
        {
            if (Owner)
            {
                Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
                return 1;
            }
            HideWindow();
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100355d0, NCHtmlViewer, OnMultiEdit)

// 0x10035690 (vslot 76) - from a combo box: only the scroll into view
INT NCHtmlViewer::OnComboBox(LPARAM Param)
{
    INT BoxY = (SWORD)HIWORD(Param);
    if (LOWORD(Param) == HTML_SCROLLTO && BoxY < 0)
    {
        OnScrollBar(BoxY - ScrollY);
        ScrollBar->SetScrollPos(-ScrollY);
    }
    return 1;
}
NW_HOOK_METHOD(0x10035690, NCHtmlViewer, OnComboBox)

// 0x100353b0 - NULL leaves the old one
void NCHtmlViewer::SetBackTexture(const TCHAR* Name)
{
    if (Name)
    {
        if (Name == HtmlTransparent)
            BackTex = NULL;
        else
            BackTex = FindTexture(Name);
    }
}
NW_HOOK_METHOD(0x100353b0, NCHtmlViewer, SetBackTexture)

// 0x100353f0 - the window a closing command goes to. One stock body for the setters of the +0x114 field of the grids
// (NCManorShopWnd, NCPrivateBuyWnd, NCPrivateShopWnd, NCRecipeShopWnd, NCShopWnd, NCStoreWnd: their SetMode): this class
// keeps the address
void NCHtmlViewer::SetOwner(NWndEntry* InOwner)
{
    Owner = InOwner;
}
NW_HOOK_METHOD(0x100353f0, NCHtmlViewer, SetOwner)

// 0x10035400 - <img src>; no name is the black texture
UTexture* NCHtmlViewer::FindPageTexture(const TCHAR* Name)
{
    if (!Name)
        Name = TEXT("NWindow.BlackTexture");
    return FindTexture(Name);
}
NW_HOOK_METHOD(0x10035400, NCHtmlViewer, FindPageTexture)

// 0x10036540 - every control moves with the page and is shown only while on its visible part
void NCHtmlViewer::ScrollControls(INT DX, INT DY)
{
    INT N = Controls.Num();
    NRectF R;
    R.X = 0;
    R.Y = 0;
    R.W = 0.0f;
    R.H = 0.0f;
    for (INT i = 0; i < N; i++)
    {
        Controls(i)->MoveWindow(DX, -DY);
        Controls(i)->GetLocalRect(&R);
        if (R.Y > (INT)Height || (INT)R.H + R.Y < 0)
            Controls(i)->HideWindow();
        else
            Controls(i)->ShowWindow();
    }
}
NW_HOOK_METHOD(0x10036540, NCHtmlViewer, ScrollControls)

// The link control (an NCDynamicLinker, 0x224 bytes) takes the item's whole geometry; its thirteen-argument
// constructor is 0x10045070 (C3: the last argument is 1 for a file link, 0 for an action link; the class and its
// constructor are not ported: stock code). The two before it are the target's string and the message of an action
// link (the constructor keeps both pointers: the code is the string the page gave).
static FORCEINLINE NControl* NewDynamicLinker(INT AbsX, INT AbsY, INT X, INT Y, INT W, INT H, INT LayoutWidth, INT TailWidth,
                                  INT ParentAbsX, INT ParentAbsY, TCHAR* Target, TCHAR* Msg, INT bFile)
{
    return new NCDynamicLinker(AbsX, AbsY, X, Y, W, H, LayoutWidth, TailWidth, ParentAbsX, ParentAbsY, Target, Msg,
                               bFile);
}

// 0x10037880 - <a link>: a control over the item that hands the page to load to the viewer. C3: the call takes the
// message too (as AddActionLink)
INT NCHtmlViewer::AddFileLink(INT AbsX, INT AbsY, INT X, INT Y, INT W, INT H, INT LayoutWidth,
                              INT TailWidth, INT ParentAbsX, INT ParentAbsY, TCHAR* File, TCHAR* Msg)
{
    NControl* Link = NewDynamicLinker(AbsX, AbsY, X, Y, W, H, LayoutWidth, TailWidth, ParentAbsX,
                                      ParentAbsY, File, Msg, 1);
    Link->ID = FileLinkID;
    Link->SetPosition(AbsX, AbsY, 0, 0);
    CreateChild(Link, NWS_HTMLCONTROL | NWS_VISIBLE | NWS_LINK);
    Controls.AddItem(Link);
    ShowIfVisible(this, Link);
    return 1;
}
NW_HOOK_METHOD(0x10037880, NCHtmlViewer, AddFileLink)

// 0x100379c0 - <a action msg>: the same for a command (msg is a systemmsg line asked first)
INT NCHtmlViewer::AddActionLink(INT AbsX, INT AbsY, INT X, INT Y, INT W, INT H, INT LayoutWidth,
                                INT TailWidth, INT ParentAbsX, INT ParentAbsY, TCHAR* Action,
                                TCHAR* Msg)
{
    NControl* Link = NewDynamicLinker(AbsX, AbsY, X, Y, W, H, LayoutWidth, TailWidth, ParentAbsX,
                                      ParentAbsY, Action, Msg, 0);
    Link->ID = ActionLinkID;
    Link->SetPosition(AbsX, AbsY, 0, 0);
    CreateChild(Link, NWS_HTMLCONTROL | NWS_VISIBLE | NWS_LINK);
    Controls.AddItem(Link);
    ShowIfVisible(this, Link);
    return 1;
}
NW_HOOK_METHOD(0x100379c0, NCHtmlViewer, AddActionLink)

// 0x10037b00 - <button>: a label "&$n;" is sysstring n
INT NCHtmlViewer::AddButton(INT X, INT Y, INT W, INT H, TCHAR* NormalTexName,
                            TCHAR* PushedTexName, TCHAR* Action, TCHAR* Label)
{
    TCHAR Head[8];
    NCHtmlButton* Button = new NCHtmlButton(NormalTexName, PushedTexName, Action, 0);
    Button->ID = ButtonID;
    Button->SetPosition(X, Y, W, H);
    CreateChild(Button, NWS_HTMLCONTROL | NWS_VISIBLE);
    if (Label)
    {
        INT Length = appStrlen(Label);
        appStrncpy(Head, Label, 3);
        if (!appStrcmp(Head, TEXT("&$")) && Label[Length - 1] == ';')
            Button->SetTextID(appAtoi(Label + 2));
        else
            Button->SetText(Label);
    }
    ShowIfVisible(this, Button);
    Controls.AddItem(Button);
    return 1;
}
NW_HOOK_METHOD(0x10037b00, NCHtmlViewer, AddButton)

// 0x10037c70 - <multiedit var>
INT NCHtmlViewer::AddMultiEdit(INT X, INT Y, INT W, INT H, TCHAR* Var)
{
    NCHtmlMultiEdit* Edit = new NCHtmlMultiEdit;
    Edit->ID = MultiEditID;
    Edit->SetPosition(X, Y, W, H);
    CreateChild(Edit, NWS_HTMLCONTROL | NWS_VISIBLE);
    Edit->SetCaption(Var);
    ShowIfVisible(this, Edit);
    Controls.AddItem(Edit);
    return 1;
}
NW_HOOK_METHOD(0x10037c70, NCHtmlViewer, AddMultiEdit)

// 0x10037d70 - <edit var type action length>: Enter sends the action; a length sets NCEditBox's limit
INT NCHtmlViewer::AddEdit(INT X, INT Y, INT W, INT H, TCHAR* Var, INT Type, TCHAR* Action, INT Length)
{
    NCHtmlEdit* Edit = new NCHtmlEdit(100, Type);
    Edit->SetPosition(X, Y, W, H);
    CreateChild(Edit, NWS_HTMLCONTROL | NWS_VISIBLE);
    Edit->ID = EditID;
    Edit->SetCaption(Var);
    Edit->EnterCommand = Action;
    Edit->SetMaxLength(Length);
    ShowIfVisible(this, Edit);
    Controls.AddItem(Edit);
    return 1;
}
NW_HOOK_METHOD(0x10037d70, NCHtmlViewer, AddEdit)

// 0x100395d0 - <combobox var list sel>: always left hidden (ShowWindow shows it)
INT NCHtmlViewer::AddComboBox(INT X, INT Y, INT W, INT H, TCHAR* Var, TCHAR* List, TCHAR* Sel)
{
    NCHtmlComboBox* Combo = new NCHtmlComboBox;
    Combo->SetPosition(X, Y, W, H);
    CreateChild(Combo, 0x12);   // C3: 0x12 (the other page controls take NWS_HTMLCONTROL | NWS_VISIBLE)
    Combo->SetCaption(Var);
    Combo->ID = ComboBoxID;
    Combo->SetList(List, Sel);
    ShowIfVisible(this, Combo);
    Combo->HideWindow();
    Controls.AddItem(Combo);
    return 1;
}
NW_HOOK_METHOD(0x100395d0, NCHtmlViewer, AddComboBox)

// 0x10038a20 - back to an empty page. The controls are taken off the window but not deleted;
// an open question of an action link is closed.
void NCHtmlViewer::Clear()
{
    if (bLoaded && Doc)
    {
        Doc->Release();
        if (Doc)
            delete Doc;
        Doc = NULL;
    }
    INT N = Controls.Num();
    for (INT i = 0; i < N; i++)
    {
        if (Controls(i)->IsA(NClass::NCActionLinker()))
        {
            NCGaraDialogBox* Box = (NCGaraDialogBox*)GDesktop->MainDialogBox;   // C2: a CGaraDialogBox
            if (Box->DialogID == DIALOG_ActionLink)
            {
                Box->HideWindow();
                Box->Reset();   // 0x10006740
            }
        }
        RemoveChildWindow(Controls(i)->Entry);
    }
    Controls.Empty();
    bLoaded = 0;
    ScrollY = 0;
    if (ScrollBar)
        ScrollBar->SetScrollPos(0);
    DeltaY = 0;
}
NW_HOOK_METHOD(0x10038a20, NCHtmlViewer, Clear)

// 0x1003bb00 - the tree from the tag list: <html> (whose imgsrc is the background) or <body>
// starts the page, which runs to its closing tag; laid out at the width without the scroll
// bar. TAB then goes through the controls in page order and back to the viewer.
void NCHtmlViewer::BuildDocument(NHtmlTagList* Tags)
{
    NHtmlTag** It = Tags->First;
    while (It != Tags->Last)
    {
        NHtmlTag* Tag = *It;
        if (Tag->Tag == HTMLTAG_Html)
        {
            SetBackTexture(Tag->HasAttr(ATTR_Html_ImgSrc) ? Tag->GetAttr(ATTR_Html_ImgSrc, 0)
                                                          : HtmlTransparent);
        }
        else if (Tag->Tag != HTMLTAG_Body)
        {
            It++;
            continue;
        }
        void* Mem = appMalloc(sizeof(NHtmlBlock), TEXT("new"));
        Doc = Mem ? ::new (Mem) NHtmlBlock(NULL, this, FontType) : NULL;
        Doc->Parse(&It, 0);
    }
    if (!Doc)
        return;

    // C3: no tab order of the controls (C2 chained them with SetTabOrder); the sizes are FLOATs
    Doc->Layout((INT)(Width - 16.0f), (INT)Height);
    Doc->Arrange((INT)(Width - 16.0f), (INT)Height);
}
NW_HOOK_METHOD(0x1003bb00, NCHtmlViewer, BuildDocument)

// 0x1003bc30 - a page from a file (links, the tutorial)
INT NCHtmlViewer::LoadHtmlFile(TCHAR* Source)
{
    NHtmlParser Parser;
    TCHAR* Text = Parser.LoadFile(Source);
    if (!Text)
    {
        Debugf(TEXT("Can not open Html file : %s"), Source);
        return 0;
    }
    if (bLoaded)
        Clear();
    NHtmlTagList* Tags = Parser.Parse(Text);
    BuildDocument(Tags);
    Parser.FreeTags(Tags);
    Parser.FreeFile();
    bLoaded = 1;
    return 1;
}
NW_HOOK_METHOD(0x1003bc30, NCHtmlViewer, LoadHtmlFile)

// 0x1003bd20 - a page from text (the server's NPC dialogs)
INT NCHtmlViewer::LoadHtml(TCHAR* Text)
{
    NHtmlParser Parser;
    if (!Text)
        return 0;
    if (bLoaded)
        Clear();
    NHtmlTagList* Tags = Parser.Parse(Text);
    BuildDocument(Tags);
    Parser.FreeTags(Tags);
    bLoaded = 1;
    if (Doc)
    {
        // C3: a viewer inside the tutorial window makes the window fit the page (0x101098B0, the window's own)
        INT ContentHeight = Doc->GetContentHeight();
        NCWnd* ParentWnd = Entry->Parent->Wnd;
        if (ParentWnd->IsA(NClass::NCTutorialViewerWnd()))
            ((NCTutorialViewerWnd*)ParentWnd)->FitToPage(ContentHeight);
    }
    return 1;
}
NW_HOOK_METHOD(0x1003bd20, NCHtmlViewer, LoadHtml)

NW_CHECK_VTABLE(NCHtmlViewer, 0x1017f248, 79)

/*-----------------------------------------------------------------------------
    NCTutorialHtmlViewer.
-----------------------------------------------------------------------------*/

// 0x100396f0 - NCHtmlViewer's constructor and its own vtable
NCTutorialHtmlViewer::NCTutorialHtmlViewer()
{
}
NW_HOOK_CTOR(0x100396f0, NCTutorialHtmlViewer)

// 0x10039740
NW_HOOK_DTOR(0x10039740, NCTutorialHtmlViewer)


// 0x10035850 (vslot 25) - NCHtmlViewer's, as a bool
INT NCTutorialHtmlViewer::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCHtmlViewer::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x10035850, NCTutorialHtmlViewer, WindowProc)

// 0x10035870 (vslot 35)
INT NCTutorialHtmlViewer::OnCreate()
{
    NCHtmlViewer::OnCreate();
    return 1;
}
NW_HOOK_METHOD(0x10035870, NCTutorialHtmlViewer, OnCreate)

// 0x10039790 (vslot 37) - guarded in C2
INT NCTutorialHtmlViewer::OnDestroy()
{
    guard(NCTutorialHtmlViewer::OnDestroy);
    NCHtmlViewer::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10039790, NCTutorialHtmlViewer, OnDestroy)

NW_CHECK_VTABLE(NCTutorialHtmlViewer, 0x1017f4d0, 79)
