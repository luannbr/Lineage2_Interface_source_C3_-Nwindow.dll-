/*=============================================================================
    NCRecipeConsoleOps.cpp - the console method only the recipe windows call (see NCRecipeConsoleOps.h).
    Stock: 0x1005ff00.
=============================================================================*/
#include "nwindow/NCRecipeConsoleOps.h"
#include "nwindow/NRecipeData.h"

// the table's name for "no additional name"
#define NAME_NONE_TEXT TEXT("NAME_None")

// 0x1005ff00 (ret 8) - "Name-Additional", or the name alone; nothing is written when the item has no name
void NRecipeConsoleOps::FormatItemName(TCHAR* Out, INT ItemID)
{
    TCHAR* Name = GL2GameData.GetItemName(ItemID);
    TCHAR* Additional = GL2GameData.GetItemAdditionalName(ItemID);
    if (Name)
    {
        if (Additional && appStrcmp(Additional, NAME_NONE_TEXT) != 0)
        {
            appSprintf(Out, TEXT("%s-%s"), Name, Additional);
            return;
        }
        appSprintf(Out, TEXT("%s"), Name);
    }
}
NW_HOOK_METHOD(0x1005ff00, NRecipeConsoleOps, FormatItemName)

// (MakeItemInfo, 0x10073aa0, is NConsoleWnd's: NConsoleWndItems.cpp hooks it and NMakeItemInfo calls it on the console)
