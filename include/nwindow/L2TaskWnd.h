#pragma once
// L2TaskWnd - TaskWnd's Lineage II skin (UClass "L2TaskWnd", 0xEC; C3 only, the messenger): the same narrow list window,
// its list an L2TaskBox and its entries L2Tasks (the map goes from the conversation window (L2ChatWnd) to its L2Task).

#include "nwindow/TaskWnd.h"
#pragma pack(push, 4)

class L2TaskBox;

class L2TaskWnd : public NCWnd
{
    NW_DECLARE_CLASS(L2TaskWnd, NCWnd, 0, 0x102C0DF8)

    NTaskMap Tasks;                  // +0xC8 (name ours) conversation window -> its L2Task window
    L2TaskBox* List;                 // +0xDC 140 x 126 at (14, 3), 6 lines shown
    UTexture* BackTex1;              // +0xE0 "L2UI_CH3.msn6_back1"
    UTexture* BackTex2;              // +0xE4 "L2UI_CH3.msn6_back2"
    UTexture* BackTex3;              // +0xE8 "L2UI_CH3.msn6_back3"

    L2TaskWnd();                                                     // 0x101263F0
    // ~L2TaskWnd                                                    // 0x10126490

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119F60 (NControl's body, folded)
    virtual INT OnCreate();                                          // 37 0x101260C0
    virtual INT OnDestroy();                                         // 38 0x101254A0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1011B110 (TaskWnd's, shared)

    // names ours (anonymous in the db)
    void RemoveTask(NCWnd* ChatWindow);                              // 0x10125580 (ret 4) the L2Task of that conversation window goes
    NCWnd* FindTask(FString Name);                                   // 0x101255D0 (ret 0xC) the L2Task of that friend, or NULL
};
NW_ASSERT_OFFSET(L2TaskWnd, Tasks, 0xC8);
NW_ASSERT_OFFSET(L2TaskWnd, List, 0xDC);
NW_ASSERT_OFFSET(L2TaskWnd, BackTex1, 0xE0);
NW_ASSERT_SIZE(L2TaskWnd, 0xEC);

#pragma pack(pop)
