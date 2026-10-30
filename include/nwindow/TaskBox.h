#pragma once
// TaskBox - the list of open conversations (UClass "TaskBox", 0x22C; C3 only, the messenger): one 140 x 21 button per
// line, in TaskWnd. A line's Param is its Task window; the button is lit while the Task blinks (a message waits), and a
// click opens the conversation. Shares its OnCreate, WindowProc and HitTest with L2TaskBox (identical code).

#include "nwindow/NCListBox.h"
#pragma pack(push, 4)

class NCWnd;

class TaskBox : public NCListBox
{
    NW_DECLARE_CLASS(TaskBox, NCListBox, 0, 0x102BC0C0)

    UTexture* NormalTex;             // +0x220 (name ours) "L2UI_CH3.Msn.msnbutton" (OnCreate)
    UTexture* SelectedTex;           // +0x224 "L2UI_CH3.Msn.msnbutton_down": the line under the mouse button
    UTexture* AlertTex;              // +0x228 "L2UI_CH3.Msn.msnbutton_light": a blinking Task, lit phase

    TaskBox();                                                       // 0x1011E7A0
    TaskBox(INT InMaxLines, INT InVisibleLines, INT InUnknown20C);   // 0x1011E870 (anonymous in the db) (ret 0xC)
    // ~TaskBox                                                      // 0x1011E820

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10124390 (shared with L2TaskBox)
    virtual INT OnCreate();                                          // 37 0x10124720 (shared with L2TaskBox)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1011ADA0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 63 0x1011AFE0 (the db calls it OnLButtonDown)
    virtual INT HitTest(INT X, INT Y);                               // 69 0x10119EA0 21-pixel lines (shared)

    // names ours
    void UpdateString(const TCHAR* Text, NCWnd* TaskWindow);         // 0x1011B060 (ret 8) the line of the Task window gets this text
};
NW_ASSERT_OFFSET(TaskBox, NormalTex, 0x220);
NW_ASSERT_OFFSET(TaskBox, AlertTex, 0x228);
NW_ASSERT_SIZE(TaskBox, 0x22C);

#pragma pack(pop)
