#pragma once
// NCGodWnd - the screen-sized top window of the game screen (UClass "CGodWnd", 0xDC; C3 only: the console keeps it at
// +0x3950, NConsoleWnd::GodWnd, made last by Initialize): the restart menu, the message line at the bottom, the options
// window and the warning box live in it, so the console shows and hides them through its methods (ShowMessage,
// ShowOptionWnd, ShowRestartMenu, ...).
//
// Layout and virtuals from the stock constructor 0x100861C0, OnCreate 0x10085DD0, OnDestroy 0x100859E0, DefaultPosition
// 0x10085B00, SetWindowSize 0x10085C80 and the small methods 0x10085620 .. 0x10085D20.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCRestartMenuWnd;
class NCMessageWnd;
class NCOptionWnd;
class NCWarningDialogBox;

class NCGodWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGodWnd, NCWnd, 0, 0x10281C70)

    NCWnd* UnknownC8;                // +0xC8 (name ours; 0 from the constructor) OnCreate makes the restart menu only while
                                     //   it is NULL and passes it to the options window's constructor
    NCRestartMenuWnd* RestartMenu;   // +0xCC 128 x 128 in the middle of the screen (hidden); not in GIsOnlyL2Console
    NCMessageWnd* MessageWnd;        // +0xD0 52 high at the bottom, anchored (8)
    NCOptionWnd* OptionWnd;          // +0xD4 256 x 401, style 0x11002, title SysString 146 (hidden)
    NCWarningDialogBox* WarningBox;  // +0xD8 256 x 108 in the middle of the screen (hidden)

    NCGodWnd();                                                      // 0x100861C0
    // ~NCGodWnd                                                     // 0x10086250

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (folded)
    virtual INT OnCreate();                                          // 37 0x10085DD0
    virtual INT OnDestroy();                                         // 38 0x100859E0 (NCWnd's is not called)
    virtual void SetWindowSize(FLOAT W, FLOAT H);                    // 43 0x10085C80 the message line follows the width

    // names from the stock guard strings (the console calls them by address)
    void ReceiveServerList(INT Param);                               // 0x10085620 the message line emptied, the options hidden
    void RestartBegin();                                             // 0x100856B0 the options and the restart menu hidden
    void ShowOptionWnd();                                            // 0x10085740 toggles the options window (sounds 5 / 6)
    void ShowMessage(const TCHAR* Text);                             // 0x10085810 the line's text (when not empty), shown
    void HideRestartMenu();                                          // 0x100858E0
    void ShowRestartMenu();                                          // 0x10085960
    void DefaultPosition();                                          // 0x10085B00 the windows back in their places
    void ShowWarningMessage(const TCHAR* Text, INT bExit);           // 0x10085D20 the box raised modal with the text
};
NW_ASSERT_OFFSET(NCGodWnd, RestartMenu, 0xCC);
NW_ASSERT_OFFSET(NCGodWnd, OptionWnd, 0xD4);
NW_ASSERT_OFFSET(NCGodWnd, WarningBox, 0xD8);
NW_ASSERT_SIZE(NCGodWnd, 0xDC);

#pragma pack(pop)
