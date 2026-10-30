#pragma once
// L2Task - Task's Lineage II skin (UClass "L2Task", 0xE4; C3 only, the messenger): the entry that stands for a
// conversation in L2TaskWnd's list. The same blinking Task as the generic one, with the name of the friend it is
// about (an FString at +0xC8) in front of the other fields, and the blink timer 0xA73.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class L2Task : public NCWnd
{
    NW_DECLARE_CLASS(L2Task, NCWnd, 0, 0x102C1848)

    FString Name;                    // +0xC8 (name ours) the friend: L2TaskWnd looks an entry up by it (0x101255D0)
    NCWnd* ChatWindow;               // +0xD4 (name ours) the conversation window (L2ChatWnd) the entry opens
    INT bAlert;                      // +0xD8 (name ours) a message is waiting: the entry blinks until it is clicked
    INT bBlink;                      // +0xDC (name ours) flipped by the timer every second
    INT TimerHandle;                 // +0xE0 (name ours) what SetTimer(0xA73, 1000) returned

    L2Task();                                                        // 0x10125DE0
    // ~L2Task                                                       // 0x10125E80

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (NControl's body, folded)
    virtual INT OnTimer(INT TimerID);                                // 36 0x10124330
    virtual INT OnCreate();                                          // 37 0x10124300
    virtual INT OnDestroy();                                         // 38 0x10124360
};
NW_ASSERT_OFFSET(L2Task, Name, 0xC8);
NW_ASSERT_OFFSET(L2Task, ChatWindow, 0xD4);
NW_ASSERT_OFFSET(L2Task, bAlert, 0xD8);
NW_ASSERT_OFFSET(L2Task, bBlink, 0xDC);
NW_ASSERT_OFFSET(L2Task, TimerHandle, 0xE0);
NW_ASSERT_SIZE(L2Task, 0xE4);

#pragma pack(pop)
