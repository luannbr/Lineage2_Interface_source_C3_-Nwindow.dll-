/*=============================================================================
    NCQuestInfoWnd.cpp - the quest window's description page. C2 stock: 0x100beea0 .. 0x100bf44f,
    0x100bfed0 .. 0x100c0083, 0x100c0090, 0x100c0200 (the quest array's destructor, NTemplates.cpp),
    0x100c0290 .. 0x100c0393, 0x100c0a20 .. 0x100c0a8d and 0x100c0b60 .. 0x100c0bac (the quest
    window and its item page sit in between).
=============================================================================*/
#include "nwindow/NCQuestInfoWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NConsoleWnd.h"

// how many characters the FString holds, as the stock tests it (the array's count, not the length)
static FORCEINLINE INT StringNum(const FString& S)
{
    return ((const TArray<TCHAR>*)&S)->Num();
}

// 0x100ddb90 - the quest array and the text start empty
NCQuestInfoWnd::NCQuestInfoWnd()
{
    Current = -1;
}
NW_HOOK_CTOR(0x100ddb90, NCQuestInfoWnd)

// 0x100ddc30 - the text and the quest array go, then NCScrollWnd's
NW_HOOK_DTOR(0x100ddc30, NCQuestInfoWnd)

// vslot 25: the stock slot is 0x10019810, the body the linker shared with NCItemWnd's (hooked there): the base's
// answer as a plain 0 / 1
INT NCQuestInfoWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCScrollWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10032b10 (vslot 37) - the body the linker shared with another window's: the base's, answering 1
INT NCQuestInfoWnd::OnCreate()
{
    NCScrollWnd::OnCreate();
    return 1;
}
NW_HOOK_METHOD(0x10032b10, NCQuestInfoWnd, OnCreate)

// 0x100dcc00 (vslot 56) - the text is drawn with the origin moved by the scroll position and measured after it,
// which sets the scroll range; the clip opens before BeginPaint (C3: the text's color is 0xFFDCDCDC, the width
// of the lines is the window's minus 20)
INT NCQuestInfoWnd::OnPaint(NCGDevice* Device)
{
    guard(NCQuestInfoWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (Current >= 0 && Current < Quests.Num() && StringNum(Text))
    {
        Device->PushOrigin(ScrollX, ScrollY);
        Device->DrawText(2, 1, 0xFFDCDCDC, *Text, (INT)(Width - 16.0f - 4.0f), 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, *Text, (INT)(Width - 16.0f - 4.0f), 0, 0, 0, 0);
        SetContentHeight(Size.cy);
        Device->PopOrigin();
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100dcc00, NCQuestInfoWnd, OnPaint)

// 0x100dcd90 (vslot 69) - NCScrollWnd's, but DeltaY is kept when already at the top
INT NCQuestInfoWnd::ScrollUp()
{
    if (ScrollY < 0)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY += LineStep;
        if (ScrollY > 0)
            ScrollY = 0;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX -= ScrollX;
        DeltaY -= ScrollY;
    }
    return 1;
}
NW_HOOK_METHOD(0x100dcd90, NCQuestInfoWnd, ScrollUp)

// 0x100dce20 (vslot 70) - NCScrollWnd's, but DeltaY is kept when already at the bottom
INT NCQuestInfoWnd::ScrollDown()
{
    if (ScrollY > MinScrollY)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY -= LineStep;
        if (ScrollY < MinScrollY)
            ScrollY = MinScrollY;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX -= ScrollX;
        DeltaY -= ScrollY;
    }
    return 1;
}
NW_HOOK_METHOD(0x100dce20, NCQuestInfoWnd, ScrollDown)

// 0x100dcec0 (vslot 71) - the slider's value is the scroll position as it comes (NCScrollWnd rounds it up to a
// whole line step)
INT NCQuestInfoWnd::OnScrollBar(INT Value)
{
    DeltaX = ScrollX;
    DeltaY = ScrollY;
    ScrollY = -Value;
    DeltaX -= ScrollX;
    DeltaY -= ScrollY;
    return 1;
}
NW_HOOK_METHOD(0x100dcec0, NCQuestInfoWnd, OnScrollBar)

// 0x100dcf20 - at the step the player is at; the text empties when the quest data has none
void NCQuestInfoWnd::ShowQuest(INT Index)
{
    Current = Index;
    ScrollBar->SetScrollPos(0);
    ScrollY = 0;
    if (Current >= 0 && Current < Quests.Num())
    {
        NQuestEntry* Quest = Quests(Current);
        Quest->ShownStep = Quest->Step;
        Quest = Quests(Current);
        const TCHAR* S = GConsole->GetQuestText(Quest->QuestID, Quest->Steps(Quest->Step));
        if (S)
        {
            Text = S;
            return;
        }
        Text.Empty();
    }
}
NW_HOOK_METHOD(0x100dcf20, NCQuestInfoWnd, ShowQuest)

// 0x100dcfb0 - the arrow forward (NCQuestWnd's command 0xF8): the next step, when the player has reached it
void NCQuestInfoWnd::NextStep()
{
    if (Current >= 0 && Current < Quests.Num())
    {
        INT Shown = Quests(Current)->ShownStep;
        if (Shown < Quests(Current)->Step)
        {
            ScrollBar->SetScrollPos(0);
            ScrollY = 0;
            Quests(Current)->ShownStep += 1;
            NQuestEntry* Quest = Quests(Current);
            const TCHAR* S = GConsole->GetQuestText(Quest->QuestID, Quest->Steps(Shown + 1));
            if (!S)
            {
                Text.Empty();
                return;
            }
            Text = S;
        }
    }
}
NW_HOOK_METHOD(0x100dcfb0, NCQuestInfoWnd, NextStep)

// 0x100dd050 - the arrow back (command 0xF9): the step before the one shown
void NCQuestInfoWnd::PrevStep()
{
    if (Current >= 0 && Current < Quests.Num())
    {
        INT Shown = Quests(Current)->ShownStep;
        if (Shown > 0)
        {
            ScrollBar->SetScrollPos(0);
            ScrollY = 0;
            Quests(Current)->ShownStep -= 1;
            NQuestEntry* Quest = Quests(Current);
            const TCHAR* S = GConsole->GetQuestText(Quest->QuestID, Quest->Steps(Shown - 1));
            if (!S)
            {
                Text.Empty();
                return;
            }
            Text = S;
        }
    }
}
NW_HOOK_METHOD(0x100dd050, NCQuestInfoWnd, PrevStep)

// 2^Index as the stock computes it (pow, then _ftol: 2^31 overflows into the sign bit)
static FORCEINLINE INT QuestBit(INT Index)
{
    return Index < 31 ? (1 << Index) : (INT)0x80000000;
}

NW_HOOK_DTOR(0x100ddaf0, NQuestEntry)

// 0x100ddcb0
void NCQuestInfoWnd::ClearQuests()
{
    for (INT i = 0; i < Quests.Num(); i++)
    {
        if (Quests(i))
            delete Quests(i);
    }
    Quests.Empty();
    Current = -1;
    Text.Empty();
}
NW_HOOK_METHOD(0x100ddcb0, NCQuestInfoWnd, ClearQuests)

// 0x100de560 (vslot 38)
INT NCQuestInfoWnd::OnDestroy()
{
    ClearQuests();
    NCScrollWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100de560, NCQuestInfoWnd, OnDestroy)

NW_CHECK_VTABLE(NCQuestInfoWnd, 0x10198b90, 72)
