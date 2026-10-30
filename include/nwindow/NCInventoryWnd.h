#pragma once
// NCInventoryWnd - the inventory (UClass "CInventoryWnd", 0xF8; the desktop, NCConsole, creates
// it in OnCreate and keeps it at +0xBC): the paperdoll (NCEquipItemWnd) on top, two tab pages
// of items below ("Item" and "Quest", both NCInvenItemWnd: the quest items, NItemInfo::Param
// 3, go to the second), the adena and the weight bar at the bottom, between the crystallize
// icon (shown with the skill) and the trash icon; the item windows light those up while an
// item is carried over them. The console fills it through AddItem / UpdateItem / ClearItems.

#include "nwindow/NCConsole.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NGameData.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCEquipItemWnd;
class NCInvenItemWnd;
class NCTabControl;
struct NItemEntry;
struct NItemInfo;

class NCInventoryWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCInventoryWnd, NCWnd, 0, 0x10283B50)

    // C3 layout (0x258), from the constructor (0x1008db00), OnCreate (0x1008dc70) and the painters
    UTexture* BackTex;               // +0xC8 "L2UI_CH3.InventoryWnd.Inventory_Back"
    UTexture* RecipeTex;             // +0xCC "L2UI_CH3.InventoryWnd.Inventory_recipe"
    UTexture* RecipeDragTex;         // +0xD0 "L2UI_CH3.InventoryWnd.Inventory_recipe_drag"
    UTexture* WeightTex[5];          // +0xD4 "L2UI_CH3.PlayerStatusWnd.PS_weightbar1".."5": the weight bar's colors, up to
                                     //   50%, 66.6%, 80%, 100%, above
    UTexture* TrashTex;              // +0xE8 "L2UI_CH3.InventoryWnd.Inventory_trash"
    UTexture* TrashDragTex;          // +0xEC "L2UI_CH3.InventoryWnd.Inventory_trash_drag"
    UTexture* OutlineTex;            // +0xF0 "L2UI_CH3.Etc.menu_outline"
    UTexture* OutlineDownTex;        // +0xF4 "L2UI_CH3.Etc.menu_outline_down"
    UTexture* CurOutlineTex;         // +0xF8 (name ours) the one of the two over the manufacture button (the pressed one
                                     //   while the mouse is down on it)
    UTexture* AdenaTex;              // +0xFC "L2UI.WindowIcon.WindowIconAdena"
    UTexture* WeightIconTex;         // +0x100 "L2UI.WindowIcon.Weight"
    NCInvenItemWnd* ItemWnd[2];      // +0x104 the pages: 0 "Item", 1 "Quest"
    NCEquipItemWnd* EquipWnd;        // +0x10C
    NCTabControl* TabCtrl;           // +0x110 the two tabs (ID 0xFE001A)
    UBOOL bOverRecipe;               // +0x114 (name ours) an item is carried over the recipe icon (the equipment window's
                                     //   OnMouseMove sets it)
    UBOOL bOverTrash;                // +0x118 (name ours) ... over the trash icon
    UBOOL bManufacturePressed;       // +0x11C (name ours) the mouse is down on the manufacture button
    INT TrashX;                      // +0x120 (208, 351)
    INT TrashY;                      // +0x124
    FLOAT TrashW;                    // +0x128 (34.0)
    FLOAT TrashH;                    // +0x12C (34.0)
    INT RecipeX;                     // +0x130 (14, 351)
    INT RecipeY;                     // +0x134
    FLOAT RecipeW;                   // +0x138 (34.0)
    FLOAT RecipeH;                   // +0x13C (34.0)
    NCButton* ManufactureButton;     // +0x140 "L2UI_CH3.InventoryWnd.Inventory_book", id 0xFE001C, at (54, 352), 32 x 32
    NCButton* TrashButton;           // +0x144 hidden until the player may crystallize
    NCButton* RecipeButton;          // +0x148 shown with the crystallize flag (ShowWindow)
    TCHAR ItemOrderFile[128];        // +0x14C "..\iwo\<name>_<server>.dat" (ReadItemOrder)
    TArray<INT> ItemOrder;           // +0x24C the ids of the "Item" page's items as they stood when the window was last
                                     //   hidden (SaveItemOrder); RestoreItemOrder puts the page back in that order

    NCInventoryWnd();                                                // 0x1008db00
    // ~NCInventoryWnd                                               // 0x1008dc10 (ItemOrder, the base)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10088880
    virtual INT OnCreate();                                          // 37 0x1008dc70
    virtual INT OnDestroy();                                         // 38 0x1008d100
    virtual void ShowWindow();                                       // 44 0x1008d2e0
    virtual void HideWindow();                                       // 45 0x1008cd30
    virtual void Minimize();                                         // 49 0x100888e0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10089ea0
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x1008c970
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x1008c930
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x10088940

    // names ours
    void AddHenna(INT HennaID, INT bUsable);                         // 0x1008d530 the paperdoll's (names ours)
    void ClearHennas();                                              // 0x1008e3b0
    void SetEnchantMode(UBOOL bOn);                                  // 0x1008a3b0 enchant scroll
    void ReloadTexts();                                              // 0x1008a420 names and tool tips
    NItemEntry* FindItem(INT ObjectID);                              // 0x1008a5b0 paperdoll, "Item" page
    void ClearItems();                                               // 0x1008d2b0
    UBOOL AddItem(NItemInfo* Info);                                  // 0x1008ca60
    UBOOL UpdateItem(NItemInfo* Info, INT Change);                   // 0x1008d3c0 1 added, 2 changed,
                                                                     //   3 removed
    virtual INT OnManufacture();                                     // 68 0x10088a20 (name ours) the manufacture button: the recipe book opens
    void SaveItemOrder();                                            // 0x1008cab0 (name ours)
    void MoveLastToFront();                                          // 0x1008ccd0 (name ours) the page's last item first
    void RestoreItemOrder();                                         // 0x1008d540 (name ours)
    void ApplyItemOrder();                                           // 0x1008d720 (name ours) when the page has items
    void ReadItemOrder();                                            // 0x1008cb80 (name ours) the saved file back into ItemOrder
    UBOOL IsInManufactureButton(INT X, INT Y);                       // 0x1008a320 (name ours) edges included
    UBOOL IsInTrash(INT X, INT Y);                                   // 0x10088960 (ret 8; name ours) the trash icon's box, edges included
    UBOOL IsInRecipe(INT X, INT Y);                                  // 0x100889c0 (ret 8; name ours) the recipe icon's box
};
NW_ASSERT_OFFSET(NCInventoryWnd, WeightTex, 0xD4);
NW_ASSERT_OFFSET(NCInventoryWnd, TrashTex, 0xE8);
NW_ASSERT_OFFSET(NCInventoryWnd, CurOutlineTex, 0xF8);
NW_ASSERT_OFFSET(NCInventoryWnd, ItemWnd, 0x104);
NW_ASSERT_OFFSET(NCInventoryWnd, bOverRecipe, 0x114);
NW_ASSERT_OFFSET(NCInventoryWnd, TrashX, 0x120);
NW_ASSERT_OFFSET(NCInventoryWnd, RecipeX, 0x130);
NW_ASSERT_OFFSET(NCInventoryWnd, ManufactureButton, 0x140);
NW_ASSERT_OFFSET(NCInventoryWnd, ItemOrderFile, 0x14C);
NW_ASSERT_OFFSET(NCInventoryWnd, ItemOrder, 0x24C);
NW_ASSERT_SIZE(NCInventoryWnd, 0x258);

// The item table's search by the item id (NDataMap::FindRef, 0x1004dda0): the entry, NULL when
// there is none.
namespace NItemTable
{
    inline NViewItemData* Find(INT* ItemID)
    {
        return GL2GameData.ItemTable->FindRef(*ItemID);
    }
}

// a system line in the desktop's chat window
inline void DesktopSystemMessage(const TCHAR* Text)
{
    GDesktop->AddSystemText(Text, 0, 0xFFFFFFFF);
}

#pragma pack(pop)