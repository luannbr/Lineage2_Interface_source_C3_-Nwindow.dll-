/*=============================================================================
    NCEmpowerWnd.cpp - the clan member privileges window (C2 only). Stock: 0x10070ca0 ..
    0x10070d4f (WindowProc), 0x10070d50 .. 0x10070e6f, 0x10070e70 .. 0x10071a7f (OnCreate and
    OnDestroy), 0x10071a80 .. 0x1007221f (the helpers and the small virtuals) and
    0x100a0830 (OnCancel, shared with NCOptionWnd's).
=============================================================================*/
#include "nwindow/NCEmpowerWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <string.h>

// ids (names ours)
enum
{
    NID_EMPOWER_FRAME = (INT)0xFF0F1B01,
    NID_EMPOWER_CHECK = (INT)0xFF0F1A01,
    NID_EMPOWER_OK = (INT)0xFF0F1A11,        // the cancel button has it too
    NID_EMPOWER_HEAD_A = (INT)0xFF0F1A15,
    NID_EMPOWER_HEAD_B = (INT)0xFF0F1A16,
    NID_EMPOWER_HEAD_C = (INT)0xFF0F1A17,
};
enum { SYSSTR_EmpowerName = 686 };           // the text after the name (name ours)
enum { PLEDGEPOWER_Set = 3 };                // the second argument of RequestPledgePower (name ours)

// the three groups of the ten privileges: the first box and the count (names ours)
static INT GroupOf(INT Bit)
{
    return Bit < 4 ? 0 : (Bit < 7 ? 1 : 2);
}

// the thirteen check boxes in the order OnCreate makes them: the index in Checks, the label
// (a system string), the id, the place (names ours)
struct NEmpowerCheckDesc
{
    INT Index;
    INT LabelID;
    INT ID;
    INT X;
    INT Y;
};
static const NEmpowerCheckDesc GEmpowerChecks[13] = {
    { 10, 0x2A1, NID_EMPOWER_HEAD_A, 21, 87 },
    { 0, 0x2A2, NID_EMPOWER_CHECK, 32, 107 },
    { 1, 0x2A3, NID_EMPOWER_CHECK, 32, 126 },
    { 2, 0x2A4, NID_EMPOWER_CHECK, 32, 145 },
    { 3, 0x2A5, NID_EMPOWER_CHECK, 32, 164 },
    { 11, 0x2A6, NID_EMPOWER_HEAD_B, 21, 191 },
    { 4, 0x2A7, NID_EMPOWER_CHECK, 32, 210 },
    { 5, 0x2AC, NID_EMPOWER_CHECK, 32, 229 },
    { 6, 0x2AD, NID_EMPOWER_CHECK, 32, 248 },
    { 12, 0x2AA, NID_EMPOWER_HEAD_C, 21, 275 },
    { 7, 0x2AB, NID_EMPOWER_CHECK, 32, 294 },
    { 8, 0x2AC, NID_EMPOWER_CHECK, 32, 313 },
    { 9, 0x2AD, NID_EMPOWER_CHECK, 32, 332 },
};

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

// 0x1007d310 - the privileges, the member and the mode are empty; the name is an empty FString
NCEmpowerWnd::NCEmpowerWnd()
{
    memset(Power, 0, sizeof(Power));
    MemberID = 0;
    Mode = 0;
    BackTex = NULL;
}
NW_HOOK_CTOR(0x1007d310, NCEmpowerWnd)

// 0x1007d3c0
NW_HOOK_DTOR(0x1007d3c0, NCEmpowerWnd)

// 0x1007be30 (vslot 25)
INT NCEmpowerWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_EMPOWER_OK)
            return OnOK() != 0;
        if ((INT)wParam == NID_EMPOWER_HEAD_A)
            return OnHeadA() != 0;
        if ((INT)wParam == NID_EMPOWER_HEAD_B)
            return OnHeadB() != 0;
        if ((INT)wParam == NID_EMPOWER_HEAD_C)
            return OnHeadC() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1007be30, NCEmpowerWnd, WindowProc)

// 0x1007c0c0 - every box unchecked (the null tests are the stock code's)
void NCEmpowerWnd::ClearChecks()
{
    for (INT i = 0; i < 3; i++)
    {
        if (Checks[i])
            Checks[i]->bChecked = 0;
        if (Checks[4 + i])
            Checks[4 + i]->bChecked = 0;
        if (Checks[7 + i])
            Checks[7 + i]->bChecked = 0;
        if (Checks[10 + i])
            Checks[10 + i]->bChecked = 0;
    }
    if (Checks[3])
        Checks[3]->bChecked = 0;
}
NW_HOOK_METHOD(0x1007c0c0, NCEmpowerWnd, ClearChecks)

// 0x1007c120 - the boxes of the bits set are checked, and the head of their group with them
void NCEmpowerWnd::ShowPower()
{
    ClearChecks();
    for (INT i = 0; i < 10; i++)
    {
        if (Power[i >> 3] & (1 << (i & 7)))
        {
            Checks[i]->bChecked = 1;
            Checks[10 + GroupOf(i)]->bChecked = 1;
        }
    }
}
NW_HOOK_METHOD(0x1007c120, NCEmpowerWnd, ShowPower)

// 0x1007c280 - the boxes checked are the bits of Power (the others cleared); unrolled in the stock code
void NCEmpowerWnd::CollectPower()
{
    memset(Power, 0, sizeof(Power));
    if (Checks[0]->bChecked)
        Power[0] |= 0x01;
    if (Checks[1]->bChecked)
        Power[0] |= 0x02;
    if (Checks[2]->bChecked)
        Power[0] |= 0x04;
    if (Checks[3]->bChecked)
        Power[0] |= 0x08;
    if (Checks[4]->bChecked)
        Power[0] |= 0x10;
    if (Checks[5]->bChecked)
        Power[0] |= 0x20;
    if (Checks[6]->bChecked)
        Power[0] |= 0x40;
    if (Checks[7]->bChecked)
        Power[0] |= 0x80;
    if (Checks[8]->bChecked)
        Power[1] |= 0x01;
    if (Checks[9]->bChecked)
        Power[1] |= 0x02;
}
NW_HOOK_METHOD(0x1007c280, NCEmpowerWnd, CollectPower)

// 0x1007c570 - nothing shown, nobody, no name, every box enabled (in the order of ClearChecks)
void NCEmpowerWnd::Reset()
{
    ClearChecks();
    memset(Power, 0, sizeof(Power));
    MemberID = 0;
    Mode = 0;
    Name.Empty();
    for (INT i = 0; i < 3; i++)
    {
        Checks[i]->bDisabled = 0;
        Checks[4 + i]->bDisabled = 0;
        Checks[7 + i]->bDisabled = 0;
        Checks[10 + i]->bDisabled = 0;
    }
    Checks[3]->bDisabled = 0;
}
NW_HOOK_METHOD(0x1007c570, NCEmpowerWnd, Reset)

// 0x1007d2e0 (ret 0xC) - the console answer: the 32 bytes of privileges (the first two arguments are ignored)
void NCEmpowerWnd::SetPower(INT P1, INT P2, const BYTE* NewPower)
{
    memset(Power, 0, sizeof(Power));
    memcpy(Power, NewPower, sizeof(Power));
    ShowPower();
}
NW_HOOK_METHOD(0x1007d2e0, NCEmpowerWnd, SetPower)

// 0x1007c360 (vslot 68) - OK: the privileges of a member are sent when they could be changed (RequestPledgePower is
// slot 404 of this client console: the member, 3 = set, the bits)
INT NCEmpowerWnd::OnOK()
{
    if (MemberID > 0 && Mode == 2)
    {
        CollectPower();
        ::nw::VirtualCall<void>(GConsole, 404, MemberID, (INT)PLEDGEPOWER_Set, (BYTE*)Power);
    }
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1007c360, NCEmpowerWnd, OnOK)

// 0x100a0830 (vslot 68) - the stock body is one with NCOptionWnd's (hooked there); nothing calls it here
INT NCEmpowerWnd::OnCancel()
{
    HideWindow();
    return 1;
}

// the head of a group: its boxes follow it (not while the head is disabled)
#define EMPOWER_HEAD(Head, First, Count)                                           \
    {                                                                              \
        NCCheckBox* H = Checks[Head];                                              \
        if (H->bDisabled == 0)                                                     \
        {                                                                          \
            if (H->bChecked != 0)                                                  \
            {                                                                      \
                for (INT i = First; i < First + Count; i++)                        \
                    Checks[i]->bChecked = 1;                                       \
                return 1;                                                          \
            }                                                                      \
            if (H->bChecked == 0)                                                  \
            {                                                                      \
                for (INT i = First; i < First + Count; i++)                        \
                    Checks[i]->bChecked = 0;                                       \
            }                                                                      \
        }                                                                          \
        return 1;                                                                  \
    }

// 0x1007c3b0 (vslot 70)
INT NCEmpowerWnd::OnHeadA()
{
    EMPOWER_HEAD(10, 0, 4)
}
NW_HOOK_METHOD(0x1007c3b0, NCEmpowerWnd, OnHeadA)

// 0x1007c450 (vslot 71)
INT NCEmpowerWnd::OnHeadB()
{
    EMPOWER_HEAD(11, 4, 3)
}
NW_HOOK_METHOD(0x1007c450, NCEmpowerWnd, OnHeadB)

// 0x1007c4e0 (vslot 72)
INT NCEmpowerWnd::OnHeadC()
{
    EMPOWER_HEAD(12, 7, 3)
}
NW_HOOK_METHOD(0x1007c4e0, NCEmpowerWnd, OnHeadC)

#undef EMPOWER_HEAD

// 0x1007c650 (vslot 44) - in view mode every box is disabled
void NCEmpowerWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Mode == 1)
    {
        for (INT i = 0; i < 3; i++)
        {
            Checks[i]->bDisabled = 1;
            Checks[4 + i]->bDisabled = 1;
            Checks[7 + i]->bDisabled = 1;
            Checks[10 + i]->bDisabled = 1;
        }
        Checks[3]->bDisabled = 1;
    }
}
NW_HOOK_METHOD(0x1007c650, NCEmpowerWnd, ShowWindow)

// 0x1007c710 (vslot 45) - closing empties it
void NCEmpowerWnd::HideWindow()
{
    NCWnd::HideWindow();
    Reset();
}
NW_HOOK_METHOD(0x1007c710, NCEmpowerWnd, HideWindow)

// 0x1007c8a0 (vslot 37) - the title bar, the thirteen boxes (200 x 12) and the two buttons
INT NCEmpowerWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("l2ui_ch3.bloodhoodwnd.empower_back"));

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->SetPosition(0, 0, 256, 20);
    FrameCtrl->ID = NID_EMPOWER_FRAME;
    CreateChild(FrameCtrl, NWS_VISIBLE);

    for (INT i = 0; i < 13; i++)
    {
        const NEmpowerCheckDesc& Desc = GEmpowerChecks[i];
        NCCheckBox* Box = new NCCheckBox(Desc.LabelID, 0, 0);
        Checks[Desc.Index] = Box;
        Box->ID = Desc.ID;
        Box->SetPosition(Desc.X, Desc.Y, 200, 12);
        CreateChild(Box, NWS_VISIBLE);
    }

    OKButton = new NCButton(TEXT("L2UI_ch3.Button.btn1_normal"), TEXT("L2UI_ch3.Button.btn1_normalon"), 0);
    OKButton->ID = NID_EMPOWER_OK;
    OKButton->SetPosition(51, 372, 76, 23);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(140);

    CancelButton = new NCButton(TEXT("L2UI_ch3.Button.btn1_normal"), TEXT("L2UI_ch3.Button.btn1_normalon"), 0);
    CancelButton->ID = NID_EMPOWER_OK;
    CancelButton->SetPosition(131, 372, 76, 23);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(141);
    return 1;
}
NW_HOOK_METHOD(0x1007c8a0, NCEmpowerWnd, OnCreate)

// 0x1007bef0 (vslot 38) - guarded in C2; NCWnd's OnDestroy comes first
INT NCEmpowerWnd::OnDestroy()
{
    guard(NCEmpowerWnd::OnDestroy);
    NCWnd::OnDestroy();
    for (INT i = 0; i < 3; i++)
    {
        DestroyChild(this, Checks[i]);
        DestroyChild(this, Checks[4 + i]);
        DestroyChild(this, Checks[7 + i]);
        DestroyChild(this, Checks[10 + i]);
    }
    DestroyChild(this, Checks[3]);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007bef0, NCEmpowerWnd, OnDestroy)

// 0x1007c720 (vslot 56) - the panel (from the title bar down) and, when there is a name, the name in yellow and the
// text after it; the clip goes on before the origin
INT NCEmpowerWnd::OnPaint(NCGDevice* Device)
{
    guard(NCEmpowerWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 20, (INT)Width, (INT)(Height - 20.0f), 0, 0, 256, 381, BackTex, 0xFF, 1);
    if (Name.Num() != 0)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, *Name, 0, 0, 0, 0, 0);
        Device->DrawText(23, 54, 0xFFFFFF00, *Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->DrawText(Size.cx + 23, 54, 0xFFDCDCDC, *SysString(SYSSTR_EmpowerName), 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                         0, 0, 0);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007c720, NCEmpowerWnd, OnPaint)

NW_CHECK_VTABLE(NCEmpowerWnd, 0x10187138, 73)

// 0x10047610 (name ours) - the pledge window sets whose privileges these are: the member's id and the mode, and the name
// (kept when none is given)
void NCEmpowerWnd::SetMember(INT InMemberID, const TCHAR* InName, INT InMode)
{
    MemberID = InMemberID;
    Mode = InMode;
    if (InName)
        Name = InName;
}
NW_HOOK_METHOD(0x10047610, NCEmpowerWnd, SetMember)
