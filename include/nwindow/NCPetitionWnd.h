#pragma once
// NCPetitionWnd - the player's petition window (UClass "CPetitionWnd", 0xA8; NConsoleWnd
// +0x36EC): the conversation with the GM (NCPetitionListBox), a line to write in (Enter sends
// it as chat type 6) and "Cancel petition" (sysstring 473). NCGMPetitionWnd is the GM's side
// (desktop +0xAC, the same layout): its line takes "//" builder commands too, and "Start" /
// "End" (474 / 475) send the builder commands startpet / endpet. The desktop adds the
// server's petition chat to them (AddMessage).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCEditBox;
class NCPetitionListBox;
class NCButton;

// NWM_COMMAND ids of the two windows' controls
enum
{
    NID_PETITION_LIST = 0xF0A0B1,
    NID_PETITION_EDIT = 0xF0A0B2,
    NID_PETITION_SEND = 0xF0A0B3,      // no such control (the command does nothing)
    NID_PETITION_CANCEL = 0xF0A0B4,
    NID_PETITION_FRAME = 0xF0A0B5,
    NID_PETITION_NEWBUTTON = 0xF0ABB6, // C3
    NID_GMPETITION_LIST = 0xF0A0C1,
    NID_GMPETITION_EDIT = 0xF0A0C2,
    NID_GMPETITION_START = 0xF0A0C3,
    NID_GMPETITION_END = 0xF0A0C4,
    NID_GMPETITION_FRAME = 0xF0A0C5,
};

class NCPetitionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetitionWnd, NCWnd, 0, 0x1029DCE0)

    UTexture* BackTex;                     // +0xC8 "L2UI.etcwnd.Petitionback"
    NCEditBox* EditBox;                    // +0xCC
    NCPetitionListBox* ListBox;            // +0xD0
    NCButton* UnusedA0;                    // +0xD4 (NCGMPetitionWnd's Start button; not made here)
    NCButton* CancelButton;                // +0xD8
    NCButton* NewButton;                   // +0xDC (new in C3, name ours) 0xF0ABB6, text 849, shown disabled until the
                                           //   petition is under way (cancel disables it, HideWindow disables it)

    NCPetitionWnd();                                                 // 0x10077970
    // ~NCPetitionWnd                                                // 0x10057c30 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100d0180
    virtual INT OnCreate();                                          // 37 0x100d2cd0
    virtual INT OnDestroy();                                         // 38 0x100d0af0
    virtual void ShowWindow();                                       // 44 0x100aa860 (folded)
    virtual void HideWindow();                                       // 45 0x100d0d50 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100d0c20
    // names ours
    virtual INT OnEdit(LPARAM lParam);                               // 68 0x100d0cc0 1: Enter
    virtual INT OnSend() { return 1; }                               // 69 0x10005fa0 (folded)
    virtual INT OnCancel();                                          // 70 0x100d0d70 disables NewButton, asks the server to cancel
    virtual INT OnNewButton();                                       // 71 0x100d0240 (C3, name ours) shows the console's warning box

    // names ours (also called on the desktop's NCGMPetitionWnd)
    void AddMessage(TCHAR* Text, DWORD Color, INT Type, INT Param);  // 0x100d0470
    void Clear();                                                    // 0x100d04b0
};
NW_ASSERT_OFFSET(NCPetitionWnd, EditBox, 0xCC);
NW_ASSERT_OFFSET(NCPetitionWnd, CancelButton, 0xD8);
NW_ASSERT_SIZE(NCPetitionWnd, 0xE0);

class NCGMPetitionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGMPetitionWnd, NCWnd, 0, 0x1029EC48)

    UTexture* BackTex;                     // +0xC8 "L2UI.etcwnd.Petitionback"
    NCEditBox* EditBox;                    // +0xCC
    NCPetitionListBox* ListBox;            // +0xD0
    NCButton* StartButton;                 // +0xD4
    NCButton* EndButton;                   // +0xD8

    NCGMPetitionWnd();                                               // 0x10056680
    // ~NCGMPetitionWnd                                              // 0x1004c650 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100d0260
    virtual INT OnCreate();                                          // 37 0x100d3080
    virtual INT OnDestroy();                                         // 38 0x100d0da0
    virtual void ShowWindow();                                       // 44 0x100aa860 (folded)
    virtual void HideWindow();                                       // 45 0x100412d0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100d0c20 (folded)
    // names ours
    virtual INT OnEdit(LPARAM lParam);                               // 68 0x100d0300 1: Enter
    virtual INT OnStart();                                           // 69 0x100d04d0
    virtual INT OnEnd();                                             // 70 0x100d0550

    // names ours: NCPetitionWnd's bodies
    void AddMessage(TCHAR* Text, DWORD Color, INT Type, INT Param);  // 0x100d0470 (folded)
    void Clear();                                                    // 0x100d04b0 (folded)
};
NW_ASSERT_OFFSET(NCGMPetitionWnd, EditBox, 0xCC);
NW_ASSERT_OFFSET(NCGMPetitionWnd, EndButton, 0xD8);
NW_ASSERT_SIZE(NCGMPetitionWnd, 0xDC);

#pragma pack(pop)