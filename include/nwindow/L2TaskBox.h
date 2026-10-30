#pragma once
// L2TaskBox - TaskBox's Lineage II skin (UClass "L2TaskBox", 0x22C; C3 only, the messenger): the list of L2Task entries.
// The same 140 x 21 buttons (the three textures of TaskBox), without the line icons, the name at +3; a line's Param is
// its L2Task window. Its AddString takes the Param before the color.

#include "nwindow/NCListBox.h"
#pragma pack(push, 4)

class NCWnd;

class L2TaskBox : public NCListBox
{
    NW_DECLARE_CLASS(L2TaskBox, NCListBox, 0, 0x102C2290)

    UTexture* NormalTex;             // +0x220 (name ours) "L2UI_CH3.Msn.msnbutton" (OnCreate, TaskBox's body)
    UTexture* SelectedTex;           // +0x224 "L2UI_CH3.Msn.msnbutton_down"
    UTexture* AlertTex;              // +0x228 "L2UI_CH3.Msn.msnbutton_light"

    L2TaskBox();                                                     // 0x10125FA0
    L2TaskBox(INT InMaxLines, INT InVisibleLines, INT InUnknown20C); // 0x10126070 (anonymous in the db) (ret 0xC)
    // ~L2TaskBox                                                    // 0x10126020

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10124390 (TaskBox's, shared)
    virtual INT OnCreate();                                          // 37 0x10124720 (TaskBox's, shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10124780
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x10124990 (the db calls it OnLButtonDown)
    virtual INT HitTest(INT X, INT Y);                               // 69 0x10119EA0 (TaskBox's, shared)
    // 70 0x10124EB0 ret 0xC: the second argument is the line's Param and the third its color (the opposite of the base)
    virtual void AddString(const TCHAR* Text, DWORD Param, INT Color);
};
NW_ASSERT_OFFSET(L2TaskBox, NormalTex, 0x220);
NW_ASSERT_OFFSET(L2TaskBox, AlertTex, 0x228);
NW_ASSERT_SIZE(L2TaskBox, 0x22C);

#pragma pack(pop)
