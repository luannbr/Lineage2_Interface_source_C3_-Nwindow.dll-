/*=============================================================================
    NCMainWnd.cpp - the character window (status, skills, actions, quests).
    Stock: 0x1003e720 .. 0x1003e7f7, 0x1003e8b0 .. 0x1003e94f, 0x10040f30 .. 0x100417e6.
=============================================================================*/
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCActionWnd.h"
#include "nwindow/NCDetailStatusWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCQuestWnd.h"
#include "nwindow/NCQuestGaraTreeWnd.h"
#include "nwindow/NCPledgeWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NClasses.gen.h"

// interface sounds (NConsoleWnd::PlayInterfaceSound; names ours): the character window opened and closed
enum { SOUND_CharOpen = 11, SOUND_CharClose = 12 };

// the title of each page (inlined twice in the stock code, name ours); other tabs keep it. C3: five pages, the clan's
// (SysString 439) is the fourth
static FORCEINLINE void SetTabTitle(NCFrameCtrl* Frame, INT Tab)
{
    if (Tab == MAINTAB_Status)
        Frame->SetTitleText(*NCObject::SysString(433));    // "Status"
    else if (Tab == MAINTAB_Skills)
        Frame->SetTitleText(*NCObject::SysString(119));    // "Skills"
    else if (Tab == MAINTAB_Actions)
        Frame->SetTitleText(*NCObject::SysString(127));    // "Actions"
    else if (Tab == MAINTAB_Pledge)
        Frame->SetTitleText(*NCObject::SysString(439));    // the clan's page
    else if (Tab == MAINTAB_Quests)
        Frame->SetTitleText(*NCObject::SysString(118));    // "Quest"
}

// 0x10056880 - an NCWnd with this class's vtable
NCMainWnd::NCMainWnd()
{
}
NW_HOOK_CTOR(0x10056880, NCMainWnd)

// 0x10053eb0 (vslot 25)
INT NCMainWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_MAIN_TABS)
        return OnTabChanged((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10053eb0, NCMainWnd, WindowProc)

// 0x10053f10 (vslot 68) - the title of the page the tab shows
INT NCMainWnd::OnTabChanged(INT Tab)
{
    SetTabTitle(FrameCtrl, Tab);
    return 1;
}
NW_HOOK_METHOD(0x10053f10, NCMainWnd, OnTabChanged)

// 0x10055140 (vslot 37) - the title bar (256 x 20), the back texture of the tab row and the five pages (all at 0, 66,
// 256 x 335: the status, the skills, the actions, the clan's - new in C3 - and the quests, which starts hidden), then
// the tab control (12, 25, 224 x 41) with a button of 44 x 41 for each. C3: the pages are of the stock's own constructors
INT NCMainWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_MAIN_FRAME;
    FrameCtrl->SetPosition(0, 0, 256, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI_CH3.TabButton.MainWndTabBack"));

    DetailWnd = new NCDetailStatusWnd;    // 0x10103830
    DetailWnd->SetPosition(0, 66, 256, 335);
    CreateChild(DetailWnd, NWS_VISIBLE);

    SkillWnd = new NCMagicSkillWnd;      // 0x100A26B0
    SkillWnd->SetPosition(0, 66, 256, 335);
    CreateChild(SkillWnd, NWS_VISIBLE);

    ActionWnd = new NCActionWnd;          // 0x1004AED0
    ActionWnd->SetPosition(0, 66, 256, 335);
    CreateChild(ActionWnd, NWS_VISIBLE);

    PledgeWnd = new NCPledgeWnd;          // 0x1004AE10
    PledgeWnd->SetPosition(0, 66, 256, 335);
    CreateChild(PledgeWnd, NWS_VISIBLE);

    QuestWnd = new NCQuestGaraTreeWnd;   // 0x100DC280
    QuestWnd->SetPosition(0, 66, 256, 335);
    {
        // registered, hidden, then attached (entry, screen position, WM_CREATE)
        NWndEntry* E = AddChildWindow(QuestWnd, NWS_VISIBLE | 0x20);
        QuestWnd->HideWindow();
        QuestWnd->Entry = E;
        if (E->Parent)
        {
            NCWnd* Owner = E->Parent->Wnd;
            QuestWnd->ScreenX += Owner->ScreenX;
            QuestWnd->ScreenY += Owner->ScreenY;
        }
        QuestWnd->WindowProc(WM_CREATE, 0, 0);
    }

    TabCtrl = new NCTabControl();
    TabCtrl->ID = NID_MAIN_TABS;
    TabCtrl->SetPosition(12, 25, 224, 41);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(TEXT(" "), DetailWnd, 44, 41, TEXT("L2UI_CH3.TabButton.MainWndTabBtn1"),
                    TEXT("L2UI_CH3.TabButton.MainWndTabBtn1On"), 1, 0xC2);
    TabCtrl->AddTab(TEXT(" "), SkillWnd, 44, 41, TEXT("L2UI_CH3.TabButton.MainWndTabBtn2"),
                    TEXT("L2UI_CH3.TabButton.MainWndTabBtn2On"), 1, 0xC4);
    TabCtrl->AddTab(TEXT(" "), ActionWnd, 44, 41, TEXT("L2UI_CH3.TabButton.MainWndTabBtn3"),
                    TEXT("L2UI_CH3.TabButton.MainWndTabBtn3On"), 1, 0xC5);
    TabCtrl->AddTab(TEXT(" "), PledgeWnd, 44, 41, TEXT("L2UI_CH3.TabButton.MainWndTabBtn4"),
                    TEXT("L2UI_CH3.TabButton.MainWndTabBtn4On"), 1, 0x37F);
    TabCtrl->AddTab(TEXT(" "), QuestWnd, 44, 41, TEXT("L2UI_CH3.TabButton.MainWndTabBtn5"),
                    TEXT("L2UI_CH3.TabButton.MainWndTabBtn5On"), 1, 0xC6);
    return 1;
}
NW_HOOK_METHOD(0x10055140, NCMainWnd, OnCreate)

// the pages and the tab control destroyed (the title bar goes with NCWnd::OnDestroy)
template <class T> static FORCEINLINE void DestroyPage(NCWnd* Parent, T*& Page)
{
    if (Page)
    {
        Parent->RemoveChildWindow(Page->Entry);
        delete Page;
        Page = NULL;
    }
}

// 0x10054900 (vslot 38)
INT NCMainWnd::OnDestroy()
{
    guard(NCMainWnd::OnDestroy);
    DestroyPage(this, DetailWnd);
    DestroyPage(this, SkillWnd);
    DestroyPage(this, ActionWnd);
    DestroyPage(this, PledgeWnd);
    DestroyPage(this, QuestWnd);
    DestroyPage(this, TabCtrl);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10054900, NCMainWnd, OnDestroy)

// 0x10054a80 - the page of a tab shown (the window opens on it, its title is set and the window takes the focus), or the
// window closed when that page is already the one shown (Alt + key); the icon it was minimized to gives its place back
void NCMainWnd::ShowTab(INT Tab)
{
    UBOOL bShown;
    switch (Tab)
    {
    case MAINTAB_Status:  bShown = DetailWnd->IsShowWindow(); break;
    case MAINTAB_Skills:  bShown = SkillWnd->IsShowWindow(); break;
    case MAINTAB_Actions: bShown = ActionWnd->IsShowWindow(); break;
    case MAINTAB_Pledge:  bShown = PledgeWnd->IsShowWindow(); break;
    case MAINTAB_Quests:  bShown = QuestWnd->IsShowWindow(); break;
    default: bShown = 0; break;
    }
    if (bShown)
    {
        GConsole->PlayInterfaceSound(SOUND_CharClose);
        HideWindow();
        return;
    }
    Style |= NWS_VISIBLE;
    FrameCtrl->ShowWindow();
    TabCtrl->ShowWindow();
    TabCtrl->SetTab(Tab, 1);
    SetTabTitle(FrameCtrl, Tab);
    if (Icon)
    {
        INT IconX = Icon->ScreenX;
        INT IconY = Icon->ScreenY;
        SetPosition(IconX, IconY, (INT)Width, (INT)Height);
        Icon->SetPosition(0, 0, 0, 0);
        Icon->HideWindow();
    }
    SetFocus();
    GConsole->PlayInterfaceSound(SOUND_CharOpen);
}
NW_HOOK_METHOD(0x10054a80, NCMainWnd, ShowTab)

// 0x1005a3a0 (vslot 44) - shown: the title bar, the tab control (with the page it keeps, 0x100565A0), and the place the
// minimized icon had; the icon goes
void NCMainWnd::ShowWindow()
{
    Style |= NWS_VISIBLE;
    FrameCtrl->ShowWindow();
    TabCtrl->ShowWindow();
    TabCtrl->ShowCurrentPage();             // slot 69 (0x114)
    UpdatePledgeTab();
    if (Icon)
    {
        INT IconX = Icon->ScreenX;
        INT IconY = Icon->ScreenY;
        SetPosition(IconX, IconY, (INT)Width, (INT)Height);
        Icon->SetPosition(0, 0, 0, 0);
        Icon->HideWindow();
    }
}
NW_HOOK_METHOD(0x1005a3a0, NCMainWnd, ShowWindow)

// 0x100565A0 (name ours; anonymous in the db) - the clan page's tab button (the fourth) enabled and the page that is
// current shown again, when the tab control has tabs at all; then the window hides itself unless it is shown (a window
// shown only through its parent's flag). The tab array is a copy (GetTabs), as in the stock code
void NCMainWnd::UpdatePledgeTab()
{
    TArray<NCRadioButton*> Tabs = TabCtrl->GetTabs();
    if (Tabs.Num() > 0)
    {
        Tabs(MAINTAB_Pledge)->EnableWindow(1);
        TabCtrl->ShowCurrentPage();
    }
    if (!IsShowWindow())
        NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x100565A0, NCMainWnd, UpdatePledgeTab)

// 0x10054c10 (vslot 56) - the back texture of the tab row, 256 x 46 at (0, 20); the clip is the window's rectangle
INT NCMainWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 20, 256, 46, 0, 0, 256, 46, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10054c10, NCMainWnd, OnPaint)

// the icon the page shown gives the minimized window (the pages by their class, up the chain)
static FORCEINLINE void SetPageIcon(NCIcon* Icon, NCWnd* Page, UClass* Class, const TCHAR* Name, INT ID)
{
    if (Page->IsA(Class))
        Icon->SetIcon(Name, ID, 1);
}

// 0x10054cb0 (vslot 49) - minimized: the icon takes the picture of the page shown (the status, skills, actions, clan
// or quest icons), is moved back on the screen when it is left of it, and takes the focus; the window hides
void NCMainWnd::Minimize()
{
    NCWnd* Page = TabCtrl->CurrentPage;
    SetPageIcon(Icon, Page, NClass::NCDetailStatusWnd(), TEXT("L2UI_CH3.TABBUTTON.MainWndTabIcon1"), 0xC2);
    SetPageIcon(Icon, Page, NClass::NCMagicSkillWnd(), TEXT("L2UI_CH3.TABBUTTON.MainWndTabIcon2"), 0xC4);
    SetPageIcon(Icon, Page, NClass::NCActionWnd(), TEXT("L2UI_CH3.TABBUTTON.MainWndTabIcon3"), 0xC5);
    SetPageIcon(Icon, Page, NClass::NCPledgeWnd(), TEXT("L2UI_CH3.TABBUTTON.MainWndTabIcon4"), 0xC5);
    SetPageIcon(Icon, Page, NClass::NCQuestGaraTreeWnd(), TEXT("L2UI_CH3.TABBUTTON.MainWndTabIcon5"), 0xC6);
    if (ScreenX < 0)
        Icon->MoveWindow(-ScreenX, 0);
    HideWindow();
    if (Icon)
    {
        Icon->ShowWindow();
        Icon->SetFocus();
    }
}
NW_HOOK_METHOD(0x10054cb0, NCMainWnd, Minimize)

// 0x100568d0 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x100568d0, NCMainWnd)

NW_CHECK_VTABLE(NCMainWnd, 0x101838b8, 69)
