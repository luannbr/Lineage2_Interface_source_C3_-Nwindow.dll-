/*=============================================================================
    NCMatchWnd.cpp - party matching. C3 stock: 0x100a4c40 .. 0x100a4e00 (WindowProc, OnTimer, Minimize, ClearList,
    OnJoin), 0x100a57e0 .. 0x100a5f60 (OnDestroy, OnPaint, SetList, the page buttons), 0x100a7c00 .. 0x100a8a51
    (constructor, OnCreate). ShowWindow's and HideWindow's bodies are shared (0x100aa860 and 0x10030c00).
=============================================================================*/
#include "nwindow/NCMatchWnd.h"
#include "nwindow/NCMatchListCtrl.h"
#include "nwindow/NCMatchFrameCtrl.h"
#include "nwindow/NCPartyRoomMakingWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleFns.h"

// one child of OnDestroy (repeated inline in the stock code)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// a column of the list (the stock code spells out each with its own record)
static FORCEINLINE void AddMatchColumn(NCListCtrl* List, NListColumnInfo& Info, INT Width, INT TitleID, UBOOL bNumeric)
{
    Info.Width = Width;
    Info.Height = 19;
    Info.bNumeric = bNumeric;
    Info.SetTitleID(TitleID);
    Info.SetNormalTex(TEXT("L2UI_CH3.ListCtrl.tab1"), 0);
    Info.SetPushedTex(TEXT("L2UI_CH3.ListCtrl.tab1_down"), 0);
    Info.SetNormalTex(TEXT("L2UI_CH3.ListCtrl.tab2"), 1);
    Info.SetPushedTex(TEXT("L2UI_CH3.ListCtrl.tab2_down"), 1);
    Info.SetNormalTex(TEXT("L2UI_CH3.ListCtrl.tab3"), 2);
    Info.SetPushedTex(TEXT("L2UI_CH3.ListCtrl.tab3_down"), 2);
    List->AddColumn(0, &Info);
}

// one of the small buttons (the page buttons are 64 x 21, the others 76 x 23: spelled out at each use in the stock
// code)
static FORCEINLINE NCButton* MakeSmallButton(NCWnd* Parent, INT ID, INT X, INT Y, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI_CH3.Button.SmallButton2"), TEXT("L2UI_CH3.Button.SmallButton2_down"), 0);
    Button->ID = ID;
    Button->SetPosition(X, Y, 0x40, 0x15);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetText(*GL2GameData.SysString[TextID]);
    return Button;
}
static FORCEINLINE NCButton* MakeWideButton(NCWnd* Parent, INT ID, INT X, INT Y, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI_CH3.Button.Btn1_normal"), TEXT("L2UI_CH3.Button.Btn1_normalOn"), 0);
    Button->ID = ID;
    Button->SetPosition(X, Y, 0x4C, 0x17);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetText(*GL2GameData.SysString[TextID]);
    return Button;
}

// the request of the page buttons (spelled out in the stock code of the previous, the next and the search button):
// the page, the place chosen (its item's value) and the kind of room chosen; then four of the buttons wait for the
// timer
static FORCEINLINE void RequestPage(NCMatchWnd* Wnd, INT NewPage)
{
    L2ParamStack Params(10);
    Params.PushBack((void*)NewPage);
    Params.PushBack((void*)((NComboItemFns*)Wnd->AreaCombo)->GetItemData(Wnd->AreaCombo->Selected));
    Params.PushBack((void*)Wnd->TypeCombo->Selected);
    ((NConsoleForwardFns*)GConsole)->RequestListPartyWating(Params);   // NConsoleWnd::RequestPartyMatchList (name ours)
    Wnd->PrevButton->EnableWindow(0);
    Wnd->NextButton->EnableWindow(0);
    Wnd->SearchButton->EnableWindow(0);
    Wnd->JoinButton->EnableWindow(0);
}

// 0x100a7c00 - NCWnd's, no page
NCMatchWnd::NCMatchWnd()
{
    Page = 0;
}
NW_HOOK_CTOR(0x100a7c00, NCMatchWnd)

// 0x100a7c50
NW_HOOK_DTOR(0x100a7c50, NCMatchWnd)

// 0x100a4c40 (vslot 25) - the five buttons
INT NCMatchWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_MATCH_PREV)
            return OnPrevPage() != 0;
        if ((INT)wParam == NID_MATCH_NEXT)
            return OnNextPage() != 0;
        if ((INT)wParam == NID_MATCH_SEARCH)
            return OnSearch() != 0;
        if ((INT)wParam == NID_MATCH_CREATE)
            return OnCreateRoom() != 0;
        if ((INT)wParam == NID_MATCH_JOIN)
            return OnJoin() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100a4c40, NCMatchWnd, WindowProc)

// 0x100a4d20 (vslot 36) - the 5 second timer: four of the buttons are enabled again
INT NCMatchWnd::OnTimer(INT InTimerID)
{
    if (InTimerID == TIMER_MatchButtons)
    {
        PrevButton->EnableWindow(1);
        NextButton->EnableWindow(1);
        SearchButton->EnableWindow(1);
        JoinButton->EnableWindow(1);
    }
    return 1;
}
NW_HOOK_METHOD(0x100a4d20, NCMatchWnd, OnTimer)

// 0x100a7ca0 (vslot 37) - C3: the timer, the title bar of its own class, the background, the list of rooms (15 rows,
// 5 columns: number, title, kind, level range, members), the five buttons and the two selection boxes
INT NCMatchWnd::OnCreate()
{
    NListColumnInfo NumberColumn, TitleColumn, TypeColumn, LevelColumn, MembersColumn;   // five records, as in the stock frame
    NCWnd::OnCreate();
    TimerID = SetTimer(TIMER_MatchButtons, 5000, NULL);
    MatchFrame = new NCMatchFrameCtrl();
    MatchFrame->ID = NID_MATCH_FRAME;
    MatchFrame->SetPosition(0, 0, 600, 20);
    CreateChild(MatchFrame, NWS_VISIBLE);
    MatchFrame->SetTitleText(*GL2GameData.SysString[389]);
    BackTex = FindTexture(TEXT("L2UI_CH3.PartyMatchWnd.PartyMatch1_Back"));
    MatchList = new NCMatchListCtrl(15, 15, 5, 16);
    MatchList->SetPosition(7, 0x53, 0x249, 0x111);
    CreateChild(MatchList, NWS_VISIBLE | NWS_UNKNOWN20);   // 0x22
    MatchList->SetTextOffset(0, 3);
    AddMatchColumn(MatchList, NumberColumn, 0x22, 0x1A0, 1);
    AddMatchColumn(MatchList, TitleColumn, 0x122, 0x19D, 1);
    AddMatchColumn(MatchList, TypeColumn, 0x94, 0x405, 0);
    AddMatchColumn(MatchList, LevelColumn, 0x48, 0x406, 0);
    AddMatchColumn(MatchList, MembersColumn, 0x29, 0x40C, 0);

    PrevButton = MakeSmallButton(this, NID_MATCH_PREV, 0xE9, 0x16E, 1037);
    NextButton = MakeSmallButton(this, NID_MATCH_NEXT, 0x12F, 0x16E, 1038);
    SearchButton = MakeWideButton(this, NID_MATCH_SEARCH, 0x171, 0x32, 1039);
    CreateButton = MakeWideButton(this, NID_MATCH_CREATE, 0x1C0, 0x192, 1040);
    JoinButton = MakeWideButton(this, NID_MATCH_JOIN, 0x20B, 0x192, 1041);

    AreaCombo = new NCComboBox();
    AreaCombo->ID = NID_MATCH_AREA;
    AreaCombo->SetPosition(0x54, 0x34, 0x5A, 0x11);
    CreateChild(AreaCombo, NWS_VISIBLE | 0x10);   // 0x12: 0x10's meaning is not known
    TypeCombo = new NCComboBox();
    TypeCombo->ID = NID_MATCH_TYPE;
    TypeCombo->SetPosition(0x108, 0x34, 0x5A, 0x11);
    CreateChild(TypeCombo, NWS_VISIBLE | 0x10);
    // the place box: -2, -1 and the thirteen places (the value of the item is what the request carries)
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1047], -2);   // NCComboBox::AddItem(Text, Value)
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1046], -1);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1048], 1);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1049], 2);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1050], 3);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1051], 4);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1052], 5);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1053], 6);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1054], 7);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1055], 8);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1056], 9);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1057], 10);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1058], 11);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1059], 12);
    ((NComboItemFns*)AreaCombo)->AddItemData(*GL2GameData.SysString[1060], 13);
    AreaCombo->SetSelected(1);
    TypeCombo->AddItem(*GL2GameData.SysString[1045]);
    TypeCombo->AddItem(*GL2GameData.SysString[1046]);
    TypeCombo->SetSelected(1);
    return 1;
}
NW_HOOK_METHOD(0x100a7ca0, NCMatchWnd, OnCreate)

// 0x100a57e0 (vslot 38) - seven of the controls go (the next page button and the title bar stay: as the stock
// code), then NCWnd's
INT NCMatchWnd::OnDestroy()
{
    guard(NCMatchWnd::OnDestroy);
    DestroyChild(this, AreaCombo);
    DestroyChild(this, TypeCombo);
    DestroyChild(this, MatchList);
    DestroyChild(this, PrevButton);
    DestroyChild(this, SearchButton);
    DestroyChild(this, CreateButton);
    DestroyChild(this, JoinButton);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100a57e0, NCMatchWnd, OnDestroy)

// stock body shared with NCSystemMenuWnd's (0x10078950) and other windows': shown again from its minimized icon, the
// window takes the icon's place and the icon goes away
void NCMatchWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Icon)
    {
        SetPosition(Icon->ScreenX, Icon->ScreenY, (INT)Width, (INT)Height);
        Icon->SetPosition(0, 0, 0, 0);
        Icon->HideWindow();
    }
}

// (vslot 45) the stock vtable (0x1018f160) points at NCWnd::HideWindow itself (0x10030c00)
#if NW_HOOK_MODE
void NCMatchWnd::HideWindow()
{
    NCWnd::HideWindow();
}
#else
NW_SLOT_IS("?HideWindow@NCMatchWnd@@UAEXXZ", "?HideWindow@NCWnd@@UAEXXZ")   // NCMatchWnd::HideWindow = NCWnd::HideWindow (0x10030c00)
#endif

// 0x100a4d80 (vslot 49) - minimizing: the window's own icon picture (text 389), see NCPetWnd::Minimize
void NCMatchWnd::Minimize()
{
    if (Icon)
        Icon->SetIcon(TEXT("L2UI_CH3.PartyMatchWnd.PartyMatchIcon"), 0x185, 1);
    if (ScreenX < 0 && Icon)
        Icon->OffsetWindow(-ScreenX, 0);
    HideWindow();
    if (Icon)
    {
        Icon->ShowWindow();
        Icon->SetFocus();
    }
}
NW_HOOK_METHOD(0x100a4d80, NCMatchWnd, Minimize)

// 0x100a5990 (vslot 56) - the background (600 x 411 under the title bar) and the two labels of the selection boxes,
// right-aligned
INT NCMatchWnd::OnPaint(NCGDevice* Device)
{
    SIZE Size;
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 20, 600, 411, 0, 0, 600, 411, BackTex, 0xFF, 1);
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[1034], 0, 0, 0, 0, 0);
    Device->DrawText(0x50 - Size.cx, 0x36, 0xFFDCDCDC, *GL2GameData.SysString[1034], 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                     0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[1035], 0, 0, 0, 0, 0);
    Device->DrawText(0x104 - Size.cx, 0x36, 0xFFDCDCDC, *GL2GameData.SysString[1035], 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                     0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100a5990, NCMatchWnd, OnPaint)

// 0x100a5c80 (vslot 68) - the previous page (the first when there is none)
INT NCMatchWnd::OnPrevPage()
{
    INT NewPage = 1;
    if (Page > 1)
        NewPage = Page - 1;
    RequestPage(this, NewPage);
    return 1;
}
NW_HOOK_METHOD(0x100a5c80, NCMatchWnd, OnPrevPage)

// 0x100a5d80 (vslot 69) - the next page
INT NCMatchWnd::OnNextPage()
{
    INT NewPage = Page + 1;
    RequestPage(this, NewPage);
    return 1;
}
NW_HOOK_METHOD(0x100a5d80, NCMatchWnd, OnNextPage)

// 0x100a5e70 (vslot 70) - the first page
INT NCMatchWnd::OnSearch()
{
    RequestPage(this, 1);
    return 1;
}
NW_HOOK_METHOD(0x100a5e70, NCMatchWnd, OnSearch)

// 0x100a5f60 (vslot 71) - the room making window opens for a new room: 9 members, the level range 5 either side of the
// player's (1 .. 75), the default title (system message 1398)
INT NCMatchWnd::OnCreateRoom()
{
    INT Min = 1;
    INT Max = 75;
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        INT Level = Me->Level;
        Max = Level + 5;
        if (Level > 5)
        {
            if (Level > 70)
                Max = 75;
            Min = Level - 5;
        }
    }
    static_cast<NCPartyRoomMakingWnd*>(GDesktop->PartyRoomMakingWnd)->SetData(0, *GL2GameData.SystemMsg[1398].Message, 7,
                                                                              Min, Max, 0);
    GDesktop->PartyRoomMakingWnd->ShowWindow();
    GDesktop->PartyRoomMakingWnd->SetFocus();
    return 1;
}
NW_HOOK_METHOD(0x100a5f60, NCMatchWnd, OnCreateRoom)

// 0x100a4e00 (vslot 72) - asks to join any room (room 0); four of the buttons wait for the timer
INT NCMatchWnd::OnJoin()
{
    L2ParamStack Params(10);
    Params.PushBack((void*)0);
    ((NConsoleForwardFns*)GConsole)->RequestJoinPartyRoom(Params);   // NConsoleWnd::RequestJoinPartyRoom (name ours)
    PrevButton->EnableWindow(0);
    NextButton->EnableWindow(0);
    SearchButton->EnableWindow(0);
    JoinButton->EnableWindow(0);
    return 1;
}
NW_HOOK_METHOD(0x100a4e00, NCMatchWnd, OnJoin)

// 0x100a5b10 (the console's slot 374) - a page of the list: the window is shown, the page number and the rooms as
// the packet carries them (the number, the title, the kind, the level range, the members now / most)
void NCMatchWnd::SetList(L2ParamStack& Params)
{
    TCHAR Buf[128];
    ShowWindow();
    Page = (INT)Params.Top();
    INT Count = (INT)Params.Top();
    while (Count > 0)
    {
        NListRow* Row = new NListRow(5);
        Row->Cells[0].SetNumber((INT)Params.Top());
        Row->Cells[1].SetText((TCHAR*)Params.Top());
        Row->Cells[2].SetText(((NConsoleLeftFns*)GConsole)->GetPartyRoomAreaName((INT)Params.Top()));
        INT First = (INT)Params.Top();
        INT Second = (INT)Params.Top();
        appSprintf(Buf, TEXT("%d-%d"), First, Second);
        Row->Cells[3].SetText(Buf);
        First = (INT)Params.Top();
        Second = (INT)Params.Top();
        appSprintf(Buf, TEXT("%d/%d"), First, Second);
        Row->Cells[4].SetText(Buf);
        MatchList->AddRow(Row);
        Count--;
    }
}
NW_HOOK_METHOD(0x100a5b10, NCMatchWnd, SetList)

// 0x100a4de0 (the console's slot 375) - back to no page, the rows go
void NCMatchWnd::ClearList()
{
    Page = 0;
    MatchList->ClearRows();
}
NW_HOOK_METHOD(0x100a4de0, NCMatchWnd, ClearList)

NW_CHECK_VTABLE(NCMatchWnd, 0x1018f160, 73)
