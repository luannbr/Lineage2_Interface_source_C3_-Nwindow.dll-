#pragma once
// NCGMSnoopManager - the GM's snoop windows (UClass "CGMSnoopManager", 0xA8; C2 only;
// NCConsole makes it): four NCGMSnoopWnd, one per conversation listened to, all hidden until
// the console shows one of them.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCGMSnoopWnd;

class NCGMSnoopManager : public NCWnd
{
    NW_DECLARE_CLASS(NCGMSnoopManager, NCWnd, 0, 0x1029D298)

    NCGMSnoopWnd* Snoops[4];         // +0xC8 (name ours)

    NCGMSnoopManager();                                              // 0x10056720
    // ~NCGMSnoopManager                                             // 0x1004c6f0 (C1 only: no C2 match)

    virtual INT OnCreate();                                          // 37 0x100d3b00
    virtual INT OnDestroy();                                         // 38 0x100d1190
    virtual void ShowWindow();                                       // 44 0x100d0680

    // names ours (the console's Snoop calls them)
    void AddLine(INT SnoopID, const TCHAR* Text, DWORD Color, INT Type, INT Unused); // 0x100d1250
    void OpenSnoop(INT SnoopID, const TCHAR* TargetName);            // 0x100d12b0
};
NW_ASSERT_OFFSET(NCGMSnoopManager, Snoops, 0xC8);
NW_ASSERT_SIZE(NCGMSnoopManager, 0xD8);

#pragma pack(pop)
