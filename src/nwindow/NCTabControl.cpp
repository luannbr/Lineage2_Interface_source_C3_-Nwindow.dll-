/*=============================================================================
    NCTabControl.cpp - tabs and their pages. Stock: 0x10022150 .. 0x10022b41.
=============================================================================*/
#include "nwindow/NCTabControl.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// interface sound of a tab change
enum { SOUND_Tab = 5 };

// 0x1002aae0 - NControl's constructor is inlined in the stock one; the tab group id is set by OnCreate
NCTabControl::NCTabControl()
{
    NextTabX = 0;
    CurrentPage = NULL;
}
NW_HOOK_CTOR(0x1002aae0, NCTabControl)

// 0x1002ab80 - Pages and Tabs are destroyed with the object (TArray<T*> destructor 0x10022980)
NW_HOOK_DTOR(0x1002ab80, NCTabControl)

// 0x1002a160 (vslot 25) - the tab buttons report here
INT NCTabControl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (WPARAM)GroupID == wParam)
        return SelectTab((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1002a160, NCTabControl, WindowProc)

// 0x1002a1c0 (vslot 37)
INT NCTabControl::OnCreate()
{
    NControl::OnCreate();
    GroupID = GNextTabGroupID++;
    return 1;
}
NW_HOOK_METHOD(0x1002a1c0, NCTabControl, OnCreate)

// 0x1002a910 (vslot 38) - the tab buttons are ours, the pages are not
INT NCTabControl::OnDestroy()
{
    guard(NCTabControl::OnDestroy);
    for (INT i = 0; i < Tabs.Num(); i++)
    {
        if (Tabs(i))
        {
            RemoveChildWindow(Tabs(i)->Entry);
            delete Tabs(i);
            Tabs(i) = NULL;
        }
        delete Tabs(i);  // NULL by now: the stock code deletes twice
    }
    Tabs.Empty();
    Pages.Empty();
    NControl::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1002a910, NCTabControl, OnDestroy)

// 0x1002a1e0 (vslot 69)
void NCTabControl::ShowCurrentPage()
{
    for (INT i = 0; i < Pages.Num(); i++)
        if (Pages(i))
            Pages(i)->HideWindow();
    if (CurrentPage)
        CurrentPage->ShowWindow();
}
NW_HOOK_METHOD(0x1002a1e0, NCTabControl, ShowCurrentPage)

// 0x1002a260 (vslot 70) - the page shown goes, the tab's page comes and takes the focus; the other tabs are
// released and the owner is told (the interface sound plays twice: when the page changes and at the end)
INT NCTabControl::SelectTab(INT Index)
{
    if (Tabs.Num() >= 0 && Index < Tabs.Num() && Tabs(Index)->IsEnabled())
    {
        if (CurrentPage != Pages(Index))
            GConsole->PlayInterfaceSound(SOUND_Tab);
        if (CurrentPage)
            CurrentPage->HideWindow();
        CurrentPage = Pages(Index);
        if (CurrentPage)
        {
            CurrentPage->ShowWindow();
            CurrentPage->SetFocus();
        }
        for (INT i = 0; i < Tabs.Num(); i++)
            Tabs(i)->bReleased = 1;
        Tabs(Index)->bReleased = 0;
        NotifyParent(Index);
        GConsole->PlayInterfaceSound(SOUND_Tab);
    }
    return 1;
}
NW_HOOK_METHOD(0x1002a260, NCTabControl, SelectTab)

// 0x1002a350 - SelectTab without sound nor focus; every page is hidden first. C3: the second argument asks for the
// owner to be told (NotifyParent)
void NCTabControl::SetTab(INT Index, BYTE bNotify)
{
    if (Tabs.Num() >= 0 && Index < Tabs.Num() && Tabs(Index)->IsEnabled())
    {
        for (INT i = 0; i < Pages.Num(); i++)
            Pages(i)->HideWindow();
        CurrentPage = Pages(Index);
        if (CurrentPage)
            CurrentPage->ShowWindow();
        for (INT i = 0; i < Tabs.Num(); i++)
            Tabs(i)->bReleased = 1;
        Tabs(Index)->bReleased = 0;
        if (bNotify)
            NotifyParent(Index);
    }
}
NW_HOOK_METHOD(0x1002a350, NCTabControl, SetTab)

/*-----------------------------------------------------------------------------
    Adding tabs. The two stock overloads repeat the same code around the label;
    it is split here in the part before the label and the part after it.
-----------------------------------------------------------------------------*/

// the tab button, registered at the end of the row (name ours)
static __forceinline NCRadioButton* NewTabButton(NCTabControl* Ctrl, INT W, INT H,
                                                 const TCHAR* NormalTexName,
                                                 const TCHAR* PushedTexName)
{
    NCRadioButton* Tab;
    if (NormalTexName && PushedTexName)
        Tab = new NCRadioButton(NormalTexName, PushedTexName);
    else
        Tab = new NCRadioButton(TEXT("NWindow.BlackTexture"), TEXT("NWindow.Darkyellow"));
    Tab->ID = Ctrl->GroupID;
    Tab->SetPosition(Ctrl->NextTabX, 0, W, H);
    Ctrl->CreateChild(Tab, NWS_VISIBLE | NWS_UNKNOWN20);   // C2: 0x22
    // NCRadioButton::Value (+0x29C in C3; NCButton.h has no field there under that name)
    ::nw::Field<INT>(Tab, 0x29C) = Ctrl->Tabs.Num();
    return Tab;
}

// the first tab is the current one; the others start hidden and released (name ours)
static __forceinline INT AddTabButton(NCTabControl* Ctrl, NCRadioButton* Tab, NCWnd* Page,
                                      INT W, INT Gap, INT ToolTipID)
{
    Tab->TextOffsetX = 2;
    Tab->TextOffsetY = 1;
    Tab->SetToolTip(ToolTipID, 1);
    if (Ctrl->Tabs.Num() == 0)
    {
        Ctrl->CurrentPage = Page;
        Tab->bReleased = 0;
    }
    else
    {
        Tab->bReleased = 1;
        Tab->HideWindow();
    }
    Ctrl->NextTabX += W + Gap;
    Ctrl->Tabs.AddItem(Tab);
    Ctrl->Pages.AddItem(Page);
    return Ctrl->Tabs.Num() - 1;
}

// 0x1002a4c0 - label from the SysString table
INT NCTabControl::AddTab(INT TextID, NCWnd* Page, INT W, INT H, const TCHAR* NormalTexName,
                         const TCHAR* PushedTexName, INT Gap, INT ToolTipID)
{
    if (!*SysString(TextID))
        return -1;
    if (Tabs.Num() == 0)
        CurrentPage = Page;
    if (!W || !H)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, *SysString(TextID), 0, 0, 0, 0, 0, L2FONT_Normal);
        W = Size.cx + 10;
        H = Size.cy + 5;
    }
    NCRadioButton* Tab = NewTabButton(this, W, H, NormalTexName, PushedTexName);
    Tab->SetTextID(TextID);
    return AddTabButton(this, Tab, Page, W, Gap, ToolTipID);
}
NW_HOOK_METHOD_SIG(0x1002a4c0, NCTabControl, AddTab,
                   INT (NCTabControl::*)(INT, NCWnd*, INT, INT, const TCHAR*, const TCHAR*, INT, INT))

// 0x1002a700
INT NCTabControl::AddTab(const TCHAR* Text, NCWnd* Page, INT W, INT H,
                         const TCHAR* NormalTexName, const TCHAR* PushedTexName, INT Gap,
                         INT ToolTipID)
{
    if (!Text)
        return -1;
    if (Tabs.Num() == 0)
        CurrentPage = Page;
    if (!W || !H)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0, L2FONT_Normal);
        W = Size.cx + 10;
        H = Size.cy + 5;
    }
    NCRadioButton* Tab = NewTabButton(this, W, H, NormalTexName, PushedTexName);
    Tab->SetText(Text);
    return AddTabButton(this, Tab, Page, W, Gap, ToolTipID);
}
NW_HOOK_METHOD_SIG(0x1002a700, NCTabControl, AddTab,
                   INT (NCTabControl::*)(const TCHAR*, NCWnd*, INT, INT, const TCHAR*, const TCHAR*,
                                         INT, INT))


// 0x1002aab0 (ret 4: the hidden pointer of the returned array; anonymous in the db) - a copy of the tab buttons' array
// (the stock calls the array's copy constructor 0x1002a430 and returns the pointer it was given)
TArray<NCRadioButton*> NCTabControl::GetTabs()
{
    return Tabs;
}
NW_HOOK_METHOD(0x1002aab0, NCTabControl, GetTabs)

NW_CHECK_VTABLE(NCTabControl, 0x1017d838, 71)
