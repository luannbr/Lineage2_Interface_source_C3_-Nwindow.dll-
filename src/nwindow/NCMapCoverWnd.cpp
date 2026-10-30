/*=============================================================================
    NCMapCoverWnd.cpp - the world map window around the NCMinimapWnd. C3 stock: 0x100abd90 (WindowProc), 0x100abe50
    (Minimize), 0x100ac630 / 0x100ac6d0 (paint, destroy), 0x100afa50 .. 0x100afbb0 (the commands, ShowWindow),
    0x100afe30 / 0x100afe80 (constructor, destructor) and OnCreate (0x100b05d0). KillFocus (0x100abeb0) is the
    NCMinimapWnd's body: it stays a stock stub here.
=============================================================================*/
#include "nwindow/NCMapCoverWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NConsoleWnd.h"

// NWM_COMMAND ids (names ours)
enum
{
    NID_MAPCOVER_FRAME = 0xB000F0,
    NID_MAPCOVER_MARK = 0xB000F5,
    NID_MAPCOVER_MYPOS = 0xB00F00,
    NID_MAPCOVER_CONTINENT = 0x1F21,
};

// sysstring lines of the buttons and of the combo box's items
enum
{
    STR_ShowMark = 886,
    STR_ShowMyPosition = 887,
    STR_Map = 885,
    STR_Aden = 888,
    STR_Elmore = 889,
};

// the player's world position as the stock reads it inline (the first viewport's controller's pawn; 0 without one)
static FORCEINLINE FVector MyLocation()
{
    FVector Result(0.0f, 0.0f, 0.0f);
    APlayerController* Controller = GConsole->Engine->Client->Viewports(0)->Actor;
    if (Controller && Controller->Pawn)
        Result = Controller->Pawn->Location;
    return Result;
}

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

// 0x100afe30 - the background is set by OnCreate; the map and the controls are left as allocated
NCMapCoverWnd::NCMapCoverWnd()
{
    BackTex = NULL;
}
NW_HOOK_CTOR(0x100afe30, NCMapCoverWnd)

// 0x100afe80 - nothing of its own to free (OnDestroy took the children)
NW_HOOK_DTOR(0x100afe80, NCMapCoverWnd)

// 0x100abd90 (vslot 25) - the three controls
INT NCMapCoverWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_MAPCOVER_MARK)
            return OnShowMark() != 0;
        if (wParam == NID_MAPCOVER_MYPOS)
            return OnShowMyPosition() != 0;
        if (wParam == NID_MAPCOVER_CONTINENT)
            return OnContinent((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100abd90, NCMapCoverWnd, WindowProc)

// 0x100abeb0 (vslot 30) - the cursor lock is let go, then NCWnd's. The body the stock vtables of this window and of
// NTownMapWnd point at (the db names it NCMinimapWnd::KillFocus, where the hook build has it; the stock NCMinimapWnd's
// own slot 30 is NCWnd::KillFocus, 0x1002fd50)
#if NW_HOOK_MODE
NW_NAKED void NCMapCoverWnd::KillFocus() NW_STOCK(0x100abeb0)
#else
void NCMapCoverWnd::KillFocus()
{
    UnlockCursor();
    NCWnd::KillFocus();
}
#endif

// 0x100abe50 (vslot 49) - minimizing: the window's own icon picture (the menu icon, text 0x1BF)
void NCMapCoverWnd::Minimize()
{
    if (Icon)
        Icon->SetIcon(TEXT("L2UI_CH3.MenuIcon.menuicon3"), 0x1BF, 1);
    if (ScreenX < 0 && Icon)
        Icon->OffsetWindow(-ScreenX, 0);
    HideWindow();
    if (Icon)
    {
        Icon->ShowWindow();
        Icon->SetFocus();
    }
}
NW_HOOK_METHOD(0x100abe50, NCMapCoverWnd, Minimize)

// 0x100ac630 (vslot 56) - the background under the title bar (334 x 363)
INT NCMapCoverWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 20, 334, 363, 0, 0, 334, 363, BackTex, 0xFF, 1);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100ac630, NCMapCoverWnd, OnPaint)

// 0x100ac6d0 (vslot 38)
INT NCMapCoverWnd::OnDestroy()
{
    guard(NCMapCoverWnd::OnDestroy);
    DestroyChild(this, Map);
    DestroyChild(this, MarkButton);
    DestroyChild(this, MyPosButton);
    DestroyChild(this, ContinentCombo);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100ac6d0, NCMapCoverWnd, OnDestroy)

// 0x100afbb0 (vslot 44) - shown where its icon was; the map goes to the player's place and forgets the continent's
// saved scroll
void NCMapCoverWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Icon)
    {
        SetPosition(Icon->ScreenX, Icon->ScreenY, (INT)Width, (INT)Height);
        Icon->SetPosition(0, 0, 0, 0);
        Icon->HideWindow();
    }
    if (Map)
        Map->CenterOnPlayer();
    if (Map)
    {
        Map->SavedScrollX = 0;
        Map->SavedScrollY = 0;
    }
}
NW_HOOK_METHOD(0x100afbb0, NCMapCoverWnd, ShowWindow)

// 0x100afb70 (vslot 68, name ours) - the mark the console reported (the first one of MarkPos), while the map shows it
INT NCMapCoverWnd::OnShowMark()
{
    if (Map && Map->bShowMark && Map->MarkPos.Num() > 0)
        Map->ShowPosition(Map->MarkPos(0).X, Map->MarkPos(0).Y, 0, 1);
    return 1;
}
NW_HOOK_METHOD(0x100afb70, NCMapCoverWnd, OnShowMark)

// 0x100afa50 (vslot 69, name ours) - the map follows the player (the stock code does not test Map here)
INT NCMapCoverWnd::OnShowMyPosition()
{
    INT PX = (INT)MyLocation().X;
    INT PY = (INT)MyLocation().Y;
    INT PZ = (INT)MyLocation().Z;
    Map->ShowPosition(PX, PY, PZ, 1);
    return 1;
}
NW_HOOK_METHOD(0x100afa50, NCMapCoverWnd, OnShowMyPosition)

// 0x100afb40 (vslot 70, name ours) - the combo's item is the continent: the map toggles when it is not that one
INT NCMapCoverWnd::OnContinent(INT Index)
{
    if (Map && Map->Continent != Index)
        Map->ToggleContinent();
    return 1;
}
NW_HOOK_METHOD(0x100afb40, NCMapCoverWnd, OnContinent)

// 0x100abe30 (ret 4; name ours) - the combo's selected item (a tail jump into NCComboBox::SetSelected)
void NCMapCoverWnd::SetContinent(INT Continent)
{
    if (Continent <= 1)
        ContinentCombo->SetSelected(Continent);
}
NW_HOOK_METHOD(0x100abe30, NCMapCoverWnd, SetContinent)

// 0x100b05d0 (vslot 37) - a title bar, the map under it and the three controls along the bottom
INT NCMapCoverWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.Minimap.MapBack"));

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_MAPCOVER_FRAME;
    FrameCtrl->SetPosition(0, 0, 334, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    Map = new NCMinimapWnd();
    Map->SetPosition(3, 21, 328, 328);
    CreateChild(Map, NWS_VISIBLE | NWS_UNKNOWN20);   // 0x22

    MarkButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    MarkButton->ID = NID_MAPCOVER_MARK;
    MarkButton->SetPosition(169, 355, 76, 23);
    CreateChild(MarkButton, NWS_VISIBLE);
    MarkButton->SetText(*GL2GameData.SysString[STR_ShowMark]);

    MyPosButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    MyPosButton->ID = NID_MAPCOVER_MYPOS;
    MyPosButton->SetPosition(249, 355, 76, 23);
    CreateChild(MyPosButton, NWS_VISIBLE);
    MyPosButton->SetText(*GL2GameData.SysString[STR_ShowMyPosition]);

    ContinentCombo = new NCComboBox();
    ContinentCombo->ID = NID_MAPCOVER_CONTINENT;
    ContinentCombo->SetPosition(13, 358, 108, 17);
    CreateChild(ContinentCombo, NWS_VISIBLE);
    // the stock buffer is 12 characters wide (it holds the short names of the two continents); room to spare here
    TCHAR Text[64];
    appSprintf(Text, TEXT("%s - %s"), *GL2GameData.SysString[STR_Map], *GL2GameData.SysString[STR_Aden]);
    ContinentCombo->AddItem(Text);
    appSprintf(Text, TEXT("%s - %s"), *GL2GameData.SysString[STR_Map], *GL2GameData.SysString[STR_Elmore]);
    ContinentCombo->AddItem(Text);
    return 1;
}
NW_HOOK_METHOD(0x100b05d0, NCMapCoverWnd, OnCreate)

NW_CHECK_VTABLE(NCMapCoverWnd, 0x10190be8, 71)
