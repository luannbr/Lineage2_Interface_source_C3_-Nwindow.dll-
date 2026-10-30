/*=============================================================================
    Task.cpp - an entry of the messenger's task list (C3 only; C3 addresses). Stock: OnCreate / OnTimer / OnDestroy
    0x10119e10 .. 0x10119e9f and the constructor / destructor 0x1011e6a0 .. 0x1011e75f.
=============================================================================*/
#include "nwindow/Task.h"

enum { TIMER_TASK_BLINK = 0xA15 };

// 0x1011e6a0 - the base window's constructor, the five fields empty
Task::Task()
{
    Unknown_C8 = 0;
    ChatWindow = NULL;
    bAlert = 0;
    bBlink = 0;
    TimerHandle = 0;
}
NW_HOOK_CTOR(0x1011E6A0, Task)

// 0x1011e710
NW_HOOK_DTOR(0x1011E710, Task)

// 0x10119f60 (vslot 25) - NControl's body, shared (hooked there)
INT Task::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10119e10 (vslot 37) - the blink timer: once a second
INT Task::OnCreate()
{
    NCWnd::OnCreate();
    TimerHandle = SetTimer(TIMER_TASK_BLINK, 1000, 0);
    return 1;
}
NW_HOOK_METHOD(0x10119E10, Task, OnCreate)

// 0x10119e40 (vslot 36) - the blink flag flips
INT Task::OnTimer(INT TimerID)
{
    if (TimerID == TIMER_TASK_BLINK)
        bBlink = (bBlink == 0);
    return 1;
}
NW_HOOK_METHOD(0x10119E40, Task, OnTimer)

// 0x10119e70 (vslot 38) - the timer goes, then the base
INT Task::OnDestroy()
{
    if (TimerHandle)
    {
        KillTimer(TimerHandle);
        TimerHandle = 0;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10119E70, Task, OnDestroy)

NW_CHECK_VTABLE(Task, 0x101A7600, 68)
