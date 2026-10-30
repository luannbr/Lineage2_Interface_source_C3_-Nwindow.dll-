/*=============================================================================
    NCPartyWnd.cpp - the party window. Stock: 0x100fb1d0 .. 0x100fb22f, 0x100fb230 .. 0x100fb248,
    0x100fd230 .. 0x100fd2fb, 0x100fd310 .. 0x100fd41f, 0x100fd420 .. 0x100fd601, 0x10101a80 .. 0x10101c0b,
    0x10103550 .. 0x10103570, 0x101035f0 .. 0x10103830.
=============================================================================*/
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"

// NWM_COMMAND id of the button that switches the members' buff icons
enum { NID_PARTY_TOGGLE = 0xFF0001F2 };

// title bar id
enum { NID_PARTY_FRAME = 0xFFA0B0 };

// the height of a member's window (SetPosition) and the step the members below move by
enum { PARTY_MEMBER_HEIGHT = 46 };

// 0x10103550 - C3: the constructor of the window clears two fields
NCPartyWnd::NCPartyWnd()
{
    bShown = 0;
    NumMembers = 0;
}
NW_HOOK_CTOR(0x10103550, NCPartyWnd)

// 0x101035a0
NW_HOOK_DTOR(0x101035a0, NCPartyWnd)

// 0x100fb1d0 (vslot 25) - the buff button
INT NCPartyWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == (INT)NID_PARTY_TOGGLE)
        return OnToggle() != 0;
    return 0;
}
NW_HOOK_METHOD(0x100fb1d0, NCPartyWnd, WindowProc)

// NCFrameCtrl(Decorated, Vertical, Small): 0x10010450 (ret 0xc, NCFrameCtrl.cpp), run on the object `new` allocates
// (StaticAllocateObject of the class in the transient package)
static NCFrameCtrl* NewFrameCtrl(INT A, INT B, INT C)
{
    return new NCFrameCtrl(A, B, C);
}

// 0x101035f0 (vslot 37) - C3: the title bar, the buff button and eight member windows
INT NCPartyWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameTopTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar1"));
    FrameCenterTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar2"));
    FrameBottomTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar3"));

    BuffButton = new NCButton(TEXT("L2UI_CH3.PartyWnd.party_buffbutton"),
                              TEXT("L2UI_CH3.PartyWnd.party_buffbutton_down"), 0);
    BuffButton->SetPosition(1, 7, 11, 13);
    BuffButton->ID = (INT)NID_PARTY_TOGGLE;
    CreateChild(BuffButton, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    BuffButton->SetToolTip(0x371, 1);

    FrameCtrl = NewFrameCtrl(0, 0, 0);
    FrameCtrl->SetPosition(0, 18, 12, (DWORD)(INT)Height);
    FrameCtrl->ID = NID_PARTY_FRAME;
    CreateChild(FrameCtrl, NWS_VISIBLE);

    for (INT i = 0; i < NUM_PARTY_SLOTS; i++)
    {
        Members[i] = new NCPartyStatusWnd(144);
        Members[i]->SetPosition(12, 0, 160, PARTY_MEMBER_HEIGHT);
        CreateChild(Members[i], NWS_VISIBLE);
    }
    bToggle = 0;
    return 1;
}
NW_HOOK_METHOD(0x101035f0, NCPartyWnd, OnCreate)

// 0x100fd230 (vslot 38) - the members, then the buff button
INT NCPartyWnd::OnDestroy()
{
    guard(NCPartyWnd::OnDestroy);
    NCWnd::OnDestroy();
    for (INT i = 0; i < NUM_PARTY_SLOTS; i++)
    {
        if (Members[i])
        {
            RemoveChildWindow(Members[i]->Entry);
            delete Members[i];
            Members[i] = NULL;
        }
    }
    if (BuffButton)
    {
        RemoveChildWindow(BuffButton->Entry);
        delete BuffButton;
        BuffButton = NULL;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100fd230, NCPartyWnd, OnDestroy)

// 0x100fd600 (vslot 44)
void NCPartyWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    UpdateLayout();
}
NW_HOOK_METHOD(0x100fd600, NCPartyWnd, ShowWindow)

// 0x100fd310 (vslot 56) - the left edge (top, center stretched, bottom) while there are members
INT NCPartyWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (NumMembers > 0)
    {
        if (FrameTopTex)
            Device->DrawTexture(0, 0, 12, 8, 0, 0, 12, 8, FrameTopTex, 0xFF, 1);
        if (FrameCenterTex)
            Device->DrawTexture(0, 8, 12, (INT)(Height - 16.0f), 0, 0, 12, 8, FrameCenterTex, 0xFF, 1);
        if (FrameBottomTex)
            Device->DrawTexture(0, (INT)(Height - 8.0f), 12, 8, 0, 0, 12, 8, FrameBottomTex, 0xFF, 1);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100fd310, NCPartyWnd, OnPaint)

// 0x100fb230 (vslot 68; name ours) - the members' windows read it (buff icons shown)
INT NCPartyWnd::OnToggle()
{
    bToggle = !bToggle;
    return 1;
}
NW_HOOK_METHOD(0x100fb230, NCPartyWnd, OnToggle)

// 0x100fd420 - C3: a seventh argument (the class); the window hides itself when no member is left
void NCPartyWnd::UpdateMember(INT MemberID, const TCHAR* Name, INT HP, INT MaxHP, INT MP, INT MaxMP, INT ClassID)
{
    if (Name)
    {
        for (INT i = 0; i < NumMembers; i++)
        {
            if (Members[i]->IsName(Name))
            {
                Members[i]->UpdateInfo(MemberID, Name, HP, MaxHP, MP, MaxMP, ClassID);
                break;
            }
        }
        if (NumMembers == 0)
            HideWindow();
    }
}
NW_HOOK_METHOD(0x100fd420, NCPartyWnd, UpdateMember)

// 0x100fd4b0 - the window is 172 wide; each member shown is 46 high and the ones below it move down
void NCPartyWnd::UpdateLayout()
{
    NRectF Rect = {0, 0, 0.0f, 0.0f};
    SetWindowSize(172.0f, 0.0f);
    GetLocalRect(&Rect);
    for (INT i = 0; i < NUM_PARTY_SLOTS; i++)
        Members[i]->SetScreenPos(12, 0, (const POINT*)&Rect);
    UBOOL bSized = 0;
    for (INT i = 0; i < NUM_PARTY_SLOTS; i++)
    {
        if (!Members[i]->IsEmpty())
        {
            Members[i]->ShowWindow();
            Members[i]->UpdateStyle();
            NRectF Cur = {0, 0, 0.0f, 0.0f};
            GetLocalRect(&Cur);
            if (!bSized)
            {
                SetWindowSize(172.0f, Cur.H + 46.0f);
                bSized = 1;
            }
            else
                SetWindowSize(172.0f, Cur.H + 46.0f);
            for (INT j = i + 1; j < NUM_PARTY_SLOTS; j++)
                Members[j]->OffsetWindow(0, PARTY_MEMBER_HEIGHT);
        }
        else
            Members[i]->HideWindow();
    }
    FrameCtrl->SetPosition(0, 18, 12, (DWORD)(INT)Height);
}
NW_HOOK_METHOD(0x100fd4b0, NCPartyWnd, UpdateLayout)

// 0x101019e0 - C3: the packet adds the leader and the loot rule in front and the class at the end
void NCPartyWnd::AddMember(INT LeaderID, INT LootRule, INT MemberID, const TCHAR* Name, INT HP, INT MaxHP, INT MP,
                           INT MaxMP, INT ClassID)
{
    if (Name)
    {
        ShowWindow();
        INT i;
        for (i = 0; i < NUM_PARTY_SLOTS; i++)
            if (Members[i]->IsEmpty())
                break;
        if (i >= NUM_PARTY_SLOTS)
            return;
        Members[i]->SetInfo(LeaderID, LootRule, MemberID, Name, HP, MaxHP, MP, MaxMP, ClassID);
        NumMembers++;
        bShown = 1;
        UpdateLayout();
    }
}
NW_HOOK_METHOD(0x101019e0, NCPartyWnd, AddMember)

// 0x10101a80 - by name; the members below move up one place, buffs included
void NCPartyWnd::DeleteMember(INT MemberID, const TCHAR* Name)
{
    if (Name)
    {
        INT i;
        for (i = 0; i < NUM_PARTY_SLOTS; i++)
            if (Members[i]->IsName(Name))
                break;
        if (i >= NUM_PARTY_SLOTS)
            return;
        Members[i]->Clear();
        for (INT j = i + 1; j < NUM_PARTY_SLOTS; j++)
        {
            NCPartyStatusWnd* Src = Members[j];
            if (!Src->IsEmpty())
            {
                Members[j - 1]->UpdateInfo(Src->MemberID, Src->Name, Src->HP, Src->MaxHP, Src->MP, Src->MaxMP,
                                           Src->ClassID);
                for (INT k = 0; k < Members[j]->BuffSkillIDs.Num(); k++)
                    Members[j - 1]->AddBuff(Members[j]->BuffSkillIDs(k));
                Members[j]->Clear();
            }
        }
        NumMembers--;
        if (NumMembers == 0)
        {
            bShown = 0;
            HideWindow();
        }
        UpdateLayout();
    }
}
NW_HOOK_METHOD(0x10101a80, NCPartyWnd, DeleteMember)

// 0x10101bb0
void NCPartyWnd::DeleteAll()
{
    for (INT i = 0; i < NumMembers; i++)
        Members[i]->Clear();
    NumMembers = 0;
    bShown = 0;
    UpdateLayout();
    HideWindow();
}
NW_HOOK_METHOD(0x10101bb0, NCPartyWnd, DeleteAll)

NW_CHECK_VTABLE(NCPartyWnd, 0x101a0270, 69)
