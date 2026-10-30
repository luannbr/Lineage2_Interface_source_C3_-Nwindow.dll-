#pragma once
// NCRecipeBuyManufactureWnd - "Item Creation" for an offer of a recipe shop (UClass "CRecipeBuyManufactureWnd", 0x1B04):
// NCRecipeManufactureWnd's window (same product, ingredients, creator MP bar, outcome line and buttons; see that header for
// the common fields) for a customer of a recipe shop: Create asks the shop's owner to make the item
// (RequestRecipeShopMakeDo(ShopID, RecipeID)) and <PREV goes back to the shop's list (RequestRecipeShopSellList(ShopID)).
// The console opens it (NConsoleWnd::vfn131 0x10059330, the answer of RequestRecipeShopMakeInfo): the buy list hidden,
// Reset, the shop id, the creator's MP, the outcome, SetRecipe, the window placed where the list was, shown and focused.
//
// Differences from NCRecipeManufactureWnd (C3 layout from the constructor 0x100E2260 and the diff of the two OnCreate /
// OnPaint / SetRecipe bodies, which are identical but for these offsets): two new fields (+0x1B30 ShopID, +0x1B34 a number
// SetRecipe is given that the Create request carries) push the outcome to +0x1B38 and the four buttons to +0x1B3C.., the
// pressed flag of the tree button is at +0x1B4C, and the close button's id is 0xF8, not 0xFD.
//
// Desktop (NCConsole) +0x148.

#include "nwindow/NCRecipeManufactureItemWnd.h"
#include "nwindow/NCRecipeManufactureWnd.h"
#include "nwindow/NRecipeData.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;

enum { NID_RECIPEBUYMAKE_CLOSE = 0xF8 };   // the close button (NCRecipeManufactureWnd's is 0xFD)

class NCRecipeBuyManufactureWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCRecipeBuyManufactureWnd, NCWnd, 0, 0x102A3458)

    NCRecipeManufactureItemWnd* IngredientWnd;  // +0xC8
    UTexture* BackTex;               // +0xCC
    UTexture* MPTex;                 // +0xD0
    UTexture* TreeTex;               // +0xD4 (see NCRecipeManufactureWnd.h)
    UTexture* TreeDownTex;           // +0xD8
    UTexture* TreeCurTex;            // +0xDC
    NItemEntry Product;              // +0xE0
    INT RecipeID;                    // +0x1B14
    INT ProductID;                   // +0x1B18
    INT ProductCount;                // +0x1B1C
    INT MPCost;                      // +0x1B20
    INT SuccessRate;                 // +0x1B24
    INT CreatorMP;                   // +0x1B28
    INT CreatorMaxMP;                // +0x1B2C
    INT ShopID;                      // +0x1B30 the shop's owner (the console writes it; the requests take it first)
    INT MakeParam;                   // +0x1B34 (name ours) SetRecipe's second argument; RequestRecipeShopMakeDo's third. Zeroed by the
                                     //   constructor and Reset
    INT Result;                      // +0x1B38 (-1) the last try: 0 failed, 1 succeeded
    NCButton* TreeButton;            // +0x1B3C id 0xFE
    NCButton* MakeButton;            // +0x1B40 id 0xF9
    NCButton* PrevButton;            // +0x1B44 id 0xFA
    NCButton* CloseButton;           // +0x1B48 id 0xF8
    UBOOL bTreePressed;              // +0x1B4C (name ours; new in C3) the mouse went down on the tree button

    NCRecipeBuyManufactureWnd();                                     // 0x100e2260
    // ~NCRecipeBuyManufactureWnd                                    // 0x100e2350

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100decb0
    virtual INT OnCreate();                                          // 37 0x100e0b80
    virtual INT OnDestroy();                                         // 38 0x100dfc90
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100dfd80
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x100E0620
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x100E05E0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x100DED80
    // names ours
    virtual INT OnPrev();                                            // 68 0x100dedf0 hides the window, asks for the shop's list
    virtual INT OnClose();                                           // 69 0x1007bee0 (NCOptionWnd::OnCancel's body)
    virtual INT OnShowTree();                                        // 70 0x100e4a80 (hooked here; NCRecipeManufactureWnd shares it)
    virtual void NewVirtual71();  // 71 0x100dee30 (only this client has it; defined in NNewVirtuals.gen.cpp)
    INT OnMake();                                            // 0x100DEE30 (slot 71 is NewVirtual71, which jumps to it) RequestRecipeShopMakeDo(ShopID, RecipeID, MakeParam)

    // names ours
    void Reset();                                                    // 0x100deda0 the numbers zeroed (not the outcome), the grid emptied
    void SetRecipe(NRecipeData* Rec, INT InMakeParam);               // 0x100E0FF0 (NCRecipeManufactureWnd::SetRecipe's code, ret 8)
    __declspec(noinline) UBOOL AddIngredient(NItemEntry* Item);      // 0x100DED60 (NCRecipeManufactureWnd's, shared body)
    UBOOL HitTree(INT X, INT Y);                                     // 0x100E0550 (ret 8; name ours)
};
NW_ASSERT_OFFSET(NCRecipeBuyManufactureWnd, TreeCurTex, 0xDC);
NW_ASSERT_OFFSET(NCRecipeBuyManufactureWnd, Product, 0xE0);
NW_ASSERT_OFFSET(NCRecipeBuyManufactureWnd, ShopID, 0x1B30);
NW_ASSERT_OFFSET(NCRecipeBuyManufactureWnd, Result, 0x1B38);
NW_ASSERT_OFFSET(NCRecipeBuyManufactureWnd, TreeButton, 0x1B3C);
NW_ASSERT_OFFSET(NCRecipeBuyManufactureWnd, CloseButton, 0x1B48);
NW_ASSERT_OFFSET(NCRecipeBuyManufactureWnd, bTreePressed, 0x1B4C);
NW_ASSERT_SIZE(NCRecipeBuyManufactureWnd, 0x1B50);

#pragma pack(pop)
