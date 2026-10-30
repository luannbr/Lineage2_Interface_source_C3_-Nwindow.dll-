/*=============================================================================
    NCAbnormalStatusWnd.cpp - the buff / debuff icons. C2 stock: 0x100d2d20 .. 0x100d2dee,
    0x100d37c0 .. 0x100d3bf5, 0x100d8540 .. 0x100d85bf (NAbnormalEntry's constructor), 0x100d8560 ..
    0x100d8a62 (0x100d84b0 is the pointer array's destructor).
=============================================================================*/
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"


#if NW_FEATURES
// phase 6: BuffTimer (NWindow_C1 src/inject/BuffTimer.cpp)
#include "features/BuffTimer.h"
#endif

// title bar id
enum { NID_ABNORMALSTATUS_FRAME = 0xFFA0B3 };

// 0x10102710 - the lists are empty (their arrays by the members' constructors), the timers and the textures none; the
// hover index is -1. The pointer at +0xE0 and the fade timer at +0xE8 are left as allocated.
NCAbnormalStatusWnd::NCAbnormalStatusWnd(INT InIconSize)
{
    TimerID = 0;
    ToolTipTex = NULL;
    TipTopTex = NULL;
    TipMidTex = NULL;
    TipBottomTex = NULL;
    IconSize = InIconSize;
    HoverIndex = -1;
}
NW_HOOK_CTOR_ARGS(0x10102710, NCAbnormalStatusWnd, (INT InIconSize), (InIconSize))


// 0x10119f60 (vslot 25) - the stock vtable points at NControl::WindowProc's body: NCWnd's, normalized to 0 or 1. The
// hook build keeps the C2 body (C2's shared 0x10004e90 returned 0 first for a disabled window except on WM_PAINT; the C3
// body has no such test): unhooked, it only serves objects our constructor makes. The standalone slot is the C3 one.
#if NW_HOOK_MODE
INT NCAbnormalStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}
#else
NW_SLOT_IS("?WindowProc@NCAbnormalStatusWnd@@UAEHIIJ@Z", "?WindowProc@NControl@@UAEHIIJ@Z")
#endif

// the blink of an icon in its last ten seconds (inlined in the stock OnTimer, for the list and for the extra icon):
// the alpha goes down by 25 until it is 0, then up until it is 255 (the direction flips at the ends)
static FORCEINLINE void FadeStep(NAbnormalEntry* E)
{
    if (E->Alpha < 0)
    {
        E->Alpha = 0;
        E->bFadeOut = 0;
    }
    else if (E->Alpha > 255)
    {
        E->Alpha = 255;
        E->bFadeOut = 1;
    }
    INT A = E->Alpha;
    if (E->bFadeOut)
        A -= 25;
    else
        A += 25;
    E->Alpha = A;
    if (A < 0)
    {
        E->Alpha = 0;
        E->bFadeOut = 0;
    }
    else if (A > 255)
    {
        E->Alpha = 255;
        E->bFadeOut = 1;
    }
}

// 0x100fc2b0 (vslot 36) - C3: two timers. Every second the time of every icon runs down (and the extra icon's, which
// is hidden when it is up and at 0 .. 10 seconds); every 100 ms the icons in their last ten seconds change their alpha
INT NCAbnormalStatusWnd::OnTimer(INT InTimerID)
{
    if (InTimerID == TIMER_AbnormalStatus)
    {
        for (INT i = 0; i < Statuses.Num(); i++)
        {
            NAbnormalEntry* Status = Statuses(i);
            if (Status->RemainTime > 0)
                Status->RemainTime--;
        }
        if (ExtraStatus)
        {
            if (ExtraStatus->RemainTime > 0)
                ExtraStatus->RemainTime--;
            if (ExtraStatus->RemainTime >= 0 && ExtraStatus->RemainTime <= 10 && ExtraStatus->Alpha <= 0)
            {
                ExtraStatus->bVisible = 0;
                return 1;
            }
        }
    }
    else if (InTimerID == TIMER_AbnormalFade)
    {
        for (INT i = 0; i < Statuses.Num(); i++)
        {
            NAbnormalEntry* Status = Statuses(i);
            if (Status->RemainTime >= 0 && Status->RemainTime <= 10)
                FadeStep(Status);
        }
        if (ExtraStatus && ExtraStatus->RemainTime >= 0 && ExtraStatus->RemainTime <= 10)
        {
            INT A = ExtraStatus->Alpha;
            if (A == 0)
                ExtraStatus->bFadeOut = 0;
            if (A == 255)
                ExtraStatus->bFadeOut = 1;
            if (ExtraStatus->bFadeOut)
                A -= 25;
            else
                A += 25;
            ExtraStatus->Alpha = A;
            if (A < 0)
            {
                ExtraStatus->Alpha = 0;
                ExtraStatus->bFadeOut = 0;
                return 1;
            }
            if (A > 255)
            {
                ExtraStatus->Alpha = 255;
                ExtraStatus->bFadeOut = 1;
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100fc2b0, NCAbnormalStatusWnd, OnTimer)

// 0x100ff510 (vslot 37) - C3: the two timers, the textures (the debuff frame is new) and the title bar (a vertical
// one, small)
INT NCAbnormalStatusWnd::OnCreate()
{
    NCWnd::OnCreate();
    TimerID = SetTimer(TIMER_AbnormalStatus, 1000, NULL);
    FadeTimerID = SetTimer(TIMER_AbnormalFade, 100, NULL);
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    TipTopTex = FindTexture(TEXT("L2UI.Tooltip.Tool02"));
    TipMidTex = FindTexture(TEXT("L2UI.Tooltip.Tool01"));
    TipBottomTex = FindTexture(TEXT("L2UI.Tooltip.Tool03"));
    BackTex = FindTexture(TEXT("L2UI.EtcWndBack.AbnormalBack"));
    DebuffTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.debuff"));

    FrameCtrl = new NCFrameCtrl(1, 1, 1);
    FrameID = NID_ABNORMALSTATUS_FRAME;
    FrameCtrl->ID = NID_ABNORMALSTATUS_FRAME;
    FrameCtrl->SetPosition(0, 0, 12, 26);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100ff510, NCAbnormalStatusWnd, OnCreate)

// 0x10103950 (vslot 38) - both timers killed, the records go (the stock code destroys only their tool tip and frees
// them: all they own), the base last
INT NCAbnormalStatusWnd::OnDestroy()
{
    guard(NCAbnormalStatusWnd::OnDestroy);
    if (TimerID)
    {
        KillTimer(TimerID);
        TimerID = 0;
    }
    if (FadeTimerID)
    {
        KillTimer(FadeTimerID);
        FadeTimerID = 0;
    }
    for (INT i = 0; i < Statuses.Num(); i++)
    {
        if (Statuses(i))
            delete Statuses(i);
    }
    Statuses.Empty();
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10103950, NCAbnormalStatusWnd, OnDestroy)

// stock body shared with NCChatWnd's (0x1003dab0)
void NCAbnormalStatusWnd::HideWindow()
{
    NCWnd::HideWindow();
}

// whether a skill is a debuff (the console's 0x1005f6c0, name ours: the skill's record of that level has its +0x48 set)
static FORCEINLINE UBOOL IsDebuff(INT SkillID, INT Level)
{
    return ((NConsoleWndSkillExt*)GConsole)->IsDebuffSkill(SkillID, Level);
}

// an icon of the first list (spelled out three times in the stock paint, one per row of ten): the back (26 x 26) at
// (BackX, BackY), the icon inside it (24 x 24, with the record's alpha: it blinks in its last ten seconds), the
// debuff frame over it; the record keeps the icon's place for its tool tip
static FORCEINLINE void DrawListIcon(NCGDevice* Device, UTexture* BackTex, UTexture* DebuffTex, NAbnormalEntry* E,
                                     INT BackX, INT BackY)
{
    Device->DrawTexture(BackX, BackY, 26, 26, 0, 0, 26, 26, BackTex, 0xFF, 1);
    if (E->bVisible)
        Device->DrawStretchedTex(BackX + 1, BackY + 1, 24, 24, E->Icon, 1, (BYTE)E->Alpha);
    E->IconX = BackX + 1;
    E->IconY = BackY + 1;
    if (IsDebuff(E->ID, E->UnknownC18) && E->bVisible)
        Device->DrawTexture(E->IconX, E->IconY, 24, 24, 0, 0, 26, 26, DebuffTex, (BYTE)E->Alpha, 1);
}

// an icon of the second list: the same, fully opaque and without the debuff frame
static FORCEINLINE void DrawSecondIcon(NCGDevice* Device, UTexture* BackTex, NAbnormalEntry* E, INT BackX, INT BackY)
{
    Device->DrawTexture(BackX, BackY, 26, 26, 0, 0, 26, 26, BackTex, 0xFF, 1);
    if (E->bVisible)
        Device->DrawStretchedTex(BackX + 1, BackY + 1, 24, 24, E->Icon, 1, 0xFF);
    E->IconX = BackX + 1;
    E->IconY = BackY + 1;
}

// the extra icon: always in the first column of the row under the first list; its tool tip place is the column after
// (39), where the second list starts when there is an extra icon
static FORCEINLINE void DrawExtraIcon(NCGDevice* Device, UTexture* BackTex, NAbnormalEntry* E, INT BackY)
{
    Device->DrawTexture(12, BackY, 26, 26, 0, 0, 26, 26, BackTex, 0xFF, 1);
    if (E->bVisible)
        Device->DrawStretchedTex(13, BackY + 1, 24, 24, E->Icon, 1, (BYTE)E->Alpha);
    E->IconX = 39;
    E->IconY = BackY + 1;
}

// 0x100fb880 (vslot 56) - C3: the window hides itself when it holds nothing. The first list: up to 30 icons, ten to a
// row (rows at y 0, 25, 50; 26 pixels apart from x 12); then the extra icon in the first column of the row after the
// list; then the second list under the first one (shifted a column right when there is an extra icon).
INT NCAbnormalStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCAbnormalStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (Statuses.Num() == 0 && Statuses2.Num() == 0 && ExtraStatus == NULL)
        HideWindow();
    for (INT i = 0; i < Statuses.Num(); i++)
    {
        NAbnormalEntry* E = Statuses(i);
        if (!E || !E->Icon)
            continue;
        if (i < 10)
            DrawListIcon(Device, BackTex, DebuffTex, E, i * 26 + 12, 0);
        else if (i < 20)
            DrawListIcon(Device, BackTex, DebuffTex, E, i * 26 - 248, 25);
        else if (i < 30)
            DrawListIcon(Device, BackTex, DebuffTex, E, i * 26 - 508, 50);
    }
    if (ExtraStatus)
    {
        INT Count = Statuses.Num();
        if (Count == 0)
            DrawExtraIcon(Device, BackTex, ExtraStatus, 0);
        else if (Count <= 10)
            DrawExtraIcon(Device, BackTex, ExtraStatus, 25);
        else if (Count <= 20)
            DrawExtraIcon(Device, BackTex, ExtraStatus, 50);
        else if (Count <= 30)
            DrawExtraIcon(Device, BackTex, ExtraStatus, 75);
    }
    for (INT i = 0; i < Statuses2.Num(); i++)
    {
        NAbnormalEntry* E = Statuses2(i);
        if (!E || !E->Icon)
            continue;
        INT Count = Statuses.Num();
        INT BackX = i * 26 + (ExtraStatus ? 38 : 12);
        if (Count == 0)
            DrawSecondIcon(Device, BackTex, E, BackX, 0);
        else if (Count <= 10)
            DrawSecondIcon(Device, BackTex, E, BackX, 25);
        else if (Count <= 20)
            DrawSecondIcon(Device, BackTex, E, BackX, 50);
        else if (Count <= 30)
            DrawSecondIcon(Device, BackTex, E, BackX, 75);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100fb880, NCAbnormalStatusWnd, OnPaint)

// the record in cell Index of the grid ten wide that HitTest numbers (the first list from the top, then the second list
// under it from the row after the first's last, the extra icon before it; the stock code spells it out for each of
// the first list's lengths): NULL when the cell is empty
static NAbnormalEntry* GridEntry(NCAbnormalStatusWnd* Wnd, INT Index)
{
    INT Count = Wnd->Statuses.Num();
    INT Count2 = Wnd->Statuses2.Num();
    INT Rows;
    if (Count == 0)
        Rows = 0;
    else if (Count <= 10)
        Rows = 10;
    else if (Count <= 20)
        Rows = 20;
    else
        Rows = 30;
    if (Index < Rows)
        return Index < Count ? Wnd->Statuses(Index) : NULL;
    INT Slot = Index - Rows;
    if (Wnd->ExtraStatus)
    {
        if (Slot == 0)
            return Wnd->ExtraStatus;
        Slot--;
    }
    return Slot < Count2 ? Wnd->Statuses2(Slot) : NULL;
}

// 0x10102820 (vslot 57) - C3: the tool tip of the icon under the mouse, in the window's tool tip skin over the icon's
// place the paint left in its record (the full box). Sets IconSize to 26 each time (HitTest divides by it); a
// HoverIndex out of 0 .. 40 ends before the clip is released, as in the stock code.
INT NCAbnormalStatusWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    IconSize = 26;
    if (HoverIndex < 0 || HoverIndex > 40)
        return 1;
    NAbnormalEntry* E = GridEntry(this, HoverIndex);
    if (E && E->bVisible)
    {
        NRectF Screen = {0, 0, 0.0f, 0.0f};
        GConsole->Desktop->GetLocalRect(&Screen);   // (the stock calls it and does not use the rectangle)
        if (ToolTipSkin)
        {
            NToolTipRect Wnd;
            Wnd.X = ScreenX;
            Wnd.Y = ScreenY;
            Wnd.W = Width;
            Wnd.H = Height;
            ToolTipSkin->DrawFull(Device, E->ToolTip, E->IconX, E->IconY, Wnd);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10102820, NCAbnormalStatusWnd, OnDrawToolTip)

// 0x100fa550 (vslot 68; name ours) - C3: the window's size from what it holds, with the title bar to match (12 wide,
// as high as the window). A row holds ten icons of 26 pixels (12 pixels of title bar before them) and is 26 high; a row
// is 25 pixels further down. The first list takes as many rows as it has tens; the second list and the extra icon go
// under it: a first list of up to ten and a second one (with the extra) longer than it makes the window as wide as
// the second one, else it is as wide as the first. More than ten icons make it ten wide (272).
void NCAbnormalStatusWnd::UpdateLayout()
{
    INT Count = Statuses.Num();
    INT Count2 = Statuses2.Num();
    if (Count <= 10)
    {
        if (!ExtraStatus && Count2 <= 0)
        {
            SetWindowSize((FLOAT)(Count * 26 + 12), 26.0f);
            FrameCtrl->SetFrameSize(12, 26);
            return;
        }
        if (Count2 + (ExtraStatus != NULL) > Count)
        {
            INT H = Count ? 51 : 26;
            INT W = ExtraStatus ? Count2 * 26 + 38 : Count2 * 26 + 12;
            SetWindowSize((FLOAT)W, (FLOAT)H);
            FrameCtrl->SetFrameSize(12, H);
            return;
        }
        SetWindowSize((FLOAT)(Count * 26 + 12), 51.0f);
        FrameCtrl->SetFrameSize(12, 51);
        return;
    }
    if (Count <= 20)
    {
        if (!ExtraStatus && Count2 <= 0)
        {
            SetWindowSize(272.0f, 51.0f);
            FrameCtrl->SetFrameSize(12, 51);
            return;
        }
        SetWindowSize(272.0f, 76.0f);
        FrameCtrl->SetFrameSize(12, 76);
        return;
    }
    if (Count <= 30 && !ExtraStatus && Count2 <= 0)
    {
        SetWindowSize(272.0f, 76.0f);
        FrameCtrl->SetFrameSize(12, 76);
        return;
    }
    SetWindowSize(272.0f, 101.0f);
    FrameCtrl->SetFrameSize(12, 101);
}
NW_HOOK_METHOD(0x100fa550, NCAbnormalStatusWnd, UpdateLayout)

// 0x100fc210 (vslot 61) - the base first (it drags a dragable window), then the icon under the mouse
INT NCAbnormalStatusWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    guard(NCAbnormalStatusWnd::OnMouseMove);
    NCWnd::OnMouseMove(wParam, lParam);
    HoverIndex = HitTest(LOWORD(lParam), HIWORD(lParam));
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100fc210, NCAbnormalStatusWnd, OnMouseMove)

// 0x100fa4f0 - the icon under a screen point, row by row of ten; the title bar is not taken off, so the hit areas sit
// 12 pixels left of the icons, and C3 has four rows
INT NCAbnormalStatusWnd::HitTest(INT X, INT Y)
{
    INT RX = X + (-12 - ScreenX);
    INT RY = Y - ScreenY;
    if (RX < 0 || RX > (IconSize * 5 + 5) * 2 || RY < 0 || RY > IconSize * 4 + 4)
        return -1;
    return RX / (IconSize + 1) + RY / (IconSize + 1) * 10;
}
NW_HOOK_METHOD(0x100fa4f0, NCAbnormalStatusWnd, HitTest)

// 0x10101d90 - a record from the console's one: the icon (by its texture name, the black texture when missing), the
// time, the skill id and level, the name and the description from the skill tables, the skill's tool tip; the window
// is opened (outside a replay) and the layout of the lists redone (vslot 68, UpdateLayout); 0 when the window holds 31 already
INT NCAbnormalStatusWnd::AddStatus(FAbnormalStatus* Status)
{
    if (Statuses.Num() > 30)
        return 0;
    if (!GL2ReplayMode)
    {
        if (!IsShowWindow())
            ShowWindow();
    }
    NAbnormalEntry* Entry = new NAbnormalEntry;
    if (Status->IconName)
        Entry->Icon = FindTexture(Status->IconName);
    if (!Entry->Icon)
        Entry->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    Entry->UnknownC18 = Status->Level;
    Entry->RemainTime = Status->RemainTime;
    Entry->ID = Status->SkillID;
    if (GConsole->GetSkillName(Status->SkillID, Status->Level))
        appStrcpy(Entry->Name, GConsole->GetSkillName(Status->SkillID, Status->Level));
    if (GConsole->GetSkillDesc(Status->SkillID, Status->Level))
        appStrcpy(Entry->Description, GConsole->GetSkillDesc(Status->SkillID, Status->Level));
    Entry->FillToolTip(NTT_SkillName, 0);
    INT i = Statuses.Add();
    Statuses(i) = Entry;
    UpdateLayout();
    return 1;
}
NW_HOOK_METHOD(0x10101d90, NCAbnormalStatusWnd, AddStatus)

// 0x10102be0 - the records go and the layout is redone (vslot 68)
void NCAbnormalStatusWnd::ClearStatus()
{
    for (INT i = 0; i < Statuses.Num(); i++)
    {
        if (Statuses(i))
            delete Statuses(i);
    }
    Statuses.Empty();
    UpdateLayout();
}
NW_HOOK_METHOD(0x10102be0, NCAbnormalStatusWnd, ClearStatus)

// 0x101027b0 - the two buff lists go, then NCWnd
NW_HOOK_DTOR(0x101027b0, NCAbnormalStatusWnd)

// 0x10101cb0 - the destructor instance of the two buff lists (the stock calls it for each from the window's)
typedef TArray<NAbnormalEntry*> TAbnormalEntryArray;
NW_HOOK_DTOR(0x10101cb0, TAbnormalEntryArray)

// 0x10101d40 - an item entry (the stock's constructor, 0x1001d8b0) with the blink state: visible, fading out, 0 s, opaque
NW_HOOK_CTOR(0x10101D40, NAbnormalEntry)

NW_CHECK_VTABLE(NCAbnormalStatusWnd, 0x1019f7f8, 69)
