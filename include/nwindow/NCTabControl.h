#pragma once
// NCTabControl - row of tab buttons, each showing one page window (UClass "CTabControl",
// 0x1C4). The tabs are NCRadioButtons whose ID is the control's GroupID and whose Value is
// the tab index: a click comes back as NWM_COMMAND(GroupID, index) and selects that page.

#include "nwindow/NControl.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCRadioButton;

#if NW_HOOK_MODE
#define GNextTabGroupID NW_GLOBAL(INT, 0x101F002C)   // starts at 0xFA0000 (name ours)
#else
extern INT GNextTabGroupID;
#endif

class NCTabControl : public NControl
{
    NW_DECLARE_CLASS(NCTabControl, NControl, 0, 0x1026F910)

    INT NextTabX;                    // +0x1D4 X of the next tab button
    TArray<NCRadioButton*> Tabs;     // +0x1D8
    INT GroupID;                     // +0x1E4 ID of the tab buttons (OnCreate)
    NCWnd* CurrentPage;              // +0x1E8
    TArray<NCWnd*> Pages;            // +0x1EC one per tab (not owned)

    NCTabControl();                                                  // 0x1002aae0
    // ~NCTabControl                                                 // 0x1002ab80

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1002a160
    virtual INT OnCreate();                                          // 37 0x1002a1c0
    virtual INT OnDestroy();                                         // 38 0x1002a910
    virtual void ShowCurrentPage();                                  // 69 0x1002a1e0 (name ours)
    virtual INT SelectTab(INT Index);                                // 70 0x1002a260 (name ours)

    // names ours; a tab of W x H 0 is sized to its label
    INT AddTab(INT TextID, NCWnd* Page, INT W, INT H, const TCHAR* NormalTexName,
               const TCHAR* PushedTexName, INT Gap, INT ToolTipID);  // 0x10022490
    INT AddTab(const TCHAR* Text, NCWnd* Page, INT W, INT H, const TCHAR* NormalTexName,
               const TCHAR* PushedTexName, INT Gap, INT ToolTipID);  // 0x100226d0
    void SetTab(INT Index, BYTE bNotify = 1);                        // 0x1002a350 silent SelectTab (C3: the owner is told only when bNotify)
    TArray<NCRadioButton*> GetTabs();                                // 0x1002AAB0 (ret 4, the hidden return pointer)
};
NW_ASSERT_OFFSET(NCTabControl, Tabs, 0x1D8);
NW_ASSERT_OFFSET(NCTabControl, GroupID, 0x1E4);
NW_ASSERT_OFFSET(NCTabControl, Pages, 0x1EC);
NW_ASSERT_SIZE(NCTabControl, 0x1F8);

#pragma pack(pop)