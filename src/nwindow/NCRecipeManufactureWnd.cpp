/*=============================================================================
    NCRecipeManufactureWnd.cpp - the two "Item Creation" windows (new in C2): NCRecipeManufactureWnd (the player's own
    recipe) and NCRecipeBuyManufactureWnd (an offer of a recipe shop). The stock code is two copies of one source with
    the fields shifted (the buy window has two more, +0x1B30 and +0x1B34), so OnCreate, OnPaint and SetRecipe are written once
    as templates over the window type and each class' hook points at its own copy:
        NCRecipeManufactureWnd     0x100DED60, 0x100E25F0 .. 0x100E2F20, 0x100E3010, 0x100E37C0 .. 0x100E3890, 0x100E53B0 .. 0x100E5FA0
        NCRecipeBuyManufactureWnd  0x100DECB0 .. 0x100DEE30, 0x100DFC90, 0x100DFD80, 0x100E0550 .. 0x100E0FF0, 0x100E2260 .. 0x100E2350
    0x100E4A80 (the tree button) is one function the two vtables share; it is hooked on the buy window and the other
    class' override has the same code. 0x100DEC70 (Close of the manufacture window) is NCRecipeBuyListWnd::OnClose's body.
=============================================================================*/
#include "nwindow/NCRecipeManufactureWnd.h"
#include "nwindow/NCRecipeBuyManufactureWnd.h"
// C3: class gone, no counterpart: #include "NCRecipeTreeWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCRecipeConsoleOps.h"
#include "nwindow/NCRecipeGaraTreeCtrl.h"
#include "nwindow/NClasses.gen.h"

// sysstring labels (SysString index = (stock offset - 0x65A8C) / 12)
enum
{
    STR_MPCost = 320,            // "MP cost"
    STR_Creator_MP = 644,        // "Creator MP"
    STR_Create = 645,
    STR_Close = 646,
    STR_ResultCount = 647,       // "Number of Resulting Items"
    STR_RetainedCount = 648,     // "Number of Retained Items"
    STR_SuccessRate = 642,       // "Success Rate"
    STR_Ingredients = 742,       // "Ingredient(s)"
    STR_Results = 743,           // "Result(s)"
    STR_Prev = 543,              // "<PREV"
    STR_TreeTip = 662,           // the tree button's tool tip
};
// systemmsg lines of the outcome (name ours): "$s1 manufacturing failure." / "$s1 (*$s2) manufacturing success."
enum { MSG_MakeFailure = 960, MSG_MakeSuccess = 959 };
enum { SOUND_Close = 6 };        // NConsoleWnd::PlayInterfaceSound

// a child window goes (repeated inline in the stock OnDestroys)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// the player's count of an item: the stacks of Grid whose id is the entry's added to Entry's Count
static FORCEINLINE void AddOwned(NCItemWnd* Grid, NItemEntry* Entry)
{
    for (INT i = 0; i < Grid->Items.Num(); i++)
        if (Entry->ID == Grid->Items(i)->ID)
            Entry->Count += Grid->Items(i)->Count;
}

// the point inside the rectangle (the tree button's local rectangle, its size in floats cut)
static FORCEINLINE UBOOL PointInButton(NCWnd* Owner, NCWnd* Button, INT X, INT Y)
{
    NRectF R;
    R.X = 0;
    R.Y = 0;
    R.W = 0.0f;
    R.H = 0.0f;
    Button->GetLocalRect(&R);
    INT RelX = X - (Owner->ScreenX + R.X);
    INT RelY = Y - (Owner->ScreenY + R.Y);
    if (RelX >= 0 && RelX <= (INT)R.W && RelY >= 0 && RelY <= (INT)R.H)
        return 1;
    return 0;
}

/*-----------------------------------------------------------------------------
    The code the two windows share, over the window type T (the same field names in both).
-----------------------------------------------------------------------------*/

// the tree button's frame textures, the title bar (id 0xFF), the background and the MP bar's texture, the ingredient grid,
// the tree button (with its tool tip), Create, <PREV and Close
template <class T> static FORCEINLINE INT MakingOnCreate(T* W, INT CloseID)
{
    W->NCWnd::OnCreate();

    W->TreeTex = W->FindTexture(TEXT("L2UI_CH3.Etc.menu_outline"));
    W->TreeDownTex = W->FindTexture(TEXT("L2UI_CH3.Etc.menu_outline_down"));
    W->TreeCurTex = W->TreeTex;

    W->FrameCtrl = new NCFrameCtrl(1, 0, 0);
    W->FrameCtrl->ID = NID_RECIPEMAKE_FRAME;
    W->FrameCtrl->SetPosition(0, 0, 256, 20);
    W->CreateChild(W->FrameCtrl, NWS_VISIBLE);

    W->BackTex = W->FindTexture(TEXT("L2UI_CH3.RecipeWnd.Recipe4_Back"));
    W->MPTex = W->FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_mpbar"));

    W->IngredientWnd = new NCRecipeManufactureItemWnd(6, 3, 160, 32);
    W->IngredientWnd->SetPosition(10, 141, 238, 103);
    W->CreateChild(W->IngredientWnd, NWS_VISIBLE | 0x20);

    W->TreeButton = new NCButton(TEXT("L2UI.RecipeWnd.RecipeTreeBtn"), TEXT("L2UI.RecipeWnd.RecipeTreeBtn_click"), 0);
    W->TreeButton->ID = NID_RECIPEMAKE_TREE;
    W->TreeButton->SetPosition(206, 71, 32, 32);
    W->CreateChild(W->TreeButton, NWS_VISIBLE | 0x20);
    W->TreeButton->SetToolTip(STR_TreeTip, 1);

    W->MakeButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    W->MakeButton->ID = NID_RECIPEMAKE_CREATE;
    W->MakeButton->SetPosition(11, 372, 76, 23);
    W->CreateChild(W->MakeButton, NWS_VISIBLE | 0x20);
    W->MakeButton->SetTextID(STR_Create);

    W->PrevButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    W->PrevButton->ID = NID_RECIPEMAKE_PREV;
    W->PrevButton->SetPosition(91, 372, 76, 23);
    W->CreateChild(W->PrevButton, NWS_VISIBLE | 0x20);
    W->PrevButton->SetTextID(STR_Prev);

    W->CloseButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    W->CloseButton->ID = CloseID;
    W->CloseButton->SetPosition(171, 372, 76, 23);
    W->CreateChild(W->CloseButton, NWS_VISIBLE | 0x20);
    W->CloseButton->SetTextID(STR_Close);
    return 1;
}

// the grid and the four buttons go (the title bar is NCWnd::OnDestroy's)
template <class T> static FORCEINLINE INT MakingOnDestroy(T* W)
{
    DestroyChild(W, W->IngredientWnd);
    DestroyChild(W, W->TreeButton);
    DestroyChild(W, W->MakeButton);
    DestroyChild(W, W->PrevButton);
    DestroyChild(W, W->CloseButton);
    W->NCWnd::OnDestroy();
    return 1;
}

// the product's count in the two inventory pages (NCInventoryWnd's "Item" and "Quest"). The buy window's copy of the code adds
// the second page's counts from the FIRST page's entries at the same index (the stock code does, kept as it is).
static FORCEINLINE INT OwnedCount(INT ProductID, UBOOL bBuy)
{
    NCInventoryWnd* Inventory = GDesktop->InventoryWnd;
    NCInvenItemWnd* Page0 = Inventory->ItemWnd[0];
    NCInvenItemWnd* Page1 = Inventory->ItemWnd[1];
    INT Total = 0;
    if (Page0)
    {
        for (INT i = 0; i < Page0->Items.Num(); i++)
            if (ProductID == Page0->Items(i)->ID)
                Total += Page0->Items(i)->Count;
    }
    if (Page1)
    {
        for (INT i = 0; i < Page1->Items.Num(); i++)
            if (ProductID == Page1->Items(i)->ID)
                Total += (bBuy ? Page0->Items(i)->Count : Page1->Items(i)->Count);
    }
    return Total;
}

// the product's name, grade and numbers, the ingredient and result captions, the tree button's frame, the outcome line, the
// count of the product the player keeps and the creator's MP bar. bBuy: the buy window (its outcome line is not limited in
// width and its count of kept items is the stock code's with the second page's bug).
template <class T> static FORCEINLINE INT MakingOnPaint(T* W, NCGDevice* Device, UBOOL bBuy)
{
    SIZE Size;
    TCHAR Text[68];
    TCHAR Line[260];
    Device->BeginPaint(W->ScreenX, W->ScreenY);
    Device->SetClip(W->ScreenX, W->ScreenY, (INT)W->Width, (INT)W->Height);
    if (W->BackTex)
        Device->DrawTexture(0, 20, 256, 381, 0, 0, 256, 381, W->BackTex, 0xFF, 1);
    if (W->Product.Icon)
        Device->DrawTexture(16, 49, 32, 32, 0, 0, 32, 32, W->Product.Icon, 0xFF, 1);

    NCGDevice::GetTextExtent(&Size, W->Product.Name, 0, 0, 0, 0, 0);
    Device->DrawText(58, 48, 0xFFDCDCDC, W->Product.Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    if (GConsole->GetGradeText(W->Product.CrystalGrade))
    {
        appSprintf(Text, TEXT("`%s`"), GConsole->GetGradeText(W->Product.CrystalGrade));
        Device->DrawText(Size.cx + 60, 48, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 1, 0, 0, 0, 0, 0, 0);
    }

    // "MP cost : N"
    NCGDevice::GetTextExtent(&Size, *NCObject::SysString(STR_MPCost), 0, 0, 0, 0, 0);
    Device->DrawText(58, 64, 0xFFA3A3A3, *NCObject::SysString(STR_MPCost), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    INT SepW = Device->DrawText(Size.cx + 58, 64, 0xFFDCDCDC, TEXT(" : "), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(SepW + 58 + Size.cx, 64, 0xFFB09B79, appItoa(W->MPCost), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);

    // "Success Rate : N%"
    NCGDevice::GetTextExtent(&Size, *NCObject::SysString(STR_SuccessRate), 0, 0, 0, 0, 0);
    Device->DrawText(58, 80, 0xFFA3A3A3, *NCObject::SysString(STR_SuccessRate), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("%d%%"), W->SuccessRate);
    SepW = Device->DrawText(Size.cx + 58, 80, 0xFFDCDCDC, TEXT(" : "), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(SepW + 58 + Size.cx, 80, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);

    // "Number of Resulting Items : N"
    NCGDevice::GetTextExtent(&Size, *NCObject::SysString(STR_ResultCount), 0, 0, 0, 0, 0);
    Device->DrawText(58, 96, 0xFFA3A3A3, *NCObject::SysString(STR_ResultCount), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    SepW = Device->DrawText(Size.cx + 58, 96, 0xFFDCDCDC, TEXT(" : "), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(SepW + 58 + Size.cx, 96, 0xFFB09B79, appItoa(W->ProductCount), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);

    // the captions of the grid and of the result line
    Device->DrawText(11, 124, 0xFFDCDCDC, *NCObject::SysString(STR_Ingredients), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(11, 290, 0xFFDCDCDC, *NCObject::SysString(STR_Results), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);

    // the tree button's frame (the pressed one while the mouse is over it)
    if (W->TreeCurTex)
        Device->DrawStretchedTex(205, 70, 36, 36, 0, 0, 34, 34, W->TreeCurTex, 0xFF);

    // the outcome of the last try (the product's name, and the count on a success)
    const TCHAR* Format = NULL;
    const TCHAR* CountText = NULL;
    if (W->Result == 0)
        Format = *GL2GameData.SystemMsg[MSG_MakeFailure].Message;
    else if (W->Result == 1)
    {
        CountText = appItoa(W->ProductCount);
        Format = *GL2GameData.SystemMsg[MSG_MakeSuccess].Message;
    }
    if (Format)
    {
        GConsole->FormatSystemMessage(Line, Format, W->Product.Name, CountText);
        if (bBuy)
            Device->DrawText(14, 329, 0xFFB2BECF, Line, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        else
            Device->DrawText(14, 329, 0xFFB2BECF, Line, (INT)(W->Width - 30.0f), 0, 1, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    }

    // "Number of Retained Items : N": the player's stacks of the product, in the two inventory pages
    NCGDevice::GetTextExtent(&Size, *NCObject::SysString(STR_RetainedCount), 0, 0, 0, 0, 0);
    Device->DrawText(14, 313, 0xFFA3A3A3, *NCObject::SysString(STR_RetainedCount), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    INT Total = OwnedCount(W->ProductID, bBuy);
    SepW = Device->DrawText(Size.cx + 14, 313, 0xFFDCDCDC, TEXT(" : "), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(SepW + 14 + Size.cx, 313, 0xFFB09B79, appItoa(Total), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);

    // "Creator MP" and its bar: CreatorMP / CreatorMaxMP of 165 pixels
    Device->DrawText(18, 261, 0xFFDCDCDC, *NCObject::SysString(STR_Creator_MP), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    INT BarW = (INT)((DOUBLE)(W->CreatorMP * 165) / W->CreatorMaxMP);
    if (BarW > 0)
    {
        if (BarW > 165)
            BarW = 165;
        Device->DrawStretchedTex(75, 261, BarW, 12, 0, 0, 8, 12, W->MPTex, 0xFF);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}

// the recipe's numbers, the product's strings, icon and grade, then an entry per ingredient (the item's own data by the
// console's MakeItemInfo, the player's count of it added from both inventory pages) into the grid. The entries made here
// are copied by the grid and never freed (as in the stock code).
template <class T> static FORCEINLINE void MakingSetRecipe(T* W, NRecipeData* Rec)
{
    W->RecipeID = Rec->RecipeID;
    W->ProductID = Rec->ProductID;
    W->ProductCount = Rec->ProductCount;
    W->MPCost = Rec->MPCost;
    W->SuccessRate = Rec->SuccessRate;
    NViewItemData* Item = NRecipeFindItem(&Rec->ProductID);
    if (Item)
    {
        appStrcpy(W->Product.Name, *Item->Name);
        appStrcpy(W->Product.Description, *NItemRecordDescription(Item));
        appStrcpy(W->Product.IconName, *Item->IconName);
        W->Product.Icon = W->FindTexture(W->Product.IconName);
        if (!W->Product.Icon)
            W->Product.Icon = W->FindTexture(TEXT("NWindow.BlackTexture"));
        NItemRecordGrade(Item, &W->Product.CrystalGrade);
    }
    for (INT i = 0; i < Rec->Materials.Num(); i++)
    {
        NRecipeMaterial* Material = Rec->Materials(i);
        INT ID = Material->ItemID;
        INT Need = Material->Count;
        NItemEntry* Entry = (NItemEntry*)appMalloc(sizeof(NItemEntry), TEXT("new"));
        if (Entry)
            Entry = new ((void*)Entry) NItemEntry;
        Entry->ID = ID;
        Entry->UnknownC34 = Need;
        NMakeItemInfo(Entry);   // the console's MakeItemInfo: the entry's strings from the item table
        Entry->Icon = W->FindTexture(Entry->IconName);
        if (!Entry->Icon)
            Entry->Icon = W->FindTexture(TEXT("NWindow.BlackTexture"));
        NCInventoryWnd* Inventory = GDesktop->InventoryWnd;
        NCInvenItemWnd* Page0 = Inventory->ItemWnd[0];
        NCInvenItemWnd* Page1 = Inventory->ItemWnd[1];
        if (Page0)
            AddOwned(Page0, Entry);
        if (Page1)
            AddOwned(Page1, Entry);
        W->AddIngredient(Entry);
    }
}

// 0x100E4A80 - the tree button: the tree window hidden when it is shown, else filled with this recipe (its id and the success
// rate) and shown with the focus. The tree window is the desktop's +0x170 (NCRecipeGaraTreeWnd).
template <class T> static FORCEINLINE INT MakingOnShowTree(T* W)
{
    if (!GDesktop->RecipeGaraTreeWnd->IsShowWindow())
    {
        static_cast<NCRecipeGaraTreeWnd*>(GDesktop->RecipeGaraTreeWnd)->Reset();
        static_cast<NCRecipeGaraTreeWnd*>(GDesktop->RecipeGaraTreeWnd)->SetRecipe(W->RecipeID, W->SuccessRate);
        GDesktop->RecipeGaraTreeWnd->ShowWindow();
        GDesktop->RecipeGaraTreeWnd->SetFocus();
        return 1;
    }
    GDesktop->RecipeGaraTreeWnd->HideWindow();
    return 1;
}

/*-----------------------------------------------------------------------------
    NCRecipeManufactureWnd
-----------------------------------------------------------------------------*/

// 0x100e53b0 - the product entry is constructed by its member; the grid, the buttons and the numbers cleared, no outcome (-1)
NCRecipeManufactureWnd::NCRecipeManufactureWnd()
{
    IngredientWnd = NULL;
    TreeButton = NULL;
    MakeButton = NULL;
    PrevButton = NULL;
    CloseButton = NULL;
    RecipeID = 0;
    ProductID = 0;
    ProductCount = 0;
    MPCost = 0;
    SuccessRate = 0;
    CreatorMP = 0;
    CreatorMaxMP = 0;
    bTreePressed = 0;
    Result = -1;
}
NW_HOOK_CTOR(0x100e53b0, NCRecipeManufactureWnd)

// 0x100e5490
NW_HOOK_DTOR(0x100e5490, NCRecipeManufactureWnd)

// 0x100e2540 (vslot 25) - the four buttons
INT NCRecipeManufactureWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_RECIPEMAKE_CLOSE)
            return OnClose() != 0;
        if (wParam == NID_RECIPEMAKE_PREV)
            return OnPrev() != 0;
        if (wParam == NID_RECIPEMAKE_TREE)
            return OnShowTree() != 0;
        if (wParam == NID_RECIPEMAKE_CREATE)
            return OnMake() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100e2540, NCRecipeManufactureWnd, WindowProc)

// 0x100e54f0 (vslot 37)
INT NCRecipeManufactureWnd::OnCreate()
{
    return MakingOnCreate(this, NID_RECIPEMAKE_CLOSE);
}
NW_HOOK_METHOD(0x100e54f0, NCRecipeManufactureWnd, OnCreate)

// 0x100e2f20 (vslot 38)
INT NCRecipeManufactureWnd::OnDestroy()
{
    return MakingOnDestroy(this);
}
NW_HOOK_METHOD(0x100e2f20, NCRecipeManufactureWnd, OnDestroy)

// 0x100e3010 (vslot 56)
INT NCRecipeManufactureWnd::OnPaint(NCGDevice* Device)
{
    guard(NCRecipeManufactureWnd::OnPaint);
    return MakingOnPaint(this, Device, 0);
    unguard;
}
NW_HOOK_METHOD(0x100e3010, NCRecipeManufactureWnd, OnPaint)

// 0x100e37c0 (ret 8; sub_ in the db) - (X, Y) inside the tree button
UBOOL NCRecipeManufactureWnd::HitTree(INT X, INT Y)
{
    return PointInButton(this, TreeButton, X, Y);
}
NW_HOOK_METHOD(0x100e37c0, NCRecipeManufactureWnd, HitTree)

// 0x100e3890 (vslot 61) - the pressed frame while the mouse is over the tree button
INT NCRecipeManufactureWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if (HitTree((SWORD)(lParam & 0xFFFF), (SWORD)((DWORD)lParam >> 16)))
        TreeCurTex = TreeDownTex;
    else
        TreeCurTex = TreeTex;
    return 1;
}
NW_HOOK_METHOD(0x100e3890, NCRecipeManufactureWnd, OnMouseMove)

// 0x100e3850 (vslot 62) - the mouse goes down on the tree button
INT NCRecipeManufactureWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (HitTree((SWORD)(lParam & 0xFFFF), (SWORD)((DWORD)lParam >> 16)))
    {
        bTreePressed = 1;
        TreeCurTex = TreeDownTex;
    }
    return 1;
}
NW_HOOK_METHOD(0x100e3850, NCRecipeManufactureWnd, OnLButtonDown)

// 0x100e25f0 (vslot 63) - released anywhere: the normal frame
INT NCRecipeManufactureWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    TreeCurTex = TreeTex;
    bTreePressed = 0;
    return 1;
}
NW_HOOK_METHOD(0x100e25f0, NCRecipeManufactureWnd, OnLButtonUp)

// 0x100e2650 (vslot 68) - back to the recipe book
INT NCRecipeManufactureWnd::OnPrev()
{
    HideWindow();
    GConsole->RequestRecipeBookOpen();
    GConsole->PlayInterfaceSound(SOUND_Close);
    return 1;
}
NW_HOOK_METHOD(0x100e2650, NCRecipeManufactureWnd, OnPrev)

// vslot 69 - Close: the stock body (0x100dec70) is NCRecipeBuyListWnd::OnClose's, hooked there
INT NCRecipeManufactureWnd::OnClose()
{
    HideWindow();
    GConsole->PlayInterfaceSound(SOUND_Close);
    return 1;
}

// vslot 70 - the tree button: 0x100e4a80, the body the two recipe making windows share (hooked on the buy window)
INT NCRecipeManufactureWnd::OnShowTree()
{
    return MakingOnShowTree(this);
}

// 0x100e2680 (vslot 71) - Create: the server makes the recipe for the player
INT NCRecipeManufactureWnd::OnMake()
{
    GConsole->RequestRecipeItemMakeSelf(RecipeID);
    return 1;
}
NW_HOOK_METHOD(0x100e2680, NCRecipeManufactureWnd, OnMake)

// 0x100e2610 - the numbers zeroed (not the outcome field, nor MP) and the grid emptied (NCItemWnd::Clear, vslot 72)
void NCRecipeManufactureWnd::Reset()
{
    RecipeID = 0;
    ProductID = 0;
    ProductCount = 0;
    MPCost = 0;
    SuccessRate = 0;
    CreatorMP = 0;
    CreatorMaxMP = 0;
    IngredientWnd->Clear();
}
NW_HOOK_METHOD(0x100e2610, NCRecipeManufactureWnd, Reset)

// 0x100e5fa0 - the recipe shown (nothing when the record is missing or has no product)
void NCRecipeManufactureWnd::SetRecipe(NRecipeData* Rec)
{
    if (Rec && Rec->ProductRec)
        MakingSetRecipe(this, Rec);
}
NW_HOOK_METHOD(0x100e5fa0, NCRecipeManufactureWnd, SetRecipe)

// 0x100ded60 - the entry goes to the ingredient grid (shared with the buy window's SetRecipe)
UBOOL NCRecipeManufactureWnd::AddIngredient(NItemEntry* Item)
{
    if (IngredientWnd)
        IngredientWnd->AddIngredient(Item);
    return 1;
}
NW_HOOK_METHOD(0x100ded60, NCRecipeManufactureWnd, AddIngredient)

NW_CHECK_VTABLE(NCRecipeManufactureWnd, 0x1019a400, 72)

/*-----------------------------------------------------------------------------
    NCRecipeBuyManufactureWnd
-----------------------------------------------------------------------------*/

// 0x100e2260 - the product entry is constructed by its member; the grid, the buttons and the numbers cleared, no outcome (-1)
NCRecipeBuyManufactureWnd::NCRecipeBuyManufactureWnd()
{
    IngredientWnd = NULL;
    TreeButton = NULL;
    MakeButton = NULL;
    PrevButton = NULL;
    CloseButton = NULL;
    RecipeID = 0;
    ProductID = 0;
    ProductCount = 0;
    MPCost = 0;
    SuccessRate = 0;
    CreatorMP = 0;
    CreatorMaxMP = 0;
    ShopID = 0;
    MakeParam = 0;
    bTreePressed = 0;
    Result = -1;
}
NW_HOOK_CTOR(0x100e2260, NCRecipeBuyManufactureWnd)

// 0x100e2350
NW_HOOK_DTOR(0x100e2350, NCRecipeBuyManufactureWnd)

// 0x100decb0 (vslot 25) - the four buttons (Close is 0xF8 here)
INT NCRecipeBuyManufactureWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_RECIPEBUYMAKE_CLOSE)
            return OnClose() != 0;
        if (wParam == NID_RECIPEMAKE_PREV)
            return OnPrev() != 0;
        if (wParam == NID_RECIPEMAKE_TREE)
            return OnShowTree() != 0;
        if (wParam == NID_RECIPEMAKE_CREATE)
            return ::nw::VirtualCall<INT>(this, 71) != 0;   // slot 71 (a placeholder in the header: OnMake)
    }
    return 0;
}
NW_HOOK_METHOD(0x100decb0, NCRecipeBuyManufactureWnd, WindowProc)

// 0x100e0b80 (vslot 37)
INT NCRecipeBuyManufactureWnd::OnCreate()
{
    return MakingOnCreate(this, NID_RECIPEBUYMAKE_CLOSE);
}
NW_HOOK_METHOD(0x100e0b80, NCRecipeBuyManufactureWnd, OnCreate)

// 0x100dfc90 (vslot 38)
INT NCRecipeBuyManufactureWnd::OnDestroy()
{
    return MakingOnDestroy(this);
}
NW_HOOK_METHOD(0x100dfc90, NCRecipeBuyManufactureWnd, OnDestroy)

// 0x100dfd80 (vslot 56)
INT NCRecipeBuyManufactureWnd::OnPaint(NCGDevice* Device)
{
    guard(NCRecipeBuyManufactureWnd::OnPaint);
    return MakingOnPaint(this, Device, 1);
    unguard;
}
NW_HOOK_METHOD(0x100dfd80, NCRecipeBuyManufactureWnd, OnPaint)

// 0x100e0550 (ret 8; sub_ in the db) - (X, Y) inside the tree button
UBOOL NCRecipeBuyManufactureWnd::HitTree(INT X, INT Y)
{
    return PointInButton(this, TreeButton, X, Y);
}
NW_HOOK_METHOD(0x100e0550, NCRecipeBuyManufactureWnd, HitTree)

// 0x100e0620 (vslot 61) - unlike the manufacture window's, the pressed frame only while the mouse went down on the button
INT NCRecipeBuyManufactureWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if (HitTree((SWORD)(lParam & 0xFFFF), (SWORD)((DWORD)lParam >> 16)) && bTreePressed)
        TreeCurTex = TreeDownTex;
    else
        TreeCurTex = TreeTex;
    return 1;
}
NW_HOOK_METHOD(0x100e0620, NCRecipeBuyManufactureWnd, OnMouseMove)

// 0x100e05e0 (vslot 62)
INT NCRecipeBuyManufactureWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (HitTree((SWORD)(lParam & 0xFFFF), (SWORD)((DWORD)lParam >> 16)))
    {
        bTreePressed = 1;
        TreeCurTex = TreeDownTex;
    }
    return 1;
}
NW_HOOK_METHOD(0x100e05e0, NCRecipeBuyManufactureWnd, OnLButtonDown)

// 0x100ded80 (vslot 63)
INT NCRecipeBuyManufactureWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    TreeCurTex = TreeTex;
    bTreePressed = 0;
    return 1;
}
NW_HOOK_METHOD(0x100ded80, NCRecipeBuyManufactureWnd, OnLButtonUp)

// 0x100dedf0 (vslot 68) - back to the shop's list
INT NCRecipeBuyManufactureWnd::OnPrev()
{
    HideWindow();
    GConsole->RequestRecipeShopSellList(ShopID);
    GConsole->PlayInterfaceSound(SOUND_Close);
    return 1;
}
NW_HOOK_METHOD(0x100dedf0, NCRecipeBuyManufactureWnd, OnPrev)

// vslot 69 - Close: stock body shared with NCOptionWnd::OnCancel (0x1007bee0)
INT NCRecipeBuyManufactureWnd::OnClose()
{
    HideWindow();
    return 1;
}

// 0x100e4a80 (vslot 70) - the tree button, the body the manufacture window shares
INT NCRecipeBuyManufactureWnd::OnShowTree()
{
    return MakingOnShowTree(this);
}
NW_HOOK_METHOD(0x100e4a80, NCRecipeBuyManufactureWnd, OnShowTree)

// 0x100dee30 (vslot 71) - Create: the shop's owner makes the recipe for the player
INT NCRecipeBuyManufactureWnd::OnMake()
{
    GConsole->RequestRecipeShopMakeDo(ShopID, RecipeID, MakeParam);
    return 1;
}
NW_HOOK_METHOD(0x100dee30, NCRecipeBuyManufactureWnd, OnMake)

// 0x100deda0 - the numbers and the shop zeroed (not the outcome field) and the grid emptied
void NCRecipeBuyManufactureWnd::Reset()
{
    RecipeID = 0;
    ProductID = 0;
    ProductCount = 0;
    MPCost = 0;
    SuccessRate = 0;
    CreatorMP = 0;
    CreatorMaxMP = 0;
    ShopID = 0;
    MakeParam = 0;
    IngredientWnd->Clear();
}
NW_HOOK_METHOD(0x100deda0, NCRecipeBuyManufactureWnd, Reset)

// 0x100e0ff0 (ret 8) - the recipe shown, with the number the shop's request carries (nothing when the record is missing or
// has no product)
void NCRecipeBuyManufactureWnd::SetRecipe(NRecipeData* Rec, INT InMakeParam)
{
    if (Rec && Rec->ProductRec)
    {
        MakeParam = InMakeParam;
        MakingSetRecipe(this, Rec);
    }
}
NW_HOOK_METHOD(0x100e0ff0, NCRecipeBuyManufactureWnd, SetRecipe)

// 0x100ded60 is NCRecipeManufactureWnd::AddIngredient's body (hooked there, one function for both windows)
UBOOL NCRecipeBuyManufactureWnd::AddIngredient(NItemEntry* Item)
{
    if (IngredientWnd)
        IngredientWnd->AddIngredient(Item);
    return 1;
}

NW_CHECK_VTABLE(NCRecipeBuyManufactureWnd, 0x10199b38, 72)

/*-----------------------------------------------------------------------------
    NCRecipeGaraTreeWnd - "Recipe Information" (C2: NCRecipeTreeWnd)
-----------------------------------------------------------------------------*/

// 0x100e5b60 - the product entry is constructed by its member; the control, the button and the numbers cleared
NCRecipeGaraTreeWnd::NCRecipeGaraTreeWnd()
{
    TreeCtrl = NULL;
    CloseButton = NULL;
    MPCost = 0;
    SuccessRate = 0;
    Level = 0;
}
NW_HOOK_CTOR(0x100e5b60, NCRecipeGaraTreeWnd)

// 0x100e5c00 - the product's tool tip object goes with it
NW_HOOK_DTOR(0x100e5c00, NCRecipeGaraTreeWnd)

// 0x100e2720 (vslot 25) - the Close button
INT NCRecipeGaraTreeWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_RECIPETREE_CLOSE)
        return OnClose() != 0;
    return 0;
}
NW_HOOK_METHOD(0x100e2720, NCRecipeGaraTreeWnd, WindowProc)

// 0x100e2780 (vslot 68) - the tree control emptied (its vslot 72), the window hidden, with the sound
INT NCRecipeGaraTreeWnd::OnClose()
{
    ::nw::VirtualCall<void>(TreeCtrl, 72);
    HideWindow();
    GConsole->PlayInterfaceSound(SOUND_Close);
    return 1;
}
NW_HOOK_METHOD(0x100e2780, NCRecipeGaraTreeWnd, OnClose)

// 0x100e27b0 - the tree emptied and the numbers zeroed (the recipe windows call it before they fill the tree again)
void NCRecipeGaraTreeWnd::Reset()
{
    ::nw::VirtualCall<void>(TreeCtrl, 72);
    MPCost = 0;
    SuccessRate = 0;
    Level = 0;
}
NW_HOOK_METHOD(0x100e27b0, NCRecipeGaraTreeWnd, Reset)

// 0x100e5c60 (vslot 37) - the title bar (id 0xFF), the background, the tree control (a class of the trees' group: made with
// its stock constructor 0x100E5A00 over its UClass 0x102A62B0) and the Close button
INT NCRecipeGaraTreeWnd::OnCreate()
{
    NCWnd::OnCreate();

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_RECIPETREE_FRAME;
    FrameCtrl->SetPosition(0, 0, 256, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    BackTex = FindTexture(TEXT("L2UI_CH3.RecipeWnd.Recipe5_Back"));

    NCWnd* Ctrl = new NCRecipeGaraTreeCtrl;   // UClass 0x102A62B0, the constructor 0x100E5A00
    TreeCtrl = Ctrl;
    Ctrl->SetPosition(7, 108, 242, 254);
    CreateChild(Ctrl, NWS_VISIBLE);

    CloseButton = new NCButton(TEXT("L2UI_CH3.Button.Btn1_Normal"), TEXT("L2UI_CH3.Button.Btn1_NormalOn"), 0);
    CloseButton->ID = NID_RECIPETREE_CLOSE;
    CloseButton->SetPosition(91, 372, 76, 23);
    CreateChild(CloseButton, NWS_VISIBLE);
    CloseButton->SetTextID(STR_Close);
    return 1;
}
NW_HOOK_METHOD(0x100e5c60, NCRecipeGaraTreeWnd, OnCreate)

// 0x100e4000 (vslot 38) - the tree control and the button go
INT NCRecipeGaraTreeWnd::OnDestroy()
{
    DestroyChild(this, TreeCtrl);
    DestroyChild(this, CloseButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100e4000, NCRecipeGaraTreeWnd, OnDestroy)

// 0x100e4070 (vslot 56) - the background, the product's icon (16, 49), its name and grade, then the lines "MP cost" and
// "Success Rate" as label : value (the label grey, the value tan), "Lv.N" at the right of the second
INT NCRecipeGaraTreeWnd::OnPaint(NCGDevice* Device)
{
    guard(NCRecipeGaraTreeWnd::OnPaint);
    SIZE Size;
    TCHAR Text[68];
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    if (BackTex)
        Device->DrawTexture(0, 20, 256, 381, 0, 0, 256, 381, BackTex, 0xFF, 1);
    if (Product.Icon)
        Device->DrawTexture(16, 49, 32, 32, 0, 0, 32, 32, Product.Icon, 0xFF, 1);

    NCGDevice::GetTextExtent(&Size, Product.Name, 0, 0, 0, 0, 0);
    Device->DrawText(58, 48, 0xFFDCDCDC, Product.Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    if (GConsole->GetGradeText(Product.CrystalGrade))
    {
        appSprintf(Text, TEXT("`%s`"), GConsole->GetGradeText(Product.CrystalGrade));
        Device->DrawText(Size.cx + 60, 48, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 1, 0, 0, 0, 0, 0, 0);
    }

    NCGDevice::GetTextExtent(&Size, *SysString(STR_MPCost), 0, 0, 0, 0, 0);
    Device->DrawText(58, 64, 0xFFA3A3A3, *SysString(STR_MPCost), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    INT SepW = Device->DrawText(Size.cx + 58, 64, 0xFFDCDCDC, TEXT(" : "), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->DrawText(SepW + 58 + Size.cx, 64, 0xFFB09B79, appItoa(MPCost), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);

    NCGDevice::GetTextExtent(&Size, *SysString(STR_SuccessRate), 0, 0, 0, 0, 0);
    Device->DrawText(58, 80, 0xFFA3A3A3, *SysString(STR_SuccessRate), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    SepW = Device->DrawText(Size.cx + 58, 80, 0xFFDCDCDC, TEXT(" : "), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("%d%%"), SuccessRate);
    Device->DrawText(SepW + 58 + Size.cx, 80, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);

    appSprintf(Text, TEXT("Lv.%d"), Level);
    Device->DrawText(214, 77, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100e4070, NCRecipeGaraTreeWnd, OnPaint)

// 0x100e4860 (ret 8) - the recipe's numbers, its product's name (the item's, with its additional name when it has one: the
// console's FormatItemName), icon, description and grade, and the tree control filled by NCRecipeGaraTreeCtrl's AddRecipe
// (0x100E4550) with (product, success rate, 0, 0). Nothing happens when the recipe is unknown or its record is empty.
void NCRecipeGaraTreeWnd::SetRecipe(INT RecipeIndex, INT Unused)
{
    NRecipeData* Rec = NGetRecipeByIndex(RecipeIndex);
    if (Rec && Rec->ProductRec)
    {
        MPCost = Rec->MPCost;
        SuccessRate = Rec->SuccessRate;
        Level = Rec->Level;
        appStrcpy(Product.IconName, *Rec->ProductRec->IconName);
        Product.Icon = FindTexture(Product.IconName);
        if (!Product.Icon)
            Product.Icon = FindTexture(TEXT("NWindow.BlackTexture"));
        NFormatItemName(Product.Name, Rec->ProductID);   // the console's: the name with its additional name
        appStrcpy(Product.Description, *NItemRecordDescription(Rec->ProductRec));
        NItemRecordGrade(Rec->ProductRec, &Product.CrystalGrade);
        if (TreeCtrl)
            ((NCRecipeGaraTreeCtrl*)TreeCtrl)->AddRecipe(Rec->ProductID, Rec->SuccessRate, 0, NULL);   // 0x100E4550
    }
}
NW_HOOK_METHOD(0x100e4860, NCRecipeGaraTreeWnd, SetRecipe)

NW_CHECK_VTABLE(NCRecipeGaraTreeWnd, 0x1019a770, 69)
