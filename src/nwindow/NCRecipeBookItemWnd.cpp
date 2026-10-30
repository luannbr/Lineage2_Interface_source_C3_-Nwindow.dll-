/*=============================================================================
    NCRecipeBookItemWnd.cpp - the recipe book's icon grid (new in C2).
    Stock (C3): 0x100E27E0 .. 0x100E2B3F (the mouse handlers), 0x100E4440 (AddItem), 0x100E4B20 .. 0x100E4BF1
    (constructors, destructor). WindowProc (0x100CBA90) is the body shared with NCInvenItemWnd.
=============================================================================*/
#include "nwindow/NCRecipeBookItemWnd.h"
#include "nwindow/NCRecipeBookWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NClasses.gen.h"

enum { DIALOG_RECIPE_DELETE = 0x22 };  // NDialogInfo::ID (name ours): delete the recipe (Param1 its object id), the answer is the console's
enum { MSG_DeleteRecipe = 0x4a };      // systemmsg line the box shows

// 0x100e4b20
NCRecipeBookItemWnd::NCRecipeBookItemWnd()
{
}
NW_HOOK_CTOR(0x100e4b20, NCRecipeBookItemWnd)

// 0x100e4bc0 (ret 0x18) - the grid of the six arguments (columns, rows, most items, icon size, gaps)
NCRecipeBookItemWnd::NCRecipeBookItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
}
NW_HOOK_CTOR_ARGS(0x100e4bc0, NCRecipeBookItemWnd, (INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY), (InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY))

// 0x100e4b70
NW_HOOK_DTOR(0x100e4b70, NCRecipeBookItemWnd)

// 0x100cba90 (vslot 25, ret 0xC): the stock body is NCInvenItemWnd's (hooked there): the double click
#if NW_HOOK_MODE
NW_NAKED INT NCRecipeBookItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x100cba90)
#else
NW_SLOT_IS("?WindowProc@NCRecipeBookItemWnd@@UAEHIIJ@Z", "?WindowProc@NCInvenItemWnd@@UAEHIIJ@Z")   // NCRecipeBookItemWnd::WindowProc = NCInvenItemWnd::WindowProc (0x100cba90)
#endif

// 0x100e4440 - a new entry from the item record (icon from its name, "NWindow.BlackTexture" when missing, written back
// into Info), the tool tip its name line (NTT_ItemName). 0 when the grid is full.
UBOOL NCRecipeBookItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
    Info->Icon = FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = NewItemEntry(Info);
    Item->FillToolTip(1, 0);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100e4440, NCRecipeBookItemWnd, AddItem)

// 0x100e27e0 (vslot 62) - a click picks the recipe up
INT NCRecipeBookItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    HoverIndex = Selected;
    if (Selected >= 0 && Selected < Items.Num())
    {
        UnselectAll();
        Items(Selected)->Select();
        bDragging = 1;
        return 1;
    }
    return 1;
}
NW_HOOK_METHOD(0x100e27e0, NCRecipeBookItemWnd, OnLButtonDown)

// 0x100e2850 (vslot 61) - the hovered recipe; with the window focused and a recipe carried its icon is the cursor, and the
// parent book learns whether the cursor is over its trash icon (C3: the cursor is not given back here)
INT NCRecipeBookItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    HoverIndex = HitTest(X, Y, 0, 0);
    if (IsFocused())
    {
        if (bDragging && Selected >= 0 && Selected < Items.Num())
        {
            SetCursor(4, Items(Selected)->GetIcon());
            LockCursor();
        }
        NCRecipeBookWnd* Book = (NCRecipeBookWnd*)Entry->Parent->Wnd;
        NRectF Trash;
        Trash.X = Book->TrashX + Book->ScreenX;
        Trash.Y = Book->TrashY + Book->ScreenY;
        Trash.W = Book->TrashW;
        Trash.H = Book->TrashH;
        Book->bTrashActive = PtInRect(X, Y, Trash) != 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x100e2850, NCRecipeBookItemWnd, OnMouseMove)

// 0x100e2940 (vslot 63) - the recipe dropped on the trash icon of the book asks to be deleted (a yes / no box, ID 0x22,
// Param1 the recipe's object id)
INT NCRecipeBookItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (Selected >= 0 && Selected < Items.Num())
    {
        NWndEntry* E = FindWindowAt(X, Y);
        if (E && E->Wnd && E->Wnd->IsA(NClass::NCRecipeBookWnd()))
        {
            NCRecipeBookWnd* Book = (NCRecipeBookWnd*)E->Wnd;
            if (Book->HitTrash(X, Y))
            {
                TCHAR Name[64];
                appSprintf(Name, TEXT("%s"), Items(Selected)->GetName());
                NDialogInfo Info;
                Info.Owner = Entry;
                Info.ID = DIALOG_RECIPE_DELETE;
                Info.bMainBox = 1;
                Info.SetSystemMessage(MSG_DeleteRecipe);
                Info.SetButtons(Name, NULL);
                Info.SetParams(Items(Selected)->ObjectID, -1);
                GConsole->ShowDialog(&Info);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100e2940, NCRecipeBookItemWnd, OnLButtonUp)

// 0x100e2ac0 (vslot 75) - a double click asks the server for the recipe's ingredients and closes the grid
INT NCRecipeBookItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemEntry* Item = Items(Selected);
        if (Item)
        {
            GConsole->RequestRecipeItemMakeInfo(Item->ObjectID);
            HideWindow();
        }
    }
    UnselectAll();
    Selected = -1;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100e2ac0, NCRecipeBookItemWnd, OnLButtonDblClick)

NW_CHECK_VTABLE(NCRecipeBookItemWnd, 0x1019a010, 76)
