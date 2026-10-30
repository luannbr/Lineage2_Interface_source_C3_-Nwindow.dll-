/*=============================================================================
    NItemInfo.cpp - an item / skill / action icon as the item windows get it.
    Stock (C3): 0x10047400 .. 0x10047530.
=============================================================================*/
#include "nwindow/NItemInfo.h"

// 0x10047400 - the stores in the stock order (C3: six strings, the ints from +0x1700, the word at +0x1790, the new
// dword at +0x178C last)
NItemInfo::NItemInfo()
{
    Name[0] = 0;
    AdditionalName[0] = 0;
    IconName[0] = 0;
    Unknown200[0] = 0;
    Unknown300[0] = 0;
    Description[0] = 0;
    Icon = NULL;
    UnknownC04 = 0;
    UnknownC08 = 0;
    Param = -1;
    Unknown1710 = -1;
    ObjectID = 0;
    ID = 0;
    Count = 0;
    UnknownC1C = 0;
    UnknownC20 = -1;
    BodyPart = 0;
    UnknownC28 = 0;
    UnknownC2C = -1;
    UnknownC30 = -1;
    for (INT i = 0; i < 13; i++)
        UnknownC34[i] = 0;
    SubType = -1;
    UnknownC6C = 0;
    UnknownC70 = 0;
    Enchant = 0;
    UnknownC84 = 0;
    bStackable = 0;
    UnknownC8C = 0;
    UnknownC90 = -1;
    UnknownC94 = 0;
    Unknown17A0 = 0;
    UnknownC78 = 0;
    UnknownC7C = 0;
    UnknownC80 = 0;
    UnknownC98 = 0;
    Unknown1788 = 0;
    New178C = -1;
}
NW_HOOK_CTOR(0x10047400, NItemInfo)
