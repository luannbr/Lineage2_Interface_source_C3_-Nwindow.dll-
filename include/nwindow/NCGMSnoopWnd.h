#pragma once
// NCGMSnoopWnd - one conversation a GM listens to (UClass "CGMSnoopWnd", 0xC4; C2 only): the
// petition window's look (title bar, the "Petitionback" panel, a conversation list) with seven
// check boxes under it (which kinds of chat are shown) and a close button that tells the
// server the snoop is over (NCGMSnoopManager keeps four of them).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCCheckBox;
class NCPetitionListBox;

class NCGMSnoopWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGMSnoopWnd, NCWnd, 0, 0x1029CD70)

    UTexture* BackTex;               // +0xC8 "L2UI.etcwnd.Petitionback"
    NCPetitionListBox* List;         // +0xCC the conversation (name ours)
    NCButton* CloseButton;           // +0xD0 text 141 (name ours)
    NCCheckBox* Filters[7];          // +0xD4 the chat kinds shown, all checked (name ours)
    INT SnoopID;                     // +0xF0 what the server calls this snoop, -1 = none (name ours)

    NCGMSnoopWnd();                                                  // 0x100d2b60
    // ~NCGMSnoopWnd                                                 // 0x100b6bc0 (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x100d05d0
    virtual INT OnCreate();                                          // 37 0x100d3420
    virtual INT OnDestroy();                                         // 38 0x100d0ed0
    virtual INT OnPaint(NCGDevice* Device);                          // 56 0x100d0fe0 (NCPetitionWnd's body)
    virtual INT OnClose();                                           // 68 0x100d0650 (name ours)

    // name ours
    __declspec(noinline) void Reset();                               // 0x100d0630 the list emptied, no snoop
    void AddLine(const TCHAR* Text, DWORD Color, INT Type, INT Unused); // 0x100d1080 a line of the chat, when its kind is shown
};
NW_ASSERT_OFFSET(NCGMSnoopWnd, BackTex, 0xC8);
NW_ASSERT_OFFSET(NCGMSnoopWnd, List, 0xCC);
NW_ASSERT_OFFSET(NCGMSnoopWnd, Filters, 0xD4);
NW_ASSERT_OFFSET(NCGMSnoopWnd, SnoopID, 0xF0);
NW_ASSERT_SIZE(NCGMSnoopWnd, 0xF4);

#pragma pack(pop)
