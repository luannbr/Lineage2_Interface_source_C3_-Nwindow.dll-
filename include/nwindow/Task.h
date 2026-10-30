#pragma once
// Task - one entry of the messenger's task list (UClass "Task", 0xDC; C3 only): the invisible window that stands for a
// conversation window in TaskWnd's list. It blinks while the conversation has an unread message: a one-second timer
// toggles bBlink, and TaskBox paints the entry lit while both flags are on. Clicking the entry in the list opens the
// conversation (TaskBox::OnLButtonUp).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class Task : public NCWnd
{
    NW_DECLARE_CLASS(Task, NCWnd, 0, 0x102BD038)

    INT Unknown_C8;                  // +0xC8 (name ours) cleared by the constructor, never used
    NCWnd* ChatWindow;               // +0xCC (name ours) the conversation window (ChatWnd) the entry opens
    INT bAlert;                      // +0xD0 (name ours) a message is waiting: the entry blinks until it is clicked
    INT bBlink;                      // +0xD4 (name ours) flipped by the timer every second
    INT TimerHandle;                 // +0xD8 (name ours) what SetTimer(0xA15, 1000) returned; killed by OnDestroy

    Task();                                                          // 0x1011E6A0
    // ~Task                                                         // 0x1011E710

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (NControl's body, folded)
    virtual INT OnTimer(INT TimerID);                                // 36 0x10119E40
    virtual INT OnCreate();                                          // 37 0x10119E10
    virtual INT OnDestroy();                                         // 38 0x10119E70
};
NW_ASSERT_OFFSET(Task, ChatWindow, 0xCC);
NW_ASSERT_OFFSET(Task, bAlert, 0xD0);
NW_ASSERT_OFFSET(Task, bBlink, 0xD4);
NW_ASSERT_OFFSET(Task, TimerHandle, 0xD8);
NW_ASSERT_SIZE(Task, 0xDC);

#pragma pack(pop)
