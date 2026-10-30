#pragma once
// NCXMasSealWnd - a small window of the Christmas event (UClass "CXMasSealWnd", 0xA4; C2 only,
// NCConsole::OnCreate makes it): a "MinimapBack" panel with a title bar, and a second picture
// drawn over it that nothing in this class sets (the console fills it).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCXMasSealWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCXMasSealWnd, NCWnd, 0, 0x102B6E48)

    INT Unknown98;                   // +0xC8 (0) cleared by the constructor; SetItem keeps the item's id here
    UTexture* BackTex;               // +0xCC "L2UI.MinimapWnd.MinimapBack"
    UTexture* SealTex;               // +0xD0 (name ours) drawn at (3, 22) over the back; set from outside

    NCXMasSealWnd();                                                 // 0x1010b270
    // ~NCXMasSealWnd                                                // 0x1010b2c0

    virtual INT OnCreate();                                          // 37 0x1010b170
    virtual INT OnDestroy();                                         // 38 0x1010b020
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1010b0a0

    void SetItem(INT ItemID);                                        // 0x1010b230 (name ours) the seal's item: its picture
};
NW_ASSERT_OFFSET(NCXMasSealWnd, BackTex, 0xCC);
NW_ASSERT_OFFSET(NCXMasSealWnd, SealTex, 0xD0);
NW_ASSERT_SIZE(NCXMasSealWnd, 0xD4);

#pragma pack(pop)
