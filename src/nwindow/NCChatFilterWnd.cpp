/*=============================================================================
    NCChatFilterWnd.cpp - chat filter panel. Stock: 0x10038450 .. 0x1003853e,
    0x100390b0 .. 0x10039d03 (0x10039080 is NCWnd::OffsetWindow), 0x10039d10 .. 0x1003aa60.
=============================================================================*/
#include "nwindow/NCChatFilterWnd.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"

static NCChatWnd* ChatWndOf(NCWnd* Wnd)
{
    return (NCChatWnd*)Wnd->Entry->Parent->Wnd;
}

// 0x10052580
NCChatFilterWnd::NCChatFilterWnd()
{
}
NW_HOOK_CTOR(0x10052580, NCChatFilterWnd)

// 0x100525d0
NW_HOOK_DTOR(0x100525d0, NCChatFilterWnd)

// 0x1004c540 (vslot 25) - C2 does not test IsEnabled first
INT NCChatFilterWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == (WPARAM)NID_CHATFILTER_OK)
            return OnOK() != 0;
        if (wParam == (WPARAM)NID_CHATFILTER_CANCEL)
            return OnCancel() != 0;
        if (wParam == (WPARAM)NID_CHATFILTER_SYSTEM)
            return OnSystemClicked() != 0;
        if (wParam == (WPARAM)NID_CHATFILTER_CHAT)
            return OnChatClicked() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1004c540, NCChatFilterWnd, WindowProc)

// one check box of the panel (repeated inline in the stock OnCreate)
static FORCEINLINE NCCheckBox* CreateFilterBox(NCWnd* Parent, INT TextID, INT ID, INT X, INT Y, INT W)
{
    NCCheckBox* Box = new NCCheckBox(TextID, 0, 0);
    Box->ID = ID;
    Box->SetPosition(X, Y, W, 12);
    Parent->CreateChild(Box, NWS_VISIBLE);
    return Box;
}

// 0x1004f9d0 (vslot 37) - C3: the CH3 back texture, the boxes moved (a margin of 17, then 28 and 138), the buttons of
// the new skin (76 x 23)
INT NCChatFilterWnd::OnCreate()
{
    BackTex = FindTexture(TEXT("L2UI_CH3.ChatWnd.Chatting_Back5"));

    Filters[CHATFILTER_System] = CreateFilterBox(this, 0x164, (INT)0xF0F01450, 17, 17, 100);
    Filters[CHATFILTER_Damage] = CreateFilterBox(this, 0x165, (INT)0xF0F01451, 28, 37, 80);
    Filters[CHATFILTER_Chat]   = CreateFilterBox(this, 0x166, (INT)0xF0F01452, 17, 61, 100);
    Filters[CHATFILTER_Normal] = CreateFilterBox(this, 0x17F, (INT)0xF0F01453, 28, 81, 80);
    Filters[CHATFILTER_Shout]  = CreateFilterBox(this, 0x167, (INT)0xF0F01454, 28, 99, 80);
    Filters[CHATFILTER_Pledge] = CreateFilterBox(this, 0x168, (INT)0xF0F01455, 28, 117, 80);
    Filters[CHATFILTER_Party]  = CreateFilterBox(this, 0x169, (INT)0xF0F01456, 138, 81, 80);
    Filters[CHATFILTER_Market] = CreateFilterBox(this, 0x16A, (INT)0xF0F01457, 138, 99, 80);
    Filters[CHATFILTER_Tell]   = CreateFilterBox(this, 0x16B, (INT)0xF0F01458, 138, 117, 80);
    Filters[CHATFILTER_Ally]   = CreateFilterBox(this, 0x250, (INT)0xF0F01470, 28, 135, 80);
    Filters[CHATFILTER_UseItems] = CreateFilterBox(this, 0x31C, (INT)0xF0F01470, 138, 37, 92);

    OKButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    OKButton->ID = NID_CHATFILTER_OK;
    OKButton->SetPosition(51, 160, 76, 23);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(0x8C);

    CancelButton = new NCButton(TEXT("L2UI_ch3.button.btn1_normal"), TEXT("L2UI_ch3.button.btn1_normalon"), 0);
    CancelButton->ID = NID_CHATFILTER_CANCEL;
    CancelButton->SetPosition(131, 160, 76, 23);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(0x8D);
    return 1;
}
NW_HOOK_METHOD(0x1004f9d0, NCChatFilterWnd, OnCreate)

// 0x1004d490 (vslot 38) - the eleven boxes in a loop, then the two buttons
INT NCChatFilterWnd::OnDestroy()
{
    guard(NCChatFilterWnd::OnDestroy);
    for (INT i = 0; i < CHATFILTER_MAX; i++)
    {
        if (Filters[i])
        {
            RemoveChildWindow(Filters[i]->Entry);
            delete Filters[i];
            Filters[i] = NULL;
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
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1004d490, NCChatFilterWnd, OnDestroy)

// 0x1004d5a0 (vslot 56) - C3: the back texture is 256 x 191
INT NCChatFilterWnd::OnPaint(NCGDevice* Device)
{
    guard(NCChatFilterWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->DrawTexture(0, 0, 256, 191, 0, 0, 256, 191, BackTex);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1004d5a0, NCChatFilterWnd, OnPaint)

// the current tab's own channel cannot be switched off (inlined where used, as in the stock code)
FORCEINLINE void NCChatFilterWnd::GreyTabChannel()
{
    NCChatWnd* Chat = ChatWndOf(this);
    if (Chat->CurTab == CHATTAB_Pledge)
        Filters[CHATFILTER_Pledge]->bDisabled = 1;
    if (Chat->CurTab == CHATTAB_Party)
        Filters[CHATFILTER_Party]->bDisabled = 1;
    if (Chat->CurTab == CHATTAB_Market)
        Filters[CHATFILTER_Market]->bDisabled = 1;
    if (Chat->CurTab == CHATTAB_Ally)
        Filters[CHATFILTER_Ally]->bDisabled = 1;
}

// 0x1004d8f0 (vslot 68) - "system" switches its two sub-boxes (damage and the used items)
INT NCChatFilterWnd::OnSystemClicked()
{
    if (!Filters[CHATFILTER_System]->bChecked)
    {
        Filters[CHATFILTER_Damage]->bDisabled = 1;
        Filters[CHATFILTER_UseItems]->bDisabled = 1;
    }
    else
    {
        Filters[CHATFILTER_Damage]->bDisabled = 0;
        Filters[CHATFILTER_UseItems]->bDisabled = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x1004d8f0, NCChatFilterWnd, OnSystemClicked)

// 0x1004d940 (vslot 69) - "chat" switches the chat channels
INT NCChatFilterWnd::OnChatClicked()
{
    if (!Filters[CHATFILTER_Chat]->bChecked)
    {
        Filters[CHATFILTER_Normal]->bDisabled = 1;
        Filters[CHATFILTER_Shout]->bDisabled = 1;
        Filters[CHATFILTER_Pledge]->bDisabled = 1;
        Filters[CHATFILTER_Party]->bDisabled = 1;
        Filters[CHATFILTER_Market]->bDisabled = 1;
        Filters[CHATFILTER_Tell]->bDisabled = 1;
        Filters[CHATFILTER_Ally]->bDisabled = 1;
    }
    else
    {
        Filters[CHATFILTER_Normal]->bDisabled = 0;
        Filters[CHATFILTER_Shout]->bDisabled = 0;
        Filters[CHATFILTER_Pledge]->bDisabled = 0;
        Filters[CHATFILTER_Party]->bDisabled = 0;
        Filters[CHATFILTER_Market]->bDisabled = 0;
        Filters[CHATFILTER_Tell]->bDisabled = 0;
        Filters[CHATFILTER_Ally]->bDisabled = 0;
    }
    GreyTabChannel();
    return 1;
}
NW_HOOK_METHOD(0x1004d940, NCChatFilterWnd, OnChatClicked)

// 0x1004da70 (vslot 70) - the boxes become the tab's filter and its chatfilter.ini section.
// The stock switch has no default (the list and section stay uninitialized); CurTab is
// always 0..4.
INT NCChatFilterWnd::OnOK()
{
    NCChatWnd* Chat = ChatWndOf(this);
    NCChatListBox* List = NULL;
    const TCHAR* Section = NULL;
    switch (Chat->CurTab)
    {
    case CHATTAB_All:
        List = Chat->Lists[CHATLIST_All];
        Section = TEXT("entire_tab");
        break;
    case CHATTAB_Market:
        List = Chat->Lists[CHATLIST_Market];
        Section = TEXT("market_tab");
        break;
    case CHATTAB_Party:
        List = Chat->Lists[CHATLIST_Party];
        Section = TEXT("party_tab");
        break;
    case CHATTAB_Pledge:
        List = Chat->Lists[CHATLIST_Pledge];
        Section = TEXT("pledge_tab");
        break;
    case CHATTAB_Ally:
        List = Chat->Lists[CHATLIST_Ally];
        Section = TEXT("ally_tab");
        break;
    }
    NChatFilter& F = List->Filter;
    F.bSystem = Filters[CHATFILTER_System]->bChecked;
    F.bDamage = Filters[CHATFILTER_Damage]->bChecked;
    F.bChat = Filters[CHATFILTER_Chat]->bChecked;
    F.bNormal = Filters[CHATFILTER_Normal]->bChecked;
    F.bShout = Filters[CHATFILTER_Shout]->bChecked;
    F.bPledge = Filters[CHATFILTER_Pledge]->bChecked;
    F.bParty = Filters[CHATFILTER_Party]->bChecked;
    F.bMarket = Filters[CHATFILTER_Market]->bChecked;
    F.bTell = Filters[CHATFILTER_Tell]->bChecked;
    F.bAlly = Filters[CHATFILTER_Ally]->bChecked;
    F.bUseItems = Filters[CHATFILTER_UseItems]->bChecked;

    const TCHAR* Ini = TEXT("chatfilter.ini");
    GConfig->SetBool(Section, TEXT("system"), Filters[CHATFILTER_System]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("damage"), Filters[CHATFILTER_Damage]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("chat"), Filters[CHATFILTER_Chat]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("normal"), Filters[CHATFILTER_Normal]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("shout"), Filters[CHATFILTER_Shout]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("pledge"), Filters[CHATFILTER_Pledge]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("party"), Filters[CHATFILTER_Party]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("market"), Filters[CHATFILTER_Market]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("tell"), Filters[CHATFILTER_Tell]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("ally"), Filters[CHATFILTER_Ally]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("useitems"), Filters[CHATFILTER_UseItems]->bChecked, Ini);
    HideWindow();
    Chat->bFilterHidden = 1;
    return 1;
}
NW_HOOK_METHOD(0x1004da70, NCChatFilterWnd, OnOK)

// 0x1004c5f0 (vslot 71)
INT NCChatFilterWnd::OnCancel()
{
    NCChatWnd* Chat = ChatWndOf(this);
    HideWindow();
    Chat->bFilterHidden = 1;
    return 1;
}
NW_HOOK_METHOD(0x1004c5f0, NCChatFilterWnd, OnCancel)

// 0x1004d690 - the boxes take the tab's filter (passed by value: 11 switches), the sub-boxes grey with their master
void NCChatFilterWnd::SetFilter(NChatFilter Filter)
{
    Filters[CHATFILTER_Normal]->bDisabled = 0;
    Filters[CHATFILTER_Shout]->bDisabled = 0;
    Filters[CHATFILTER_Pledge]->bDisabled = 0;
    Filters[CHATFILTER_Party]->bDisabled = 0;
    Filters[CHATFILTER_Market]->bDisabled = 0;
    Filters[CHATFILTER_Tell]->bDisabled = 0;
    Filters[CHATFILTER_Ally]->bDisabled = 0;
    Filters[CHATFILTER_System]->bChecked = Filter.bSystem;
    Filters[CHATFILTER_Damage]->bChecked = Filter.bDamage;
    Filters[CHATFILTER_Chat]->bChecked = Filter.bChat;
    Filters[CHATFILTER_Normal]->bChecked = Filter.bNormal;
    Filters[CHATFILTER_Shout]->bChecked = Filter.bShout;
    Filters[CHATFILTER_Pledge]->bChecked = Filter.bPledge;
    Filters[CHATFILTER_Party]->bChecked = Filter.bParty;
    Filters[CHATFILTER_Market]->bChecked = Filter.bMarket;
    Filters[CHATFILTER_Tell]->bChecked = Filter.bTell;
    Filters[CHATFILTER_Ally]->bChecked = Filter.bAlly;
    Filters[CHATFILTER_UseItems]->bChecked = Filter.bUseItems;

    if (!Filters[CHATFILTER_System]->bChecked)
    {
        Filters[CHATFILTER_Damage]->bDisabled = 1;
        Filters[CHATFILTER_UseItems]->bDisabled = 1;
    }
    else
    {
        Filters[CHATFILTER_Damage]->bDisabled = 0;
        Filters[CHATFILTER_UseItems]->bDisabled = 0;
    }
    if (!Filters[CHATFILTER_Chat]->bChecked)
    {
        Filters[CHATFILTER_Normal]->bDisabled = 1;
        Filters[CHATFILTER_Shout]->bDisabled = 1;
        Filters[CHATFILTER_Pledge]->bDisabled = 1;
        Filters[CHATFILTER_Party]->bDisabled = 1;
        Filters[CHATFILTER_Market]->bDisabled = 1;
        Filters[CHATFILTER_Tell]->bDisabled = 1;
        Filters[CHATFILTER_Ally]->bDisabled = 1;
    }
    else
    {
        Filters[CHATFILTER_Normal]->bDisabled = 0;
        Filters[CHATFILTER_Shout]->bDisabled = 0;
        Filters[CHATFILTER_Pledge]->bDisabled = 0;
        Filters[CHATFILTER_Party]->bDisabled = 0;
        Filters[CHATFILTER_Market]->bDisabled = 0;
        Filters[CHATFILTER_Tell]->bDisabled = 0;
        Filters[CHATFILTER_Ally]->bDisabled = 0;
    }
    GreyTabChannel();
}
NW_HOOK_METHOD(0x1004d690, NCChatFilterWnd, SetFilter)

// one [xxx_tab] section of chatfilter.ini (inlined five times in the stock code); a missing
// key takes the tab's default: system, damage, chat, shout and tell on, and the given ones
static FORCEINLINE void LoadTabFilter(const TCHAR* Section, NChatFilter& F, UBOOL bNormal,
                                      UBOOL bPledge, UBOOL bParty, UBOOL bMarket, UBOOL bAlly)
{
    const TCHAR* Ini = TEXT("chatfilter.ini");
    if (!GConfig->GetBool(Section, TEXT("system"), F.bSystem, Ini))
        F.bSystem = 1;
    if (!GConfig->GetBool(Section, TEXT("chat"), F.bChat, Ini))
        F.bChat = 1;
    if (!GConfig->GetBool(Section, TEXT("normal"), F.bNormal, Ini))
        F.bNormal = bNormal;
    if (!GConfig->GetBool(Section, TEXT("shout"), F.bShout, Ini))
        F.bShout = 1;
    if (!GConfig->GetBool(Section, TEXT("pledge"), F.bPledge, Ini))
        F.bPledge = bPledge;
    if (!GConfig->GetBool(Section, TEXT("party"), F.bParty, Ini))
        F.bParty = bParty;
    if (!GConfig->GetBool(Section, TEXT("market"), F.bMarket, Ini))
        F.bMarket = bMarket;
    if (!GConfig->GetBool(Section, TEXT("tell"), F.bTell, Ini))
        F.bTell = 1;
    if (!GConfig->GetBool(Section, TEXT("damage"), F.bDamage, Ini))
        F.bDamage = 1;
    if (!GConfig->GetBool(Section, TEXT("ally"), F.bAlly, Ini))
        F.bAlly = bAlly;
    if (!GConfig->GetBool(Section, TEXT("useitems"), F.bUseItems, Ini))
        F.bUseItems = 0;
}

// 0x1004dd70 - the five sections of chatfilter.ini read into the lists' filters; the one of the current tab goes to the
// boxes
void NCChatFilterWnd::LoadFilters()
{
    NCChatWnd* Chat = ChatWndOf(this);

    NChatFilter& All = Chat->Lists[CHATLIST_All]->Filter;
    LoadTabFilter(TEXT("entire_tab"), All, 1, 1, 1, 0, 0);
    if (Chat->CurTab == CHATTAB_All)
        SetFilter(All);

    NChatFilter& Pledge = Chat->Lists[CHATLIST_Pledge]->Filter;
    LoadTabFilter(TEXT("pledge_tab"), Pledge, 0, 1, 0, 0, 0);
    if (Chat->CurTab == CHATTAB_Pledge)
        SetFilter(Pledge);

    NChatFilter& Party = Chat->Lists[CHATLIST_Party]->Filter;
    LoadTabFilter(TEXT("party_tab"), Party, 0, 0, 1, 0, 0);
    if (Chat->CurTab == CHATTAB_Party)
        SetFilter(Party);

    NChatFilter& Market = Chat->Lists[CHATLIST_Market]->Filter;
    LoadTabFilter(TEXT("market_tab"), Market, 0, 0, 0, 1, 0);
    if (Chat->CurTab == CHATTAB_Market)
        SetFilter(Market);

    NChatFilter& Ally = Chat->Lists[CHATLIST_Ally]->Filter;
    LoadTabFilter(TEXT("ally_tab"), Ally, 0, 0, 0, 0, 1);
    if (Chat->CurTab == CHATTAB_Ally)
        SetFilter(Ally);
}
NW_HOOK_METHOD(0x1004dd70, NCChatFilterWnd, LoadFilters)

NW_CHECK_VTABLE(NCChatFilterWnd, 0x10182910, 72)
