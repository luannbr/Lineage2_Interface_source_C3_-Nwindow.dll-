/*=============================================================================
    NCQuestItemWnd.cpp - the quest window's item page. Stock: 0x1009fe70 .. 0x100a036d,
    0x100a0b40 .. 0x100a0fc5, 0x100a1490 .. 0x100a1578 and 0x100a17c0 .. 0x100a1889.
=============================================================================*/
#include "nwindow/NCQuestItemWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCQuestWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"

enum { DIALOG_DESTROY_ITEM = 5 };  // NDialogInfo::ID (name ours)

// 0x100ddd30
NCQuestItemWnd::NCQuestItemWnd()
{
}
NW_HOOK_CTOR(0x100ddd30, NCQuestItemWnd)

// 0x100dddd0 (name ours: the quest window's 6 x 4 icons) - NCItemWnd's constructor with a gap of 1 pixel
NCQuestItemWnd::NCQuestItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, 1, 1)
{
}
NW_HOOK_CTOR_ARGS(0x100dddd0, NCQuestItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x100ddd80 - the items are NCItemWnd's
NW_HOOK_DTOR(0x100ddd80, NCQuestItemWnd)


// 0x100dc990 (vslot 25) - NCItemWnd's, then only the destroy dialog's answer: the scroll buttons and the slider
// of this page do nothing (the wheel still scrolls)
INT NCQuestItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_DIALOG_RESULT)
        return ::nw::VirtualCall<INT>(this, 75, wParam, lParam) != 0;   // OnDialogResult, by number: NCScrollWnd.h shifts the slots of NCItemWnd's children by one for now
    return 0;
}
NW_HOOK_METHOD(0x100dc990, NCQuestItemWnd, WindowProc)

// 0x100dc9f0 (vslot 37) - NCItemWnd's, answering 1
INT NCQuestItemWnd::OnCreate()
{
    NCItemWnd::OnCreate();
    return 1;
}
NW_HOOK_METHOD(0x100dc9f0, NCQuestItemWnd, OnCreate)

// vslot 72: the stock slot is 0x10019c40, `ret 0x10` shared with NCItemWnd's (hooked there): no tool tip
void NCQuestItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
}

// 0x100dd0e0 (vslot 56) - NCItemWnd's, but the frame of the selected item goes over its icon
INT NCQuestItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCQuestItemWnd::OnPaint);
    Device->SetClip(ScreenX - 1, ScreenY - 1, (INT)(Width + 2.0f), (INT)(Height + 2.0f));
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 0;
    INT Y = 0;
    for (INT i = TopRow * Columns; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (!Item)
            continue;
        if (i % Columns == 0)
        {
            X = 0;
            if (i != TopRow * Columns)
                Y += CellSize + 3;
            if (VisibleRows * (CellSize + 1) <= Y)
                break;
        }
        else
            X += CellSize + 3;
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon());
        if (Item->IsSelected())
            Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, SelectTex);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100dd0e0, NCQuestItemWnd, OnPaint)

// 0x100dd2a0 (vslot 63) - the carried item is put down; over the quest window's drop zone it asks whether to
// destroy it (systemmsg 73 with the amount for a stack, else 74); the answer comes back as NWM_DIALOG_RESULT.
// The name buffer is the stock frame's 64 characters. C3: nothing of the cursor is touched first
INT NCQuestItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (Selected >= 0 && Selected < Items.Num() && bDragging)
    {
        INT X = LOWORD(lParam);
        INT Y = HIWORD(lParam);
        NWndEntry* E = FindWindowAt(X, Y);
        if (E && E->Wnd && E->Wnd->IsA(NCQuestWnd::StaticClass())
            && ((NCQuestWnd*)E->Wnd)->IsInDropZone(X, Y))
        {
            NItemEntry* Item = Items(Selected);
            TCHAR Name[64];
            if (Item->IsStackable() && Item->Count > 1)
            {
                appSprintf(Name, TEXT("%s"), Item->GetName());
                NDialogInfo Info;
                Info.ID = DIALOG_DESTROY_ITEM;
                Info.Type = 3;
                Info.Owner = Entry;
                Info.bMainBox = 1;
                Info.bInput = 1;
                Info.SetSystemMessage(73);  // "How many $s1(s) do you want to destroy?"
                Info.SetButtons(Name, NULL);
                Info.Unknown74 = Item->Count;
                GConsole->ShowDialog(&Info);
            }
            else
            {
                appSprintf(Name, TEXT("%s"), Item->GetName());
                NDialogInfo Info;
                Info.Owner = Entry;
                Info.ID = DIALOG_DESTROY_ITEM;
                Info.bMainBox = 1;
                Info.bInput = 0;
                Info.SetSystemMessage(74);  // "Destroy $s1. Do you want to continue?"
                Info.SetButtons(Name, NULL);
                GConsole->ShowDialog(&Info);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100dd2a0, NCQuestItemWnd, OnLButtonUp)

// 0x100dd4e0 (vslot 75) - the destroy dialog's answer (wParam: the amount typed, 1 without an edit line): the
// request for the selected item
INT NCQuestItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemRequest Request;
        Request.ObjectID = Items(Selected)->ObjectID;
        Request.Count = (INT)wParam;
        ::nw::VirtualCall<void>(GConsole, 395, Request);   // RequestDestroyItem, by number: NConsoleWnd.h compiles its new virtuals 76 slots too high for now
        ::nw::VirtualCall<void>(this, 73);                 // UnselectAll, by number (see WindowProc)
        Selected = -1;
    }
    return 1;
}
NW_HOOK_METHOD(0x100dd4e0, NCQuestItemWnd, OnDialogResult)

NW_CHECK_VTABLE(NCQuestItemWnd, 0x10198cb0, 76)
