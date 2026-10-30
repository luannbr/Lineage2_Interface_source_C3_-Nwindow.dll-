/*=============================================================================
    NCSkillWnd.cpp - the skill list and its GM copy (C3 addresses). Stock: 0x100a1130, 0x100a1150, 0x100a1aa0,
    0x100a23b0 .. 0x100a2470 .. 0x100a26a0, 0x1007e2a0 (WindowProc is NCButton's body 0x100127a0, OnPaint
    NCGMActionWnd's 0x10081e00), NCGMSkillWnd's 0x1007f780, 0x100849d0 .. 0x10084cc8.
=============================================================================*/
#include "nwindow/NCSkillWnd.h"
#include "nwindow/NCSkillItemWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NCGDevice.h"

/*-----------------------------------------------------------------------------
    NCSkillWnd
-----------------------------------------------------------------------------*/

// 0x100a23b0 - NControl's constructor is inlined in the stock one; only the tab control is cleared
NCSkillWnd::NCSkillWnd()
{
    TabCtrl = NULL;
}
NW_HOOK_CTOR(0x100a23b0, NCSkillWnd)

// 0x100a2420
NW_HOOK_DTOR(0x100a2420, NCSkillWnd)

// 0x100127a0 (vslot 25) - NCButton's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCSkillWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x100127a0)
#else
NW_SLOT_IS("?WindowProc@NCSkillWnd@@UAEHIIJ@Z", "?WindowProc@NCButton@@UAEHIIJ@Z")   // NCSkillWnd::WindowProc = NCButton::WindowProc (0x100127a0)
#endif

// a list of the window (repeated inline in the stock OnCreate): 6 x 8 icons of 32, 5 apart across and 3 down
static FORCEINLINE NCSkillItemWnd* CreateSkillList(NCWnd* Parent)
{
    NCSkillItemWnd* List = new NCSkillItemWnd(6, 8, 80, 32, 5, 3);
    List->SetPosition(9, 40, 239, 280);
    Parent->CreateChild(List, NWS_VISIBLE | 0x20);
    return List;
}

// 0x100a2470 (vslot 37) - the tabs "Active" / "Passive" (sysstring 120 / 121); C3: the first tab is the one shown
INT NCSkillWnd::OnCreate()
{
    NControl::OnCreate();
    ActiveList = CreateSkillList(this);
    PassiveList = CreateSkillList(this);
    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_SKILL_TABS;
    TabCtrl->SetPosition(12, 8, 189, 23);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(0x78, ActiveList, 94, 23, TEXT("L2UI_CH3.skillwnd.skill_tab2"),
                    TEXT("L2UI_CH3.skillwnd.skill_tab1"), 1, -1);
    TabCtrl->AddTab(0x79, PassiveList, 94, 23, TEXT("L2UI_CH3.skillwnd.skill_tab2"),
                    TEXT("L2UI_CH3.skillwnd.skill_tab1"), 0, -1);
    return 1;
}
NW_HOOK_METHOD(0x100a2470, NCSkillWnd, OnCreate)

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

// 0x100a1aa0 (vslot 38) - also the GM window's (folded in the stock code)
INT NCSkillWnd::OnDestroy()
{
    guard(NCSkillWnd::OnDestroy);
    DestroyChild(this, ActiveList);
    DestroyChild(this, PassiveList);
    DestroyChild(this, TabCtrl);
    NControl::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100a1aa0, NCSkillWnd, OnDestroy)

// 0x100a1130 (vslot 44) - also NCGMSkillWnd's
void NCSkillWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
}
NW_HOOK_METHOD(0x100a1130, NCSkillWnd, ShowWindow)

// 0x10081e00 (vslot 56) - NCGMActionWnd's body, shared (ported there)
#if NW_HOOK_MODE
NW_NAKED INT NCSkillWnd::OnPaint(NCGDevice* Device) NW_STOCK(0x10081e00)
#else
NW_SLOT_IS("?OnPaint@NCSkillWnd@@UAEHPAVNCGDevice@@@Z", "?OnPaint@NCGMActionWnd@@UAEHPAVNCGDevice@@@Z")   // NCSkillWnd::OnPaint = NCGMActionWnd::OnPaint (0x10081e00)
#endif

// 0x1007e2a0 - into the passive list when NItemInfo::Param is 1; also NCGMSkillWnd's
INT NCSkillWnd::AddSkill(NItemInfo* Info)
{
    if (Info->Param == 1)
        PassiveList->AddSkill(Info);
    else
        ActiveList->AddSkill(Info);
    return 1;
}
NW_HOOK_METHOD(0x1007e2a0, NCSkillWnd, AddSkill)

// 0x100a1150 (sub_100a1150) - both lists empty (the stock body serves NCGMSkillWnd's as well)
void NCSkillWnd::ClearLists()
{
    ActiveList->Clear();
    PassiveList->Clear();
}
NW_HOOK_METHOD(0x100a1150, NCSkillWnd, ClearLists)

NW_CHECK_VTABLE(NCSkillWnd, 0x1018e320, 69)

/*-----------------------------------------------------------------------------
    NCGMSkillWnd
-----------------------------------------------------------------------------*/

// 0x100849d0
NCGMSkillWnd::NCGMSkillWnd()
{
    TabCtrl = NULL;
}
NW_HOOK_CTOR(0x100849d0, NCGMSkillWnd)

// 0x10084a40
NW_HOOK_DTOR(0x10084a40, NCGMSkillWnd)

// 0x100127a0 (vslot 25) - NCButton's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED INT NCGMSkillWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x100127a0)
#else
NW_SLOT_IS("?WindowProc@NCGMSkillWnd@@UAEHIIJ@Z", "?WindowProc@NCButton@@UAEHIIJ@Z")   // NCGMSkillWnd::WindowProc = NCButton::WindowProc (0x100127a0)
#endif

// a list of the GM window (repeated inline in the stock OnCreate): 6 x 7 icons
static FORCEINLINE NCGMSkillItemWnd* CreateGMSkillList(NCWnd* Parent)
{
    NCGMSkillItemWnd* List = new NCGMSkillItemWnd(6, 7, 80, 32, 5, 3);
    List->SetPosition(9, 40, 239, 280);
    Parent->CreateChild(List, NWS_VISIBLE | 0x20);
    return List;
}

// 0x10084a90 (vslot 37) - NCSkillWnd's with the GM lists (7 rows) and the tabs' id 0xF0F012
INT NCGMSkillWnd::OnCreate()
{
    NControl::OnCreate();
    ActiveList = CreateGMSkillList(this);
    PassiveList = CreateGMSkillList(this);
    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_GMSKILL_TABS;
    TabCtrl->SetPosition(12, 8, 189, 23);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(0x78, ActiveList, 94, 23, TEXT("L2UI_CH3.skillwnd.skill_tab2"),
                    TEXT("L2UI_CH3.skillwnd.skill_tab1"), 1, -1);
    TabCtrl->AddTab(0x79, PassiveList, 94, 23, TEXT("L2UI_CH3.skillwnd.skill_tab2"),
                    TEXT("L2UI_CH3.skillwnd.skill_tab1"), 0, -1);
    return 1;
}
NW_HOOK_METHOD(0x10084a90, NCGMSkillWnd, OnCreate)

// 0x1007f780 (vslot 38) - NCSkillWnd's (0x100a1aa0) body under its own guard
INT NCGMSkillWnd::OnDestroy()
{
    guard(NCGMSkillWnd::OnDestroy);
    DestroyChild(this, ActiveList);
    DestroyChild(this, PassiveList);
    DestroyChild(this, TabCtrl);
    NControl::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007f780, NCGMSkillWnd, OnDestroy)

// 0x100a1130 (vslot 44) - NCSkillWnd's body, shared (hooked there)
#if NW_HOOK_MODE
NW_NAKED void NCGMSkillWnd::ShowWindow() NW_STOCK(0x100a1130)
#else
NW_SLOT_IS("?ShowWindow@NCGMSkillWnd@@UAEXXZ", "?ShowWindow@NCSkillWnd@@UAEXXZ")   // NCGMSkillWnd::ShowWindow = NCSkillWnd::ShowWindow (0x100a1130)
#endif

// 0x10081e00 (vslot 56) - NCGMActionWnd's body, shared (ported there)
#if NW_HOOK_MODE
NW_NAKED INT NCGMSkillWnd::OnPaint(NCGDevice* Device) NW_STOCK(0x10081e00)
#else
NW_SLOT_IS("?OnPaint@NCGMSkillWnd@@UAEHPAVNCGDevice@@@Z", "?OnPaint@NCGMActionWnd@@UAEHPAVNCGDevice@@@Z")   // NCGMSkillWnd::OnPaint = NCGMActionWnd::OnPaint (0x10081e00)
#endif

// the stock code folded this with NCSkillWnd::AddSkill (0x1007e2a0)
INT NCGMSkillWnd::AddSkill(NItemInfo* Info)
{
    if (Info->Param == 1)
        PassiveList->AddSkill(Info);
    else
        ActiveList->AddSkill(Info);
    return 1;
}

// the stock code folded this with NCSkillWnd::ClearLists (0x100a1150)
void NCGMSkillWnd::ClearLists()
{
    ActiveList->Clear();
    PassiveList->Clear();
}

NW_CHECK_VTABLE(NCGMSkillWnd, 0x10188840, 69)
