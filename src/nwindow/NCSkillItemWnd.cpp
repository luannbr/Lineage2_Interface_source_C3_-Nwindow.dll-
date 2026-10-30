/*=============================================================================
    NCSkillItemWnd.cpp - the skill grids of the skill windows (C3 addresses). Stock: 0x100a1050 .. 0x100a2250
    (WindowProc, mouse, paint, Clear, AddSkill), the constructors 0x100a2250 / 0x100a22f0, the destructor 0x100840f0,
    and NCGMSkillItemWnd's 0x1007e260 .. 0x1007e29f, 0x100848f0, 0x10084940, 0x10084990.
=============================================================================*/
#include "nwindow/NCSkillItemWnd.h"
#include "nwindow/NCCoolTimeIcon.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCMacroEditList.h"
#include "nwindow/NCMacroEditWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NConsoleWnd.h"

enum { SOUND_CoolTimeEnd = 9 };  // interface sound of a cool time ending (name ours)

// the "/" a macro line made from a skill starts with: this unit's pointer to the string at 0x10107e78 (name ours)
#if NW_HOOK_MODE
#define GSkillItemCmdPrefix NW_GLOBAL(const TCHAR*, 0x1021971C)
#else
extern const TCHAR* GSkillItemCmdPrefix;
#endif

// C3: the console's IsAttackKeyDown (vslot 297, 0x10066410: Ctrl on the engine's input or the console's flag) is what
// the stock grid calls for "Ctrl is down"; NConsoleWnd's IsCtrlDown is a plain flag read of ours with no stock body
static UBOOL ConsoleKeyDownCtrl()
{
    return ::nw::VirtualCall<UBOOL>(GConsole, 297);
}

// 0x100a10a0 (ret 8; the shophtml lane's anonymous function): NCItemWnd's HitTest of the grid with the cell gaps 5 and 3
// as constants (the stock grid draws and hits cells CellSize + 5 across and CellSize + 3 down)
static FORCEINLINE INT SkillHitTest(NCSkillItemWnd* Wnd, INT X, INT Y)
{
    return Wnd->HitTestSkill(X, Y);
}

/*-----------------------------------------------------------------------------
    NCSkillItemWnd
-----------------------------------------------------------------------------*/

// 0x100a2250 - the default one: the icon frames of the inventory window (NCItemWnd's default constructor first;
// the cool times start empty)
NCSkillItemWnd::NCSkillItemWnd()
{
    bPressed = 0;
    IconBackTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.inventory_outline"));
    IconClickTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.inventory_outline_down"));
    ToggleTex = FindTexture(TEXT("L2_SkillTime.ToggleEffect.ToggleEffect001"));
}
NW_HOOK_CTOR(0x100a2250, NCSkillItemWnd)

// 0x100a22f0 (sub_100a22f0, ret 0x18) - the skill windows' lists: columns, visible rows, most items, cell size and the
// two cell gaps go to NCItemWnd; the icon frames are the NWindow ones
NCSkillItemWnd::NCSkillItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
    bPressed = 0;
    IconBackTex = FindTexture(TEXT("l2ui.nwindow.icon_back"));
    IconClickTex = FindTexture(TEXT("l2ui.nwindow.icon_click"));
    ToggleTex = FindTexture(TEXT("L2_SkillTime.ToggleEffect.ToggleEffect001"));
}
NW_HOOK_CTOR_ARGS(0x100a22f0, NCSkillItemWnd, (INT A, INT B, INT C, INT D, INT E, INT F), (A, B, C, D, E, F))

// 0x100840f0 - the cool times' array goes (0x10083640) and NCItemWnd's destructor follows
NW_HOOK_DTOR(0x100840f0, NCSkillItemWnd)

// 0x100a1050 (vslot 25) - the base's answer as a plain 0 / 1 (no enabled test in C3)
INT NCSkillItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCItemWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x100a1050, NCSkillItemWnd, WindowProc)

// the cool time icons go (repeated inline in the stock OnDestroy and Clear)
static FORCEINLINE void DeleteCoolTimes(TArray<NCCoolTimeIcon*>& CoolTimes)
{
    for (INT i = 0; i < CoolTimes.Num(); i++)
        if (CoolTimes(i))
            delete CoolTimes(i);
    CoolTimes.Empty();
}

// 0x100a1fb0 (vslot 38)
INT NCSkillItemWnd::OnDestroy()
{
    guard(NCSkillItemWnd::OnDestroy);
    DeleteCoolTimes(CoolTimes);
    NCItemWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100a1fb0, NCSkillItemWnd, OnDestroy)

// 0x100a21a0 (vslot 72) - the items are deleted here and NCItemWnd's finds none left
void NCSkillItemWnd::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i))
            delete Items(i);
    Items.Empty();
    DeleteCoolTimes(CoolTimes);
    NCItemWnd::Clear();
}
NW_HOOK_METHOD(0x100a21a0, NCSkillItemWnd, Clear)

// A new cool time icon as the stock code makes one inline: GMalloc->Malloc(0x58, "new"), then the constructor
static FORCEINLINE NCCoolTimeIcon* NewCoolTimeIcon()
{
    NCCoolTimeIcon* Icon = (NCCoolTimeIcon*)appMalloc(sizeof(NCCoolTimeIcon), TEXT("new"));
    if (Icon)
        Icon = new ((void*)Icon) NCCoolTimeIcon;
    return Icon;
}

// 0x100a2080 - from NCSkillWnd::AddSkill; MaxItems is not checked. The skill's cool time icon is added stopped (the
// console starts it).
INT NCSkillItemWnd::AddSkill(NItemInfo* Info)
{
    Info->Icon = FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = NewItemEntry(Info);
    CoolTimes.AddItem(NewCoolTimeIcon());
    Item->FillToolTip(4, 0);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100a2080, NCSkillItemWnd, AddSkill)

// 0x100a1180 (vslot 56) - NCItemWnd's grid (its guard name too) with literal gaps (CellSize + 5 across, + 3 down),
// the selection frame the size of the icon and over each skill, in order: the icon frame of an active skill
// (NItemEntry Type 0) and the click frame of the pressed one, the toggle animation of a toggle skill whose effect is
// on the player, the end flash with its sound once, the cool time
INT NCSkillItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCItemWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 1;
    INT Y = 1;
    for (INT i = TopRow * Columns; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (!Item)
            continue;
        if (i % Columns == 0)
        {
            X = 1;
            if (i != TopRow * Columns)
                Y += CellSize + 3;
            if (VisibleRows * (CellSize + 1) <= Y)
                break;
        }
        else
            X += CellSize + 5;
        if (Item->IsSelected())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, SelectTex, 0xFF, 1);
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon(), 0xFF, 1);
        if (Item->Type == 0)
        {
            if (IconBackTex)
                Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, IconBackTex, 0xFF, 1);
            if (IconClickTex && Selected == i && bPressed)
                Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, IconClickTex, 0xFF, 1);
        }
        if (GConsole->IsToggleSkill(Item->ID, Item->UnknownC18) && GConsole->HasAbnormal(Item->ID))
            Device->DrawTexture(X, Y, 32, 32, 0, 0, 32, 32, ToggleTex, 0xFF, 1);
        if (CoolTimes(i)->bFlashing)
        {
            Device->DrawBrightenTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize,
                                        CoolTimes(i)->GetFlashTexture(), 0xFF008080);
            if (CoolTimes(i)->bSoundDue)
            {
                GConsole->PlayInterfaceSound(SOUND_CoolTimeEnd);
                CoolTimes(i)->bSoundDue = 0;
            }
        }
        if (CoolTimes(i)->bRunning && !CoolTimes(i)->IsIdle() && CoolTimes(i)->GetTexture())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, CoolTimes(i)->GetTexture(), 0xFF, 1);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100a1180, NCSkillItemWnd, OnPaint)

// 0x100e4980 (vslot 57) - the body of NCRecipeManufactureItemWnd's slot 57, shared; not this lane's
#if NW_HOOK_MODE
NW_NAKED INT NCSkillItemWnd::OnDrawToolTip(NCGDevice* Device) NW_STOCK(0x100e4980)
#else
NW_SLOT_IS("?OnDrawToolTip@NCSkillItemWnd@@UAEHPAVNCGDevice@@@Z", "?OnDrawToolTip@NCRecipeManufactureItemWnd@@UAEHPAVNCGDevice@@@Z")   // NCSkillItemWnd::OnDrawToolTip = NCRecipeManufactureItemWnd::OnDrawToolTip (0x100e4980)
#endif

// 0x100a1070 (vslot 62)
INT NCSkillItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    NCItemWnd::OnLButtonDown(wParam, lParam);
    bPressed = 1;
    return 1;
}
NW_HOOK_METHOD(0x100a1070, NCSkillItemWnd, OnLButtonDown)

// 0x100a14f0 (vslot 61) - the icon under the mouse; while a skill is carried the cursor is its icon and an edit line of
// the macro editor (the window the desktop keeps at +0x130) under the mouse is marked (+0x208 of the edit box);
// anywhere else the editor is told to unmark. C3: with nothing carried nothing is done (C2 put the cursor back unless
// Ctrl was down). The guard string has the original's typo
INT NCSkillItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    guard(NCSKillItemWnd::OnMouseMove);
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    HoverIndex = SkillHitTest(this, X, Y);
    if (IsFocused() && bDragging)
    {
        if (Selected >= 0 && Selected < Items.Num())
        {
            SetCursor(4, Items(Selected)->GetIcon());
            LockCursor();
        }
        NWndEntry* Found = FindWindowAt(X, Y);
        if (Found && Found->Wnd && Found->Wnd->IsA(NClass::NCEditBox()))
        {
            // the edit line's grandparent: the macro editor
            NCWnd* Editor = NULL;
            if (Found->Parent && Found->Parent->Parent)
                Editor = Found->Parent->Parent->Wnd;
            if (Editor)
            {
                NCConsole* Desk = GDesktop;
                if (Desk && Desk->NewWndFC == Editor)
                {
                    NCMacroEditList* Lines = ((NCMacroEditWnd*)Editor)->EditList;
                    INT Line = Lines->HitLine(X, Y);
                    if (Line >= 0)
                        Lines->Edits(Line)->Unknown1D8 = 1;
                    return 1;
                }
                NCWnd* Target = Editor;
                if (Desk)
                    Target = Desk->NewWndFC;
                if (Target)
                    ((NCMacroEditWnd*)Target)->ClearMarks();
                return 1;
            }
        }
        if (GConsole && GDesktop && GDesktop->NewWndFC)
            ((NCMacroEditWnd*)GDesktop->NewWndFC)->ClearMarks();
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100a14f0, NCSkillItemWnd, OnMouseMove)

// 0x100a16c0 (vslot 63) - released on the icon clicked: the skill is used (the server gets its id, Ctrl and Shift:
// NetHandler 0x268, the function this header calls RequestSkillList, 0x1005d040); dropped on a slot of the shortcut bar:
// the server registers it there (type 2, then 1); dropped on a line of the macro window's edit lines (the window the
// desktop keeps at +0x130, the macro editor), a skill (Type 0) writes its command there - "/<command> <name>" when
// it fits in 32 characters - and the editor is told the line (it marks it). C3: the cursor is not put back here
INT NCSkillItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    guard(NCSkillItemWnd::OnLButtonUp);
    bPressed = 0;
    if (Selected >= 0 && Selected < Items.Num())
    {
        INT X = LOWORD(lParam);
        INT Y = HIWORD(lParam);
        if (SkillHitTest(this, X, Y) == Selected)
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)(INT_PTR)Items(Selected)->ID);
            Params.PushBack((void*)(INT_PTR)ConsoleKeyDownCtrl());
            Params.PushBack(
                (void*)(INT_PTR)GConsole->Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
            GConsole->RequestSkillList(Params);   // 0x1005d040: the skill USE request in this client
        }
        else if (bDragging)
        {
            NWndEntry* Target = FindWindowAt(X, Y);
            if (Target && Target->Wnd)
            {
                NCWnd* Wnd = Target->Wnd;
                if (Wnd->IsA(NClass::NCShortCutWnd()))
                {
                    // the bar's slot under the point (by the bar's layout), -1 none
                    INT Slot = ((NCShortCutWnd*)Wnd)->SlotAtPoint(X, Y);
                    if (Slot >= 0)
                    {
                        L2ParamStack Params(10);
                        Params.PushBack((void*)(INT_PTR)SHORTCUT_Skill);
                        Params.PushBack((void*)(INT_PTR)Slot);
                        Params.PushBack((void*)(INT_PTR)Items(Selected)->ID);
                        Params.PushBack((void*)1);
                        GConsole->RequestShortCutReg(Params);
                    }
                }
                else if (Wnd->IsA(NClass::NCEditBox()))
                {
                    // the edit line's grandparent: the macro editor
                    NCWnd* Editor = NULL;
                    if (Target->Parent && Target->Parent->Parent)
                        Editor = Target->Parent->Parent->Wnd;
                    if (Editor && GDesktop && GDesktop->NewWndFC == Editor)
                    {
                        // the editor's lines (a control at +0xCC holds the edit boxes in an array at +0x100)
                        NCMacroEditList* Lines = ((NCMacroEditWnd*)Editor)->EditList;
                        INT Line = Lines->HitLine(X, Y);
                        if (Line >= 0)
                        {
                            if (Items(Selected)->Type == 0)
                            {
                                TCHAR Command[1028];
                                appSprintf(Command, TEXT("%s%s %s"), GSkillItemCmdPrefix,
                                           *GL2GameData.Unknown54188[25], Items(Selected)->Name);
                                if (appStrlen(Command) <= 32)
                                {
                                    NCEditBox* Edit = Lines->Edits(Line);
                                    Edit->ClearText();
                                    Edit->InsertText(Command, 1);
                                }
                            }
                            ((NCMacroEditWnd*)Editor)->ClearMark(Line);
                        }
                    }
                }
            }
        }
        UnselectAll();
        Selected = -1;
    }
    bDragging = 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100a16c0, NCSkillItemWnd, OnLButtonUp)

NW_CHECK_VTABLE(NCSkillItemWnd, 0x10187dd0, 75)

/*-----------------------------------------------------------------------------
    NCGMSkillItemWnd
-----------------------------------------------------------------------------*/

// 0x100848f0 - NCSkillItemWnd's default constructor, then its own vtable
NCGMSkillItemWnd::NCGMSkillItemWnd()
{
}
NW_HOOK_CTOR(0x100848f0, NCGMSkillItemWnd)

// 0x10084990 (sub_10084990, ret 0x18) - the six-argument one the GM skill window's lists are made with
NCGMSkillItemWnd::NCGMSkillItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX,
                                   INT InGapY)
    : NCSkillItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
}
NW_HOOK_CTOR_ARGS(0x10084990, NCGMSkillItemWnd, (INT A, INT B, INT C, INT D, INT E, INT F), (A, B, C, D, E, F))

// 0x10084940
NW_HOOK_DTOR(0x10084940, NCGMSkillItemWnd)

// 0x1007e260 (vslot 25) - NCItemWnd's answer as a plain 0 / 1
INT NCGMSkillItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCItemWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1007e260, NCGMSkillItemWnd, WindowProc)

// 0x1007e280 (vslot 63) - the carried skill is only put down; also NCGMInvenItemWnd's slots
// 57, 58 and 68
INT NCGMSkillItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1007e280, NCGMSkillItemWnd, OnLButtonUp)

NW_CHECK_VTABLE(NCGMSkillItemWnd, 0x10188710, 75)
