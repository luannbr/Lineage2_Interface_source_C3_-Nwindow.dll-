#pragma once
// NCComboBox - drop-down list (UClass "CComboBox", 0x1F8). Closed, it shows the selected item
// and a drop button (id 0x100B); open, the control itself grows to hold the item list and
// a click picks the item under the mouse (NWM_COMMAND, lParam = index). Losing the focus
// closes it. Row height = height of L"\xAC00" (Hangul GA) in the default font.

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NString;
class NCButton;

enum { NID_COMBO_DROP = 0x100B };

class NCComboBox : public NControl
{
    NW_DECLARE_CLASS(NCComboBox, NControl, 0, 0x10263860)

    // C3 layout (constructor 0x10005b20, OnCreate 0x10005570, OnPaint 0x10004f50): the item list is the first member
    INT TextX;                       // +0x1D4 (5)
    INT TextY;                       // +0x1D8 (0)
    TArray<NString*> Items;          // +0x1DC the item texts (the box owns them)
    UTexture* LeftTex;               // +0x1E8 "L2UI_CH3.Etc.textbox1" the closed box: left cap, middle, right cap
    UTexture* MidTex;                // +0x1EC "L2UI_CH3.Etc.textbox2"
    UTexture* RightTex;              // +0x1F0 "L2UI_CH3.Etc.textbox3"
    UTexture* LeftDisabledTex;       // +0x1F4 "..._disable" the same while disabled
    UTexture* MidDisabledTex;        // +0x1F8
    UTexture* RightDisabledTex;      // +0x1FC
    UTexture* SelectTex;             // +0x200 "L2UI_CH3.ListCtrl.TextSelect" the row under the mouse in the open list
    UTexture* SelectTex2;            // +0x204 "L2UI_CH3.ListCtrl.TextSelect2" (its right end)
    NCButton* DropButton;            // +0x208 id 0x100B
    UBOOL bDropped;                  // +0x20C
    INT ClosedW;                     // +0x210 size when closed (OnCreate: (INT) of the local rectangle)
    INT ClosedH;                     // +0x214
    SIZE RowSize;                    // +0x218 extent of "A": cy centers the text in a row
    INT Selected;                    // +0x220 -1 = none
    INT Hover;                       // +0x224 item under the mouse in the open list
    INT RowStep;                     // +0x228 (17) height of a row of the open list (name ours)

    NCComboBox();                                                    // 0x10005b20
    // ~NCComboBox                                                   // 0x10005bd0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10004ce0
    virtual void KillFocus();                                        // 30 0x10004e60
    virtual INT OnCreate();                                          // 37 0x10005570
    virtual INT OnDestroy();                                         // 38 0x10005740
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x10004f50
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 61 0x10004d40
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 62 0x10004df0
    virtual INT DropDown();                                          // 69 0x10005500 (name ours)

    // names ours
    void SetSelected(INT Index);                                     // 0x10004ea0
    void UpdateSize();                                               // 0x10004eb0
    const TCHAR* GetItemText(INT Index);                             // 0x100054a0
    void ClearItems();                                               // 0x10005840
    void AddItem(INT SysStringID);                                   // 0x100058b0
    void AddItem(const TCHAR* Text);                                 // 0x10005970
};
NW_ASSERT_OFFSET(NCComboBox, Items, 0x1DC);
NW_ASSERT_OFFSET(NCComboBox, LeftTex, 0x1E8);
NW_ASSERT_OFFSET(NCComboBox, SelectTex2, 0x204);
NW_ASSERT_OFFSET(NCComboBox, DropButton, 0x208);
NW_ASSERT_OFFSET(NCComboBox, RowSize, 0x218);
NW_ASSERT_OFFSET(NCComboBox, Hover, 0x224);
NW_ASSERT_OFFSET(NCComboBox, RowStep, 0x228);
NW_ASSERT_SIZE(NCComboBox, 0x22C);

#pragma pack(pop)