/*=============================================================================
    NCRecipeManufactureItemWnd.cpp - the ingredient grid of the recipe windows (new in C2).
    Stock (C3): 0x100B1E70 (OnMouseMove), 0x100E2D50 (OnPaint), 0x100E4980 (tool tip), 0x100E4FD0 .. 0x100E5070
    (constructors, destructor), 0x100E50A0 / 0x100E5ED0 (the entries).
    WindowProc (0x10090b10), OnCreate (0x100DC9F0) and OnDestroy (0x100D7090) are bodies shared with
    NCSkillItemWnd / NCQuestItemWnd / NCShopItemWnd, hooked there.
=============================================================================*/
#include "nwindow/NCRecipeManufactureItemWnd.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NClasses.gen.h"

/*-----------------------------------------------------------------------------
    NCRecipeManufactureItemWnd
-----------------------------------------------------------------------------*/

// 0x100e4fd0
NCRecipeManufactureItemWnd::NCRecipeManufactureItemWnd()
{
}
NW_HOOK_CTOR(0x100e4fd0, NCRecipeManufactureItemWnd)

// 0x100e5070 (ret 0x10) - the sizes go to NCItemWnd's constructor of six arguments, the gaps 1 and 1 (the recipe windows ask
// for 6, 3, 160, 32); the paint adds its own gaps (5 and 3)
NCRecipeManufactureItemWnd::NCRecipeManufactureItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, 1, 1)
{
}
NW_HOOK_CTOR_ARGS(0x100e5070, NCRecipeManufactureItemWnd, (INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize), (InColumns, InVisibleRows, InMaxItems, InCellSize))

// 0x100e5020
NW_HOOK_DTOR(0x100e5020, NCRecipeManufactureItemWnd)

// the stock bodies shared with other item windows (hooked there): NCItemWnd's, with the result as 0 / 1
// 0x100a1050 (vslot 25, ret 0xC): NCSkillItemWnd's
#if NW_HOOK_MODE
NW_NAKED INT NCRecipeManufactureItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam) NW_STOCK(0x100a1050)
#else
NW_SLOT_IS("?WindowProc@NCRecipeManufactureItemWnd@@UAEHIIJ@Z", "?WindowProc@NCSkillItemWnd@@UAEHIIJ@Z")   // NCRecipeManufactureItemWnd::WindowProc = NCSkillItemWnd::WindowProc (0x100a1050)
#endif

// 0x100dc9f0 (vslot 37): NCQuestItemWnd's
#if NW_HOOK_MODE
NW_NAKED INT NCRecipeManufactureItemWnd::OnCreate() NW_STOCK(0x100dc9f0)
#else
NW_SLOT_IS("?OnCreate@NCRecipeManufactureItemWnd@@UAEHXZ", "?OnCreate@NCQuestItemWnd@@UAEHXZ")   // NCRecipeManufactureItemWnd::OnCreate = NCQuestItemWnd::OnCreate (0x100dc9f0)
#endif

// 0x100d7090 (vslot 38): NCShopItemWnd's
#if NW_HOOK_MODE
NW_NAKED INT NCRecipeManufactureItemWnd::OnDestroy() NW_STOCK(0x100d7090)
#else
NW_SLOT_IS("?OnDestroy@NCRecipeManufactureItemWnd@@UAEHXZ", "?OnDestroy@NCShopItemWnd@@UAEHXZ")   // NCRecipeManufactureItemWnd::OnDestroy = NCShopItemWnd::OnDestroy (0x100d7090)
#endif

// 0x100e2d50 (vslot 56) - NCItemWnd's loop without the selection frame: each icon, then a "ChatBack" square over it when
// the player has less of the ingredient (Count) than the recipe needs (UnknownC34). C3: the clip opens a pixel around the
// window before BeginPaint (the size a float plus 2), and the cells are 5 and 3 pixels apart.
INT NCRecipeManufactureItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCRecipeManufactureItemWnd::OnPaint);
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
            X += CellSize + 5;
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon(), 0xFF, 1);
        if (Item->Count < Item->UnknownC34)
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, FindTexture(TEXT("NWindow.ChatBack")), 0xFF, 1);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100e2d50, NCRecipeManufactureItemWnd, OnPaint)

// 0x100e4980 (vslot 57; the db's OnPostPaint) - the hovered entry's tool tip, always the full box (the grid has no
// selection): the skin gets a copy of the entry's tool tip object by value, the cell's position and the window's rectangle
INT NCRecipeManufactureItemWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (HoverIndex >= 0 && HoverIndex < Items.Num())
    {
        INT Index = HoverIndex - TopRow * Columns;
        INT Y = (CellSize + 3) * (Index / Columns);
        INT X = (CellSize + 5) * (Index % Columns);
        if (ToolTipSkin)
        {
            NToolTipRect Wnd;
            Wnd.X = ScreenX;
            Wnd.Y = ScreenY;
            Wnd.W = Width;
            Wnd.H = Height;
            ToolTipSkin->DrawFull(Device, Items(HoverIndex)->ToolTip, X, Y, Wnd);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x100e4980, NCRecipeManufactureItemWnd, OnDrawToolTip)

// 0x100b1e70 (vslot 61) - the entry under the mouse
INT NCRecipeManufactureItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    HoverIndex = HitTest(LOWORD(lParam), HIWORD(lParam), 0, 0);
    return 1;
}
NW_HOOK_METHOD(0x100b1e70, NCRecipeManufactureItemWnd, OnMouseMove)

// 0x100e5ed0 - a copy of Src as a new entry, its tool tip a recipe's (NTT_Recipe); 0 when the grid is full
UBOOL NCRecipeManufactureItemWnd::AddIngredient(NItemEntry* Src)
{
    if (Items.Num() >= MaxItems)
        return 0;
    NItemEntry* Item = (NItemEntry*)appMalloc(sizeof(NItemEntry), TEXT("new"));
    if (Item)
        Item = new ((void*)Item) NItemEntry;
    NItemEntryAssign(Item, Src);
    Item->FillToolTip(NTT_Recipe, 0);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100e5ed0, NCRecipeManufactureItemWnd, AddIngredient)

/*-----------------------------------------------------------------------------
    NItemEntry's plain assignment.
-----------------------------------------------------------------------------*/

// 0x100e50a0 - every field: the strings in full (the loops of 0x100, 0x100, 0x80 x 3, 0x800 and 0x100 characters), the
// numbers one by one (the word at +0x1940 as a word, its padding left alone), the tool tip object last. C3: the two numbers
// at +0x19B0 and +0x19B4 before the tool tip.
NItemEntry* NItemEntryOps::Assign(NItemEntry* Src)
{
    NItemEntry* Dst = this;
    Dst->Icon = Src->Icon;
    Dst->Unknown004 = Src->Unknown004;
    Dst->Unknown008 = Src->Unknown008;
    for (INT i = 0; i < 256; i++)
        Dst->Name[i] = Src->Name[i];
    for (INT i = 0; i < 256; i++)
        Dst->AdditionalName[i] = Src->AdditionalName[i];
    for (INT i = 0; i < 128; i++)
        Dst->IconName[i] = Src->IconName[i];
    for (INT i = 0; i < 128; i++)
        Dst->Unknown20C[i] = Src->Unknown20C[i];
    for (INT i = 0; i < 128; i++)
        Dst->Unknown30C[i] = Src->Unknown30C[i];
    for (INT i = 0; i < 2048; i++)
        Dst->Description[i] = Src->Description[i];
    for (INT i = 0; i < 256; i++)
        Dst->DisplayName[i] = Src->DisplayName[i];
    Dst->Type = Src->Type;
    Dst->Unknown1910 = Src->Unknown1910;
    Dst->ObjectID = Src->ObjectID;
    Dst->ID = Src->ID;
    Dst->UnknownC18 = Src->UnknownC18;
    Dst->Count = Src->Count;
    Dst->UnknownC20 = Src->UnknownC20;
    Dst->BodyPart = Src->BodyPart;
    Dst->Weight = Src->Weight;
    Dst->UnknownC2C = Src->UnknownC2C;
    Dst->UnknownC30 = Src->UnknownC30;
    Dst->UnknownC34 = Src->UnknownC34;
    Dst->Enchant = Src->Enchant;
    Dst->UnknownC3C = Src->UnknownC3C;
    Dst->bStackable = Src->bStackable;
    Dst->bEquipped = Src->bEquipped;
    Dst->TradeCount = Src->TradeCount;
    Dst->bSelected = Src->bSelected;
    Dst->Unknown1C58 = Src->Unknown1C58;
    Dst->CrystalGrade = Src->CrystalGrade;
    Dst->Unknown1C60 = Src->Unknown1C60;
    Dst->Unknown1C64 = Src->Unknown1C64;
    Dst->Unknown1C68 = Src->Unknown1C68;
    Dst->Unknown1C6C = Src->Unknown1C6C;
    Dst->Unknown1C70 = Src->Unknown1C70;
    Dst->SubType = Src->SubType;
    Dst->Unknown1C78 = Src->Unknown1C78;
    Dst->Unknown1C7C = Src->Unknown1C7C;
    Dst->Unknown1C80 = Src->Unknown1C80;
    Dst->Unknown1C84 = Src->Unknown1C84;
    Dst->Unknown1C88 = Src->Unknown1C88;
    Dst->Unknown1C8C = Src->Unknown1C8C;
    Dst->Unknown1C90 = Src->Unknown1C90;
    Dst->Unknown1C94 = Src->Unknown1C94;
    Dst->Unknown1C98 = Src->Unknown1C98;
    Dst->Unknown1998 = Src->Unknown1998;
    Dst->Unknown1C9C = Src->Unknown1C9C;
    Dst->Unknown1CA0 = Src->Unknown1CA0;
    Dst->Unknown1CA4 = Src->Unknown1CA4;
    Dst->Unknown1CA8 = Src->Unknown1CA8;
    Dst->Unknown19AC = Src->Unknown19AC;
    Dst->Unknown19B4 = Src->Unknown19B4;
    Dst->ToolTip = Src->ToolTip;
    return Dst;
}
NW_HOOK_METHOD(0x100e50a0, NItemEntryOps, Assign)

NW_CHECK_VTABLE(NCRecipeManufactureItemWnd, 0x1019a2d0, 75)
