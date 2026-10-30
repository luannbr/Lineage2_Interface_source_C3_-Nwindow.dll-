#pragma once
// ChatFrameCtrl - the title bar of a conversation window (UClass "ChatFrameCtrl", 0x238; C3 only, the messenger). A
// frame control whose buttons do not act on the window but tell the parent (NWM_COMMAND with the frame's minimize /
// close ids), and whose bar shows "<title> - <SysString 1002>" cut to the width.

#include "nwindow/NCFrameCtrl.h"
#pragma pack(push, 4)

class ChatFrameCtrl : public NCFrameCtrl
{
    NW_DECLARE_CLASS(ChatFrameCtrl, NCFrameCtrl, 0, 0x102BBBA0)

    ChatFrameCtrl();                                                 // 0x1011DB50
    ChatFrameCtrl(INT InDecorated, INT InVertical, INT InSmall);     // 0x1011DBF0 (anonymous in the db) (ret 0xC)
    // ~ChatFrameCtrl                                                // 0x1011DBA0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x10119AA0 (shared with L2ChatFrameCtrl)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x1011AB40
    virtual INT OnClose();                                           // 70 0x10124210 the parent hears NID_FRAME_CLOSE (shared)
    virtual INT OnMinimize();                                        // 71 0x101241E0 the parent hears NID_FRAME_MINIMIZE (shared)
};
NW_ASSERT_SIZE(ChatFrameCtrl, 0x238);

#pragma pack(pop)
