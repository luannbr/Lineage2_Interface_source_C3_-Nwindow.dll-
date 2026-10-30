/*=============================================================================
    NCStatusWnd.cpp - the HP / MP windows: base, player, target, party member, pet, summon.
    Stock: 0x100ae300 .. 0x100ae9ac, 0x100aeb70 .. 0x100aebe3, 0x100af2b0 .. 0x100b16e0.
=============================================================================*/
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCProgressCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleFns.h"
#include "nwindow/NCStatusBarCtrl.h"
#include "nwindow/NGameData.h"

#include <stdio.h>
#include <wchar.h>

// the stock code calls the CRT's wcscpy; the intrinsic would copy inline
#pragma function(wcscpy)

// exp at the start of each level (index = level), a table of the stock image
#if NW_HOOK_MODE
#define GExpTable (reinterpret_cast<const INT*>(::nw::Addr(0x1019E4CC)))  // C3: read from the stock OnPaint (0x10100566)
#else
extern const INT GExpTable[];
#endif

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_WindowClose = 6 };

// title bar ids of the status windows, the summon's time bar
enum
{
    NID_PLAYERSTATUS_FRAME = 0xFFB000,
    NID_TARGETSTATUS_FRAME = 0xF8,
    NID_PETSTATUS_FRAME = 0xFFA0B1,
    NID_SUMMONSTATUS_FRAME = 0xFFA0B2,
    NID_SUMMONSTATUS_TIME = 0xF0A0B0,
    NID_SUMMONSTATUS_TOGGLE = 0xF0A0B1,  // C3: the buff icons' button
    NID_PETSTATUS_TOGGLE = 0xF0F00A1,    // C3
};

// C2 NConsoleWnd::RequestAttack (0x10059f20, ret 0x18) takes two more words than C1's: the
// callers push 0, 0 after the position
static void RequestAttackC2(INT ObjectID, const FVector& Origin)
{
    GConsole->RequestAttack(ObjectID, Origin, 0, 0);
}

// C3: the console's IsCtrlDown is vslot 297 (+0x4a4); NConsoleWnd.h has it non-virtual for now
static UBOOL ConsoleIsCtrlDown()
{
    return ::nw::VirtualCall<UBOOL>(GConsole, 297);
}

// NCFrameCtrl(Decorated, Vertical, Small): 0x10010450 (ret 0xc, NCFrameCtrl.cpp), run on the object `new` allocates
// (StaticAllocateObject of the class in the transient package)
static NCFrameCtrl* NewFrameCtrl(INT A, INT B, INT C)
{
    return new NCFrameCtrl(A, B, C);
}

// the name when it is not empty, else NULL (inlined at each use in the stock paints)
static FORCEINLINE const TCHAR* NonEmpty(const TCHAR* Text)
{
    return Text && *Text ? Text : NULL;
}

/*-----------------------------------------------------------------------------
    NCStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10102c70 - C3: the bar width goes to +0x128 (the new ClassID sits before it)
NCStatusWnd::NCStatusWnd(INT InBarWidth)
{
    MPTex = NULL;
    HPTex = NULL;
    EXPTex = NULL;
    BackTex = NULL;
    BarWidth = InBarWidth;
}
NW_HOOK_CTOR_ARGS(0x10102c70, NCStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x10102450 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x10102450, NCStatusWnd)


// stock body shared with NControl's (0x10004e90)
INT NCStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100fa720 (vslot 37) - C3: Clear is a virtual (slot 68), the party member's adds its buffs
INT NCStatusWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("NWindow.WhiteTexture"));
    Clear();
    return 1;
}
NW_HOOK_METHOD(0x100fa720, NCStatusWnd, OnCreate)

// 0x100fa750 (vslot 38)
INT NCStatusWnd::OnDestroy()
{
    guard(NCStatusWnd::OnDestroy);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100fa750, NCStatusWnd, OnDestroy)

// 0x100fc4a0 (vslot 56) - draws nothing
INT NCStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100fc4a0, NCStatusWnd, OnPaint)

// 0x100fa7d0
UBOOL NCStatusWnd::IsName(const TCHAR* InName)
{
    if (!InName || !Name[0])
        return 0;
    return wcscmp(Name, InName) == 0;
}
NW_HOOK_METHOD(0x100fa7d0, NCStatusWnd, IsName)

// 0x100fa800 (vslot 68)
void NCStatusWnd::Clear()
{
    Name[0] = 0;
    HP = 0;
    MaxHP = 0;
    MP = 0;
    MaxMP = 0;
}
NW_HOOK_METHOD(0x100fa800, NCStatusWnd, Clear)

// 0x100faf90 (name ours) - out of line like the stock code
__declspec(noinline) UBOOL NCStatusWnd::IsEmpty()
{
    return Name[0] == 0;
}
NW_HOOK_METHOD(0x100faf90, NCStatusWnd, IsEmpty)

/*-----------------------------------------------------------------------------
    NCPlayerStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10102db0 - C3: one more field cleared (+0x358)
NCPlayerStatusWnd::NCPlayerStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    Unknown358 = 0;
    ExtraWidth = 0;
}
NW_HOOK_CTOR_ARGS(0x10102db0, NCPlayerStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x10102e10 - the vtable, ConditionalDestroy, NCStatusWnd
NW_HOOK_DTOR(0x10102e10, NCPlayerStatusWnd)


// 0x100faa60 (vslot 25) - the right edge reports how far it was dragged (C2 has no IsEnabled
// check of its own: NCStatusWnd's does it)
INT NCPlayerStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCStatusWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_PLAYERSTATUS_RESIZE)
        return OnResize((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100faa60, NCPlayerStatusWnd, WindowProc)

// 0x10102e60 (vslot 37) - C3: the title bar (undecorated: no close button), the right edge that widens the
// window, the bar of the HP (a status bar control) and the textures
INT NCPlayerStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    MaxExtraWidth = 202;

    FrameCtrl = NewFrameCtrl(1, 1, 1);
    FrameCtrl->ID = NID_PLAYERSTATUS_FRAME;
    FrameCtrl->SetPosition(0, 0, 12, 76);
    CreateChild(FrameCtrl, NWS_VISIBLE | NWS_NOCLOSE);

    // NCResizeFrame's constructor (0x100105c0, ret 4) on the object `new` allocates
    ResizeFrame = new NCResizeFrame(1);
    ResizeFrame->ID = NID_PLAYERSTATUS_RESIZE;
    ResizeFrame->SetPosition((INT)(Width - 10.0f), 0, 10, (DWORD)(INT)Height);
    CreateChild(ResizeFrame, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown

    // the HP status bar (NCStatusBarCtrl, UClass 0x102b16d0): the constructor 0x101025e0 (ret 0x1c) takes the label (the
    // stock's string "HP" at 0x1019FE2C), the textures' size and the four textures
    HPBar = new NCStatusBarCtrl(TEXT("HP"), 8, 12,
        TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbar"), TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbar_back"),
        TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbarfill"), TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbarwarn1"));
    ((NControl*)HPBar)->ID = (INT)0xFFA103F0;
    HPBar->SetPosition(16, 33, 150, 12);
    CreateChild(HPBar, NWS_VISIBLE | 0x4);   // 6

    BackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.Smallwindow2_back2"));
    HPTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbar"));
    MPTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_mpbar"));
    EXPTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_expbar"));
    LeftBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.Smallwindow2_back1"));
    RightBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_sizecontrol1"));
    PlayerHPBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbar_back"));
    PlayerMPBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_mpbar_back"));
    PlayerEXPBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_expbar_back"));
    CPBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.smallbar_cpback"));
    CPTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.smallbar_cp"));
    LevelBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_levelback"));
    SizeControl2Tex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_sizecontrol2"));
    return 1;
}
NW_HOOK_METHOD(0x10102e60, NCPlayerStatusWnd, OnCreate)

// 0x100fc630 (vslot 38) - C2 guards it
INT NCPlayerStatusWnd::OnDestroy()
{
    guard(NCPlayerStatusWnd::OnDestroy);
    NCStatusWnd::OnDestroy();
    if (ResizeFrame)
    {
        RemoveChildWindow(ResizeFrame->Entry);
        delete ResizeFrame;
        ResizeFrame = NULL;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100fc630, NCPlayerStatusWnd, OnDestroy)

// 0x10100360 (vslot 56) - C3: the window is drawn from pieces (the left edge, the middle stretched to the width, the
// right edge, the level's frame, the handle of the right edge); the HP bar is a control of its own (HPBar: told the
// HP and the maximum by each paint); the bars under it are drawn here: CP (4 pixels high), MP and exp, BarW =
// ExtraWidth + 150 pixels wide at most (their texture is 8 texels); exp in percent with two decimals, the name, the
// level (a level of two digits is drawn digit by digit) and "MP now / max" in the large digits.
INT NCPlayerStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPlayerStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (LeftBackTex)
        Device->DrawTexture(12, 0, 16, 76, 0, 0, 16, 76, LeftBackTex, 0xFF, 1);
    if (BackTex)
        Device->DrawTexture(28, 0, (INT)(Width - 12.0f - 16.0f - 4.0f), 76, 0, 0, 16, 76, BackTex, 0xFF, 1);
    if (RightBackTex)
        Device->DrawTexture((INT)(Width - 4.0f), 0, 4, 76, 0, 0, 4, 76, RightBackTex, 0xFF, 1);
    if (LevelBackTex)
        Device->DrawTexture(16, 6, 22, 20, 0, 0, 22, 20, LevelBackTex, 0xFF, 1);
    if (SizeControl2Tex)
        Device->DrawTexture(ExtraWidth + 167, 4, 9, 13, 0, 0, 9, 13, SizeControl2Tex, 0xFF, 0);
    Player = GConsole->GetMyUser();
    if (Player)
    {
        // the HP bar control takes the HP and the maximum (0x100FF4B0, ret 8)
        ((NCStatusBarCtrl*)HPBar)->SetValues(Player->HP, Player->MaxHP);

        INT BarW = ExtraWidth + 150;
        INT MPW = 0;
        INT CPW = 0;
        INT EXPW = 0;
        INT MaxCP = nw::Field<INT>(Player, 0x204);   // C3 (names ours): the CP and its maximum
        INT CP = nw::Field<INT>(Player, 0x208);
        if (Player->MaxMP)
            MPW = (INT)((DOUBLE)Player->MP * BarW / Player->MaxMP);
        if (MaxCP)
            CPW = (INT)((DOUBLE)CP * BarW / MaxCP);
        INT NextExp = GExpTable[Player->Level + 1];
        if (NextExp)
            EXPW = (INT)((DOUBLE)(Player->Exp - GExpTable[Player->Level]) * BarW / (NextExp - GExpTable[Player->Level]));

        Device->DrawTexture(16, 28, BarW, 4, 0, 0, 8, 4, CPBackTex, 0xFF, 1);
        Device->DrawTexture(16, 47, BarW, 12, 0, 0, 8, 12, PlayerMPBackTex, 0xFF, 1);
        Device->DrawTexture(16, 61, BarW, 12, 0, 0, 8, 12, PlayerEXPBackTex, 0xFF, 1);
        if (CPW > 0 && CPW <= BarW)
            Device->DrawTexture(16, 28, CPW, 4, 0, 0, 8, 4, CPTex, 0xFF, 1);
        else if (CPW > BarW)
            Device->DrawTexture(16, 28, BarW, 4, 0, 0, 8, 4, CPTex, 0xFF, 1);
        if (MPW > 0 && MPW <= BarW)
            Device->DrawTexture(16, 47, MPW, 12, 0, 0, 8, 12, MPTex, 0xFF, 1);
        else if (MPW > BarW)
            Device->DrawTexture(16, 47, BarW, 12, 0, 0, 8, 12, MPTex, 0xFF, 1);
        if (EXPW > 0 && EXPW <= BarW)
            Device->DrawTexture(16, 61, EXPW, 12, 0, 0, 8, 12, EXPTex, 0xFF, 1);
        else if (EXPW > BarW)
            Device->DrawTexture(16, 61, BarW, 12, 0, 0, 8, 12, EXPTex, 0xFF, 1);

        TCHAR Buf[64];
        SIZE Size;
        INT LevelExp = GExpTable[Player->Level];
        _swprintf(Buf, TEXT("%.2f%s"),
                  (DOUBLE)(Player->Exp - LevelExp) * 100.0 / (GExpTable[Player->Level + 1] - LevelExp), TEXT("%"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, BarW / 2 - Size.cx / 2 + 12, 63, 0xFFDCDCDC, Buf);

        appSprintf(Buf, TEXT("%s"), Player->GetName());
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawText(40, 11, 0xFFDCDCDC, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);

        if (Player->Level <= 9)
        {
            appSprintf(Buf, TEXT("%d"), Player->Level);
            NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
            Device->DrawSpecialDigit(1, 27 - Size.cx / 2, 12, 0xFFB09B79, Buf);
        }
        else
        {
            appSprintf(Buf, TEXT("%d"), Player->Level);
            TCHAR Digit[2];
            Digit[0] = Buf[0];
            Digit[1] = 0;
            Device->DrawSpecialDigit(1, 20, 12, 0xFFB09B79, Digit);
            Digit[0] = Buf[1];
            Digit[1] = 0;
            Device->DrawSpecialDigit(1, 27, 12, 0xFFB09B79, Digit);
        }

        Device->DrawSpecialDigit(1, 25, 49, 0xFFDCDCDC, TEXT("MP"));
        appSprintf(Buf, TEXT("%s"), TEXT("/"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, BarW / 2 - Size.cx / 2 + 16, 49, 0xFFDCDCDC, Buf);
        appSprintf(Buf, TEXT("%d"), Player->MP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, BarW / 2 - Size.cx + 8, 49, 0xFFDCDCDC, Buf);
        appSprintf(Buf, TEXT("%d"), Player->MaxMP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, BarW / 2 + 20, 49, 0xFFDCDCDC, Buf);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10100360, NCPlayerStatusWnd, OnPaint)

// 0x100faad0 (vslot 62) - targets the player
INT NCPlayerStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    GConsole->TargetMyself();
    return 1;
}
NW_HOOK_METHOD(0x100faad0, NCPlayerStatusWnd, OnLButtonDown)

// 0x100fc6e0 (vslot 69) - the right edge was dragged by Delta: the window grows or shrinks within 0 .. 202 pixels,
// the edge and the HP bar follow
INT NCPlayerStatusWnd::OnResize(INT Delta)
{
    NRectF Rect = {0, 0, 0.0f, 0.0f};
    GetLocalRect(&Rect);
    if (Delta > 0)
    {
        if (ExtraWidth >= MaxExtraWidth)
            return 1;
        INT NewExtra = ExtraWidth + Delta;
        ExtraWidth = NewExtra;
        if (NewExtra > MaxExtraWidth)
        {
            Delta += MaxExtraWidth - NewExtra;
            ExtraWidth = MaxExtraWidth;
        }
    }
    else
    {
        if (Delta >= 0)
            return 1;
        if (ExtraWidth <= 0)
            return 1;
        INT NewExtra = ExtraWidth + Delta;
        ExtraWidth = NewExtra;
        if (NewExtra < 0)
        {
            Delta -= NewExtra;
            ExtraWidth = 0;
        }
    }
    SetWindowSize((FLOAT)Delta + Rect.W, Rect.H);
    NRectF EdgeRect = {0, 0, 0.0f, 0.0f};
    ResizeFrame->GetLocalRect(&EdgeRect);
    ResizeFrame->SetPosition((INT)(Width - EdgeRect.W), 0, (DWORD)(INT)EdgeRect.W, (DWORD)(INT)EdgeRect.H);
    NRectF BarRect = {0, 0, 0.0f, 0.0f};
    HPBar->GetLocalRect(&BarRect);
    HPBar->SetWindowSize(BarRect.W + (FLOAT)Delta, 12.0f);
    return 1;
}
NW_HOOK_METHOD(0x100fc6e0, NCPlayerStatusWnd, OnResize)

// 0x100faac0 - the console restores the saved width with it
INT NCPlayerStatusWnd::Resize(INT Delta)
{
    return OnResize(Delta);
}
NW_HOOK_METHOD(0x100faac0, NCPlayerStatusWnd, Resize)

NW_CHECK_VTABLE(NCPlayerStatusWnd, 0x1019fa30, 70)

/*-----------------------------------------------------------------------------
    NCTargetStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10102cb0 - C3: the base's, the tool tip of the buff icon under the mouse (a member: NToolTipInfo's constructor),
// then the level difference, the buff icon under the mouse and the expanded flag cleared
NCTargetStatusWnd::NCTargetStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    LevelDiff = 0;
    bExpanded = 0;
    HoverBuff = 0;
}
NW_HOOK_CTOR_ARGS(0x10102cb0, NCTargetStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x10102d50 - the buff tool tip goes, then NCStatusWnd
NW_HOOK_DTOR(0x10102d50, NCTargetStatusWnd)


// 0x100fa830 (vslot 25) - C2 has no IsEnabled check of its own
INT NCTargetStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCStatusWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_TARGETSTATUS_CLOSE)
            return OnClose() != 0;
        if ((INT)wParam == NID_TARGETSTATUS_TOGGLE)
            return OnToggle() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100fa830, NCTargetStatusWnd, WindowProc)

// 0x100ff660 (vslot 37) - C3: the bars are 150 wide, two windows' backgrounds, a close button, the title bar (id
// 0xF8) and the toggle button of the buffs
INT NCTargetStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    BarWidth = 150;
    BackTex3[0] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_Back1"));
    BackTex3[1] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_Back2"));
    BackTex3[2] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_Back3"));
    BackTex3[3] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow2_Back1"));
    BackTex3[4] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow2_Back2"));
    BackTex3[5] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow2_Back3"));
    FrameTopTex = FindTexture(TEXT("L2UI_CH3.FRAMECTRL.SmallBar1"));
    FrameCenterTex = FindTexture(TEXT("L2UI_CH3.FRAMECTRL.SmallBar2"));
    FrameBottomTex = FindTexture(TEXT("L2UI_CH3.FRAMECTRL.SmallBar3"));
    UpBtnTex = FindTexture(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton_Down"));
    UpBtnClickTex = FindTexture(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton_Down"));
    DownBtnTex = FindTexture(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"));
    DownBtnClickTex = FindTexture(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"));
    HPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HpBar"));
    MPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MpBar"));
    HPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HpBar_back"));
    MPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MpBar_back"));

    CloseButton = new NCButton(TEXT("L2UI_CH3.FrameCtrl.FrameCloseBtn"), TEXT("L2UI_CH3.FrameCtrl.FrameCloseOnBtn"), 0);
    CloseButton->ID = NID_TARGETSTATUS_CLOSE;
    CloseButton->SetPosition(153, 6, 15, 15);
    CreateChild(CloseButton, NWS_VISIBLE);

    FrameCtrl = NewFrameCtrl(0, 0, 0);   // not decorated: the stock pushes edi (0) three times; (1, 0, 0) made the
                                         // frame a close button of its own, a second X beside the toggle button
    FrameID = NID_TARGETSTATUS_FRAME;
    FrameCtrl->ID = NID_TARGETSTATUS_FRAME;
    FrameCtrl->SetPosition(0, 19, 12, (DWORD)(INT)(Height - 19.0f));
    CreateChild(FrameCtrl, NWS_VISIBLE);

    ToggleButton = new NCButton(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"),
                                TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"), 0);
    ToggleButton->ID = NID_TARGETSTATUS_TOGGLE;
    ToggleButton->SetPosition(1, 6, 11, 13);
    CreateChild(ToggleButton, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100ff660, NCTargetStatusWnd, OnCreate)

// 0x100fc560 (vslot 38) - the close button and the toggle button go, then the base's
INT NCTargetStatusWnd::OnDestroy()
{
    guard(NCTargetStatusWnd::OnDestroy);
    if (CloseButton)
    {
        RemoveChildWindow(CloseButton->Entry);
        delete CloseButton;
        CloseButton = NULL;
    }
    if (ToggleButton)
    {
        RemoveChildWindow(ToggleButton->Entry);
        delete ToggleButton;
        ToggleButton = NULL;
    }
    NCStatusWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100fc560, NCTargetStatusWnd, OnDestroy)

// the color of the name by the player's level minus the target's (the stock tests ascend from -9): red when the target
// is much stronger, blue when much weaker (spelled out at its one use)
static FORCEINLINE DWORD LevelDiffColor(INT Diff)
{
    if (Diff <= -9)
        return 0xFFFF0000;
    if (Diff <= -6)
        return 0xFFFF9191;
    if (Diff <= -3)
        return 0xFFFAFE91;
    if (Diff <= 2)
        return 0xFFDCDCDC;
    if (Diff <= 5)
        return 0xFFA2FFAB;
    if (Diff <= 8)
        return 0xFFA2A8FC;
    return 0xFF0000FF;
}

// the NPC table's record of an NPC (0x1012b460 inlined: the hash chain of the key); the C3 record of an NPC
// ends its first part with the list of the skills the target window shows as icons (names ours): a TArray<INT> at
// +0x4C (data) / +0x50 (count). The stock code does not test the result (an NPC the window shows is in the table)
static FORCEINLINE BYTE* FindNpcRecord(INT NpcID)
{
    return (BYTE*)GL2GameData.NpcTable->FindRef(NpcID);
}

// 0x100ff9a0 (vslot 56) - C3: the window's back (three pieces, 46 high; 76 when expanded), the frame, then by what
// the target is: nothing (the window hides itself), a door or other static object (its name), a creature. A monster,
// the player, its own pet or summon and a siege NPC show the name colored by the level difference and the HP bar
// (the MP bar too, except for the monsters); anything else its name alone. A player, expanded, has the clan and the
// alliance (crests and names); an NPC, expanded, the icons of its skills (8 to a row) from the NPC table. Without a
// target the window hides itself, leaving the clip and the origin set, as in the stock code.
INT NCTargetStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCTargetStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (!bExpanded)
    {
        if (BackTex3[0])
            Device->DrawTexture(12, 0, 16, (INT)Height, 0, 0, 16, 46, BackTex3[0], 0xFF, 1);
        if (BackTex3[1])
            Device->DrawTexture(28, 0, (INT)(Width - 32.0f - 12.0f), (INT)Height, 0, 0, 16, 46, BackTex3[1], 0xFF, 1);
        if (BackTex3[2])
            Device->DrawTexture((INT)(Width - 16.0f), 0, 16, (INT)Height, 0, 0, 16, 46, BackTex3[2], 0xFF, 1);
    }
    else
    {
        if (BackTex3[3])
            Device->DrawTexture(12, 0, 16, (INT)Height, 0, 0, 16, 76, BackTex3[3], 0xFF, 1);
        if (BackTex3[4])
            Device->DrawTexture(28, 0, (INT)(Width - 32.0f - 12.0f), (INT)Height, 0, 0, 16, 76, BackTex3[4], 0xFF, 1);
        if (BackTex3[5])
            Device->DrawTexture((INT)(Width - 16.0f), 0, 16, (INT)Height, 0, 0, 16, 76, BackTex3[5], 0xFF, 1);
    }
    if (FrameTopTex)
        Device->DrawTexture(0, 0, 12, 8, 0, 0, 12, 8, FrameTopTex, 0xFF, 1);
    if (FrameCenterTex)
        Device->DrawTexture(0, 8, 12, (INT)(Height - 16.0f), 0, 0, 12, 8, FrameCenterTex, 0xFF, 1);
    if (FrameBottomTex)
        Device->DrawTexture(0, (INT)(Height - 8.0f), 12, 8, 0, 0, 12, 8, FrameBottomTex, 0xFF, 1);

    User* Target = GConsole->GetSelectedActor();
    User* Me = GConsole->GetMyUser();
    AActor* StaticActor = GConsole->GetSelectedStaticActor();
    SIZE Size;
    if (!Target)
    {
        if (!StaticActor)
        {
            HideWindow();
            return 1;
        }
        // a door or other static object: its name only
        FServerStaticObject* Info = GConsole->FindStaticObjectInfo(nw::Field<INT>(StaticActor, 0x54));
        if (Info)
        {
            TCHAR* ObjectName = GL2GameData.GetStaticObjectName(Info->StaticObjectID);
            if (ObjectName)
            {
                NCGDevice::GetTextExtent(&Size, ObjectName, 0, 0, 0, 0, 0);
                Device->DrawText((INT)((Width - 30.0f) * 0.5f - Size.cx / 2 + 12.0f), 9, 0xFFDCDCDC, ObjectName, 0, 0, 0,
                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            }
        }
    }
    else
    {
        // a monster, the player, its own pet or summon, a siege NPC: the bars
        UBOOL bShowBars = 0;
        if (Target->Unknown8 && !Target->Unknown0C && Target->Unknown10)
            bShowBars = 1;
        else if (Me && Target == Me)
            bShowBars = 1;
        else if (Target->Unknown8 && Target->Unknown0C && Target->ObjectID == GConsole->GetPetInfo().ObjectID)
            bShowBars = 1;
        else if (Target->Unknown8 && ((NConsoleLeftFns*)GConsole)->NpcShowsBars(Target->NpcID))
            bShowBars = 1;   // 0x10073b90 (C3: the NPC record's +0xCC is set; NConsoleWnd::IsSiegeNpc's place)

        if (bShowBars)
        {
            INT HPWidth = 0;
            INT MPWidth = 0;
            if (Target->MaxHP)
                HPWidth = (INT)((DOUBLE)BarWidth * Target->HP / Target->MaxHP);
            if (Target->MaxMP)
                MPWidth = (INT)((DOUBLE)BarWidth * Target->MP / Target->MaxMP);
            if (HPWidth >= BarWidth)
                HPWidth = BarWidth;
            if (MPWidth >= BarWidth)
                MPWidth = BarWidth;
            NCGDevice::GetTextExtent(&Size, Target->GetName(), 0, 0, 0, 0, 0);
            DWORD Color = LevelDiffColor(LevelDiff);
            Device->DrawText((INT)((Width - 30.0f) * 0.5f - Size.cx / 2 + 12.0f), 9, Color, Target->GetName(), 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            if (HPWidth >= 0)
            {
                Device->DrawTexture(16, 26, BarWidth, 6, 0, 0, 8, 6, HPBackTex, 0xFF, 1);
                Device->DrawTexture(16, 26, HPWidth, 6, 0, 0, 8, 6, HPTex, 0xFF, 1);
            }
            // a monster has no MP bar
            if ((!Target->Unknown8 || Target->Unknown0C || !Target->Unknown10) && MPWidth >= 0)
            {
                Device->DrawTexture(16, 33, BarWidth, 6, 0, 0, 8, 6, MPBackTex, 0xFF, 1);
                Device->DrawTexture(16, 33, MPWidth, 6, 0, 0, 8, 6, MPTex, 0xFF, 1);
            }
        }
        else
        {
            NCGDevice::GetTextExtent(&Size, Target->GetName(), 0, 0, 0, 0, 0);
            Device->DrawText((INT)((Width - 30.0f) * 0.5f - Size.cx / 2 + 12.0f), 5, 0xFFDCDCDC, Target->GetName(), 0,
                             0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }

        if (Target->Unknown8)
        {
            // an NPC: the icons of its skills when expanded
            if (bExpanded)
            {
                BYTE* Npc = FindNpcRecord(Target->NpcID);
                INT Count = Npc ? nw::Field<INT>(Npc, 0x50) : 0;   // (the stock does not test it)
                for (INT i = 0; i < Count; i++)
                {
                    const TCHAR* IconName = GConsole->GetSkillIconName(nw::Field<INT*>(Npc, 0x4C)[i], 1);
                    if (IconName)
                    {
                        UTexture* Icon = FindTexture(IconName);
                        if (Icon)
                            Device->DrawStretchedTex((i % 8 + 1) * 18, i / 8 * 18 + 40, 15, 15, 0, 0, 32, 32, Icon, 0xFF);
                    }
                }
            }
        }
        else if (bExpanded)
        {
            // a player: the clan and the alliance
            Device->DrawText(20, 43, 0xFFA3A3A3, *SysString(604), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            Device->DrawText(20, 59, 0xFFA3A3A3, *SysString(605), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            UBOOL bCrest = 0;
            PledgeInfo* Pledge = GConsole->GetPledgeInfo(Target->PledgeID);
            if (!Pledge)
                Device->DrawText(45, 43, 0xFF808080, *SysString(431), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);   // no clan
            else
            {
                // C3 puts the crest textures of the record at +0xDC and +0xE0 (the header's PledgeInfo is C2's)
                if (nw::Field<UTexture*>(Pledge, 0xDC))
                {
                    Device->DrawPledgeCrest(45, 43, Target->PledgeID);
                    bCrest = 1;
                }
                const TCHAR* PledgeName = Pledge->Name;
                if (PledgeName && *PledgeName)
                    Device->DrawText(bCrest ? 63 : 45, 43, 0xFFB09B79, PledgeName, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                                     0);
                UBOOL bAllyCrest = 0;
                if (nw::Field<UTexture*>(Pledge, 0xE0))
                {
                    Device->DrawAllyCrest(53, 59, Target->PledgeID);
                    bAllyCrest = 1;
                }
                const TCHAR* AllyName = Pledge->AllyName;
                if (AllyName && *AllyName)
                {
                    Device->DrawText(bAllyCrest ? 63 : 45, 59, 0xFFB09B79, AllyName, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                                     0, 0);
                    goto Done;
                }
            }
            Device->DrawText(45, 59, 0xFF808080, *SysString(591), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);   // no alliance
        }
    }
Done:
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100ff9a0, NCTargetStatusWnd, OnPaint)

// 0x100fa8b0 (vslot 62) - targets again; with Ctrl, attacks
INT NCTargetStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        FVector Origin = Me->Pawn->Location;
        if (ConsoleIsCtrlDown())
            RequestAttackC2(ObjectID, Origin);
        else
            GConsole->RequestTarget(ObjectID);
    }
    return 1;
}
NW_HOOK_METHOD(0x100fa8b0, NCTargetStatusWnd, OnLButtonDown)

// 0x10100270 (vslot 61) - C3: which of the NPC's skill icons (16 x 16, 8 to a row from (18, 40), 18 apart) is under
// the mouse, as 1 + its index (0: none); only for an NPC target with the window expanded. The mouse position is
// taken relative to the window in 16 bits (a position left of or above the window wraps to a large one: no hit).
// The base's (the drag of a dragable window) is not called.
INT NCTargetStatusWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    User* Target = GConsole->GetSelectedActor();
    HoverBuff = 0;
    if (Target && Target->Unknown8 && bExpanded)
    {
        INT X = (WORD)((WORD)LOWORD(lParam) - (WORD)ScreenX);
        INT Y = (WORD)((WORD)HIWORD(lParam) - (WORD)ScreenY);
        BYTE* Npc = FindNpcRecord(Target->NpcID);
        INT Count = Npc ? nw::Field<INT>(Npc, 0x50) : 0;   // (the stock does not test it)
        for (INT i = 0; i < Count; i++)
        {
            INT IconX = (i % 8) * 18;
            INT IconY = i / 8 * 18;
            if (IconX + 18 <= X && X <= IconX + 33 && IconY + 40 <= Y && Y <= IconY + 55)
            {
                HoverBuff = i + 1;
                return 1;
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10100270, NCTargetStatusWnd, OnMouseMove)

// 0x10102100 (vslot 57) - C3: the tool tip of the NPC skill icon under the mouse: the skill's name and description
// (level 1) in the window's tool tip skin, over the icon's place (the full box)
INT NCTargetStatusWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    User* Target = GConsole->GetSelectedActor();
    if (Target && Target->Unknown8 && bExpanded)
    {
        BYTE* Npc = FindNpcRecord(Target->NpcID);
        if (Npc && HoverBuff > 0)   // (the stock does not test Npc)
        {
            const TCHAR* Desc = GConsole->GetSkillDesc(nw::Field<INT*>(Npc, 0x4C)[HoverBuff - 1], 1);
            const TCHAR* Name = GConsole->GetSkillName(nw::Field<INT*>(Npc, 0x4C)[HoverBuff - 1], 1);
            if (Name && Desc)
            {
                // sub_1002d7f0 (ret 0xC): the tip's name and description (a null one is left), Style NTT_Text, laid out
                BuffToolTip.SetNameDesc(Name, Desc, (INT)NTT_Text);
                INT Index = HoverBuff - 1;
                INT X = (Index % 8 + 1) * 18;
                INT Y = Index / 8 * 18 + 40;
                if (ToolTipSkin)
                {
                    NToolTipRect Wnd;
                    Wnd.X = ScreenX;
                    Wnd.Y = ScreenY;
                    Wnd.W = Width;
                    Wnd.H = Height;
                    ToolTipSkin->DrawFull(Device, BuffToolTip, X, Y, Wnd);
                }
            }
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10102100, NCTargetStatusWnd, OnDrawToolTip)

// 0x100fa9f0 (vslot 69) - the close button: the server drops the target
INT NCTargetStatusWnd::OnClose()
{
    ::nw::TraceCall("NCTargetStatusWnd::OnClose", this);
    L2ParamStack Params(10);
    GConsole->RequestTargetCancel(Params);
    PlayInterfaceSound(SOUND_WindowClose);
    return 1;
}
NW_HOOK_METHOD(0x100fa9f0, NCTargetStatusWnd, OnClose)

// 0x100fa950 (vslot 70) - doubles or halves the height for the buff icons: 46 (collapsed) or 76 (expanded)
INT NCTargetStatusWnd::OnToggle()
{
    FLOAT H;
    if (bExpanded)
    {
        ToggleButton->SetNormalTex(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"));
        ToggleButton->SetPushedTex(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"));
        H = 46.0f;
    }
    else
    {
        ToggleButton->SetNormalTex(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton_Down"));
        ToggleButton->SetPushedTex(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton_Down"));
        H = 76.0f;
    }
    SetWindowSize(Width, H);
    bExpanded = !bExpanded;
    FrameCtrl->SetPosition(0, 19, 12, (DWORD)(INT)(Height - 19.0f));
    return 1;
}
NW_HOOK_METHOD(0x100fa950, NCTargetStatusWnd, OnToggle)

// 0x100fa940 - from the console with each new target
void NCTargetStatusWnd::SetLevelDiff(INT Diff)
{
    LevelDiff = Diff;
}
NW_HOOK_METHOD(0x100fa940, NCTargetStatusWnd, SetLevelDiff)

NW_CHECK_VTABLE(NCTargetStatusWnd, 0x1019f910, 71)

/*-----------------------------------------------------------------------------
    NCPartyStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10103410 - C3: -1 for the member, the two buff arrays emptied
NCPartyStatusWnd::NCPartyStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    MemberID = -1;
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
    Unknown35C = 0;
}
NW_HOOK_CTOR_ARGS(0x10103410, NCPartyStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x101034e0 - the buff icons and skill ids go, then NCStatusWnd
NW_HOOK_DTOR(0x101034e0, NCPartyStatusWnd)


// one of the small windows made by OnCreate: 11 x 11 at (4, 9), the title bar's coordinates
static FORCEINLINE NCButton* CreatePartyIcon(NCWnd* Parent, const TCHAR* TexName, INT ID, UBOOL bHide)
{
    NCButton* Icon = new NCButton(TexName, TexName, 0);
    Icon->SetPosition(4, 9, 11, 11);
    Icon->ID = ID;
    Parent->CreateChild(Icon, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    if (bHide)
        Icon->HideWindow();
    return Icon;
}

// 0x101010c0 (vslot 37) - C3: the member's frame, bars, nine class icons and the leader's icon
INT NCPartyStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    LeftBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_back1"));
    BackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_back2"));
    RightBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_back3"));
    HPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.hpbar"));
    MPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.mpbar"));
    HPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.hpbar_back"));
    MPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.mpbar_back"));
    StyleIcon[0] = new NCButton(TEXT("L2UI_CH3.PartyWnd.party_styleicon1"), TEXT("L2UI_CH3.PartyWnd.party_styleicon1"), 0);
    StyleIcon[1] = new NCButton(TEXT("L2UI_CH3.PartyWnd.party_styleicon2"), TEXT("L2UI_CH3.PartyWnd.party_styleicon2"), 0);
    StyleIcon[2] = new NCButton(TEXT("L2UI_CH3.PartyWnd.party_styleicon3"), TEXT("L2UI_CH3.PartyWnd.party_styleicon3"), 0);
    StyleIcon[3] = new NCButton(TEXT("L2UI_CH3.PartyWnd.party_styleicon4"), TEXT("L2UI_CH3.PartyWnd.party_styleicon4"), 0);
    StyleIcon[4] = new NCButton(TEXT("L2UI_CH3.PartyWnd.party_styleicon5"), TEXT("L2UI_CH3.PartyWnd.party_styleicon5"), 0);
    StyleIcon[5] = new NCButton(TEXT("L2UI_CH3.PartyWnd.party_styleicon6"), TEXT("L2UI_CH3.PartyWnd.party_styleicon6"), 0);
    StyleIcon[6] = new NCButton(TEXT("L2UI_CH3.PartyWnd.party_styleicon7"), TEXT("L2UI_CH3.PartyWnd.party_styleicon7"), 0);
    StyleIcon[7] = new NCButton(TEXT("L2UI_CH3.PartyWnd.party_styleicon1_1"), TEXT("L2UI_CH3.PartyWnd.party_styleicon1_1"), 0);
    StyleIcon[8] = new NCButton(TEXT("L2UI_CH3.PartyWnd.party_styleicon1_2"), TEXT("L2UI_CH3.PartyWnd.party_styleicon1_2"), 0);
    DebuffFrameTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.debuff"));
    for (INT i = 0; i < 9; i++)
    {
        StyleIcon[i]->SetPosition(4, 9, 11, 11);
        StyleIcon[i]->ID = i + (INT)0xFF0001F2;
        CreateChild(StyleIcon[i], NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
        StyleIcon[i]->HideWindow();
    }
    LeaderIcon = new NCButton(TEXT("L2UI_CH3.PartyWnd.party_leadericon"), TEXT("L2UI_CH3.PartyWnd.party_leadericon"), 0);
    LeaderIcon->SetPosition(4, 9, 11, 11);
    LeaderIcon->ID = (INT)0xFF000FF2;
    CreateChild(LeaderIcon, NWS_VISIBLE | 0x20);
    LeaderIcon->HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x101010c0, NCPartyStatusWnd, OnCreate)

// 0x100fd0c0 (vslot 38) - the nine class icons, then the leader's
INT NCPartyStatusWnd::OnDestroy()
{
    for (INT i = 0; i < 9; i++)
    {
        if (StyleIcon[i])
        {
            RemoveChildWindow(StyleIcon[i]->Entry);
            delete StyleIcon[i];
            StyleIcon[i] = NULL;
        }
    }
    if (LeaderIcon)
    {
        RemoveChildWindow(LeaderIcon->Entry);
        delete LeaderIcon;
        LeaderIcon = NULL;
    }
    NCStatusWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100fd0c0, NCPartyStatusWnd, OnDestroy)

// the console's tests of a skill id (its skill data record, level 1): 0x1005f6c0 (name ours) says the
// record has its +0x48 set, which sends the icon to the second pass of the paint (framed). Its body is
// NConsoleWndSkillExt::IsDebuffSkill (NCBroadcastObserverBuffWnd.cpp, a stand-in no header declares): the hook build
// calls the stock entry, the standalone runs the same test
static FORCEINLINE UBOOL IsFramedBuff(INT SkillID)
{
#if NW_HOOK_MODE
    return ::nw::StockCallAt<UBOOL>(0x1005f6c0, GConsole, SkillID, 1);
#else
    FL2MagicSkillData* Data = GL2GameData.GetMSData(SkillID, 1);
    if (Data && ::nw::Field<INT>(Data, 0x48))
        return 1;
    return 0;
#endif
}

// 0x101014c0 (vslot 56) - C3: the name, the two bars, then the buffs in two rows of ten at the right (the
// framed ones last, over the others), when the party window's button has them switched on
INT NCPartyStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPartyStatusWnd::OnPaint);
    INT HPWidth = 0, MPWidth = 0;
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, 16, 46, 0, 0, 16, 46, LeftBackTex, 0xFF, 1);
    Device->DrawStretchedTex(16, 0, (INT)(Width - 16.0f - 16.0f), (INT)Height, 0, 0, 16, 46, BackTex, 0xFF);
    Device->DrawTexture((INT)(Width - 16.0f), 0, 16, 46, 0, 0, 16, 46, RightBackTex, 0xFF, 1);
    if (Name && Name[0])
    {
        if (MaxHP != 0)
        {
            if (HP >= MaxHP)
                HP = MaxHP;
            HPWidth = HP * 150 / MaxHP;
        }
        if (MaxMP != 0)
        {
            if (MP >= MaxMP)
                MP = MaxMP;
            MPWidth = MP * 150 / MaxMP;
        }
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Name, 0, 0, 0, 0, 0);
        Device->DrawText((INT)(Width * 0.5f - Size.cx / 2), 9, 0xFFDCDCDC, Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                         0, 0, 0);
        Device->DrawTexture(4, 26, 150, 6, 0, 0, 8, 6, HPBackTex, 0xFF, 1);
        if (HPWidth > 0)
            Device->DrawTexture(4, 26, HPWidth, 5, 0, 0, 8, 6, HPTex, 0xFF, 1);
        Device->DrawTexture(4, 33, 150, 6, 0, 0, 8, 6, MPBackTex, 0xFF, 1);
        if (MPWidth > 0)
            Device->DrawTexture(4, 33, MPWidth, 5, 0, 0, 8, 6, MPTex, 0xFF, 1);
        Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
        if (((NCPartyWnd*)Entry->Parent->Wnd)->bToggle)
        {
            TArray<INT> XPos;
            TArray<INT> YPos;
            INT i = 0;
            while (i < BuffIcons.Num())
            {
                if (i < 10)
                {
                    if (!IsFramedBuff(BuffSkillIDs(i)))
                        Device->DrawTexture((INT)((FLOAT)(i << 4) + Width + 1.0f), 5, 16, 16, 0, 0, 32, 32,
                                            BuffIcons(i), 0xFF, 0);
                    else
                    {
                        XPos.AddItem((INT)((FLOAT)(i << 4) + Width + 1.0f));
                        YPos.AddItem(5);
                    }
                }
                else if (i < 20)
                {
                    if (!IsFramedBuff(BuffSkillIDs(i)))
                        Device->DrawTexture((INT)((FLOAT)((i - 10) << 4) + Width + 1.0f), 21, 16, 16, 0, 0, 32,
                                            32, BuffIcons(i), 0xFF, 0);
                    else
                    {
                        XPos.AddItem((INT)((FLOAT)((i - 10) << 4) + Width + 1.0f));
                        YPos.AddItem(21);
                    }
                }
                i++;
            }
            INT k = 0;
            for (i = 0; i < BuffIcons.Num(); i++)
            {
                if (IsFramedBuff(BuffSkillIDs(i)))
                {
                    Device->DrawTexture(XPos(k), YPos(k), 16, 16, 0, 0, 32, 32, BuffIcons(i), 0xFF, 0);
                    Device->DrawTexture(XPos(k), YPos(k), 16, 16, 0, 0, 26, 26, DebuffFrameTex, 0xFF, 0);
                    k++;
                }
            }
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x101014c0, NCPartyStatusWnd, OnPaint)

// 0x100fae70 (vslot 62) - targets the member; with Ctrl, attacks it
INT NCPartyStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        FVector Origin = Me->Pawn->Location;
        if (ConsoleIsCtrlDown())
            RequestAttackC2(MemberID, Origin);
        else
            GConsole->RequestTarget(MemberID);
    }
    return 1;
}
NW_HOOK_METHOD(0x100fae70, NCPartyStatusWnd, OnLButtonDown)

// 0x100faf00 (vslot 65) - targets what the member targets (signed coordinates here)
INT NCPartyStatusWnd::OnRButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (PtInRect((SWORD)LOWORD(lParam), (SWORD)HIWORD(lParam), Rect))
    {
        User* Me = GConsole->GetMyUser();
        if (Me)
            GConsole->AssistMember(MemberID, Me->Pawn->Location);
    }
    return 1;
}
NW_HOOK_METHOD(0x100faf00, NCPartyStatusWnd, OnRButtonUp)

// 0x100fd140 (name ours) - a member joins: the stock code stores the party's leader and loot rule too
void NCPartyStatusWnd::SetInfo(INT InLeaderID, INT InLootRule, INT InMemberID, const TCHAR* InName, INT InHP,
                               INT InMaxHP, INT InMP, INT InMaxMP, INT InClassID)
{
    if (InName)
    {
        MemberID = InMemberID;
        wcscpy(Name, InName);
        HP = InHP;
        MaxHP = InMaxHP;
        MP = InMP;
        MaxMP = InMaxMP;
        ClassID = InClassID;
        LeaderID = InLeaderID;
        LootRule = InLootRule;
        UpdateStyle();
    }
}
NW_HOOK_METHOD(0x100fd140, NCPartyStatusWnd, SetInfo)

// 0x100fd1c0 (name ours) - the server's update of a member
void NCPartyStatusWnd::UpdateInfo(INT InMemberID, const TCHAR* InName, INT InHP, INT InMaxHP, INT InMP, INT InMaxMP,
                                  INT InClassID)
{
    if (InName)
    {
        MemberID = InMemberID;
        wcscpy(Name, InName);
        MaxHP = InMaxHP;
        HP = InHP;
        ClassID = InClassID;
        MP = InMP;
        MaxMP = InMaxMP;
        UpdateStyle();
    }
}
NW_HOOK_METHOD(0x100fd1c0, NCPartyStatusWnd, UpdateInfo)

// 0x100fad60 (ret 4, name ours) - the name of the group of a class: a sysstring per group (the class id goes through the
// console's group table, 0x10060400)
const TCHAR* NCPartyStatusWnd::GetStyleName(INT ClassID)
{
    switch (((NConsoleLeftFns*)GConsole)->GetClassGroup(ClassID))
    {
    case 0: return *GL2GameData.SysString[874];
    case 1: return *GL2GameData.SysString[875];
    case 2: return *GL2GameData.SysString[876];
    case 3: return *GL2GameData.SysString[877];
    case 4: return *GL2GameData.SysString[878];
    case 5: return *GL2GameData.SysString[879];
    case 6: return *GL2GameData.SysString[880];
    case 7: return *GL2GameData.SysString[175];
    case 8: return *GL2GameData.SysString[176];
    }
    return NULL;
}
NW_HOOK_METHOD(0x100fad60, NCPartyStatusWnd, GetStyleName)

// 0x100fafa0 (name ours) - the icon of the member's class group alone is shown (tool tip "<group> - <class>"),
// the leader's icon sits right of the name while the member leads (tool tip: the loot rule)
void NCPartyStatusWnd::UpdateStyle()
{
    if (Name[0] == 0)
        return;
    TCHAR Tip[512];
    TCHAR Rule[512];
    for (INT i = 0; i < 9; i++)
        StyleIcon[i]->HideWindow();
    INT Group = ((NConsoleLeftFns*)GConsole)->GetClassGroup(ClassID);
    StyleIcon[Group]->ShowWindow();
    appSprintf(Tip, TEXT("%s - %s"), GetStyleName(ClassID), GConsole->GetClassNameString(ClassID));
    StyleIcon[Group]->SetToolTipText(Tip, 1);   // 0x10001410
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, Name, 0, 0, 0, 0, 0);
    INT X = (INT)(Width * 0.5f - Size.cx / 2 - 11.0f);
    if (MemberID == LeaderID)
    {
        LeaderIcon->ShowWindow();
        LeaderIcon->SetPosition(X, 9, 0, 0);
    }
    else
        LeaderIcon->HideWindow();
    Rule[0] = 0;
    switch (LootRule)
    {
    case 0:
        appSprintf(Rule, TEXT("%s : %s"), *GL2GameData.SysString[489], *GL2GameData.SysString[487]);
        break;
    case 1:
        appSprintf(Rule, TEXT("%s : %s"), *GL2GameData.SysString[489], *GL2GameData.SysString[488]);
        break;
    case 2:
        appSprintf(Rule, TEXT("%s : %s"), *GL2GameData.SysString[489], *GL2GameData.SysString[798]);
        break;
    case 3:
        appSprintf(Rule, TEXT("%s : %s"), *GL2GameData.SysString[489], *GL2GameData.SysString[799]);
        break;
    case 4:
        appSprintf(Rule, TEXT("%s : %s"), *GL2GameData.SysString[489], *GL2GameData.SysString[800]);
        break;
    }
    LeaderIcon->SetToolTipText(Rule, 1);   // 0x10001410
}
NW_HOOK_METHOD(0x100fafa0, NCPartyStatusWnd, UpdateStyle)

// 0x101019d0 (vslot 68) - the base's clearing, then the buffs
void NCPartyStatusWnd::Clear()
{
    NCStatusWnd::Clear();
    ClearBuffs();
}
NW_HOOK_METHOD(0x101019d0, NCPartyStatusWnd, Clear)

// 0x10100ca0 (name ours) - out of line like the stock code
__declspec(noinline) void NCPartyStatusWnd::ClearBuffs()
{
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
}
NW_HOOK_METHOD(0x10100ca0, NCPartyStatusWnd, ClearBuffs)

// 0x10100ce0 (name ours) - a buff of the member: its skill id and the icon texture of its skill
void NCPartyStatusWnd::AddBuff(INT SkillID)
{
    BuffSkillIDs.AddItem(SkillID);
    BuffIcons.AddItem(FindTexture(GConsole->GetSkillIconName(SkillID, 1)));
}
NW_HOOK_METHOD(0x10100ce0, NCPartyStatusWnd, AddBuff)

NW_CHECK_VTABLE(NCStatusWnd, 0x1019f5c8, 69)
NW_CHECK_VTABLE(NCPartyStatusWnd, 0x101a0158, 69)

/*-----------------------------------------------------------------------------
    NCPetStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10103170 - C3: the two buff arrays emptied, the record's id -1, its name and two numbers cleared
NCPetStatusWnd::NCPetStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    Pet.Name[0] = 0;
    Pet.UnknownB0 = 0;
    Pet.ObjectID = -1;
    Pet.UnknownA8 = 0;
    Unknown410 = 0;
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
}
NW_HOOK_CTOR_ARGS(0x10103170, NCPetStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x10103250 - the buff icons and skill ids go, then NCStatusWnd
NW_HOOK_DTOR(0x10103250, NCPetStatusWnd)


// 0x100faaf0 (vslot 25) - C3: the button that switches the buff icons
INT NCPetStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCStatusWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_PETSTATUS_TOGGLE)
        return OnToggle() != 0;
    return 0;
}
NW_HOOK_METHOD(0x100faaf0, NCPetStatusWnd, WindowProc)

// the stock code calls NCPartyStatusWnd's ClearBuffs (0x10100ca0, hooked there): the same two arrays at +0x338 / +0x344
__declspec(noinline) void NCPetStatusWnd::ClearBuffs()
{
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
}

// 0x100fac70 (vslot 69; name ours) - the buff icons on / off
INT NCPetStatusWnd::OnToggle()
{
    bBuffs = !bBuffs;
    return 1;
}
NW_HOOK_METHOD(0x100fac70, NCPetStatusWnd, OnToggle)

// 0x10100a60 (vslot 37) - C3: the fatigue bar's textures, the title bar (id 0xFFA0B1) and the buffs button
INT NCPetStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back2"));
    LeftBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back1"));
    RightBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back3"));
    HPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HPBAR"));
    MPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MPBAR"));
    HPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HPBAR_BACK"));
    MPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MPBAR_BACK"));
    FedTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.FATIGUEBAR"));
    FedBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.FATIGUEBAR_BACK"));
    PetFrameTopTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar1"));
    PetFrameCenterTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar2"));
    PetFrameBottomTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar3"));

    FrameCtrl = NewFrameCtrl(0, 0, 0);
    FrameCtrl->SetPosition(0, 17, 12, (DWORD)(INT)Height);
    FrameCtrl->ID = NID_PETSTATUS_FRAME;
    CreateChild(FrameCtrl, NWS_VISIBLE);

    ToggleButton = new NCButton(TEXT("L2UI_CH3.Etc.pet_button"), TEXT("L2UI_CH3.Etc.pet_button_down"), 0);
    ToggleButton->SetPosition(1, 6, 11, 13);
    ToggleButton->ID = NID_PETSTATUS_TOGGLE;
    CreateChild(ToggleButton, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    ToggleButton->SetToolTip(0x371, 1);
    return 1;
}
NW_HOOK_METHOD(0x10100a60, NCPetStatusWnd, OnCreate)

// 0x10102280 (vslot 38) - the buffs are forgotten, the button goes, then the base's (the guard's name is
// NCSummonedStatusWnd's: the stock code was copied from it)
INT NCPetStatusWnd::OnDestroy()
{
    guard(NCSummonedStatusWnd::OnDestroy);
    ClearBuffs();
    if (ToggleButton)
    {
        RemoveChildWindow(ToggleButton->Entry);
        delete ToggleButton;
        ToggleButton = NULL;
    }
    NCStatusWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10102280, NCPetStatusWnd, OnDestroy)

// 0x100fac30 (vslot 44) - hidden again unless the player has a pet (C2 also hides it while
// the record has no object id)
void NCPetStatusWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    PetInfo Info = GConsole->GetPetInfo();
    if (Info.ObjectID == -1 || Info.Type != 2)
        HideWindow();
}
NW_HOOK_METHOD(0x100fac30, NCPetStatusWnd, ShowWindow)

// 0x100fc840 (vslot 56) - C3: the frame on the left, the window's three-part background, the name, the HP / MP /
// fatigue bars (150 pixels, the record read again) and the buff icons in two rows of ten
INT NCPetStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPetStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, 12, 8, 0, 0, 12, 8, PetFrameTopTex, 0xFF, 1);
    Device->DrawTexture(0, 8, 12, (INT)(Height - 16.0f), 0, 0, 12, 8, PetFrameCenterTex, 0xFF, 1);
    Device->DrawTexture(0, (INT)(Height - 8.0f), 12, 8, 0, 0, 12, 8, PetFrameBottomTex, 0xFF, 1);
    INT H = (INT)Height;
    Device->DrawTexture(12, 0, 16, H, 0, 0, 16, H, LeftBackTex, 0xFF, 1);
    Device->DrawStretchedTex(28, 0, (INT)(Width - 16.0f - 16.0f - 12.0f), (INT)Height, 0, 0, 16, (INT)Height,
                             BackTex, 0xFF);
    INT H2 = (INT)Height;
    Device->DrawTexture((INT)(Width - 16.0f), 0, 16, H2, 0, 0, 16, H2, RightBackTex, 0xFF, 1);
    Pet = GConsole->GetPetInfo();
    INT HPWidth = 0, MPWidth = 0, FedWidth = 0;
    if (Pet.MaxHP != 0)
        HPWidth = Pet.HP * 150 / Pet.MaxHP;
    if (Pet.MaxMP != 0)
        MPWidth = Pet.MP * 150 / Pet.MaxMP;
    if (Pet.MaxFed != 0)
        FedWidth = Pet.Fed * 150 / Pet.MaxFed;
    if (Pet.Name[0])
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Pet.Name, 0, 0, 0, 0, 0);
        Device->DrawText((INT)((Width - 12.0f) * 0.5f - Size.cx / 2 + 12.0f), 6, 0xFFDCDCDC, Pet.Name, 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->DrawTexture(16, 19, 150, 5, 0, 0, 8, 6, HPBackTex, 0xFF, 1);
    if (HPWidth > 0)
        Device->DrawTexture(16, 19, HPWidth, 6, 0, 0, 8, 6, HPTex, 0xFF, 1);
    Device->DrawTexture(16, 26, 150, 5, 0, 0, 8, 6, MPBackTex, 0xFF, 1);
    if (MPWidth > 0)
        Device->DrawTexture(16, 26, MPWidth, 6, 0, 0, 8, 6, MPTex, 0xFF, 1);
    Device->DrawTexture(16, 33, 150, 5, 0, 0, 8, 6, FedBackTex, 0xFF, 1);
    if (FedWidth > 0)
        Device->DrawTexture(16, 33, FedWidth, 6, 0, 0, 8, 6, FedTex, 0xFF, 1);
    if (bBuffs)
    {
        for (INT i = 0; i < BuffIcons.Num(); i++)
        {
            if (i < 10)
                Device->DrawStretchedTex((INT)((FLOAT)(i << 4) + Width + 1.0f), 5, 16, 16, BuffIcons(i), 0, 0xFF);
            else if (i < 20)
                Device->DrawStretchedTex((INT)((FLOAT)((i - 10) << 4) + Width + 1.0f), 21, 16, 16, BuffIcons(i), 0,
                                         0xFF);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100fc840, NCPetStatusWnd, OnPaint)

// 0x100fabd0 (vslot 62) - right of the title bar: acts on the pet (as a click on it)
INT NCPetStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if ((SWORD)LOWORD(lParam) > ScreenX + 13)
    {
        User* Me = GConsole->GetMyUser();
        if (Me)
            GConsole->RequestAction(Pet.ObjectID, Me->Pawn->Location);
    }
    return 1;
}
NW_HOOK_METHOD(0x100fabd0, NCPetStatusWnd, OnLButtonDown)

// 0x100fab50 - from the console (vslot 171); the paint reads the network handler's record instead of these
void NCPetStatusWnd::SetInfo(L2ParamStack& Params)
{
    ObjectID = (INT)Params.Top();
    TCHAR* InName = (TCHAR*)Params.Top();
    if (InName)
        wcscpy(Name, InName);
    HP = (INT)Params.Top();
    MaxHP = (INT)Params.Top();
    MP = (INT)Params.Top();
    MaxMP = (INT)Params.Top();
    Unknown41C = (INT)Params.Top();
    Unknown420 = (INT)Params.Top();
    Unknown424 = (INT)Params.Top();
}
NW_HOOK_METHOD(0x100fab50, NCPetStatusWnd, SetInfo)

NW_CHECK_VTABLE(NCPetStatusWnd, 0x1019ff28, 70)

/*-----------------------------------------------------------------------------
    NCSummonedStatusWnd.
-----------------------------------------------------------------------------*/

// 0x101032c0 - C3: the record's id -1, its name and two numbers cleared, the buff arrays emptied
NCSummonedStatusWnd::NCSummonedStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    Summon.Name[0] = 0;
    Summon.UnknownB0 = 0;
    Summon.ObjectID = -1;
    Summon.UnknownA8 = 0;
    Unknown3B4 = 0;
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
}
NW_HOOK_CTOR_ARGS(0x101032c0, NCSummonedStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x101033a0 - the buff icons and skill ids go, then NCStatusWnd
NW_HOOK_DTOR(0x101033a0, NCSummonedStatusWnd)


// 0x100fac90 (vslot 25) - C3: the button that switches the buff icons
INT NCSummonedStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCStatusWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_SUMMONSTATUS_TOGGLE)
        return OnToggle() != 0;
    return 0;
}
NW_HOOK_METHOD(0x100fac90, NCSummonedStatusWnd, WindowProc)

// 0x100facf0 (vslot 69; name ours) - the buff icons on / off
INT NCSummonedStatusWnd::OnToggle()
{
    bBuffs = !bBuffs;
    return 1;
}
NW_HOOK_METHOD(0x100facf0, NCSummonedStatusWnd, OnToggle)

// 0x10100d40 (vslot 37) - C3: the bars' textures, the title bar (id 0xFFA0B2), the time bar (a progress control of
// 10000) and the buffs button
INT NCSummonedStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back2"));
    LeftBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back1"));
    RightBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back3"));
    HPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HPBAR"));
    MPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MPBAR"));
    HPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HPBAR_BACK"));
    MPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MPBAR_BACK"));
    SummonFrameTopTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar1"));
    SummonFrameCenterTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar2"));
    SummonFrameBottomTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar3"));

    FrameCtrl = NewFrameCtrl(0, 0, 0);
    FrameCtrl->SetPosition(0, 17, 12, (DWORD)(INT)Height);
    FrameCtrl->ID = NID_SUMMONSTATUS_FRAME;
    CreateChild(FrameCtrl, NWS_VISIBLE);

    TimeBar = new NCProgressCtrl(TEXT("L2UI_CH3.SmallWnd.FATIGUEBAR_BACK"), TEXT("L2UI_CH3.SmallWnd.FATIGUEBAR"), 0);
    TimeBar->ID = NID_SUMMONSTATUS_TIME;
    TimeBar->SetPosition(16, 33, 150, 6);
    CreateChild(TimeBar, NWS_VISIBLE);
    TimeBar->SetMax(10000);
    TimeBar->Reset();

    ToggleButton = new NCButton(TEXT("L2UI_CH3.Etc.pet_button"), TEXT("L2UI_CH3.Etc.pet_button_down"), 0);
    ToggleButton->SetPosition(1, 6, 11, 13);
    ToggleButton->ID = NID_SUMMONSTATUS_TOGGLE;
    CreateChild(ToggleButton, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    ToggleButton->SetToolTip(0x371, 1);
    return 1;
}
NW_HOOK_METHOD(0x10100d40, NCSummonedStatusWnd, OnCreate)

// 0x10101020 (name ours) - the buffs forgotten; out of line like the stock code
__declspec(noinline) void NCSummonedStatusWnd::ClearBuffs()
{
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
}
NW_HOOK_METHOD(0x10101020, NCSummonedStatusWnd, ClearBuffs)

// 0x10101060 (name ours) - a buff of the summon: its skill id and the icon texture of its skill
void NCSummonedStatusWnd::AddBuff(INT SkillID)
{
    BuffSkillIDs.AddItem(SkillID);
    BuffIcons.AddItem(FindTexture(GConsole->GetSkillIconName(SkillID, 1)));
}
NW_HOOK_METHOD(0x10101060, NCSummonedStatusWnd, AddBuff)

// 0x10102330 (vslot 38) - the buffs are forgotten, the time bar and the button go, then the base's
INT NCSummonedStatusWnd::OnDestroy()
{
    guard(NCSummonedStatusWnd::OnDestroy);
    ClearBuffs();
    if (TimeBar)
    {
        RemoveChildWindow(TimeBar->Entry);
        delete TimeBar;
        TimeBar = NULL;
    }
    if (ToggleButton)
    {
        RemoveChildWindow(ToggleButton->Entry);
        delete ToggleButton;
        ToggleButton = NULL;
    }
    NCStatusWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10102330, NCSummonedStatusWnd, OnDestroy)

// 0x100fad20 (vslot 44) - hidden again unless the player has a summon (C2 also hides it while
// the record has no object id)
void NCSummonedStatusWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    PetInfo Info = GConsole->GetPetInfo();
    if (Info.ObjectID == -1 || Info.Type != 1)
        HideWindow();
}
NW_HOOK_METHOD(0x100fad20, NCSummonedStatusWnd, ShowWindow)

// 0x100fccc0 (vslot 56) - C3: like the pet's, the background 46 high, the name at y 7, the bars 5 high; the
// time bar is a child
INT NCSummonedStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCSummonedStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, 12, 8, 0, 0, 12, 8, SummonFrameTopTex, 0xFF, 1);
    Device->DrawTexture(0, 8, 12, (INT)(Height - 16.0f), 0, 0, 12, 8, SummonFrameCenterTex, 0xFF, 1);
    Device->DrawTexture(0, (INT)(Height - 8.0f), 12, 8, 0, 0, 12, 8, SummonFrameBottomTex, 0xFF, 1);
    Device->DrawTexture(12, 0, 16, 46, 0, 0, 16, 46, LeftBackTex, 0xFF, 1);
    Device->DrawStretchedTex(28, 0, (INT)(Width - 16.0f - 16.0f - 12.0f), (INT)Height, 0, 0, 16, 46, BackTex, 0xFF);
    Device->DrawTexture((INT)(Width - 16.0f), 0, 16, 46, 0, 0, 16, 46, RightBackTex, 0xFF, 1);
    Summon = GConsole->GetPetInfo();
    INT HPWidth = 0, MPWidth = 0;
    if (Summon.MaxHP != 0)
        HPWidth = Summon.HP * 150 / Summon.MaxHP;
    if (Summon.MaxMP != 0)
        MPWidth = Summon.MP * 150 / Summon.MaxMP;
    if (Summon.Name[0])
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Summon.Name, 0, 0, 0, 0, 0);
        Device->DrawText((INT)((Width - 12.0f) * 0.5f + 12.0f - Size.cx / 2), 7, 0xFFDCDCDC, Summon.Name, 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->DrawTexture(16, 19, 150, 5, 0, 0, 8, 6, HPBackTex, 0xFF, 1);
    if (HPWidth > 0)
        Device->DrawTexture(16, 19, HPWidth, 5, 0, 0, 8, 6, HPTex, 0xFF, 1);
    Device->DrawTexture(16, 26, 150, 5, 0, 0, 8, 6, MPBackTex, 0xFF, 1);
    if (MPWidth > 0)
        Device->DrawTexture(16, 26, MPWidth, 5, 0, 0, 8, 6, MPTex, 0xFF, 1);
    if (bBuffs)
    {
        for (INT i = 0; i < BuffIcons.Num(); i++)
        {
            if (i < 10)
                Device->DrawStretchedTex((INT)((FLOAT)(i << 4) + Width + 1.0f), 5, 16, 16, BuffIcons(i), 0, 0xFF);
            else if (i < 20)
                Device->DrawStretchedTex((INT)((FLOAT)((i - 10) << 4) + Width + 1.0f), 21, 16, 16, BuffIcons(i), 0,
                                         0xFF);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100fccc0, NCSummonedStatusWnd, OnPaint)

// 0x100fabd0 (vslot 62) - stock body shared with NCPetStatusWnd's (the linker folded them: the record sits at +0x364
// in both, and that one's hook is the address's): right of the title bar, acts on the summon
INT NCSummonedStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if ((SWORD)LOWORD(lParam) > ScreenX + 13)
    {
        User* Me = GConsole->GetMyUser();
        if (Me)
            GConsole->RequestAction(Summon.ObjectID, Me->Pawn->Location);
    }
    return 1;
}

// 0x100fad10 - from the console (vslot 229) once the timer runs
void NCSummonedStatusWnd::SetRemaining(DWORD Remaining)
{
    TimeBar->SetRemaining(Remaining);
}
NW_HOOK_METHOD(0x100fad10, NCSummonedStatusWnd, SetRemaining)

// 0x10102410 - the buffs are forgotten, the bar runs for Max
void NCSummonedStatusWnd::StartTimer(DWORD Max)
{
    ClearBuffs();
    bTimerStarted = 1;
    TimeBar->SetMax(Max);
    TimeBar->Start();
}
NW_HOOK_METHOD(0x10102410, NCSummonedStatusWnd, StartTimer)

NW_CHECK_VTABLE(NCSummonedStatusWnd, 0x101a0040, 70)
